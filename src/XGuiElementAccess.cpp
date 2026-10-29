#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"

#include <cstring>


namespace NXBaseGui
{
	using namespace NC2Lifetime;
	static void* __fastcall BaseElement_Delete(void* element, void* unusedEdx, std::uint32_t flags);
	// Complete eighteen-entry base-element table 004F43F4..004F443B.
	const std::uintptr_t gBaseElementVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Delete>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Message_Handle),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};
	// Original 00504E58 DWORD: all exported xrefs assign then increment it.
	static std::uint32_t gNextElementSerial = 1;

	// Original function: FUN_00478936; serial assignment follows state/flags.
	static void BaseElement_ConstructState(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageMask)
	{
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiBaseElementOffset::Rectangle, 0, sizeof(SBaseElementStorage) - NC2Layout::GuiBaseElementOffset::Rectangle);
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 0);
		std::memcpy(static_cast<std::byte*>(element) + NC2Layout::GuiBaseElementOffset::Rectangle, rectangle, sizeof(*rectangle));
		flags |= NXBasics::BitMask::Bit5;
		if ((flags & 2u) != 0) flags |= 1u;
		Write(element, NC2Layout::GuiBaseElementOffset::Flags, flags);
		Write(element, NC2Layout::GuiBaseElementOffset::MessageMask, messageMask);
		Write(element, NC2Layout::GuiBaseElementOffset::Serial, gNextElementSerial);
		++gNextElementSerial;
	}

	// Original function: FUN_004788c6.
	void* BaseElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageMask)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseElementVirtualTable + 0);
		BaseElement_ConstructState(element, rectangle, flags, messageMask);
		return element;
	}

	// Original function: FUN_00478917; base destruction preserves the table.
	void BaseElement_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiBaseElementOffset::Rectangle, 0, sizeof(SBaseElementStorage) - NC2Layout::GuiBaseElementOffset::Rectangle);
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 0);
	}

	// Original function: FUN_004788e6.
	static void* __fastcall BaseElement_Delete(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void) unusedEdx;
		BaseElement_Destruct(element);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(element);
		return element;
	}

	// Original function: FUN_004788a7.
	bool __fastcall BaseElement_HasSubElements(void* element)
	{
		return (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit6) != 0;
	}

	// Original function: FUN_004788b1; the base hit test accepts every local point.
	bool __fastcall BaseElement_IsHit(void*, void*, const std::int32_t*)
	{
		return true;
	}

	// Original function: FUN_004788b6.
	bool __fastcall BaseElement_ToolTip_AtOnce(void*)
	{
		return false;
	}

	// Original function: FUN_004788b9; the original consumes two stack arguments.
	std::uint32_t __fastcall BaseElement_UnresolvedVirtual40(void*, void*, std::uint32_t, std::uint32_t)
	{
		return 0;
	}

	// Original function: FUN_004788be; shared added-to-desktop/pressed-action slot.
	void __fastcall BaseElement_Notify(void*)
	{}

	// Original function: FUN_004788bf.
	void __fastcall BaseWindow_DrawOverlays(void*, void*, NXBasics::CBitmap*, const NXBasics::SRectangle*)
	{}

	// Original function: FUN_004788c2.
	void* __fastcall BaseWindow_GetRealActiveElementPtr(void* window)
	{
		return Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild);
	}

	// Original function: FUN_00478983; the base renderer is an empty original body.
	void __fastcall BaseElement_Draw(void*, void*, NXBasics::CBitmap*, const NXBasics::SRectangle*)
	{}

	// Original function: FUN_00478986.
	void __fastcall BaseElement_Activate(void* element, void*, const std::int32_t*)
	{
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 1);
	}

	// Original function: FUN_00478994.
	void __fastcall BaseElement_DeActivate(void* element)
	{
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 0);
	}

	// Original function: FUN_0047899f.
	bool __fastcall BaseElement_ButtonPressed(void* element, void*, const std::int32_t*)
	{
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 2); return true;
	}

	// Original function: FUN_004789af.
	bool __fastcall BaseElement_ButtonReleased(void* element)
	{
		const auto state = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState);
		if (state == 1) Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 0);
		if (state != 2) return false;
		if ((Read<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & 8u) != 0)
		{
			auto* parent = Read<CBaseWindow*>(element, NC2Layout::GuiBaseElementOffset::ParentWindow);
			BaseWindow_AddToCloseList(parent);
		}
		Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 1);
		using TPressedAction = void(__thiscall*)(void*);
		Read<TPressedAction*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[17](element);
		return true;
	}

	// Original function: FUN_00478a36; ret 14h establishes five stack DWORDs.
	bool __fastcall BaseElement_Message_Handle(void*, void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		return false;
	}

	// Original function: FUN_004789eb; entering while the button is down restores pressed state.
	void BaseElement_MouseEnteredArea(void* element)
	{
		Write<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1, Read<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1) | 1u);
		if (BaseElement_CanBePressed(element) && Read<std::uint8_t>(CDesktop::sTheObjectPtr, NC2Layout::GuiDesktopOffset::LeftButtonDownFlag) != 0 && Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState) == 1) Write<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::InteractionState, 2);
	}
	// Original function: FUN_00478a3b.
	bool BaseElement_CanBeActivated(const void* element)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags); return (flags & NXBasics::BitMask::Bit10) == 0 && (flags & 1u) != 0;
	}
	// Original function: FUN_00478a4f.
	bool BaseElement_CanBePressed(const void* element)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags); return (flags & NXBasics::BitMask::Bit10) == 0 && (flags & 2u) != 0;
	}

	// Original function: FUN_00478a65; unsigned subtraction preserves x86 wraparound.
	void __fastcall BaseElement_ParentToLocalPosition(void* element, void*, std::int32_t* point)
	{
		Write(point, NC2Layout::PixelPointOffset::X, Read<std::uint32_t>(point, NC2Layout::PixelPointOffset::X) - Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::PositionX));
		Write(point, NC2Layout::PixelPointOffset::Y, Read<std::uint32_t>(point, NC2Layout::PixelPointOffset::Y) - Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::PositionY));
	}

	// Original function: FUN_00478ad8.
	void* __fastcall BaseElement_FindElementOnPosition(void* element, void*, const std::int32_t* point, bool includeDisabled)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);
		if (!includeDisabled && ((flags & NXBasics::BitMask::Bit10) != 0 || (flags & 1u) == 0)) return nullptr;
		if ((flags & NXBasics::BitMask::Bit9Unsigned) != 0) return nullptr;
		using THitTest = bool(__thiscall*)(void*, const std::int32_t*);
		return Read<THitTest*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[8](element, point) ? element : nullptr;
	}

	// Original function: FUN_0048e569; shared identity-returning virtual entry.
	void* __fastcall BaseElement_GetRealActiveElementPtr(void* element)
	{
		return element;
	}

	// Original function: FUN_004b8659.
	bool __fastcall BaseElement_ToolTip_Available(void*, void*, const char**)
	{
		return false;
	}

	// Original function: FUN_004bf2f4.
	bool __fastcall BaseElement_Hide_CanBeHidden(void*)
	{
		return true;
	}
}
