#include "RemasterAILog.h"
#include "C2AIBuildingAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineTerrainData.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HouseUserControl.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "SwitchIdentifiers.h"
#include "RemasterAIEconomy.h"
#include <initializer_list>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		// Stack descriptor 00471F5B: all offsets and widths come from Windows stores.
		struct SBuildSearch
		{
			void* village; std::int32_t engineType; std::uint32_t stage; std::uint8_t checkTerrain; std::byte padding[3]; void* output; std::uint32_t userData;
		};
		static_assert(sizeof(SBuildSearch) == 24);
		void* House(std::int32_t id)
		{
			return CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
		}
		std::uint32_t Distance(const void* a, const void* b)
		{
			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_00453bd4; the inlined original method name remains unresolved.
	bool AIVillage_IsPointInside(const void* village, const void* point)
	{
		return Distance(point, static_cast<const std::byte*>(village) + NC2Layout::AIVillageOffset::CenterPoint) < Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius);
	}

	// Original function: FUN_004544fb; both moveable lists precede houses and animals.
	bool AI_IsEnemyObjectAround(std::uint32_t player, const void* point, std::uint32_t radius)
	{
		if (Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, true, 1, 0, true, false) != -1) return true;
		if (Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, true, 1, 0, false, false) != -1) return true;
		if (Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, true, 1, 0) != -1) return true;
		return Tool_Animals_GetOneOfNextEnemyInRange(player, point, 0, radius, true, false, 1, 0) != -1;
	}

	// Original function: FUN_0045742d; need counters increase recursively even after a failed input.
	bool AIGoodManager_IsProducingGood(void* manager, std::uint32_t good, bool includeInputs)
	{
		auto* entry = static_cast<std::byte*>(manager) + good * NC2Layout::RecordSize::AIGoodUnsigned;
		if (!includeInputs) return Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag) != 0;
		Write(entry, NC2Layout::AIGoodOffset::NeedCount, Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) + 1u);
		const auto* definition = StaticGoodTypeData_GetPtr(good);
		bool available = true;
		for (std::uint32_t index = 0; index < definition->distinctProductionInputCount; ++index)
			if (!AIGoodManager_IsProducingGood(manager, static_cast<std::uint32_t>(definition->distinctProductionInputs[index]), true)) available = false;
		if (!available) return false;
		return Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag) != 0;
	}

	// Remaster: Egypt has only the large warehouse role (9), not the small warehouse (7).
	std::int32_t Remaster_AI_GetStockHouseId(std::uint32_t tribe, std::uint32_t* stage)
	{
		for (const auto logic : { 7u, 8u, 9u })
		{
			const auto engine = NC2E2::Engine2d_EntryPoint_GetHouseId(tribe, logic, stage);
			if (engine != -1) return engine;
		}
		return -1;
	}

	// Original function: FUN_00471cf6; the five distinct status codes are retained.
	std::int32_t AIVillageManager_House_BuildHouse(void* village, std::uint32_t logicType, const void* source, bool disableOnFailure, MapSearchCallback callback, std::uint32_t userData)
	{
		const auto report = [&] (std::int32_t result, const char* reason) { RemasterAILog::Event(Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId), "build_decision", static_cast<std::int32_t>(logicType), "village=%u logic=%u result=%d reason=%s", Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::VillageId), logicType, result, reason); return result; };
		const bool starterHome = RemasterAIEconomy::NeedsStarterHome(village);
		if (starterHome && logicType != static_cast<std::uint32_t>(EHouseType::Home00)) return report(-2, "starter_home_has_priority");
		if (!RemasterAIEconomy::CanBuild(Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId), logicType)) return report(-2, "workforce_or_build_budget");
		if (Read<std::uint8_t>(village, NC2Layout::AIVillageOffset::HouseBuildAllowedFlagsUnsigned + logicType) == 0) return report(0, "house_not_allowed_or_placement_cache");
		const auto player = Read<std::int32_t>(village, NC2Layout::AIVillageOffset::PlayerId);
		const auto tribe = player < 20 ? Read<std::uint32_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType) : 1u;
		std::uint32_t stage;
		const auto engineType = NC2E2::Engine2d_EntryPoint_GetHouseId(tribe, logicType, &stage);
		if (engineType == -1) return report(0, "no_civilization_building_definition");
		auto* build = reinterpret_cast<SHouseBuild*>(static_cast<std::byte*>(village) + NC2Layout::AIVillageOffset::BuildHouse);
		if (build->IsValid()) return report(-2, "another_construction_is_active");
		const auto* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType);
		bool available = true;
		for (std::uint32_t level = 0; level < stage + 1u; ++level)
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::ConstructionGoodsUnsigned + level * NC2Layout::RecordSize::HouseConstructionGoodListUnsigned); ++index)
				if (!AIGoodManager_IsProducingGood(static_cast<std::byte*>(village) + NC2Layout::AIVillageOffset::Goods, Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::FirstConstructionGoodUnsigned + level * NC2Layout::RecordSize::HouseConstructionGoodListUnsigned + index * NC2Layout::RecordSize::HouseStageValueUnsigned), true)) available = false;
		// The first home requests its real materials above, then lets native delivery supply it.
		if (!available && !starterHome && !RemasterAIEconomy::HasConstructionGoods(village, static_cast<std::uint32_t>(engineType), stage)) return report(-1, "construction_materials_not_supplied_or_stocked");
		std::uint32_t point = 0;
		if (!AIVillageManager_l_House_GetPossibleBuildPosition(village, source, engineType, stage, &point, false, callback, userData))
		{
			if (RemasterAIEconomy::IsActive(static_cast<std::uint32_t>(player))) return report(-2, "no_valid_build_position_retrying");
			if (disableOnFailure) Write<std::uint8_t>(village, NC2Layout::AIVillageOffset::HouseBuildAllowedFlagsUnsigned + logicType, 0);
			return report(0, "no_valid_build_position");
		}
		if (AI_IsEnemyObjectAround(static_cast<std::uint32_t>(player), &point, 30)) return report(-3, "enemy_near_build_position");
		build->BuildHouse(static_cast<std::uint32_t>(player), &point, static_cast<std::uint32_t>(engineType), stage, false);
		return report(1, "construction_requested");
	}

	// Original function: FUN_00471e59; optional macro terrain checks follow the cumulative six-step ring.
	bool __cdecl slAIVillageManagerSearchBuildPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
	{
		const auto& search = *static_cast<const SBuildSearch*>(data->userData);
		if (!AIVillage_IsPointInside(search.village, point)) return false;
		if (search.checkTerrain != 0)
		{
			std::uint32_t macro;
			MapMagPoint_ConstructFromMig(&macro, point, false);
			const auto check = [] (const void* position)
			{
				const void* terrain = GameMap_GetConstMapMacroPointPtr(position);
				return Read<std::uint8_t>(&gStaticTrianglePatternTypes[Read<std::uint8_t>(terrain, NC2Layout::MapMacroPointOffset::PatternA)], NC2Layout::TrianglePatternTypeOffset::BioCanPlantOn) != 0 && Read<std::uint8_t>(&gStaticTrianglePatternTypes[Read<std::uint8_t>(terrain, NC2Layout::MapMacroPointOffset::PatternB)], NC2Layout::TrianglePatternTypeOffset::BioCanPlantOn) != 0;
			};
			if (!check(&macro)) return false;
			MapMagPoint_MoveCursorUnwrapped(&macro, 4);
			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				MapMagPoint_MoveCursorUnwrapped(&macro, direction);
				if (MapMagPoint_IsInsideMap(&macro) && !check(&macro)) return false;
			}
		}
		if (!Tool_House_IsAllowedToBeBuild(Read<std::uint32_t>(search.village, NC2Layout::AIVillageOffset::PlayerId), search.engineType, search.stage, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), false, false)) return false;
		Write(search.output, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		return true;
	}

	// Original function: FUN_00471f5b; snapping mutates the local source, never the caller's point.
	bool AIVillageManager_l_House_GetPossibleBuildPosition(void* village, const void* source, std::int32_t engineType, std::uint32_t stage, void* output, bool checkTerrain, MapSearchCallback callback, std::uint32_t userData)
	{
		SBuildSearch search{ village, engineType, stage, static_cast<std::uint8_t>(checkTerrain),{ }, output, userData };
		auto point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		Tool_Map_GetNotBlockedPosition(&point, true);
		return CMapPathfinderElement::SearchOverMap(&point, Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius), true, callback == nullptr ? slAIVillageManagerSearchBuildPos : callback, &search, false, Read<std::int32_t>(village, NC2Layout::AIVillageOffset::PlayerId));
	}

	// Original function: FUN_0047261f; every next-stage material is checked before returning -1.
	std::int32_t AIVillageManager_House_TryToUpgradeHouse(void* village, void* house)
	{
		if (!House_Construction_CanBeUpgraded(house)) return 0;
		auto* upgrade = reinterpret_cast<SHouseUpgrade*>(static_cast<std::byte*>(village) + NC2Layout::AIVillageOffset::UpgradeHouse);
		if (upgrade->IsValid()) return -2;
		const auto* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto level = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) + 1u;
		bool available = true;
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::ConstructionGoodsUnsigned + level * NC2Layout::RecordSize::HouseConstructionGoodListUnsigned); ++index)
			if (!AIGoodManager_IsProducingGood(static_cast<std::byte*>(village) + NC2Layout::AIVillageOffset::Goods, Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::FirstConstructionGoodUnsigned + level * NC2Layout::RecordSize::HouseConstructionGoodListUnsigned + index * NC2Layout::RecordSize::HouseStageValueUnsigned), true)) available = false;
		if (!available) return -1;
		upgrade->UpgradeHouse(house);
		return 1;
	}

	// Original function: FUN_004729ff; a distance of exactly twenty lies outside the defend range.
	bool AIHouseManager_Tower_IsPointInDefendRange(const void* manager, const void* point)
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(manager, NC2Layout::AIHouseManagerOffset::DefendPointCount); ++index)
			if (Distance(point, static_cast<const std::byte*>(manager) + 0xC4u + index * 4u) < 20) return true;
		return false;
	}
	// Original function: FUN_00472a5a.
	bool AIHouseManager_Tower_IsHouseInDefendRange(const void* manager, const void* house)
	{
		return AIHouseManager_Tower_IsPointInDefendRange(manager, static_cast<const std::byte*>(house) + 0x1C);
	}

	// Original function: FUN_00476be4; tower candidates also reject already defended points.
	bool __cdecl slAITowerManagerSearchBuildPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
	{
		const auto& search = *static_cast<const SBuildSearch*>(data->userData);
		if (!AIVillage_IsPointInside(search.village, point)) return false;
		if (AIHouseManager_Tower_IsPointInDefendRange(static_cast<const std::byte*>(search.village) + 0x1C, point)) return false;
		if (!Tool_House_IsAllowedToBeBuild(Read<std::uint32_t>(search.village, NC2Layout::AIVillageOffset::PlayerId), search.engineType, search.stage, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), false, false)) return false;
		Write(search.output, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		return true;
	}

	// Original function: FUN_00476c43; a build attempt finishes this tick, even on a failed status.
	void AITowerManager_DoOneLogicGameTick(void* manager, std::uint32_t player, void* houseManager, void* village)
	{
		const auto build = Read<SHouseBuild>(village, NC2Layout::AIVillageOffset::BuildHouse);
		if (build.IsValidCheck()) return;
		auto last = Read<std::int32_t>(manager, NC2Layout::AITowerManagerOffset::LastHouseId);
		if (last != -1 && Read<std::uint8_t>(House(last), NC2Layout::HouseOffset::AllocatedFlag) == 0)
		{
			Write<std::int32_t>(manager, NC2Layout::AITowerManagerOffset::LastHouseId, -1); last = -1;
		}
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			if (last != -1)
			{
				if (last <= id)
				{
					Write<std::int32_t>(manager, NC2Layout::AITowerManagerOffset::LastHouseId, -1); last = -1;
				} continue;
			}
			auto* house = House(id);
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
			if (type == 40 || type == 41) AIVillageManager_House_TryToUpgradeHouse(village, house);
			else if (type == 1) continue;
			if (AIHouseManager_Tower_IsHouseInDefendRange(houseManager, house)) continue;
			const auto* point = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
			const auto result = AIVillageManager_House_BuildHouse(village, 41, point, false, slAITowerManagerSearchBuildPos, 0);
			if (result == -3 || result == -2) Write(manager, NC2Layout::AITowerManagerOffset::LastHouseId, id - 1);
			else if (result >= -1 && result <= 0)
			{
				const auto fallback = AIVillageManager_House_BuildHouse(village, 40, point, false, slAITowerManagerSearchBuildPos, 0);
				if (fallback == -2 || fallback == -1) Write(manager, NC2Layout::AITowerManagerOffset::LastHouseId, id - 1);
			}
			Write(manager, NC2Layout::AITowerManagerOffset::LastHouseId, id);
			return;
		}
	}

	// Original function: FUN_00476d4b; failed validation clears the original active flag.
	bool SHouseBuild::IsValid()
	{
		if (active == 0) return false;
		if (houseId != -1)
		{
			const void* house = House(houseId);
			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == uniqueId && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) return true;
		}
		active = 0;
		return false;
	}
	// Original function: FUN_00476d89; this check deliberately leaves the flag untouched.
	bool SHouseBuild::IsValidCheck() const
	{
		if (active == 0 || houseId == -1) return false;
		const void* house = House(houseId);
		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == uniqueId && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3;
	}
	// Original function: FUN_00476dc5; future stock is modified before construction stock.
	void SHouseBuild::BuildHouse(std::uint32_t player, const void* point, std::uint32_t engineType, std::uint32_t stage, bool fillConstructionStock)
	{
		const auto id = Logic_EntryPoint_House_Set(player, engineType, stage, point, false, 0);
		RemasterAILog::Event(player, "construction_placed", id, "engine=%u stage=%u position=(%d,%d) fill_stock=%u result=%s", engineType, stage, Read<std::int16_t>(point, 0), Read<std::int16_t>(point, 2), fillConstructionStock, id == -1 ? "placement_failed" : "created");
		if (id == -1) return;
		void* house = House(id);
		houseId = id; uniqueId = Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId); active = 1;
		if (!fillConstructionStock) return;
		const auto* definition = static_cast<const std::byte*>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType))) + 0x48;
		auto remaining = stage + 1u;
		do
		{
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(definition, NC2Layout::HouseConstructionGoodsOffset::Count); ++index)
			{
				const auto good = Read<std::uint32_t>(definition, NC2Layout::HouseConstructionGoodsOffset::GoodsUnsigned + index * NC2Layout::RecordSize::GoodIdUnsigned);
				House_Stock_ModifyFutureAmount(house, good, 1, true);
				House_Stock_ModifyAmount(house, nullptr, good, 1, true);
			}
			definition += 0x40;
		} while (--remaining != 0);
	}
	// Original function: FUN_00476e5d; reaching the requested stage retires the upgrade.
	bool SHouseUpgrade::IsValid()
	{
		if (active == 0) return false;
		if (houseId != -1)
		{
			const void* house = House(houseId);
			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == uniqueId && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) != stage && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) return true;
		}
		active = 0;
		return false;
	}
	// Original function: FUN_00476ea4; active is set after starting construction.
	void SHouseUpgrade::UpgradeHouse(void* house)
	{
		stage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) + 1u; houseId = Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId); uniqueId = Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId); House_Construction_UpgradeStart(house, nullptr); active = 1;
	}
}
