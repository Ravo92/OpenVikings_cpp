#pragma once

#include "C2AnimalIterator.h"

namespace NC2Logic
{
	// Windows shares the constructor and 2C-byte layout with CAnimalIterator.
	class CHumanIterator : private CAnimalIterator
	{
	  public:
		CHumanIterator() = default;
		std::int32_t GetFirstHumanId();
		std::int32_t GetFirstHumanId(std::uint32_t player);
		std::int32_t GetFirstHumanId(std::uint32_t player, const void* point, std::int32_t radius);
		std::int32_t GetFirstHumanId(const void* point, std::int32_t radius);
		std::int32_t GetNextHumanId();
		std::int32_t GetFirstSectorHumanId(std::uint32_t player, std::uint32_t point, std::int32_t radius, bool useSecondList);
		std::int32_t GetFirstSectorHumanId(std::uint32_t player, bool useSecondList);
		std::int32_t GetNextSectorHumanId();

	  private:
		std::int32_t l_GetNext(std::int32_t previous);
		std::int32_t l_GetNextSectorHumanId(std::int32_t previous);
	};

	static_assert(sizeof(CHumanIterator) == 44);
}