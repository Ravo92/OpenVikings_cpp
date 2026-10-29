#pragma once

#include "C2GameControlStartup.h"
#include "C2ControlStartup.h"
#include "C2GameState.h"
#include "C2ObjectLayout.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CStringArray;
}

// Original singleton/storage bindings. Complete owning class layouts remain unresolved.
namespace NC2Logic
{
	// Original data: 005106F8; shared startup input, independent of graphics owners.
	inline SGameControlStartupData gGameControlStartupData;
	class CCampaignStaticDataManager;
	class CStaticStringsManager;
	class CSavegamesManager;
	class CCleanMapsManager;
	class CStaticTrianglePatternTypeDataManager;
	class CStaticLandscapeTypeDataManager;
	class CStaticGoodTypeDataManager;
	class CStaticHouseTypeDataManager;
	class CStaticVehicleTypeDataManager;
	class CStaticAtomicAnimationDataManager;
	class CStaticTribeTypeDataManager;
	struct SStaticTribeTypeData;
	struct SStaticAtomicAnimationData;
	struct SStaticVehicleTypeData;
	struct SStaticHouseTypeData;
	struct SStaticGoodTypeData;
	struct SStaticLandscapeTypeData;
	// Original global: 005109D0.
	inline void* gGame = nullptr;
	// Original global: 005109D4.
	inline void* gAIManager = nullptr;
	// Original global: 005109EC.
	inline void*& gGameControl = gGameControlRuntimeStorage.owner;
	// Original global: 00510A14.
	inline void* gMarkerArray = nullptr;
	// Original global: 00510A20.
	inline void* gUserNetworkCommandManager = nullptr;
	// Original global: 00510A28.
	inline void* gGameMapModifyManager = nullptr;
	// Complete CGameMapModifyManager::mStaticVars at 00510A24; nesting depth.
	inline std::uint32_t gGameMapModifyNestingDepth = 0;
	// Original global: 00510A40.
	inline void* gStaticHumanJobExperienceTypeDataManager = nullptr;
	// Original global: 00510D40.
	inline void* gStaticDependenciesManager = nullptr;
	// Complete CStaticDependenciesManager array 00510A80..00510D3F.
	inline std::int32_t gStaticDependencyGoodByLandscape[176]{ };
	// Original global: 00510D50.
	inline void* gStaticAnimalTypeDataManager = nullptr;
	// Original global: 00510DFC.
	inline void* gStaticArmorTypeDataManager = nullptr;
	// Original global: 00510EF0.
	inline void* gStaticWeaponTypeDataManager = nullptr;
	// Original global: 00510FA4.
	inline void* gStaticJobTypeDataManager = nullptr;
	// Original global: 00511024.
	extern CStaticTribeTypeDataManager*& gStaticTribeTypeDataManager;
	// Original global: 00511120.
	extern CStaticAtomicAnimationDataManager*& gStaticAtomicAnimationDataManager;
	// Original global: 00511134.
	extern CStaticVehicleTypeDataManager*& gStaticVehicleTypeDataManager;
	// Original global: 005111C4.
	extern CStaticHouseTypeDataManager*& gStaticHouseTypeDataManager;
	// Original global: 00511244.
	extern CStaticGoodTypeDataManager*& gStaticGoodTypeDataManager;
	// Original global: 005112E4.
	extern CStaticTrianglePatternTypeDataManager*& gStaticTrianglePatternTypeDataManager;
	// Original global: 00511354.
	extern CStaticLandscapeTypeDataManager*& gStaticLandscapeTypeDataManager;
	// Original global: 005114AC.
	inline CSavegamesManager* gSavegamesManager = nullptr;
	// Original global: 005114B4.
	inline CCleanMapsManager* gCleanMapsManager = nullptr;
	// Original global: 005114B8.
	inline CCampaignStaticDataManager* gCampaignStaticDataManager = nullptr;
	// Original global: 005114EC.
	inline CStaticStringsManager* gStaticStringsManager = nullptr;
	// Original global: 005114F0.
	inline void* gMissionOnscreenInfoManager = nullptr;
	// Original global: 005114F4.
	inline void* gGameAssistantManager = nullptr;
	// Original global: 00511500.
	inline void* gMultiPlayerGoalManager = nullptr;
	// Original global: 00511504.
	inline void* gHappinessManager = nullptr;
	// Original global: 00511508.
	inline void* gEmbeddedSavegameManager = nullptr;
	// Original global: 0051150C.
	inline void* gTributeManager = nullptr;
	// Original global: 00511510.
	inline void* gDelayedActionManager = nullptr;
	// Original global: 00511514.
	inline void* gPlayerStatisticManager = nullptr;
	// Original global: 00511550.
	inline void* gGuideManager = nullptr;
	// Original global: 0051155C.
	inline void* gHumanMerchantArray = nullptr;
	// Original global: 00511564.
	inline void* gMapPathfinderManager = nullptr;
	// Original global: 00511568.
	inline void* gMissionManager = nullptr;
	// Original global: 0051257C.
	inline void* gGameMap = nullptr;
	// Original global: 00512590.
	inline void* gMapBioRegrowManager = nullptr;
	// Original global: 00512594.
	inline void* gMapLandscapeAutoTransitionManager = nullptr;
	// Original global: 00512598.
	inline void* gWallBuildManager = nullptr;
	// Original global: 0051259C.
	inline void* gRoadBuildManager = nullptr;
	// Original global: 00550E64.
	inline void* gMapFishManager = nullptr;
	// Original global: 00510A10.
	inline void* gMarkerEntries = nullptr;
	// Original global: 00510A38.
	inline void* gStaticHumanJobExperienceData = nullptr;
	// Original global: 00510A3C.
	inline void* gStaticHumanJobExperienceTable00510A3C = nullptr;
	// Original global: 00510D4C.
	inline void* gStaticAnimalTypeData = nullptr;
	// Original global: 00510DF8.
	inline void* gStaticArmorTypeData = nullptr;
	// Original global: 00510FA0.
	inline void* gStaticJobTypeData = nullptr;
	// Original global: 00511020.
	extern SStaticTribeTypeData*& gStaticTribeTypeData;
	// Original global: 0051111C.
	extern SStaticAtomicAnimationData*& gStaticAtomicAnimationData;
	// Original global: 00511130.
	extern SStaticVehicleTypeData*& gStaticVehicleTypeData;
	// Original global: 005111C0.
	extern SStaticHouseTypeData*& gStaticHouseTypeData;
	// Original global: 00511240.
	extern SStaticGoodTypeData*& gStaticGoodTypeData;
	// Original global: 00511348.
	extern SStaticLandscapeTypeData*& gStaticLandscapeTypeData;
	// Original global: 00511560.
	inline void* gMapPathfinderEntries = nullptr;
	// Complete CStaticWeaponTypeDataManager pointer array, 00510E48..00510EEF.
	inline void* gStaticWeaponTypeArrays[42]{ };
	// Complete CStaticStringsManager pointer array, 005114C0..005114EB.
	inline NXBasics::CStringArray* gStaticStrings[11]{ };
	// SMapMigPoint/SMapMagPoint dimension mirrors, complete 005125F8..005125FF.
	struct SMapPointDimensions
	{
		std::int16_t microWidth;
		std::int16_t microHeight;
		std::int16_t macroWidth;
		std::int16_t macroHeight;
	};
	inline SMapPointDimensions gMapPointDimensions{ };
	static_assert(sizeof(SMapPointDimensions) == 8);
	static_assert(offsetof(SMapPointDimensions, macroWidth) == 4);
}
