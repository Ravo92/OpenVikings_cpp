#pragma once
#include "InGameGuiStorage.h"

#include "XBasicTypes.h"
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
}

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows owner; undocumented bytes retain an explicit unresolved identity.
	struct SMissionInfoCallbackStorage
	{
		std::uint8_t called;
		std::byte padding[3];
		std::uint32_t command, argument1, argument2;
	};

	struct SMissionInfoWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage inheritedToolWindow;
		std::uint32_t page;
		std::uint8_t hasMapRoot;
		char mapRoot[260];
		std::uint8_t hasCenteredMainDisplay;
		std::byte unresolved17A[2];
		std::uint8_t hasBriefingFile;
		std::byte padding17D[3];
		std::int32_t briefingId;
		char briefingDirectory[260], briefingFilename[260];
		SMissionInfoCallbackStorage callback;
		std::uint8_t ownsGoalStrings;
		std::byte padding39D[3];
		void* goalStrings;
		NXBasics::SRectangle historyFrame;
		void* hypertextElement;
	};

	static_assert(sizeof(SMissionInfoWindowStorage) == (sizeof(void*) == 8 ? 1032 : 952));

	void* MissionInfoWindow_Construct(void* window, std::uint32_t briefingId, bool goalsPage);
	void MissionInfoWindow_l_Briefing_SetId(void* window, std::int32_t briefingId, bool recordHistory);
	void MissionInfoWindow_l_Briefing_FreeData(void* window);
	void MissionInfoWindow_l_GuiButtons_Add(void* window, bool autoScroll);
	void MissionInfoWindow_MissionBriefingHistory_Add(std::int32_t briefingId);
	void MissionInfoWindow_MissionBriefingHistory_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void MissionInfoWindow_MissionBriefingHistory_IO_Savegame_Load(NC2Logic::CIoHelper* io);
}