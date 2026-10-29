#include "X2DApplication.h"
#include "XCrtFileAccess.h"
#include "XMMessageDialogs.h"
#include "XResultIdentifiers.h"
#include "XSystemMouseCursor.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include <Windows.h>

namespace NXSysMisc
{
	// Original function: FUN_00479e40. Windows-specific HRESULT names and format.
	static void FormatDirectDrawError(char* buffer, std::int32_t result, const char* sourceFile, std::int32_t line)
	{
		const char* name;

		switch (static_cast<std::uint32_t>(result))
		{
			case SwitchIds::DirectDrawResult::Code_DDERR_UNSUPPORTED:
				name = "DDERR_UNSUPPORTED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_GENERIC:
				name = "DDERR_GENERIC";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTINITIALIZED:
				name = "DDERR_NOTINITIALIZED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OUTOFMEMORY:
				name = "DDERR_OUTOFMEMORY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDPARAMS:
				name = "DDERR_INVALIDPARAMS";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_ALREADYINITIALIZED:
				name = "DDERR_ALREADYINITIALIZED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANNOTATTACHSURFACE:
				name = "DDERR_CANNOTATTACHSURFACE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANNOTDETACHSURFACE:
				name = "DDERR_CANNOTDETACHSURFACE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CURRENTLYNOTAVAIL:
				name = "DDERR_CURRENTLYNOTAVAIL";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_EXCEPTION:
				name = "DDERR_EXCEPTION";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_HEIGHTALIGN:
				name = "DDERR_HEIGHTALIGN";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INCOMPATIBLEPRIMARY:
				name = "DDERR_INCOMPATIBLEPRIMARY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDCAPS:
				name = "DDERR_INVALIDCAPS";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDCLIPLIST:
				name = "DDERR_INVALIDCLIPLIST";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDMODE:
				name = "DDERR_INVALIDMODE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDOBJECT:
				name = "DDERR_INVALIDOBJECT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDPIXELFORMAT:
				name = "DDERR_INVALIDPIXELFORMAT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDRECT:
				name = "DDERR_INVALIDRECT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_LOCKEDSURFACES:
				name = "DDERR_LOCKEDSURFACES";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NO3D:
				name = "DDERR_NO3D";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOALPHAHW:
				name = "DDERR_NOALPHAHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCLIPLIST:
				name = "DDERR_NOCLIPLIST";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCOLORCONVHW:
				name = "DDERR_NOCOLORCONVHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCOOPERATIVELEVELSET:
				name = "DDERR_NOCOOPERATIVELEVELSET";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCOLORKEY:
				name = "DDERR_NOCOLORKEY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCOLORKEYHW:
				name = "DDERR_NOCOLORKEYHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NODIRECTDRAWSUPPORT:
				name = "DDERR_NODIRECTDRAWSUPPORT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOEXCLUSIVEMODE:
				name = "DDERR_NOEXCLUSIVEMODE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOFLIPHW:
				name = "DDERR_NOFLIPHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOGDI:
				name = "DDERR_NOGDI";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOMIRRORHW:
				name = "DDERR_NOMIRRORHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTFOUND:
				name = "DDERR_NOTFOUND";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOOVERLAYHW:
				name = "DDERR_NOOVERLAYHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NORASTEROPHW:
				name = "DDERR_NORASTEROPHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOROTATIONHW:
				name = "DDERR_NOROTATIONHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOSTRETCHHW:
				name = "DDERR_NOSTRETCHHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOT4BITCOLOR:
				name = "DDERR_NOT4BITCOLOR";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOT4BITCOLORINDEX:
				name = "DDERR_NOT4BITCOLORINDEX";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOT8BITCOLOR:
				name = "DDERR_NOT8BITCOLOR";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTEXTUREHW:
				name = "DDERR_NOTEXTUREHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOVSYNCHW:
				name = "DDERR_NOVSYNCHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOZBUFFERHW:
				name = "DDERR_NOZBUFFERHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOZOVERLAYHW:
				name = "DDERR_NOZOVERLAYHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OUTOFCAPS:
				name = "DDERR_OUTOFCAPS";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OUTOFVIDEOMEMORY:
				name = "DDERR_OUTOFVIDEOMEMORY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OVERLAYCANTCLIP:
				name = "DDERR_OVERLAYCANTCLIP";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
				name = "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_PALETTEBUSY:
				name = "DDERR_PALETTEBUSY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_COLORKEYNOTSET:
				name = "DDERR_COLORKEYNOTSET";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACEALREADYATTACHED:
				name = "DDERR_SURFACEALREADYATTACHED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACEALREADYDEPENDENT:
				name = "DDERR_SURFACEALREADYDEPENDENT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACEBUSY:
				name = "DDERR_SURFACEBUSY";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANTLOCKSURFACE:
				name = "DDERR_CANTLOCKSURFACE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACEISOBSCURED:
				name = "DDERR_SURFACEISOBSCURED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACELOST:
				name = "DDERR_SURFACELOST";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_SURFACENOTATTACHED:
				name = "DDERR_SURFACENOTATTACHED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_TOOBIGHEIGHT:
				name = "DDERR_TOOBIGHEIGHT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_TOOBIGSIZE:
				name = "DDERR_TOOBIGSIZE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_TOOBIGWIDTH:
				name = "DDERR_TOOBIGWIDTH";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_UNSUPPORTEDFORMAT:
				name = "DDERR_UNSUPPORTEDFORMAT";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_UNSUPPORTEDMASK:
				name = "DDERR_UNSUPPORTEDMASK";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_VERTICALBLANKINPROGRESS:
				name = "DDERR_VERTICALBLANKINPROGRESS";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_WASSTILLDRAWING:
				name = "DDERR_WASSTILLDRAWING";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_XALIGN:
				name = "DDERR_XALIGN";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDDIRECTDRAWGUID:
				name = "DDERR_INVALIDDIRECTDRAWGUID";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_DIRECTDRAWALREADYCREATED:
				name = "DDERR_DIRECTDRAWALREADYCREATED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NODIRECTDRAWHW:
				name = "DDERR_NODIRECTDRAWHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_PRIMARYSURFACEALREADYEXISTS:
				name = "DDERR_PRIMARYSURFACEALREADYEXISTS";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOEMULATION:
				name = "DDERR_NOEMULATION";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_REGIONTOOSMALL:
				name = "DDERR_REGIONTOOSMALL";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CLIPPERISUSINGHWND:
				name = "DDERR_CLIPPERISUSINGHWND";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOCLIPPERATTACHED:
				name = "DDERR_NOCLIPPERATTACHED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOHWND:
				name = "DDERR_NOHWND";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_HWNDSUBCLASSED:
				name = "DDERR_HWNDSUBCLASSED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_HWNDALREADYSET:
				name = "DDERR_HWNDALREADYSET";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOPALETTEATTACHED:
				name = "DDERR_NOPALETTEATTACHED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOPALETTEHW:
				name = "DDERR_NOPALETTEHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_BLTFASTCANTCLIP:
				name = "DDERR_BLTFASTCANTCLIP";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOBLTHW:
				name = "DDERR_NOBLTHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NODDROPSHW:
				name = "DDERR_NODDROPSHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_OVERLAYNOTVISIBLE:
				name = "DDERR_OVERLAYNOTVISIBLE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOOVERLAYDEST:
				name = "DDERR_NOOVERLAYDEST";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDPOSITION:
				name = "DDERR_INVALIDPOSITION";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTAOVERLAYSURFACE:
				name = "DDERR_NOTAOVERLAYSURFACE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_EXCLUSIVEMODEALREADYSET:
				name = "DDERR_EXCLUSIVEMODEALREADYSET";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTFLIPPABLE:
				name = "DDERR_NOTFLIPPABLE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANTDUPLICATE:
				name = "DDERR_CANTDUPLICATE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTLOCKED:
				name = "DDERR_NOTLOCKED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANTCREATEDC:
				name = "DDERR_CANTCREATEDC";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NODC:
				name = "DDERR_NODC";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_WRONGMODE:
				name = "DDERR_WRONGMODE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_IMPLICITLYCREATED:
				name = "DDERR_IMPLICITLYCREATED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTPALETTIZED:
				name = "DDERR_NOTPALETTIZED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_UNSUPPORTEDMODE:
				name = "DDERR_UNSUPPORTEDMODE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOMIPMAPHW:
				name = "DDERR_NOMIPMAPHW";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_INVALIDSURFACETYPE:
				name = "DDERR_INVALIDSURFACETYPE";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_DCALREADYCREATED:
				name = "DDERR_DCALREADYCREATED";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANTPAGELOCK:
				name = "DDERR_CANTPAGELOCK";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_CANTPAGEUNLOCK:
				name = "DDERR_CANTPAGEUNLOCK";
				break;
			case SwitchIds::DirectDrawResult::Code_DDERR_NOTPAGELOCKED:
				name = "DDERR_NOTPAGELOCKED";
				break;
			default:
				name = "Unknown DD Error";
				break;
		}

		NXBasics::Runtime_PrintString(buffer, "NXSysMisc: DirectDraw error occured at %s - %d: <%s>\n", sourceFile, line, name);
	}

