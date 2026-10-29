#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2MilitaryTargetSearch.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Windows layout at 0043AB64/0043AD8B; original template/type names remain unresolved.
		struct LandscapeTargetList
		{
			std::uint32_t count;
			std::uint32_t capacity;
			void* entries;
		};

		static_assert(sizeof(LandscapeTargetList) == (sizeof(void*) == 8 ? 16 : 12));

		// Original function: FUN_0043ab64.
		LandscapeTargetList* LandscapeTargetList_Construct(LandscapeTargetList* list, std::uint32_t capacity)
		{
			void* entries = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::size_t>(capacity) * NC2Layout::RecordSize::EnemyRangeResultUnsigned, 1);

			if (entries != nullptr && NXBasics::Runtime_TimeSigned32(capacity - 1u) >= 0)
			{
				for (std::uint32_t index = 0; index < capacity; ++index)
				{
					Write<std::uint32_t>(entries, index * NC2Layout::RecordSize::EnemyRangeResultUnsigned, 0);
				}
			}

			list->count = 0;
			list->capacity = capacity;
			list->entries = entries;

			return list;
		}

		// Original function: FUN_0043ad8b; equal-distance entries retain their discovery order.
		bool LandscapeTargetList_Add(LandscapeTargetList* list, const std::uint32_t* entry)
		{
			if (list->count == 0)
			{
				Write(list->entries, NC2Layout::EnemyLandscapeResultOffset::MapPosition, entry[NC2Runtime::EnemyRangeResultComponent::MapPosition]);
				Write(list->entries, NC2Layout::EnemyLandscapeResultOffset::Distance, entry[NC2Runtime::EnemyRangeResultComponent::Distance]);
				++list->count;
				return true;
			}

			std::uint32_t index = 0;

			while (index < list->count && entry[NC2Runtime::EnemyRangeResultComponent::Distance] >= Read<std::uint32_t>(list->entries, index * NC2Layout::RecordSize::EnemyRangeResultUnsigned + NC2Layout::EnemyRangeResultOffset::DistanceUnsigned))
				++index;

			auto shift = list->count;

			if (list->count == list->capacity)
			{
				if (index == list->count)
					return false;

				--shift;
			}
			else
			{
				++list->count;
			}

			while (shift > index)
			{
				Write(list->entries, shift * NC2Layout::RecordSize::EnemyRangeResultUnsigned, Read<std::uint32_t>(list->entries, (shift - 1u) * NC2Layout::RecordSize::EnemyRangeResultUnsigned));
				Write(list->entries, shift * NC2Layout::RecordSize::EnemyRangeResultUnsigned + NC2Layout::EnemyRangeResultOffset::DistanceUnsigned, Read<std::uint32_t>(list->entries, (shift - 1u) * NC2Layout::RecordSize::EnemyRangeResultUnsigned + NC2Layout::EnemyRangeResultOffset::DistanceUnsigned));
				--shift;
			}

			Write(list->entries, index * NC2Layout::RecordSize::EnemyRangeResultUnsigned, entry[NC2Runtime::EnemyRangeResultComponent::MapPosition]);
			Write(list->entries, index * NC2Layout::RecordSize::EnemyRangeResultUnsigned + NC2Layout::EnemyRangeResultOffset::DistanceUnsigned, entry[NC2Runtime::EnemyRangeResultComponent::Distance]);

			return true;
		}
	}

	// Original function: FUN_0043aae7; unlike object-choice wrappers, eligibility starts at zero.
	bool Tool_GetOneOfNearestEnemyLandscapes(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, bool sameContinent, std::uint32_t limit, std::uint32_t distanceThreshold, void* output)
	{
		LandscapeTargetList list;
		LandscapeTargetList_Construct(&list, limit);
		const bool result = Tool_GetNextNearestEnemyLandscapeInRange(player, point, minimumDistance, maximumDistance, sameContinent, &list);

		if (result)
		{
			const auto cutoff = Read<std::uint32_t>(list.entries, NC2Layout::EnemyLandscapeResultOffset::Distance) + distanceThreshold;
			std::uint32_t eligible = 0;

			while (eligible < list.count && Read<std::uint32_t>(list.entries, eligible * NC2Layout::RecordSize::EnemyRangeResultUnsigned + NC2Layout::EnemyRangeResultOffset::DistanceUnsigned) <= cutoff)
				++eligible;

			Write(output, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(list.entries, (Game_Random_GetNumber() % eligible) * NC2Layout::RecordSize::EnemyRangeResultUnsigned));
		}

		NXBasics::Runtime_Free(list.entries);

		return result;
	}

	// Original function: FUN_0043aba9.
	bool Tool_GetNextNearestEnemyLandscapeInRange(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, bool sameContinent, void* list)
	{
		std::uint32_t enemyCount = 0;

		for (std::uint32_t enemy = 0; enemy < NC2Runtime::EnemyRangeLimits::Players; ++enemy)
		{
			if (Read<std::uint8_t>(gGameControl, enemy * NC2Layout::RecordSize::GameControlPlayer) != 0 && GameControl_Diplomacy_GetState(gGameControl, player, enemy) == SwitchIds::DiplomacyState::Enemy)
				++enemyCount;
		}

		if (enemyCount == 0)
			return false;

		if (minimumDistance == 0 && !Tool_FillArrayFromPoint(NXBasics::Runtime_TimeSigned32(player), point, point, list))
			return false;

		auto cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

		if (minimumDistance != 0)
		{
			MapMigPoint_MoveCursorUnwrapped(&cursor, 4, minimumDistance - 1u);
		}
		else
		{
			++minimumDistance;
		}

		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);

		for (auto ring = minimumDistance; ring < maximumDistance + 1u; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(cursor, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				// 0043AC66..0043ACB8 uses maximumDistance steps on every ring, not the ring radius.

				for (std::uint32_t step = 0; step < maximumDistance; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(cursor, direction);

					if (!MapMigPoint_IsInsideMap(&cursor))
						continue;

					if (sameContinent && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&cursor), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
						continue;

					if (!Tool_FillArrayFromPoint(NXBasics::Runtime_TimeSigned32(player), point, &cursor, list))
						return Read<std::uint32_t>(list, NC2Layout::EnemyLandscapeTargetListOffset::Count) != 0;
				}
			}
		}

		return Read<std::uint32_t>(list, NC2Layout::EnemyLandscapeTargetListOffset::Count) != 0;
	}

	// Original function: FUN_0043ace1.
	bool Tool_IsEnemyLandscapeOnPoint(std::int32_t player, const void* point)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0)
			return false;

		const auto owner = Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId);

		if (owner == -1)
			return false;

		return player == -1 || GameControl_Diplomacy_GetState(gGameControl, static_cast<std::uint32_t>(player), static_cast<std::uint32_t>(owner)) == SwitchIds::DiplomacyState::Enemy;
	}

	// Original function: FUN_0043ad26; a full list stops traversal before the point predicate.
	bool Tool_FillArrayFromPoint(std::int32_t player, const void* source, const void* point, void* list)
	{
		auto* targets = static_cast<LandscapeTargetList*>(list);

		if (targets->count == targets->capacity)
			return false;

		if (!Tool_IsEnemyLandscapeOnPoint(player, point))
			return true;

		const std::uint32_t entry[2] = { Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y))) };

		return LandscapeTargetList_Add(targets, entry);
	}
}