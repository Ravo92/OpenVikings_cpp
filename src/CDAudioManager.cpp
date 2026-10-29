#include "CDAudioManager.h"
#include "OriginalStrings.h"
#include "XCrtFileAccess.h"

#include <cstring>
#include <cstdio>

namespace NCDAudioManager
{
	// Original function: FUN_00403e45
	CCDAudioManager::CCDAudioManager()
	{
		static_assert(offsetof(CCDAudioManager, mDeviceId) == 0x004);
		static_assert(offsetof(CCDAudioManager, mCurrentTrack) == 0x008);
		static_assert(offsetof(CCDAudioManager, mLastError) == 0x00C);
		static_assert(offsetof(CCDAudioManager, mMixer) == 0x138);
		static_assert(offsetof(CCDAudioManager, mVolumeDetails) == (sizeof(void*) == 8 ? 320 : 0x13C));
		static_assert(offsetof(CCDAudioManager, mVolume) == (sizeof(void*) == 8 ? 352 : 0x154));
		static_assert(offsetof(CCDAudioManager, mSavedVolume) == (sizeof(void*) == 8 ? 356 : 0x158));
		static_assert(offsetof(CCDAudioManager, mPlaylistEnabled) == (sizeof(void*) == 8 ? 360 : 0x15C));
		static_assert(offsetof(CCDAudioManager, mPlaylistIndex) == (sizeof(void*) == 8 ? 364 : 0x160));
		static_assert(offsetof(CCDAudioManager, mPlaylistTracks) == (sizeof(void*) == 8 ? 368 : 0x164));
		static_assert(offsetof(CCDAudioManager, mTrackEndedCallback) == (sizeof(void*) == 8 ? 488 : 0x1DC));
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_00403e62
	CCDAudioManager::~CCDAudioManager()
	{
		Exit();
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_00403e6f
	bool CCDAudioManager::Init(char driveLetter)
	{
		if (mInitialized != 0)
			return true;

		MCI_OPEN_PARMSA parameters{ };
		parameters.lpstrDeviceType = MAKEINTRESOURCEA(MCI_DEVTYPE_CD_AUDIO);
		char driveName[12];
		DWORD flags = MCI_OPEN_TYPE_ID | MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE;

		if (driveLetter != 0)
		{
			NXBasics::Runtime_PrintString(driveName, OriginalStrings::CDAudioDriveNameFormat, static_cast<int>(static_cast<std::int8_t>(driveLetter)));
			parameters.lpstrElementName = driveName;
			flags |= MCI_OPEN_ELEMENT;
		}

		const MCIERROR error = ::mciSendCommandA(0, MCI_OPEN, flags, reinterpret_cast<DWORD_PTR>(&parameters));

		if (error != 0)
		{
			l_RecordMciError(error);
			return false;
		}

		mDeviceId = parameters.wDeviceID;
		l_InitializeMixer();
		mSavedVolume = mVolume.dwValue;
		mInitialized = 1;
		mPlaying = 1;
		StopTrack();

		return true;
	}

	// Original function: FUN_00403f19
	bool CCDAudioManager::Exit()
	{
		if (mInitialized == 0)
			return true;

		if (IsStillPlaying())
			StopTrack();

		SetVolume(mSavedVolume);

		if (mMixer != nullptr)
		{
			::mixerClose(mMixer);
			mMixer = nullptr;
		}

		::mciSendCommandA(mDeviceId, MCI_CLOSE, 0, 0);
		std::memset(this, 0, sizeof(*this));

		return true;
	}

	// Original function: FUN_00403f85
	bool CCDAudioManager::StartTrack(std::uint32_t track)
	{
		if (!Init(0))
			return false;

		MCI_SET_PARMS timeFormat{ };
		timeFormat.dwTimeFormat = MCI_FORMAT_TMSF;
		MCIERROR error = ::mciSendCommandA(mDeviceId, MCI_SET, MCI_SET_TIME_FORMAT, reinterpret_cast<DWORD_PTR>(&timeFormat));

		if (error == 0)
		{
			MCI_PLAY_PARMS play{ };
			play.dwFrom = static_cast<std::uint8_t>(track);
			DWORD flags = MCI_FROM;

			if (track != NumberOfTracks())
			{
				play.dwTo = static_cast<std::uint8_t>(track + 1u);
				flags |= MCI_TO;
			}

			error = ::mciSendCommandA(mDeviceId, MCI_PLAY, flags, reinterpret_cast<DWORD_PTR>(&play));
		}

		if (error != 0)
		{
			l_RecordMciError(error);
			Exit();
			return false;
		}

		mPlaying = 1;
		mCurrentTrack = track;

		return true;
	}

	// Original function: FUN_0040401e
	bool CCDAudioManager::StopTrack()
	{
		mCurrentTrack = 0;

		if (!Init(0))
			return false;

		if (!IsStillPlaying())
			return true;

		MCI_GENERIC_PARMS parameters{ };
		const MCIERROR error = ::mciSendCommandA(mDeviceId, MCI_STOP, MCI_WAIT, reinterpret_cast<DWORD_PTR>(&parameters));

		if (error != 0)
		{
			l_RecordMciError(error);
			return false;
		}

		mPlaying = 0;

		return true;
	}

	// Original function: FUN_00404070
	bool CCDAudioManager::IsStillPlaying()
	{
		if (mPlaying == 0 || !Init(0))
			return false;

		MCI_STATUS_PARMS parameters{ };
		parameters.dwItem = MCI_STATUS_MODE;
		const MCIERROR error = ::mciSendCommandA(mDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&parameters));

		if (error != 0)
		{
			l_RecordMciError(error);
			return false;
		}

		mPlaying = static_cast<std::uint8_t>(parameters.dwReturn == MCI_MODE_PLAY);
		// Every successful status query clears the Windows playlist state.
		mPlaylistEnabled = 0;
		mPlaylistIndex = 0;
		std::memset(mPlaylistTracks, 0, sizeof(mPlaylistTracks));

		return mPlaying != 0;
	}

	// Original function: FUN_004040eb
	bool CCDAudioManager::CheckLoopTrack()
	{
		if (mCurrentTrack == 0 || !Init(0) || IsStillPlaying())
			return false;

		if (mTrackEndedCallback != nullptr)
			mTrackEndedCallback();

		if (mPlaylistEnabled != 0)
		{
			++mPlaylistIndex;

			if (mPlaylistIndex == 30u)
				mPlaylistIndex = 0;

			for (std::uint32_t index = mPlaylistIndex; index < 30u; ++index)
			{
				if (mPlaylistTracks[index] > 1u)
				{
					const std::uint32_t trackCount = NumberOfTracks();
					// Re-read the entry after NumberOfTracks, which can initialize the device.

					if (mPlaylistTracks[index] < trackCount)
					{
						mPlaylistIndex = index;
						StartTrack(mPlaylistTracks[index]);
						break;
					}
				}
			}
			// The playlist path ignores StartTrack's result and returns false.
			return false;
		}

		return StartTrack(mCurrentTrack);
	}

	// Original function: FUN_00404196
	std::uint32_t CCDAudioManager::NumberOfTracks()
	{
		if (!Init(0))
			return 0;

		MCI_STATUS_PARMS parameters{ };
		parameters.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
		const MCIERROR error = ::mciSendCommandA(mDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&parameters));

		if (error != 0)
		{
			l_RecordMciError(error);
			return 0;
		}

		return static_cast<std::uint32_t>(parameters.dwReturn);
	}

