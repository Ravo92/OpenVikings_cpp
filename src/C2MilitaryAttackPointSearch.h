#pragma once

#include "XNumericConstants.h"
#include <cstdint>

namespace NC2Logic
{
	// Windows shares this search record between human and animal attacks; the original type name is unresolved.
	struct SAttackPointSearch
	{
		std::uint32_t target;
		std::uint32_t source;
		std::uint32_t count;
		std::uint32_t points[6];
		std::uint32_t bestDistance;
		std::uint32_t minimumRange;
		std::uint32_t maximumRange;
		const void* owner;

		// Original function: FUN_00442fc7; range and owner fields are filled by the caller.
		SAttackPointSearch() : target(0), source(0), count(0), points{ }, bestDistance(NXBasics::IntegerRange::UInt32Maximum)
		{
		}
	};

	static_assert(sizeof(SAttackPointSearch) == (sizeof(void*) == 8 ? 56 : 52));
}