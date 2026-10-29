#pragma once

#include "C2RuntimeConstants.h"
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SGoodSearch
	{
		std::uint32_t player; std::uint32_t good; std::uint32_t point;
	};
	static_assert(sizeof(SGoodSearch) == 12);
	std::int32_t GuideManager_l_GetGuideIdWithGood(void* manager, std::int32_t guideId, std::uint32_t good);
	bool GuideManager_IsGoodInLinkSystem(void* manager, std::int32_t guideId, std::uint32_t good);
	bool GuideManager_GetMapPositionWithGoods(void* manager, std::uint32_t radius, std::int32_t guideId, SGoodSearch& data, bool anyPlayer);
	bool Human_FindGood_Complex(const void* human, std::uint32_t good, void* realPosition, void* workPosition, const void* source);
	bool Human_FindEquipment_Complex_Nearby(const void* human, std::uint32_t good, bool anyPlayer);
	struct SHumanSearchGoodsComplexData
	{
		explicit SHumanSearchGoodsComplexData(std::uint32_t playerId);
		std::uint32_t goodCount;
		std::uint32_t goods[NC2Runtime::GoodSearchLimits::Goods];
		std::uint32_t landscapeTypes[NC2Runtime::GoodSearchLimits::Goods];
		std::uint32_t player;
		std::int32_t firstDistance;
		std::uint8_t firstWasLandscape;
		std::byte padding21D[3];
		std::uint32_t landscapeCount;
		std::uint32_t landscapePoints[NC2Runtime::GoodSearchLimits::LandscapeCandidates];
		std::uint32_t landscapeGoods[NC2Runtime::GoodSearchLimits::LandscapeCandidates];
		std::uint32_t houseCount;
		void* houses[NC2Runtime::GoodSearchLimits::HouseCandidates];
		std::uint32_t houseGoods[NC2Runtime::GoodSearchLimits::HouseCandidates];
		std::uint32_t guideCount;
		std::uint32_t guides[NC2Runtime::GoodSearchLimits::GuideCandidates];
		std::uint32_t guidePosition;
	};
	static_assert(sizeof(SHumanSearchGoodsComplexData) == (sizeof(void*) == 8 ? 680 : 668));
	static_assert(offsetof(SHumanSearchGoodsComplexData, player) == 0x214);
	static_assert(offsetof(SHumanSearchGoodsComplexData, landscapeCount) == 0x220);
	static_assert(offsetof(SHumanSearchGoodsComplexData, houses) == 0x278);
	static_assert(offsetof(SHumanSearchGoodsComplexData, guidePosition) == (sizeof(void*) == 8 ? 672 : 664));

	std::int32_t GuideManager_l_GetGuideIdWithAnyGood(void* manager, std::int32_t guideId, const SHumanSearchGoodsComplexData& data);
	bool GuideManager_IsAnyGoodInLinkSystem(void* manager, std::int32_t guideId, const SHumanSearchGoodsComplexData& data);
	bool GuideManager_GetMapPositionWithGoodsComplex(void* manager, std::uint32_t radius, std::int32_t guideId, SHumanSearchGoodsComplexData& data);
	bool Human_FindFoodInHomeHouse(const void* human, void* realPosition, void* workPosition);
	std::uint32_t Human_FindFood_Complex(const void* human, void* realPosition, void* workPosition, const void* source);
	bool Human_FindGoodsDemandedForHomeHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);
	bool Human_FindGoodsDemandedForBuildHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);
	bool Human_FindGoodsDemandedForVehicle(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);
	bool Human_FindGoodsDemandedForStockHouse(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);
	void* Human_GetHousePtrAtTargetPosition(const void* human);
	void* Human_GetTargetHousePtr(const void* human);
	bool Human_l_StartAtomicWalkAndFetch(void* human, const void* realPosition, const void* workPosition, bool forSelf);
	bool Human_l_StartAtomicWalkAndBring(void* human, const void* realPosition, const void* workPosition);
	bool Human_l_StartAtomicWalkAndDrop(void* human, const void* realPosition, const void* workPosition, std::uint32_t good);
	bool Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(void* human);
	bool Human_FindConsumerNearWorkCenter(const void* human, std::uint32_t good);
	bool Human_FindConsumerNearWorkCenter(const void* human, std::uint32_t good, void* realPosition, void* workPosition);
	bool Map_GoodTool_PutOnMap(std::uint32_t good, const void* point, bool preserveEmptyLandscape);
	bool Map_GoodTool_PutOnMap(std::uint32_t good, std::uint32_t amount, std::uint32_t point, std::uint32_t radius, bool preserveEmptyLandscape);
	bool Map_GoodTool_CanBePutOnMap(std::uint32_t good, const void* point);
	bool Human_FindPilePointNearby(const void* human, void* realPosition, void* workPosition);
	bool Human_FindPilePointNearby(const void* human, std::uint32_t good, void* realPosition, void* workPosition);
	bool Human_FindPilePointNearPosition(const void* human, std::uint32_t good, const void* source, void* realPosition, void* workPosition);
	bool Human_FindPilePointNearWorkCenter(const void* human, void* realPosition, void* workPosition);
	void Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(void* human);
	void Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(void* human);
	void Human_l_PerformTaskEvent_Pickup(void* human);
	void Human_l_PerformTaskEvent_Pileup(void* human);
	void Human_Target_RestoreSecondaryTarget(void* human);
	bool Human_FindGoodsDemandedForBuildVehicle(void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);


	std::uint32_t Human_Target_GetType(const void* human);
	const void* Human_Target_GetWorkPositionRef(const void* human);
	const void* Human_Target_GetRealPositionRef(const void* human);
	void* Human_GetHousePtrAtMapPosition(const void* human, const void* point);
}
