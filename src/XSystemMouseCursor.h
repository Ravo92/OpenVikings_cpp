#pragma once

#include <Windows.h>

namespace NXSysMisc
{
	// Original function: FUN_00478b1b
	void __stdcall XWS_SystemMouseCursor_SetNormal();

	// Original function: FUN_00478b37
	void __stdcall XWS_SystemMouseCursor_SetWaitMode();

	// Original function: FUN_00478b53
	void __cdecl XWS_SystemMouseCursor_SetCustom(HCURSOR cursor);

	// Original function: FUN_00478b94
	void __stdcall XWS_SystemMouseCursor_Reset();

	// Original function: FUN_00478bc6
	bool __stdcall XWS_SystemMouseCursor_IsShown();

	// Original function: FUN_00478bcc
	void __stdcall XWS_SystemMouseCursor_Hide();

	// Original function: FUN_00478be9
	void __stdcall XWS_SystemMouseCursor_Show();
}