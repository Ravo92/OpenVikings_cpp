#pragma once
#include "InGameGuiStorage.h"
#include <cstdint>

namespace NC2InGameGuiSelectedAction
{

	struct SHumanEquipmentWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint32_t category;
		std::int32_t slot;
		std::uint8_t validSelection;
		std::byte unresolved79[3];
		std::uint32_t selectionChangeCount;
	};

	static_assert(sizeof(SHumanEquipmentWindowStorage) == (sizeof(void*) == 8 ? 200 : 128));

	void* SelectedSingleHumanChangeEquippedGoodsWindow_Construct(void* window, std::uint32_t type, std::int32_t slot);
}