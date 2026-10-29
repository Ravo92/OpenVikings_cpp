#pragma once

#include <cstdint>

namespace NC2MainMenuGuiManager
{
	void* GuiObjectVars_Construct(void* object);
	void GuiObjectVars_Destruct(void* manager);
	void* DefaultMessageHandler_Construct(void* element);
	void DefaultMessageHandler_Destruct(void* element);
	void* PrimaryMessageHandler_Construct(void* element);
	void* MousePointer_Construct(void* pointer);
}

namespace NC2MainMenuGuiWorldElement
{
	void* WorldDisplayElement_Construct(void* element, const void* rectangle, bool active);
	void WorldDisplayElement_FollowHuman(void* element, std::int32_t humanId);
}

namespace NC2InGameGuiBase
{
	void BaseToolTextButton_SetCaptionString(void* element, const char* text);
	void BaseToolTextButton_SetBackgroundColor(void* element, const void* color);
	void BaseToolTextButton_SetToolTipString(void* element, const char* text);
}