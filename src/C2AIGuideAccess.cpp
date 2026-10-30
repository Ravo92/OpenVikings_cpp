#include "C2AIGuideAccess.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIObjectAccess.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2GameMapAccess.h"
#include "C2GuideAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Complete Windows static region 005125A0..005125F3: ten link IDs and points.
		struct SGuideCandidates
		{
			std::uint32_t count;
			std::uint32_t links[NC2Runtime::AIGuideLimits::CandidateLinkSystems];
			std::uint32_t points[NC2Runtime::AIGuideLimits::CandidateLinkSystems];
		} sGuideCandidates{ };

		static_assert(sizeof(SGuideCandidates) == 84);
		std::uint8_t sGuideCandidateInitFlags = 0; // 005125F4.

		// Original function: FUN_00429e61.
		void AIGuideCandidateList_Construct(SGuideCandidates* candidates)
		{
			for (std::uint32_t i = 0; i < NC2Runtime::AIGuideLimits::CandidateLinkSystems; ++i)
			{
				Write<std::uint16_t>(&candidates->points[i], NC2Layout::MapMigPointOffset::MapY, 0);
				Write<std::uint16_t>(&candidates->points[i], NC2Layout::MapMigPointOffset::MapX, 0);
			}
		}

		// Original function: FUN_00429e79; Windows registers this empty exit callback.
		int __cdecl AIGuideCandidateList_OnExit()
		{

			return 0;
		}

		struct SGuideBuildSearch
		{
			const void* source;
			const void* destination;
			std::uint32_t result;
			std::uint32_t distance;
			std::uint32_t player;
			std::int32_t danger;
		};

		static_assert(sizeof(SGuideBuildSearch) == (sizeof(void*) == 8 ? 32 : 24));
		static_assert(offsetof(SGuideBuildSearch, source) == 0);
		static_assert(offsetof(SGuideBuildSearch, destination) == sizeof(void*));
		static_assert(offsetof(SGuideBuildSearch, result) == 2 * sizeof(void*));
		static_assert(offsetof(SGuideBuildSearch, danger) == 2 * sizeof(void*) + 12);

		std::uint32_t Distance(const void* first, const void* second)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(first, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(first, NC2Layout::MapMigPointOffset::MapY), Read<std::uint16_t>(second, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(second, NC2Layout::MapMigPointOffset::MapY)));
		}

		// Original function: FUN_00429d9c; player and danger belong to the caller.
		void* AIGuideBuildSearch_Construct(SGuideBuildSearch* search, const void* source, const void* destination)
		{
			search->destination = destination;
			search->source = source;
			Write<std::uint16_t>(&search->result, NC2Layout::MapMigPointOffset::MapY, 0);
			Write<std::uint16_t>(&search->result, NC2Layout::MapMigPointOffset::MapX, 0);
			search->distance = Distance(source, destination);

			return search;
		}

		// Original function: FUN_00429ddb.
		bool __cdecl slAIHighLevelSearchGuideBuildPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
		{
			auto* search = static_cast<SGuideBuildSearch*>(data->userData);

			if (data->distance < 16)
				return false;

			const auto distance = Distance(point, search->destination);
			const auto terrain = (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingFlags) >> 3) & 15;
			const auto score = distance + terrain * terrain;

			if (score > search->distance || !Tool_Guide_IsAllowedToBeBuild(search->player, point, true))
				return false;

			const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(point, search->player);

			if (danger >= search->danger)
				return false;

			search->result = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
			search->distance = score;
			search->danger = danger;

			return false;
		}

		// Original function: FUN_00429ef8.
		bool __cdecl slAIGuideManagerSearchGuideBuildPosNearPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data)
		{
			auto* search = static_cast<SGuideBuildSearch*>(data->userData);

			if (!Tool_Guide_IsAllowedToBeBuild(search->player, point, true))
				return false;

			const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(point, search->player);

			if (danger >= search->danger)
				return false;

			search->result = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
			search->danger = danger;

			return false;
		}

		void* GridEntry(const void* manager, const void* position)
		{
			const SSectorGridCursor sector(Read<std::uint32_t>(position, NC2Layout::MapMigPointOffset::Coordinates));

			return static_cast<std::byte*>(Read<void*>(manager, NC2Layout::AIGuideManagerOffset::GridCells)) + static_cast<std::uint32_t>(sector.y * SSectorGridCursor::sGridWidth + sector.x) * NC2Layout::RecordSize::AIGuideGridCell;
		}
	}

	// Original function: FUN_00429e7a.
	bool AIGuideManager_IsGuideAlreadyBuild(const void*, std::uint32_t player, const void* point)
	{
		CGuideIterator guides;

		return guides.GetFirstGuideId(player, point, 20) != -1;
	}

	// Original function: FUN_00429ea5.
	void AIGuideManager_BuildGuideNearPosition(void* manager, std::uint32_t player, const void* point)
	{
		SGuideBuildSearch search;
		AIGuideBuildSearch_Construct(&search, point, point);
		search.player = player;
		search.danger = 10000;
		CMapPathfinderElement::SearchOverMap(point, 40, true, slAIGuideManagerSearchGuideBuildPosNearPos, &search, true, player);

		if (search.danger != 10000)
			Write(manager, NC2Layout::AIGuideManagerOffset::Destination, search.result);
	}

	// Original function: FUN_00429613.
	void AIGuideManager_CheckAndBuildGuides(void* manager, std::uint32_t player, void* military)
	{
		AIGuideManager_CalcVars_CreateGuideCalcArray(static_cast<std::byte*>(manager) + NC2Layout::AIGuideManagerOffset::CalculationGrid);

		if (Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::CalculationGrid) == 0 || Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::GridHeight) == 0)
			return;

		auto* destination = static_cast<std::byte*>(manager) + NC2Layout::AIGuideManagerOffset::Destination;

		if (Read<std::uint16_t>(destination, NC2Layout::MapMigPointOffset::MapX) != NXBasics::IntegerRange::UInt16MaximumSigned && !Tool_Guide_IsAllowedToBeBuild(player, destination, true))
			Write<std::uint16_t>(destination, NC2Layout::MapMigPointOffset::MapX, NXBasics::IntegerRange::UInt16MaximumSigned);

		if (Read<std::uint16_t>(destination, NC2Layout::MapMigPointOffset::MapX) == NXBasics::IntegerRange::UInt16MaximumSigned)
		{
			NXBasics::Runtime_FillMemory(Read<void*>(manager, NC2Layout::AIGuideManagerOffset::GridCells), 0, static_cast<std::size_t>(Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::CalculationGrid)) * Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::GridHeight) * NC2Layout::RecordSize::AIGuideGridCell);
			CGuideIterator guides;

			for (auto id = guides.GetFirstGuideId(player); id != -1; id = guides.GetNextGuideId())
			{
				const auto* guide = CGuideArray::mStaticVars + static_cast<std::uint32_t>(id) * CGuideArray::RuntimeElementSize;
				auto* entry = GridEntry(manager, guide + NC2Layout::GuideOffset::MapPosition);

				if (Read<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasGuideFlag) == 0)
				{
					Write<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasGuideFlag, 1);
					Write(entry, NC2Layout::AIGuideGridCellOffset::GuideId, Read<std::uint32_t>(guide, NC2Layout::GuideOffset::GuideId));
				}
			}

			CHouseIterator houses;

			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
					continue;

				auto* entry = GridEntry(manager, house + NC2Layout::HouseOffset::MapPosition);

				if (Read<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasHouseFlag) == 0)
				{
					Write<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasHouseFlag, 1);
					Write(entry, NC2Layout::AIGuideGridCellOffset::HouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
				}
			}

			AIGuideManager_l_CheckForNewGuideBuildPos(manager, player);
		}

		if (Read<std::uint16_t>(destination, NC2Layout::MapMigPointOffset::MapX) == NXBasics::IntegerRange::UInt16MaximumSigned)
		{
			if (Read<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex) != -1)
			{
				AIMilitaryManager_MilitaryPoint_Clear(military, Read<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex));
				Write<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex, -1);
			}

			AIScout_Clear(manager);
			return;
		}

		if (!AIScout_IsValid(manager))
		{
			if (Read<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex) != -1)
			{
				AIMilitaryManager_MilitaryPoint_Clear(military, Read<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex));
				Write<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex, -1);
			}

			Write<std::uint8_t>(manager, NC2Layout::AIGuideManagerOffset::ScoutRequestedFlag, 1);
			return;
		}

		auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::ScoutHumanId) * CHumanArray::RuntimeElementSize;

		if (Read<std::int32_t>(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex) == -1)
		{
			std::uint32_t target[3]{ 1, Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::ScoutHumanId), Read<std::uint32_t>(manager, NC2Layout::AIGuideManagerOffset::ScoutHumanRevision) };
			Write(manager, NC2Layout::AIGuideManagerOffset::MilitaryPointIndex, AIMilitaryManager_MilitaryPoint_Add(military, 1, 0, target, 40, 1, 1));
		}

		if (AIMilitaryManager_Military_GetCivilianDangerValueForPosition(Human_GetMapPositionRef(human), player) > 10)
		{
			AIScout_MoveToPos(manager, Human_GetMapPositionRef(human));
			return;
		}

		if (!AIScout_CheckAndChangeJob(manager))
			return;

		if (AIMilitaryManager_Military_GetCivilianDangerValueForPosition(destination, player) > 20)
		{
			std::byte target[12]{ };
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 5);
			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, Read<std::uint32_t>(destination, NC2Layout::MapMigPointOffset::Coordinates));
			Write<std::uint16_t>(target, NC2Layout::TargetDataOffset::ObjectRevisionHighWord, 0);
			AIMilitaryManager_MilitaryPoint_Add(military, 2, 0, target, 60, 4, 4);

			return;
		}

		const auto owner = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		Human_DoAddNewUserCommand(human, 19, static_cast<std::uint8_t>(owner), 0, 0, MapMigPoint_GetLong(destination), 0);
	}

	// Original function: FUN_004298ae.
	void AIGuideManager_l_CheckForNewGuideBuildPos(void* manager, std::uint32_t player)
	{
		if ((sGuideCandidateInitFlags & 1) == 0)
		{
			sGuideCandidateInitFlags |= 1;
			AIGuideCandidateList_Construct(&sGuideCandidates);
			NXBasics::Runtime_AtExit(AIGuideCandidateList_OnExit);
		}

		sGuideCandidates.count = 0;
		Write<std::uint8_t>(manager, NC2Layout::AIGuideManagerOffset::GuidePairSearchExhaustedFlag, 0);

		for (std::int32_t y = 0; y < SSectorGridCursor::sGridHeight; ++y)
		{
			for (std::int32_t x = 0; x < SSectorGridCursor::sGridWidth; ++x)
			{
				const auto* entry = static_cast<const std::byte*>(Read<void*>(manager, NC2Layout::AIGuideManagerOffset::GridCells)) + (y * SSectorGridCursor::sGridWidth + x) * NC2Layout::RecordSize::AIGuideGridCell;

				if (Read<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasGuideFlag) == 0)
					continue;

				const auto* guide = CGuideArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIGuideGridCellOffset::GuideId) * CGuideArray::RuntimeElementSize;
				std::uint32_t i = 0;

				for (; i < sGuideCandidates.count; ++i)
				{
					if (sGuideCandidates.links[i] == Read<std::uint32_t>(guide, NC2Layout::GuideOffset::LinkSystemId))
						break;
				}

				if (i != sGuideCandidates.count || sGuideCandidates.count >= NC2Runtime::AIGuideLimits::CandidateLinkSystems)
					continue;

				sGuideCandidates.links[i] = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::LinkSystemId);
				sGuideCandidates.points[i] = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
				++sGuideCandidates.count;
			}
		}

		auto* pathManager = static_cast<CArrayHandler*>(gMapPathfinderManager);
		const auto pathId = MapPathfinderManager_AllocatePathfinder(pathManager);

		if (pathId == -1)
			return;

		for (std::uint32_t i = 0; i < sGuideCandidates.count; ++i)
		{
			for (std::uint32_t j = i + 1; j < sGuideCandidates.count; ++j)
			{
				auto* first = &sGuideCandidates.points[i];
				auto* second = &sGuideCandidates.points[j];

				if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(first), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(second), NC2Layout::MapMicroPointOffset::ContinentId))
					continue;

				auto distance = Distance(first, second);
				CGuideIterator guides;

				for (auto id = guides.GetFirstGuideId(player); id != -1; id = guides.GetNextGuideId())
				{
					const auto* guide = CGuideArray::mStaticVars + static_cast<std::uint32_t>(id) * CGuideArray::RuntimeElementSize;
					const auto link = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::LinkSystemId);

					if (link == sGuideCandidates.links[i])
					{
						const auto candidate = Distance(guide + NC2Layout::GuideOffset::MapPosition, second);

						if (candidate <= distance)
						{
							*first = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
							distance = candidate;
						}
					}
					else if (link == sGuideCandidates.links[j])
					{
						const auto candidate = Distance(guide + NC2Layout::GuideOffset::MapPosition, first);

						if (candidate <= distance)
						{
							*second = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
							distance = candidate;
						}
					}
				}

				auto* path = GetUnsafePathfinderPtr(pathId);
				MapPathfinderElement_StartPathfinder(path, first, second, 40, true, true, false, false);
				auto point = *first;

				for (auto direction = MapPathfinderElement_GetNextDirection(path); direction >= 0; direction = MapPathfinderElement_GetNextDirection(path))
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (Tool_Guide_IsAllowedToBeBuild(player, &point, true))
					{
						Write(manager, NC2Layout::AIGuideManagerOffset::Destination, point);
						MapPathfinderManager_FreePathfinder(pathManager, pathId);
						return;
					}
				}

				SGuideBuildSearch search;
				AIGuideBuildSearch_Construct(&search, first, second);
				search.player = player;
				search.danger = 10000;
				CMapPathfinderElement::SearchOverMap(first, 40, true, slAIHighLevelSearchGuideBuildPos, &search, true, player);

				if (search.danger != 10000)
				{
					Write(manager, NC2Layout::AIGuideManagerOffset::Destination, search.result);
					MapPathfinderManager_FreePathfinder(pathManager, pathId);
					return;
				}

				MapPathfinderElement_StartPathfinder(path, first, second, 40, true, true, false, false);
				point = *first;
				Logic_EntryPoint_Edit_Start();

				for (auto direction = MapPathfinderElement_GetNextDirection(path); direction >= 0; direction = MapPathfinderElement_GetNextDirection(path))
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					Logic_EntryPoint_Edit_MapMicroPoint_Road_Set(&point, true);
				}

				Logic_EntryPoint_Edit_Stop();
			}
		}

		MapPathfinderManager_FreePathfinder(pathManager, pathId);
		Write<std::uint8_t>(manager, NC2Layout::AIGuideManagerOffset::GuidePairSearchExhaustedFlag, 1);

		for (std::int32_t row = 0; row < SSectorGridCursor::sGridHeight; ++row)
		{
			for (std::int32_t column = 0; column < SSectorGridCursor::sGridWidth; ++column)
			{
				const auto* entry = static_cast<const std::byte*>(Read<void*>(manager, NC2Layout::AIGuideManagerOffset::GridCells)) + (row * SSectorGridCursor::sGridWidth + column) * NC2Layout::RecordSize::AIGuideGridCell;

				if (Read<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasHouseFlag) == 0 || Read<std::uint8_t>(entry, NC2Layout::AIGuideGridCellOffset::HasGuideFlag) != 0)
					continue;

				const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIGuideGridCellOffset::HouseId) * CHouseArray::RuntimeElementSize;
				const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(house + NC2Layout::HouseOffset::MapPosition), NC2Layout::MapMicroPointOffset::ContinentId);
				const auto x = column * 20, y = row * 20;
				std::uint32_t point = 0;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(x + 10), static_cast<std::int16_t>(y + 10), false);

				if (MapMigPoint_IsInsideMap(&point) && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId) == continent && Tool_Guide_IsAllowedToBeBuild(player, &point, true))
				{
					Write(manager, NC2Layout::AIGuideManagerOffset::Destination, point);
					return;
				}

				for (std::uint32_t radius = 1; radius < 20; ++radius)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 4);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						for (std::uint32_t step = 0; step < radius; ++step)
						{
							MapMigPoint_MoveCursorUnwrapped(point, direction);

							if (!MapMigPoint_IsInsideMap(&point))
								continue;

							const auto px = Read<std::int16_t>(&point, NC2Layout::MapMigPointOffset::MapX), py = Read<std::int16_t>(&point, NC2Layout::MapMigPointOffset::MapY);

							if (px < x || px > x + 20 || py < y || py > y + 20)
								continue;

							if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId) == continent && Tool_Guide_IsAllowedToBeBuild(player, &point, true))
							{
								Write(manager, NC2Layout::AIGuideManagerOffset::Destination, point);

								return;
							}
						}
					}
				}
			}
		}
	}
}