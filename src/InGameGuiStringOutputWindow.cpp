#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiClockToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiStringOutputWindow.h"
#include "NetworkShutdown.h"
#include "StringInputHandler.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"
#include "XStringArray.h"
#include "XPalette.h"

#include <Windows.h>

// Shared graphic-button/clock construction, base-window destruction and framing.

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;

	static void StringOutputWindow_String_Init(void* window);
	static void StringOutputWindow_String_Exit(void* window);
	static void* __fastcall StringOutputWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags);
	static bool __fastcall StringOutputWindow_MessageHandle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall StringOutputWindow_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);

	// Complete nineteen-entry table 004F7620..004F766B. Its inherited slots
	// preserve the base-tool-window methods, including slots 15..18.
	static const std::uintptr_t gStringOutputWindowVirtualTable[19] = {
		reinterpret_cast<std::uintptr_t>(&StringOutputWindow_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&StringOutputWindow_MessageHandle),
		reinterpret_cast<std::uintptr_t>(&StringOutputWindow_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DrawOverlays)
	};

	static std::int32_t AddWrapped(std::int32_t value, std::uint32_t amount)
	{
		return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) + amount);
	}

	static const NXBasics::SColorRGB* FontColour255(const NXBasics::CPalette* palette)
	{
		return reinterpret_cast<const NXBasics::SColorRGB*>(palette->GetColorData() + 255 * 4);
	}

	// Original function: FUN_004c7030; CStringOutputWindow constructor.
	void* StringOutputWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(170, AddWrapped(state.displayHeight, 0xFFFFFFC0u), AddWrapped(state.displayWidth, 0xFFFFFE48u), 64);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gStringOutputWindowVirtualTable + 0);
		Write<std::uint32_t>(window, NC2Layout::StringOutputWindowOffset::Strings, 0);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		void* closer = nullptr;
		if (allocation != nullptr)
		{
			const NXBasics::SRectangle closeRectangle(AddWrapped(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width), 0xFFFFFFEEu), 2, 16, 14);
			closer = NC2InGameGuiBase::WindowCloserButton_Construct(allocation, nullptr, &closeRectangle);
		}
		NXBaseGui::BaseWindow_Element_Add(window, closer);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiClockElement, 1);
		void* clock = nullptr;
		if (allocation != nullptr)
		{
			const NXBasics::SRectangle clockRectangle(AddWrapped(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width), 0xFFFFFFE8u), AddWrapped(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Height), 0xFFFFFFE8u), 20, 20);
			clock = NC2InGameGuiBase::ToolClockElement_Construct(allocation, &clockRectangle);
		}
		Write<std::uint8_t>(clock, NC2Layout::ClockElementOffset::AnalogMode, 1);
		NXBaseGui::BaseWindow_Element_Add(window, clock);
		StringOutputWindow_String_Init(window);
		return window;
	}

	// Original function: FUN_004c70fe; deleting destructor, preserving both phases.
	static void* __fastcall StringOutputWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags)
	{
		(void) unusedEdx;
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gStringOutputWindowVirtualTable + 0);
		StringOutputWindow_String_Exit(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(window);
		return window;
	}

	// Original function: FUN_004c712d.
	static bool __fastcall StringOutputWindow_MessageHandle(void* window, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		(void) unusedEdx;
		(void) param2;
		(void) param3;
		if (message == 0x1000) return command == 0x7DC;
		if (message != 0x800) return false;
		if (command == 0x4E20)
			Read<NXBasics::CStringArray*>(window, NC2Layout::StringOutputWindowOffset::Strings)->InsertString(reinterpret_cast<const char*>(param1));
		else if (command == 0x4E21)
		{
			StringOutputWindow_String_Exit(window);
			StringOutputWindow_String_Init(window);
		}
		return true;
	}

	// Original function: FUN_004c7181.
	static void __fastcall StringOutputWindow_Draw(void* window, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		(void) unusedEdx;
		NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		if (NNetwork::Network_IsInSession())
		{
			const auto seconds = NC2Logic::gGameTime.tick / 12u;
			const auto minutes = seconds / 60u;
			char elapsed[100];
			NXBasics::Runtime_PrintString(elapsed, "%d:%2.2d:%2.2d", static_cast<std::int32_t>(minutes / 60u), static_cast<std::int32_t>(minutes % 60u), static_cast<std::int32_t>(seconds % 60u));
			const auto* colour = FontColour255(resources.fontDimmedPalette);
			auto* font = resources.font08;
			const auto textWidth = NXBasics::Font_GetPixelWidth(font, elapsed);
			const auto x = AddWrapped(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width), 0u - static_cast<std::uint32_t>(textWidth) - 2u);
			bitmap->Text_Print(font, colour, x, 20, elapsed);
		}
		auto* font = resources.font08;
		std::int32_t y = 0;
		if (NC2Logic::gGameStaticVars.networkWait != 0)
		{
			const auto* colour = FontColour255(resources.fontRedPalette);
			const char* waiting = NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(0x122);
			bitmap->Text_Print(font, colour, 4, 0, waiting);
			y = 14;
		}
		auto* input = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetStringInputHandler();
		if (input->mEditing != 0)
		{
			bitmap->Text_Print(font, FontColour255(resources.fontWhitePalette), 4, y, ">");
			bitmap->Text_Print(font, FontColour255(resources.fontWhitePalette), 14, y, input->GetString());
			const auto shade = static_cast<std::uint8_t>(((timeGetTime() / 200u) & 1u) != 0 ? 255 : 100);
			const NXBasics::SColorRGB cursorColour(shade, shade, shade);
			const auto cursorX = NXBasics::Runtime_TimeSigned32(input->GetCursorPixelPosition(font) + 14u);
			bitmap->Draw_VerticalLine(cursorX, static_cast<std::uint32_t>(y) + 2u, 10, cursorColour);
			y = AddWrapped(y, 12);
		}
		auto index = Read<NXBasics::CStringArray*>(window, NC2Layout::StringOutputWindowOffset::Strings)->GetNumberOfStrings() - 1u;
		while (NXBasics::Runtime_TimeSigned32(index) >= 0 && y < Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Height))
		{
			const auto* colour = FontColour255(resources.fontWhitePalette);
			const char* text = Read<NXBasics::CStringArray*>(window, NC2Layout::StringOutputWindowOffset::Strings)->GetStringPtr(index);
			bitmap->Text_Print(font, colour, 4, y, text);
			--index;
			y = AddWrapped(y, 12);
		}
	}

	// Original function: FUN_004c733e.
	static void StringOutputWindow_String_Init(void* window)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CStringArray), 1);
		auto* strings = allocation == nullptr ? nullptr : new (allocation) NXBasics::CStringArray(1);
		Write(window, NC2Layout::StringOutputWindowOffset::Strings, strings);
	}

	// Original function: FUN_004c735f; the original leaves +70 unchanged.
	static void StringOutputWindow_String_Exit(void* window)
	{
		auto* strings = Read<NXBasics::CStringArray*>(window, NC2Layout::StringOutputWindowOffset::Strings);
		if (strings != nullptr) delete strings;
	}
}