	// Original function: FUN_004041de
	std::uint32_t CCDAudioManager::GetVolume()
	{
		if (!Init(0) || mMixer == nullptr)
			return 0;

		if (::mixerGetControlDetailsA(reinterpret_cast<HMIXEROBJ>(mMixer), &mVolumeDetails, 0) != MMSYSERR_NOERROR)
			return 0;

		return mVolume.dwValue;
	}

	// Original function: FUN_00404216
	bool CCDAudioManager::SetVolume(std::uint32_t volume)
	{
		if (!Init(0) || mMixer == nullptr)
			return false;

		mVolume.dwValue = volume;

		return ::mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(mMixer), &mVolumeDetails, 0) == MMSYSERR_NOERROR;
	}

	// Original function: FUN_00404256
	std::uint32_t CCDAudioManager::GetCurrentTrack()
	{
		if (!IsStillPlaying())
			return 0;

		return mPlaylistEnabled != 0 ? mPlaylistTracks[mPlaylistIndex] : mCurrentTrack;
	}

	// Original function: FUN_00404283; Windows-only helper name inferred from its writes.
	void CCDAudioManager::l_RecordMciError(MCIERROR error)
	{
		char text[200]{ };

		if (!::mciGetErrorStringA(error, text, sizeof(text)))
			std::snprintf(text, sizeof(text), "MCI error %lu", static_cast<unsigned long>(error));

		std::snprintf(mLastError, sizeof(mLastError), "%s%s", OriginalStrings::CDAudioErrorPrefix, text);
	}

	// Original function: FUN_004042c7; Windows-only mixer helper.
	bool CCDAudioManager::l_InitializeMixer()
	{
		static_assert(sizeof(MIXERLINEA) == (sizeof(void*) == 8 ? 172 : 168));
		static_assert(sizeof(MIXERCONTROLA) == 148);
		static_assert(sizeof(MIXERLINECONTROLSA) == (sizeof(void*) == 8 ? 28 : 24));
		bool found = false;

		for (std::uint32_t device = 0; device < ::mixerGetNumDevs(); ++device)
		{
			HMIXER candidate = nullptr;

			if (::mixerOpen(&candidate, device, 0, 0, 0) != MMSYSERR_NOERROR)
				continue;

			MIXERCONTROLDETAILS details{ };
			MIXERCONTROLDETAILS_UNSIGNED volume{ };
			const auto query = [&]() -> bool
			{
				MIXERLINEA line{ };
				line.cbStruct = sizeof(line);
				line.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

				if (::mixerGetLineInfoA(reinterpret_cast<HMIXEROBJ>(candidate), &line, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
				{
					line.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY;

					if (::mixerGetLineInfoA(reinterpret_cast<HMIXEROBJ>(candidate), &line, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
						return false;
				}

				MIXERCONTROLA control{ };
				MIXERLINECONTROLSA controls{ };
				controls.cbStruct = sizeof(controls);
				controls.dwLineID = line.dwLineID;
				controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				controls.cControls = 1;
				controls.cbmxctrl = sizeof(control);
				controls.pamxctrl = &control;

				if (::mixerGetLineControlsA(reinterpret_cast<HMIXEROBJ>(candidate), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
					return false;

				details.cbStruct = sizeof(details);
				details.dwControlID = control.dwControlID;
				details.cChannels = 1;
				details.cbDetails = sizeof(volume);
				details.paDetails = &volume;

				return ::mixerGetControlDetailsA(reinterpret_cast<HMIXEROBJ>(candidate), &details, 0) == MMSYSERR_NOERROR;
			};

			if (!query())
			{
				::mixerClose(candidate);
				continue;
			}
			// Remaster: preserve last-usable selection without losing earlier handles.

			if (mMixer != nullptr)
				::mixerClose(mMixer);

			mMixer = candidate;
			mVolume = volume;
			mVolumeDetails = details;
			mVolumeDetails.paDetails = &mVolume;
			found = true;
		}

		return found;
	}
}