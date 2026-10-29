#include "C2ChecksumTools.h"
#include "C2DefaultInteraction.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanTaskAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticLandscapeTypeData.h"
#include "C2WallRoadConstruction.h"
#include "C2WallRoadPathfinder.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Complete Windows region 004F39C0..004F39E3: three distinct direction tables.
		constexpr std::uint32_t gPossibleWallGateDirections[3]{ 3, 4, 2 };
		constexpr std::uint32_t gSetWallGateDirections[3]{ 3, 4, 2 };
		constexpr std::uint32_t gWallGateStateDirections[3]{ 3, 4, 2 };

		// Original function: FUN_00427b6d; Windows-only outlined removal leaf.
		void WallGate_RemoveElement(std::uint32_t player, const void* point)
		{
			if (!MapMigPoint_IsInsideMap(point))
			{

				return;
			}

			auto* micro = GameMap_GetMapMicroPointPtr(point);
			const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && (type == SwitchIds::WallLandscapeType::Wall || type == SwitchIds::WallLandscapeType::GateOpen || type == SwitchIds::WallLandscapeType::GateClosed) && Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) == static_cast<std::int32_t>(player))
				GameMap_MapMicroPoint_Landscape_Clear(point);
		}

		// Original function: FUN_00427bfe; Windows-only outlined wall eligibility leaf.
		bool WallGate_IsWallElementUsable(std::uint32_t player, const void* point)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == SwitchIds::WallLandscapeType::Wall && Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) == static_cast<std::int32_t>(player) && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) >= gStaticLandscapeTypeStorage.records[SwitchIds::WallLandscapeType::Wall].maximumValency * 75u / 100u;
		}
	}

	// Original function: FUN_004274c7; hash the player count, then each allocated 260h record in player order.
	std::uint32_t WallBuildManager_Checksum_Calculate(const void* manager)
	{
		auto checksum = Tool_Checksum_Calculate(manager, NC2Layout::RecordSize::BuildPlayerCount);

		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			const auto* data = Read<std::byte* const*>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

			if (data != nullptr)
				Tool_Checksum_CalculateAndCombine(&checksum, data, NC2Layout::RecordSize::WallBuildPlayer);
		}

		return checksum;
	}

	// Original function: FUN_00428171; unused entries and padding are part of the native 4B8h record hash.
	std::uint32_t RoadBuildManager_Checksum_Calculate(const void* manager)
	{
		auto checksum = Tool_Checksum_Calculate(manager, NC2Layout::RecordSize::BuildPlayerCount);

		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			const auto* data = Read<std::byte* const*>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

			if (data != nullptr)
				Tool_Checksum_CalculateAndCombine(&checksum, data, NC2Layout::RecordSize::RoadBuildPlayer);
		}

		return checksum;
	}

	// Original function: FUN_00427b04; radius one removes only the center.
	void MapTool_WallGate_Remove(std::uint32_t player, std::uint32_t point, std::uint32_t radius)
	{
		GameMapModifyManager_StartGameMapModify();
		WallGate_RemoveElement(player, &point);

		for (std::uint32_t ring = 1; ring < radius; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					WallGate_RemoveElement(player, &point);
				}
			}
		}

		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_00427bbc.
	bool MapTool_WallGate_IsPossibleMapPosition(std::uint32_t player, const void* point)
	{
		if (!WallGate_IsWallElementUsable(player, point))
			return false;

		for (auto direction : gPossibleWallGateDirections)
		{
			if (ls_MapTool_WallGate_IsPossibleMapPosition_GatePossible(player, point, direction))
				return true;
		}

		return false;
	}

	// Original function: FUN_00427c51; five wall points and both four-point passages are checked.
	bool ls_MapTool_WallGate_IsPossibleMapPosition_GatePossible(std::uint32_t player, const void* source, std::uint32_t direction)
	{
		if (direction >= 6 || !WallGate_IsWallElementUsable(player, source))
			return false;

		const auto height = GameMap_GetHeightAtMapMigPoint(source);
		auto point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(&point, VE_HexagonDirection_OppositeDirection(direction), 2);

		for (std::uint32_t i = 0; i < 5; ++i)
		{
			if (!MapMigPoint_IsInsideMap(&point) || !WallGate_IsWallElementUsable(player, &point))
				return false;

			const auto difference = static_cast<std::int32_t>(height) - GameMap_GetHeightAtMapMigPoint(&point);

			if (difference > 8 || difference < -8)
				return false;

			MapMigPoint_MoveCursorUnwrapped(point, direction);
		}

		for (std::uint32_t side = 0; side < 2; ++side)
		{
			point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(&point, VE_HexagonDirection_OppositeDirection(direction), 2);
			MapMigPoint_MoveCursorUnwrapped(point, (direction + (side == 0 ? 2u : 4u)) % 6u);

			for (std::uint32_t i = 0; i < 4; ++i)
			{
				if (!MapMigPoint_IsInsideMap(&point) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
					return false;

				MapMigPoint_MoveCursorUnwrapped(point, direction);
			}
		}

		return true;
	}

	// Original function: FUN_00427d89; the native final candidate is retained even when no candidate succeeds.
	void MapTool_WallGate_SetIntoWall(std::uint32_t player, const void* source)
	{
		std::uint32_t direction = NXBasics::IntegerRange::UInt32Maximum;

		for (auto candidate : gSetWallGateDirections)
		{
			direction = candidate;

			if (ls_MapTool_WallGate_IsPossibleMapPosition_GatePossible(player, source, direction))
				break;
		}

		if (direction == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		GameMapModifyManager_StartGameMapModify();
		auto point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(&point, VE_HexagonDirection_OppositeDirection(direction), 1);

		for (std::uint32_t i = 0; i < 3; ++i)
		{
			GameMap_MapMicroPoint_Landscape_Clear(&point);
			MapMigPoint_MoveCursorUnwrapped(point, direction);
		}

		const auto engine = NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(direction, true);
		const auto* logic = NC2LogicExternals::External_Landscape_GetLogicDataPtr(engine);
		GameMap_MapMicroPoint_Landscape_SetType(source, Read<std::uint32_t>(logic, NC2Layout::LandscapeLogicDataOffset::LogicType), Read<std::uint32_t>(logic, NC2Layout::LandscapeLogicDataOffset::MaximumValency), engine, true, player);
		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_00427e27; command flag means closed, the external graphic flag means open.
	void MapTool_WallGate_SetState(std::uint32_t player, const void* point, bool closed)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Landscape || (type != SwitchIds::WallLandscapeType::GateOpen && type != SwitchIds::WallLandscapeType::GateClosed) || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != static_cast<std::int32_t>(player))
			return;

		const auto current = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point);
		std::uint32_t direction = NXBasics::IntegerRange::UInt32Maximum;

		for (auto candidate : gWallGateStateDirections)
		{
			if (NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(candidate, true) == current || NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(candidate, false) == current)
			{
				direction = candidate;
				break;
			}
		}

		if (direction == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		GameMapModifyManager_StartGameMapModify();
		const auto engine = NC2LogicExternals::External_Misc_GetEngineWallGateLandscapeId(direction, !closed);
		const auto* logic = NC2LogicExternals::External_Landscape_GetLogicDataPtr(engine);
		const auto valency = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
		GameMap_MapMicroPoint_Landscape_SetType(point, Read<std::uint32_t>(logic, NC2Layout::LandscapeLogicDataOffset::LogicType), valency, engine, true, player);
		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_004278e7; Windows folds the road/wall point lookup.
	void* RoadBuildManager_GetRoadBuildDataPtr(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount))
			return nullptr;

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		if (data == nullptr)
			return nullptr;

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint) == Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				return data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint;
		}

		return nullptr;
	}

	// Original function: FUN_0042856f; Windows folds the road/wall player lookup.
	void* RoadBuildManager_GetRoadBuildDataPtr(void* manager, std::int32_t player)
	{

		return player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) ? nullptr : Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
	}

	// Original function: FUN_00428586; Windows folds the road/wall any-player predicate.
	bool RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(const void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte* const*>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr && RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
				return true;
		}

		return false;
	}

	// Original function: FUN_0042750a.
	void __fastcall WallBuildManager_SetWallBuildFlag(void* manager, void*, std::uint32_t player, const void* point)
	{
		if (!CMapPathfinderSpecialWallRoadElement::IsPointOkForBuilding(player, false, point))
			return;

		if (static_cast<std::int32_t>(player) >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr)
			return;

		if (RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
			return;

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
		const auto count = Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount);

		if (count == NC2Runtime::BuildLimits::WallPoints)
		{

			return;
		}

		Write(data, NC2Layout::BuildPlayerOffset::Points + count * NC2Layout::RecordSize::BuildPoint, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + count * NC2Layout::RecordSize::BuildPoint, 0);
		Write(data, NC2Layout::BuildPlayerOffset::PointCount, count + 1u);
		auto* micro = GameMap_GetMapMicroPointPtr(point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) | NC2Runtime::MapTerrainFlags::WallBuildPendingUnsigned);
	}

	// Original function: FUN_00427583.
	bool WallBuildManager_ClearWallBuildFlag(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
			return false;

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint) != Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				continue;

			if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) != 0)
				Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) - 1u);

			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			std::memmove(data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, data + NC2Layout::BuildPlayerOffset::SecondPoint + i * NC2Layout::RecordSize::BuildPoint, (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - i - 1) * NC2Layout::RecordSize::BuildPoint);
			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			Write(data, NC2Layout::BuildPlayerOffset::PointCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - 1u);
			break;
		}

		if (!RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
		{
			auto* micro = GameMap_GetMapMicroPointPtr(point);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::WallBuildPendingUnsigned);
		}

		return true;
	}

	// Original function: FUN_0042765e; removal restarts the scan at entry zero.
	void __fastcall WallBuildManager_ClearWallBuildFlagArea(void* manager, void*, std::uint32_t player, const void* center, std::uint32_t radius)
	{
		if (static_cast<std::int32_t>(player) >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr)
		{

			return;
		}

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player], NC2Layout::BuildPlayerOffset::PointCount);)
		{
			auto point = Read<std::uint32_t>(Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player], NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint);
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(center, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(center, NC2Layout::MapCoordinatePairOffset::Y));

			if (static_cast<std::uint32_t>(distance) <= radius)
			{
				WallBuildManager_ClearWallBuildFlag(manager, player, &point);
				i = 0;
			}
			else
			{
				++i;
			}
		}
	}

	// Original function: FUN_004277c6; kept separate from ClearWallBuildFlag.
	static void WallBuildManager_l_FinishWall(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
		{

			return;
		}

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint) != Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				continue;

			if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) != 0)
				Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) - 1u);

			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			std::memmove(data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, data + NC2Layout::BuildPlayerOffset::SecondPoint + i * NC2Layout::RecordSize::BuildPoint, (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - i - 1) * NC2Layout::RecordSize::BuildPoint);
			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			Write(data, NC2Layout::BuildPlayerOffset::PointCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - 1u);
			break;
		}

		if (!RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
		{
			auto* micro = GameMap_GetMapMicroPointPtr(point);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::WallBuildPendingUnsigned);
		}
	}

	// Original function: FUN_0042779d.
	static void WallBuildManager_l_FinishWallForAnyPlayer(void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr)
				WallBuildManager_l_FinishWall(manager, player, point);
		}
	}

	// Original function: FUN_004276ef; pending flags are removed before attempting the map edit.
	bool WallBuildManager_FinishWall(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
			return false;

		WallBuildManager_l_FinishWallForAnyPlayer(manager, point);
		auto* micro = GameMap_GetMapMicroPointPtr(point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::WallBuildPendingUnsigned);
		GameMapModifyManager_StartGameMapModify();
		const auto* landscape = GameMap_GetConstMapMicroPointPtr(point);

		if (Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::ObjectData) == 1)
			GameMap_MapMicroPoint_Landscape_Clear(point);

		const auto engine = NC2LogicExternals::External_Misc_GetEngineWallElementLandscapeId();
		const auto* logic = NC2LogicExternals::External_Landscape_GetLogicDataPtr(engine);
		const bool result = GameMap_MapMicroPoint_Landscape_SetType(point, Read<std::uint32_t>(logic, NC2Layout::LandscapeLogicDataOffset::LogicType), Read<std::uint32_t>(logic, NC2Layout::LandscapeLogicDataOffset::MaximumValency), engine, true, player);
		GameMapModifyManager_StopGameMapModify();

		return result;
	}

	// Original function: FUN_00427936.
	void WallBuildManager_ClearWallBuildFlagForAnyPlayer(void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr)
				WallBuildManager_ClearWallBuildFlag(manager, player, point);
		}
	}

	// Original function: FUN_0042795f; only idle builders without a build-site attachment are assigned.
	void __fastcall WallBuildManager_DoOneGameSecond(void* manager, void*)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

			if (data == nullptr || Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) >= NC2Runtime::BuildLimits::WallWorkers)
				continue;

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
			{
				if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) == 0)
				{
					CHumanIterator iterator;

					for (auto id = iterator.GetFirstHumanId(player, data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, NC2Runtime::BuildLimits::WorkerSearchRadius); id != -1; id = iterator.GetNextHumanId())
					{
						auto* human = const_cast<void*>(GetHumanPtr(id));

						if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Builder || Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != SwitchIds::GoodCirculationMode::None || Human_IsAttachedToBuildSite(human))
							continue;

						Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
						data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
						Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint));
						Human_WorkCenterMarker_Update(human);
						data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
						Write<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint, 1);
						Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) + 1u);
						break;
					}
				}

				data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

				if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) >= NC2Runtime::BuildLimits::WallWorkers)
					break;
			}
		}
	}

	// Original function: FUN_004281b4.
	void __fastcall RoadBuildManager_SetRoadBuildFlag(void* manager, void*, std::uint32_t player, const void* point)
	{
		if (!CMapPathfinderSpecialWallRoadElement::IsPointOkForBuilding(player, true, point))
			return;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::TerrainFlags) & NC2Runtime::MapTerrainFlags::RoadUnsigned) != 0)
			return;

		if (static_cast<std::int32_t>(player) >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr)
			return;

		if (RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
			return;

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
		const auto count = Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount);

		if (count == NC2Runtime::BuildLimits::RoadPoints)
		{

			return;
		}

		Write(data, NC2Layout::BuildPlayerOffset::Points + count * NC2Layout::RecordSize::BuildPoint, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + count * NC2Layout::RecordSize::BuildPoint, 0);
		Write(data, NC2Layout::BuildPlayerOffset::PointCount, count + 1u);
		auto* micro = GameMap_GetMapMicroPointPtr(point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) | NC2Runtime::MapTerrainFlags::RoadBuildPendingUnsigned);
	}

	// Original function: FUN_0042823c.
	bool RoadBuildManager_ClearRoadBuildFlag(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
			return false;

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint) != Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				continue;

			if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) != 0)
				Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) - 1u);

			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			std::memmove(data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, data + NC2Layout::BuildPlayerOffset::SecondPoint + i * NC2Layout::RecordSize::BuildPoint, (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - i - 1) * NC2Layout::RecordSize::BuildPoint);
			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			Write(data, NC2Layout::BuildPlayerOffset::PointCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - 1u);
			break;
		}

		if (!RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
		{
			auto* micro = GameMap_GetMapMicroPointPtr(point);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::RoadBuildPendingUnsigned);
		}

		return true;
	}

	// Original function: FUN_00428317; removal restarts the scan at entry zero.
	void __fastcall RoadBuildManager_ClearRoadBuildFlagArea(void* manager, void*, std::uint32_t player, const void* center, std::uint32_t radius)
	{
		if (static_cast<std::int32_t>(player) >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr)
		{

			return;
		}

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player], NC2Layout::BuildPlayerOffset::PointCount);)
		{
			auto point = Read<std::uint32_t>(Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player], NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint);
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(center, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(center, NC2Layout::MapCoordinatePairOffset::Y));

			if (static_cast<std::uint32_t>(distance) <= radius)
			{
				RoadBuildManager_ClearRoadBuildFlag(manager, player, &point);
				i = 0;
			}
			else
			{
				++i;
			}
		}
	}

	// Original function: FUN_00428466; kept separate from ClearRoadBuildFlag.
	static void RoadBuildManager_l_FinishRoad(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
		{

			return;
		}

		auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
		{
			if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint) != Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates))
				continue;

			if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) != 0)
				Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) - 1u);

			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			std::memmove(data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, data + NC2Layout::BuildPlayerOffset::SecondPoint + i * NC2Layout::RecordSize::BuildPoint, (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - i - 1) * NC2Layout::RecordSize::BuildPoint);
			data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
			Write(data, NC2Layout::BuildPlayerOffset::PointCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount) - 1u);
			break;
		}

		if (!RoadBuildManager_l_IsRoadBuildFlagSetForAnyPlayer(manager, point))
		{
			auto* micro = GameMap_GetMapMicroPointPtr(point);
			Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::RoadBuildPendingUnsigned);
		}
	}

	// Original function: FUN_0042843d.
	static void RoadBuildManager_l_FinishRoadForAnyPlayer(void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr)
				RoadBuildManager_l_FinishRoad(manager, player, point);
		}
	}

	// Original function: FUN_004283a8; pending flags are removed before attempting the map edit.
	bool RoadBuildManager_FinishRoad(void* manager, std::int32_t player, const void* point)
	{
		if (player >= Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount) || Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] == nullptr || !RoadBuildManager_IsRoadBuildFlagSetForPlayer(manager, player, point))
			return false;

		RoadBuildManager_l_FinishRoadForAnyPlayer(manager, point);
		auto* micro = GameMap_GetMapMicroPointPtr(point);
		Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & ~NC2Runtime::MapTerrainFlags::RoadBuildPendingUnsigned);
		GameMapModifyManager_StartGameMapModify();
		const bool result = GameMap_MapMicroPoint_Road_Set(point, true);
		const auto* landscape = GameMap_GetConstMapMicroPointPtr(point);

		if (result && Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::ObjectData) == 1)
			GameMap_MapMicroPoint_Landscape_Clear(point);

		GameMapModifyManager_StopGameMapModify();

		return result;
	}

	// Original function: FUN_004285b9.
	void RoadBuildManager_ClearRoadBuildFlagForAnyPlayer(void* manager, const void* point)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			if (Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player] != nullptr)
				RoadBuildManager_ClearRoadBuildFlag(manager, player, point);
		}
	}

	// Original function: FUN_004285e2; only idle builders without a build-site attachment are assigned.
	void __fastcall RoadBuildManager_DoOneGameSecond(void* manager, void*)
	{
		for (std::int32_t player = 0; player < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++player)
		{
			auto* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

			if (data == nullptr || Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) >= NC2Runtime::BuildLimits::RoadWorkers)
				continue;

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::PointCount); ++i)
			{
				if (Read<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint) == 0)
				{
					CHumanIterator iterator;

					for (auto id = iterator.GetFirstHumanId(player, data + NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint, NC2Runtime::BuildLimits::WorkerSearchRadius); id != -1; id = iterator.GetNextHumanId())
					{
						auto* human = const_cast<void*>(GetHumanPtr(id));

						if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Builder || Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != SwitchIds::GoodCirculationMode::None || Human_IsAttachedToBuildSite(human))
							continue;

						Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
						data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
						Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::Points + i * NC2Layout::RecordSize::BuildPoint));
						Human_WorkCenterMarker_Update(human);
						Human_Target_ResetSecondaryTarget(human);
						data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];
						Write<std::uint8_t>(data, NC2Layout::BuildPlayerOffset::FirstPointAssigned + i * NC2Layout::RecordSize::BuildPoint, 1);
						Write(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount, Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) + 1u);
						break;
					}
				}

				data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[player];

				if (Read<std::uint32_t>(data, NC2Layout::BuildPlayerOffset::AssignedWorkerCount) >= NC2Runtime::BuildLimits::RoadWorkers)
					break;
			}
		}
	}

}