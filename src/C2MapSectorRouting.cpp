#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapPathfinderSectorSearch.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "C2VehicleLifecycle.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		std::uint32_t SectorIndex(const SSectorGridCursor& cursor)
		{

			return static_cast<std::uint32_t>(CMapWalkSectorManager::mStaticVars.gridWidth) * static_cast<std::uint32_t>(cursor.y) + static_cast<std::uint32_t>(cursor.x);
		}

		SMapWalkSectorData* Sector(const SSectorGridCursor& cursor, bool land)
		{

			return land ? CMapWalkSectorManager::GetLandSectorDataPtr(cursor) : CMapWalkSectorManager::GetWaterSectorDataPtr(cursor);
		}

		std::uint32_t Distance(const void* a, const void* b)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y)));
		}

		std::uint32_t PackSector(const SSectorGridCursor& cursor)
		{

			return static_cast<std::uint16_t>(cursor.x) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(cursor.y)) << 16);
		}

		SSectorGridCursor UnpackSector(std::uint32_t point)
		{
			SSectorGridCursor cursor;
			cursor.x = static_cast<std::int16_t>(point);
			cursor.y = static_cast<std::int16_t>(point >> 16);

			return cursor;
		}

		struct SBasePointSearch
		{
			std::uint32_t point;
			std::uint8_t land;
			std::byte padding[3];
			std::uint32_t clearance;
		};

		static_assert(sizeof(SBasePointSearch) == 12);

		// Original function: FUN_00450471; Mac: slWalkSectorBasePointSearch.
		bool __cdecl slWalkSectorBasePointSearch(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SBasePointSearch*>(callback->userData);

			if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::TerrainFlags) & NXBasics::BitMask::Bit4) == 0)
				return false;

			auto* sector = Sector(SSectorGridCursor(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates)), data.land != 0);

			for (std::uint32_t direction = 0; direction < 8; ++direction)
			{
				if ((sector->connectionFlags & (1u << direction)) == 0 || sector->connectionValues[direction] < data.clearance)
					continue;

				data.point = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
				return true;
			}

			return false;
		}
	}

	// Original function: FUN_00454405; Mac: CAIMilitaryManager::Military_GetCivilianDangerValueForSector.
	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForSector(const SSectorGridCursor& cursor, std::int32_t player)
	{
		const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
		const auto index = static_cast<std::uint32_t>(manager->GetGridWidth()) * static_cast<std::uint32_t>(cursor.y) + static_cast<std::uint32_t>(cursor.x);
		const auto* sector = manager->GetSectorData() + static_cast<std::size_t>(index) * 0x540u;
		std::uint32_t total = 0;

		for (std::int32_t other = 0; other < 21; ++other)
		{
			if (other == 20)
			{
				total += Read<std::uint32_t>(sector, NC2Layout::SectorAttachmentOffset::NeutralAggressiveAnimalCount) * 10u;
				continue;
			}

			if (Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(other) * NC2Layout::RecordSize::GameControlPlayer) == 0)
				continue;

			const auto* data = sector + static_cast<std::uint32_t>(other) * 0x40u;
			const auto common = Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryVehicleCount) * 5u + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHouseCount) * 4u;
			const auto relation = GameControl_Diplomacy_GetState(gGameControl, player, other);

			if (relation == 4)
			{
				total -= Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianCount) + (common + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHumanCount)) * 10u;
			}
			else if (relation == 3)
			{
				total += Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianCount) + (Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianHouseCount) + (common + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::AggressiveAnimalCount) + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHumanCount)) * 2u) * 5u;
			}
			else if (relation == 2)
			{
				total += Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianHouseCount) * 3u + (common + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::AggressiveAnimalCount) + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHumanCount)) * 5u;
			}
		}

		return NXBasics::Runtime_TimeSigned32(total);
	}

	// Original function: FUN_0045437A. The ring walks four even directions with two steps per radius.
	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForPosition(const void* point, std::int32_t player, std::uint32_t radius)
	{
		SSectorGridCursor cursor(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		std::uint32_t total = static_cast<std::uint32_t>(AIMilitaryManager_Military_GetCivilianDangerValueForSector(cursor, player));

		for (std::int32_t ring = 1; ring < NXBasics::Runtime_TimeSigned32(radius); ++ring)
		{
			cursor.MoveCursorUnwrapped(5);

			for (std::uint32_t direction = 0; direction < 8; direction += 2)
			{
				for (std::int32_t step = 0; step < ring * 2; ++step)
				{
					cursor.MoveCursorUnwrapped(direction);

					if (cursor.IsInsideMap())
						total += static_cast<std::uint32_t>(AIMilitaryManager_Military_GetCivilianDangerValueForSector(cursor, player) / (ring * ring));
				}
			}
		}

		return NXBasics::Runtime_TimeSigned32(total);
	}

	// Original function: FUN_0044FD3E. Both reciprocal connections use the original bounded search workspace.
	void CMapWalkSectorManager::l_Sectors_CheckAndSetConnection(SSectorGridCursor cursor, std::uint32_t direction)
	{
		std::int16_t rectangle[4] = { static_cast<std::int16_t>(cursor.x * 20), static_cast<std::int16_t>(cursor.y * 20), 20, 20 };
		// Jump-table entries recovered from Game.exe at 0044FFF4.

		if (direction == 3 || direction == 4 || direction == 5)
			rectangle[0] = static_cast<std::int16_t>(rectangle[0] - 20);

		if (direction == 5 || direction == 6 || direction == 7)
			rectangle[1] = static_cast<std::int16_t>(rectangle[1] - 20);

		if (direction == 0 || direction == 1 || direction == 3 || direction == 4 || direction == 5 || direction == 7)
			rectangle[2] = 40;

		if (direction >= 1 && direction <= 3 || direction >= 5 && direction <= 7)
			rectangle[3] = 40;

		auto* sourceLand = GetLandSectorDataPtr(cursor);
		auto* sourceWater = GetWaterSectorDataPtr(cursor);
		cursor.MoveCursorUnwrapped(direction);

		if (!cursor.IsInsideMap())
			return;

		auto* targetLand = GetLandSectorDataPtr(cursor);
		auto* targetWater = GetWaterSectorDataPtr(cursor);

		if (targetLand->calculated == 0 || targetWater->calculated == 0)
		{

			return;
		}

		const auto reverse = (direction - 4u) & 7u;
		auto* workspace = mStaticVars.specialSectorSearchWorkspace;
		const auto connect = [&](SMapWalkSectorData* source, SMapWalkSectorData* target, std::uint32_t maximum)
		{
			if (source->continentId == 0 || source->continentId != target->continentId)
			{
				source->connectionFlags &= static_cast<std::uint8_t>(~(1u << direction));
				target->connectionFlags &= static_cast<std::uint8_t>(~(1u << reverse));

				return;
			}

			CMapPathfinderSpecialSectorSearchElement::Init(workspace);
			Write<std::uint8_t>(workspace, NC2Layout::SectorPathfinderOffset::AllowVerticalDirectionsFlag, 1);
			std::int32_t clearance = static_cast<std::int32_t>(source->connectionValues[direction] > maximum ? maximum : source->connectionValues[direction]);
			std::int32_t result = static_cast<std::int32_t>(direction);

			while (clearance >= 0)
			{
				Write<std::int32_t>(workspace, NC2Layout::SectorPathfinderOffset::RequiredClearance, clearance);
				CMapPathfinderSpecialSectorSearchElement::StartPathfinder(workspace, source->basePoints, target->basePoints, 50, true, rectangle);
				result = CMapPathfinderSpecialSectorSearchElement::GetNextDirection(workspace);

				if (result >= 0)
					break;

				--clearance;
			}

			if (result != -3 && result != -1)
			{
				source->connectionValues[direction] = static_cast<std::uint32_t>(clearance);
				target->connectionValues[reverse] = static_cast<std::uint32_t>(clearance);
				source->connectionFlags |= static_cast<std::uint8_t>(1u << direction);
				target->connectionFlags |= static_cast<std::uint8_t>(1u << reverse);
			}
			else
			{
				source->connectionFlags &= static_cast<std::uint8_t>(~(1u << direction));
				target->connectionFlags &= static_cast<std::uint8_t>(~(1u << reverse));
			}
		};
		connect(sourceLand, targetLand, 1);
		connect(sourceWater, targetWater, 4);
	}

	// Original Windows inline calculation-map accesses in 00450014/00450505.
	std::uint32_t CMapWalkSectorManager::Sectors_CalculationMap_GetValue(const SSectorGridCursor& cursor)
	{

		return mStaticVars.calculationMap[SectorIndex(cursor)];
	}

	void CMapWalkSectorManager::l_Sectors_CalculationMap_SetValue(const SSectorGridCursor& cursor, std::uint32_t value)
	{
		mStaticVars.calculationMap[SectorIndex(cursor)] = value;
	}

	// Original function: FUN_00450014.
	SMapWalkSectorData* CMapWalkSectorManager::l_GetNextSectorPtr(const SSectorGridCursor& source, const SSectorGridCursor& destination, std::uint32_t clearance, bool land, SMapWalkSectorData*& second, std::uint32_t radius, const void* sourcePoint, std::int32_t player)
	{
		auto* first = Sector(source, land);
		const auto* target = Sector(destination, land);

		if (first->calculated == 0 || target->calculated == 0 || first->continentId != target->continentId || !WayPossible(source, destination, clearance, land, player))
			return nullptr;

		auto cursor = source;

		if ((Sectors_CalculationMap_GetValue(cursor) & NXBasics::BitMask::Bits16Through23) != 0)
			first = nullptr;

		SMapWalkSectorData* best = nullptr;
		std::uint32_t bestDistance = 0;

		for (std::uint32_t step = 0; step < 2; ++step)
		{
			const auto value = Sectors_CalculationMap_GetValue(cursor);

			if ((value & NXBasics::BitMask::Bits16Through23) == 0)
				return first;

			std::uint32_t directions[8], count = 0;

			for (std::uint32_t direction = 0; direction < 8; ++direction)
			{
				if ((value & (0x10000u << direction)) != 0)
					directions[count++] = direction;
			}

			cursor.MoveCursorUnwrapped(directions[Game_Random_GetNumber() % count]);

			if (!cursor.IsInsideMap())
				continue;

			if (step == 0)
			{
				first = Sector(cursor, land);
			}
			else
			{
				second = Sector(cursor, land);
				const auto distance = Distance(sourcePoint, second->basePoints);

				if (distance < radius && distance > bestDistance)
				{
					best = second;
					bestDistance = distance;
				}
			}
		}

		second = best;

		return first;
	}

	// Original function: FUN_00450222. An absent second sector leaves the caller's second point untouched.
	bool CMapWalkSectorManager::GetNextSectorBasePoint(const void* source, const void* destination, std::uint32_t clearance, bool land, void* firstPoint, void* secondPoint, std::uint32_t radius, std::int32_t player)
	{
		SBasePointSearch data{ 0, static_cast<std::uint8_t>(land), { }, clearance };

		if (!CMapPathfinderElement::SearchOverMap(source, 50, true, slWalkSectorBasePointSearch, &data, true, player))
			return false;

		const SSectorGridCursor sourceSector(data.point);
		auto* sourceData = Sector(sourceSector, land);

		if (sourceData->calculated == 0 || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId) != sourceData->continentId)
			return false;

		if (!CMapPathfinderElement::SearchOverMap(destination, 50, true, slWalkSectorBasePointSearch, &data, true, player))
			return false;

		if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&data.point), NC2Layout::MapMicroPointOffset::ContinentId) != sourceData->continentId)
			return false;

		const SSectorGridCursor targetSector(data.point);

		if (sourceSector.x == targetSector.x && sourceSector.y == targetSector.y)
			return false;

		if (Sector(targetSector, land)->continentId != sourceData->continentId)
			return false;

		SMapWalkSectorData* second = nullptr;
		auto* first = l_GetNextSectorPtr(sourceSector, targetSector, clearance, land, second, radius, source, player);

		if (first == nullptr)
			return false;

		if (second != nullptr)
		{
			Write(secondPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, second->basePoints[Game_Random_GetNumber() % second->basePointCount]);

			if (Distance(secondPoint, source) > radius)
			{
				Write(secondPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, first->basePoints[0]);

				if (Distance(secondPoint, source) > radius)
					Write(secondPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, sourceData->basePoints[0]);
			}
		}

		Write(firstPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, first->basePoints[Game_Random_GetNumber() % first->basePointCount]);

		if (Distance(firstPoint, source) > radius)
		{
			Write(firstPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, first->basePoints[0]);

			if (Distance(firstPoint, source) > radius)
				Write(firstPoint, NC2Layout::MapCoordinatePairOffset::Coordinates, sourceData->basePoints[0]);
		}

		return true;
	}

	// Original function: FUN_00450505. Sector costs use sixteen shared buckets and stop when the source is inserted.
	bool CMapWalkSectorManager::WayPossible(const SSectorGridCursor& source, const SSectorGridCursor& destination, std::uint32_t clearance, bool land, std::int32_t player)
	{
		l_Sectors_CalculationMap_ClearAll();
		l_Sectors_CalculationMap_SetValue(source, 0);
		std::uint32_t budget = NXBasics::IntegerRange::UInt16Maximum;

		for (auto& queue : sMapPathfinderStaticVariables)
		{
			queue.count = 0;
		}

		sMapPathfinderStaticVariables[0].count = 1;
		sMapPathfinderStaticVariables[0].points[0] = PackSector(destination);
		const bool dangerCosts = player != -1 && player < 20 && Read<std::uint32_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::Type) == 2;
		l_Sectors_CalculationMap_SetValue(destination, NXBasics::IntegerRange::UInt16Maximum);
		std::uint32_t current = 0, pending = 1;
		do
		{
			const auto remaining = budget - 1u;
			auto& queue = sMapPathfinderStaticVariables[current];

			for (std::uint32_t index = 0; index < queue.count; ++index)
			{
				--pending;
				const auto cursor = UnpackSector(queue.points[index]);
				const auto* sector = Sector(cursor, land);

				for (std::uint32_t direction = 0; direction < 8; ++direction)
				{
					if ((sector->connectionFlags & (1u << direction)) == 0 || sector->connectionValues[direction] < clearance)
						continue;

					auto next = cursor;
					next.MoveCursorUnwrapped(direction);

					if (!next.IsInsideMap())
						continue;

					const auto value = Sectors_CalculationMap_GetValue(next);
					const auto oldCost = value & NXBasics::IntegerRange::UInt16Maximum;
					std::uint32_t cost = 1;

					if (dangerCosts)
					{
						const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(sector->basePoints, player, 2) / 10;
						cost = danger <= 0 ? 1u : danger >= 16 ? 15u
						                                       : static_cast<std::uint32_t>(danger);
					}

					if (oldCost >= budget - cost)
						continue;

					const auto nextCost = remaining - cost;
					const auto reverse = 0x10000u << ((direction + 4u) % 8u);

					if (nextCost == oldCost)
					{
						l_Sectors_CalculationMap_SetValue(next, value | reverse);
						continue;
					}

					auto& target = sMapPathfinderStaticVariables[(current + cost) & 15u];

					if (target.count == 4000u)
						continue;

					target.points[target.count++] = PackSector(next);
					++pending;
					l_Sectors_CalculationMap_SetValue(next, nextCost | reverse);

					if (next.x == source.x && next.y == source.y)
						return true;
				}
			}

			queue.count = 0;

			if (pending == 0)
				break;

			current = (current + 1u) & 15u;
			budget = remaining;
		} while (budget > 17u);

		return (Sectors_CalculationMap_GetValue(source) & NXBasics::IntegerRange::UInt16Maximum) != 0;
	}

	// Original function: FUN_00450A4D; select the last low-danger interior base point on the native route.
	bool __stdcall CMapWalkSectorManager::AI_Military_GetFormPoint(std::int32_t player, const void* source, const void* destination, void* output)
	{
		SBasePointSearch data{ };
		data.land = 1;

		if (!CMapPathfinderElement::SearchOverMap(source, 50, true, slWalkSectorBasePointSearch, &data, true, player))
			return false;

		const SSectorGridCursor from(data.point);
		auto* sourceSector = GetLandSectorDataPtr(from);

		if (sourceSector->calculated == 0 || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId) != sourceSector->continentId)
			return false;

		if (!CMapPathfinderElement::SearchOverMap(destination, 50, true, slWalkSectorBasePointSearch, &data, true, player))
			return false;

		if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&data.point), NC2Layout::MapMicroPointOffset::ContinentId) != sourceSector->continentId)
			return false;

		const SSectorGridCursor target(data.point);

		if ((target.x == from.x && target.y == from.y) || GetLandSectorDataPtr(target)->continentId != sourceSector->continentId)
			return false;

		if (!WayPossible(from, target, 0, true, player))
			return false;

		auto cursor = from;
		SMapWalkSectorData* selected = nullptr;

		for (;;)
		{
			const auto value = Sectors_CalculationMap_GetValue(cursor);

			if ((value & NXBasics::BitMask::Bits16Through23) == 0)
				break;

			std::uint32_t directions[8], count = 0;

			for (std::uint32_t direction = 0; direction < 8; ++direction)
			{
				if ((value & (0x10000u << direction)) != 0)
					directions[count++] = direction;
			}

			cursor.MoveCursorUnwrapped(directions[Game_Random_GetNumber() % count]);

			if (!cursor.IsInsideMap())
				continue;

			auto* sector = GetLandSectorDataPtr(cursor);

			if (AIMilitaryManager_Military_GetDangerValueForPosition(sector->basePoints, player) >= 10)
				break;

			selected = sector;
		}

		if (selected == nullptr)
			return false;

		Write(output, NC2Layout::MapCoordinatePairOffset::Coordinates, selected->basePoints[0]);

		return true;
	}

	// Original function: FUN_00454192; four square-sector rings, with signed division by radius squared.
	std::int32_t AIMilitaryManager_Military_GetDangerValueForPosition(const void* point, std::int32_t player)
	{
		SSectorGridCursor cursor(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		auto total = static_cast<std::uint32_t>(AIMilitaryManager_Military_GetDangerValueForSector(cursor, player));

		for (std::uint32_t radius = 1; radius < 5; ++radius)
		{
			cursor.MoveCursorUnwrapped(5);

			for (std::uint32_t direction = 0; direction < 8; direction += 2)
			{
				for (std::uint32_t step = 0; step < radius * 2u; ++step)
				{
					cursor.MoveCursorUnwrapped(direction);

					if (cursor.IsInsideMap())
						total += static_cast<std::uint32_t>(AIMilitaryManager_Military_GetDangerValueForSector(cursor, player) / static_cast<std::int32_t>(radius * radius));
				}
			}
		}

		return NXBasics::Runtime_TimeSigned32(total);
	}

	// Original function: FUN_00454218; score hostile/neutral caches and traverse their native human/vehicle lists.
	std::int32_t AIMilitaryManager_Military_GetDangerValueForSector(const SSectorGridCursor& cursor, std::int32_t player)
	{
		const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
		const auto index = static_cast<std::uint32_t>(manager->GetGridWidth()) * static_cast<std::uint32_t>(cursor.y) + static_cast<std::uint32_t>(cursor.x);
		const auto* sector = manager->GetSectorData() + static_cast<std::size_t>(index) * 0x540u;
		std::uint32_t total = 0;

		for (std::int32_t other = 0; other < 21; ++other)
		{
			if (other == 20)
			{
				total += Read<std::uint32_t>(sector, NC2Layout::SectorAttachmentOffset::NeutralAggressiveAnimalCount) * 10u;
				continue;
			}

			if (Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(other) * NC2Layout::RecordSize::GameControlPlayer) == 0)
				continue;

			const auto state = GameControl_Diplomacy_GetState(gGameControl, player, other);
			const auto* data = sector + static_cast<std::uint32_t>(other) * 0x40u;

			if (state == 3)
			{
				total += (Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianHouseCount) + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHouseCount) * 8u) * 5u + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianCount);

				for (auto id = Read<std::int32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHumanHead); id != -1;)
				{
					const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
					total += 10;
					const auto armor = Human_Equipment_GetArmorType(human);
					const auto weapon = Human_Equipment_GetWeaponType(human);

					if (armor != 0)
						total += 10;

					if (weapon != 1)
						total += 10;

					if (weapon == 8)
						total += 10;

					id = Read<std::int32_t>(human, NC2Layout::HumanOffset::NextSectorHumanId);
				}

				for (auto id = Read<std::int32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryVehicleHead); id != -1;)
				{
					const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
					total += Vehicle_Passengers_IsCommanderInVehicle(vehicle) ? 50u : 10u;
					id = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::NextSectorVehicleId);
				}
			}
			else if (state == 2)
			{
				total += (Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryVehicleCount) * 5u + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHouseCount) * 4u + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::AggressiveAnimalCount) + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::MilitaryHumanCount)) * 5u + Read<std::uint32_t>(data, NC2Layout::SectorPlayerOffset::CivilianHouseCount) * 3u;
			}
		}

		return NXBasics::Runtime_TimeSigned32(total);
	}

	// Original function: FUN_00454367; distinct fixed-radius overload.
	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForPosition(const void* point, std::int32_t player)
	{

		return AIMilitaryManager_Military_GetCivilianDangerValueForPosition(point, player, 5);
	}
}