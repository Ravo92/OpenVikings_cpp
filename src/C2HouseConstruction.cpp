#include "C2CallbackManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GuideAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2UserMessageAccess.h"
#include "C2WallRoadConstruction.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_004340f5.
	void House_Door_GetEntryPoint(const void* house, void* point)
	{
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(House_Door_GetEntryPointRef(house), NC2Layout::MapCoordinatePairOffset::Coordinates));
	}

	// Original function: FUN_00423071; Windows-only outlined entry, no separate Mac name established.
	void House_HitPoints_ModifyValue(void* house, std::int32_t delta)
	{
		if ((Read<std::uint32_t>(house, NC2Layout::HouseOffset::BehaviourFlags) & 1u) != 0 && delta < 0)
		{

			return;
		}

		const auto value = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(house, NC2Layout::HouseOffset::Hitpoints) + static_cast<std::uint32_t>(delta));
		Write(house, NC2Layout::HouseOffset::Hitpoints, value);

		if (value < 0)
		{
			Write<std::uint32_t>(house, NC2Layout::HouseOffset::Hitpoints, 0);
		}
		else if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 1 && House_Repair_IsFinished(house))
		{
			House_HitPoints_Reset(house);
		}
	}

	// Original function: FUN_0043222f; multiple role bits retain independent update calls.
	void House_Inhabitants_UpdateAllHumans(void* house)
	{
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::Attachments + slot * NC2Layout::RecordSize::HouseAttachment;

			if (Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) == 0)
				continue;

			void* human = CHumanArray::mStaticVars + Read<std::uint32_t>(house, offset + NC2Layout::HouseAttachmentOffset::HumanId) * CHumanArray::ElementSize;

			if ((Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) & 4u) != 0)
				Human_UpdateCurrentBuildHouse(human);

			if ((Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) & 2u) != 0)
				Human_UpdateCurrentWorkHouse(human);

			if ((Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) & 8u) != 0)
				Human_UpdateCurrentWorkHouse(human);

			if ((Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) & 1u) != 0)
				Human_UpdateCurrentHomeHouse(human);
		}
	}

	// Original function: FUN_00432d6b; an outstanding reservation suppresses a new material search.
	bool House_Construction_IsNeedingGoods(const void* house)
	{
		for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;

			if (Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::RequiredAmount) > Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount) + Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::IncomingAmount))
				return true;
		}

		return false;
	}

	// Original function: FUN_00432df3.
	bool House_Construction_CanUseOneGoodNow(const void* house)
	{
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepLimit))
			return false;

		for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;

			if (Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::ConsumedAmount) != Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount))
				return true;
		}

		return false;
	}

	// Original function: FUN_00432e2d; consume the first available material in descriptor order.
	bool House_Construction_DoUseOneGood(void* house)
	{
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepLimit))
			return false;

		for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;

			if (Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::ConsumedAmount) == Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount))
				continue;

			Write(house, offset + NC2Layout::HouseConstructionGoodOffset::ConsumedAmount, Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::ConsumedAmount) + 1u);
			const auto consumed = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsConsumed) + 1u;
			Write(house, NC2Layout::HouseOffset::ConstructionGoodsConsumed, consumed);
			Write(house, NC2Layout::HouseOffset::ConstructionStepLimit, consumed * 30u);
			return true;
		}

		return false;
	}

	// Original function: FUN_00432e8b.
	bool House_Construction_IsUseGoodRunning(const void* house)
	{

		return Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepLimit);
	}

	// Original function: FUN_00432ea5; repairs and construction deliberately share this Windows entry.
	bool House_Construction_DoOneStep(void* house)
	{
		if (!House_Repair_IsFinished(house))
		{
			if (House_Repair_IsFinished(house))
				return false;

			House_HitPoints_ModifyValue(house, 100);
			return true;
		}

		const auto state = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState);

		if (state == 1 && Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepLimit))
		{
			const auto before = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) * 100u) / Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired);
			const auto step = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) + 1u;
			Write(house, NC2Layout::HouseOffset::ConstructionStepsDone, step);

			if (NXBasics::Runtime_TimeSigned32(step) >= Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired))
			{
				Write<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState, 3);
				House_l_Exploration_Do(house);
				House_UserMessage_SendToUserMessage(house, 47);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), 5, 0);
				House_Inhabitants_UpdateAllWorkers(house);
			}

			const auto after = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 ? 100 : NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) * 100u) / Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired);
			const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
			const auto gain = Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::MaximumHitpointsUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned) * static_cast<std::uint32_t>(after - before) / 100u;
			House_HitPoints_ModifyValue(house, NXBasics::Runtime_TimeSigned32(gain));
			return true;
		}

		if (state != 2 || Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) >= Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepLimit))
			return false;

		const auto step = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) + 1u;
		Write(house, NC2Layout::HouseOffset::ConstructionStepsDone, step);

		if (NXBasics::Runtime_TimeSigned32(step) < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired))
			return true;

		Write<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState, 3);
		GameMapModifyManager_StartGameMapModify();
		void* door = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
		void* micro = GameMap_GetMapMicroPointPtr(door);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2)
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 0);

		Write(house, NC2Layout::HouseOffset::DevelopmentStage, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) + 1u);
		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		Write(house, NC2Layout::HouseOffset::LogicType, Read<std::uint32_t>(definition, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned));
		definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto health = Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::MaximumHitpointsUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned);
		const bool temple = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 1;
		Write(house, NC2Layout::HouseOffset::Hitpoints, health);

		if (temple)
			Write<std::uint32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Value, 1);

		House_l_DoorPoint_Calculate(house);
		GameMap_MapMicroPoint_Landscape_Clear(door);
		micro = GameMap_GetMapMicroPointPtr(door);
		// Original outlined function 00431DC6; preserve the lower flag bit and other micro-point data.

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 0)
		{
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData, (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & NXBasics::BitMask::ByteExceptBit0) | 2u);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 2);
			Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId, static_cast<std::uint16_t>(Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId)));
		}

		House_Misc_Blocks_SetToMap(house);
		House_Misc_ClearLandscapesAndGuides(house);
		GameMapModifyManager_StopGameMapModify();
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1E, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), 0, 0);
		House_UserMessage_SendToUserMessage(house, 48);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), 5, 0);
		House_Inhabitants_UpdateAllWorkers(house);

		return true;
	}

	// Original function: FUN_004333c7.
	void House_l_Exploration_Do(const void* house)
	{
		GameMap_Exploration_DoExplore(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 1 ? 30u : 15u);
	}

	// Original function: FUN_00433645.
	void House_UserMessage_SendToUserMessage(const void* house, std::uint32_t type)
	{
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::Type, type);
		Write(message, NC2Layout::ToUserMessageOffset::HouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_00434101; raw offset arithmetic is the outlined 0042AD91 operation.
	void House_l_DoorPoint_Calculate(void* house)
	{
		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto offset = 0x188u + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * 4u;
		std::uint32_t door;
		MapMigPoint_ConstructOffset(&door, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, Read<std::int16_t>(definition, offset), Read<std::int16_t>(definition, offset + NC2Layout::MapCoordinatePairOffset::Y), false);
		Write(house, NC2Layout::HouseOffset::DoorEntryPoint, door);
		House_Inhabitants_UpdateAllHumans(house);
	}

	// Original function: FUN_0043435a; Windows-only entry, no separate Mac name established.
	void House_Misc_ClearLandscapesAndGuides(const void* house)
	{
		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto* area = Read<const CMapArea*>(definition, NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned);

		for (std::uint32_t row = 0; row < area->rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, area->rows[row].x, area->rows[row].y, false);

			for (std::uint32_t column = 0; column < area->rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point))
				{
					const void* micro = GameMap_GetConstMapMicroPointPtr(&point);

					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0)
						GameMap_MapMicroPoint_Landscape_Clear(&point);

					RoadBuildManager_ClearRoadBuildFlagForAnyPlayer(gRoadBuildManager, &point);

					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 3)
						GuideManager_RemoveGuide(gGuideManager, nullptr, Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId));
				}

				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00434763.
	void House_HitPoints_Reset(void* house)
	{
		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		Write(house, NC2Layout::HouseOffset::Hitpoints, Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::MaximumHitpointsUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned));
	}
}