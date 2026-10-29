#pragma once
#include "InGameGuiStorage.h"

namespace NC2InGameGuiSelection
{

	struct SLandscapeSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t validSelection;
		std::byte unresolved71;
		std::byte mapPosition[4]; // Packed Windows map point remains unaligned, read through the existing byte helpers.
		std::byte unresolved76[2];
		std::uint32_t playerId, engineType, valency, stateChecksum;
	};

	static_assert(sizeof(SLandscapeSelectionWindowStorage) == (sizeof(void*) == 8 ? 208 : 136));

	void* SelectionLandscapeTypeWindow_Construct(void* window);
}