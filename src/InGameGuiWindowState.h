#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiMiscWindows
{
	// Complete CMissionInfoWindow history region 005690E0..005691AB.
	struct SMissionBriefingHistory
	{
		std::uint32_t count;
		std::int32_t entries[50];
	};

	inline SMissionBriefingHistory gMissionBriefingHistory{ };

	// Physical state models; filter bytes without established names remain explicit.
	struct SDiplomacyWindowStateStorage
	{
		std::uint32_t page;
	};

	struct SHumanListWindowStateStorage
	{
		std::uint8_t filterFlags[16];
		std::uint32_t jobFilter, possibleJobFilter;
	};

	struct SStatisticsWindowStateStorage
	{
		std::uint32_t page, goodGroup, timeScaleHours;
	};

	// Original raw storage and initialization/serialization remain unchanged.
	alignas(4) inline std::byte gTechTreeWindowStatics[16]{ };   // 005691B8..005691C7.
	alignas(4) inline std::byte gDiplomacyWindowStatics[4]{ };   // 005691E4..005691E7.
	alignas(4) inline std::byte gHumanListWindowStatics[24]{ };  // 005691E8..005691FF.
	alignas(4) inline std::byte gStatisticsWindowStatics[12]{ }; // 00569220..0056922B.
	void MissionInfoWindow_MissionBriefingHistory_Init();
	void TechTreeWindow_Statics_Init();
	void DiplomacyWindow_Statics_Init();
	void HumanListWindow_Statics_Init();
	void HumanListWindow_l_Filter_Reset();
	void StatisticsWindow_Statics_Init();
}

namespace NC2InGameGuiSelection
{
	alignas(4) inline std::byte gSelectionHouseWindowStatics[12]{ };        // 005691C8..005691D3.
	alignas(4) inline std::byte gSelectionSingleVehicleWindowStatics[6]{ }; // 005691D4..005691D9.
	alignas(4) inline std::byte gSelectionSingleHumanWindowStatics[7]{ };   // 005691DC..005691E2.
	void SelectionHouseWindow_Statics_Init();
	void SelectionSingleVehicleWindow_Statics_Init();
	void SelectionSingleHumanWindow_Statics_Init();
}