#pragma once

#include <cstdint>

void* __fastcall InGameGuiManagerDefaultMessageHandlerElement_Construct(void* element);
void* __fastcall InGameGuiManagerPrimaryMessageHandlerElement_Construct(void* element);
void* __fastcall InGameGuiManagerDefaultMessageHandlerElement_Delete(void* element, void* unusedEdx, std::uint32_t flags);
bool __fastcall InGameGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle(void* element, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);