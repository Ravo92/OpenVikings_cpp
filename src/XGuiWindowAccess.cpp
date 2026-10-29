#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "XBitmap.h"
#include "XGuiDesktop.h"
#include "XGuiLists.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"

#include <new>

namespace NXBaseGui
{
	using namespace NC2Lifetime;

	using TElementList = NXBasics::CList<CBaseElement>;
	static TElementList* WindowList(void* window, std::size_t offset)
	{
		return std::launder(reinterpret_cast<TElementList*>(static_cast<std::byte*>(window) + offset));
	}
	// Original function: FUN_00478820; the separate original name remains unresolved.
	void* BaseWindow_Message_SendToChildElements(void* window, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t); NXBasics::CListBase::Iterator iterator(*WindowList(window, NC2Layout::GuiBaseWindowOffset::InputChildren));
		for (void* element = iterator.First(); element != nullptr; element = iterator.Next()) if (Read<TMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](element, 0x1000, command, param1, param2, param3)) return element; return nullptr;
	}
	static void* __fastcall BaseWindow_Delete(void* window, void*, std::uint32_t flags);
	// Complete Windows table 004F43A8..004F43F3.
	const std::uintptr_t gBaseWindowVirtualTable[19] = {
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseWindow_Delete>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Draw>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseWindow_Activate>()),
		reinterpret_cast<std::uintptr_t>(&BaseWindow_DeActivate),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseWindow_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&BaseWindow_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&BaseWindow_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseWindow_DrawOverlays>())
	};

	// Original function: FUN_0047846e; construction precedes the original reset.
	static void BaseWindow_ConstructVars(void* window)
	{
		new (static_cast<std::byte*>(window) + NC2Layout::GuiBaseWindowOffset::DrawChildren) TElementList(false);
		new (static_cast<std::byte*>(window) + NC2Layout::GuiBaseWindowOffset::InputChildren) TElementList(false);
	}

	// Original function: FUN_004784c1; Windows resets the entire 0x40-byte region,
	// including the previously constructed list headers, before setting the area.
	static void BaseWindow_ConstructState(void* window, const NXBasics::SRectangle* rectangle)
	{
		std::memset(static_cast<std::byte*>(window) + NC2Layout::GuiBaseWindowOffset::ClippingRectangle, 0, sizeof(SBaseWindowStorage) - NC2Layout::GuiBaseWindowOffset::ClippingRectangle);
		const NXBasics::SRectangle area(0, 0, rectangle->width, rectangle->height);
		Write(window, NC2Layout::GuiBaseWindowOffset::ClippingRectangle, area);
	}

	// Original function: FUN_0047841b.
	void* __fastcall BaseWindow_Construct(void* window, void*, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId)
	{
		BaseElement_Construct(window, rectangle, flags | NXBasics::BitMask::Bits0Through1AndBit6, elementId);
		BaseWindow_ConstructVars(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseWindowVirtualTable + 0);
		BaseWindow_ConstructState(window, rectangle);
		return window;
	}

	// Original function: FUN_00478452.
	static void* __fastcall BaseWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		BaseWindow_Destruct(window);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(window);
		return window;
	}

	// Original function: FUN_004784eb.
	static void BaseWindow_DestructState(void* window)
	{
		BaseWindow_Element_DeleteAll(window);
		CDesktop::sTheObjectPtr->Element_Remove(static_cast<CBaseElement*>(window));
		std::memset(static_cast<std::byte*>(window) + NC2Layout::GuiBaseWindowOffset::ClippingRectangle, 0, sizeof(SBaseWindowStorage) - NC2Layout::GuiBaseWindowOffset::ClippingRectangle);
	}

	// Original function: FUN_0047849a.
	void __fastcall BaseWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseWindowVirtualTable + 0);
		BaseWindow_DestructState(window);
		WindowList(window, NC2Layout::GuiBaseWindowOffset::InputChildren)->~TElementList();
		WindowList(window, NC2Layout::GuiBaseWindowOffset::DrawChildren)->~TElementList();
		BaseElement_Destruct(window);
	}

	// Original function: FUN_004785e8.
	void BaseWindow_Element_Remove(void* window, void* element)
	{
		Write<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1, Read<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1) & NXBasics::BitMask::ByteExceptBit3);
		if (Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild) == element)
		{
			using TDeactivate = void(__thiscall*)(void*);
			Read<TDeactivate*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](element);
			Write<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild, nullptr);
		}
		CDesktop::sTheObjectPtr->Element_Remove(static_cast<CBaseElement*>(element));
		WindowList(window, NC2Layout::GuiBaseWindowOffset::DrawChildren)->l_Base_RemoveElement(element);
		WindowList(window, NC2Layout::GuiBaseWindowOffset::InputChildren)->l_Base_RemoveElement(element);
	}

	// Original function: FUN_00478627; removal occurs even before the null check.
	void BaseWindow_Element_Delete(void* window, void* element)
	{
		BaseWindow_Element_Remove(window, element);
		if (element != nullptr)
		{
			using TDelete = void* (__thiscall*)(void*, std::uint32_t);
			Read<TDelete*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[0](element, 1);
		}
	}

	// Original function: FUN_00478642.
	void BaseWindow_Element_DeleteAll(void* window)
	{
		while (void* element = WindowList(window, NC2Layout::GuiBaseWindowOffset::DrawChildren)->l_Base_GetStartElement()) BaseWindow_Element_Delete(window, element);
	}

	// Original function: FUN_004787ac; point containment uses inclusive, wrapped
	// right/bottom bounds exactly as Windows SPoint::IsInside (004398AF).
	void* BaseWindow_FindElementOnPosition(void* window, const std::int32_t* point, bool includeDisabled)
	{
		NXBasics::CListBase::Iterator iterator(*WindowList(window, NC2Layout::GuiBaseWindowOffset::InputChildren));
		for (void* child = iterator.First(); child != nullptr; child = iterator.Next())
		{
			const NXBasics::SPoint position{ point[0], point[1] };
			const NXBasics::SRectangle rectangle(Read<std::int32_t>(child, NC2Layout::GuiBaseElementOffset::PositionX), Read<std::int32_t>(child, NC2Layout::GuiBaseElementOffset::PositionY), Read<std::int32_t>(child, NC2Layout::GuiBaseElementOffset::Width), Read<std::int32_t>(child, NC2Layout::GuiBaseElementOffset::Height));
			if (!position.IsInside(rectangle)) continue;
			std::int32_t localPoint[2] = { point[0], point[1] };
			using TConvert = void(__thiscall*)(void*, std::int32_t*);
			Read<TConvert*>(child, NC2Layout::GuiBaseElementOffset::VirtualTable)[11](child, localPoint);
			using TFind = void* (__thiscall*)(void*, const std::int32_t*, bool);
			if (void* hit = Read<TFind*>(child, NC2Layout::GuiBaseElementOffset::VirtualTable)[10](child, localPoint, includeDisabled)) return hit;
		}
		return nullptr;
	}

	// Original function: FUN_004786a7.
	void __fastcall BaseWindow_Activate(void* window, void*, const std::int32_t* point)
	{
		BaseElement_Activate(window, nullptr, point);
		CDesktop::sTheObjectPtr->Window_BringToFront(static_cast<CBaseWindow*>(window));
		std::int32_t localPoint[2] = { point[0], point[1] };
		using TConvert = void(__thiscall*)(void*, std::int32_t*);
		Read<TConvert*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[11](window, localPoint);
		void* hit = BaseWindow_FindElementOnPosition(window, localPoint, false);
		void* active = Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild);
		using TDeactivate = void(__thiscall*)(void*);
		if (hit == nullptr)
		{
			if (active != nullptr)
			{
				Read<TDeactivate*>(active, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](active); Write<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild, nullptr);
			}
		}
		else if (hit != active)
		{
			if (active != nullptr) Read<TDeactivate*>(active, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](active);
			Write(window, NC2Layout::GuiBaseWindowOffset::ActiveChild, hit);
			using TActivate = void(__thiscall*)(void*, const std::int32_t*);
			Read<TActivate*>(hit, NC2Layout::GuiBaseElementOffset::VirtualTable)[4](hit, localPoint);
		}
	}

	// Original function: FUN_00478720.
	void __fastcall BaseWindow_DeActivate(void* window)
	{
		void* active = Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild);
		if (active != nullptr)
		{
			using TDeactivate = void(__thiscall*)(void*);
			Read<TDeactivate*>(active, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](active);
			Write<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild, nullptr);
		}
		BaseElement_DeActivate(window);
	}

	// Original function: FUN_0047873c; the press callback receives the window's
	// local point, while the search itself applies each child's virtual conversion.
	bool __fastcall BaseWindow_ButtonPressed(void* window, void*, const std::int32_t* point)
	{
		if (Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::InteractionState) != 1) return false;
		std::int32_t localPoint[2] = { point[0], point[1] };
		using TConvert = void(__thiscall*)(void*, std::int32_t*);
		Read<TConvert*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[11](window, localPoint);
		void* hit = BaseWindow_FindElementOnPosition(window, localPoint, false);
		if (hit == nullptr) return false;
		if (!BaseElement_CanBePressed(hit)) return false;
		if (hit != Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild)) return true;
		using TPress = bool(__thiscall*)(void*, const std::int32_t*);
		return Read<TPress*>(hit, NC2Layout::GuiBaseElementOffset::VirtualTable)[6](hit, localPoint);
	}

	// Original function: FUN_0047879d.
	bool __fastcall BaseWindow_ButtonReleased(void* window)
	{
		void* active = Read<void*>(window, NC2Layout::GuiBaseWindowOffset::ActiveChild);
		if (active == nullptr) return false;
		using TRelease = bool(__thiscall*)(void*);
		return Read<TRelease*>(active, NC2Layout::GuiBaseElementOffset::VirtualTable)[7](active);
	}

	// Original function: FUN_00478511; child draw order follows the forward list.
	void BaseWindow_DrawAllChildElements(void* window, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle& rectangle)
	{
		NXBasics::CListBase::Iterator iterator(*WindowList(window, NC2Layout::GuiBaseWindowOffset::DrawChildren));
		for (void* child = iterator.First(); child != nullptr; child = iterator.Next())
		{
			if ((Read<std::uint32_t>(child, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) != 0) continue;
			auto area = BaseElement_GetRectangle(child);
			if (!rectangle.IsTouching(area)) continue;
			area.CutInside(rectangle);
			NXBasics::CBitmap clipped(bitmap, area);
			area.x = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) - Read<std::uint32_t>(child, NC2Layout::GuiBaseElementOffset::PositionX));
			area.y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) - Read<std::uint32_t>(child, NC2Layout::GuiBaseElementOffset::PositionY));
			using TDraw = void(__thiscall*)(void*, NXBasics::CBitmap*, const NXBasics::SRectangle*);
			Read<TDraw*>(child, NC2Layout::GuiBaseElementOffset::VirtualTable)[3](child, &clipped, &area);
		}
	}

	// Original function: FUN_004785b1; CBaseWindow::BW_Element_Add.
	void BaseWindow_Element_Add(void* window, void* element)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);
		if ((flags & NXBasics::BitMask::Bit11) != 0) return;
		Write(element, NC2Layout::GuiBaseElementOffset::Flags, flags | NXBasics::BitMask::Bit11);
		Write(element, NC2Layout::GuiBaseElementOffset::ParentWindow, window);
		auto* bytes = static_cast<std::byte*>(window);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + NC2Layout::GuiBaseWindowOffset::DrawChildren))->InsertAtEnd(element);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + NC2Layout::GuiBaseWindowOffset::InputChildren))->InsertAtStart(element);
	}

	// Original function: FUN_00478687; CBaseWindow::BW_Internal_Hide.
	void BaseWindow_InternalHide(void* window)
	{
		if ((Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) == 0) BaseElement_InternalHide(window);
	}

	// Original function: FUN_00478874; CBaseWindow::XGui_BE_Internal_Element_AddedToDesktop.
	void BaseWindow_NotifyAddedToDesktop(void* window)
	{
		auto* list = std::launder(reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(window) + NC2Layout::GuiBaseWindowOffset::DrawChildren));
		NXBasics::CListBase::Iterator iterator(*list);
		using TAdded = void(__thiscall*)(void*);
		for (void* element = iterator.First(); element != nullptr; element = iterator.Next()) Read<TAdded*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[12](element);
	}

	// Original function: FUN_00478697.
	void BaseWindow_InternalUnHide(void* window)
	{
		if ((Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) != 0) BaseElement_InternalUnHide(window);
	}

	// Original function: FUN_0047867a; the desktop handles a null window.
	void BaseWindow_AddToCloseList(void* window)
	{
		CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<CBaseWindow*>(window));
	}

	// Original function: FUN_00478aa3; CBaseElement::BE_Internal_Element_Hide.
	void BaseElement_InternalHide(void* element)
	{
		if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) != 0) return;
		CDesktop::sTheObjectPtr->Element_Deactivate(static_cast<CBaseElement*>(element));
		Write<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1, static_cast<std::uint8_t>(Read<std::uint8_t>(element, NC2Layout::GuiBaseElementOffset::FlagsByte1) | 2u));
	}
	// Original function: FUN_00478ac2; an already visible element is not written.
	void BaseElement_InternalUnHide(void* element)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);
		if ((flags & NXBasics::BitMask::Bit9Unsigned) != 0) Write(element, NC2Layout::GuiBaseElementOffset::Flags, flags & ~0x200u);
	}
}
