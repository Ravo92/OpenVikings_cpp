#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapPathfinderSectorSearch.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using NC2Lifetime::Read;
	using NC2Lifetime::Write;

	namespace
	{
		std::int32_t PathfinderDistance(const void* source, const void* destination)
		{

			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(destination, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(destination, NC2Layout::MapCoordinatePairOffset::Y));
		}
	}

	// Original function: FUN_004703AE.
	void CMapPathfinderSpecialSectorSearchElement::Init(void* pathfinder)
	{
		std::memset(pathfinder, 0, NC2Layout::RecordSize::SectorPathfinderSigned);
		Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchRadius, 5);
	}

	// Original function: FUN_004703CA.
	void CMapPathfinderSpecialSectorSearchElement::StartPathfinder(void* pathfinder, const void* source, const void* destination, std::uint32_t radius, bool useResistance, const std::int16_t* rectangle)
	{
		Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::SearchRequired);
		Write(pathfinder, NC2Layout::SectorPathfinderOffset::CurrentPosition, Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(pathfinder, NC2Layout::SectorPathfinderOffset::DestinationPosition, Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates));

		if (Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates))
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);

		Write(pathfinder, NC2Layout::SectorPathfinderOffset::SearchRadius, radius);
		Write<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::UseResistanceFlag, useResistance ? 1 : 0);
		std::memcpy(static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle, rectangle, 8);
	}

	// Original function: FUN_0047041B.
	std::int32_t CMapPathfinderSpecialSectorSearchElement::GetNextDirection(void* pathfinder)
	{
		const auto state = Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState);

		if (state == NC2Runtime::PathfinderSearchState::Stopped)
			return NC2Runtime::PathfinderDirectionResult::Stopped;

		if (state == NC2Runtime::PathfinderSearchState::Arrived)
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);
			return NC2Runtime::PathfinderDirectionResult::Arrived;
		}

		auto* source = static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::CurrentPosition;
		auto* destination = static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::DestinationPosition;
		const auto stop = [&]()
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);

			return NC2Runtime::PathfinderDirectionResult::Failed;
		};

		if (state == NC2Runtime::PathfinderSearchState::SearchRequired)
		{
			if (Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates))
			{
				Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);
				return NC2Runtime::PathfinderDirectionResult::Arrived;
			}

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::ContinentId))
				return stop();

			if (!l_FindWayAndBuildVectorList(pathfinder, source, destination, Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchRadius)))
				return stop();

			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::RouteAvailable);
		}

		auto run = Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunIndex);
		// Windows reads the count byte before checking whether all twenty runs were consumed.

		if ((Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunCounts + run * NC2Layout::RecordSize::PathfinderDirectionRun) == 0 || run == 20u) && Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState) == NC2Runtime::PathfinderSearchState::RouteAvailable)
		{
			if (!l_FindWayAndBuildVectorList(pathfinder, source, destination, Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchRadius)))
				return stop();

			run = Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunIndex);
		}

		const auto direction = Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRuns + run * NC2Layout::RecordSize::PathfinderDirectionRun);
		const auto count = static_cast<std::uint8_t>(Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunCounts + run * NC2Layout::RecordSize::PathfinderDirectionRun) - 1u);
		Write(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunCounts + run * NC2Layout::RecordSize::PathfinderDirectionRun, count);

		if (count == 0)
			Write(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunIndex, run + 1u);

		MapMigPoint_MoveCursorUnwrapped(source, direction, 1);
		const auto blocked = Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::BlockedAreaSteps);

		if (blocked != 0)
		{
			Write(pathfinder, NC2Layout::SectorPathfinderOffset::BlockedAreaSteps, blocked - 1u);
		}
		else if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
		{
			MapMigPoint_MoveCursorUnwrapped(source, VE_HexagonDirection_OppositeDirection(direction), 1);
			return stop();
		}

		if (Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates))
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Arrived);

		return direction;
	}

	// Original function: FUN_00470535.
	bool CMapPathfinderSpecialSectorSearchElement::l_FindWayAndBuildVectorList(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			return false;

		bool found;

		if (Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance) != 0)
		{
			if ((flags & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance))
				return false;

			found = Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::UseResistanceFlag) != 0 ? l_VehicleFindWayDirectedWithResitance(pathfinder, source, destination, radius) : l_VehicleFindWayDirected(pathfinder, source, destination, radius);
		}
		else
		{
			found = Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::UseResistanceFlag) != 0 ? l_FindWayDirectedWithResitance(pathfinder, source, destination, radius) : l_FindWayDirected(pathfinder, source, destination, radius);
		}

		if (!found)
			return false;

		Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunIndex, 0);
		Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::BlockedAreaSteps, 0);
		auto point = Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::CurrentPosition);
		std::uint32_t runs = 0;
		std::uint32_t previous = 1000;
		bool blockedArea = (GameMap_CalculationMap_GetValue(&point) & NXBasics::BitMask::Bit9) != 0;

		while (true)
		{
			const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&point));

			if ((value & NXBasics::BitMask::WordExceptBits0Through9) == 0)
				break;

			std::uint32_t direction = 1000;
			std::uint32_t first = 1000;

			for (std::uint32_t candidate = 0; candidate < 6; ++candidate)
			{
				if ((value & (NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << candidate)) == 0)
					continue;

				if (first == 1000u)
					first = candidate;

				if (candidate == previous)
					direction = previous;
			}

			if (direction == 1000u)
				direction = first;

			if (static_cast<std::uint32_t>(PathfinderDistance(&point, static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::DestinationPosition)) > 3u && (direction == 1u || direction == 2u || direction == 4u || direction == 5u) && Read<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::AllowVerticalDirectionsFlag) != 0)
			{
				auto next = point;
				MapMigPoint_MoveCursorUnwrapped(next, direction);
				const auto nextValue = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

				if ((nextValue & NXBasics::BitMask::WordExceptBits0Through9) != 0)
				{
					if (direction == 1u && (nextValue & NXBasics::BitMask::Bit12) != 0)
					{
						direction = 7;
					}
					else if (direction == 2u && (nextValue & NXBasics::BitMask::Bit11) != 0)
					{
						direction = 7;
					}
					else if (direction == 4u && (nextValue & NXBasics::BitMask::Bit15) != 0)
					{
						direction = 6;
					}
					else if (direction == 5u && (nextValue & NXBasics::BitMask::Bit14) != 0)
					{
						direction = 6;
					}
				}
			}

			const auto countOffset = NC2Layout::SectorPathfinderOffset::DirectionRunCountsUnsigned + (runs - 1u) * NC2Layout::RecordSize::PathfinderDirectionRun;

			if (direction == previous && Read<std::uint8_t>(pathfinder, countOffset) != 255u)
			{
				Write<std::uint8_t>(pathfinder, countOffset, static_cast<std::uint8_t>(Read<std::uint8_t>(pathfinder, countOffset) + 1u));
			}
			else
			{
				if (runs == 20u)
					break;

				Write<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRuns + runs * NC2Layout::RecordSize::PathfinderDirectionRun, static_cast<std::uint8_t>(direction));
				Write<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunCounts + runs * NC2Layout::RecordSize::PathfinderDirectionRun, 1);
				++runs;
				previous = direction;
			}

			MapMigPoint_MoveCursorUnwrapped(point, direction);

			if (blockedArea && (GameMap_CalculationMap_GetValue(&point) & NXBasics::BitMask::Bit9) != 0)
			{
				Write(pathfinder, NC2Layout::SectorPathfinderOffset::BlockedAreaSteps, Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::BlockedAreaSteps) + 1u);
			}
			else
			{
				blockedArea = false;
			}
		}

		if (runs < 20u)
			Write<std::uint8_t>(pathfinder, NC2Layout::SectorPathfinderOffset::DirectionRunCounts + runs * NC2Layout::RecordSize::PathfinderDirectionRun, 0);

		Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::RouteAvailable);

		return true;
	}

	// Original function: FUN_00470735; this blocked-area search is independent of vehicle clearance.
	void CMapPathfinderSpecialSectorSearchElement::l_UnblockArea(void* pathfinder, const void* point)
	{
		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[1].count = 0;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		GameMap_CalculationMap_SetValue(point, NC2Runtime::PathfinderCalculationValue::BlockedStartAreaFlag);
		std::uint32_t current = 0;
		do
		{
			auto& source = sMapPathfinderStaticVariables[current];
			auto& destination = sMapPathfinderStaticVariables[(current - 1u) & 1u];
			destination.count = 0;

			for (std::uint32_t index = 0; index < source.count; ++index)
			{
				const void* microPoint = GameMap_GetConstMapMicroPointPtr(source.points + index);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = source.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if ((GameMap_CalculationMap_GetValue(&next) & NXBasics::BitMask::Bit9) != 0)
						continue;

					const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);

					if ((flags & NXBasics::BitMask::Bit7) == 0)
						continue;

					if (destination.count == 4000u)
						continue;

					destination.points[destination.count++] = next;
					GameMap_CalculationMap_SetValue(&next, NC2Runtime::PathfinderCalculationValue::BlockedStartAreaFlag);
				}
			}

			current = (current + 1u) & 1u;
		} while (sMapPathfinderStaticVariables[current].count != 0);
	}

	// Original function: FUN_0043EC04; the special sector workspace has its own direction fields.
	void CMapPathfinderSpecialSectorSearchElement::l_GetPreferedDirections(void* pathfinder, const void* source, const void* destination)
	{
		const auto dx = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapCoordinatePairOffset::X)) - Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X);
		const auto dy = static_cast<std::int32_t>(Read<std::int16_t>(destination, NC2Layout::MapCoordinatePairOffset::Y)) - Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y);
		auto x = dx < 0 ? -dx : dx;
		const auto y = dy < 0 ? -dy : dy;

		if ((y & 1) != 0 && ((Read<std::int16_t>(destination, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 ? dx < 0 : dx > 0))
			--x;

		const auto halfY = static_cast<std::int16_t>(y / 2);

		if (x > halfY)
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1, dy < 0 ? (dx < 0 ? 4u : 5u) : (dx < 0 ? 2u : 1u));
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2, dx < 0 ? 3u : 0u);
		}
		else
		{
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1, dy < 0 ? 5u : 1u);
			Write<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2, dy < 0 ? 4u : 2u);
		}
	}

	// Original function: FUN_00470860. Rectangle, queue costs and equal-cost direction masks follow Windows.
	bool CMapPathfinderSpecialSectorSearchElement::l_FindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > 511u)
			return false;

		const auto* rectangle = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle);

		if (!MapMigPoint_IsInside(source, rectangle) || !MapMigPoint_IsInside(destination, rectangle))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 4u;

		if (budget > 511u)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			l_UnblockArea(pathfinder, source);

		for (std::uint32_t index = 0; index < 12; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
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
				l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInside(&next, rectangle))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = 0;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2))
						cost += 4u;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + 3u) % 6u);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost + 1u) % 12u];

					if (target.count == 4000u)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % 12u;
			budget = remaining;
		} while (budget > 13u);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_00470B42. Rectangle, queue costs and equal-cost direction masks follow Windows.
	bool CMapPathfinderSpecialSectorSearchElement::l_VehicleFindWayDirected(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > 511u)
			return false;

		const auto* rectangle = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle);

		if (!MapMigPoint_IsInside(source, rectangle) || !MapMigPoint_IsInside(destination, rectangle))
			return false;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 4u;

		if (budget > 511u)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			l_UnblockArea(pathfinder, source);

		for (std::uint32_t index = 0; index < 12; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
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
				l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInsideMap(&next))
						continue;

					if (!MapMigPoint_IsInside(&next, rectangle))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if ((nextFlags & NXBasics::BitMask::Bit7) != 0 && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					if ((nextFlags & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance))
						continue;

					std::uint32_t cost = 0;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2))
						cost += 4u;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + 3u) % 6u);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost + 1u) % 12u];

					if (target.count == 4000u)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % 12u;
			budget = remaining;
		} while (budget > 13u);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_00470E3D. Rectangle, queue costs and equal-cost direction masks follow Windows.
	bool CMapPathfinderSpecialSectorSearchElement::l_FindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > 511u)
			return false;

		const auto* rectangle = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle);

		if (!MapMigPoint_IsInside(source, rectangle) || !MapMigPoint_IsInside(destination, rectangle))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 8u;

		if (budget > 511u)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			l_UnblockArea(pathfinder, source);

		for (std::uint32_t index = 0; index < 16; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
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
				l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					if ((Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::WalkingDirections) & (1u << direction)) == 0)
						continue;

					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInside(&next, rectangle))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if (((nextFlags & NXBasics::BitMask::Bit7) != 0) && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					std::uint32_t cost = (nextFlags >> 3) & 15u;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2))
						cost += 4u;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + 3u) % 6u);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost) & 15u];

					if (target.count == 4000u)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % 16u;
			budget = remaining;
		} while (budget > 17u);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_0047111E. Rectangle, queue costs and equal-cost direction masks follow Windows.
	bool CMapPathfinderSpecialSectorSearchElement::l_VehicleFindWayDirectedWithResitance(void* pathfinder, const void* source, const void* destination, std::uint32_t radius)
	{
		if (static_cast<std::uint32_t>(PathfinderDistance(source, destination)) > radius || radius > 511u)
			return false;

		const auto* rectangle = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle);

		if (!MapMigPoint_IsInside(source, rectangle) || !MapMigPoint_IsInside(destination, rectangle))
			return false;

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance))
			return false;

		GameMap_CalculationMap_Clear(destination, static_cast<std::int32_t>(radius * 2u));
		std::uint32_t budget = radius * 8u;

		if (budget > 511u)
			return false;

		GameMap_CalculationMap_SetValue(source, 0);
		const auto flags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::WalkingFlags);

		if ((flags & NXBasics::BitMask::Bit7) != 0)
			l_UnblockArea(pathfinder, source);

		for (std::uint32_t index = 0; index < 16; ++index)
		{
			sMapPathfinderStaticVariables[index].count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
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
				l_GetPreferedDirections(pathfinder, queue.points + index, source);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto next = queue.points[index];
					MapMigPoint_MoveCursorUnwrapped(next, direction);

					if (!MapMigPoint_IsInsideMap(&next))
						continue;

					if (!MapMigPoint_IsInside(&next, rectangle))
						continue;

					const auto nextFlags = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&next), NC2Layout::MapMicroPointOffset::WalkingFlags);
					const auto value = static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&next));

					if ((nextFlags & NXBasics::BitMask::Bit7) != 0 && (value & NXBasics::BitMask::Bit9Unsigned) == 0)
						continue;

					if ((nextFlags & 7u) < Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::RequiredClearance))
						continue;

					std::uint32_t cost = (nextFlags >> 3) & 15u;

					if (direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection1) && direction != Read<std::uint32_t>(pathfinder, NC2Layout::SectorPathfinderOffset::PreferredDirection2))
						cost += 4u;

					const std::uint32_t oldCost = value & NXBasics::BitMask::Low9Bits;

					if (oldCost >= budget - cost)
						continue;

					const auto reverseMask = NC2Runtime::PathfinderCalculationValue::DirectionMaskBase << ((direction + 3u) % 6u);

					if (remaining - cost == oldCost)
					{
						GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>(value | reverseMask));
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost) & 15u];

					if (target.count == 4000u)
						continue;

					target.points[target.count++] = next;
					++queued;
					GameMap_CalculationMap_SetValue(&next, static_cast<std::int16_t>((value & NXBasics::BitMask::Bit9Unsigned) | reverseMask | (remaining - cost)));

					if (next == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			current = (current + 1u) % 16u;
			budget = remaining;
		} while (budget > 17u);

		return (GameMap_CalculationMap_GetValue(source) & NXBasics::BitMask::Low9BitsSigned) != 0;
	}

	// Original function: FUN_0044F14C; outlined member construction, no search-state initialization.
	void CMapPathfinderSpecialSectorSearchElement::ConstructMembers(void* pathfinder)
	{
		MapMigPoint_ConstructZero(static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::CurrentPosition);
		MapMigPoint_ConstructZero(static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::DestinationPosition);
		MapMigRectangle_ConstructZero(reinterpret_cast<std::int16_t*>(static_cast<std::byte*>(pathfinder) + NC2Layout::SectorPathfinderOffset::SearchRectangle));
	}

}