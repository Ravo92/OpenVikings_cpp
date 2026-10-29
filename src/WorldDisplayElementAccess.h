#pragma once

#include "C2EngineDisplayAccess.h"
#include "InGameGuiStorage.h"
#include "XGuiElementAccess.h"
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2InGameGuiWorldElement
{

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