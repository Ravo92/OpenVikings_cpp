#pragma once

#include "C2MapSectorGridCursor.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CAnimalIterator
	{
	  public:
		CAnimalIterator();
		std::int32_t GetFirstAnimalId();
		std::int32_t GetFirstAnimalId(std::uint32_t player);
		std::int32_t GetFirstAnimalId(std::uint32_t player, const void* point, std::int32_t radius);
		std::int32_t GetNextAnimalId();

		std::int32_t GetFirstSectorAnimalId(std::uint32_t selection, std::uint32_t packedCenter, std::int32_t radius, bool flag1, bool flag2);
		std::int32_t GetNextSectorAnimalId();

		std::uint32_t flags;
		std::uint32_t selection;
		std::uint32_t packedCenter;
		std::int32_t radius;
		SSectorGridCursor firstSector;
		SSectorGridCursor lastSector;
		SSectorGridCursor currentSector;
		std::int32_t currentObjectId;

	  private:
		CAnimalIterator* l_InitIteratorFields();
		std::int32_t l_GetNext(std::int32_t previousAnimalId);
		std::int32_t l_GetNextSectorAnimalId(std::int32_t previousAnimalId);
	};

	static_assert(sizeof(CAnimalIterator) == 44);
	static_assert(offsetof(CAnimalIterator, flags) == 0);
	static_assert(offsetof(CAnimalIterator, selection) == 0x04);
	static_assert(offsetof(CAnimalIterator, packedCenter) == 0x08);
	static_assert(offsetof(CAnimalIterator, radius) == 0x0C);
	static_assert(offsetof(CAnimalIterator, firstSector) == 0x10);
	static_assert(offsetof(CAnimalIterator, lastSector) == 0x18);
	static_assert(offsetof(CAnimalIterator, currentSector) == 0x20);
	static_assert(offsetof(CAnimalIterator, currentObjectId) == 0x28);
}