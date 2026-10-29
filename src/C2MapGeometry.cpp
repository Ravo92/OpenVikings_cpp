#include "C2GameMapAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_004478b5; both WORD coordinates are cleared and the destination is returned.
	// No separate Mac default-constructor symbol has been established.
	void* MapMigPoint_ConstructZero(void* packedMigPoint)
	{
		const std::int16_t zero = 0;
		std::memcpy(static_cast<std::byte*>(packedMigPoint) + 2, &zero, 2);
		std::memcpy(packedMigPoint, &zero, 2);

		return packedMigPoint;
	}

	// Original function: FUN_0042a98a; SetPosition returns the destination pointer.
	void* MapMigPoint_SetPosition(void* packedMigPoint, std::int16_t x, std::int16_t y, bool validate)
	{
		std::memcpy(packedMigPoint, &x, 2);
		std::memcpy(static_cast<std::byte*>(packedMigPoint) + 2, &y, 2);

		if (validate)
			MapMigPoint_WrapPosition(packedMigPoint);

		return packedMigPoint;
	}

	// Original function: FUN_0042aa34; coordinate constructor uses wrapping validation.
	void MapMigPoint_Construct(void* packedMigPoint, std::int16_t x, std::int16_t y, bool validate)
	{
		std::memcpy(packedMigPoint, &x, 2);
		std::memcpy(static_cast<std::byte*>(packedMigPoint) + 2, &y, 2);

		if (validate)
			MapMigPoint_WrapPosition(packedMigPoint);
	}

	// Original function: FUN_0042ad91; odd vertical offsets correct odd source rows.
	void MapMigPoint_AddOffset(void* point, std::int16_t x, std::int16_t y)
	{
		auto* coordinates = static_cast<std::int16_t*>(point);
		coordinates[0] = static_cast<std::int16_t>(static_cast<std::uint16_t>(coordinates[0]) + static_cast<std::uint16_t>(x));

		if ((static_cast<std::uint16_t>(y) & 1u) != 0 && (static_cast<std::uint16_t>(coordinates[1]) & 1u) != 0)
			coordinates[0] = static_cast<std::int16_t>(static_cast<std::uint16_t>(coordinates[0]) + 1u);

		coordinates[1] = static_cast<std::int16_t>(static_cast<std::uint16_t>(coordinates[1]) + static_cast<std::uint16_t>(y));
	}

	// Original function: FUN_0042a894; retain Windows atan2 arguments and scaling order.
	std::int32_t VE_HexagonDirection_GetDirectionFromPositions360Degrees(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY)
	{
		const auto dx = static_cast<std::int32_t>(static_cast<std::int16_t>(targetX * 2 + (targetY & 1) - sourceX * 2 - (sourceY & 1)));
		const auto dy = static_cast<std::int32_t>(static_cast<std::int16_t>(targetY * 2 - sourceY * 2));

		if (dy == 0)
			return dx >= 0 ? 90 : 270;
		// Complete numeric tail of Windows region 004F3A70..004F3A87.
		const double scale = 1000.0;
		const double pi = 3.1415926535;
		const double degrees = 180.0;
		const double inverseScale = -0.001;
		const double scaledAngle = (((std::atan2(static_cast<double>(dy), static_cast<double>(dx)) * scale) / pi) * degrees) * inverseScale;
		const std::int64_t result = NXBasics::Runtime_TruncateDoubleToInt64(scaledAngle);
		auto angle = (90 - static_cast<std::int32_t>(result)) % 360;

		return angle < 0 ? angle + 360 : angle;
	}

	// Original function: FUN_0042a84a; complete direction tables 004F3A38..004F3A6F.
	std::uint32_t VE_HexagonDirection_GetDirectionFromPositions(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY, bool includeVertical)
	{
		static constexpr std::uint32_t directions8[] = { 6, 5, 0, 1, 7, 2, 3, 4 };
		static constexpr std::uint32_t directions6[] = { 5, 0, 1, 2, 3, 4 };
		const auto angle = static_cast<std::uint32_t>(VE_HexagonDirection_GetDirectionFromPositions360Degrees(sourceX, sourceY, targetX, targetY));

		return includeVertical ? directions8[((angle + 22u) / 45u) & 7u] : directions6[(angle / 60u) % 6u];
	}

	// Original function: FUN_0042a6b7; directions 6/7 bridge the two vertical turns.
	std::int32_t VE_HexagonDirection_DirectionToTurn(std::uint32_t source, std::uint32_t target, bool includeVertical)
	{
		if (source == target)
			return 0;

		if (includeVertical)
		{
			if (source == 6)
				return target == 2 || target == 3 || target == 4 ? -1 : 1;

			if (source == 7)
				return target == 2 || target == 3 || target == 4 ? 1 : -1;

			if (target == 6)
				return source <= 1 || source == 5 ? -1 : 1;

			if (target == 7)
				return source <= 1 || source == 5 ? 1 : -1;
		}

		if (source == 6 || source == 7 || target == 6 || target == 7)
			return 0;

		const auto difference = NXBasics::Runtime_TimeSigned32(target - source);

		if (difference >= 0)
			return difference <= 3 ? difference : NXBasics::Runtime_TimeSigned32(target - source - 6u);

		return difference >= -3 ? difference : NXBasics::Runtime_TimeSigned32(target - source + 6u);
	}

	// Original function: FUN_0042a753.
	std::uint32_t VE_HexagonDirection_NextDirection(std::uint32_t source, std::uint32_t target, bool includeVertical)
	{
		if (source == target)
			return source;

		if (includeVertical)
		{
			if (source == 6)
				return target == 2 || target == 3 || target == 4 ? 4u : 5u;

			if (source == 7)
				return target == 2 || target == 3 || target == 4 ? 2u : 1u;

			if (target == 6)
			{
				if (source == 0)
					return 5;

				if (source == 1)
					return 0;

				if (source == 2)
					return 3;

				if (source == 3)
					return 4;

				return 6;
			}

			if (target == 7)
			{
				if (source == 0)
					return 1;

				if (source == 3)
					return 2;

				if (source == 4)
					return 3;

				if (source == 5)
					return 0;

				return 7;
			}
		}

		if (source == 6 || source == 7 || target == 6 || target == 7)
			return target;

		const bool counterclockwise = VE_HexagonDirection_DirectionToTurn(source, target, false) < 0;
		auto next = counterclockwise ? (source != 0 ? source - 1u : 5u) : (source + 1u == 6 ? 0u : source + 1u);

		if (includeVertical)
		{
			if (counterclockwise)
			{
				if (next == 4)
				{
					next = 6;
				}
				else if (next == 1)
				{
					next = 7;
				}
			}
			else
			{
				if (next == 5)
				{
					next = 6;
				}
				else if (next == 2)
				{
					next = 7;
				}
			}
		}

		return next;
	}

	// Original function: FUN_0042ae0c; inclusive rectangle ends wrap as WORDs.
	bool MapMigPoint_IsInside(const void* packedMigPoint, const std::int16_t* rectangle)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMigPoint, sizeof(point));

		for (unsigned axis = 0; axis < 2; ++axis)
		{
			const auto end = static_cast<std::int16_t>(rectangle[axis] + rectangle[axis + 2] - 1);

			if (point[axis] < rectangle[axis] || point[axis] > end)
				return false;
		}

		return true;
	}

	// Original function: FUN_0042b40f.
	std::uint32_t VE_MicroPoint_GetMicroPointCount(std::uint32_t radius)
	{

		return (radius + 1u) * radius * 3u + 1u;
	}

	// Original function: FUN_0042af01; signed WORD endpoints wrap before comparison.
	bool MapMigRectangle_IsTouching(const std::int16_t* rectangle, const std::int16_t* other)
	{
		for (unsigned axis = 0; axis < 2; ++axis)
		{
			const auto end = static_cast<std::int16_t>(rectangle[axis] + rectangle[axis + 2] - 1);
			const auto otherEnd = static_cast<std::int16_t>(other[axis] + other[axis + 2] - 1);

			if (static_cast<std::int32_t>(end) + 1 <= other[axis] || static_cast<std::int32_t>(otherEnd) + 1 <= rectangle[axis])
				return false;
		}

		return true;
	}

	// Original function: FUN_0042b3a7.
	void MapMagRectangle_CutInsideMap(std::int16_t* rectangle)
	{
		for (unsigned axis = 0; axis < 2; ++axis)
		{
			if (rectangle[axis] < 0)
			{
				rectangle[axis + 2] = static_cast<std::int16_t>(rectangle[axis + 2] + rectangle[axis]);
				rectangle[axis] = 0;
			}

			const auto limit = axis == 0 ? gMapPointDimensions.macroWidth : gMapPointDimensions.macroHeight;

			if (rectangle[axis] + rectangle[axis + 2] > limit)
				rectangle[axis + 2] = static_cast<std::int16_t>(limit - rectangle[axis]);
		}
	}

	// Original function: FUN_0042b376.
	void MapMagRectangle_Construct(std::int16_t* rectangle, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height)
	{
		rectangle[0] = x;
		rectangle[1] = y;
		rectangle[2] = width;
		rectangle[3] = height;
		MapMagRectangle_CutInsideMap(rectangle);
	}

	// Original function: FUN_0042ac72; distance is an unchecked unsigned loop count.
	void MapMigPoint_MoveCursorUnwrapped(void* packedMigPoint, std::uint32_t direction, std::uint32_t distance)
	{
		while (distance-- != 0)
		{
			std::uint32_t point;
			std::memcpy(&point, packedMigPoint, sizeof(point));
			MapMigPoint_MoveCursorUnwrapped(point, direction);
			std::memcpy(packedMigPoint, &point, sizeof(point));
		}
	}

	// Original function: FUN_00423761.
	bool MapMagPoint_IsInsideMap(const void* packedMagPoint)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMagPoint, sizeof(point));

		return point[1] >= 0 && point[1] < gMapPointDimensions.macroHeight && point[0] >= 0 && point[0] < gMapPointDimensions.macroWidth;
	}

	// Original function: FUN_0042b33b.
	bool MapMagPoint_IsBorderPoint(const void* packedMagPoint)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMagPoint, sizeof(point));

		return point[0] < 2 || point[0] >= static_cast<std::int32_t>(gMapPointDimensions.macroWidth) - 2 || point[1] < 2 || point[1] >= static_cast<std::int32_t>(gMapPointDimensions.macroHeight) - 2;
	}

	// Original function: FUN_0042ad4e.
	bool MapMigPoint_IsBorderPoint(const void* packedMigPoint)
	{
		std::int16_t point[2], macro[2];
		std::memcpy(point, packedMigPoint, sizeof(point));
		VE_MapCoords_MicroGridMapToMacroGridMapPoint(point[0], point[1], &macro[0], &macro[1]);

		return MapMagPoint_IsBorderPoint(macro);
	}

	static std::int16_t WrapCoordinate(std::int16_t coordinate, std::int16_t dimension)
	{
		if (coordinate >= dimension)
			return static_cast<std::int16_t>(coordinate % dimension);

		if (coordinate < 0)
		{
			const auto result = static_cast<std::int16_t>(dimension - (-static_cast<std::int32_t>(coordinate) % dimension));
			return result == dimension ? 0 : result;
		}

		return coordinate;
	}

	// Original function: FUN_0042a632.
	void VE_MapCoords_GetMicroGridMapOffset(std::int16_t x, std::int16_t y, std::int16_t* offsetX, std::int16_t* offsetY)
	{
		*offsetX = static_cast<std::int16_t>((x - ((static_cast<std::uint16_t>(y) & 3u) == 1 || (static_cast<std::uint16_t>(y) & 3u) == 2 ? 1 : 0)) & 1);
		*offsetY = static_cast<std::int16_t>(y & 1);
	}

	// Original function: FUN_0042a670; preserve the four low-bit alignment cases and WORD results.
	void VE_MapCoords_SnapMicroGridToMacroGridMapPoint(std::int16_t x, std::int16_t y, std::int16_t* resultX, std::int16_t* resultY)
	{
		std::int32_t alignedX = x;

		switch (static_cast<std::uint16_t>(y) & 3u)
		{
			case SwitchIds::GridRowAlignment::AlignDownEven:
				alignedX &= ~1;
				break;
			case SwitchIds::GridRowAlignment::AlignUpEven:
				alignedX = (alignedX + 1) & ~1;
				break;
			case SwitchIds::GridRowAlignment::AlignDownOdd:
				alignedX = ((alignedX - 1) & ~1) + 1;
				break;
			case SwitchIds::GridRowAlignment::AlignUpOdd:
				alignedX = (alignedX & ~1) + 1;
				break;
		}

		*resultX = static_cast<std::int16_t>(alignedX);
		*resultY = static_cast<std::int16_t>(y & ~1);
	}

	// Original function: FUN_0042ad79.
	void MapMigPoint_MakeMacroGridAligned(void* point)
	{
		auto* coordinates = static_cast<std::int16_t*>(point);
		VE_MapCoords_SnapMicroGridToMacroGridMapPoint(coordinates[0], coordinates[1], coordinates, coordinates + 1);
	}

	// Original function: FUN_0042adb8.
	bool MapMigPoint_IsOnMacroGrid(const void* point)
	{
		std::int16_t coordinates[2], offsets[2];
		std::memcpy(coordinates, point, sizeof(coordinates));
		VE_MapCoords_GetMicroGridMapOffset(coordinates[0], coordinates[1], offsets, offsets + 1);

		return offsets[0] == 0 && offsets[1] == 0;
	}

	// Original function: FUN_0042a9b0.
	void* MapMigPoint_ConstructFromMag(void* packedMigPoint, const void* packedMagPoint, bool validate)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMagPoint, sizeof(point));
		const std::int16_t result[2] = { static_cast<std::int16_t>(point[0] * 2 + (point[1] & 1)), static_cast<std::int16_t>(point[1] * 2) };
		std::memcpy(packedMigPoint, result, sizeof(result));

		if (validate)
			MapMigPoint_WrapPosition(packedMigPoint);

		return packedMigPoint;
	}

	// Original function: FUN_0042aa54.
	bool MapMigPoint_MoveCursorWrapped(void* packedMigPoint, std::uint32_t direction)
	{
		if (direction > 7)
			return false;

		std::uint32_t point;
		std::memcpy(&point, packedMigPoint, sizeof(point));
		MapMigPoint_MoveCursorUnwrapped(point, direction);
		std::int16_t coordinates[2];
		std::memcpy(coordinates, &point, sizeof(point));
		bool wrapped = false;

		if (direction < 6)
		{
			if (coordinates[0] == gMapPointDimensions.microWidth)
			{
				coordinates[0] = 0;
				wrapped = true;
			}
			else if (coordinates[0] < 0)
			{
				coordinates[0] = static_cast<std::int16_t>(gMapPointDimensions.microWidth - 1);
				wrapped = true;
			}

			if (coordinates[1] == gMapPointDimensions.microHeight)
			{
				coordinates[1] = 0;
				wrapped = true;
			}
			else if (coordinates[1] < 0)
			{
				coordinates[1] = static_cast<std::int16_t>(gMapPointDimensions.microHeight - 1);
				wrapped = true;
			}
		}
		else if (direction == 6 && coordinates[1] < 0)
		{
			coordinates[1] = static_cast<std::int16_t>(coordinates[1] + gMapPointDimensions.microHeight);
			wrapped = true;
		}
		else if (direction == 7 && coordinates[1] >= gMapPointDimensions.microHeight)
		{
			coordinates[1] = static_cast<std::int16_t>(coordinates[1] - gMapPointDimensions.microHeight);
			wrapped = true;
		}

		std::memcpy(packedMigPoint, coordinates, sizeof(coordinates));

		return wrapped;
	}

	// Original function: FUN_0042afcb.
	void MapMagPoint_MoveCursorWrapped(void* packedMagPoint, std::uint32_t direction)
	{
		if (direction > 7)
		{

			return;
		}

		MapMagPoint_MoveCursorUnwrapped(packedMagPoint, direction);
		std::int16_t coordinates[2];
		std::memcpy(coordinates, packedMagPoint, sizeof(coordinates));

		if (coordinates[0] == gMapPointDimensions.macroWidth)
		{
			coordinates[0] = 0;
		}
		else if (coordinates[0] < 0)
		{
			coordinates[0] = static_cast<std::int16_t>(gMapPointDimensions.macroWidth - 1);
		}

		if (direction < 6)
		{
			if (coordinates[1] == gMapPointDimensions.macroHeight)
			{
				coordinates[1] = 0;
			}
			else if (coordinates[1] < 0)
			{
				coordinates[1] = static_cast<std::int16_t>(gMapPointDimensions.macroHeight - 1);
			}
		}
		else if (direction == 6 && coordinates[1] < 0)
		{
			coordinates[1] = static_cast<std::int16_t>(coordinates[1] + gMapPointDimensions.macroHeight);
		}
		else if (direction == 7 && coordinates[1] >= gMapPointDimensions.macroHeight)
		{
			coordinates[1] = static_cast<std::int16_t>(coordinates[1] - gMapPointDimensions.macroHeight);
		}

		std::memcpy(packedMagPoint, coordinates, sizeof(coordinates));
	}

	// Original function: FUN_0042b128.
	void MapMagPoint_MoveCursorWrappedEast(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		*point = static_cast<std::int16_t>(*point + 1);

		if (*point == gMapPointDimensions.macroWidth)
			*point = 0;
	}

	// Original function: FUN_0042b13c.
	void MapMagPoint_MoveCursorWrappedWest(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		*point = static_cast<std::int16_t>(*point - 1);

		if (*point < 0)
			*point = static_cast<std::int16_t>(gMapPointDimensions.macroWidth - 1);
	}

	// Original function: FUN_0042b14f.
	void MapMagPoint_MoveCursorUnwrapped(void* packedMagPoint, std::uint32_t direction)
	{
		std::uint32_t point;
		std::memcpy(&point, packedMagPoint, sizeof(point));
		MapMigPoint_MoveCursorUnwrapped(point, direction);
		std::memcpy(packedMagPoint, &point, sizeof(point));
	}

	// Original function: FUN_0042b1e2.
	void MapMagPoint_MoveCursorUnwrappedEast(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		*point = static_cast<std::int16_t>(*point + 1);
	}

	// Original function: FUN_0042b1e6.
	void MapMagPoint_MoveCursorUnwrappedWest(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		*point = static_cast<std::int16_t>(*point - 1);
	}

	// Original function: FUN_0042b271.
	void MapMagPoint_ValidateXPosition(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		*point = WrapCoordinate(*point, gMapPointDimensions.macroWidth);
	}

	// Original function: FUN_0042b2b4.
	void MapMagPoint_ValidateYPosition(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);
		point[1] = WrapCoordinate(point[1], gMapPointDimensions.macroHeight);
	}

	// Original function: FUN_0042b2fd.
	void MapMagPoint_CutInsideMap(void* packedMagPoint)
	{
		auto* point = static_cast<std::int16_t*>(packedMagPoint);

		if (point[0] < 0)
		{
			point[0] = 0;
		}
		else if (point[0] >= gMapPointDimensions.macroWidth)
		{
			point[0] = static_cast<std::int16_t>(gMapPointDimensions.macroWidth - 1);
		}

		if (point[1] < 0)
		{
			point[1] = 0;
		}
		else if (point[1] >= gMapPointDimensions.macroHeight)
		{
			point[1] = static_cast<std::int16_t>(gMapPointDimensions.macroHeight - 1);
		}
	}

	// Original function: FUN_0042a5f1.
	void VE_MapCoords_MicroGridMapToMacroGridMapPoint(std::int16_t x, std::int16_t y, std::int16_t* macroX, std::int16_t* macroY)
	{
		const auto row = static_cast<std::uint16_t>(y) & 3u;
		*macroX = static_cast<std::int16_t>((static_cast<std::int32_t>(x) + (row == 1 ? 1 : row == 2 ? -1 : 0)) >> 1);
		*macroY = static_cast<std::int16_t>(y >> 1);
	}

	// Original function: FUN_0042ac91; SMapMigPoint::ValidatePosition wraps coordinates.
	void MapMigPoint_WrapPosition(void* packedMigPoint)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMigPoint, sizeof(point));
		point[0] = WrapCoordinate(point[0], gMapPointDimensions.microWidth);
		point[1] = WrapCoordinate(point[1], gMapPointDimensions.microHeight);
		std::memcpy(packedMigPoint, point, sizeof(point));
	}

	// Original function: FUN_0042b1ea; SMapMagPoint::ValidatePosition.
	void MapMagPoint_ValidatePosition(void* packedMagPoint)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMagPoint, sizeof(point));
		point[0] = WrapCoordinate(point[0], gMapPointDimensions.macroWidth);
		point[1] = WrapCoordinate(point[1], gMapPointDimensions.macroHeight);
		std::memcpy(packedMagPoint, point, sizeof(point));
	}

	// Original function: FUN_0042af98.
	void* MapMagPoint_ConstructFromMig(void* packedMagPoint, const void* packedMigPoint, bool validate)
	{
		std::int16_t point[2];
		std::memcpy(point, packedMigPoint, sizeof(point));
		VE_MapCoords_MicroGridMapToMacroGridMapPoint(point[0], point[1], static_cast<std::int16_t*>(packedMagPoint), reinterpret_cast<std::int16_t*>(static_cast<std::byte*>(packedMagPoint) + 2));

		if (validate)
			MapMagPoint_ValidatePosition(packedMagPoint);

		return packedMagPoint;
	}

	// Original function: FUN_0042a9e7.
	void* MapMigPoint_ConstructOffset(void* packedMigPoint, const void* source, std::int16_t xOffset, std::int16_t yOffset, bool validate)
	{
		std::uint16_t point[2];
		std::memcpy(point, source, sizeof(point));
		point[0] = static_cast<std::uint16_t>(point[0] + static_cast<std::uint16_t>(xOffset));

		if ((static_cast<std::uint16_t>(yOffset) & 1u) != 0 && (point[1] & 1u) != 0)
			++point[0];

		point[1] = static_cast<std::uint16_t>(point[1] + static_cast<std::uint16_t>(yOffset));
		std::memcpy(packedMigPoint, point, sizeof(point));

		if (validate)
			MapMigPoint_WrapPosition(packedMigPoint);

		return packedMigPoint;
	}

	// Original function: FUN_0042adec; the argument is a packed DWORD, not a pointer.
	void MapMigPoint_AssignLong(void* packedMigPoint, std::uint32_t value)
	{
		const std::uint16_t x = static_cast<std::uint16_t>(value);
		const std::uint16_t y = static_cast<std::uint16_t>(value >> 16);
		std::memcpy(packedMigPoint, &x, sizeof(x));
		std::memcpy(static_cast<std::byte*>(packedMigPoint) + 2, &y, sizeof(y));
	}

	// Original function: FUN_0042a83e; complete eight-entry table at 004f3a18.
	std::uint32_t VE_HexagonDirection_OppositeDirection(std::uint32_t direction)
	{
		static constexpr std::uint32_t oppositeDirections[8] = { 3, 4, 5, 0, 1, 2, 7, 6 };

		return oppositeDirections[direction];
	}

	// Original function: FUN_0042adff; sign-extended X participates in the OR.
	std::uint32_t MapMigPoint_GetLong(const void* packedMigPoint)
	{
		std::int16_t coordinates[2];
		std::memcpy(coordinates, packedMigPoint, sizeof(coordinates));

		return static_cast<std::uint32_t>(coordinates[0]) | (static_cast<std::uint32_t>(coordinates[1]) << 16);
	}

	// Original function: FUN_0042abdd; distinct jump tables for odd and even rows.
	void MapMigPoint_MoveCursorUnwrapped(std::uint32_t& packedMigPoint, std::uint32_t direction)
	{
		if (direction > 7u)
		{

			return;
		}

		const bool oddRow = (packedMigPoint & NXBasics::BitMask::Bit16) != 0;
		static constexpr std::int32_t oddDeltaX[8] = { 1, 1, 0, -1, 0, 1, 0, 0 };
		static constexpr std::int32_t evenDeltaX[8] = { 1, 0, -1, -1, -1, 0, 0, 0 };
		static constexpr std::int32_t deltaY[8] = { 0, 1, 1, 0, -1, -1, -2, 2 };
		const std::uint16_t x = static_cast<std::uint16_t>(packedMigPoint + static_cast<std::uint32_t>(oddRow ? oddDeltaX[direction] : evenDeltaX[direction]));
		const std::uint16_t y = static_cast<std::uint16_t>((packedMigPoint >> 16) + static_cast<std::uint32_t>(deltaY[direction]));
		packedMigPoint = static_cast<std::uint32_t>(x) | (static_cast<std::uint32_t>(y) << 16);
	}

	// Original function: FUN_0042a935
	std::int32_t VE_HexagonDirection_GetDistance(std::int16_t sourceX, std::int16_t sourceY, std::int16_t targetX, std::int16_t targetY)
	{
		const auto signedLowWord = [](std::int32_t value) -> std::int32_t
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(value);
			std::int16_t result;
			std::memcpy(&result, &bits, sizeof(result));

			return result;
		};

		const std::int32_t deltaX = static_cast<std::int32_t>(targetX) - sourceX;
		const std::int32_t deltaY = static_cast<std::int32_t>(targetY) - sourceY;
		const std::int32_t signedDeltaX = signedLowWord(deltaX);

		std::int32_t horizontal = signedDeltaX < 0 ? -deltaX : deltaX;
		std::int32_t vertical = signedLowWord(deltaY) < 0 ? -deltaY : deltaY;

		if ((static_cast<std::uint32_t>(vertical) & 1u) != 0)
		{
			const bool targetRowIsOdd = (static_cast<std::uint16_t>(targetY) & 1u) != 0;

			if ((targetRowIsOdd && signedDeltaX < 0) || (!targetRowIsOdd && signedDeltaX > 0))
				--horizontal;
		}

		vertical = signedLowWord(vertical);
		horizontal = signedLowWord(horizontal);

		const std::int32_t halfVertical = vertical / 2;

		if (horizontal > halfVertical)
			return vertical + horizontal - halfVertical;

		return vertical;
	}

	// Original function: FUN_0042ae99
	void MapMigRectangle_CutInsideMap(std::int16_t* rectangle)
	{
		const auto storeShort = [](std::int16_t& destination, std::int32_t value)
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(value);
			std::memcpy(&destination, &bits, sizeof(destination));
		};

		if (rectangle[0] < 0)
		{
			storeShort(rectangle[2], static_cast<std::int32_t>(rectangle[2]) + rectangle[0]);
			rectangle[0] = 0;
		}

		if (rectangle[1] < 0)
		{
			storeShort(rectangle[3], static_cast<std::int32_t>(rectangle[3]) + rectangle[1]);
			rectangle[1] = 0;
		}

		const std::int32_t right = static_cast<std::int32_t>(rectangle[0]) + rectangle[2];
		const std::int32_t mapWidth = GameMap_GetWidth();

		if (right > mapWidth)
			storeShort(rectangle[2], mapWidth - rectangle[0]);

		const std::int32_t bottom = static_cast<std::int32_t>(rectangle[1]) + rectangle[3];
		const std::int32_t mapHeight = GameMap_GetHeight();

		if (bottom > mapHeight)
			storeShort(rectangle[3], mapHeight - rectangle[1]);
	}

	// Original function: FUN_0040b580
	bool MapMigPoint_IsInsideMap(const void* packedMigPoint)
	{
		std::int16_t coordinates[2];
		std::memcpy(coordinates, packedMigPoint, sizeof(coordinates));

		if (coordinates[1] < 0 || coordinates[1] >= GameMap_GetHeight())
			return false;

		if (coordinates[0] < 0 || coordinates[0] >= GameMap_GetWidth())
			return false;

		return true;
	}

	// Original function: FUN_0042ad18
	void MapMigPoint_ValidatePosition(void* packedMigPoint)
	{
		std::int16_t coordinates[2];
		std::memcpy(coordinates, packedMigPoint, sizeof(coordinates));

		const auto storeLowWord = [](std::int16_t& destination, std::int32_t value)
		{
			const std::uint16_t bits = static_cast<std::uint16_t>(value);
			std::memcpy(&destination, &bits, sizeof(destination));
		};

		if (coordinates[0] < 0)
			coordinates[0] = 0;

		const std::int16_t mapWidth = GameMap_GetWidth();

		if (coordinates[0] >= mapWidth)
			storeLowWord(coordinates[0], static_cast<std::int32_t>(mapWidth) - 1);

		if (coordinates[1] < 0)
			coordinates[1] = 0;

		const std::int16_t mapHeight = GameMap_GetHeight();

		if (coordinates[1] >= mapHeight)
			storeLowWord(coordinates[1], static_cast<std::int32_t>(mapHeight) - 1);

		std::memcpy(packedMigPoint, coordinates, sizeof(coordinates));
	}

	// Original function: FUN_0042ABAC; PE bytes resolve an overlapping disassembly line.
	bool MapMigPoint_MoveCursorWrappedEast(void* point)
	{
		auto* x = static_cast<std::int16_t*>(point);
		*x = static_cast<std::int16_t>(*x + 1);

		if (*x != gMapPointDimensions.microWidth)
			return false;

		*x = 0;

		return true;
	}

	// Original function: FUN_0042ABC5.
	bool MapMigPoint_MoveCursorWrappedWest(void* point)
	{
		auto* x = static_cast<std::int16_t*>(point);
		*x = static_cast<std::int16_t>(*x - 1);

		if (*x >= 0)
			return false;

		*x = static_cast<std::int16_t>(static_cast<std::uint32_t>(gMapPointDimensions.microWidth) - 1u);

		return true;
	}

	// Original function: FUN_0042AE54.
	void* MapMigRectangle_ConstructZero(std::int16_t* rectangle)
	{
		rectangle[3] = 0;
		rectangle[2] = 0;
		rectangle[1] = 0;
		rectangle[0] = 0;

		return rectangle;
	}

	// Original function: FUN_0042AE68.
	void* MapMigRectangle_Construct(std::int16_t* rectangle, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height)
	{
		rectangle[0] = x;
		rectangle[1] = y;
		rectangle[2] = width;
		rectangle[3] = height;
		MapMigRectangle_CutInsideMap(rectangle);

		return rectangle;
	}

	// Original function: FUN_0042AF72.
	void* MapMagPoint_Construct(void* point, std::int16_t x, std::int16_t y, bool validate)
	{
		std::memcpy(point, &x, 2);
		std::memcpy(static_cast<std::byte*>(point) + 2, &y, 2);

		if (validate)
			MapMagPoint_ValidatePosition(point);

		return point;
	}

}