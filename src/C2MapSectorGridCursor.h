#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SSectorGridCursor
	{
		// Original data: DAT_00554cf0; shared sector-grid width.
		inline static std::int32_t sGridWidth = 0;

		// Original data: DAT_00554cec; shared sector-grid height.
		inline static std::int32_t sGridHeight = 0;

		SSectorGridCursor() = default;
		SSectorGridCursor(const SSectorGridCursor& source);
		explicit SSectorGridCursor(std::uint32_t packedMigPoint);
		SSectorGridCursor(std::int32_t column, std::int32_t row);

		bool IsInsideMap() const;
		void CutInsideMap();
		void MoveCursorUnwrapped(std::uint32_t direction);
		void MoveCursorUnwrapped(std::uint32_t direction, std::uint32_t distance);

		std::int32_t x; // +0x00: sector column.
		std::int32_t y; // +0x04: sector row.
	};

	static_assert(sizeof(SSectorGridCursor) == 8);
	static_assert(offsetof(SSectorGridCursor, y) == 4);
}