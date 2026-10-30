#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HouseUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticHouseTypeData.h"
#include "C2WallRoadConstruction.h"
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_0043b8f9; all eligible house flags precede the separate gate pass.
	void Tool_GlobalDefendMode_Start(std::uint32_t player)
	{
		using namespace NC2Lifetime;
		{
			CHouseIterator houses;

			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

				if (House_CanEnableDefenceMode(house))
					House_SetDefenceModeFlag(house, true);
			}
		}
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			std::uint32_t point = Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::MapPosition);

			for (std::uint32_t ring = 1; ring < 50; ++ring)
			{
				MapMigPoint_MoveCursorUnwrapped(&point, 4, 1);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					for (std::uint32_t step = 0; step < ring; ++step)
					{
						MapMigPoint_MoveCursorUnwrapped(&point, direction, 1);

						if (!MapMigPoint_IsInsideMap(&point))
							continue;

						const void* micro = GameMap_GetConstMapMicroPointPtr(&point);

						if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != static_cast<std::int32_t>(player))
							continue;

						const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

						if (type == 84 || type == 83)
							MapTool_WallGate_SetState(player, &point, false);
					}
				}
			}
		}
	}

	// Original function: FUN_0043ba13; the distinct stop pass retains the native opposite gate argument.
	void Tool_GlobalDefendMode_Stop(std::uint32_t player)
	{
		using namespace NC2Lifetime;
		{
			CHouseIterator houses;

			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

				if (House_CanEnableDefenceMode(house))
					House_SetDefenceModeFlag(house, false);
			}
		}
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			std::uint32_t point = Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::MapPosition);

			for (std::uint32_t ring = 1; ring < 50; ++ring)
			{
				MapMigPoint_MoveCursorUnwrapped(&point, 4, 1);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					for (std::uint32_t step = 0; step < ring; ++step)
					{
						MapMigPoint_MoveCursorUnwrapped(&point, direction, 1);

						if (!MapMigPoint_IsInsideMap(&point))
							continue;

						const void* micro = GameMap_GetConstMapMicroPointPtr(&point);

						if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != static_cast<std::int32_t>(player))
							continue;

						const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

						if (type == 84 || type == 83)
							MapTool_WallGate_SetState(player, &point, true);
					}
				}
			}
		}
	}

	using namespace NC2Lifetime;

	// Original function: FUN_0040b04b; an unallocated house leaves output untouched.
	bool Logic_UserControl_GetHouseCommands(std::int32_t id, std::int32_t player, SUserCommandsForHouseHandler* commands)
	{
		const auto* house = CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * CHouseArray::RuntimeElementSize;

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0)
			return false;

		House_UserControl_GetCommands(house, player, commands);

		return true;
	}

	// Original function: FUN_004329b6.
	bool House_Construction_CanBeUpgraded(const void* house)
	{
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
			return false;

		const auto* stages = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		const auto nextStage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) + 1u;

		if (nextStage >= Read<std::uint32_t>(stages, NC2Layout::EngineHouseLogicViewOffset::DevelopmentStageCount))
			return false;

		const auto nextLogicType = Read<std::uint32_t>(stages, nextStage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		const auto permissionIndex = Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer + 0x18u + Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType) * 55u + nextLogicType;

		return Read<std::uint8_t>(gGameControl, permissionIndex + NC2Layout::PlayerAllowedThingsOffset::AllowedHouses) != 0 && Read<std::uint8_t>(gGameControl, permissionIndex + NC2Layout::PlayerAllowedThingsOffset::AvailableHouses) != 0;
	}

	// Original function: FUN_00433549.
	void House_UserControl_GetCommands(const void* house, std::int32_t player, SUserCommandsForHouseHandler* commands)
	{
		commands->Init();

		if (static_cast<std::uint32_t>(player) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
		{

			return;
		}

		commands->AddCommand(1);

		if (House_Construction_CanBeUpgraded(house))
			commands->AddCommand(2);

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 2)
			commands->AddCommand(3);

		const auto* type = gStaticHouseTypeData + Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

		if (type->mainType == 2)
		{
			commands->AddCommand(Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancers) != 0 ? 5 : 4);
			commands->AddCommand(Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer1Allowed) != 0 ? 7 : 6);

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) >= 2)
				commands->AddCommand(Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Allowed) != 0 ? 9 : 8);
		}

		if (House_CanEnableDefenceMode(house))
			commands->AddCommand(Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) != 0 ? 15 : 14);
	}

	// Original function: FUN_00433621; Windows-only outlined defense-mode predicate.
	bool House_CanEnableDefenceMode(const void* house)
	{

		return Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && (gStaticHouseTypeData + Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType))->canEnableDefenceMode != 0;
	}

	// Original function: FUN_004408a6.
	void SUserCommandsForHouseHandler::Init()
	{
		count = 0;
		std::memset(commands, 0, sizeof(commands));
	}

	// Original function: FUN_004408b6.
	void SUserCommandsForHouseHandler::AddCommand(std::uint32_t command)
	{
		if (count != 20)
			commands[count++] = command;
	}
}