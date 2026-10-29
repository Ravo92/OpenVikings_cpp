#include "C2MapSectorGridCursor.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_004584c7
	SSectorGridCursor::SSectorGridCursor(std::uint32_t packedMigPoint)
	{
		const std::uint16_t xBits = static_cast<std::uint16_t>(packedMigPoint);
		const std::uint16_t yBits = static_cast<std::uint16_t>(packedMigPoint >> 16);

		std::int16_t pointX;
		std::int16_t pointY;
		std::memcpy(&pointX, &xBits, sizeof(pointX));
		std::memcpy(&pointY, &yBits, sizeof(pointY));

		x = static_cast<std::int32_t>(pointX) / 20;
		y = static_cast<std::int32_t>(pointY) / 20;
	}

	// Original function: FUN_004584f5; direct sector coordinates require no conversion or clamping.
	SSectorGridCursor::SSectorGridCursor(std::int32_t column, std::int32_t row) : x(column), y(row)
	{
	}

	// Original function: FUN_004585A4.
	bool SSectorGridCursor::IsInsideMap() const
	{

		return x < sGridWidth && x >= 0 && y < sGridHeight && y >= 0;
	}

	// Original function: FUN_00458572
	void SSectorGridCursor::CutInsideMap()
	{
		const std::int32_t width = sGridWidth;

		if (x >= width)
		{
			const std::uint32_t bits = static_cast<std::uint32_t>(width) - 1u;
			std::memcpy(&x, &bits, sizeof(x));
		}
		else if (x < 0)
		{
			x = 0;
		}

		const std::int32_t height = sGridHeight;

		if (y >= height)
		{
			const std::uint32_t bits = static_cast<std::uint32_t>(height) - 1u;
			std::memcpy(&y, &bits, sizeof(y));
		}
		else if (y < 0)
		{
			y = 0;
		}
	}

	// Original function: FUN_00458505; the eight entries were recovered from the Windows jump table.
	void SSectorGridCursor::MoveCursorUnwrapped(std::uint32_t direction)
	{
		if (direction > 7u)
		{

			return;
		}

		static constexpr std::int32_t deltaX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		static constexpr std::int32_t deltaY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		const std::uint32_t xBits = static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(deltaX[direction]);
		const std::uint32_t yBits = static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(deltaY[direction]);
		std::memcpy(&x, &xBits, sizeof(x));
		std::memcpy(&y, &yBits, sizeof(y));
	}

	// Original function: FUN_00458553.
	void SSectorGridCursor::MoveCursorUnwrapped(std::uint32_t direction, std::uint32_t distance)
	{
		while (distance != 0)
		{
			MoveCursorUnwrapped(direction);
			--distance;
		}
	}

	// Original function: FUN_0044FA93.
	SSectorGridCursor::SSectorGridCursor(const SSectorGridCursor& source) : x(source.x), y(source.y)
	{
	}

}