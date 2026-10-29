#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XList.h"
#include "XNumericConstants.h"

#include <new>

namespace NXBaseGui
{
	using namespace NC2Lifetime;

	// Original function: FUN_00477163.
	void CDesktop::Element_Add(CBaseElement* element, bool insertAtEnd)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);

		if ((flags & NXBasics::BitMask::Bit11) != 0)
		{

			return;
		}

		Write(element, NC2Layout::GuiBaseElementOffset::Flags, flags | NXBasics::BitMask::Bit11);
		std::launder(reinterpret_cast<NXBasics::SRectangle*>(reinterpret_cast<std::byte*>(element) + NC2Layout::GuiBaseElementOffset::Rectangle))->PlaceInside(mRectangle);
		auto* bytes = reinterpret_cast<std::byte*>(this);
		auto* drawList = std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mBackgroundElementDrawListStorage)));
		auto* inputList = std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mBackgroundElementHitTestListStorage)));

		if (insertAtEnd)
		{
			drawList->InsertAtEnd(element);
			inputList->InsertAtStart(element);
		}
		else
		{
			drawList->InsertAtStart(element);
			inputList->InsertAtEnd(element);
		}

		using TAdded = void(__thiscall*)(void*);
		Read<TAdded*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[12](element);
	}

	// Original function: FUN_004771be; overlay draw/input lists are distinct from normal elements.
	void CDesktop::Element_AddOverlay(CBaseElement* element, bool insertAtEnd)
	{
		const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);

		if ((flags & NXBasics::BitMask::Bit11) != 0)
		{

			return;
		}

		Write(element, NC2Layout::GuiBaseElementOffset::Flags, flags | NXBasics::BitMask::Bit11);
		std::launder(reinterpret_cast<NXBasics::SRectangle*>(reinterpret_cast<std::byte*>(element) + NC2Layout::GuiBaseElementOffset::Rectangle))->PlaceInside(mRectangle);
		auto* bytes = reinterpret_cast<std::byte*>(this);
		auto* drawList = std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mOverlayElementDrawListStorage)));
		auto* inputList = std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mOverlayElementHitTestListStorage)));

		if (insertAtEnd)
		{
			drawList->InsertAtEnd(element);
			inputList->InsertAtStart(element);
		}
		else
		{
			drawList->InsertAtStart(element);
			inputList->InsertAtEnd(element);
		}

		Read<void(__thiscall**)(void*)>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[12](element);
	}

	// Original function: FUN_00477219.
	void CDesktop::Window_Add(CBaseWindow* window)
	{
		if (window == nullptr)
			return;

		const auto flags = Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Flags);

		if ((flags & NXBasics::BitMask::Bit11) != 0)
		{

			return;
		}

		Write<std::uint8_t>(window, NC2Layout::GuiBaseWindowOffset::DesktopListFlags, static_cast<std::uint8_t>(Read<std::uint8_t>(window, NC2Layout::GuiBaseWindowOffset::DesktopListFlags) | 1u));
		Write(window, NC2Layout::GuiBaseElementOffset::Flags, flags | NXBasics::BitMask::Bit11);
		std::launder(reinterpret_cast<NXBasics::SRectangle*>(reinterpret_cast<std::byte*>(window) + NC2Layout::GuiBaseElementOffset::Rectangle))->PlaceInside(mRectangle);
		auto* bytes = reinterpret_cast<std::byte*>(this);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mWindowOrderListStorage)))->InsertAtEnd(window);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mWindowMessageListStorage)))->InsertAtStart(window);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mWindowDrawListStorage)))->InsertAtEnd(window);
		std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offsetof(CDesktop, mWindowHitTestListStorage)))->InsertAtStart(window);
		BaseWindow_NotifyAddedToDesktop(window);
		using TAdded = void(__thiscall*)(void*);
		Read<TAdded*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[12](window);
		Element_Activate(reinterpret_cast<CBaseElement*>(window));
	}

	// Original function: FUN_004773f6.
	std::uint32_t CDesktop::Window_Hide(CBaseWindow* window)
	{
		using TCanBeHidden = bool(__thiscall*)(void*);

		if (window == nullptr || !Read<TCanBeHidden*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[15](window))
			return 0;

		if (static_cast<void*>(Window_GetActivePtr()) == window)
		{
			using TDeactivate = void(__thiscall*)(void*);

			if (mActiveElement != nullptr)
			{
				auto* active = mActiveElement;
				Read<TDeactivate*>(active, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](active);
				mActiveElement = nullptr;
			}

			Read<TDeactivate*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[5](window);
		}

		BaseWindow_InternalHide(window);

		return Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Serial);
	}

	// Original function: FUN_00477532.
	std::uint32_t CDesktop::Element_Hide(CBaseElement* element)
	{
		using TCanBeHidden = bool(__thiscall*)(void*);

		if (element == nullptr || !Read<TCanBeHidden*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[15](element))
			return 0;

		Element_Deactivate(element);
		BaseElement_InternalHide(element);

		return Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Serial);
	}

	// Original function: FUN_00477564.
	void CDesktop::Element_UnHide(CBaseElement* element)
	{
		if (element != nullptr)
			BaseElement_InternalUnHide(element);
	}

	// Original function: FUN_00477943; the original climbs at most one parent.
	CBaseElement* CDesktop::Window_GetActivePtr() const
	{
		auto* active = mActiveElement;

		if (active == nullptr || (Read<std::uint32_t>(active, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit6) != 0)
			return active;

		return Read<CBaseElement*>(active, NC2Layout::GuiBaseElementOffset::ParentWindow);
	}
}