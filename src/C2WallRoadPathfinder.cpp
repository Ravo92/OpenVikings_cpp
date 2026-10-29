#include "C2GameMapAccess.h"
#include "C2HumanGoods.h"
#include "C2LifetimeAccess.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2WallRoadPathfinder.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Original function: FUN_0043e84b; constructor-only embedded point initialization.
		void PathfinderWallRoadElement_ConstructPoints(void* element)
		{
			Write<std::uint32_t>(element, NC2Layout::WallRoadPathfinderOffset::Source, 0);
			Write<std::uint32_t>(element, NC2Layout::WallRoadPathfinderOffset::Target, 0);
		}

		std::uint32_t Distance(const void* a, const void* b)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		std::uint16_t Calculation(std::uint32_t point)
		{

			return static_cast<std::uint16_t>(GameMap_CalculationMap_GetValue(&point));
		}

		void SetCalculation(std::uint32_t point, std::uint32_t value)
		{
			GameMap_CalculationMap_SetValue(&point, static_cast<std::int16_t>(value));
		}
	}

	// Original function: FUN_0043e82e.
	CMapPathfinderSpecialWallRoadElement::CMapPathfinderSpecialWallRoadElement()
	{
		PathfinderWallRoadElement_ConstructPoints(this);
		std::memset(this, 0, sizeof(*this));
		state = 0;
	}

	// Original function: FUN_0043e860.
	void CMapPathfinderSpecialWallRoadElement::StartPathfinder(std::uint32_t playerId, bool isRoad, const void* start, const void* end, std::uint32_t searchRadius)
	{
		player = playerId;
		road = isRoad;
		state = 1;
		source = Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates);
		target = Read<std::uint32_t>(end, NC2Layout::MapCoordinatePairOffset::Coordinates);

		if (source == target)
			state = 3;

		radius = searchRadius;
	}

	// Original function: FUN_0043e8aa; -3 inactive, -2 arrived, -1 failed.
	std::int32_t CMapPathfinderSpecialWallRoadElement::GetNextDirection()
	{
		if (state == 0)
			return -3;

		if (state == 3 || (state == 1 && source == target))
			return -2;

		if (state == 1)
		{
			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&source), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&target), NC2Layout::MapMicroPointOffset::ContinentId))
				return -1;

			if (!l_FindWayAndBuildVectorList(&source, &target, radius))
				return -1;

			state = 2;
		}

		if ((vectorIndex == 20 || vectors[vectorIndex].amount == 0) && state == 2 && !l_FindWayAndBuildVectorList(&source, &target, radius))
			return -1;

		const auto direction = vectors[vectorIndex].direction;

		if (--vectors[vectorIndex].amount == 0)
			++vectorIndex;

		MapMigPoint_MoveCursorUnwrapped(source, direction);

		if (source == target)
			state = 3;

		return direction;
	}

	// Original function: FUN_0043e992; calculation-map direction ties retain the previous run.
	bool CMapPathfinderSpecialWallRoadElement::l_FindWayAndBuildVectorList(const void* start, const void* end, std::uint32_t searchRadius)
	{
		if (!IsPointOkForBuilding(player, road, end) || (!l_FindWayWithLine(start, end, searchRadius) && !l_FindWayDirected(start, end, searchRadius)))
			return false;

		vectorIndex = 0;
		auto point = source;
		std::uint32_t count = 0, previous = 1000;

		for (auto value = Calculation(point); (value & NXBasics::BitMask::WordExceptBits0Through9) != 0; value = Calculation(point))
		{
			std::uint32_t choices[6], choicesCount = 0, direction = 1000;

			for (std::uint32_t i = 0; i < 6; ++i)
			{
				if ((value & (0x400u << i)) != 0)
				{
					choices[choicesCount++] = i;

					if (i == previous)
						direction = previous;
				}
			}

			if (direction == 1000)
				direction = choices[0];

			if (direction == previous && vectors[count - 1].amount != 255)
			{
				++vectors[count - 1].amount;
			}
			else
			{
				if (count == 20)
					break;

				vectors[count++] = { static_cast<std::uint8_t>(direction), 1 };
				previous = direction;
			}

			MapMigPoint_MoveCursorUnwrapped(point, direction);
		}

		if (count < 20)
			vectors[count].amount = 0;

		state = 2;

		return true;
	}

	// Original function: FUN_0043eaac.
	bool CMapPathfinderSpecialWallRoadElement::l_FindWayWithLine(const void* start, const void* end, std::uint32_t searchRadius)
	{
		if (Distance(start, end) > searchRadius || searchRadius > 511)
			return false;

		GameMap_CalculationMap_Clear(end, static_cast<std::int32_t>(searchRadius + 2));
		SetCalculation(Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates), 0);
		SetCalculation(Read<std::uint32_t>(end, NC2Layout::MapCoordinatePairOffset::Coordinates), searchRadius);
		auto point = Read<std::uint32_t>(end, NC2Layout::MapCoordinatePairOffset::Coordinates);

		if (!IsPointOkForBuilding(player, road, &point))
			return false;

		for (;;)
		{
			l_GetPreferedDirectionsAndDistance(&point, start);
			auto direction = preferred[distance[0] > distance[1] ? 0 : 1];

			if (!l_IsPointOkForBuilding(point, direction))
			{
				direction = preferred[distance[0] > distance[1] ? 1 : 0];

				if (!l_IsPointOkForBuilding(point, direction))
					return false;
			}

			MapMigPoint_MoveCursorUnwrapped(point, direction);
			GameMap_GetConstMapMicroPointPtr(&point);
			Calculation(point);
			SetCalculation(point, (0x400u << ((direction + 3u) % 6u)) | (searchRadius - 1u));

			if (point == Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates))
				return true;

			if (--searchRadius <= 1)
			{

				return false;
			}
		}
	}

	// Original function: FUN_0043ec04.
	void CMapPathfinderSpecialWallRoadElement::l_GetPreferedDirections(const void* start, const void* end)
	{
		const auto dx = Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::X) - Read<std::int16_t>(start, NC2Layout::MapCoordinatePairOffset::X), dy = Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y) - Read<std::int16_t>(start, NC2Layout::MapCoordinatePairOffset::Y);
		auto x = dx < 0 ? -dx : dx;
		const auto y = dy < 0 ? -dy : dy;

		if ((y & 1) != 0 && ((Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 ? dx < 0 : dx > 0))
			--x;

		if (x > static_cast<std::int16_t>(y / 2))
		{
			preferred[0] = dy >= 0 ? dx >= 0 ? 1u : 2u : dx >= 0 ? 5u
			                                                     : 4u;
			preferred[1] = dx >= 0 ? 0u : 3u;
		}
		else
		{
			preferred[0] = dy >= 0 ? 1u : 5u;
			preferred[1] = dy >= 0 ? 2u : 4u;
		}
	}

	// Original function: FUN_0043ecb7; distinct Windows entry also calculates direction weights.
	void CMapPathfinderSpecialWallRoadElement::l_GetPreferedDirectionsAndDistance(const void* start, const void* end)
	{
		const auto dx = Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::X) - Read<std::int16_t>(start, NC2Layout::MapCoordinatePairOffset::X), dy = Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y) - Read<std::int16_t>(start, NC2Layout::MapCoordinatePairOffset::Y);
		auto x = dx < 0 ? -dx : dx;
		const auto y = dy < 0 ? -dy : dy;

		if ((y & 1) != 0 && ((Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 ? dx < 0 : dx > 0))
			--x;

		const auto half = static_cast<std::int16_t>(y / 2);

		if (x > half)
		{
			preferred[0] = dy >= 0 ? dx >= 0 ? 1u : 2u : dx >= 0 ? 5u
			                                                     : 4u;
			preferred[1] = dx >= 0 ? 0u : 3u;
			distance[0] = static_cast<std::uint32_t>(y);
			distance[1] = static_cast<std::uint32_t>(x - half);
		}
		else
		{
			preferred[0] = dy >= 0 ? 1u : 5u;
			preferred[1] = dy >= 0 ? 2u : 4u;

			if (half != 0)
			{
				if (dx >= 0)
				{
					distance[1] = static_cast<std::uint32_t>(half - x);
					distance[0] = static_cast<std::uint32_t>(y) - distance[1];
				}
				else
				{
					distance[0] = static_cast<std::uint32_t>(half - x);
					distance[1] = static_cast<std::uint32_t>(y) - distance[0];
				}
			}
			else if (y != 0 && (dx > 0 || (dx == 0 && x == 0 && (Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0)))
			{
				distance[0] = static_cast<std::uint32_t>(y);
				distance[1] = 0;
			}
			else
			{
				distance[0] = 0;
				distance[1] = static_cast<std::uint32_t>(y);
			}
		}
	}

	// Original function: FUN_0043edee; twelve of the shared sixteen queues are used.
	bool CMapPathfinderSpecialWallRoadElement::l_FindWayDirected(const void* start, const void* end, std::uint32_t searchRadius)
	{
		if (Distance(start, end) > searchRadius || searchRadius > 511 || !IsPointOkForBuilding(player, road, end))
			return false;

		GameMap_CalculationMap_Clear(end, static_cast<std::int32_t>(searchRadius * 2u));
		auto cost = searchRadius * 4u;

		if (cost > 511)
			return false;

		SetCalculation(Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates), 0);

		for (std::uint32_t i = 0; i < 12; ++i)
		{
			sMapPathfinderStaticVariables[i].count = 0;
		}

		sMapPathfinderStaticVariables[0].points[0] = Read<std::uint32_t>(end, NC2Layout::MapCoordinatePairOffset::Coordinates);
		sMapPathfinderStaticVariables[0].count = 1;
		SetCalculation(Read<std::uint32_t>(end, NC2Layout::MapCoordinatePairOffset::Coordinates), cost);
		std::uint32_t queued = 1, ring = 0;

		for (;;)
		{
			auto& queue = sMapPathfinderStaticVariables[ring];
			const auto next = cost - 1u;

			for (std::uint32_t i = 0; i < queue.count; ++i)
			{
				const auto current = queue.points[i];
				--queued;
				GameMap_GetConstMapMicroPointPtr(&current);
				l_GetPreferedDirections(&current, start);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					if (!l_IsPointOkForBuilding(current, direction))
						continue;

					auto point = current;
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					GameMap_GetConstMapMicroPointPtr(&point);
					const auto value = Calculation(point);
					const auto penalty = direction == preferred[0] || direction == preferred[1] ? 0u : 4u;

					if ((value & 511u) >= cost - penalty)
						continue;

					const auto reverse = 0x400u << ((direction + 3u) % 6u);

					if ((value & 511u) == next - penalty)
					{
						SetCalculation(point, value | reverse);
						continue;
					}

					auto& destination = sMapPathfinderStaticVariables[(ring + penalty + 1u) % 12u];

					if (destination.count == 4000)
						continue;

					destination.points[destination.count++] = point;
					++queued;
					SetCalculation(point, reverse | (next - penalty));

					if (point == Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates))
						return true;
				}
			}

			queue.count = 0;

			if (queued == 0)
				break;

			ring = (ring + 1u) % 12u;
			cost = next;

			if (next <= 13)
				break;
		}

		return (Calculation(Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates)) & 511u) != 0;
	}

	// Original function: FUN_0043f062; point is copied and moved without wrapping.
	bool CMapPathfinderSpecialWallRoadElement::l_IsPointOkForBuilding(std::uint32_t point, std::uint32_t direction) const
	{
		MapMigPoint_MoveCursorUnwrapped(point, direction);

		return IsPointOkForBuilding(player, road, &point);
	}

	// Original function: FUN_0043f08b; endpoint marks use two alternating queues.
	void CMapPathfinderSpecialWallRoadElement::SearchOverMap(std::uint32_t playerId, bool isRoad, const void* start, std::uint32_t searchRadius)
	{
		if (searchRadius > 511 || !IsPointOkForBuilding(playerId, isRoad, start))
		{

			return;
		}

		GameMap_CalculationMap_Clear(start, static_cast<std::int32_t>(searchRadius + 2u));
		auto& first = sMapPathfinderStaticVariables[0];
		sMapPathfinderStaticVariables[0].count = sMapPathfinderStaticVariables[1].count = 0;
		first.count = 1;
		first.points[0] = Read<std::uint32_t>(start, NC2Layout::MapCoordinatePairOffset::Coordinates);
		SetCalculation(first.points[0], searchRadius);
		std::uint32_t ring = 0;

		for (;;)
		{
			auto& queue = sMapPathfinderStaticVariables[ring];
			auto& nextQueue = sMapPathfinderStaticVariables[(ring - 1u) & 1u];
			nextQueue.count = 0;
			const auto next = searchRadius - 1u;

			for (std::uint32_t i = 0; i < queue.count; ++i)
			{
				GameMap_GetConstMapMicroPointPtr(&queue.points[i]);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto point = queue.points[i];
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (!IsPointOkForBuilding(playerId, isRoad, &point) || (Calculation(point) & 511u) != 0 || nextQueue.count == 4000)
						continue;

					nextQueue.points[nextQueue.count++] = point;
					SetCalculation(point, next);
				}
			}

			ring = (ring + 1u) % 2u;

			if (nextQueue.count == 0 || next <= 1)
				break;

			searchRadius = next;
		}
	}

	// Original function: FUN_0043f1dc; land, exploration, ground good and wall clearance.
	bool CMapPathfinderSpecialWallRoadElement::IsPointOkForBuilding(std::uint32_t playerId, bool isRoad, const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point) || MapMigPoint_IsBorderPoint(point) || !GameMap_Exploration_HasBeenSeenOnce(playerId, point))
			return false;

		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId)) != 1 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) == 0 || !Map_GoodTool_CanBePutOnMap(isRoad ? 3u : 5u, point))
			return false;

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 1)
			return false;

		if (isRoad)
			return true;

		auto cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

		for (std::uint32_t ring = 1; ring <= 2; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(cursor, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(cursor, direction);
					const auto* neighbor = GameMap_GetConstMapMicroPointPtr(&cursor);

					if ((Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0)
						continue;

					const auto type = Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::ObjectData);

					if (Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Landscape || type == 0 || (type != SwitchIds::WallLandscapeType::Wall && type != SwitchIds::WallLandscapeType::GateOpen && type != SwitchIds::WallLandscapeType::GateClosed))
						return false;
				}
			}
		}

		return true;
	}

	// Original function: FUN_00427ad5.
	void MapTool_Wall_Build_MarkPossibleEndPoints(std::uint32_t playerId, const void* point)
	{
		CMapPathfinderSpecialWallRoadElement::SearchOverMap(playerId, false, point, 20);
	}

	// Original function: FUN_00427aea.
	void MapTool_Wall_Build_GetPossibleVectorList(std::uint32_t playerId, void* path, const void* start, const void* end)
	{
		static_cast<CMapPathfinderSpecialWallRoadElement*>(path)->StartPathfinder(playerId, false, start, end, 20);
	}

	// Original function: FUN_00428707.
	void Tool_RoadBuildManager_MarkPossibleEndPoints(std::uint32_t playerId, const void* point)
	{
		CMapPathfinderSpecialWallRoadElement::SearchOverMap(playerId, true, point, 20);
	}

	// Original function: FUN_0042871c.
	void Tool_RoadBuildManager_GetPossibleVectorList(std::uint32_t playerId, void* path, const void* start, const void* end)
	{
		static_cast<CMapPathfinderSpecialWallRoadElement*>(path)->StartPathfinder(playerId, true, start, end, 20);
	}

	std::int32_t __fastcall MapPathfinderSpecialWallRoadElement_GetNextDirection(void* path, void*)
	{

		return static_cast<CMapPathfinderSpecialWallRoadElement*>(path)->GetNextDirection();
	}
}