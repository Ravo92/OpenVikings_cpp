#include "C2GameMapStorage.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "XBasicTypes.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_00431cd3; Windows-only outlined root registration.
	void MapMicroPointData_House_SetRoot(void* micro, std::uint32_t house)
	{
		if (NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 0)
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData, (NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & NXBasics::BitMask::ByteExceptBit1) | 1u);
		NC2Lifetime::Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 2);
		NC2Lifetime::Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId, static_cast<std::uint16_t>(house));
	}

	// Original function: FUN_00431dc6; Windows-only outlined door registration.
	void MapMicroPointData_House_SetDoor(void* micro, std::uint32_t house)
	{
		if (NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 0)
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData, (NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & NXBasics::BitMask::ByteExceptBit0) | 2u);
		NC2Lifetime::Write<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType, 2);
		NC2Lifetime::Write<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId, static_cast<std::uint16_t>(house));
	}

	// Original function: FUN_004249ae; the Mac build inlines this predicate.
	bool GameMap_Misc_CanBeUsedAsLandscapeWorkPosition(const void* point)
	{
		const void* micro = GameMap_GetConstMapMicroPointPtr(point);

		return (NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && NC2Lifetime::Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingDirections) != 0;
	}

	// Original function: FUN_004248ed; try five independently selected area positions.
	bool GameMap_Misc_GetLandscapeWorkPosition(const void* realPosition, void* workPosition)
	{
		using namespace NC2Lifetime;
		const void* micro = GameMap_GetConstMapMicroPointPtr(realPosition);
		// Windows checks the computed micro address, not the unoffset buffer pointer.

		if (micro == nullptr)
			return false;

		const auto engineId = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(realPosition);

		if (engineId == -1)
			return false;

		const auto valency = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
		const auto* area = NC2LogicExternals::External_Landscape_GetAreaPtr(engineId, valency, 2);

		if (area == nullptr)
		{
			if (!GameMap_Misc_CanBeUsedAsLandscapeWorkPosition(realPosition))
				return false;

			Write(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates));
			return true;
		}

		std::int16_t x, y;

		for (std::uint32_t attempt = 0; attempt < 5; ++attempt)
		{
			area->AreaTool_GetRandomPositionDelta(&x, &y);
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, realPosition, x, y, false);

			if (!MapMigPoint_IsInsideMap(&point) || !GameMap_Misc_CanBeUsedAsLandscapeWorkPosition(&point))
				continue;

			Write(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, point);
			return true;
		}

		return false;
	}

	std::int16_t GameMap_GetWidth()
	{
		std::int16_t width;
		std::memcpy(&width, gGameMapStaticStorage, sizeof(width));

		return width;
	}

	std::int16_t GameMap_GetHeight()
	{
		std::int16_t height;
		std::memcpy(&height, gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroHeight, sizeof(height));

		return height;
	}

	std::uint16_t GameMap_GetMacroWidth()
	{
		std::uint16_t width;
		std::memcpy(&width, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(width));

		return width;
	}

	std::uint16_t GameMap_GetMacroHeight()
	{
		std::uint16_t height;
		std::memcpy(&height, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, sizeof(height));

		return height;
	}

	// Original instruction range: 004233AE-004233E6 in FUN_0042329f.
	void GameMap_ReleaseMapBuffers()
	{
		const auto releaseBuffer = [](std::size_t offset)
		{
			void* buffer;
			std::memcpy(&buffer, gGameMapStaticStorage + offset, sizeof(buffer));

			NXBasics::Runtime_Free(buffer);

			buffer = nullptr;
			std::memcpy(gGameMapStaticStorage + offset, &buffer, sizeof(buffer));
		};

		releaseBuffer(NC2Layout::GameMapOffset::PackingScratchBuffer);
		releaseBuffer(NC2Layout::GameMapOffset::MacroPointStorage);
		releaseBuffer(NC2Layout::GameMapOffset::MicroPointStorage);
	}

	// Original function: FUN_004234c4
	void GameMap_InitAndCreateEmptyMap(std::int16_t macroWidth, std::int16_t macroHeight)
	{
		static_assert(sizeof(NXBasics::SRectangle) == 16);
		static_assert(sizeof(gGameMapStaticStorage) == sizeof(SLogicMapStorage));

		const auto doubleLowWord = [](std::int16_t value) -> std::int16_t
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(static_cast<std::int32_t>(value) * 2);
			std::int16_t result;
			std::memcpy(&result, &bits, sizeof(result));

			return result;
		};

		std::memset(gGameMapStaticStorage, 0, sizeof(gGameMapStaticStorage));

		// The continent count precedes the 250 serialized 16-byte continent records.
		const std::uint32_t initialValue3C = 1;
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, &initialValue3C, sizeof(initialValue3C));

		const std::uint32_t macroPointCount = static_cast<std::uint32_t>(static_cast<std::int32_t>(macroWidth) * static_cast<std::int32_t>(macroHeight));

		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, &macroWidth, sizeof(macroWidth));
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, &macroHeight, sizeof(macroHeight));
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroPointCount, &macroPointCount, sizeof(macroPointCount));

		const NXBasics::SRectangle macroRectangle(0, 0, macroWidth, macroHeight);
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroRectangle, &macroRectangle, sizeof(macroRectangle));

		const std::int16_t microWidth = doubleLowWord(macroWidth);
		const std::int16_t microHeight = doubleLowWord(macroHeight);
		const std::uint32_t microPointCount = static_cast<std::uint32_t>(static_cast<std::int32_t>(microWidth) * static_cast<std::int32_t>(microHeight));

		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroWidth, &microWidth, sizeof(microWidth));
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroHeight, &microHeight, sizeof(microHeight));
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroPointCount, &microPointCount, sizeof(microPointCount));

		const NXBasics::SRectangle microRectangle(0, 0, microWidth, microHeight);
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroRectangle, &microRectangle, sizeof(microRectangle));

		// Original mirror writes at 0042354D..00423577, before buffer allocation.
		gMapPointDimensions.macroWidth = static_cast<std::int16_t>(GameMap_GetMacroWidth());
		gMapPointDimensions.macroHeight = static_cast<std::int16_t>(GameMap_GetMacroHeight());
		gMapPointDimensions.microWidth = GameMap_GetWidth();
		gMapPointDimensions.microHeight = GameMap_GetHeight();
		void* macroPoints = NXBasics::Runtime_AllocateArrayWithNewHandler(macroPointCount, sizeof(SLogicMapMacroPointStorage));
		const std::size_t macroBytes = static_cast<std::size_t>(macroPointCount) * sizeof(SLogicMapMacroPointStorage);
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroPointStorage, &macroPoints, sizeof(macroPoints));
		std::memset(macroPoints, 0, macroBytes);

		auto* microPoints = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(microPointCount, sizeof(SLogicMapMicroPointStorage)));
		const std::size_t microBytes = static_cast<std::size_t>(microPointCount) * sizeof(SLogicMapMicroPointStorage);
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroPointStorage, &microPoints, sizeof(microPoints));
		std::memset(microPoints, 0, microBytes);

		const std::uint16_t initialFirstMapMoveableId = NXBasics::IntegerRange::UInt16Maximum;
		std::size_t microOffset = 0;

		for (std::uint32_t index = 0; index < microPointCount; ++index)
		{
			// The empty cell list uses the original all-one WORD sentinel.
			std::memcpy(microPoints + microOffset + NC2Layout::MapMicroPointOffset::FirstMapMoveableId, &initialFirstMapMoveableId, sizeof(initialFirstMapMoveableId));
			microOffset += 14u;
		}

		void* allocation38 = NXBasics::Runtime_AllocateArrayWithNewHandler(microPointCount, sizeof(std::int16_t));
		const std::size_t allocationBytes38 = static_cast<std::size_t>(microPointCount) * sizeof(std::int16_t);
		std::memcpy(gGameMapStaticStorage + NC2Layout::GameMapOffset::PackingScratchBuffer, &allocation38, sizeof(allocation38));
		std::memset(allocation38, 0, allocationBytes38);
	}

	// Original function: FUN_0042378b
	const void* GameMap_GetConstMapMacroPointPtr(std::int16_t x, std::int16_t y)
	{
		std::int16_t width;
		std::int16_t height;

		std::memcpy(&width, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(width));
		std::memcpy(&height, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, sizeof(height));

		if (x < 0 || x >= width || y < 0 || y >= height)
			return nullptr;

		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroPointStorage, sizeof(baseAddress));

		const std::uint32_t index = static_cast<std::uint32_t>(static_cast<std::int32_t>(y)) * static_cast<std::uint32_t>(static_cast<std::int32_t>(width)) + static_cast<std::uint32_t>(static_cast<std::int32_t>(x));
		const std::uintptr_t address = baseAddress + static_cast<std::size_t>(index) * 3u;

		return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address));
	}

	// Original function: FUN_00423838; unlike the const accessor, there is no bounds check.
	void* GameMap_GetMapMacroPointPtr(const void* packedMagPoint)
	{
		std::int16_t x, y, width;
		std::uintptr_t base;
		std::memcpy(&x, packedMagPoint, sizeof(x));
		std::memcpy(&y, static_cast<const std::byte*>(packedMagPoint) + 2, sizeof(y));
		std::memcpy(&width, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(width));
		std::memcpy(&base, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroPointStorage, sizeof(base));
		const std::intptr_t index = static_cast<std::int32_t>(y) * static_cast<std::int32_t>(width) + static_cast<std::int32_t>(x);

		return reinterpret_cast<void*>(base + index * std::uintptr_t{ 3 });
	}

	// Original function: FUN_0042372d; the point overload checks the geometry dimensions.
	const void* GameMap_GetConstMapMacroPointPtr(const void* packedMagPoint)
	{
		if (!MapMagPoint_IsInsideMap(packedMagPoint))
			return nullptr;

		return GameMap_GetMapMacroPointPtr(packedMagPoint);
	}

	// Original function: FUN_004237d2
	const void* GameMap_GetConstMapMicroPointPtr(const void* packedMigPoint)
	{
		const auto* position = static_cast<const std::byte*>(packedMigPoint);

		std::int16_t y;
		std::int16_t width;
		std::int16_t x;

		std::memcpy(&y, position + 2, sizeof(y));
		width = GameMap_GetWidth();
		std::memcpy(&x, position, sizeof(x));

		const std::intptr_t index = static_cast<std::int32_t>(y) * static_cast<std::int32_t>(width) + static_cast<std::int32_t>(x);
		const std::uintptr_t offset = index * std::uintptr_t{ 14 };

		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroPointStorage, sizeof(baseAddress));

		const std::uintptr_t address = baseAddress + offset;

		return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address));
	}

	// Original function: FUN_0042f6d5; distinct native micro-point forwarding entry.
	std::uint32_t MapMicroPointData_Continent_GetContinentTyp(const void* micro)
	{
		std::uint8_t continent;
		std::memcpy(&continent, static_cast<const std::byte*>(micro) + 6, sizeof(continent));

		return GameMap_Continent_GetContinentTyp(continent);
	}

	// Original function: FUN_0042385b.
	void* GameMap_GetMapMicroPointPtr(const void* packedMigPoint)
	{
		if (!MapMigPoint_IsInsideMap(packedMigPoint))
			return nullptr;

		return const_cast<void*>(GameMap_GetConstMapMicroPointPtr(packedMigPoint));
	}

	// Original function: FUN_00424cc1.
	std::int16_t GameMap_CalculationMap_GetValue(const void* packedMigPoint)
	{
		std::int16_t x;
		std::int16_t y;
		std::memcpy(&x, packedMigPoint, sizeof(x));
		std::memcpy(&y, static_cast<const std::byte*>(packedMigPoint) + 2, sizeof(y));
		const std::intptr_t index = static_cast<std::int32_t>(y) * static_cast<std::int32_t>(GameMap_GetWidth()) + static_cast<std::int32_t>(x);
		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::PackingScratchBuffer, sizeof(baseAddress));
		std::int16_t value;
		std::memcpy(&value, reinterpret_cast<const void*>(baseAddress + index * std::uintptr_t{ 2 }), sizeof(value));

		return value;
	}

	// Original function: FUN_00424ce2.
	void GameMap_CalculationMap_SetValue(const void* packedMigPoint, std::int16_t value)
	{
		std::int16_t x;
		std::int16_t y;
		std::memcpy(&x, packedMigPoint, sizeof(x));
		std::memcpy(&y, static_cast<const std::byte*>(packedMigPoint) + 2, sizeof(y));
		const std::intptr_t index = static_cast<std::int32_t>(y) * static_cast<std::int32_t>(GameMap_GetWidth()) + static_cast<std::int32_t>(x);
		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::PackingScratchBuffer, sizeof(baseAddress));
		std::memcpy(reinterpret_cast<void*>(baseAddress + index * std::uintptr_t{ 2 }), &value, sizeof(value));
	}

	// Original function: FUN_00424d09; zero-fill branch of FUN_0041095a.
	void GameMap_CalculationMap_Clear(const void* packedMigPoint, std::int32_t radius)
	{
		std::int16_t x;
		std::int16_t y;
		std::memcpy(&x, packedMigPoint, sizeof(x));
		std::memcpy(&y, static_cast<const std::byte*>(packedMigPoint) + 2, sizeof(y));
		const std::int32_t diameter = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(radius) * 2u);
		NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(static_cast<std::int32_t>(x)) - static_cast<std::uint32_t>(radius)), NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(static_cast<std::int32_t>(y)) - static_cast<std::uint32_t>(radius)), diameter, diameter);
		NXBasics::SRectangle bounds(0, 0, 0, 0);
		std::memcpy(&bounds, gGameMapStaticStorage + NC2Layout::GameMapOffset::MicroRectangle, sizeof(bounds));
		rectangle.CutInside(bounds);
		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::PackingScratchBuffer, sizeof(baseAddress));
		const std::uint32_t stride = static_cast<std::uint32_t>(static_cast<std::int32_t>(GameMap_GetWidth())) * 2u;
		std::uintptr_t address = baseAddress + static_cast<std::size_t>(rectangle.y) * stride + static_cast<std::size_t>(rectangle.x) * 2u;

		for (std::int32_t row = 0; row < rectangle.height; ++row)
		{
			std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), 0, static_cast<std::uint32_t>(rectangle.width) * 2u);
			address += stride;
		}
	}

	// Original function: FUN_00424d67; callers supply an already clipped rectangle.
	void GameMap_CalculationMap_Clear(const std::int16_t* rectangle)
	{
		std::uintptr_t baseAddress;
		std::memcpy(&baseAddress, gGameMapStaticStorage + NC2Layout::GameMapOffset::PackingScratchBuffer, sizeof(baseAddress));
		const auto stride = static_cast<std::int32_t>(GameMap_GetWidth());
		auto* values = reinterpret_cast<std::int16_t*>(static_cast<std::uintptr_t>(baseAddress)) + rectangle[1] * stride + rectangle[0];

		for (std::int32_t row = 0; row < rectangle[3]; ++row)
		{
			std::memset(values, 0, static_cast<std::uint32_t>(static_cast<std::int32_t>(rectangle[2])) * 2u);
			values += stride;
		}
	}

	// Original function: FUN_00452aec.
	std::uint32_t GameMap_Continent_GetContinentTyp(std::uint32_t continentId)
	{
		std::uint32_t count;
		std::memcpy(&count, gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, sizeof(count));

		if (continentId >= count)
			return 0;

		std::uint32_t type;
		std::memcpy(&type, gGameMapStaticStorage + NC2Layout::GameMapOffset::Continents + continentId * 0x10u, sizeof(type));

		return type;
	}

	// Original function: FUN_00423fb0
	std::uint8_t GameMap_GetHeightAtMapMigPoint(const void* packedMigPoint)
	{
		std::int16_t coordinates[2];
		std::memcpy(coordinates, packedMigPoint, sizeof(coordinates));

		std::int16_t macroWidth;
		std::int16_t macroHeight;

		std::memcpy(&macroWidth, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(macroWidth));
		std::memcpy(&macroHeight, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, sizeof(macroHeight));

		const auto floorHalf = [](std::int32_t value) -> std::int32_t
		{

			return value >= 0 ? value / 2 : -((-value + 1) / 2);
		};

		const std::uint32_t rowType = static_cast<std::uint16_t>(coordinates[1]) & 3u;
		std::int32_t adjustedX = coordinates[0];

		// Original coordinate conversion: FUN_0042a5f1.

		if (rowType == 1u)
		{
			++adjustedX;
		}
		else if (rowType == 2u)
		{
			--adjustedX;
		}

		std::int32_t macroX = floorHalf(adjustedX);
		std::int32_t macroY = floorHalf(coordinates[1]);

		// Original sub-position calculation: FUN_0042a632.
		std::uint32_t subX = static_cast<std::uint16_t>(coordinates[0]) & 1u;

		if (rowType == 1u || rowType == 2u)
			subX ^= 1u;

		const std::uint32_t subY = static_cast<std::uint16_t>(coordinates[1]) & 1u;

		while (macroX < 0)
			macroX += macroWidth;

		while (macroX >= macroWidth)
			macroX -= macroWidth;

		while (macroY < 0)
			macroY += macroHeight;

		while (macroY >= macroHeight)
			macroY -= macroHeight;

		const auto readHeight = [](std::int32_t x, std::int32_t y) -> std::uint8_t
		{
			const auto* point = static_cast<const std::uint8_t*>(GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y)));

			return point[2];
		};

		const std::uint8_t firstHeight = readHeight(macroX, macroY);
		const std::uint32_t subPosition = subX | (subY << 1);

		if (subPosition == 0)
			return firstHeight;

		if (subPosition == 1)
		{
			++macroX;

			if (macroX == macroWidth)
				macroX = 0;
		}
		else
		{
			const bool oddMacroRow = (static_cast<std::uint32_t>(macroY) & 1u) != 0;

			if (subPosition == 2 && !oddMacroRow)
			{
				--macroX;

				if (macroX < 0)
					macroX += macroWidth;
			}
			else if (subPosition == 3 && oddMacroRow)
			{
				++macroX;

				if (macroX == macroWidth)
					macroX = 0;
			}

			++macroY;

			if (macroY == macroHeight)
				macroY = 0;
		}

		const std::uint8_t secondHeight = readHeight(macroX, macroY);

		return static_cast<std::uint8_t>((static_cast<std::uint32_t>(firstHeight) + secondHeight) / 2u);
	}

	// Original function: FUN_004237F3; distinct coordinate overload with signed bounds.
	const void* GameMap_GetConstMapMicroPointPtr(std::int16_t x, std::int16_t y)
	{
		const auto width = GameMap_GetWidth();

		if (x < 0 || x >= width || y < 0 || y >= GameMap_GetHeight())
			return nullptr;

		const auto* base = NC2Lifetime::Read<const std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

		return base + static_cast<std::size_t>(y * width + x) * 14u;
	}

	// Original function: FUN_0042388D; distinct linear-index overload.
	void* GameMap_GetMapMicroPointPtr(std::uint32_t index)
	{
		if (index >= NC2Lifetime::Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount))
			return nullptr;

		return NC2Lifetime::Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage) + static_cast<std::size_t>(index) * 14u;
	}

	// Original function: FUN_00423EEC.
	void GameMap_MapMacroPoint_Height_Set(const void* point, std::int32_t height)
	{
		auto* macro = GameMap_GetMapMacroPointPtr(point);

		if (macro == nullptr || MapMagPoint_IsBorderPoint(point))
			return;

		if (height < 0)
			height = 0;

		if (static_cast<std::uint32_t>(height) > 250u)
			height = 250;

		if (NC2Lifetime::Read<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::Height) == height)
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::Height, static_cast<std::uint8_t>(height));
		GameMapModifyManager_GameMapModifiedMacro(13, point, 0);
	}

	// Original function: FUN_00423F4C; wrapped DWORD addition is interpreted as signed before clipping.
	void GameMap_MapMacroPoint_Height_Modify(const void* point, std::int32_t delta)
	{
		auto* macro = GameMap_GetMapMacroPointPtr(point);

		if (macro == nullptr || MapMagPoint_IsBorderPoint(point))
			return;

		const auto previous = NC2Lifetime::Read<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::Height);
		auto height = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + static_cast<std::uint32_t>(delta));

		if (height < 0)
			height = 0;

		if (static_cast<std::uint32_t>(height) > 250u)
			height = 250;

		if (previous == height)
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::Height, static_cast<std::uint8_t>(height));
		GameMapModifyManager_GameMapModifiedMacro(13, point, 0);
	}

}