#pragma once
#include "InGameGuiStorage.h"

namespace NC2InGameGuiSelection
{
	struct SWallBuildWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SWallBuildWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	struct SRoadBuildWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SRoadBuildWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	void* SelectionWallBuildWindow_Construct(void* window);
	void* SelectionRoadBuildWindow_Construct(void* window);
}