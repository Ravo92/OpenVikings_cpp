#pragma once
#include "InGameGuiSelectionWindowAccess.h"

namespace NC2InGameGuiMiscWindows
{

	struct SNetworkInfoWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint32_t page;
		NC2InGameGuiBase::CStaticToolTipHandler tooltips;
	};

	static_assert(sizeof(SNetworkInfoWindowStorage) == (sizeof(void*) == 8 ? 12192 : 12120));

	void* NetworkInfoWindow_Construct(void* window);
	void NetworkInfoWindow_l_GuiButtons_Add(void* window);
}