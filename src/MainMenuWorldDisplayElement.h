#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	struct SRectangle;
}

namespace NC2MainMenuGuiWorldElement
{
	void WorldDisplayElement_Destruct(void* element);
	void WorldDisplayElement_l_FollowMode_Update(void* element);
	void WorldDisplayElement_l_Tool_DarkenBitmap_Exit(void* element);
	void WorldDisplayElement_l_Tool_DarkenBitmap_Do(const void* element, NXBasics::CBitmap* bitmap);
}

void* __fastcall MainMenuWorldDisplayElement_Delete(void* element, void* unusedEdx, std::uint32_t flags);
void __fastcall MainMenuWorldDisplayElement_XGui_BE_Element_Draw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
bool __fastcall MainMenuWorldDisplayElement_XGui_BE_Message_Handle(void* element, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);
void __fastcall MainMenuWorldDisplayElement_l_Tool_DarkenBitmap_Init(void* element);