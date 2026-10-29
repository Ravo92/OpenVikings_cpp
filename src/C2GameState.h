#pragma once

#include "C2MapGuid.h"
#include "XRandom.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
#pragma pack(push, 4)
	// Complete clock region 00510838..0051089F, cleared by GameTime_Reset.
	struct SGameTimeState
	{
		std::uint32_t tick;
		std::byte unknown004[4];
		double lastRealTime;
		double speed;
		double tickDuration;
		double retainedSpeed;
		std::uint32_t networkBaseTick;
		std::uint32_t networkBaseRealTime;
		std::uint32_t networkFrequency;
		std::uint32_t latencySampleTime;
		std::uint32_t latencySampleIndex;
		std::uint32_t latencySamples[10];
		std::uint8_t autoAdjustNetworkFrequency;
		std::uint8_t maximizeSpeed;
		std::byte unknown066[2];
	};
	static_assert(sizeof(SGameTimeState) == 104);
	static_assert(offsetof(SGameTimeState, lastRealTime) == 8);
	static_assert(offsetof(SGameTimeState, networkFrequency) == 0x30);
	static_assert(offsetof(SGameTimeState, latencySamples) == 0x3C);
	static_assert(offsetof(SGameTimeState, maximizeSpeed) == 0x65);

	// Complete Windows autosave region 005108A0..005108AB.
	struct SGameAutosaveState
	{
		std::uint32_t intervalTicks;
		std::uint32_t lastSaveTick;
		std::uint32_t sequence;
	};
	static_assert(sizeof(SGameAutosaveState) == 12);


#pragma pack(pop)

	// Complete Windows CGame::mStaticVars region, 00510818..005109CF.
	struct SGameStaticVars
	{
		unsigned char initialized;
		std::byte unknown001[7];
		SCleanMapGuid mapGuid;                   // +0x008; original 00510820
		std::uint32_t mapType;                   // +0x018; original 00510830, misc_maptype/maptype.
		std::byte unknown01C[4];
		SGameTimeState gameTime;
		SGameAutosaveState autosave;
		SGameRandomState random;
		std::uint32_t uniqueIdCounter;
		char retainedMapRoot[260];                // +0x0A0; copied when retaining the map INI.
		NXBasics::CIniFileReader* retainedMapIni;    // +0x1A4; original 005109BC, owned until game shutdown.
		void* mapFileLibrary;                      // +0x1A8; original 005109C0, owned registered map archive.
		std::uint8_t networkGame;                  // +0x1AC; original 005109C4, startup/savegame network mode.
		std::uint8_t networkWait;                    // +0x1AD; original 005109C5.
		std::byte unknown1AE[2];
		std::uint8_t suppressDeathStatistics;      // +0x1B0; original 005109C8.
		std::uint8_t suppressDestructionLandscape; // +0x1B1; original 005109C9.
		std::byte unknown1B2[6];
	};

	inline SGameStaticVars gGameStaticVars{ };
	inline unsigned char& gGameInitialized = gGameStaticVars.initialized;
	inline SGameRandomState& gGameRandomState = gGameStaticVars.random;
	std::uint32_t Game_Random_GetNumber();
	std::uint32_t Game_UniqueId_Generate();
	std::uint32_t Logic_EntryPoint_Random_GetNumber();
	inline SGameTimeState& gGameTime = gGameStaticVars.gameTime;
	static_assert(sizeof(SGameRandomState) == 8);
	static_assert(alignof(SGameStaticVars) == alignof(void*));
	static_assert(offsetof(SGameStaticVars, suppressDeathStatistics) == (sizeof(void*) == 8 ? 444 : 432));
	static_assert(offsetof(SGameStaticVars, suppressDestructionLandscape) == (sizeof(void*) == 8 ? 445 : 433));
	static_assert(sizeof(SGameStaticVars) == (sizeof(void*) == 8 ? 456 : 440));
	static_assert(offsetof(SGameStaticVars, mapGuid) == 0x08);
	static_assert(offsetof(SGameStaticVars, mapType) == 0x18);
	static_assert(offsetof(SGameStaticVars, gameTime) == 0x20);
	static_assert(offsetof(SGameStaticVars, autosave) == 0x88);
	static_assert(offsetof(SGameStaticVars, random) == 0x94);
	static_assert(offsetof(SGameStaticVars, uniqueIdCounter) == 0x9C);
	static_assert(offsetof(SGameStaticVars, retainedMapRoot) == 0xA0);
	static_assert(offsetof(SGameStaticVars, retainedMapIni) == (sizeof(void*) == 8 ? 424 : 420));
	static_assert(offsetof(SGameStaticVars, mapFileLibrary) == (sizeof(void*) == 8 ? 432 : 424));
	static_assert(offsetof(SGameStaticVars, networkGame) == (sizeof(void*) == 8 ? 440 : 428));
	static_assert(offsetof(SGameStaticVars, networkWait) == (sizeof(void*) == 8 ? 441 : 429));
}
