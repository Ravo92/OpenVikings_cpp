#pragma once

#include <cstdint>

namespace NC2E2
{
	// Windows speech storage; response IDs refer to the static sound-FX owner.
	struct SHumanSoundResponseGroupStorage
	{
		std::uint32_t count, soundIds[15];
	};

	struct SHumanSoundRecordStorage
	{
		std::uint32_t seenHumanCount, screamSoundId, genericSoundId;
		SHumanSoundResponseGroupStorage responses[2];
	};

	struct SHumanSoundManagerStorage
	{
		std::uint32_t lastGenericSoundTick;
		SHumanSoundRecordStorage records[24];
	};

	void E2HumanSoundManager_ClearSeenHumanCounter(void* manager);
	void E2HumanSoundManager_IncreaseSeenCounter(void* manager, const void* human);
	void E2HumanSoundManager_PlayGenericSounds(void* manager, const void* display);
	void E2HumanSoundManager_PlayHumanScreamSound(const void* manager, const void* human);
	void Tool_Sound_PlaySoundOrientedOnDisplay(const void* point, std::uint32_t soundId, const void* display, bool skipRunning, bool attenuate);
	void E2HumanSoundManager_PlayRespondingSound(const void* manager, std::uint32_t responseType, const void* human, const void* display);
	void E2HumanSoundManager_PlayRespondingSound(const void* manager, std::uint32_t responseType, std::int32_t humanId);
}