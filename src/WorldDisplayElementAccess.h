#pragma once

#include "C2EngineDisplayAccess.h"
#include "XGuiElementAccess.h"
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiWorldElement
{
	// Windows widget storage. Embedded engine search data and command buffers retain their widths.
	struct SWorldGuiElementStorage
	{
		NXBaseGui::SBaseElementStorage baseElement;
		NXBasics::CBitmap* bitmap;
		void* display;
		std::uint8_t inputEnabled;
		std::byte unresolved35[3];
		NC2E2::SDisplaySearchObject objectUnderMouse;
		std::uint32_t humanInteractionAction;
		std::uint32_t vehicleInteractionAction;
		std::byte primaryCommand[20];
		std::byte secondaryCommand[20];
		std::uint8_t areaSelectionActive;
		std::byte unresolved1F5[3];
		std::int32_t areaStartX, areaStartY, areaEndX, areaEndY;
		std::uint32_t frameMode;
		void* leftClickHandler;
		std::uint32_t leftClickCommand;
		void* doubleClickHandler;
		std::uint32_t doubleClickCommand;
		std::uint32_t lastDoubleClickEventTime;
		std::uint8_t followEnabled, followHuman;
		std::byte unresolved222[2];
		std::int32_t followHumanId;
		std::uint32_t followHumanUniqueId;
		std::uint8_t followVehicle;
		std::byte unresolved22D[3];
		std::int32_t followVehicleId;
		std::uint32_t followVehicleUniqueId;
		std::uint32_t darkenVariant;
		void* darkenPalette;
		std::uint16_t* darkenHighColorTable;
	};

	void* WorldDisplayElement_Construct(void* element, const NXBasics::SRectangle* rectangle, bool active, bool inputEnabled);
	void WorldDisplayElement_Destruct(void* element);
	void WorldDisplayElement_FollowMode_FollowHuman(void* element, std::int32_t id);
	std::int32_t WorldDisplayElement_FollowMode_GetHumanId(void* element);
	void WorldDisplayElement_FollowMode_FollowVehicle(void* element, std::int32_t id);
	std::int32_t WorldDisplayElement_FollowMode_GetVehicleId(void* element);
	void WorldDisplayElement_l_FollowMode_Validate(void* element);
	void WorldDisplayElement_l_FollowMode_Update(void* element);
	void WorldDisplayElement_FollowMode_End(void* element);
	void WorldDisplayElement_DrawFrame_SetMode(void* element, std::uint32_t mode);
	void WorldDisplayElement_Misc_LeftMouseClick_HandlerSet(void* element, void* handler, std::uint32_t command);
	void WorldDisplayElement_Misc_LeftMouseDoubleClick_HandlerSet(void* element, void* handler, std::uint32_t command);
	void WorldDisplayElement_DarkenBitmap_Exit(void* element);
	void WorldDisplayElement_DarkenBitmap_Init(void* element, std::uint32_t variant);
	void WorldDisplayElement_Draw(void* element, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle);
	void WorldDisplayElement_ValidateObjectUnderMouse(void* element);
	void WorldDisplayElement_DarkenBitmap_Do(const void* element, NXBasics::CBitmap* bitmap);
	bool WorldDisplayElement_MouseSelectingArea_IsAreaValid(const void* element);
	bool WorldDisplayElement_l_TryToBringUpActionButtonsForObjectUnderMouse(void* element);
	bool WorldDisplayElement_l_TryToSelectAndBringUpActionButtonsForObjectUnderMouse(void* element);
}