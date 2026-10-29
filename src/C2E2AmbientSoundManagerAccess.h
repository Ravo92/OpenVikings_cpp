#pragma once
#include "C2MapSectorGridCursor.h"

namespace NC2E2
{
	// Complete physical Windows records; the raw access code preserves original lifetimes.
	struct SAmbientLoopEntryStorage
	{
		std::uint32_t ambientId, volume;
	};

	struct SAmbientLoopListStorage
	{
		std::int32_t count;
		SAmbientLoopEntryStorage entries[10];
	};

	struct SAmbientSectorStorage
	{
		std::int32_t patternCount;
		SAmbientLoopEntryStorage patterns[3];
		std::int32_t landscapeCount;
		SAmbientLoopEntryStorage landscapes[3];
	};

	struct SAmbientSoundManagerStorage
	{
		std::int32_t sectorWidth, sectorHeight;
		SAmbientSectorStorage* sectors;
		SAmbientLoopListStorage activeLoops;
		std::uint32_t lastLandscapeSoundTick;
	};

	static_assert(sizeof(SAmbientSoundManagerStorage) == (sizeof(void*) == 8 ? 104 : 100));
	static_assert(offsetof(SAmbientSoundManagerStorage, activeLoops) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(offsetof(SAmbientSoundManagerStorage, lastLandscapeSoundTick) == (sizeof(void*) == 8 ? 100 : 96));

	void* __fastcall AmbientSoundManager_Construct(void* manager, void* unusedEdx);
	void AmbientSoundManager_AllocateMemory(void* manager);
	void AmbientSoundManager_RecalculateAmbientSoundSector(void* manager, const NC2Logic::SSectorGridCursor* sector, bool patterns, bool landscapes);
	void AmbientSoundManager_RecalculateAllAmbientSoundSector(void* manager);
	void AmbientSoundManager_UpdateAmbientSounds(void* manager, const void* display);
	void AmbientSoundManager_StopAllAmbientSounds(void* manager);
}