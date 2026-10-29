#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"

namespace NXBasics
{
	class CBitmap;
}

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage; drawing bitmap pointers are borrowed, not owned.
	struct SStatisticsWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage baseToolWindow;
		void* jobsList;
		void* housesList;
		void* graveyardList;
		std::uint32_t jobsListUpdateTick, housesListUpdateTick, graveyardListUpdateTick;
		NXBasics::CBitmap* bitmap;
		NXBasics::CBitmap* diagramBitmap;
		std::int32_t diagramWidth, diagramHeight;
		std::uint32_t diagramPlayerId;
		std::int32_t observedMinimum, observedMaximum, displayMinimum, displayMaximum;
		NXBasics::SColorRGB diagramColors[12];
		std::byte unresolvedDC[400];
	};

	static_assert(sizeof(SStatisticsWindowStorage) == (sizeof(void*) == 8 ? 720 : 620));

	void* StatisticsWindow_Construct(void* window);
	void StatisticsWindow_l_GuiButtons_Add(void* window);
	void StatisticsWindow_l_GuiButtons_Update(void* window);
	void StatisticsWindow_l_GuiButtons_UpdateJobsListBox(void* window);
	void StatisticsWindow_l_GuiButtons_UpdateHousesListBox(void* window);
	void StatisticsWindow_l_GuiButtons_UpdateGraveyardListBox(void* window);
}