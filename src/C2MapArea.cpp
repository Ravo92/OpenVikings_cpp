#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0045310d.
	void CMapArea::Tool_CountGoods(const void* source, std::uint32_t* amounts) const
	{
		std::memset(amounts, 0, 66 * sizeof(std::uint32_t));
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point))
				{
					const void* micro = GameMap_GetConstMapMicroPointPtr(&point);
					const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && landscape != 0)
					{
						const auto good = gStaticDependencyGoodByLandscape[landscape];
						if (good != 0) amounts[good] += Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency);
					}
				}
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00452bdc; changed walking bits notify the map manager.
	void CMapArea::WalkBlock_Set(const void* source, bool blocked) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point))
				{
					auto* micro = GameMap_GetMapMicroPointPtr(&point);
					const auto value = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags);
					if (((value >> 7) != 0) != blocked)
					{
						Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags, static_cast<std::uint8_t>((value & NXBasics::IntegerRange::Int8Maximum) | (blocked ? 0x80u : 0u)));
						GameMapModifyManager_GameMapModified(14, &point, 0);
					}
				}
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00452c87; outside-map points are skipped.
	bool CMapArea::WalkBlock_Check(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point) && (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00452cfe.
	void CMapArea::BuildBlock_Set(const void* source, bool blocked) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point))
				{
					auto* micro = GameMap_GetMapMicroPointPtr(&point);
					Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags, static_cast<std::uint8_t>((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & NXBasics::BitMask::ByteExceptBit0) | (blocked ? 1u : 0u)));
				}
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00452d79; out-of-map points are skipped.
	bool CMapArea::BuildBlock_Check(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point) && (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::TerrainFlags) & 1u) != 0) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00452df0; every inside-map point invokes Clear separately.
	void CMapArea::Landscapes_RemoveAll(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point)) GameMap_MapMicroPoint_Landscape_Clear(&point);
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_00452e5b.
	bool CMapArea::Check_CanLandscapeBeSetOnPattern(const void* source, std::uint32_t landscape) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point) && !GameMap_Misc_IsLandscapeAllowedOnPattern(&point, landscape)) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00452ed4; the source and every area point must share a land continent.
	bool CMapArea::Check_AreAllPointsOnSameLandContinent(const void* source) const
	{
		const void* micro = GameMap_GetMapMicroPointPtr(source);
		if (MapMicroPointData_Continent_GetContinentTyp(micro) != 1) return false;
		const auto continent = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId);
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point) || Read<std::uint8_t>(GameMap_GetMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId) != continent) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00452f6d; all six surrounding triangle descriptions must allow building.
	bool CMapArea::Check_AreAllPointsAroundOnBuildAllowedPattern(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return false;
				const void* descriptions[6];
				if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(&point, descriptions)) return false;
				for (std::uint32_t triangle = 0; triangle < 6; ++triangle) if (Read<std::uint8_t>(descriptions[triangle], NC2Layout::TrianglePatternTypeOffset::HouseCanBeBuiltOn) == 0) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00452ffe; both native regrow bytes are required on every triangle.
	bool CMapArea::Check_AreAllPointsAroundOnBioRegrowPattern(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return false;
				const void* descriptions[6];
				if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(&point, descriptions)) return false;
				for (std::uint32_t triangle = 0; triangle < 6; ++triangle) if (Read<std::uint8_t>(descriptions[triangle], NC2Layout::TrianglePatternTypeOffset::BioCanGrowOn) == 0 || Read<std::uint8_t>(descriptions[triangle], NC2Layout::TrianglePatternTypeOffset::BioCanPlantOn) == 0) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00453095.
	bool CMapArea::Check_NoGuideOnArea(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point) || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ObjectType) == 3) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00453347.
	bool CMapArea::Check_NoHouseBuildForbiddenFlag(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::TerrainFlags) & NXBasics::BitMask::Bit5) != 0) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_004531b9; an outside point rejects the entire area.
	bool CMapArea::AdditionalAttachmentPoints_CheckIfPointsFreeOrToBeFreed(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return false;
				const auto type = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ObjectType);
				if (type != 0 && type != 1) return false;
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
		return true;
	}

	// Original function: FUN_00453236; only empty points receive attachment offsets.
	void CMapArea::AdditionalAttachmentPoints_SetToMap(const void* source) const
	{
		const auto original = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, &original, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return;
				auto* micro = GameMap_GetMapMicroPointPtr(&point);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 0)
				{
					Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 4);
					Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData, static_cast<std::uint8_t>(Read<std::int16_t>(&original, NC2Layout::MapCoordinatePairOffset::X) - Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X)));
					Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency, static_cast<std::uint8_t>(Read<std::int16_t>(&original, NC2Layout::MapCoordinatePairOffset::Y) - Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)));
				}
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_004532d9; stop at the first outside point.
	void CMapArea::AdditionalAttachmentPoints_ClearFromMap(const void* source) const
	{
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, source, rows[row].x, rows[row].y, false);
			for (std::uint32_t column = 0; column < rows[row].width; ++column)
			{
				if (!MapMigPoint_IsInsideMap(&point)) return;
				Write<std::uint8_t>(GameMap_GetMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ObjectType, 0);
				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}
	}

	// Original function: FUN_004533be; the random call precedes checking rowCount.
	void CMapArea::AreaTool_GetRandomPositionDelta(std::int16_t* x, std::int16_t* y) const
	{
		const auto selected = Game_Random_GetNumber() % pointCount;
		std::uint32_t visited = 0;
		for (std::uint32_t row = 0; row < rowCount; ++row)
		{
			for (std::uint32_t column = 0; column < rows[row].width; ++column, ++visited)
			{
				if (visited != selected) continue;
				*x = static_cast<std::int16_t>(static_cast<std::uint32_t>(rows[row].x) + column);
				*y = rows[row].y;
				return;
			}
		}
	}

	// Original function: FUN_00452b05.
	CMapArea::CMapArea(std::uint32_t numberOfRows)
	{
		std::memset(this, 0, sizeof(*this));
		rowCount = numberOfRows;
		const std::size_t bytes = static_cast<std::size_t>(numberOfRows) * sizeof(SMapAreaRow);
		rows = static_cast<SMapAreaRow*>(NXBasics::Runtime_AllocateArrayWithNewHandler(numberOfRows, sizeof(SMapAreaRow)));
		std::memset(rows, 0, bytes);
	}

	// Original function: FUN_00452b3a.
	CMapArea::~CMapArea()
	{
		NXBasics::Runtime_Free(rows);
	}

	// Original function: FUN_00452b44.
	void CMapArea::UpdateCalculatedVariables()
	{
		pointCount = 0;
		for (std::uint32_t index = 0; index < rowCount; ++index)
			pointCount += rows[index].width;

		minX = minY = 1000;
		maxX = maxY = -1000;
		for (std::uint32_t index = 0; index < rowCount; ++index)
		{
			const SMapAreaRow& row = rows[index];
			// ADD AX/DX and the subsequent comparisons retain only the low word.
			const std::uint16_t endBits = static_cast<std::uint16_t>(row.width + static_cast<std::uint16_t>(row.x) - 1u);
			std::int16_t endX;
			std::memcpy(&endX, &endBits, sizeof(endX));
			if (row.x < minX) minX = row.x;
			if (row.x > maxX) maxX = row.x;
			if (endX < minX) minX = endX;
			if (endX > maxX) maxX = endX;
			if (row.y < minY) minY = row.y;
			if (row.y > maxY) maxY = row.y;
		}
	}
}
