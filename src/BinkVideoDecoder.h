#pragma once
#include <memory>

namespace NXSys2DApp
{
	class CDisplay;
}

namespace NWindowsVideo
{
	// Native FFmpeg backend; owns only transient playback resources.
	class CBinkVideoDecoder
	{
	  public:
		CBinkVideoDecoder();
		~CBinkVideoDecoder();
		CBinkVideoDecoder(const CBinkVideoDecoder&) = delete;
		CBinkVideoDecoder& operator=(const CBinkVideoDecoder&) = delete;
		bool Open(const char* fileName, bool enableAudio);
		bool Start();
		void Stop();
		bool Update(NXSys2DApp::CDisplay* display);
		bool IsPlaying() const;

	  private:
		struct SPlayback;
		std::unique_ptr<SPlayback> mPlayback;
	};
}