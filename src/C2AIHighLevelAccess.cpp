#include "C2AIAnimalDiplomacy.h"
#include "MasterControlProgram.h"
#include "C2AIBuildingAccess.h"
#include "C2AIGlobalGoods.h"
#include "C2AIGuideAccess.h"
#include "C2AIHighLevelAccess.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIObjectAccess.h"
#include "C2AIResourceAccess.h"
#include "C2AIRoadAccess.h"
#include "C2AIVillageAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerCenterPosition.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		// Original function: FUN_00428f0e.
		void AIInvalidAreaManager_Construct(void* areas)
		{
			for (std::uint32_t i = 0; i < 50; ++i)
			{
				Write<std::uint16_t>(areas, NC2Layout::AIInvalidAreaManagerOffset::FirstAreaY + i * NC2Layout::RecordSize::AIInvalidArea, 0); Write<std::uint16_t>(areas, NC2Layout::AIInvalidAreaManagerOffset::Areas + i * NC2Layout::RecordSize::AIInvalidArea, 0);
			}
		}
	}
	// Original function: FUN_00428bb2; alternating village updates retain the Windows cadence.
	void AIHighLevelHandler_DoOneGameTick(void* handler, std::uint32_t tick)
	{
		if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::Enabled) == 0) return;
		auto* bytes = static_cast<std::byte*>(handler);
		auto* villages = bytes + 0x1C; auto* resources = bytes + 0x67B4; auto* roads = bytes + 0x94E4;
		auto* catcher = bytes + 0x94E8; auto* goods = bytes + 0x9520; auto* military = bytes + 0x7578; auto* guides = bytes + NC2Layout::AIHighLevelOffset::GuideManager;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId);
		if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::InitializationPending) != 0)
		{
			if (!AIHighLevelHandler_l_CreateVillages(handler)) return;
			AIResourceManager_Init(resources, player, villages); AIRoadManager_Init(roads); AIAnimalCatcher_Init(catcher); AIGlobalGoods_Init(goods);
			AIMilitaryManager_Init(military, player, bytes + 0x34, Read<std::uint32_t>(AIVillagesData_GetVillageDataById(villages, 0), NC2Layout::AIVillagePhysicalSlotOffset::Center));
			Write<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::InitializationPending, 0);
		}
		const auto* program = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;
		const bool menu = program != nullptr && program->Remaster_IsMainMenuActive();
		if (!menu && Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::RandomDestructionEnabled) != 0) AIHighLevelHandler_l_DestroyRandom(handler, tick);
		if ((tick & 1) == 0) AIResourceManager_UpdateResourcePointArray(resources, player, villages);
		AIGuideManager_CheckAndBuildGuides(guides, player, military);
		if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::GuideScoutRequestedFlag) != 0)
		{
			const auto id = AIHighLevelHandler_Tool_GetUnusedHuman(handler, bytes + NC2Layout::AIHighLevelOffset::GuideDestination);
			if (id != -1) AIScout_Set(guides, id);
		}
		if ((tick & 1) == 0)
		{
			AIVillagesData_DoOneLogicTick(villages, resources, goods, tick); AIResourceManager_ClearResourceUpdateFlags(resources);
		}
		if (tick % 5 == 0) AIRoadManager_DoOneLogicTick(roads, player, resources);
		AIAnimalCatcher_DoOneLogicTick(catcher, player, bytes + 0x34, military);
		if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::AnimalCatcherHumanRequestedFlag) != 0)
		{
			// Windows also uses the guide destination when assigning the animal catcher.
			const auto id = AIHighLevelHandler_Tool_GetUnusedHuman(handler, bytes + NC2Layout::AIHighLevelOffset::GuideDestination);
			if (id != -1) AIScout_Set(catcher, id);
		}
		AIHighLevelHandler_l_Villages_EnhanceVillages(handler); AIHighLevelHandler_l_Villages_CheckForNeedments(handler);
		AIMilitaryManager_DoOneLogicGameTick(military, handler, tick);
		Write(handler, NC2Layout::AIHighLevelOffset::ShoeScanHumanId, AIHighLevelHandler_l_Humans_CheckShoes(handler, Read<std::int32_t>(handler, NC2Layout::AIHighLevelOffset::ShoeScanHumanId)));
		AIDiplomacy_DoOneLogicTick(bytes + NC2Layout::AIHighLevelOffset::Diplomacy, player, villages);
	}
	// Original function: FUN_00428d5a; stop at the first allocated human of this player.
	std::int32_t AIHighLevelHandler_l_Humans_CheckShoes(void* handler, std::int32_t previous)
	{
		auto* villages = static_cast<std::byte*>(handler) + 0x1C;
		if (!AIVillagesData_Good_IsProducingGood(villages, 30, true)) return -1;
		for (auto id = static_cast<std::int32_t>(static_cast<std::uint32_t>(previous) + 1); id < CHumanArray::sTheObjectPtr->mLastScanIndex; ++id)
		{
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId)) continue;
			if (Human_IsAgeAdult(human) && !Human_HasEquippedShoes(human) && AIVillagesData_GetVillageId(villages, Human_GetMapPositionRef(human)) != -1) Human_DoAddNewUserCommand(human, 79, Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId), id, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 30, 0);
			return id;
		}
		return -1;
	}
	// Original function: FUN_00428df8.
	void AIHighLevelHandler_l_Villages_CheckForNeedments(void* handler)
	{
		auto* bytes = static_cast<std::byte*>(handler); auto* villages = bytes + 0x1C; auto* resources = bytes + 0x67B4;
		std::byte areas[404]; AIInvalidAreaManager_Construct(areas); Write<std::uint32_t>(areas, NC2Layout::AIInvalidAreaManagerOffset::Count, 0);
		for (std::uint32_t good = 0; good < 66; ++good)
		{
			if (!AIGlobalGoods_IsNeedingVillage(bytes + 0x9520, good)) continue;
			if (Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::ResourceGoodRequirements + good * NC2Layout::RecordSize::AIResourceGood) == 1)
			{
				while (AIResourceManager_IsResourceOnMap(resources, good))
				{
					if (AIVillagesData_Villages_CreateNew(villages, 1, good, resources, areas) != 0) break;
					std::uint32_t area[2]{ 0, 25 }; AIResourceManager_GetResourcePosition(resources, good, area); AIInvalidAreaManager_AddArea(areas, area);
					AIResourceManager_UpdateGoodResource(resources, good, areas, villages);
				}
			}
			else AIVillagesData_Villages_CreateNew(villages, 0, 0, resources, areas);
		}
		if (!AIVillagesData_Military_IsBarrackBuild(villages) && !AIVillagesData_House_CanBuildAnyFurther(villages, 39)) AIVillagesData_Villages_CreateNew(villages, 0, 0, resources, areas);
		AIHighLevelHandler_l_Trader_Setup(handler);
	}
	// Original function: FUN_00428f26.
	void AIHighLevelHandler_l_Villages_EnhanceVillages(void* handler)
	{
		auto* bytes = static_cast<std::byte*>(handler); auto* guides = bytes + NC2Layout::AIHighLevelOffset::GuideManager;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId);
		if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::PendingVillageFlag) != 0 && Read<std::uint16_t>(handler, NC2Layout::AIHighLevelOffset::GuideDestination) == NXBasics::IntegerRange::UInt16MaximumSigned)
		{
			if (AIGuideManager_IsGuideAlreadyBuild(guides, player, bytes + 0x67AE))
			{
				if (Read<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::GuidePairSearchExhaustedFlag) != 0) Write<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::PendingVillageConfirmedFlag, 1);
			}
			else AIGuideManager_BuildGuideNearPosition(guides, player, bytes + 0x67AE);
		}
		auto* villages = bytes + 0x1C;
		if (!AIGoodManager_IsProducingGood(static_cast<std::byte*>(AIVillagesData_GetVillageDataById(villages, 0)) + 0x274, 19, false)) return;
		if (AIGoodManager_IsProducingGood(static_cast<std::byte*>(AIVillagesData_GetVillageDataById(villages, 0)) + 0x274, 31, true)) AIGoodManager_IsProducingGood(static_cast<std::byte*>(AIVillagesData_GetVillageDataById(villages, 0)) + 0x274, 32, true);
		if (AIAnimalCatcher_IsType10Available(bytes + 0x94E8)) AIGoodManager_IsProducingGood(static_cast<std::byte*>(AIVillagesData_GetVillageDataById(villages, 0)) + 0x274, 30, true);
	}
	// Original function: FUN_00428fec.
	void AIHighLevelHandler_l_Trader_Setup(void* handler)
	{
		for (std::uint32_t village = 0; village < 8; ++village)
		{
			auto* entry = static_cast<std::byte*>(handler) + 0x24 + village * 0xCF0;
			if (Read<std::uint8_t>(entry, NC2Layout::AIVillagePhysicalSlotOffset::Active) == 0) continue;
			for (std::uint32_t good = 0; good < 66; ++good)
				if (Read<std::uint8_t>(entry, NC2Layout::AIHighLevelVillageActiveViewOffset::FirstGoodImportingFlag + good * NC2Layout::RecordSize::AIGood) != 0 && Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::FirstGlobalGoodHasProducer + good * NC2Layout::RecordSize::AIGlobalGood) == 1 && Read<std::int32_t>(handler, NC2Layout::AIHighLevelOffset::GlobalGoods + good * NC2Layout::RecordSize::AIGlobalGood) != -1) AIGoodManager_SetTraderBringFlag(entry + 0x278, good, true);
		}
	}
	// Original function: FUN_00429191.
	bool AIHighLevelHandler_l_CreateVillages(void* handler)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId); std::uint32_t count = 0;
		CHumanIterator humans;
		for (auto id = humans.GetFirstSectorHumanId(player, false); id != -1; id = humans.GetNextSectorHumanId()) if (!Human_IsJobHero(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize)) ++count;
		std::uint32_t center = 0;
		if (!Tool_Misc_GetPlayerCenterPosition(player, &center))
		{
			Write<std::uint8_t>(handler, NC2Layout::AIHighLevelOffset::Enabled, 0); return false;
		}
		CHouseIterator houses; auto chosen = center; std::uint32_t distance = 10000;
		for (auto id = houses.GetFirstSectorHouseId(player, &chosen, 10000, false); id != -1; id = houses.GetNextSectorHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 1)
			{
				chosen = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint); break;
			}
			if (House_GetHouseMainType(house) != 1) continue;
			const auto candidate = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointX), Read<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointY), Read<std::uint16_t>(&chosen, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(&chosen, NC2Layout::MapCoordinatePairOffset::Y)));
			if (candidate < distance)
			{
				chosen = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint); distance = candidate;
			}
		}
		for (; count < 4; ++count)
		{
			const auto tribe = static_cast<std::int32_t>(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
			CHumanArray::sTheObjectPtr->AllocateHuman(player, tribe, 6, &chosen, 0, 0);
		}
		AIVillagesData_Init(static_cast<std::byte*>(handler) + 0x1C, player, &chosen); return true;
	}
	// Original function: FUN_00429340; Windows can wrap the house iterator while choosing.
	void AIHighLevelHandler_l_DestroyRandom(void* handler, std::uint32_t tick)
	{
		if (tick % 24 != 0) return;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId);
		if ((Game_Random_GetNumber() & 1) == 0)
		{
			auto remaining = Game_Random_GetNumber() % 100 + 1; CHumanIterator humans;
			for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId()) if (--remaining == 0)
			{
				CHumanArray::sTheObjectPtr->FreeHuman(id); return;
			}
		}
		auto remaining = Game_Random_GetNumber() % 50 + 1; CHouseIterator houses;
		auto id = houses.GetFirstHouseId(player);
		while (id != -1)
		{
			if (--remaining == 0)
			{
				CHouseArray::sTheObjectPtr->FreeHouse(id); return;
			}
			id = houses.GetNextHouseId(); if (id == -1) id = houses.GetFirstHouseId(player);
		}
	}
}
