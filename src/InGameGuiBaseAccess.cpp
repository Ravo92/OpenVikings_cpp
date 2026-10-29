#include "ApplicationGlobals.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"

#include <cstring>

// The shared base-window constructor, rendering and inherited virtual methods
// retain their genuine Windows bodies until their GUI families are reconstructed.

namespace NC2InGameGuiBase
{
	using namespace NC2Lifetime;

	// Original function: FUN_004a91e6; folded singleton getter, original method name unresolved.
	NXBaseGui::CDesktop* GetDesktopPtr()
	{

		return NXBaseGui::CDesktop::sTheObjectPtr;
	}

	// Original function: FUN_004a91ec; folded static-field getter, original method name unresolved.
	std::int32_t GetDisplayWidth()
	{

		return NInGameGuiManager::CInGameGuiManager::mStaticVars.displayWidth;
	}

	// Original function: FUN_004a91f2; folded static-field getter, original method name unresolved.
	std::int32_t GetDisplayHeight()
	{

		return NInGameGuiManager::CInGameGuiManager::mStaticVars.displayHeight;
	}

	// Original function: FUN_004c9313; this shared wrapper destroys the base window.
	void* __fastcall BaseToolWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		NXBaseGui::BaseWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004b8d42.
	void __fastcall BaseToolWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		if (Read<std::uint32_t>(window, NC2Layout::BaseToolWindowOffset::BackgroundStyle) != SwitchIds::ToolWindowBackgroundStyle::Papyrus)
		{
			NC2GuiToolsBase::DrawTool_FillArea(window, bitmap, rectangle, 1);
			NC2GuiToolsBase::DrawTool_DrawFrame(window, bitmap, rectangle, 1);

			return;
		}

		std::uint32_t x = 0, y = 0;

		while (bitmap->GetSourceBitmap() != nullptr)
		{
			x += static_cast<std::uint32_t>(bitmap->GetSourceX());
			y += static_cast<std::uint32_t>(bitmap->GetSourceY());
			bitmap = bitmap->GetSourceBitmap();
		}

		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		NXBasics::BobManager_PrintBob(resources.windowBobManager, 0x19, bitmap, NXBasics::Runtime_TimeSigned32(x), NXBasics::Runtime_TimeSigned32(y), resources.papyrusPalette);
	}

	// Original function: FUN_004b95a4.
	void ToolGraphics_SetPalettePtr(void* object, NXBasics::CPalette* palette)
	{
		std::memcpy(static_cast<std::byte*>(object) + NC2Layout::GuiGfxButtonOffset::Palette, &palette, sizeof(palette));
	}

	// Complete nineteen-entry table at 004F52A4..004F52EF.
	const std::uintptr_t gBaseToolWindowVirtualTable[19] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NC2InGameGuiBase::BaseToolWindow_Delete>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NC2InGameGuiBase::BaseToolWindow_Draw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseWindow_Activate>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseWindow_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseWindow_DrawOverlays>())
	};

	// Original function: FUN_004b8543.
	void BaseToolDesktop_AddWindow(NXBaseGui::CBaseWindow* window, bool showWhileHidden)
	{
		NXBaseGui::CDesktop::sTheObjectPtr->Window_Add(window);

		if (Read<std::uint32_t>(NC2InGameGuiInputManager::gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0 && !showWhileHidden)
			NXBaseGui::CDesktop::sTheObjectPtr->Window_Hide(window);
	}

	// Original function: FUN_004b84ed.
	void BaseToolDesktop_AddOverlayElement(NXBaseGui::CBaseElement* element)
	{
		NXBaseGui::CDesktop::sTheObjectPtr->Element_AddOverlay(element, true);

		if (Read<std::uint32_t>(NC2InGameGuiInputManager::gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Hide(element);
	}

	// Original function: FUN_004b8518; Windows normal desktop-element adapter.
	void BaseToolDesktop_AddElement(NXBaseGui::CBaseElement* element)
	{
		NXBaseGui::CDesktop::sTheObjectPtr->Element_Add(element, true);

		if (Read<std::uint32_t>(NC2InGameGuiInputManager::gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Hide(element);
	}

	// Original function: FUN_004b8573.
	void BaseToolDesktop_SelectionActiveChanged()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (Read<std::int32_t>(NC2InGameGuiInputManager::gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
		{
			desktop->Window_HideAll(nullptr);
			desktop->Element_HideAll(static_cast<NXBaseGui::CBaseElement*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement)));
			desktop->Window_UnHide(static_cast<NXBaseGui::CBaseWindow*>(manager->MiscWindows_OverviewWindow_GetPtr()));
			desktop->Window_UnHide(static_cast<NXBaseGui::CBaseWindow*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::MissionOnscreenInfoWindow)));
			desktop->Element_Activate(static_cast<NXBaseGui::CBaseElement*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement)));
		}
		else
		{
			desktop->Window_UnHideAll();
			desktop->Element_UnHideAll();
			desktop->Element_Activate(desktop->Window_GetActive());
		}
	}

	// Original function: FUN_004b8d11; CBaseToolWindow constructor.
	void* BaseToolWindow_Construct(void* window, const NXBasics::SRectangle* rectangle)
	{
		NXBaseGui::BaseWindow_Construct(window, nullptr, rectangle, 0, 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::BaseToolWindowOffset::BackgroundStyle, 0, 4);
		Write<std::uint32_t>(window, NC2Layout::BaseToolWindowOffset::BackgroundStyle, SwitchIds::ToolWindowBackgroundStyle::Framed);

		return window;
	}
}