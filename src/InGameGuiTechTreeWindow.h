#pragma once
#include "InGameGuiSelectionWindowAccess.h"
#include "XBasicTypes.h"

namespace NC2E2
{
	std::int32_t StaticDataHouses_GetHouseId(std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage);
}

namespace NC2InGameGuiMiscWindows
{
	// Physical Windows storage; raw callbacks and ownership remain unchanged.
	struct STechTreeWindowStorage
	{
		std::byte baseToolWindow[112];
		NC2InGameGuiBase::CDynamicSubButtonsHandler dynamicButtons;
	};

	struct STechTreeTypeElementStorage
	{
		std::byte baseTextButton[96];
		std::uint32_t typeId;
		void* window;
	};

	struct STechTreeLineElementStorage
	{
		std::byte baseElement[44];
		std::int32_t firstX, firstY, secondX, secondY;
	};

	struct STechTreeWindowStateStorage
	{
		std::uint32_t page, selectedHouseType, selectedGoodType, selectedJobType;
	};

	void* TechTreeWindow_Construct(void* window);
	void TechTreeWindow_l_GuiButtons_Add(void* window);
	void TechTreeWindow_l_GuiButtons_Update(void* window);
	bool TechTreeWindow_IsHouseTypeFree(std::uint32_t type);
	bool TechTreeWindow_IsGoodTypeFree(std::uint32_t type);
	bool TechTreeWindow_IsJobTypeFree(std::uint32_t type);
	bool TechTreeWindow_IsHouseTypeForbidden(std::uint32_t type);
	bool TechTreeWindow_IsGoodTypeForbidden(std::uint32_t type);
	bool TechTreeWindow_IsJobTypeForbidden(std::uint32_t type);
}