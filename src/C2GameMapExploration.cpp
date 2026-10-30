#include "C2CallbackManager.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	// Original function: FUN_0042469e. This helper does not issue a redraw callback.
	static bool ls_Exploration_ExploreArea(std::uint32_t player, const void* point, const CMapArea* area)
	{
		if (area == nullptr)
			return false;

		bool changed = false;

		for (std::uint32_t rowIndex = 0; rowIndex < area->rowCount; ++rowIndex)
		{
			const auto& row = area->rows[rowIndex];
			std::uint32_t cursor;
			MapMigPoint_ConstructOffset(&cursor, point, row.x, row.y, false);

			for (std::uint32_t column = 0; column < row.width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&cursor))
				{
					auto* micro = GameMap_GetMapMicroPointPtr(&cursor);

					if (player < 16)
					{
						const auto mask = static_cast<std::uint16_t>(1u << player);
						const auto seen = NC2Lifetime::Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask);

						if ((seen & mask) == 0)
						{
							NC2Lifetime::Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask, seen | mask);
							changed = true;
						}
					}
				}

				cursor = (cursor & NXBasics::BitMask::DwordExceptBits0Through15) | static_cast<std::uint16_t>(cursor + 1u);
			}
		}

		return changed;
	}

	// Original function: FUN_0042475a; Windows outline of the Mac's house-area branch.
	static bool Exploration_ExploreHouseFootprint(std::uint32_t player, const void* point)
	{
		using namespace NC2Lifetime;
		const auto* micro = GameMap_GetMapMicroPointPtr(point);
		const auto houseId = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) : NXBasics::IntegerRange::UInt32Maximum;
		const auto* house = CHouseArray::mStaticVars + houseId * CHouseArray::RuntimeElementSize;
		const auto* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));

		return ls_Exploration_ExploreArea(player, point, Read<const CMapArea*>(definition, NC2Layout::EngineHouseLogicViewOffset::BuildArea));
	}

	// Original function: FUN_004243c6.
	bool GameMap_Exploration_DoExplore(std::uint32_t player, const void* point, std::uint32_t radius)
	{
		using namespace NC2Lifetime;

		if (player >= 16)
			return false;

		bool changed = false;
		std::uint32_t extraExtent = 2;
		const auto explorePoint = [&](const void* current)
		{
			if (!MapMigPoint_IsInsideMap(current))
			{

				return;
			}

			auto* micro = GameMap_GetMapMicroPointPtr(current);
			const auto mask = static_cast<std::uint16_t>(1u << player);
			const auto seen = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask);

			if ((seen & mask) == 0)
			{
				Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask, seen | mask);
				changed = true;
			}

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) != 0)
			{
				changed |= Exploration_ExploreHouseFootprint(player, current);
				extraExtent = 5;
			}

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 4)
			{
				const auto x = static_cast<std::uint16_t>(Read<std::int16_t>(current, NC2Layout::MapCoordinatePairOffset::X) + Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData));
				const auto y = static_cast<std::uint16_t>(Read<std::int16_t>(current, NC2Layout::MapCoordinatePairOffset::Y) + Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency));
				const std::uint32_t origin = x | (static_cast<std::uint32_t>(y) << 16);
				const auto* landscape = GameMap_GetConstMapMicroPointPtr(&origin);
				const auto valency = Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(landscape, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0;
				const auto engineId = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(&origin);
				const auto* area = NC2LogicExternals::External_Landscape_GetAreaPtr(engineId, valency, 1);
				changed |= ls_Exploration_ExploreArea(player, &origin, area);
				extraExtent = 10;
			}
		};
		explorePoint(point);
		auto cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

		for (std::uint32_t ring = 1; ring <= radius && radius != 0; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(cursor, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					explorePoint(&cursor);
					MapMigPoint_MoveCursorUnwrapped(cursor, direction);
				}
			}
		}

		if (changed)
		{
			const auto extent = radius + extraExtent;
			// Retain Windows DWORD packing, including overflow outside the low WORD.
			const auto size = (extent * 2u) | (extent << 17);
			const auto x = (static_cast<std::uint32_t>(Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::X)) - extent) & NXBasics::IntegerRange::UInt16Maximum;
			const auto y = static_cast<std::uint32_t>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)) - extent;
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0D, true, player, x | (y << 16), size);
		}

		return changed;
	}

	// Original function: FUN_0042479b.
	bool GameMap_Exploration_DoExploreAll(std::uint32_t player)
	{
		using namespace NC2Lifetime;

		if (player >= 16)
			return false;

		bool changed = false;
		const auto mask = static_cast<std::uint16_t>(1u << player);

		for (std::int16_t y = 0; y < GameMap_GetHeight(); ++y)
		{
			for (std::int16_t x = 0; x < GameMap_GetWidth(); ++x)
			{
				const auto point = static_cast<std::uint16_t>(x) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y)) << 16);
				auto* micro = GameMap_GetMapMicroPointPtr(&point);
				const auto seen = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask);

				if ((seen & mask) == 0)
				{
					Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask, seen | mask);
					changed = true;
				}
			}
		}

		if (changed)
		{
			const auto size = static_cast<std::uint32_t>(GameMap_GetWidth()) | (static_cast<std::uint32_t>(GameMap_GetHeight()) << 16);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x0D, true, player, 0, size);
		}

		return changed;
	}
}