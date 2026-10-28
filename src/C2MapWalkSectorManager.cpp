#include "ApplicationGlobals.h"
#include "C2CallbackManager.h"
#include "C2GameMapAccess.h"
#include "C2IoHelper.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSectorSearch.h"
#include "C2MapSectorGridCursor.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	namespace
	{
		std::uint32_t PackPoint(std::int32_t x, std::int32_t y)
		{
			return static_cast<std::uint16_t>(x) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y)) << 16);
		}

		std::int16_t PointCoordinate(std::uint32_t point, bool y)
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(y ? point >> 16 : point);
			std::int16_t value;
			std::memcpy(&value, &bits, sizeof(value));
			return value;
		}

		std::uint8_t MicroPointByte(const void* point, std::size_t offset)
		{
			return static_cast<const std::uint8_t*>(point)[offset];
		}

		std::uint32_t SectorBytes()
		{
			return static_cast<std::uint32_t>(CMapWalkSectorManager::mStaticVars.gridWidth) * static_cast<std::uint32_t>(CMapWalkSectorManager::mStaticVars.gridHeight) * 0x34u;
		}

		SMapWalkSectorData* AllocateWalkSectorArray(std::int32_t sectorCount)
		{
			auto* records = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(sectorCount), sizeof(SMapWalkSectorData)));

			if (records != nullptr)
			{
				for (std::int32_t index = 0; index < sectorCount; ++index)
				{
					auto* points = records + static_cast<std::size_t>(index) * sizeof(SMapWalkSectorData) + 0x24u;
					for (std::uint32_t point = 0; point < 3; ++point) MapMigPoint_ConstructZero(points + point * 4u);
				}
			}

			return reinterpret_cast<SMapWalkSectorData*>(records);
		}


	}

	// Original function: FUN_0044f16d.
	CMapWalkSectorManager::CMapWalkSectorManager()
	{
		sTheObjectPtr = this;
		std::memset(&mStaticVars, 0, sizeof(mStaticVars));

		AllocateArrays();
		RegisterCallbacks();
		CMapPathfinderSpecialSectorSearchElement::Init(mStaticVars.specialSectorSearchWorkspace);
	}

	// Original function: FUN_0044f1a2.
	CMapWalkSectorManager::~CMapWalkSectorManager()
	{
		CCallbackManager::sTheObjectPtr->UnRegisterCallback(0x31u, l_Sectors_Callback, 0);
		CCallbackManager::sTheObjectPtr->UnRegisterCallback(0x32u, l_Sectors_Callback, 0);
		CCallbackManager::sTheObjectPtr->UnRegisterCallback(0x33u, l_Sectors_Callback, 0);
		CCallbackManager::sTheObjectPtr->UnRegisterCallback(0x16u, l_Sectors_Callback, 0);

		if (mStaticVars.landSectors != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.landSectors);
			mStaticVars.landSectors = nullptr;
		}

		if (mStaticVars.waterSectors != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.waterSectors);
			mStaticVars.waterSectors = nullptr;
		}

		if (mStaticVars.calculationMap != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.calculationMap);
			mStaticVars.calculationMap = nullptr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_0044f232.
	void CMapWalkSectorManager::AllocateArrays()
	{
		std::int16_t macroWidth;
		std::int16_t macroHeight;
		std::memcpy(&macroWidth, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(macroWidth));
		std::memcpy(&macroHeight, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, sizeof(macroHeight));

		mStaticVars.gridWidth = (static_cast<std::int32_t>(macroWidth) + 9) / 10;
		SSectorGridCursor::sGridWidth = mStaticVars.gridWidth;

		mStaticVars.gridHeight = (static_cast<std::int32_t>(macroHeight) + 9) / 10;
		SSectorGridCursor::sGridHeight = mStaticVars.gridHeight;

		if (mStaticVars.landSectors != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.landSectors);
			mStaticVars.landSectors = nullptr;
		}

		if (mStaticVars.waterSectors != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.waterSectors);
			mStaticVars.waterSectors = nullptr;
		}

		const std::int32_t firstSectorCount = mStaticVars.gridWidth * mStaticVars.gridHeight;
		mStaticVars.landSectors = AllocateWalkSectorArray(firstSectorCount);

		const std::int32_t secondSectorCount = mStaticVars.gridWidth * mStaticVars.gridHeight;
		mStaticVars.waterSectors = AllocateWalkSectorArray(secondSectorCount);

		if (mStaticVars.calculationMap != nullptr)
		{
			NXBasics::Runtime_Free(mStaticVars.calculationMap);
			mStaticVars.calculationMap = nullptr;
		}

		mStaticVars.calculationMap = static_cast<std::uint32_t*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(mStaticVars.gridWidth * mStaticVars.gridHeight), sizeof(*mStaticVars.calculationMap)));

		// The original function does not clear the calculation map here.
		l_Sectors_ClearAll();
	}

	// Original function: FUN_0044f379.
	void CMapWalkSectorManager::RegisterCallbacks()
	{
		CCallbackManager::sTheObjectPtr->RegisterCallback(0x31u, l_Sectors_Callback, 0, 5);
		CCallbackManager::sTheObjectPtr->RegisterCallback(0x32u, l_Sectors_Callback, 0, 5);
		CCallbackManager::sTheObjectPtr->RegisterCallback(0x33u, l_Sectors_Callback, 0, 5);
		CCallbackManager::sTheObjectPtr->RegisterCallback(0x16u, l_Sectors_Callback, 0, 100);
	}

	// Original function: FUN_0044f3c9.
	void CMapWalkSectorManager::l_Sectors_CalculationMap_ClearAll()
	{
		const std::size_t allocationBytes = static_cast<std::size_t>(static_cast<std::uint32_t>(mStaticVars.gridWidth * mStaticVars.gridHeight)) * sizeof(*mStaticVars.calculationMap);
		std::memset(mStaticVars.calculationMap, 0, allocationBytes);
	}

	// Original function: FUN_0044f3ea.
	void CMapWalkSectorManager::l_Sectors_ClearAll()
	{
		for (std::int32_t y = 0; y < mStaticVars.gridHeight; ++y)
		{
			for (std::int32_t x = 0; x < mStaticVars.gridWidth; ++x)
			{
				const std::uint32_t index = static_cast<std::uint32_t>(y) * static_cast<std::uint32_t>(mStaticVars.gridWidth) + static_cast<std::uint32_t>(x);
				const std::size_t offset = static_cast<std::size_t>(index) * sizeof(SMapWalkSectorData);

				std::memset(reinterpret_cast<std::byte*>(mStaticVars.landSectors) + offset, 0, 52);
				std::memset(reinterpret_cast<std::byte*>(mStaticVars.waterSectors) + offset, 0, 52);
			}
		}
	}

	// Original function: FUN_0044f452.
	void CMapWalkSectorManager::l_Sectors_RecalculateAll()
	{
		SSectorGridCursor cursor{ };
		for (std::int32_t y = 0; y < mStaticVars.gridHeight; ++y)
		{
			for (std::int32_t x = 0; x < mStaticVars.gridWidth; ++x)
			{
				cursor.x = x;
				cursor.y = y;
				auto* land = GetLandSectorDataPtr(cursor);
				bool changed = land->basePointCount != 0 && MicroPointByte(GameMap_GetConstMapMicroPointPtr(land->basePoints), 6) != land->continentId;
				if (!changed)
				{
					auto* water = GetWaterSectorDataPtr(cursor);
					changed = water->basePointCount != 0 && MicroPointByte(GameMap_GetConstMapMicroPointPtr(water->basePoints), 6) != water->continentId;
				}
				if (changed)
				{
					GetLandSectorDataPtr(cursor)->calculated = 0;
					GetWaterSectorDataPtr(cursor)->calculated = 0;
				}
			}
		}

		// Complete invalidation before updating any sector and its connections.
		for (std::int32_t y = 0; y < mStaticVars.gridHeight; ++y)
		{
			for (std::int32_t x = 0; x < mStaticVars.gridWidth; ++x)
			{
				cursor.x = x;
				cursor.y = y;
				if (GetLandSectorDataPtr(cursor)->calculated == 0 || GetWaterSectorDataPtr(cursor)->calculated == 0)
					l_Sectors_UpdateSector(cursor);
			}
		}
	}

	// Original function: FUN_0044f5a0.
	void CMapWalkSectorManager::l_Sectors_UpdateSector(const SSectorGridCursor& cursor)
	{
		auto* land = GetLandSectorDataPtr(cursor);
		auto* water = GetWaterSectorDataPtr(cursor);
		for (std::uint32_t index = 0; index < land->basePointCount; ++index)
			static_cast<std::uint8_t*>(GameMap_GetMapMicroPointPtr(land->basePoints + index))[13] &= NXBasics::BitMask::ByteExceptBit4;
		for (std::uint32_t index = 0; index < water->basePointCount; ++index)
			static_cast<std::uint8_t*>(GameMap_GetMapMicroPointPtr(water->basePoints + index))[13] &= NXBasics::BitMask::ByteExceptBit4;

		std::memset(land, 0, 52);
		std::memset(water, 0, 52);
		std::memset(mStaticVars.basePointCandidates, 0, sizeof(mStaticVars.basePointCandidates));
		const std::uint32_t center = PackPoint(10 + static_cast<std::int32_t>(static_cast<std::uint16_t>(cursor.x) * 20u), 10 + static_cast<std::int32_t>(static_cast<std::uint16_t>(cursor.y) * 20u));
		if (MapMigPoint_IsInsideMap(&center))
		{
			const void* point = GameMap_GetConstMapMicroPointPtr(&center);
			const std::uint8_t continent = MicroPointByte(point, 6);
			const std::uint8_t flags = MicroPointByte(point, 0x0C);
			if (continent != 0 && (flags & NXBasics::BitMask::Bit7) == 0)
			{
				auto& candidate = mStaticVars.basePointCandidates[continent];
				candidate.packedBasePoint = center;
				++candidate.selectionCount;
				candidate.lowBitsValue = flags & 7u;
			}
		}

		for (std::uint32_t radius = 1, sideLength = 2; sideLength < 20u; ++radius, sideLength += 2)
		{
			SSectorGridCursor scan{ };
			scan.x = PointCoordinate(center, false);
			scan.y = PointCoordinate(center, true);
			scan.MoveCursorUnwrapped(5, radius);
			for (std::uint32_t direction = 0; direction < 8; direction += 2)
			{
				for (std::uint32_t step = 0; step < sideLength; ++step)
				{
					const std::uint32_t packedPoint = PackPoint(scan.x, scan.y);
					if (MapMigPoint_IsInsideMap(&packedPoint))
					{
						const void* point = GameMap_GetConstMapMicroPointPtr(&packedPoint);
						const std::uint8_t continent = MicroPointByte(point, 6);
						const std::uint8_t flags = MicroPointByte(point, 0x0C);
						if (continent != 0 && (flags & NXBasics::BitMask::Bit7) == 0)
						{
							std::uint32_t value = flags & 7u;
							const std::uint32_t limit = GameMap_Continent_GetContinentTyp(continent) == 1u ? 1u : 4u;
							if (value > limit)
								value = limit;
							auto& candidate = mStaticVars.basePointCandidates[continent];
							if (candidate.selectionCount == 0 || value > candidate.lowBitsValue)
							{
								candidate.packedBasePoint = packedPoint;
								candidate.lowBitsValue = value;
								++candidate.selectionCount;
							}
						}
					}
					scan.MoveCursorUnwrapped(direction);
				}
			}
		}

		std::uint32_t continentCount;
		std::memcpy(&continentCount, gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, sizeof(continentCount));
		std::uint32_t landContinent = 0;
		std::uint32_t waterContinent = 0;
		// The Windows counter is a byte, including its original wraparound.
		for (std::uint8_t continent = 1; continent < continentCount; ++continent)
		{
			const std::uint32_t type = GameMap_Continent_GetContinentTyp(continent);
			const auto& candidate = mStaticVars.basePointCandidates[continent];
			if (type == 1u)
			{
				if (candidate.selectionCount != 0 && (landContinent == 0 || candidate.selectionCount > mStaticVars.basePointCandidates[landContinent].selectionCount))
					landContinent = continent;
			}
			else if (candidate.selectionCount != 0 && (waterContinent == 0 || candidate.selectionCount > mStaticVars.basePointCandidates[waterContinent].selectionCount))
				waterContinent = continent;
		}
		if (landContinent != 0)
		{
			land->continentId = static_cast<std::uint8_t>(landContinent);
			land->basePoints[land->basePointCount++] = mStaticVars.basePointCandidates[landContinent].packedBasePoint;
		}
		if (waterContinent != 0)
		{
			water->continentId = static_cast<std::uint8_t>(waterContinent);
			water->basePoints[water->basePointCount++] = mStaticVars.basePointCandidates[waterContinent].packedBasePoint;
		}

		std::int32_t countScanIndex = 7;
		if (land->continentId != 0)
		{
			l_Sectors_SpecialSectorFloodFill(*land, cursor);
			while (countScanIndex >= 0 && mStaticVars.floodFillPointCounts[countScanIndex] == 0)
				--countScanIndex;
			const std::int32_t group = countScanIndex;
			std::uint32_t point = 0;
			while (group >= 0 && land->basePointCount < 3u)
			{
				l_Sectors_GetNextBasePoint(mStaticVars.floodFillPoints[group], mStaticVars.floodFillPointCounts[group], mStaticVars.nextFloodFillPointIndices[group], point);
				if (!MapMigPoint_IsInsideMap(&point))
					break;
				land->basePoints[land->basePointCount++] = point;
			}
		}
		if (water->continentId != 0)
		{
			l_Sectors_SpecialSectorFloodFill(*water, cursor);
			// Remaster: water flood-fill groups have their own counts and scan origin.
			std::int32_t group = 7;
			while (group >= 0 && mStaticVars.floodFillPointCounts[group] == 0) --group;
			std::uint32_t point = 0;
			while (group >= 0 && water->basePointCount < 3u)
			{
				l_Sectors_GetNextBasePoint(mStaticVars.floodFillPoints[group], mStaticVars.floodFillPointCounts[group], mStaticVars.nextFloodFillPointIndices[group], point);
				if (!MapMigPoint_IsInsideMap(&point))
					break;
				water->basePoints[water->basePointCount++] = point;
			}
		}

		if (land->basePointCount != 0)
		{
			for (std::uint32_t direction = 0; direction < 8; ++direction)
				land->connectionValues[direction] = MicroPointByte(GameMap_GetConstMapMicroPointPtr(land->basePoints), 0x0C) & 7u;
		}
		if (water->basePointCount != 0)
		{
			for (std::uint32_t direction = 0; direction < 8; ++direction)
				water->connectionValues[direction] = MicroPointByte(GameMap_GetConstMapMicroPointPtr(water->basePoints), 0x0C) & 7u;
		}
		l_Sectors_CheckAndSetConnection(cursor, 0);
		l_Sectors_CheckAndSetConnection(cursor, 4);
		l_Sectors_CheckAndSetConnection(cursor, 6);
		l_Sectors_CheckAndSetConnection(cursor, 2);
		land->calculated = 1;
		water->calculated = 1;
		for (std::uint32_t index = 0; index < land->basePointCount; ++index)
			static_cast<std::uint8_t*>(GameMap_GetMapMicroPointPtr(land->basePoints + index))[13] |= NXBasics::BitMask::Bit4;
		for (std::uint32_t index = 0; index < water->basePointCount; ++index)
			static_cast<std::uint8_t*>(GameMap_GetMapMicroPointPtr(water->basePoints + index))[13] |= NXBasics::BitMask::Bit4;
	}

	// Original function: FUN_0044faa6.
	void CMapWalkSectorManager::l_Sectors_SpecialSectorFloodFill(SMapWalkSectorData& sector, const SSectorGridCursor& cursor)
	{
		const std::int32_t left = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(cursor.x) * 20u);
		const std::int32_t top = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(cursor.y) * 20u);
		std::memset(mStaticVars.floodFillPoints, 0, sizeof(mStaticVars.floodFillPoints));
		std::memset(mStaticVars.floodFillPointCounts, 0, sizeof(mStaticVars.floodFillPointCounts));
		std::memset(mStaticVars.nextFloodFillPointIndices, 0, sizeof(mStaticVars.nextFloodFillPointIndices));
		const std::uint32_t center = PackPoint(10 + static_cast<std::int32_t>(static_cast<std::uint16_t>(cursor.x) * 20u), 10 + static_cast<std::int32_t>(static_cast<std::uint16_t>(cursor.y) * 20u));
		GameMap_CalculationMap_Clear(&center, 30);
		const std::uint32_t basePoint = sector.basePoints[0];
		GameMap_CalculationMap_SetValue(&basePoint, 1);

		// Original shared pathfinder storage: clear only the first two queue counts.
		auto* queues = sMapPathfinderStaticVariables;
		queues[0].count = 1;
		queues[0].points[0] = basePoint;
		queues[1].count = 0;
		std::uint32_t current = 0;
		do
		{
			auto& source = queues[current];
			auto& destination = queues[(current - 1u) & 1u];
			destination.count = 0;
			for (std::uint32_t index = 0; index < source.count; ++index)
			{
				GameMap_GetConstMapMicroPointPtr(source.points + index);
				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					std::uint32_t point = source.points[index];
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					const std::int32_t x = PointCoordinate(point, false);
					const std::int32_t y = PointCoordinate(point, true);
					if (x < left || x >= NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(left) + 20u) || y < top || y >= NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(top) + 20u) || !MapMigPoint_IsInsideMap(&point) || GameMap_CalculationMap_GetValue(&point) != 0)
						continue;
					const void* microPoint = GameMap_GetConstMapMicroPointPtr(&point);
					const std::uint8_t flags = MicroPointByte(microPoint, 0x0C);
					if ((flags & NXBasics::BitMask::Bit7) != 0 || MicroPointByte(microPoint, 6) != sector.continentId || destination.count == 4000u)
						continue;

					destination.points[destination.count++] = point;
					std::uint32_t value = flags & 7u;
					const std::uint32_t limit = GameMap_Continent_GetContinentTyp(MicroPointByte(microPoint, 6)) == 1u ? 1u : 4u;
					if (value > limit)
						value = limit;
					auto& count = mStaticVars.floodFillPointCounts[value];
					if (count != 400)
					{
						mStaticVars.floodFillPoints[value][count++] = point;
						// Preserve the original order: marking occurs only after both capacity checks.
						GameMap_CalculationMap_SetValue(&point, 1);
					}
				}
			}
			current = (current + 1u) & 1u;
		} while (queues[current].count != 0);
	}

	// Original function: FUN_0044fceb.
	void CMapWalkSectorManager::l_Sectors_GetNextBasePoint(std::uint32_t* points, std::int32_t& count, std::int32_t& nextIndex, std::uint32_t& point)
	{
		if (count == nextIndex)
		{
			count = 0;
			point = NXBasics::IntegerRange::UInt32Maximum;
			return;
		}
		if (MapMigPoint_IsInsideMap(points + nextIndex))
		{
			point = points[nextIndex];
			points[nextIndex] = NXBasics::IntegerRange::UInt32Maximum;
			++nextIndex;
		}
		// An invalid entry leaves the output and next index unchanged in Windows.
	}

	// Original function: FUN_004507f2.
	bool __cdecl CMapWalkSectorManager::l_Sectors_Callback(TCallbackTypes type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t /*param3*/)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		if (type == 0x33u)
		{
			if (param1 == 0x0Eu || param1 == 0x0Bu)
			{
				const SSectorGridCursor cursor(param2);
				GetLandSectorDataPtr(cursor)->calculated = 0;
				GetWaterSectorDataPtr(cursor)->calculated = 0;
			}
		}
		else if (type == 0x32u)
			l_Sectors_RecalculateAll();
		else if (type == 0x16u && mStaticVars.requiresSectorRecalculation != 0)
		{
			l_Sectors_ClearAll();
			l_Sectors_RecalculateAll();
			mStaticVars.requiresSectorRecalculation = 0;
		}
		return true;
	}

	// Original function: FUN_00450883.
	SMapWalkSectorData* CMapWalkSectorManager::GetLandSectorDataPtr(const SSectorGridCursor& cursor)
	{
		const std::size_t offset = (static_cast<std::size_t>(static_cast<std::uint32_t>(mStaticVars.gridWidth)) * static_cast<std::uint32_t>(cursor.y) + static_cast<std::uint32_t>(cursor.x)) * sizeof(SMapWalkSectorData);
		return reinterpret_cast<SMapWalkSectorData*>(reinterpret_cast<std::byte*>(mStaticVars.landSectors) + offset);
	}

	// Semantic counterpart of the Windows inline water-sector accesses.
	SMapWalkSectorData* CMapWalkSectorManager::GetWaterSectorDataPtr(const SSectorGridCursor& cursor)
	{
		const std::size_t offset = (static_cast<std::size_t>(static_cast<std::uint32_t>(mStaticVars.gridWidth)) * static_cast<std::uint32_t>(cursor.y) + static_cast<std::uint32_t>(cursor.x)) * sizeof(SMapWalkSectorData);
		return reinterpret_cast<SMapWalkSectorData*>(reinterpret_cast<std::byte*>(mStaticVars.waterSectors) + offset);
	}

	// Original function: FUN_0045089d.
	void CMapWalkSectorManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(SwitchIds::LogicMapChunk::WalkSectors, 3);
		io.IO_Memory_WriteBinary(&mStaticVars, 8);
		io.IO_Memory_WriteBinary(mStaticVars.landSectors, SectorBytes());
		io.IO_Memory_WriteBinary(mStaticVars.waterSectors, SectorBytes());
		io.IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00450901.
	CMapWalkSectorManager::CMapWalkSectorManager(CIoHelper& io)
	{
		sTheObjectPtr = this;
		std::memset(&mStaticVars, 0, sizeof(mStaticVars));
		AllocateArrays();
		RegisterCallbacks();
		CMapPathfinderSpecialSectorSearchElement::Init(mStaticVars.specialSectorSearchWorkspace);
		if (io.mReadHeader.version != 3u)
		{
			mStaticVars.requiresSectorRecalculation = 1;
			io.IO_File_Chunk_Skip();
		}
		else
		{
			// Windows reads saved dimensions after allocating from the current map dimensions.
			io.IO_File_ReadBinary(&mStaticVars, 8);
			io.IO_File_ReadBinary(mStaticVars.landSectors, SectorBytes());
			io.IO_File_ReadBinary(mStaticVars.waterSectors, SectorBytes());
		}
	}

	// Original function: FUN_0045099b.
	bool CMapWalkSectorManager::IO_Cleanmap_Load(CIoHelper& io)
	{
		if (io.mReadHeader.version != 4u)
		{
			mStaticVars.requiresSectorRecalculation = 1;
			io.IO_File_Chunk_Skip();
		}
		else
		{
			io.IO_File_ReadBinary(mStaticVars.landSectors, SectorBytes());
			io.IO_File_ReadBinary(mStaticVars.waterSectors, SectorBytes());
		}
		return true;
	}
}
