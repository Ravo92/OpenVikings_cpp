#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"

namespace NXBasics
{
	class CStringArray;
}

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage. The mission string array keeps its original ownership.
	struct SDiplomacyWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseToolWindow;
		std::uint32_t playerCount, playerIds[20];
		std::uint8_t hiddenPlayerFlags[20];
		std::uint32_t visiblePlayerCount, lastButtonsUpdateTick;
		NXBasics::CStringArray* missionStrings;
		NXBasics::SRectangle overviewRectangle;
		void* overviewElement;
	};

	static_assert(sizeof(SDiplomacyWindowStorage) == (sizeof(void*) == 8 ? 328 : 248));

	void* DiplomacyWindow_Construct(void* window);
	void DiplomacyWindow_l_GuiButtons_Add(void* window);
	void DiplomacyWindow_l_GuiButtons_Update(void* window);
}