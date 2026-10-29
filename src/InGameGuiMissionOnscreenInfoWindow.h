#pragma once
#include "InGameGuiStorage.h"
#include "C2MapGuid.h"

void* __fastcall MissionOnscreenInfoStaticGuiWindow_Construct(void* window);

namespace NC2InGameGuiMiscWindows
{
	struct SMissionOnscreenTextRowStorage
	{
		char text[400];
		std::uint8_t compactFont;
	};

	struct SMissionOnscreenWindowStorage
	{
		NXBaseGui::SBaseWindowStorage base;
		NC2Logic::SCleanMapGuid mapGuid;
		void* missionStrings;
		std::uint32_t lastRefreshTime;
		SMissionOnscreenTextRowStorage rows[25];
	};

	static_assert(sizeof(SMissionOnscreenTextRowStorage) == 401);
	static_assert(sizeof(SMissionOnscreenWindowStorage) == (sizeof(void*) == 8 ? 10232 : 10160));

	void MissionOnscreenInfoStaticGuiWindow_l_CurrentMapStrings_Update(void* window);
}