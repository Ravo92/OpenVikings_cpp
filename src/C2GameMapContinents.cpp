#include "C2GameMapStorage.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		struct SGameMapContinentCreation
		{
			std::uint32_t point;
			std::uint32_t count;
			std::uint32_t oldIds[10];
			std::uint32_t oldIdCount;
			std::uint32_t type;
		};

		struct SContinentChangeWork
		{
			std::uint32_t ids[250];
			std::uint32_t idCount;
			std::uint32_t samePoints[250];
			std::uint32_t differentPoints[250];
			std::uint32_t changedCounts[250];
			std::uint32_t creationCount;
			SGameMapContinentCreation creations[150];
			SContinentChangeWork();
		};

		static_assert(sizeof(SGameMapContinentCreation) == 56);
		static_assert(sizeof(SContinentChangeWork) == 12408);
		static_assert(offsetof(SContinentChangeWork, samePoints) == 0x3EC);
		static_assert(offsetof(SContinentChangeWork, creations) == 0xFA8);

		std::uint32_t ContinentCount()
		{

			return Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::ContinentCount);
		}

		void SetContinentCount(std::uint32_t value)
		{
			Write(gGameMapStaticStorage, NC2Layout::GameMapOffset::ContinentCount, value);
		}

		std::byte* Continent(std::uint32_t id)
		{

			return gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentsUnsigned + id * NC2Layout::RecordSize::MapContinentUnsigned;
		}

		std::uint32_t PointContinent(const void* point)
		{

			return Read<std::uint8_t>(GameMap_GetMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);
		}

		void SetPointContinent(const void* point, std::uint32_t id)
		{
			Write<std::uint8_t>(GameMap_GetMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId, static_cast<std::uint8_t>(id));
		}

		bool InChangedArea(const void* point, const void* first, const void* end)
		{

			return Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X) >= Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X) && Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X) < Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::X) && Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y) >= Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y) && Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y) < Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y);
		}

		// Original function: FUN_004521F3; only embedded point constructors run here.
		SContinentChangeWork::SContinentChangeWork()
		{
			for (auto& point : samePoints)
			{
				MapMigPoint_ConstructZero(&point);
			}

			for (auto& point : differentPoints)
			{
				MapMigPoint_ConstructZero(&point);
			}

			for (auto& creation : creations)
			{
				MapMigPoint_ConstructZero(&creation.point);
			}
		}
	}

	// Original function: FUN_00451503; separate native terrain classification entry.
	std::uint32_t GameMap_Continent_CalculateContinentType(const void* point)
	{
		const void* patterns[6];

		if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(point, patterns))
			return 0;

		for (const auto* pattern : patterns)
		{
			if (Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsLand) != 0)
				return 1;
		}

		for (const auto* pattern : patterns)
		{
			if (Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsWater) == 0)
				return 0;
		}

		return 2;
	}

	// Original function: FUN_004523A7; the second native argument is unused.
	static void GameMap_l_Continents_SubChanged(SContinentChangeWork* work, std::uint32_t)
	{
		for (std::uint32_t index = 0; index < work->idCount; ++index)
		{
			const auto id = work->ids[index];

			if (work->changedCounts[id] != 0)
				Write(Continent(id), NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(Continent(id), NC2Layout::MapContinentOffset::PointCount) - work->changedCounts[id]);
		}
	}

	// Original function: FUN_004523E4; queue capacity is checked before marking a neighbor.
	static std::uint32_t GameMap_l_Continents_SearchFillArea(const void* source, std::uint32_t* changedCounts, const void* first, const void* end, std::uint32_t id, std::uint32_t type)
	{
		auto& queues = sMapPathfinderStaticVariables;
		queues[0].count = 0;
		queues[1].count = 0;
		queues[0].count = 1;
		queues[0].points[0] = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		++changedCounts[PointContinent(source)];
		SetPointContinent(source, id);
		std::uint32_t count = 1, current = 0;
		do
		{
			auto& from = queues[current];
			auto& to = queues[(current - 1u) & 1u];
			to.count = 0;

			for (std::uint32_t index = 0; index < from.count; ++index)
			{
				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto point = from.points[index];
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (!InChangedArea(&point, first, end))
						continue;

					const auto oldId = PointContinent(&point);

					if (oldId == id || GameMap_Continent_CalculateContinentType(&point) != type)
						continue;

					if (to.count == 4000)
						return 0;

					to.points[to.count++] = point;
					++changedCounts[oldId];
					++count;
					SetPointContinent(&point, id);
				}
			}

			current = (current + 1u) & 1u;
		} while (queues[current].count != 0);

		return count;
	}

	// Original function: FUN_0045253D; marked old-continent counters are reduced before the queue capacity check.
	static std::uint32_t GameMap_l_Continents_FillChangedArea(const void* source, const void* first, const void* end, std::uint32_t id, const std::uint32_t* changedIds, std::uint32_t type)
	{
		auto& queues = sMapPathfinderStaticVariables;
		queues[0].count = 0;
		queues[1].count = 0;
		std::uint32_t remaining = 0;

		for (std::uint32_t old = 0; old < 250; ++old)
		{
			if (changedIds[old] != 0 && Read<std::uint32_t>(Continent(old), NC2Layout::MapContinentOffset::PointCount) != 0)
				++remaining;
		}

		if (remaining == 0)
			return 0;

		queues[0].count = 1;
		queues[0].points[0] = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		const auto old = PointContinent(source);
		SetPointContinent(source, id);
		const auto reduce = [&](std::uint32_t oldId)
		{
			if (changedIds[oldId] == 0)
			{

				return;
			}

			const auto value = Read<std::uint32_t>(Continent(oldId), NC2Layout::MapContinentOffset::PointCount) - 1u;
			Write(Continent(oldId), NC2Layout::MapContinentOffset::PointCount, value);

			if (value == 0)
				--remaining;
		};
		reduce(old);
		std::uint32_t count = 1, current = 0;
		do
		{
			auto& from = queues[current];
			auto& to = queues[(current - 1u) & 1u];
			to.count = 0;

			for (std::uint32_t index = 0; index < from.count; ++index)
			{
				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto point = from.points[index];
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (!InChangedArea(&point, first, end))
						continue;

					const auto oldId = PointContinent(&point);

					if (oldId == id || GameMap_Continent_CalculateContinentType(&point) != type)
						continue;

					reduce(oldId);

					if (to.count == 4000)
						return 0;

					to.points[to.count++] = point;
					++count;
					SetPointContinent(&point, id);
				}
			}

			current = (current + 1u) & 1u;
		} while (queues[current].count != 0 && remaining != 0);

		return count;
	}

	// Original function: FUN_004528CF.
	static std::uint32_t GameMap_l_Continents_FillLand(std::uint32_t id, const void* source)
	{
		auto& queues = sMapPathfinderStaticVariables;
		std::uint32_t from = 0, to = 1, pending = 1, count = 1;
		queues[from].count = 1;
		queues[from].points[0] = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		SetPointContinent(source, id);
		do
		{
			queues[to].count = 0;

			for (std::uint32_t index = 0; index < queues[from].count; ++index)
			{
				--pending;

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto point = queues[from].points[index];
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (PointContinent(&point) != 255)
						continue;

					const void* patterns[2];

					if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundWay(&queues[from].points[index], direction, patterns))
						continue;

					if (Read<std::uint8_t>(patterns[0], NC2Layout::TrianglePatternTypeOffset::IsLand) == 0 && Read<std::uint8_t>(patterns[1], NC2Layout::TrianglePatternTypeOffset::IsLand) == 0)
						continue;

					queues[to].points[queues[to].count] = point;

					if (PointContinent(&point) == 255)
					{
						++count;
						SetPointContinent(&point, id);
					}

					++pending;

					if (++queues[to].count == 4000)
						return 0;
				}
			}

			if (pending == 0)
				break;

			const auto swap = from;
			from = to;
			to = swap;
		} while (true);

		return count;
	}

	// Original function: FUN_004529D7.
	static std::uint32_t GameMap_l_Continents_FillWater(std::uint32_t id, const void* source)
	{
		auto& queues = sMapPathfinderStaticVariables;
		std::uint32_t from = 0, to = 1, pending = 1, count = 1;
		queues[from].count = 1;
		queues[from].points[0] = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		SetPointContinent(source, id);
		do
		{
			queues[to].count = 0;

			for (std::uint32_t index = 0; index < queues[from].count; ++index)
			{
				--pending;

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					auto point = queues[from].points[index];
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (PointContinent(&point) != 255)
						continue;

					const void* patterns[6];

					if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(&point, patterns))
						continue;

					bool water = true;

					for (const auto* pattern : patterns)
					{
						if (Read<std::uint8_t>(pattern, NC2Layout::TrianglePatternTypeOffset::IsWater) == 0)
							water = false;
					}

					if (!water)
						continue;

					queues[to].points[queues[to].count] = point;

					if (PointContinent(&point) == 255)
					{
						++count;
						SetPointContinent(&point, id);
					}

					++pending;

					if (++queues[to].count == 4000)
						return 0;
				}
			}

			if (pending == 0)
				break;

			const auto swap = from;
			from = to;
			to = swap;
		} while (true);

		return count;
	}

	// Original function: FUN_004526F0.
	void GameMap_l_Continents_RecalculateAll()
	{
		for (std::int16_t y = 0; y < GameMap_GetHeight(); ++y)
		{
			for (std::int16_t x = 0; x < GameMap_GetWidth(); ++x)
			{
				std::int16_t point[2] = { x, y };
				SetPointContinent(point, 255);
			}
		}

		std::memset(Continent(0), 0, 250 * NC2Layout::RecordSize::MapContinent);
		SetContinentCount(1);
		std::uint32_t nextId = 1;

		for (std::int16_t y = 0; y < GameMap_GetHeight(); ++y)
		{
			for (std::int16_t x = 0; x < GameMap_GetWidth(); ++x)
			{
				std::int16_t point[2] = { x, y };

				if (PointContinent(point) != 255)
					continue;

				const auto type = GameMap_Continent_CalculateContinentType(point);

				if (type == 0)
				{
					SetPointContinent(point, 0);
					continue;
				}

				if (nextId == 250)
				{

					return;
				}

				auto* record = Continent(ContinentCount());
				Write(record, NC2Layout::MapContinentOffset::Type, type);
				Write(record, NC2Layout::MapContinentOffset::SeedPoint, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
				const auto count = type == 1 ? GameMap_l_Continents_FillLand(nextId, point) : GameMap_l_Continents_FillWater(nextId, point);
				record = Continent(ContinentCount());
				Write(record, NC2Layout::MapContinentOffset::PointCount, count);

				if (count < 20)
				{
					for (std::int16_t sy = 0; sy < GameMap_GetHeight(); ++sy)
					{
						for (std::int16_t sx = 0; sx < GameMap_GetWidth(); ++sx)
						{
							std::int16_t scan[2] = { sx, sy };

							if (PointContinent(scan) == nextId)
								SetPointContinent(scan, 0);
						}
					}
				}
				else
				{
					++nextId;
					Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 1);
					SetContinentCount(ContinentCount() + 1u);
				}
			}
		}
	}

	// Original function: FUN_00452241; remap outward in hexagonal rings, then fall back to a full recalculation.
	static void GameMap_l_Continents_FillUpFast(const std::uint32_t* replacements, const void* first, const void* end)
	{
		std::int16_t center[2];

		for (std::uint32_t axis = 0; axis < 2; ++axis)
		{
			center[axis] = static_cast<std::int16_t>((Read<std::int16_t>(first, axis * NC2Layout::RecordSize::MapCoordinateComponentUnsigned) + Read<std::int16_t>(end, axis * NC2Layout::RecordSize::MapCoordinateComponentUnsigned)) / 2);
		}

		std::uint32_t remaining = 0;

		for (std::uint32_t id = 0; id < 250; ++id)
		{
			if (replacements[id] != 0 && Read<std::uint32_t>(Continent(id), NC2Layout::MapContinentOffset::PointCount) != 0)
				++remaining;
		}

		const auto replace = [&](const void* point)
		{
			const auto id = PointContinent(point), target = replacements[id];

			if (target == 0)
			{

				return;
			}

			SetPointContinent(point, target == 255 ? 0u : target);
			const auto count = Read<std::uint32_t>(Continent(id), NC2Layout::MapContinentOffset::PointCount) - 1u;
			Write(Continent(id), NC2Layout::MapContinentOffset::PointCount, count);

			if (count == 0)
				--remaining;
		};

		if (remaining != 0)
			replace(center);

		const auto maximum = static_cast<std::uint32_t>((GameMap_GetHeight() < GameMap_GetWidth() ? GameMap_GetWidth() : GameMap_GetHeight()) * 2);

		for (std::uint32_t radius = 1; remaining != 0; ++radius)
		{
			if (radius > maximum)
			{
				GameMap_l_Continents_RecalculateAll();
				break;
			}

			auto point = Read<std::uint32_t>(center, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(&point, 4, radius);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < radius; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (MapMigPoint_IsInsideMap(&point))
						replace(&point);
				}
			}
		}
	}

	// Original function: FUN_00451559; retain the native split/merge phases and temporary ID accounting.
	void GameMap_l_Continents_UpdateArea(const void* first, const void* end)
	{
		SContinentChangeWork work;
		std::memset(&work, 0, sizeof(work));
		Write<std::uint32_t>(Continent(0), NC2Layout::MapContinentOffset::PointCount, 0);
		const auto temporaryBase = ContinentCount() + 50u;
		std::uint32_t replacements[250]{ };

		for (;;)
		{
			std::memset(work.ids, 0, sizeof(work.ids));
			work.idCount = 0;

			for (std::uint32_t id = 0; id < 250; ++id)
			{
				MapMigPoint_Construct(&work.samePoints[id], -1, -1, false);
				MapMigPoint_Construct(&work.differentPoints[id], -1, -1, false);
			}

			std::int16_t point[2] = { 0, Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::Y) };

			for (; point[1] < Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::Y); ++point[1])
			{
				for (point[0] = Read<std::int16_t>(first, NC2Layout::MapCoordinatePairOffset::X); point[0] < Read<std::int16_t>(end, NC2Layout::MapCoordinatePairOffset::X); ++point[0])
				{
					const auto oldId = PointContinent(point);

					if (oldId >= ContinentCount())
						continue;

					std::uint32_t slot = 0;

					while (slot < work.idCount && work.ids[slot] != oldId)
						++slot;

					if (slot == work.idCount)
						work.ids[work.idCount++] = oldId;

					const auto type = GameMap_Continent_CalculateContinentType(point);

					if (type == 0)
					{
						SetPointContinent(point, temporaryBase);
						++work.creations[0].count;
					}
					else if (GameMap_Continent_GetContinentTyp(oldId) == type)
					{
						work.samePoints[oldId] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
					}
					else
					{
						work.differentPoints[oldId] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
					}
				}
			}

			if (work.creations[0].count != 0 && work.creationCount == 0)
			{
				work.creationCount = 1;
				MapMigPoint_Construct(&work.creations[0].point, -1, -1, false);
				replacements[temporaryBase] = 255;
				Write(Continent(temporaryBase), NC2Layout::MapContinentOffset::PointCount, work.creations[0].count);
			}

			bool sameFound = false;

			for (std::uint32_t slot = 0; slot < work.idCount; ++slot)
			{
				if (MapMigPoint_IsInsideMap(&work.samePoints[work.ids[slot]]))
				{
					sameFound = true;
					break;
				}
			}

			if (sameFound)
			{
				for (std::uint32_t slot = 0; slot < work.idCount; ++slot)
				{
					std::memset(work.changedCounts, 0, sizeof(work.changedCounts));
					const auto oldId = work.ids[slot];
					auto& source = work.samePoints[oldId];

					if (!MapMigPoint_IsInsideMap(&source))
						continue;

					const auto now = PointContinent(&source);

					if (now != oldId)
					{
						auto& creation = work.creations[now - temporaryBase];
						creation.oldIds[creation.oldIdCount++] = oldId;
						Write<std::uint8_t>(Continent(oldId), NC2Layout::MapContinentOffset::RetainedFlag, 0);
						continue;
					}

					const auto type = GameMap_Continent_CalculateContinentType(&source);
					auto& creation = work.creations[work.creationCount];
					const auto count = GameMap_l_Continents_SearchFillArea(&source, work.changedCounts, first, end, temporaryBase + work.creationCount, type);
					creation.point = source;
					creation.oldIds[creation.oldIdCount++] = oldId;
					creation.count = count;
					creation.type = type;
					GameMap_l_Continents_SubChanged(&work, work.creationCount);
					++work.creationCount;
					Write<std::uint8_t>(Continent(oldId), NC2Layout::MapContinentOffset::RetainedFlag, 0);
				}

				continue;
			}

			bool differentFound = false;

			for (std::uint32_t slot = 0; slot < work.idCount; ++slot)
			{
				if (MapMigPoint_IsInsideMap(&work.differentPoints[work.ids[slot]]))
				{
					differentFound = true;
					break;
				}
			}

			if (!differentFound)
				break;

			for (std::uint32_t slot = 0; slot < work.idCount; ++slot)
			{
				std::memset(work.changedCounts, 0, sizeof(work.changedCounts));
				const auto oldId = work.ids[slot];
				auto& source = work.differentPoints[oldId];

				if (!MapMigPoint_IsInsideMap(&source) || PointContinent(&source) != oldId)
					continue;

				const auto type = GameMap_Continent_CalculateContinentType(&source);
				auto& creation = work.creations[work.creationCount];
				creation.count = GameMap_l_Continents_SearchFillArea(&source, work.changedCounts, first, end, temporaryBase + work.creationCount, type);
				creation.point = source;
				creation.oldIds[creation.oldIdCount++] = NXBasics::IntegerRange::UInt32Maximum;
				creation.type = type;
				GameMap_l_Continents_SubChanged(&work, work.creationCount);
				++work.creationCount;
			}
		}

		const auto finalizeUnowned = [&]()
		{
			for (std::uint32_t index = 0; index < work.creationCount; ++index)
			{
				auto& creation = work.creations[index];

				if (!MapMigPoint_IsInsideMap(&creation.point) || creation.oldIds[0] != NXBasics::IntegerRange::UInt32Maximum)
					continue;

				std::uint32_t target = 255;

				if (creation.type != 0)
				{
					auto* record = Continent(ContinentCount());
					Write(record, NC2Layout::MapContinentOffset::PointCount, creation.count);

					if (creation.count >= 20)
					{
						Write(record, NC2Layout::MapContinentOffset::SeedPoint, creation.point);
						Write(record, NC2Layout::MapContinentOffset::Type, creation.type);
						Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 1);
						target = ContinentCount();
						SetContinentCount(target + 1u);
					}
				}

				replacements[temporaryBase + index] = target;
				Write(Continent(temporaryBase + index), NC2Layout::MapContinentOffset::PointCount, creation.count);
				MapMigPoint_Construct(&creation.point, -1, -1, false);
			}
		};
		finalizeUnowned();
		const auto columns = ContinentCount(), rows = work.creationCount;
		const std::size_t cellCount = static_cast<std::size_t>(columns) * rows;
		const std::size_t bytes = cellCount * sizeof(std::uint32_t);
		auto* links = static_cast<std::uint32_t*>(NXBasics::Runtime_AllocateArrayWithNewHandler(cellCount, sizeof(std::uint32_t)));
		NXBasics::Runtime_FillMemory(links, 0, bytes);

		for (std::uint32_t row = 0; row < rows; ++row)
		{
			const auto& creation = work.creations[row];

			if (!MapMigPoint_IsInsideMap(&creation.point))
				continue;

			for (std::uint32_t old = 0; old < creation.oldIdCount; ++old)
			{
				links[static_cast<std::size_t>(row) * columns + creation.oldIds[old]] = 1;
			}
		}

		for (std::uint32_t row = 0; row < rows; ++row)
		{
			auto& creation = work.creations[row];
			std::uint32_t matches = 0, id = 0;

			for (std::uint32_t column = 0; column < columns; ++column)
			{
				if (links[static_cast<std::size_t>(row) * columns + column] != 0)
				{
					++matches;
					id = column;
				}
			}

			if (matches != 1)
				continue;

			matches = 0;

			for (std::uint32_t other = 0; other < rows; ++other)
			{
				if (links[static_cast<std::size_t>(other) * columns + id] != 0)
					++matches;
			}

			if (matches != 1)
				continue;

			auto target = id;

			if (creation.type != 0)
			{
				auto* record = Continent(creation.oldIds[0]);
				Write(record, NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) + creation.count);
				Write(record, NC2Layout::MapContinentOffset::SeedPoint, creation.point);
				Write(record, NC2Layout::MapContinentOffset::Type, creation.type);
				Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 1);

				if (Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) < 20)
				{
					Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 0);
					replacements[creation.oldIds[0]] = 255;
					target = 255;
					Write(record, NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) - creation.count);
				}
			}

			if (target == 0)
				target = 255;

			Write(Continent(temporaryBase + row), NC2Layout::MapContinentOffset::PointCount, creation.count);
			replacements[temporaryBase + row] = target;
			links[static_cast<std::size_t>(row) * columns + id] = 0;
		}

		for (std::uint32_t column = 0; column < columns; ++column)
		{
			std::uint32_t matchingRows[150], matchCount = 0;

			for (std::uint32_t row = 0; row < rows; ++row)
			{
				if (links[static_cast<std::size_t>(row) * columns + column] != 0)
					matchingRows[matchCount++] = row;
			}

			if (matchCount < 2)
				continue;

			std::uint32_t best = 0;

			for (std::uint32_t candidate = 1; candidate < matchCount; ++candidate)
			{
				if (work.creations[matchingRows[candidate]].count > work.creations[matchingRows[best]].count)
					best = candidate;
			}

			const auto selectedRow = matchingRows[best];
			auto* selected = &work.creations[selectedRow];
			links[static_cast<std::size_t>(selectedRow) * columns + column] = 0;
			std::uint32_t changedIds[250]{ };

			for (std::uint32_t index = 0; index < matchCount; ++index)
			{
				const auto row = matchingRows[index];
				changedIds[temporaryBase + row] = 1;
				Write(Continent(temporaryBase + row), NC2Layout::MapContinentOffset::PointCount, work.creations[row].count);
			}

			const std::int16_t mapFirst[2] = { 0, 0 }, mapEnd[2] = { GameMap_GetWidth(), GameMap_GetHeight() };
			const auto mergedCount = GameMap_l_Continents_FillChangedArea(&selected->point, mapFirst, mapEnd, temporaryBase + work.creationCount, changedIds, selected->type);
			std::uint32_t consumed = 0;

			for (std::uint32_t index = 0; index < matchCount; ++index)
			{
				const auto row = matchingRows[index];
				auto& creation = work.creations[row];

				if (!MapMigPoint_IsInsideMap(&creation.point) || PointContinent(&creation.point) != temporaryBase + work.creationCount)
					continue;

				MapMigPoint_Construct(&creation.point, -1, -1, false);
				++consumed;
				links[static_cast<std::size_t>(row) * columns + column] = 0;
			}

			Write(Continent(temporaryBase + work.creationCount), NC2Layout::MapContinentOffset::PointCount, mergedCount);
			replacements[temporaryBase + work.creationCount] = selected->oldIds[0] == 0 ? 255u : selected->oldIds[0];
			++work.creationCount;
			auto* record = Continent(selected->oldIds[0]);

			if (consumed == matchCount)
			{
				for (std::uint32_t index = 0; index < matchCount; ++index)
				{
					if (index != best)
						selected->count += work.creations[matchingRows[index]].count;
				}

				Write(record, NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) + selected->count);
			}
			else
			{
				Write(record, NC2Layout::MapContinentOffset::PointCount, mergedCount);
			}

			Write(record, NC2Layout::MapContinentOffset::SeedPoint, selected->point);
			Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 1);

			if (Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) < 20)
			{
				// Windows uses the already incremented creation index here.
				replacements[temporaryBase + work.creationCount] = 255;
				Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 0);
			}

			for (std::uint32_t index = 0; index < matchCount; ++index)
			{
				const auto row = matchingRows[index];
				auto& creation = work.creations[row];
				links[static_cast<std::size_t>(row) * columns + column] = 0;

				if (!MapMigPoint_IsInsideMap(&creation.point))
					continue;

				selected = &work.creations[work.creationCount];
				selected->oldIds[selected->oldIdCount++] = NXBasics::IntegerRange::UInt32Maximum;
				selected->point = creation.point;
				MapMigPoint_Construct(&creation.point, -1, -1, false);
				selected->type = creation.type;
				selected->count = GameMap_l_Continents_SearchFillArea(&selected->point, work.changedCounts, mapFirst, mapEnd, temporaryBase + work.creationCount, selected->type);
				// The native scan starts at matchingRows[1], independently of the current row.

				for (std::uint32_t other = 1; other < matchCount; ++other)
				{
					const auto otherRow = matchingRows[other];
					auto& candidate = work.creations[otherRow];

					if (MapMigPoint_IsInsideMap(&candidate.point) && PointContinent(&candidate.point) == temporaryBase + work.creationCount)
					{
						MapMigPoint_Construct(&candidate.point, -1, -1, false);
						links[static_cast<std::size_t>(otherRow) * columns + column] = 0;
					}
				}

				++work.creationCount;
			}

			finalizeUnowned();
		}

		GameMap_l_Continents_FillUpFast(replacements, first, end);
		std::memset(replacements, 0, sizeof(replacements));

		for (std::uint32_t row = 0; row < rows; ++row)
		{
			const auto& creation = work.creations[row];
			std::uint32_t ids[250], count = 0;

			for (std::uint32_t id = 0; id < columns; ++id)
			{
				if (links[static_cast<std::size_t>(row) * columns + id] != 0)
					ids[count++] = id;
			}

			if (count < 2)
				continue;

			auto* record = Continent(creation.oldIds[0]);
			Write(record, NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) + creation.count);
			Write(record, NC2Layout::MapContinentOffset::SeedPoint, creation.point);
			Write(record, NC2Layout::MapContinentOffset::Type, creation.type);
			GameMap_l_Continents_SearchFillArea(static_cast<std::byte*>(record) + NC2Layout::MapContinentOffset::SeedPoint, work.changedCounts, first, end, creation.oldIds[0], creation.type);
			Write<std::uint8_t>(record, NC2Layout::MapContinentOffset::RetainedFlag, 1);
			links[static_cast<std::size_t>(row) * columns + creation.oldIds[0]] = 0;

			for (std::uint32_t index = 0; index < count; ++index)
			{
				const auto id = ids[index];

				if (id == creation.oldIds[0])
					continue;

				Write(record, NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(record, NC2Layout::MapContinentOffset::PointCount) + Read<std::uint32_t>(Continent(id), NC2Layout::MapContinentOffset::PointCount));
				replacements[id] = creation.oldIds[0];
				links[static_cast<std::size_t>(row) * columns + id] = 0;
			}
		}

		NXBasics::Runtime_Free(links);
		std::uint32_t id = 1;

		while (id < ContinentCount())
		{
			if (Read<std::uint8_t>(Continent(id), NC2Layout::MapContinentOffset::RetainedFlag) == 0 && replacements[id] == 0)
			{
				const auto last = ContinentCount() - 1u;
				SetContinentCount(last);

				if (replacements[last] == 0)
				{
					if (last == id)
					{
						Write<std::uint8_t>(Continent(id), NC2Layout::MapContinentOffset::RetainedFlag, 0);
						break;
					}

					Write(Continent(id), NC2Layout::MapContinentOffset::PointCount, Read<std::uint32_t>(Continent(last), NC2Layout::MapContinentOffset::PointCount));
					Write(Continent(id), NC2Layout::MapContinentOffset::SeedPoint, Read<std::uint32_t>(Continent(last), NC2Layout::MapContinentOffset::SeedPoint));
					Write(Continent(id), NC2Layout::MapContinentOffset::Type, Read<std::uint32_t>(Continent(last), NC2Layout::MapContinentOffset::Type));
					Write(Continent(id), NC2Layout::MapContinentOffset::RetainedFlag, Read<std::uint8_t>(Continent(last), NC2Layout::MapContinentOffset::RetainedFlag));
					Write<std::uint8_t>(Continent(last), NC2Layout::MapContinentOffset::RetainedFlag, 0);
					replacements[last] = id;
					id = 1;
				}
			}

			++id;
		}

		GameMap_l_Continents_FillUpFast(replacements, first, end);
	}
}