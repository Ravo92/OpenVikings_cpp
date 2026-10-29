#pragma once
#include "XBasicTypes.h"
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage; callbacks, child ownership and raw access remain unchanged.
	struct SHumanListReferenceStorage
	{
		std::uint32_t humanId, uniqueId;
	};

	struct SHumanListWindowStorage
	{
		std::byte baseToolWindow[112];
		std::uint32_t humanCount;
		SHumanListReferenceStorage humans[500];
		void* jobSelectionWindow;
	};

	struct SHumanListJobSelectionWindowStorage
	{
		std::byte baseToolWindow[112];
		void* ownerWindow;
		std::uint8_t currentJob;
		std::byte unresolved75[3];
	};

	struct SHumanListButtonStorage
	{
		std::byte baseToolElement[48];
		std::uint32_t humanId, uniqueId, pressedCommand, doubleClickCommand;
		void* handler;
		std::uint8_t debug;
		std::byte unresolved45[3];
	};

	void* HumanListWindow_Construct(void* window, std::uint32_t job, std::uint32_t possibleJob);
	bool HumanListWindow_l_UpdateHumanDataArray(void* window);
	void HumanListWindow_l_UpdateHumanListElements(void* window, bool force);
	void HumanListWindow_l_GuiButtons_Add(void* window);
	void HumanListWindow_l_JobSelectionWindow_Open(void* window, bool currentJob);
}