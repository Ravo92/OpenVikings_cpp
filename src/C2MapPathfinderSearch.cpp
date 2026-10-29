#include "C2GameControlStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideRouting.h"
#include "C2HouseBuildPosition.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	// Original function: FUN_00449706. The callback precedes queue insertion and marking.
	bool CMapPathfinderElement::SearchOverMap(const void* point, std::uint32_t radius, bool skipBlocked, MapSearchCallback callback, void* userData, bool useWalkCosts, std::int32_t playerId)
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;

		if (radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		const bool recordStatistics = playerId >= 0 && playerId < NC2Runtime::PathfinderSearchLimits::Players;

		if (recordStatistics)
		{
			const std::uint32_t offset = static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::AreaSearchCount;
			Write(gGameControl, offset, Read<std::uint32_t>(gGameControl, offset) + 1u);
		}

		SMapPathfinderManagerSearchOverMapCallbackData data{ 0, userData };

		if (callback(point, &data))
			return true;

		std::uint32_t remaining = useWalkCosts ? radius * NC2Runtime::PathfinderSearchLimits::WeightedRadiusMultiplierUnsigned + NC2Runtime::PathfinderSearchLimits::WeightedBudgetPaddingUnsigned : radius;
		GameMap_CalculationMap_Clear(point, static_cast<std::int32_t>(remaining + 2u));
		const std::uint32_t queueCount = useWalkCosts ? NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned : NC2Runtime::PathfinderSearchLimits::PlainQueuesUnsigned;

		for (std::uint32_t index = 0; index < queueCount; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(point, static_cast<std::int16_t>(remaining));

		std::uint32_t current = 0;
		std::uint32_t queued = 1;
		do
		{
			++data.distance;
			--remaining;
			auto& source = sMapPathfinderStaticVariables[current];

			if (!useWalkCosts)
				sMapPathfinderStaticVariables[(current - 1u) & NC2Runtime::PathfinderSearchLimits::PlainQueueMaskUnsigned].count = 0;

			for (std::uint32_t index = 0; index < source.count; ++index)
			{
				if (useWalkCosts)
					--queued;

				const void* microPoint = GameMap_GetConstMapMicroPointPtr(source.points + index);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					std::uint32_t next = source.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (GameMap_CalculationMap_GetValue(&next) != 0)
						continue;

					std::uint32_t cost = 1;

					if (useWalkCosts || skipBlocked)
					{
						const void* nextMicroPoint = GameMap_GetConstMapMicroPointPtr(&next);
						const std::uint8_t flags = Read<std::uint8_t>(nextMicroPoint, NC2Layout::MapMicroPointOffset::WalkingFlags);

						if (skipBlocked && (flags & NXBasics::BitMask::Bit7) != 0)
							continue;

						if (useWalkCosts)
						{
							cost = (flags >> NC2Runtime::MapWalkingFlags::SpeedShift) & NC2Runtime::MapWalkingFlags::SpeedMaskUnsigned;

							if (cost == 0)
								continue;
						}
					}

					if (callback(&next, &data))
						return true;

					auto& destination = sMapPathfinderStaticVariables[useWalkCosts ? (current + cost) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned : (current - 1u) & NC2Runtime::PathfinderSearchLimits::PlainQueueMaskUnsigned];

					if (destination.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					destination.points[destination.count] = next;
					++destination.count;

					if (useWalkCosts)
						++queued;

					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(remaining));
				}
			}

			if (useWalkCosts)
			{
				source.count = 0;

				if (queued == 0)
					break;

				current = (current + 1u) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned;

				if (remaining <= NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned)
					break;
			}
			else
			{
				current = (current + 1u) & NC2Runtime::PathfinderSearchLimits::PlainQueueMaskUnsigned;

				if (sMapPathfinderStaticVariables[current].count == 0 || remaining <= 1u)
					break;
			}
		} while (true);

		if (recordStatistics)
		{
			const std::uint32_t offset = static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::AreaSearchWithoutEarlyExitCount;
			Write(gGameControl, offset, Read<std::uint32_t>(gGameControl, offset) + 1u);
		}

		return false;
	}

	using NC2Lifetime::Read;
	using NC2Lifetime::Write;

	namespace
	{
		std::int32_t PathfinderDistance(const void* source, const void* destination)
		{

			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY));
		}
	}

	// Original function: FUN_00447837.
	void MapPathfinderElement_StartPathfinder(void* pathfinder, const void* source, const void* destination, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides, bool deterministic)
	{
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::SearchRequired);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode, NC2Runtime::PathfinderTargetMode::Position);
		Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPosition, Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates));
		Write(pathfinder, NC2Layout::MapPathfinderOffset::DestinationPosition, Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates));

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPosition) == Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::DestinationPosition))
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);

		Write(pathfinder, NC2Layout::MapPathfinderOffset::SearchRadius, radius);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseResistanceFlag, useResistance);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseSectorsFlag, useSectors);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseGuidesFlag, useGuides);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DeterministicDirectionChoiceFlag, deterministic);
		const auto player = Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId);

		if (player >= 0 && player < NC2Runtime::PathfinderSearchLimits::Players)
		{
			const auto offset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::PointPathfinderStartCount;
			Write(gGameControl, offset, Read<std::uint32_t>(gGameControl, offset) + 1u);
		}
	}

	// Original function: FUN_004478C1. Windows tests map attachment before the revision lookup.
	void MapPathfinderElement_StartPathfinder(void* pathfinder, const void* source, std::int32_t moveableId, std::int32_t distance, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides, bool deterministic)
	{
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::SearchRequired);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode, NC2Runtime::PathfinderTargetMode::Moveable);
		Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPosition, Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates));
		Write(pathfinder, NC2Layout::MapPathfinderOffset::TargetMoveableId, moveableId);
		const void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(moveableId) * NC2Layout::RecordSize::MapMoveable;

		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);

			return;
		}

		Write(pathfinder, NC2Layout::MapPathfinderOffset::TargetObjectRevision, MapMoveable_GetUniqueId(moveable));
		Write(pathfinder, NC2Layout::MapPathfinderOffset::TargetDistance, distance);
		std::uint32_t point = 0;
		MapMoveable_Attachment_GetNextMapPosition(moveable, &point);

		if (PathfinderDistance(static_cast<std::byte*>(pathfinder) + NC2Layout::MapPathfinderOffset::CurrentPosition, &point) <= distance)
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);

		Write(pathfinder, NC2Layout::MapPathfinderOffset::SearchRadius, radius);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseResistanceFlag, useResistance);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseSectorsFlag, useSectors);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseGuidesFlag, useGuides);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DeterministicDirectionChoiceFlag, deterministic);
	}

	// Original function: FUN_0044796E.
	void MapPathfinderElement_StopPathfinder(void* pathfinder)
	{
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
	}

	// Original function: FUN_0044797B. Returns -4 deferred, -3 stopped, -2 arrived, -1 failed.
	std::int32_t MapPathfinderElement_GetNextDirection(void* pathfinder)
	{
		const auto stop = [pathfinder](std::int32_t result)
		{
			MapPathfinderElement_StopPathfinder(pathfinder);

			return result;
		};
		const auto currentPoint = static_cast<std::byte*>(pathfinder) + NC2Layout::MapPathfinderOffset::CurrentPosition;
		const auto destination = static_cast<std::byte*>(pathfinder) + NC2Layout::MapPathfinderOffset::DestinationPosition;
		const auto target = [pathfinder]() -> const void*
		{
			const void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMoveableId) * NC2Layout::RecordSize::MapMoveable;

			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0)
				return nullptr;

			if (MapMoveable_GetUniqueId(moveable) != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetObjectRevision))
				return nullptr;

			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0)
				return nullptr;

			return moveable;
		};

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::Stopped)
			return NC2Runtime::PathfinderDirectionResult::Stopped;

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::ArrivedUnsigned)
		{
			if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::MoveableUnsigned)
			{
				const void* moveable = target();

				if (moveable == nullptr)
					return stop(NC2Runtime::PathfinderDirectionResult::Failed);

				std::uint32_t point = 0;
				MapMoveable_Attachment_GetNextMapPosition(moveable, &point);

				if (PathfinderDistance(currentPoint, &point) <= Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetDistance))
					return stop(NC2Runtime::PathfinderDirectionResult::Arrived);

				Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::SearchRequired);
			}
			else if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::PositionUnsigned)
			{
				return stop(NC2Runtime::PathfinderDirectionResult::Arrived);
			}
		}

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::SearchRequiredUnsigned)
		{
			if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::MoveableUnsigned)
			{
				const void* moveable = target();

				if (moveable == nullptr)
					return stop(NC2Runtime::PathfinderDirectionResult::Failed);

				MapMoveable_Attachment_GetNextMapPosition(moveable, destination);
			}

			if (static_cast<std::uint32_t>(PathfinderDistance(destination, currentPoint)) > 6u && !GameControl_System_PathfinderLimit_IsSearchAllowed(gGameControl, Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId)))
				return NC2Runtime::PathfinderDirectionResult::Deferred;
		}
		else if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::MoveableUnsigned)
		{
			const void* moveable = target();

			if (moveable == nullptr)
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			std::uint32_t point = 0;
			MapMoveable_Attachment_GetNextMapPosition(moveable, &point);
			const auto distance = PathfinderDistance(currentPoint, destination);
			const auto changed = PathfinderDistance(&point, destination);

			if (distance <= 3 || (distance <= 10 && changed > 4) || (distance <= 20 && changed > 8) || (distance <= 50 && changed > 12) || (distance > 50 && changed > 20))
			{
				Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::SearchRequired);
				Write(destination, NC2Layout::MapMigPointOffset::Coordinates, point);
			}
		}

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::SearchRequiredUnsigned)
		{
			if (!MapMigPoint_IsInsideMap(currentPoint) || !MapMigPoint_IsInsideMap(destination))
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			if (Read<std::uint32_t>(currentPoint, NC2Layout::MapMigPointOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates))
				return stop(NC2Runtime::PathfinderDirectionResult::Arrived);

			const auto sourceContinent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(currentPoint), NC2Layout::MapMicroPointOffset::ContinentId);
			const auto destinationContinent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::ContinentId);

			if (sourceContinent != destinationContinent)
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			if (!MapPathfinderElement_l_FindWayAndBuildVectorList(pathfinder, currentPoint, destination, Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchRadius)))
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::RouteAvailable);
		}

		auto index = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunIndex);
		// Remaster: reject corrupt cursors before indexing the twenty-run buffer.

		if (index > NC2Runtime::PathfinderSearchLimits::DirectionRunsUnsigned)
			return stop(NC2Runtime::PathfinderDirectionResult::Failed);
		// Index 20 means all twenty runs were consumed. Do not read beyond the original array.

		if ((index == NC2Runtime::PathfinderSearchLimits::DirectionRunsUnsigned || Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunCounts + index * NC2Layout::RecordSize::PathfinderDirectionRun) == 0) && Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::RouteAvailableUnsigned)
		{
			if (!MapPathfinderElement_l_FindWayAndBuildVectorList(pathfinder, currentPoint, destination, Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchRadius)))
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			index = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunIndex);
		}

		if (index >= NC2Runtime::PathfinderSearchLimits::DirectionRunsUnsigned)
			return stop(NC2Runtime::PathfinderDirectionResult::Failed);

		const auto direction = Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRuns + index * NC2Layout::RecordSize::PathfinderDirectionRun);
		const auto remaining = Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunCounts + index * NC2Layout::RecordSize::PathfinderDirectionRun);

		if (direction > SwitchIds::HexagonDirection::South || remaining == 0)
			return stop(NC2Runtime::PathfinderDirectionResult::Failed);

		auto point = Read<std::uint32_t>(currentPoint, NC2Layout::MapMigPointOffset::Coordinates);

		if (!MapMigPoint_IsInsideMap(&point))
			return stop(NC2Runtime::PathfinderDirectionResult::Failed);

		MapMigPoint_MoveCursorUnwrapped(point, direction);
		// Validate before committing the step or reading its unchecked micro-point pointer.

		if (!MapMigPoint_IsInsideMap(&point))
			return stop(NC2Runtime::PathfinderDirectionResult::Failed);

		const auto count = static_cast<std::uint8_t>(remaining - 1u);
		Write(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunCounts + index * NC2Layout::RecordSize::PathfinderDirectionRun, count);

		if (count == 0)
			Write(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunIndex, index + 1u);

		Write(currentPoint, NC2Layout::MapMigPointOffset::Coordinates, point);
		const auto blockedAreaSteps = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::BlockedAreaSteps);

		if (blockedAreaSteps != 0)
		{
			Write(pathfinder, NC2Layout::MapPathfinderOffset::BlockedAreaSteps, blockedAreaSteps - 1u);
		}
		else
		{
			const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(currentPoint), NC2Layout::MapMicroPointOffset::WalkingFlags);

			if ((flags & NXBasics::BitMask::Bit7) != 0 || (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			{
				MapMigPoint_MoveCursorUnwrapped(point, VE_HexagonDirection_OppositeDirection(direction));
				Write(currentPoint, NC2Layout::MapMigPointOffset::Coordinates, point);
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);
			}
		}

		if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::MoveableUnsigned)
		{
			const auto distance = PathfinderDistance(currentPoint, destination);

			if (target() == nullptr)
				return stop(NC2Runtime::PathfinderDirectionResult::Failed);

			if (distance <= Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetDistance))
				Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);
		}
		else if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::TargetMode) == NC2Runtime::PathfinderTargetMode::PositionUnsigned && point == Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates))
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);
		}

		return direction;
	}

	// Original function: FUN_00447CF1. Local, sector and guide searches retain their distinct native order.
	bool MapPathfinderElement_l_FindWayAndBuildVectorList(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (!MapMigPoint_IsInsideMap(source) || !MapMigPoint_IsInsideMap(destination))
			return false;

		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			return false;

		const bool vehicle = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance) != 0;

		if (vehicle && (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			return false;

		bool found;

		if (vehicle)
		{
			if (Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseResistanceFlag) != 0)
			{
				found = MapPathfinderElement_l_VehicleFindWayDirectedWithResitance(pathfinder, source, destination, radius);
			}
			else
			{
				found = MapPathfinderElement_l_VehicleFindWayWithLine(pathfinder, source, destination, radius) || MapPathfinderElement_l_VehicleFindWayDirected(pathfinder, source, destination, radius);
			}
		}
		else
		{
			if (Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseResistanceFlag) != 0)
			{
				found = MapPathfinderElement_l_FindWayDirectedWithResitance(pathfinder, source, destination, radius);
			}
			else
			{
				found = MapPathfinderElement_l_FindWayWithLine(pathfinder, source, destination, radius) || MapPathfinderElement_l_FindWayDirected(pathfinder, source, destination, radius);
			}
		}

		if (!found)
		{
			const bool resistance = Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseResistanceFlag) != 0;

			if (Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseSectorsFlag) != 0)
			{
				std::uint32_t firstPoint = 0, secondPoint = 0;
				const bool land = GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId)) == 1;

				if (!CMapWalkSectorManager::GetNextSectorBasePoint(source, destination, Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance), land, &firstPoint, &secondPoint, radius, Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId)))
					return false;

				if (vehicle && resistance && !MapMigPoint_IsInsideMap(&firstPoint))
					return false;

				if (firstPoint == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
					return false;

				if (vehicle)
				{
					if (resistance)
					{
						found = MapPathfinderElement_l_VehicleFindWayDirectedWithResitance(pathfinder, source, &secondPoint, radius) || MapPathfinderElement_l_VehicleFindWayDirectedWithResitance(pathfinder, source, &firstPoint, radius);
					}
					else
					{
						found = MapPathfinderElement_l_VehicleFindWayDirected(pathfinder, source, &secondPoint, radius) || MapPathfinderElement_l_VehicleFindWayWithLine(pathfinder, source, &firstPoint, radius) || MapPathfinderElement_l_VehicleFindWayDirected(pathfinder, source, &firstPoint, radius);
					}
				}
				else
				{
					if (resistance)
					{
						found = MapPathfinderElement_l_FindWayDirectedWithResitance(pathfinder, source, &secondPoint, radius) || MapPathfinderElement_l_FindWayDirectedWithResitance(pathfinder, source, &firstPoint, radius);
					}
					else
					{
						found = MapPathfinderElement_l_FindWayDirected(pathfinder, source, &secondPoint, radius) || MapPathfinderElement_l_FindWayWithLine(pathfinder, source, &firstPoint, radius) || MapPathfinderElement_l_FindWayDirected(pathfinder, source, &firstPoint, radius);
					}
				}
			}
			else if (!vehicle && Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::UseGuidesFlag) != 0)
			{
				const auto guide = [](std::int32_t id)
				{

					return CGuideArray::mStaticVars + static_cast<std::intptr_t>(id) * CGuideArray::ElementSize;
				};
				auto previousGuide = Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId);
				auto sourceGuide = Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId);

				if (previousGuide == sourceGuide && (!resistance || previousGuide != -1))
				{
					Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
					Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
					sourceGuide = -1;
				}

				bool cached = sourceGuide != -1 && Read<std::uint8_t>(guide(sourceGuide), NC2Layout::GuideOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(guide(sourceGuide), NC2Layout::GuideOffset::UniqueId) == Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideRevision);
				const auto currentGuide = Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId);
				cached = cached && currentGuide != -1 && Read<std::uint8_t>(guide(currentGuide), NC2Layout::GuideOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(guide(currentGuide), NC2Layout::GuideOffset::UniqueId) == Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideRevision) && Read<std::uint32_t>(guide(currentGuide), NC2Layout::GuideOffset::LinkSystemId) == Read<std::uint32_t>(guide(sourceGuide), NC2Layout::GuideOffset::LinkSystemId);

				if (cached)
				{
					Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, GuideManager_GetNextGuideId(gGuideManager, currentGuide, sourceGuide));
				}
				else
				{
					const auto next = GuideManager_GetNextGuideId(gGuideManager, Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId), radius, source, destination, sourceGuide, previousGuide);
					Write(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, sourceGuide);
					Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, next);

					if (sourceGuide != -1)
						Write(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideRevision, Read<std::uint32_t>(guide(sourceGuide), NC2Layout::GuideOffset::UniqueId));
				}

				auto nextGuide = Read<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId);

				if (nextGuide == -1)
					return false;

				std::uint32_t point = 0;
				Guide_GetMovePoint(guide(nextGuide), &point);
				found = resistance ? MapPathfinderElement_l_FindWayDirectedWithResitance(pathfinder, source, &point, radius) : MapPathfinderElement_l_FindWayDirected(pathfinder, source, &point, radius);

				if (!found)
				{
					Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, previousGuide);
					nextGuide = previousGuide;
					Guide_GetMovePoint(guide(nextGuide), &point);
					found = resistance ? MapPathfinderElement_l_FindWayDirectedWithResitance(pathfinder, source, &point, radius) : MapPathfinderElement_l_FindWayDirected(pathfinder, source, &point, radius);
				}

				if (found)
					Write(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideRevision, Read<std::uint32_t>(guide(nextGuide), NC2Layout::GuideOffset::UniqueId));
			}

			if (!found)
				return false;
		}

		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunIndex, 0);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::BlockedAreaSteps, 0);
		auto point = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPosition);
		std::uint32_t runs = 0;
		std::uint32_t previous = NC2Runtime::PathfinderDirection::Unassigned;
		bool blockedArea = (GameMap_CalculationMap_GetValue(&point) & NXBasics::BitMask::Bit9) != 0;

		while (true)
		{
			const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&point));

			if ((value & NXBasics::BitMask::WordExceptBits0Through9) == 0)
				break;

			std::uint32_t direction = NC2Runtime::PathfinderDirection::Unassigned;
			std::uint32_t first = NC2Runtime::PathfinderDirection::Unassigned;

			for (std::uint32_t candidate = 0; candidate < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++candidate)
			{
				if ((value & (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << candidate)) == 0)
					continue;

				if (first == NC2Runtime::PathfinderDirection::UnassignedUnsigned)
					first = candidate;

				if (candidate == previous)
					direction = previous;
			}

			if (direction == NC2Runtime::PathfinderDirection::UnassignedUnsigned)
				direction = first;

			if (static_cast<std::uint32_t>(PathfinderDistance(&point, static_cast<std::byte*>(pathfinder) + NC2Layout::MapPathfinderOffset::DestinationPosition)) > 3u && (direction == SwitchIds::HexagonDirection::SouthEastUnsigned || direction == SwitchIds::HexagonDirection::SouthWestUnsigned || direction == SwitchIds::HexagonDirection::NorthWestUnsigned || direction == SwitchIds::HexagonDirection::NorthEastUnsigned) && Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::AllowVerticalDirectionsFlag) != 0)
			{
				auto next = point;
				MapMigPoint_MoveCursorUnwrapped(next, direction);
				const auto nextValue = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

				if ((nextValue & NXBasics::BitMask::WordExceptBits0Through9) != 0)
				{
					if (direction == SwitchIds::HexagonDirection::SouthEastUnsigned && (nextValue & NXBasics::BitMask::Bit12) != 0)
					{
						direction = SwitchIds::HexagonDirection::South;
					}
					else if (direction == SwitchIds::HexagonDirection::SouthWestUnsigned && (nextValue & NXBasics::BitMask::Bit11) != 0)
					{
						direction = SwitchIds::HexagonDirection::South;
					}
					else if (direction == SwitchIds::HexagonDirection::NorthWestUnsigned && (nextValue & NXBasics::BitMask::Bit15) != 0)
					{
						direction = SwitchIds::HexagonDirection::North;
					}
					else if (direction == SwitchIds::HexagonDirection::NorthEastUnsigned && (nextValue & NXBasics::BitMask::Bit14) != 0)
					{
						direction = SwitchIds::HexagonDirection::North;
					}
				}
			}

			const auto countOffset = NC2Layout::MapPathfinderOffset::DirectionRunCountsUnsigned + (runs - 1u) * NC2Layout::RecordSize::PathfinderDirectionRun;

			if (direction == previous && Read<std::uint8_t>(pathfinder, countOffset) != NC2Runtime::PathfinderSearchLimits::MaximumRunLengthUnsigned)
			{
				Write<std::uint8_t>(pathfinder, countOffset, static_cast<std::uint8_t>(Read<std::uint8_t>(pathfinder, countOffset) + 1u));
			}
			else
			{
				if (runs == NC2Runtime::PathfinderSearchLimits::DirectionRunsUnsigned)
					break;

				Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRuns + runs * NC2Layout::RecordSize::PathfinderDirectionRun, static_cast<std::uint8_t>(direction));
				Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunCounts + runs * NC2Layout::RecordSize::PathfinderDirectionRun, 1);
				++runs;
				previous = direction;
			}

			MapMigPoint_MoveCursorUnwrapped(point, direction);

			if (blockedArea && (GameMap_CalculationMap_GetValue(&point) & NXBasics::BitMask::Bit9) != 0)
			{
				Write(pathfinder, NC2Layout::MapPathfinderOffset::BlockedAreaSteps, Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::BlockedAreaSteps) + 1u);
			}
			else
			{
				blockedArea = false;
			}
		}

		if (runs < NC2Runtime::PathfinderSearchLimits::DirectionRunsUnsigned)
			Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DirectionRunCounts + runs * NC2Layout::RecordSize::PathfinderDirectionRun, 0);
		// A successful search can still yield no runs; never consume the stale direction byte.

		if (runs == 0)
			return false;

		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::RouteAvailable);

		return true;
	}

	// Original function: FUN_00448649. Unlike the vehicle helper, this tests walk-direction bits.
	bool MapPathfinderElement_l_CheckOtherDirection(void* pathfinder, std::uint32_t direction, void* point, const void* microPoint, std::uint32_t remaining)
	{
		auto other = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1);

		if (other == direction)
		{
			if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight2) == 0)
				return false;

			other = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2);
		}
		else if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight1) == 0)
		{
			return false;
		}

		if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << other)) == 0)
			return false;

		auto packed = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(packed, other);
		Write(point, NC2Layout::MapMigPointOffset::Coordinates, packed);
		const auto nextMicroPoint = GameMap_GetConstMapMicroPointPtr(point);
		const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(point));

		if ((Read<std::uint8_t>(nextMicroPoint, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
			return false;

		GameMap_CalculationMap_SetValue(point, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((other + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned)) | (remaining - 1u)));

		return true;
	}

	// Original function: FUN_0044889C. This helper also checks the vehicle clearance.
	bool MapPathfinderElement_l_VehicleCheckOtherDirection(void* pathfinder, std::uint32_t direction, void* point, std::uint32_t remaining)
	{
		auto other = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1);

		if (other == direction)
		{
			if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight2) == 0)
				return false;

			other = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2);
		}
		else if (Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight1) == 0)
		{
			return false;
		}

		auto packed = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(packed, other);
		Write(point, NC2Layout::MapMigPointOffset::Coordinates, packed);
		const auto nextMicroPoint = GameMap_GetConstMapMicroPointPtr(point);
		const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(point));
		const auto flags = Read<std::uint8_t>(nextMicroPoint, NC2Layout::MapMicroPointOffset::WalkingFlags);

		if (((flags & NXBasics::BitMask::Bit7) != 0 || (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance)) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
			return false;

		GameMap_CalculationMap_SetValue(point, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((other + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned)) | (remaining - 1u)));

		return true;
	}

	// Original function: FUN_00448932. The two original queues mark the connected blocked area.
	void MapPathfinderElement_l_MakeMoveableArea(void* pathfinder, const void* point)
	{
		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[1].count = 0;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(point, NC2Runtime::PathfinderCalculationValue::BlockedStartAreaFlag);
		std::uint32_t current = 0;
		do
		{
			auto& source = sMapPathfinderStaticVariables[current];
			auto& destination = sMapPathfinderStaticVariables[(current - 1u) & NC2Runtime::PathfinderSearchLimits::PlainQueueMaskUnsigned];
			destination.count = 0;

			for (std::uint32_t index = 0; index < source.count; ++index)
			{
				const void* microPoint = GameMap_GetConstMapMicroPointPtr(source.points + index);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = source.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if ((GameMap_CalculationMap_GetValue(&next) & NXBasics::BitMask::Bit9) != 0)
						continue;

					const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);

					if ((flags & NXBasics::BitMask::Bit7) == 0 && (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) >= Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
						continue;

					if (destination.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					destination.points[destination.count++] = next;
					GameMap_CalculationMap_SetValue(&next, NC2Runtime::PathfinderCalculationValue::BlockedStartAreaFlag);
				}
			}

			current = (current + 1u) & NC2Runtime::PathfinderSearchLimits::PlainQueueMaskUnsigned;
		} while (sMapPathfinderStaticVariables[current].count != 0);
	}

	// Original function: FUN_00448A6B. Windows sign-extends the half-distance word.
	void MapPathfinderElement_l_GetPreferedDirections(void* pathfinder, const void* source, const void* destination)
	{
		const auto dx = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapX)) - Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapX);
		const auto dy = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY)) - Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapY);
		auto x = dx < 0 ? -dx : dx;
		const auto y = dy < 0 ? -dy : dy;

		if ((y & 1) != 0 && ((Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY) & 1) != 0 ? dx < 0 : dx > 0))
			--x;

		const auto halfY = static_cast<std::int16_t>(y / 2);

		if (x > halfY)
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1, dy < 0 ? (dx < 0 ? SwitchIds::HexagonDirection::NorthWestUnsigned : SwitchIds::HexagonDirection::NorthEastUnsigned) : (dx < 0 ? SwitchIds::HexagonDirection::SouthWestUnsigned : SwitchIds::HexagonDirection::SouthEastUnsigned));
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2, dx < 0 ? SwitchIds::HexagonDirection::WestUnsigned : SwitchIds::HexagonDirection::EastUnsigned);
		}
		else
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1, dy < 0 ? SwitchIds::HexagonDirection::NorthEastUnsigned : SwitchIds::HexagonDirection::SouthEastUnsigned);
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2, dy < 0 ? SwitchIds::HexagonDirection::NorthWestUnsigned : SwitchIds::HexagonDirection::SouthWestUnsigned);
		}
	}

	// Original function: FUN_00448B1E. This is a distinct geometry routine, including weights.
	void MapPathfinderElement_l_GetPreferedDirectionsAndDistance(void* pathfinder, const void* source, const void* destination)
	{
		const auto dx = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapX)) - Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapX);
		const auto dy = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY)) - Read<std::int16_t>(source, NC2Layout::MapMigPointOffset::MapY);
		auto x = dx < 0 ? -dx : dx;
		const auto y = dy < 0 ? -dy : dy;

		if ((y & 1) != 0 && ((Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY) & 1) != 0 ? dx < 0 : dx > 0))
			--x;

		const auto halfY = static_cast<std::int16_t>(y / 2);
		std::int32_t firstWeight, secondWeight;

		if (x > halfY)
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1, dy < 0 ? (dx < 0 ? SwitchIds::HexagonDirection::NorthWestUnsigned : SwitchIds::HexagonDirection::NorthEastUnsigned) : (dx < 0 ? SwitchIds::HexagonDirection::SouthWestUnsigned : SwitchIds::HexagonDirection::SouthEastUnsigned));
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2, dx < 0 ? SwitchIds::HexagonDirection::WestUnsigned : SwitchIds::HexagonDirection::EastUnsigned);
			firstWeight = y;
			secondWeight = x - halfY;
		}
		else
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1, dy < 0 ? SwitchIds::HexagonDirection::NorthEastUnsigned : SwitchIds::HexagonDirection::SouthEastUnsigned);
			Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2, dy < 0 ? SwitchIds::HexagonDirection::NorthWestUnsigned : SwitchIds::HexagonDirection::SouthWestUnsigned);

			if (halfY != 0)
			{
				if (dx >= 0)
				{
					secondWeight = halfY - x;
					firstWeight = y - secondWeight;
				}
				else
				{
					firstWeight = halfY - x;
					secondWeight = y - firstWeight;
				}
			}
			else if (y != 0 && dx > 0)
			{
				firstWeight = y;
				secondWeight = 0;
			}
			else if (y != 0 && dx < 0)
			{
				firstWeight = 0;
				secondWeight = y;
			}
			else if (y != 0 && x == 0 && (Read<std::int16_t>(destination, NC2Layout::MapMigPointOffset::MapY) & 1) != 0)
			{
				firstWeight = y;
				secondWeight = 0;
			}
			else
			{
				firstWeight = 0;
				secondWeight = y;
			}
		}

		Write(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight1, static_cast<std::uint32_t>(firstWeight));
		Write(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight2, static_cast<std::uint32_t>(secondWeight));
	}

	// Original function: FUN_00448460.
	bool MapPathfinderElement_l_FindWayWithLine(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius + 2u));
		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
		{
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);
		}
		else
		{
			Remaster_House_PrepareConstructionEscape(source);
		}

		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(radius));
		auto point = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		do
		{
			MapPathfinderElement_l_GetPreferedDirectionsAndDistance(pathfinder, &point, source);
			const auto firstWeight = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight1);
			const auto secondWeight = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight2);
			const bool first = Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DeterministicDirectionChoiceFlag) == 0 ? Game_Random_GetNumber() % (firstWeight + secondWeight) < firstWeight : firstWeight > secondWeight;
			const auto direction = Read<std::uint32_t>(pathfinder, first ? NC2Layout::MapPathfinderOffset::PreferredDirection1 : NC2Layout::MapPathfinderOffset::PreferredDirection2);
			auto alternative = point;
			const void* microPoint = GameMap_GetConstMapMicroPointPtr(&point);

			if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
			{
				if (!MapPathfinderElement_l_CheckOtherDirection(pathfinder, direction, &alternative, microPoint, radius))
					return false;

				point = alternative;
			}
			else
			{
				MapMigPoint_MoveCursorUnwrapped(point, direction);
				const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags);
				const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&point));

				if (((nextFlags & NXBasics::BitMask::Bit7) != 0) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
				{
					if (!MapPathfinderElement_l_CheckOtherDirection(pathfinder, direction, &alternative, microPoint, radius))
						return false;

					point = alternative;
				}
				else
				{
					GameMap_CalculationMap_SetValue(&point, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned)) | (radius - 1u)));
				}
			}

			if (point == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
				return true;

			--radius;
		} while (radius > 1u);

		return false;
	}

	// Original function: FUN_004486DC.
	bool MapPathfinderElement_l_VehicleFindWayWithLine(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius + 2u));
		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0 || (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);

		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(radius));
		auto point = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		do
		{
			MapPathfinderElement_l_GetPreferedDirectionsAndDistance(pathfinder, &point, source);
			const auto firstWeight = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight1);
			const auto secondWeight = Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirectionWeight2);
			const bool first = Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::DeterministicDirectionChoiceFlag) == 0 ? Game_Random_GetNumber() % (firstWeight + secondWeight) < firstWeight : firstWeight > secondWeight;
			const auto direction = Read<std::uint32_t>(pathfinder, first ? NC2Layout::MapPathfinderOffset::PreferredDirection1 : NC2Layout::MapPathfinderOffset::PreferredDirection2);
			auto alternative = point;
			{
				MapMigPoint_MoveCursorUnwrapped(point, direction);
				const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags);
				const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&point));

				if (((nextFlags & NXBasics::BitMask::Bit7) != 0 || (nextFlags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance)) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
				{
					if (!MapPathfinderElement_l_VehicleCheckOtherDirection(pathfinder, direction, &alternative, radius))
						return false;

					point = alternative;
				}
				else
				{
					GameMap_CalculationMap_SetValue(&point, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned)) | (radius - 1u)));
				}
			}

			if (point == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
				return true;

			--radius;
		} while (radius > 1u);

		return false;
	}

	// Original function: FUN_00448C55. Queue capacity and equal-cost direction masks are retained.
	bool MapPathfinderElement_l_FindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 4u;

		if (budget > NC2Runtime::PathfinderSearchLimits::MaximumBudgetUnsigned)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
		{
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);
		}
		else
		{
			Remaster_House_PrepareConstructionEscape(source);
		}

		for (std::uint32_t index = 0; index < NC2Runtime::PathfinderSearchLimits::WeightedQueues; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(budget));
		std::uint32_t current = 0;
		std::uint32_t queued = 1;
		do
		{
			const auto remaining = budget - 1u;
			auto& queue = sMapPathfinderStaticVariables[current];

			for (std::uint32_t index = 0; index < queue.count; ++index)
			{
				--queued;
				const void* microPoint = GameMap_GetConstMapMicroPointPtr(queue.points + index);
				MapPathfinderElement_l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);
					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = 0;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2))
						cost += NC2Runtime::PathfinderSearchLimits::NonPreferredDirectionCostUnsigned;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost + 1u) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned];

					if (target.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned;
			budget = remaining;
		} while (budget > NC2Runtime::PathfinderSearchLimits::DirectedStopBudgetUnsigned);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_00448EF0. Queue capacity and equal-cost direction masks are retained.
	bool MapPathfinderElement_l_VehicleFindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 4u;

		if (budget > NC2Runtime::PathfinderSearchLimits::MaximumBudgetUnsigned)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0 || (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);

		for (std::uint32_t index = 0; index < NC2Runtime::PathfinderSearchLimits::WeightedQueues; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(budget));
		std::uint32_t current = 0;
		std::uint32_t queued = 1;
		do
		{
			const auto remaining = budget - 1u;
			auto& queue = sMapPathfinderStaticVariables[current];

			for (std::uint32_t index = 0; index < queue.count; ++index)
			{
				--queued;
				MapPathfinderElement_l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInsideMap(&next))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0 || (nextFlags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance)) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = 0;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2))
						cost += NC2Runtime::PathfinderSearchLimits::NonPreferredDirectionCostUnsigned;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost + 1u) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned];

					if (target.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % NC2Runtime::PathfinderSearchLimits::WeightedQueuesUnsigned;
			budget = remaining;
		} while (budget > NC2Runtime::PathfinderSearchLimits::DirectedStopBudgetUnsigned);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_004491AD. Queue capacity and equal-cost direction masks are retained.
	bool MapPathfinderElement_l_FindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * NC2Runtime::PathfinderSearchLimits::ResistanceBudgetMultiplierUnsigned;

		if (budget > NC2Runtime::PathfinderSearchLimits::MaximumBudgetUnsigned)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
		{
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);
		}
		else
		{
			Remaster_House_PrepareConstructionEscape(source);
		}

		for (std::uint32_t index = 0; index < NC2Runtime::PathfinderSearchLimits::ResistanceQueues; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(budget));
		std::uint32_t current = 0;
		std::uint32_t queued = 1;
		do
		{
			const auto remaining = budget - 1u;
			auto& queue = sMapPathfinderStaticVariables[current];

			for (std::uint32_t index = 0; index < queue.count; ++index)
			{
				--queued;
				const void* microPoint = GameMap_GetConstMapMicroPointPtr(queue.points + index);
				MapPathfinderElement_l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);
					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = (nextFlags >> NC2Runtime::MapWalkingFlags::SpeedShift) & NC2Runtime::MapWalkingFlags::SpeedMaskUnsigned;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2))
						cost += NC2Runtime::PathfinderSearchLimits::NonPreferredDirectionCostUnsigned;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost) & NC2Runtime::PathfinderSearchLimits::ResistanceQueueMaskUnsigned];

					if (target.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % NC2Runtime::PathfinderSearchLimits::ResistanceQueuesUnsigned;
			budget = remaining;
		} while (budget > NC2Runtime::PathfinderSearchLimits::ResistanceStopBudgetUnsigned);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_00449447. Queue capacity and equal-cost direction masks are retained.
	bool MapPathfinderElement_l_VehicleFindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned)
			return false;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * NC2Runtime::PathfinderSearchLimits::ResistanceBudgetMultiplierUnsigned;

		if (budget > NC2Runtime::PathfinderSearchLimits::MaximumBudgetUnsigned)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0 || (flags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance))
			MapPathfinderElement_l_MakeMoveableArea(pathfinder, source);

		for (std::uint32_t index = 0; index < NC2Runtime::PathfinderSearchLimits::ResistanceQueues; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates);
		GameMap_CalculationMap_SetValue(destination, static_cast<std::int16_t>(budget));
		std::uint32_t current = 0;
		std::uint32_t queued = 1;
		do
		{
			const auto remaining = budget - 1u;
			auto& queue = sMapPathfinderStaticVariables[current];

			for (std::uint32_t index = 0; index < queue.count; ++index)
			{
				--queued;
				MapPathfinderElement_l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
				{
					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInsideMap(&next))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0 || (nextFlags & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) < Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance)) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = (nextFlags >> NC2Runtime::MapWalkingFlags::SpeedShift) & NC2Runtime::MapWalkingFlags::SpeedMaskUnsigned;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PreferredDirection2))
						cost += NC2Runtime::PathfinderSearchLimits::NonPreferredDirectionCostUnsigned;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + NC2Runtime::PathfinderGeometry::OppositeDirectionOffsetUnsigned) % NC2Runtime::PathfinderGeometry::NeighbourDirectionsUnsigned);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost) & NC2Runtime::PathfinderSearchLimits::ResistanceQueueMaskUnsigned];

					if (target.count == NC2Runtime::PathfinderSearchLimits::QueuePointsUnsigned)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % NC2Runtime::PathfinderSearchLimits::ResistanceQueuesUnsigned;
			budget = remaining;
		} while (budget > NC2Runtime::PathfinderSearchLimits::ResistanceStopBudgetUnsigned);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

}