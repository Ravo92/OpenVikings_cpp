#include "GameSetupStartup.h"
#include "C2AnimalAccess.h"
#include "C2CallbackManager.h"
#include "C2Debug.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "C2WallRoadConstruction.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00434b65; move the sector links before exploring for the new owner.
	void House_ChangePlayerId(void* house, std::uint32_t player)
	{
		House_l_DetachFromSector(house);
		Write(house, NC2Layout::HouseOffset::PlayerId, player);
		House_l_AttachToSector(house);
		House_l_Exploration_Do(house);
	}

	// Original function: FUN_0040ac27.
	std::int32_t CHouseArray::AllocateHouse(std::uint32_t player, std::uint32_t engineType, std::uint32_t stage, const void* point, bool finished, std::uint32_t flags)
	{
		const auto id = Element_Allocate();
		if (id == -1) return -1;
		if (!House_Init(mStaticVars + static_cast<std::uint32_t>(id) * ElementSize, id, player, engineType, stage, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), finished, flags))
		{
			FreeHouse(id);
			return -1;
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0A, true, id, 1, 0);
		return id;
	}

	// Original function: FUN_0040ac95; notification precedes allocation bookkeeping and Exit.
	void CHouseArray::FreeHouse(std::int32_t id)
	{
		void* house = mStaticVars + static_cast<std::uint32_t>(id) * ElementSize;
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0) return;
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0A, false, id, 0, 0);
		if (Element_Free(id, true)) House_Exit(house);
	}

	// Original function: FUN_0040ae38.
	int __cdecl Logic_EntryPoint_House_Set(unsigned int player, unsigned int engineType, unsigned int stage, const void* point, bool finished, unsigned int flags)
	{
		if (engineType == NXBasics::IntegerRange::UInt32Maximum) return -1;
		return CHouseArray::sTheObjectPtr->AllocateHouse(player, engineType, stage, point, finished, flags);
	}

	// Original function: FUN_0040ae62.
	void Logic_EntryPoint_House_Remove(const void* point)
	{
		const auto id = Logic_EntryPoint_House_GetOnPosition(point);
		if (id != -1) CHouseArray::sTheObjectPtr->FreeHouse(id);
	}

	// Original function: FUN_00430fce; the position is a by-value Windows argument.
	bool House_Init(void* house, std::int32_t id, std::uint32_t player, std::uint32_t engineType, std::uint32_t stage, std::uint32_t point, bool finished, std::uint32_t missionCheckId)
	{
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0) return false;
		// Remaster: resolve the complete native definition before placement, doors and stock initialization.
		NRemaster::GameSetup::Startup::ResolveStartupHouse(player, engineType, stage, point);
		std::memset(house, 0, CHouseArray::ElementSize);
		Write<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag, 1);
		MapMigPoint_MakeMacroGridAligned(&point);
		Write(house, NC2Layout::HouseOffset::HouseId, id);
		Write(house, NC2Layout::HouseOffset::PlayerId, player);
		Write(house, NC2Layout::HouseOffset::DevelopmentStage, stage);
		Write(house, NC2Layout::HouseOffset::EngineType, engineType);
		Write(house, NC2Layout::HouseOffset::MapPosition, point);
		Write(house, NC2Layout::HouseOffset::LogicType, Read<std::uint32_t>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType), stage * NC2Layout::RecordSize::HouseStageValueUnsigned));
		Write(house, NC2Layout::HouseOffset::TribeType, Read<std::uint32_t>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType), NC2Layout::EngineHouseLogicViewOffset::TribeType));
		if (!Tool_House_IsAllowedToBeBuild(player, engineType, stage, point, House_GetHouseTypeDataPtr(house)->ignoreContinents != 0, false)) return false;
		Write(house, NC2Layout::HouseOffset::UniqueId, Game_UniqueId_Generate());
		if (finished) House_HitPoints_Reset(house);
		else Write<std::uint32_t>(house, NC2Layout::HouseOffset::Hitpoints, 1);
		House_l_Inhabitants_Reset(house);
		House_l_DoorPoint_Calculate(house);
		GameMapModifyManager_StartGameMapModify();
		const auto* area = Read<const CMapArea*>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType)), NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned);
		std::uint32_t amounts[66];
		auto* base = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;
		auto* door = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
		area->Tool_CountGoods(base, amounts);
		const void* micro = GameMap_GetConstMapMicroPointPtr(door);
		const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0;
		const auto good = gStaticDependencyGoodByLandscape[landscape];
		if (good != 0)
		{
			micro = GameMap_GetConstMapMicroPointPtr(door);
			amounts[good] += Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0;
		}
		GameMap_MapMicroPoint_Landscape_Clear(base);
		GameMap_MapMicroPoint_Landscape_Clear(door);
		area->Landscapes_RemoveAll(base);
		MapMicroPointData_House_SetRoot(GameMap_GetMapMicroPointPtr(base), static_cast<std::uint32_t>(id));
		MapMicroPointData_House_SetDoor(GameMap_GetMapMicroPointPtr(door), static_cast<std::uint32_t>(id));
		House_Misc_Blocks_SetToMap(house);
		House_Misc_ClearLandscapesAndGuides(house);
		House_Misc_MapHeights_Modify(house);
		RoadBuildManager_ClearRoadBuildFlagForAnyPlayer(gRoadBuildManager, door);
		for (std::uint32_t index = 0; index < 66; ++index)
		{
			if (amounts[index] != 0) Map_GoodTool_PutOnMap(index, amounts[index], Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint), 10, false);
		}
		GameMapModifyManager_StopGameMapModify();
		House_l_AttachToSector(house);
		Write<std::uint8_t>(house, NC2Layout::HouseOffset::MappedFlag, 1);
		if (finished)
		{
			Write<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState, 3);
			House_l_Exploration_Do(house);
		}
		else House_l_Construction_InitConstruction(house, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage), false);
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
		if (House_GetHouseMainType(house) == 2)
		{
			const auto offset = player * NC2Layout::RecordSize::GameControlPlayer + Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType) * 0x42u;
			Write(house, NC2Layout::HouseOffset::HomeQualityEnhancer1Allowed, Read<std::uint8_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::AvailableGoods + 28));
			Write(house, NC2Layout::HouseOffset::HomeQualityEnhancers, Read<std::uint8_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::AvailableGoods + 29));
			Write(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Allowed, Read<std::uint8_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::AvailableGoods + 15));
		}
		else if (type == 1 || type == 37)
		{
			Write<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Allowed, 1);
			Write<std::uint32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Value, 1);
		}
		Write(house, NC2Layout::HouseOffset::MissionCheckId, missionCheckId);
		Write<std::uint32_t>(house, NC2Layout::HouseOffset::ProductionCounter, 1000000);
		std::memset(static_cast<std::byte*>(house) + NC2Layout::HouseOffset::LandscapeOverlayActiveFlagsSigned, 0, 10);
		return true;
	}

	// Original function: FUN_00431276.
	void __fastcall House_Misc_Destroy_House(void* house, void*)
	{
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0) return;
		House_l_ActionOverlay_StopAll(house);
		House_Inhabitants_DetachAllHumans(house);
		auto id = static_cast<std::int32_t>(Read<std::int16_t>(GameMap_GetConstMapMicroPointPtr(static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint), NC2Layout::MapMicroPointOffset::FirstMapMoveableId));
		while (id != -1)
		{
			void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
			const auto type = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType);
			if (type == 1) Human_HouseMisc_Exit(CHumanArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CHumanArray::ElementSize);
			else if (type == 2)
			{
				Animal_HouseMisc_Exit(CAnimalArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CAnimalArray::ElementSize);
			}
			else if (type != 3 && type != 4) C2Debug_WarningBox("unknown map moveable type on house\xB4s door point!");
			id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId);
		}
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 1)
		{
			const auto percentage = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone) * 100u) / Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired);
			Write<std::uint16_t>(house, NC2Layout::HouseOffset::DemolitionStepsDone, 0);
			Write<std::uint16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired, static_cast<std::uint16_t>(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(percentage) * 24u) / 100));
		}
		else Write<std::uint16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired, 24);
		if (Read<std::uint16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired) == 0)
		{
			CHouseArray::sTheObjectPtr->FreeHouse(Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId));
			return;
		}
		if (Read<std::int16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired) >= 12) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, 0x2A, MapMigPoint_GetLong(static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition), 1);
		else CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1D, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), 0, 0);
		Write<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag, 1);
	}

	// Original function: FUN_00431390.
	void House_Exit(void* house)
	{
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0) return;
		House_l_ActionOverlay_StopAll(house);
		House_Inhabitants_DetachAllHumans(house);
		auto id = static_cast<std::int32_t>(Read<std::int16_t>(GameMap_GetConstMapMicroPointPtr(static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint), NC2Layout::MapMicroPointOffset::FirstMapMoveableId));
		while (id != -1)
		{
			void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
			if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1) Human_HouseMisc_Exit(CHumanArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CHumanArray::ElementSize);
			id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId);
		}
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::MappedFlag) != 0)
		{
			GameMapModifyManager_StartGameMapModify();
			auto* base = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;
			void* micro = GameMap_GetMapMicroPointPtr(base);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2) Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 0);
			micro = GameMap_GetMapMicroPointPtr(static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2) Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 0);
			House_Misc_Blocks_ClearAndRebuild(house);
			GameMapModifyManager_StopGameMapModify();
			House_l_DetachFromSector(house);
			if (House_GetHouseMainType(house) != 6)
			{
				GameMapModifyManager_StartGameMapModify();
				// Original unresolved control bytes 005109C9/005109C8 retain their storage offsets.
				if (Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDestructionLandscapeFlag) == 0)
				{
					const auto* area = Read<const CMapArea*>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType)), NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned);
					for (std::uint32_t index = 0; index < (area->pointCount >> 1); ++index)
					{
						std::uint32_t point = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
						std::int16_t x, y;
						area->AreaTool_GetRandomPositionDelta(&x, &y);
						MapMigPoint_ConstructOffset(&point, &point, x, y, false);
						const auto engine = NC2LogicExternals::External_HouseType_GetEngineRuineLandscapeId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType), Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType));
						if (engine != -1) Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 1, 1, engine, 0, -1);
					}
				}
				if (Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag) == 0)
				{
					Write<std::uint32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer1Value, 0);
					Write<std::uint32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer0Value, 0);
					Write<std::uint32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Value, 0);
					const auto player = Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId);
					const bool ai = player < 20 && Read<std::uint32_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::Type) == 2;
					const auto mainType = House_GetHouseMainType(house);
					const auto* type = House_GetHouseTypeDataPtr(house);
					// Remaster: warehouses drop their real contents for every owner, including AI.
					if (!ai || mainType != 4)
					{
						for (std::uint32_t slot = 0; slot < type->stockCount; ++slot)
						{
							const auto good = static_cast<std::uint32_t>(type->stock[slot].goodType);
							auto amount = House_Stock_GetAmountOfGoods(house, good, mainType == 1);
							if (ai && mainType != 1 && amount > 25) amount = 25;
							Map_GoodTool_PutOnMap(good, amount, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), 10, false);
						}
					}
					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 1)
					{
						SHouseConstructionGoods goods;
						House_Construction_CalculateGoodsNeeded(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage), false, &goods);
						for (std::int32_t slot = 0; slot < goods.count; ++slot)
						{
							const auto amount = (goods.entries[slot].requiredAmount + 1u) >> 1;
							if (amount != 0) Map_GoodTool_PutOnMap(goods.entries[slot].good, amount, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), 10, false);
						}
					}
				}
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
				{
					for (std::int32_t slot = 0; slot < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++slot)
					{
						const auto offset = NC2Layout::HouseOffset::ConstructionGoodEntries + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HouseConstructionGood;
						const auto used = Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::ConsumedAmount);
						const auto amount = ((used + 1u) >> 1) + Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::DeliveredAmount) - used;
						if (amount != 0) Map_GoodTool_PutOnMap(Read<std::uint32_t>(house, offset + NC2Layout::HouseConstructionGoodOffset::GoodType), amount, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), 10, false);
					}
				}
				GameMapModifyManager_StopGameMapModify();
			}
		}
		Write<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag, 0);
	}

	// Original function: FUN_0043217c; keep the four distinct attachment passes.
	void House_Inhabitants_DetachAllHumans(void* house)
	{
		House_Inhabitants_DetachAllHumans(house, 4);
		House_Inhabitants_DetachAllHumans(house, 2);
		House_Inhabitants_DetachAllHumans(house, 8);
		House_Inhabitants_DetachAllHumans(house, 1);
	}

	// Original function: FUN_004321a3.
	void House_Inhabitants_DetachAllHumans(void* house, std::uint32_t mask)
	{
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto offset = NC2Layout::HouseOffset::Attachments + slot * NC2Layout::RecordSize::HouseAttachment;
			const auto role = Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask);
			if ((role & mask) == 0) continue;
			Write<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask, static_cast<std::uint8_t>(role & ~mask));
			void* human = CHumanArray::mStaticVars + Read<std::uint32_t>(house, offset + NC2Layout::HouseAttachmentOffset::HumanId) * CHumanArray::ElementSize;
			if (mask == 1) Human_DetachFromHomeHouse(human);
			else if (mask == 2 || mask == 4 || mask == 8) Human_DetachFromWorkHouse(human);
			if (mask == 8)
			{
				Write<std::uint32_t>(house, offset + NC2Layout::HouseAttachmentOffset::TrainingJobType, 0);
				Write<std::uint32_t>(house, offset + NC2Layout::HouseAttachmentOffset::TrainingGoodType, 0);
			}
			if (Read<std::uint8_t>(house, offset + NC2Layout::HouseAttachmentOffset::RoleMask) == 0) Write<std::int32_t>(house, offset + NC2Layout::HouseAttachmentOffset::HumanId, -1);
		}
	}

	// Original function: FUN_004328f8.
	void House_l_Inhabitants_Reset(void* house)
	{
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			Write<std::uint8_t>(house, NC2Layout::HouseOffset::Attachments + slot * NC2Layout::RecordSize::HouseAttachment, 0);
			Write<std::int32_t>(house, NC2Layout::HouseOffset::AttachmentHumanIds + slot * NC2Layout::RecordSize::HouseAttachment, -1);
		}
	}

	// Original function: FUN_00434197; signed comparison matches the native index check.
	void House_ActionOverlay_Stop(void* house, std::int32_t overlay)
	{
		if (overlay >= 5) return;
		Write<std::uint32_t>(house, NC2Layout::HouseOffset::ActionOverlayCounters + static_cast<std::uint32_t>(overlay) * NC2Layout::RecordSize::HouseCounterUnsigned, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1C, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), overlay, 0);
	}

	// Original function: FUN_004341de.
	void House_l_ActionOverlay_StopAll(void* house)
	{
		for (std::int32_t overlay = 0; overlay < 5; ++overlay)
		{
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ActionOverlayCounters + static_cast<std::uint32_t>(overlay) * NC2Layout::RecordSize::HouseCounterUnsigned) != 0) House_ActionOverlay_Stop(house, overlay);
		}
	}

	// Original function: FUN_004342ae.
	void House_Misc_Blocks_ClearAndRebuild(const void* house)
	{
		const auto* base = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;
		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		Read<const CMapArea*>(definition, NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned)->WalkBlock_Set(base, false);
		definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto* area = Read<const CMapArea*>(definition, NC2Layout::EngineHouseLogicViewOffset::BuildArea);
		area->BuildBlock_Set(base, false);
		std::int16_t rectangle[4] = { static_cast<std::int16_t>(Read<std::uint16_t>(base, NC2Layout::MapCoordinatePairOffset::X) + area->minX - 15), static_cast<std::int16_t>(Read<std::uint16_t>(base, NC2Layout::MapCoordinatePairOffset::Y) + area->minY - 15), static_cast<std::int16_t>(area->maxX - area->minX + 30), static_cast<std::int16_t>(area->maxY - area->minY + 30) };
		MapMigRectangle_CutInsideMap(rectangle);
		GameMap_Misc_LetBlocksBeReDrawn(rectangle);
	}

	// Original function: FUN_0043442c; each macro height moves at most ten levels.
	void House_Misc_MapHeights_Modify(const void* house)
	{
		const auto* base = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;
		std::uint32_t macro;
		MapMagPoint_ConstructFromMig(&macro, base, false);
		const auto height = Read<std::uint8_t>(GameMap_GetConstMapMacroPointPtr(&macro), NC2Layout::MapMacroPointOffset::Height);
		const auto* area = Read<const CMapArea*>(NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType)), NC2Layout::EngineHouseLogicViewOffset::BuildArea);
		for (std::uint32_t row = 0; row < area->rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, base, area->rows[row].x, area->rows[row].y, false);
			for (std::uint32_t column = 0; column < area->rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point) && MapMigPoint_IsOnMacroGrid(&point))
				{
					MapMagPoint_ConstructFromMig(&macro, &point, false);
					void* data = GameMap_GetMapMacroPointPtr(&macro);
					const auto previous = Read<std::uint8_t>(data, NC2Layout::MapMacroPointOffset::Height);
					auto delta = static_cast<std::int32_t>(height) - previous;
					if (delta < -10) delta = -10;
					if (delta > 10) delta = 10;
					Write<std::uint8_t>(data, NC2Layout::MapMacroPointOffset::Height, static_cast<std::uint8_t>(previous + delta));
					GameMapModifyManager_GameMapModified(13, &point, 0);
				}
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00434519; 004345F2 selects the original two house lists.
	void House_l_AttachToSector(void* house)
	{
		const SSectorGridCursor cursor(Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
		auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
		void* sector = manager->GetSectorData() + (static_cast<std::uint32_t>(cursor.y) * static_cast<std::uint32_t>(manager->GetGridWidth()) + static_cast<std::uint32_t>(cursor.x)) * 0x540u;
		const auto offset = House_GetPlayerId(house) * 64u + (House_IsMilitarySectorType(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType)) ? 0x28u : 0x10u);
		const auto previous = Read<std::int32_t>(sector, offset);
		Write(sector, offset, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
		Write(house, NC2Layout::HouseOffset::NextSectorHouseId, previous);
		if (previous != -1) Write(CHouseArray::mStaticVars + static_cast<std::uint32_t>(previous) * CHouseArray::ElementSize, NC2Layout::HouseOffset::PreviousSectorHouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
		Write<std::int32_t>(house, NC2Layout::HouseOffset::PreviousSectorHouseId, -1);
		Write(sector, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(sector, offset + NC2Layout::SectorObjectListOffset::Count) + 1u);
	}

	// Original function: FUN_00434611; native non-head removal only updates the predecessor's next link.
	void House_l_DetachFromSector(void* house)
	{
		const SSectorGridCursor cursor(Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
		auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
		void* sector = manager->GetSectorData() + (static_cast<std::uint32_t>(cursor.y) * static_cast<std::uint32_t>(manager->GetGridWidth()) + static_cast<std::uint32_t>(cursor.x)) * 0x540u;
		const auto offset = House_GetPlayerId(house) * 64u + (House_IsMilitarySectorType(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType)) ? 0x28u : 0x10u);
		const auto id = Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId);
		auto previous = Read<std::uint32_t>(sector, offset);
		if (previous == id)
		{
			const auto next = Read<std::int32_t>(house, NC2Layout::HouseOffset::NextSectorHouseId);
			Write(sector, offset, next);
			if (next != -1) Write<std::int32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(next) * CHouseArray::ElementSize, NC2Layout::HouseOffset::PreviousSectorHouseId, -1);
		}
		else
		{
			for (;;)
			{
				void* entry = CHouseArray::mStaticVars + previous * CHouseArray::ElementSize;
				const auto next = Read<std::uint32_t>(entry, NC2Layout::HouseOffset::NextSectorHouseId);
				if (next == id)
				{
					Write(entry, NC2Layout::HouseOffset::NextSectorHouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::NextSectorHouseId));
					break;
				}
				previous = next;
			}
		}
		Write(sector, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(sector, offset + NC2Layout::SectorObjectListOffset::Count) - 1u);
	}
}
