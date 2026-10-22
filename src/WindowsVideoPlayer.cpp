#include "WindowsVideoPlayer.h"
#include "BinkVideoDecoder.h"
#include "GameProfile.h"
#include "XFile.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XCrtFileAccess.h"
#include <cstring>
#include <memory>
#include <new>

namespace NWindowsVideo
{
	// Native, nonserialized backend for the existing singleton owner.
	static CVideoPlayer* gBinkOwner = nullptr;
	static std::unique_ptr<CBinkVideoDecoder> gBink;

	// Complete seven-GUID Windows region 004FC47C..004FC4EB.
	static const GUID gFilterGraphInterface = { 0x56A868A9, 0x0AD4, 0x11CE,{ 0xB0, 0x3A, 0, 0x20, 0xAF, 0x0B, 0xA7, 0x70 } };
	static const GUID gDrawMediaStreamInterface = { 0xF4104FCE, 0x9A70, 0x11D0,{ 0x8F, 0xDE, 0, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };
	static const GUID gMultiMediaStreamClass = { 0x49C47CE5, 0x9BA4, 0x11D0,{ 0x82, 0x12, 0, 0xC0, 0x4F, 0xC3, 0x2C, 0x45 } };
	static const GUID gMultiMediaStreamInterface = { 0xBEBE595C, 0x9A6F, 0x11D0,{ 0x8F, 0xDE, 0, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };
	static const GUID gFilterGraphClass = { 0xE436EBB3, 0x524F, 0x11CE,{ 0x9F, 0x53, 0, 0x20, 0xAF, 0x0B, 0xA7, 0x70 } };
	static const GUID gPrimaryAudioPurpose = { 0xA35FF56B, 0x9FDA, 0x11D0,{ 0x8F, 0xDF, 0, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };
	static const GUID gPrimaryVideoPurpose = { 0xA35FF56A, 0x9FDA, 0x11D0,{ 0x8F, 0xDF, 0, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };

	// Original function: FUN_00404452.
	CVideoPlayer::CVideoPlayer()
	{
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_0040446c.
	CVideoPlayer::~CVideoPlayer()
	{
		Stop();
		Close();
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004046da. Only S_OK accepts the probe.
	bool CVideoPlayer::l_CheckAvailability()
	{
		if (FAILED(CoInitialize(nullptr))) return false;
		IUnknown* graph = nullptr;
		if (CoCreateInstance(gFilterGraphClass, nullptr, CLSCTX_INPROC_SERVER, gFilterGraphInterface, reinterpret_cast<void**>(&graph)) == S_OK)
		{
			graph->Release();
			mAvailable = 1;
			return true;
		}
		CoUninitialize();
		mAvailable = 0;
		return false;
	}

	// Original function: FUN_00404484.
	bool CVideoPlayer::Open(const char* fileName, bool enableAudio)
	{
		Stop();
		Close();
		if (fileName == nullptr) return false;
		const char* extension = std::strrchr(fileName, '.');
		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2 && extension != nullptr && NXBasics::Runtime_CompareStringsIgnoreCase(extension, ".bik") == 0)
		{
			char resolvedFile[MAX_PATH];
			if (!NXBasics::CFile::FileSystem_Tool_GetRealFilename(fileName, resolvedFile)) return false;
			auto backend = std::unique_ptr<CBinkVideoDecoder>(new (std::nothrow) CBinkVideoDecoder());
			if (!backend || !backend->Open(resolvedFile, enableAudio)) return false;
			gBink = std::move(backend);
			gBinkOwner = this;
			return true;
		}
		if (!l_CheckAvailability()) return false;
		char resolvedFile[MAX_PATH];
		if (!NXBasics::CFile::FileSystem_Tool_GetRealFilename(fileName, resolvedFile)) { Close(); return false; }
		IAMMultiMediaStream* stream = nullptr;
		// Decoder-only DirectDraw object: no primary surface, exclusive mode or mode switch.
		HRESULT result = DirectDrawCreate(nullptr, &mDirectDraw, nullptr);
		if (SUCCEEDED(result)) result = mDirectDraw->SetCooperativeLevel(NXSys2DApp::CApplication::sTheObjectPtr->GetWindowHandle(), DDSCL_NORMAL);
		if (SUCCEEDED(result)) result = CoCreateInstance(gMultiMediaStreamClass, nullptr, CLSCTX_INPROC_SERVER, gMultiMediaStreamInterface, reinterpret_cast<void**>(&stream));
		mMultiMediaStream = stream;
		if (SUCCEEDED(result)) result = stream->Initialize(STREAMTYPE_READ, 1, nullptr);
		if (SUCCEEDED(result))
		{
			result = stream->AddMediaStream(mDirectDraw, &gPrimaryVideoPurpose, 0, nullptr);
		}
		if (SUCCEEDED(result))
		{
			if (enableAudio) stream->AddMediaStream(nullptr, &gPrimaryAudioPurpose, 1, nullptr);
			WCHAR wideFile[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, resolvedFile, -1, wideFile, MAX_PATH);
			result = stream->OpenFile(wideFile, 0);
		}
		if (SUCCEEDED(result))
		{
			result = stream->GetMediaStream(gPrimaryVideoPurpose, &mMediaStream);
		}
		if (SUCCEEDED(result)) result = mMediaStream->QueryInterface(gDrawMediaStreamInterface, reinterpret_cast<void**>(&mDrawMediaStream));
		if (SUCCEEDED(result)) result = mDrawMediaStream->CreateSample(nullptr, nullptr, 0, &mSample);
		if (SUCCEEDED(result)) result = mSample->GetSurface(&mSurface, &mSourceRectangle);
		if (SUCCEEDED(result))
		{
			// The shipped 640 x 480 intro contains a 640 x 400 picture with an
			// encoded 40-row matte at each edge. The mission film uses all 480 rows.
			const char* baseName = NXBasics::Runtime_FindLastCharacter(fileName, '\\');
			baseName = baseName != nullptr ? baseName + 1 : fileName;
			if (NXBasics::Runtime_CompareStringsIgnoreCase(baseName, "intro.mpg") == 0 && mSourceRectangle.right - mSourceRectangle.left == 640 && mSourceRectangle.bottom - mSourceRectangle.top == 480)
			{
				mSourceRectangle.top += 40;
				mSourceRectangle.bottom -= 40;
			}
		}
		if (SUCCEEDED(result)) return true;
		// Remaster cleanup also releases partially opened decoder resources.
		Close();
		mAvailable = 0;
		return false;
	}

	// Original function: FUN_004045cb; remaster also stops playback and balances COM.
	void CVideoPlayer::Close()
	{
		Stop();
		if (gBinkOwner == this) { gBink.reset(); gBinkOwner = nullptr; }
		mPlaying = 0;
		if (mSurface != nullptr)
		{
			mSurface->Release(); mSurface = nullptr;
		}
		if (mSample != nullptr)
		{
			mSample->Release(); mSample = nullptr;
		}
		if (mDrawMediaStream != nullptr)
		{
			mDrawMediaStream->Release(); mDrawMediaStream = nullptr;
		}
		if (mMediaStream != nullptr)
		{
			mMediaStream->Release(); mMediaStream = nullptr;
		}
		if (mMultiMediaStream != nullptr)
		{
			mMultiMediaStream->Release(); mMultiMediaStream = nullptr;
		}
		if (mDirectDraw != nullptr)
		{
			mDirectDraw->Release(); mDirectDraw = nullptr;
		}
		if (mAvailable != 0) CoUninitialize();
		mAvailable = 0;
	}

	// Original function: FUN_00404635; remaster rejects unavailable/failed streams.
	void CVideoPlayer::Start()
	{
		if (gBinkOwner == this && gBink)
		{
			mPlaying = gBink->Start() ? 1 : 0;
			return;
		}
		if (mAvailable != 0 && mMultiMediaStream != nullptr)
		{
			if (SUCCEEDED(mMultiMediaStream->Seek(0)) && SUCCEEDED(mMultiMediaStream->SetState(STREAMSTATE_RUN))) mPlaying = 1;
		}
	}

	// Original function: FUN_0040465c.
	void CVideoPlayer::Stop()
	{
		if (gBinkOwner == this && gBink)
		{
			gBink->Stop();
			mPlaying = 0;
			return;
		}
		if (mAvailable != 0 && mPlaying != 0)
		{
			mMultiMediaStream->SetState(STREAMSTATE_STOP);
			mPlaying = 0;
		}
	}

	// Original function: FUN_0040467c. Sample update requires exact S_OK.
	bool CVideoPlayer::Update(NXSys2DApp::CDisplay* display)
	{
		if (gBinkOwner == this && gBink)
		{
			if (mPlaying == 0 || display == nullptr) return false;
			const bool updated = gBink->Update(display);
			mPlaying = gBink->IsPlaying() ? 1 : 0;
			return updated;
		}
		if (mAvailable == 0 || display == nullptr) return false;
		if (mSample->Update(0, nullptr, nullptr, 0) != S_OK)
		{
			Stop();
			return false;
		}
		DDSURFACEDESC frame{ };
		frame.dwSize = sizeof(frame);
		if (FAILED(mSurface->Lock(&mSourceRectangle, &frame, DDLOCK_WAIT | DDLOCK_READONLY, nullptr))) { Stop(); return false; }
		const auto& format = frame.ddpfPixelFormat;
		const bool updated = (format.dwFlags & DDPF_RGB) != 0 && display->VideoFrameToScreen(frame.lpSurface, frame.lPitch, mSourceRectangle.right - mSourceRectangle.left, mSourceRectangle.bottom - mSourceRectangle.top, format.dwRGBBitCount, format.dwRBitMask, format.dwGBitMask, format.dwBBitMask);
		mSurface->Unlock(frame.lpSurface);
		if (!updated) Stop();
		return updated;
	}
}
