#pragma once

#include "C2AIGuideAccess.h"
#include "C2AIManagerAccess.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Only the scalar prefixes and pointed-to fixed-width data are saved.
	struct SPlayerStatisticManagerStorage
	{
		std::uint32_t currentSampleIndex, lastSampleTime;
		void* historyByPlayer[20];
		void* currentByPlayer[20];
	};

	struct SEmbeddedSavegameManagerStorage
	{
		std::uint32_t size;
		void* data;
	};

	static_assert(sizeof(SPlayerStatisticManagerStorage) == (sizeof(void*) == 8 ? 328 : 168));
	static_assert(offsetof(SPlayerStatisticManagerStorage, historyByPlayer) == 8);
	static_assert(offsetof(SPlayerStatisticManagerStorage, currentByPlayer) == (sizeof(void*) == 8 ? 168 : 88));
	static_assert(sizeof(SEmbeddedSavegameManagerStorage) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SEmbeddedSavegameManagerStorage, data) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(sizeof(SAIGuideGridStorage) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(sizeof(SAIGuideManagerStorage) == (sizeof(void*) == 8 ? 40 : 36));
	static_assert(offsetof(SAIGuideManagerStorage, calculationGrid) == 24);
	static_assert(sizeof(SAIManagerStorage) == 40 * sizeof(void*));
}