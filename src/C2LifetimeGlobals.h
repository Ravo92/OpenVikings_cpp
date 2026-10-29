#pragma once

#include "C2ObjectLayout.h"
#include "C2EngineLifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"

#include "C2E2StaticDataHouses.h"
#include "C2E2StaticDataPalettes.h"

#include "C2E2StaticDataTerrain.h"
#include "C2GameControlStartup.h"
#include "C2GameState.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CStringArray;
}

// Logic bindings are shared without importing engine graphics layouts.


namespace NC2E2
{
	// Original global: 00554DA4.
	inline void* gE2Manager = nullptr;
	// Original global: 00568374.
	inline void* gEngineMap = nullptr;
	// Original global: 00568378.
	inline void* gE2MapWeatherManagerSectorManager = nullptr;
	// Original global: 00568384.
	inline void* gE2HumanSoundManager = nullptr;
	// Original global: 00568418.
	inline void* gSingleton00568418 = nullptr;
	// Original global: 005550D4.
	inline void* gE2AmbientSoundManager = nullptr;
	// Original global: 00568428.
	inline void* gE2ParticelManager = nullptr;
	// Original global: 005684C0.
	inline void* gMacroMapManager = nullptr;
	// Original global: 005684D8.
	inline void* gE2StaticDataParticelEffectManager = nullptr;
	// Original global: 00568564.
	inline void* gE2StaticDataMiscLandscapeManager = nullptr;
	// Original global: 00568580.
	inline void* gE2StaticDataGoodManager = nullptr;
	// Original global: 00568978.
	inline void* gE2StaticDataHumanMiscManager = nullptr;
	// Original global: 005689C8.
	inline void* gE2StaticDataMapMoveablesManager = nullptr;
	// Original global: 005689E8.
	inline void* gE2StaticDataParticelManager = nullptr;
	// Original global: 00568A1C.
	inline void* gE2StaticDataTransitionsManager = nullptr;
	// Original global: 00568A50.
	inline void* gE2StaticDataPatternManager = nullptr;
	// Original global: 00568A6C.
	inline void*& gE2StaticDataHousesManager = gStaticHouseStorage.owner;
	// Original global: 00568AEC.
	inline void* gE2StaticDataLandscapesManager = nullptr;
	// Original global: 00568AF8.
	inline void* gE2StaticDataTexturesManager = nullptr;
	// Original global: 00568B48.
	// Complete generator storage at 00568B44..00568B4B, zero-filled in the PE.
	// Both Windows drawing consumers select timeGetTime()/83 instead of game ticks
	// when this byte is nonzero. The original member name and writer remain unresolved.
	struct SE2ParticelGeneratorStorage
	{
		std::uint8_t useRealTimeAnimationClock;
		std::byte padding01[3];
		void* owner;
	};
	inline SE2ParticelGeneratorStorage gE2ParticelGeneratorStorage{ };
	inline void*& gE2ParticelGeneratorManager = gE2ParticelGeneratorStorage.owner;
	static_assert(sizeof(SE2ParticelGeneratorStorage) == 8);
	static_assert(offsetof(SE2ParticelGeneratorStorage, owner) == 4);
	// Original global: 0056842C.
	inline void* gE2ParticelWind = nullptr;
	// Original global: 005684D0.
	inline void* gStaticParticelEffects = nullptr;
	// Original global: 0056854C.
	// Count at 00568548 belongs to the adjacent 12-byte landscape mapping array.
	inline std::uint32_t gStaticMiscLandscapeEntryCount = 0;
	inline void* gStaticMiscLandscapeEntries = nullptr;
	// Original global: 00568554.
	inline void* gStaticMiscLandscapeGroups = nullptr;
	// Original global: 00568578.
	inline void* gStaticGoodData = nullptr;
	// Complete static particle region 005689D0..005689E7; vector stride 0x48.
	struct SStaticParticelStorage
	{
		std::int32_t munitionIds[4]; std::int32_t count; void* entries;
	};
	inline SStaticParticelStorage gStaticParticelStorage{ };
	inline void*& gStaticParticelData = gStaticParticelStorage.entries;
	static_assert(sizeof(SStaticParticelStorage) == 24);
	static_assert(offsetof(SStaticParticelStorage, count) == 0x10);
	// Original global: 00568A34.
	inline void*& gStaticPatternEntries = gStaticPatternStorage.entries;
	// Original global: 00568A64.
	inline void*& gStaticHouseEntries = gStaticHouseStorage.entries;
	// Original global: 00568A7C.
	inline void*& gStaticLandscapeEntries = gStaticLandscapeStorage.entries;
	// Complete regions cleared by their Windows destructors. Unidentified fields
	// retain explicit offsets rather than speculative member names.
	// 00568590..00568957; color records occupy +0x58..+0x3C7.
	alignas(4) inline std::byte gStaticHumanMiscData[968]{ };
	inline std::byte* const gStaticHumanColors = gStaticHumanMiscData + NC2Layout::StaticHumanMiscOffset::PlayerColors;
	// 00568998..005689BB; counts, record arrays and auxiliary allocations.
	alignas(4) inline std::byte gStaticMapMoveableData[32]{ };
	// 005689F0..00568A0F; transition counts and tables.
	inline std::byte(&gStaticTransitionData)[sizeof(SStaticTransitionStorage)] = *reinterpret_cast<std::byte(*)[sizeof(SStaticTransitionStorage)]>(&gStaticTransitionStorage);
	inline std::uint32_t gStaticMiscLandscapeGroupCount = 0; // 00568550
	inline std::int32_t& gStaticHouseCount = gStaticHouseStorage.count; // 00568A60
	inline std::int32_t& gStaticLandscapeCount = gStaticLandscapeStorage.count; // 00568A78
	inline std::uint32_t& gStaticPatternCount = gStaticPatternStorage.count;   // 00568A30
	// Complete Windows music selection region 005683B0..00568417. The owning
	// Windows-only class name is unresolved; field roles follow its Windows xrefs.
	struct SWorldMusicState
	{
		std::uint32_t selectedPlayer;
		std::uint8_t changePending;
		std::byte padding05[3];
		std::uint32_t changeAtGameTime;
		std::uint32_t eventVariant;
		std::uint32_t controllingPlayer;
		std::uint32_t visiblePlayerCounts[20];
		std::uint32_t musicType;
	};
	inline SWorldMusicState gWorldMusicState{ };
	static_assert(sizeof(SWorldMusicState) == 104);
	static_assert(offsetof(SWorldMusicState, changePending) == 4);
	static_assert(offsetof(SWorldMusicState, visiblePlayerCounts) == 0x14);
	static_assert(offsetof(SWorldMusicState, musicType) == 0x64);
}
