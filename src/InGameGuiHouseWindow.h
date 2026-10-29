#pragma once
#include "InGameGuiStorage.h"
#include "InGameGuiSelectionWindowAccess.h"

namespace NC2InGameGuiSelection
{
	// Physical Windows storage; runtime construction and ownership remain unchanged.
	struct SSelectionHouseWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		std::uint8_t tabsEnabled[11];
		std::uint8_t tabsCollapsed[11];
		std::byte unresolved86[2];
		std::uint32_t tabStateChecksum;
		NXBasics::SRectangle tabRectangles[11];
		std::int32_t computedWidth, computedHeight;
		std::int32_t tabHeightOverrides[11];
		std::uint8_t layoutInvalid;
		std::byte unresolved171[3];
		void* previewElement;
		std::uint32_t selectedStockGood, stockPage;
		std::uint32_t stockGoods[7][14];
		NC2InGameGuiBase::CStaticToolTipHandler tooltips;
		NC2InGameGuiBase::CDynamicSubButtonsHandler dynamicButtons;
	};

	static_assert(sizeof(SSelectionHouseWindowStorage) == (sizeof(void*) == 8 ? 16872 : 15584));

	void* SelectionHouseWindow_Construct(void* window);
}