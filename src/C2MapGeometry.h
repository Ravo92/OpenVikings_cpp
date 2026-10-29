#pragma once

#include <cstdint>

namespace NC2Logic
{
	// Shared physical coordinate pair; this model does not convert between grids.
	struct SMapCoordinatePairStorage
	{
		std::int16_t x, y;
	};

	void* MapMigPoint_ConstructZero(void* packedMigPoint);
	bool MapMigPoint_MoveCursorWrappedEast(void* point);
	bool MapMigPoint_MoveCursorWrappedWest(void* point);
	void* MapMagPoint_Construct(void* point, std::int16_t x, std::int16_t y, bool validate);
	void* MapMigRectangle_ConstructZero(std::int16_t* rectangle);
	void* MapMigRectangle_Construct(std::int16_t* rectangle, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);
	std::uint32_t VE_MicroPoint_GetMicroPointCount(std::uint32_t radius);
	std::uint32_t VE_HexagonDirection_OppositeDirection(std::uint32_t direction);
	std::int32_t VE_HexagonDirection_DirectionToTurn(std::uint32_t source, std::uint32_t target, bool includeVertical);
	std::uint32_t VE_HexagonDirection_NextDirection(std::uint32_t source, std::uint32_t target, bool includeVertical);
	std::int32_t VE_HexagonDirection_GetDistance(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY);
	std::int32_t VE_HexagonDirection_GetDirectionFromPositions360Degrees(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY);
	std::uint32_t VE_HexagonDirection_GetDirectionFromPositions(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY, bool includeVertical);
	void MapMigRectangle_CutInsideMap(std::int16_t* rectangle);
	bool MapMigRectangle_IsTouching(const std::int16_t* rectangle, const std::int16_t* other);
	void MapMagRectangle_CutInsideMap(std::int16_t* rectangle);
	void MapMagRectangle_Construct(std::int16_t* rectangle, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);

	// Original function: FUN_0040b580
	bool MapMigPoint_IsInsideMap(const void* packedMigPoint);
	bool MapMigPoint_IsInside(const void* packedMigPoint, const std::int16_t* rectangle);
	bool MapMagPoint_IsInsideMap(const void* packedMagPoint);
	bool MapMagPoint_IsBorderPoint(const void* packedMagPoint);
	bool MapMigPoint_IsBorderPoint(const void* packedMigPoint);

	// Original function: FUN_0042ad18
	void MapMigPoint_ValidatePosition(void* packedMigPoint);
	void MapMigPoint_WrapPosition(void* packedMigPoint);
	void MapMagPoint_ValidatePosition(void* packedMagPoint);
	void MapMagPoint_ValidateXPosition(void* packedMagPoint);
	void MapMagPoint_ValidateYPosition(void* packedMagPoint);
	void MapMagPoint_CutInsideMap(void* packedMagPoint);
	void* MapMigPoint_ConstructFromMag(void* packedMigPoint, const void* packedMagPoint, bool validate);
	void MapMigPoint_Construct(void* packedMigPoint, std::int16_t x, std::int16_t y, bool validate);
	void* MapMigPoint_SetPosition(void* packedMigPoint, std::int16_t x, std::int16_t y, bool validate);
	bool MapMigPoint_MoveCursorWrapped(void* packedMigPoint, std::uint32_t direction);
	void MapMagPoint_MoveCursorWrapped(void* packedMagPoint, std::uint32_t direction);
	void MapMagPoint_MoveCursorUnwrapped(void* packedMagPoint, std::uint32_t direction);
	void MapMagPoint_MoveCursorWrappedEast(void* packedMagPoint);
	void MapMagPoint_MoveCursorWrappedWest(void* packedMagPoint);
	void MapMagPoint_MoveCursorUnwrappedEast(void* packedMagPoint);
	void MapMagPoint_MoveCursorUnwrappedWest(void* packedMagPoint);
	void VE_MapCoords_GetMicroGridMapOffset(std::int16_t x, std::int16_t y, std::int16_t* offsetX, std::int16_t* offsetY);
	void VE_MapCoords_SnapMicroGridToMacroGridMapPoint(std::int16_t x, std::int16_t y, std::int16_t* resultX, std::int16_t* resultY);
	void MapMigPoint_MakeMacroGridAligned(void* point);
	bool MapMigPoint_IsOnMacroGrid(const void* point);
	void VE_MapCoords_MicroGridMapToMacroGridMapPoint(std::int16_t x, std::int16_t y, std::int16_t* macroX, std::int16_t* macroY);
	void* MapMagPoint_ConstructFromMig(void* packedMagPoint, const void* packedMigPoint, bool validate);
	void* MapMigPoint_ConstructOffset(void* packedMigPoint, const void* source, std::int16_t xOffset, std::int16_t yOffset, bool validate);
	std::uint32_t MapMigPoint_GetLong(const void* packedMigPoint);
	void MapMigPoint_AssignLong(void* packedMigPoint, std::uint32_t value);
	void MapMigPoint_AddOffset(void* point, std::int16_t x, std::int16_t y);
	void MapMigPoint_MoveCursorUnwrapped(std::uint32_t& packedMigPoint, std::uint32_t direction);
	void MapMigPoint_MoveCursorUnwrapped(void* packedMigPoint, std::uint32_t direction, std::uint32_t distance);
}