	// Original function: FUN_0047a567; corrected entry decoded from Game.exe.
	void __cdecl XWS_DirectDrawErrorBox(std::int32_t result, const char* sourceFile, std::int32_t line)
	{
		if (result == 0)
		{

			return;
		}

		char buffer[200];
		FormatDirectDrawError(buffer, result, sourceFile, line);
		OutputDebugStringA(buffer);
		XWS_WarningBox(buffer);
	}

	// Original function: FUN_0047aaa3.
	void __cdecl XWS_AbortBox(const char* message)
	{
		XWS_SystemMouseCursor_Show();
		ClipCursor(nullptr);
		auto* application = NXSys2DApp::CApplication::sTheObjectPtr;
		application->FlipToGDISurface();
		const HWND windowHandle = application->GetWindowHandle();
		char buffer[1000];
		std::strcpy(buffer, message);
		std::strcat(buffer, "\n\n(press cancel to debug)");

		if (MessageBoxA(windowHandle, buffer, "Fatal error!", MB_OKCANCEL | MB_ICONHAND) == IDCANCEL)
			__debugbreak();

		NXBasics::Runtime_RaiseSignal(22);
	}

	// Original function: FUN_0047ab1a. The Windows callback receives three arguments.
	void __cdecl XWS_AssertBox(const char* message, const char* sourceFile, std::int32_t line)
	{
		XWS_SystemMouseCursor_Show();
		ClipCursor(nullptr);
		auto* application = NXSys2DApp::CApplication::sTheObjectPtr;
		application->FlipToGDISurface();
		const HWND windowHandle = application->GetWindowHandle();
		char buffer[1000];
		NXBasics::Runtime_PrintStringN(buffer, sizeof(buffer) - 1, "%s\n\nFile %s - Line %d\n\n(press retry to debug)", message, sourceFile, line);
		buffer[sizeof(buffer) - 1] = '\0';
		const int selection = MessageBoxA(windowHandle, buffer, "XWS assert", MB_ABORTRETRYIGNORE);

		if (selection == IDABORT)
			NXBasics::Runtime_RaiseSignal(22);

		if (selection == IDABORT || selection == IDRETRY)
			__debugbreak();
	}

