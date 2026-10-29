#pragma once
#include <amstream.h>
#include <cstddef>
#include <cstdint>
#include <ddraw.h>
#include <ddstream.h>
#include <Windows.h>

namespace NXSys2DApp
{
	class CDisplay;
}

namespace NWindowsVideo
{
	// The original class name is unresolved; fields follow verified COM identities.
	class CVideoPlayer
	{
	  public:
		CVideoPlayer();
		~CVideoPlayer();
		CVideoPlayer(const CVideoPlayer&) = delete;
		CVideoPlayer& operator=(const CVideoPlayer&) = delete;
		bool Open(const char* fileName, bool enableAudio);
		void Close();
		void Start();
		void Stop();
		bool Update(NXSys2DApp::CDisplay* display);

		bool IsPlaying() const
		{

			return mPlaying != 0;
		}

		inline static CVideoPlayer* sTheObjectPtr = nullptr;

	  private:
		bool l_CheckAvailability();
		IDirectDraw* mDirectDraw;
		IAMMultiMediaStream* mMultiMediaStream;
		IMediaStream* mMediaStream;
		IDirectDrawMediaStream* mDrawMediaStream;
		IDirectDrawSurface* mSurface;
		IDirectDrawStreamSample* mSample;
		RECT mSourceRectangle;
		std::uint8_t mAvailable;
		std::uint8_t mPlaying;
		std::byte mPadding[2];
	};

	// All members are native COM pointers or fixed-width scalar/RECT fields.
	static_assert(sizeof(CVideoPlayer) == (sizeof(void*) == 8 ? 72 : 44));
}