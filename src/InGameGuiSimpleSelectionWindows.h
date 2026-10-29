#pragma once
#include "InGameGuiSelectionWindowAccess.h"

namespace NC2InGameGuiSelection
{
	struct SHouseTypeSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SHouseTypeSelectionWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	struct SSingleAnimalSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SSingleAnimalSelectionWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	struct SGuideSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		NC2InGameGuiBase::CStaticToolTipHandler tooltips;
	};

	static_assert(sizeof(SGuideSelectionWindowStorage) == (sizeof(void*) == 8 ? 12192 : 12116));

	void* SelectionGuideWindow_Construct(void* window);
	void* SelectionHouseTypeWindow_Construct(void* window);
	void* SelectionSingleAnimalWindow_Construct(void* window);
}