	// Original function: FUN_0047a9ea
	void __cdecl XWS_WarningBoxV(const char* format, ...)
	{
		char buffer[1000];
		va_list arguments;
		va_start(arguments, format);
		// Match the legacy _vsnprintf limit and explicit final terminator.
#pragma warning(suppress : 4996)
		_vsnprintf(buffer, sizeof(buffer) - 1, format, arguments);
		va_end(arguments);
		buffer[sizeof(buffer) - 1] = '\0';
		XWS_WarningBox(buffer);
	}

	// Original function: FUN_0047aa20
	void __cdecl XWS_WarningBox(const char* message)
	{
		const bool restoreHiddenCursor = !XWS_SystemMouseCursor_IsShown();

		if (restoreHiddenCursor)
		{
			XWS_SystemMouseCursor_Show();
		}

		ClipCursor(nullptr);
		auto* application = NXSys2DApp::CApplication::sTheObjectPtr;
		application->FlipToGDISurface();
		const HWND windowHandle = application->GetWindowHandle();

		// The original builds a suffixed copy but displays the original message.
		// Omit the unused, unbounded copy while preserving the displayed text.

		if (MessageBoxA(windowHandle, message, "Warning!", MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
		{
			__debugbreak();
		}

		if (restoreHiddenCursor)
		{
			XWS_SystemMouseCursor_Hide();
		}
	}
}