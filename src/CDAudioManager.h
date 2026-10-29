#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace NCDAudioManager
{
	class CCDAudioManager
	{
	public:
		CCDAudioManager();
		~CCDAudioManager();
		CCDAudioManager(const CCDAudioManager&) = delete;
		CCDAudioManager& operator=(const CCDAudioManager&) = delete;
		bool Init(char driveLetter);
		bool Exit();
		bool StartTrack(std::uint32_t track);
		bool StopTrack();
		bool IsStillPlaying();
		bool CheckLoopTrack();
		std::uint32_t NumberOfTracks();
		std::uint32_t GetVolume();
		bool SetVolume(std::uint32_t volume);
		std::uint32_t GetCurrentTrack();

		// Original data: 0050f69c; Mac symbol: CCDAudioManager::sTheObjectPtr.
		inline static CCDAudioManager* sTheObjectPtr = nullptr;

	private:
		void l_RecordMciError(MCIERROR error);
		bool l_InitializeMixer();

		std::uint8_t mInitialized;                  // +0x000
		std::uint8_t mPlaying;                      // +0x001
		std::byte mPadding02[2];                    // +0x002
		MCIDEVICEID mDeviceId;                      // +0x004
		std::uint32_t mCurrentTrack;                // +0x008
		char mLastError[300];                       // +0x00C
		HMIXER mMixer;                             // +0x138
		MIXERCONTROLDETAILS mVolumeDetails;         // +0x13C
		MIXERCONTROLDETAILS_UNSIGNED mVolume;       // +0x154
		std::uint32_t mSavedVolume;                 // +0x158
		std::uint8_t mPlaylistEnabled;              // +0x15C
		std::byte mPadding15D[3];                   // +0x15D
		std::uint32_t mPlaylistIndex;               // +0x160
		std::uint32_t mPlaylistTracks[30];          // +0x164
		void(__cdecl* mTrackEndedCallback)();      // +0x1DC; original callback name unresolved.
	};

	static_assert(sizeof(MIXERCONTROLDETAILS) == (sizeof(void*) == 8 ? 32 : 24));
	static_assert(sizeof(MIXERCONTROLDETAILS_UNSIGNED) == 4);
	static_assert(sizeof(CCDAudioManager) == (sizeof(void*) == 8 ? 496 : 480));
}
