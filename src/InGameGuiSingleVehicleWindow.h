#pragma once
#include "InGameGuiStorage.h"
#include "InGameGuiSelectionWindowAccess.h"

namespace NC2InGameGuiSelection
{
	// Windows storage only; runtime constructors and child ownership remain unchanged.
	struct SSingleVehicleRouteControlsStorage
	{
		void* select;
		void* assign;
		void* remove;
		void* label;
	};

	struct SSingleVehicleWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseWindow;
		std::uint8_t tabsEnabled[5];
		std::uint8_t tabsCollapsed[5];
		std::byte unresolved7A[2];
		std::uint32_t tabStateChecksum;
		NXBasics::SRectangle tabRectangles[5];
		std::int32_t computedWidth, computedHeight;
		std::int32_t tabHeightOverrides[5];
		std::uint8_t layoutInvalid;
		std::byte unresolvedED[3];
		void* previewElement;
		SSingleVehicleRouteControlsStorage routeControls[2];
		void* traderConfigButton;
		std::uint32_t stockPage;
		std::uint32_t stockGoods[7][14];
		NC2InGameGuiBase::CStaticToolTipHandler tooltips;
		NC2InGameGuiBase::CDynamicSubButtonsHandler dynamicButtons;
		void* traderConfigWindow;
	};

	static_assert(sizeof(SSingleVehicleWindowStorage) == (sizeof(void*) == 8 ? 16808 : 15488));

	void* SelectionSingleVehicleWindow_Construct(void* window);
}