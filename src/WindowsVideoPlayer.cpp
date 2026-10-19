#include "WindowsVideoPlayer.h"
#include "GameProfile.h"
#include "XFile.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XCrtFileAccess.h"
#include <cstring>

namespace NWindowsVideo
{
	// C2 00404C90..00404CDE establishes the four public Bink header fields.
	// The proprietary decoder remains in the installation's existing x86 DLL.
	struct SBinkHeader
	{
		std::uint32_t width, height, frames, frameNumber;
	};
	static_assert(sizeof(SBinkHeader) == 16);
	using BinkOpenFn = SBinkHeader* (__stdcall*)(const char*, std::uint32_t);
	using BinkHandleFn = void (__stdcall*)(SBinkHeader*);
	using BinkWaitFn = std::int32_t (__stdcall*)(SBinkHeader*);
	using BinkCopyFn = void (__stdcall*)(SBinkHeader*, void*, std::int32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	using BinkSwitchFn = std::int32_t (__stdcall*)(SBinkHeader*, std::int32_t);
	using BinkGotoFn = void (__stdcall*)(SBinkHeader*, std::uint32_t, std::int32_t);
	using BinkSoundOpenFn = void* (__stdcall*)(std::uintptr_t);
	using BinkSoundSystemFn = std::int32_t (__stdcall*)(BinkSoundOpenFn, std::uintptr_t);
	struct SBinkPlayback
	{
		CVideoPlayer* owner = nullptr;
		HMODULE module = nullptr;
		SBinkHeader* movie = nullptr;
		void* pixels = nullptr;
		BinkOpenFn open = nullptr;
		BinkHandleFn close = nullptr, decode = nullptr, next = nullptr;
		BinkWaitFn wait = nullptr;
		BinkCopyFn copy = nullptr;
		BinkSwitchFn pause = nullptr, sound = nullptr;
		BinkGotoFn goTo = nullptr;
		bool started = false;
	};
	// Nonserialized state for the existing singleton video owner (44 bytes).
	static SBinkPlayback gBink;

	static void CloseBinkPlayback()
	{
		if (gBink.movie != nullptr) gBink.close(gBink.movie);
		NXBasics::Runtime_Free(gBink.pixels);
		if (gBink.module != nullptr) FreeLibrary(gBink.module);
		gBink = {};
	}

	static bool OpenBinkPlayback(CVideoPlayer* owner, const char* fileName, bool enableAudio)
	{
		if (gBink.owner != nullptr) return false;
		char dllPath[MAX_PATH];
		const DWORD length = GetFullPathNameA("binkw32.dll", MAX_PATH, dllPath, nullptr);
		if (length == 0 || length >= MAX_PATH) return false;
		gBink.owner = owner;
		gBink.module = LoadLibraryA(dllPath);
		if (gBink.module == nullptr) { OutputDebugStringA("OpenVikings: could not load the installation's binkw32.dll.\n"); CloseBinkPlayback(); return false; }
		gBink.open = reinterpret_cast<BinkOpenFn>(GetProcAddress(gBink.module, "_BinkOpen@8"));
		gBink.close = reinterpret_cast<BinkHandleFn>(GetProcAddress(gBink.module, "_BinkClose@4"));
		gBink.decode = reinterpret_cast<BinkHandleFn>(GetProcAddress(gBink.module, "_BinkDoFrame@4"));
		gBink.next = reinterpret_cast<BinkHandleFn>(GetProcAddress(gBink.module, "_BinkNextFrame@4"));
		gBink.wait = reinterpret_cast<BinkWaitFn>(GetProcAddress(gBink.module, "_BinkWait@4"));
		gBink.copy = reinterpret_cast<BinkCopyFn>(GetProcAddress(gBink.module, "_BinkCopyToBuffer@28"));
		gBink.pause = reinterpret_cast<BinkSwitchFn>(GetProcAddress(gBink.module, "_BinkPause@8"));
		gBink.sound = reinterpret_cast<BinkSwitchFn>(GetProcAddress(gBink.module, "_BinkSetSoundOnOff@8"));
		gBink.goTo = reinterpret_cast<BinkGotoFn>(GetProcAddress(gBink.module, "_BinkGoto@12"));
		const auto setSoundSystem = reinterpret_cast<BinkSoundSystemFn>(GetProcAddress(gBink.module, "_BinkSetSoundSystem@8"));
		const auto openDirectSound = reinterpret_cast<BinkSoundOpenFn>(GetProcAddress(gBink.module, "_BinkOpenDirectSound@4"));
		if (gBink.open == nullptr || gBink.close == nullptr || gBink.decode == nullptr || gBink.next == nullptr || gBink.wait == nullptr || gBink.copy == nullptr || gBink.pause == nullptr || gBink.sound == nullptr || gBink.goTo == nullptr || setSoundSystem == nullptr || openDirectSound == nullptr) { OutputDebugStringA("OpenVikings: the Bink DLL is missing a required export.\n"); CloseBinkPlayback(); return false; }
		// Remaster: use the DLL's own DirectSound backend, without a Miles handle.
		const bool soundReady = setSoundSystem(openDirectSound, 0) != 0;
		if (enableAudio && !soundReady) OutputDebugStringA("OpenVikings: Bink audio unavailable; trying silent playback.\n");
		gBink.movie = gBink.open(fileName, 0);
		if (gBink.movie == nullptr) { OutputDebugStringA("OpenVikings: Bink could not open the movie.\n"); CloseBinkPlayback(); return false; }
		const auto& movie = *gBink.movie;
		if (movie.frames == 0 || !NXSys2DApp::CDisplay::IsSupportedMode(movie.width, movie.height, 32)) { CloseBinkPlayback(); return false; }
		gBink.pixels = NXBasics::Runtime_Allocate(static_cast<std::size_t>(movie.width) * movie.height * 4);
		if (gBink.pixels == nullptr) { CloseBinkPlayback(); return false; }
		gBink.sound(gBink.movie, enableAudio && soundReady ? 1 : 0);
		gBink.pause(gBink.movie, 1);
		return true;
	}
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
			return NXBasics::CFile::FileSystem_Tool_GetRealFilename(fileName, resolvedFile) && OpenBinkPlayback(this, resolvedFile, enableAudio);
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
		if (gBink.owner == this) CloseBinkPlayback();
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
		if (gBink.owner == this && gBink.movie != nullptr)
		{
			if (gBink.started) gBink.goTo(gBink.movie, 1, 0);
			gBink.pause(gBink.movie, 0);
			gBink.started = true;
			mPlaying = 1;
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
		if (gBink.owner == this && gBink.movie != nullptr)
		{
			gBink.pause(gBink.movie, 1);
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
		if (gBink.owner == this && gBink.movie != nullptr)
		{
			if (mPlaying == 0 || display == nullptr) return false;
			bool decoded = false, finished = false;
			// C2 00404C77..00404CA4 bounds catch-up to ten decoder frames.
			for (unsigned frame = 0; frame < 10 && gBink.wait(gBink.movie) == 0; ++frame)
			{
				gBink.decode(gBink.movie);
				decoded = true;
				// Remaster presents the final frame rather than C2's Frames - 1 cutoff.
				if (gBink.movie->frameNumber >= gBink.movie->frames) { finished = true; break; }
				gBink.next(gBink.movie);
			}
			if (!decoded) return true;
			const auto width = gBink.movie->width, height = gBink.movie->height;
			// Surface format 3 is the native C2 32-bit copy path at 00404CB2.
			gBink.copy(gBink.movie, gBink.pixels, static_cast<std::int32_t>(width * 4), height, 0, 0, 3);
			const bool updated = display->VideoFrameToScreen(gBink.pixels, static_cast<LONG>(width * 4), static_cast<std::int32_t>(width), static_cast<std::int32_t>(height), 32, 0x00FF0000, 0x0000FF00, 0x000000FF);
			if (!updated || finished) Stop();
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
