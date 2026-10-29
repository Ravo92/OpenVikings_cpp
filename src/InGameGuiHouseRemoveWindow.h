#pragma once
#include "InGameGuiStorage.h"

namespace NC2InGameGuiSelectedAction
{

	struct SHouseRemoveWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t selectionValidFlag;
		std::byte unresolved71[3];
		std::uint32_t selectionChangeCount;
	};

	static_assert(sizeof(SHouseRemoveWindowStorage) == (sizeof(void*) == 8 ? 192 : 120));

	void* SelectedHouseRemoveWindow_Construct(void* window);
}