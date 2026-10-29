#include "C2Debug.h"
#include "C2GameEngineStartup.h"
#include "C2LifetimeGlobals.h"
#include "X2DApplication.h"
#include "XCrtFileAccess.h"
#include "XMMessageDialogs.h"

#include <cstdarg>
#include <Windows.h>

namespace NC2Logic
{
	// Original function: FUN_0043fdba
	void __cdecl C2Debug_WarningBoxV(const char* format, ...)
	{
		char message[1000];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(message, 1000, format, arguments);
		va_end(arguments);
		message[999] = '\0';
		C2Debug_WarningBox(message);
	}

	// Original function: FUN_0043fdf0
	void C2Debug_WarningBox(const char* message)
	{
		if (NXSys2DApp::CApplication::sTheObjectPtr->IsFullscreen())
			NXSys2DApp::CApplication::sTheObjectPtr->ToggleFullscreen();

		ClipCursor(nullptr);

		if (gGame != nullptr && gGameInitialized != 0 && gGameStaticVars.networkGame == 0)
			GameTime_SetSpeed(0.0);

		NXSysMisc::XWS_WarningBox(message);
	}
}