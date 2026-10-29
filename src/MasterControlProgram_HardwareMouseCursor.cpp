#include "MasterControlProgram.h"
#include "XMMouseManager.h"
#include "XSystemMouseCursor.h"

namespace NMasterControlProgram
{
	// Original function: FUN_004016ee
	void CMasterControlProgram::HardwareMouseCursor_Enable()
	{
		mHardwareMouseCursorEnabled = true;
		NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(false);
		NXSysMisc::XWS_SystemMouseCursor_SetNormal();
		NXSysMisc::XWS_SystemMouseCursor_Show();
	}

	// Original function: FUN_0040170c
	void CMasterControlProgram::HardwareMouseCursor_Disable()
	{
		mHardwareMouseCursorEnabled = false;
		NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(true);
		NXSysMisc::XWS_SystemMouseCursor_SetNormal();
		NXSysMisc::XWS_SystemMouseCursor_Hide();
	}

	// Original function: FUN_0040172a
	void CMasterControlProgram::HardwareMouseCursor_SetMode(TMouseCursorModes mouseCursorMode)
	{
		if (!mHardwareMouseCursorEnabled)
		{
			return;
		}

		const int mouseCursorIndex = static_cast<int>(mouseCursorMode);

		if (mouseCursorIndex == -1)
		{
			NXSysMisc::XWS_SystemMouseCursor_SetCustom(nullptr);

			return;
		}

		NXSysMisc::XWS_SystemMouseCursor_SetCustom(mMouseCursors[mouseCursorIndex]);
	}

	// Original function: FUN_00401750
	void CMasterControlProgram::HardwareMouseCursor_ReSet()
	{
		if (mHardwareMouseCursorEnabled)
		{
			NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(false);
			NXSysMisc::XWS_SystemMouseCursor_SetNormal();
			NXSysMisc::XWS_SystemMouseCursor_Show();

			if (mHardwareMouseCursorEnabled)
			{
				NXSysMisc::XWS_SystemMouseCursor_SetCustom(mMouseCursors[0]);
			}
		}
		else
		{
			NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(true);
			NXSysMisc::XWS_SystemMouseCursor_SetNormal();
			NXSysMisc::XWS_SystemMouseCursor_Hide();
		}
	}
}