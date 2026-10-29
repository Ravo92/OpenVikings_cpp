#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "X2DDisplay.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiLists.h"
#include "XGuiWindowAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

namespace
{
	void DestroyDesktopOwnedElement(void* object)
	{
		if (object == nullptr)
		{

			return;
		}

		using TDeletingDestructor = void*(__thiscall*)(void*, unsigned int);

		TDeletingDestructor* methods;
		std::memcpy(&methods, object, sizeof(methods));
		methods[0](object, 1u);
	}

	void CallDesktopElementVirtual14(void* element)
	{
		using TVirtualFunction = void(__thiscall*)(void*);
		TVirtualFunction* methods = nullptr;

		std::memcpy(&methods, element, sizeof(methods));
		methods[5](element);
	}

	NXBaseGui::CBaseElement* ReadBaseElementParent(const NXBaseGui::CBaseElement* element)
	{
		NXBaseGui::CBaseElement* parent = nullptr;
		const auto* bytes = reinterpret_cast<const std::byte*>(element);

		std::memcpy(&parent, bytes + NC2Layout::GuiBaseElementOffset::ParentWindow, sizeof(parent));

		return parent;
	}

	void CallDesktopElementVirtual10(void* element)
	{
		using TVirtualFunction = void(__thiscall*)(void*, const void*);
		TVirtualFunction* methods = nullptr;

		// The original call passes two signed 32-bit coordinates.
		const std::int32_t position[2] = { -1, -1 };

		std::memcpy(&methods, element, sizeof(methods));
		methods[4](element, position);
	}

	// Original function: FUN_00478a24
	// Original method name: CBaseElement::BE_Internal_Element_MouseLeftArea
	void BaseElement_MouseLeftArea(NXBaseGui::CBaseElement* element)
	{
		auto* bytes = reinterpret_cast<unsigned char*>(element);
		bytes[NC2Layout::GuiBaseElementOffset::FlagsByte1] &= NXBasics::BitMask::ByteExceptBit0;

		std::int32_t state;
		std::memcpy(&state, bytes + NC2Layout::GuiBaseElementOffset::InteractionState, sizeof(state));

		if (state == 2)
		{
			state = 1;
			std::memcpy(bytes + NC2Layout::GuiBaseElementOffset::InteractionState, &state, sizeof(state));
		}
	}
}

namespace NXBaseGui
{
	// Original function: FUN_00476eca
	CDesktop::CDesktop(std::int32_t width, std::int32_t height, std::int32_t bitmapDepth) : mRectangle(0, 0, 0, 0)
	{
		static_assert(std::is_standard_layout_v<CDesktop>);
		static_assert(offsetof(CDesktop, mRectangle) == offsetof(SDesktopStorage, mRectangle));
		static_assert(offsetof(CDesktop, mDisplayBitmap) == offsetof(SDesktopStorage, mDisplayBitmap));
		static_assert(offsetof(CDesktop, mBackgroundElementDrawListStorage) == offsetof(SDesktopStorage, mBackgroundElementDrawListStorage));
		static_assert(offsetof(CDesktop, mBackgroundElementHitTestListStorage) == offsetof(SDesktopStorage, mBackgroundElementHitTestListStorage));
		static_assert(offsetof(CDesktop, mOverlayElementDrawListStorage) == offsetof(SDesktopStorage, mOverlayElementDrawListStorage));
		static_assert(offsetof(CDesktop, mOverlayElementHitTestListStorage) == offsetof(SDesktopStorage, mOverlayElementHitTestListStorage));
		static_assert(offsetof(CDesktop, mWindowOrderListStorage) == offsetof(SDesktopStorage, mWindowOrderListStorage));
		static_assert(offsetof(CDesktop, mWindowMessageListStorage) == offsetof(SDesktopStorage, mWindowMessageListStorage));
		static_assert(offsetof(CDesktop, mWindowDrawListStorage) == offsetof(SDesktopStorage, mWindowDrawListStorage));
		static_assert(offsetof(CDesktop, mWindowHitTestListStorage) == offsetof(SDesktopStorage, mWindowHitTestListStorage));
		static_assert(offsetof(CDesktop, mOverlayWindowOrderListStorage) == offsetof(SDesktopStorage, mOverlayWindowOrderListStorage));
		static_assert(offsetof(CDesktop, mOverlayWindowMessageListStorage) == offsetof(SDesktopStorage, mOverlayWindowMessageListStorage));
		static_assert(offsetof(CDesktop, mOverlayWindowDrawListStorage) == offsetof(SDesktopStorage, mOverlayWindowDrawListStorage));
		static_assert(offsetof(CDesktop, mOverlayWindowHitTestListStorage) == offsetof(SDesktopStorage, mOverlayWindowHitTestListStorage));
		static_assert(offsetof(CDesktop, mWindowCloseListStorage) == offsetof(SDesktopStorage, mWindowCloseListStorage));
		static_assert(offsetof(CDesktop, mActiveElement) == offsetof(SDesktopStorage, mActiveElement));
		static_assert(offsetof(CDesktop, mMouseOverElement) == offsetof(SDesktopStorage, mMouseOverElement));
		static_assert(offsetof(CDesktop, mKeyboardTargetRoot) == offsetof(SDesktopStorage, mKeyboardTargetRoot));
		static_assert(offsetof(CDesktop, mPrimaryMessageHandler) == offsetof(SDesktopStorage, mPrimaryMessageHandler));
		static_assert(offsetof(CDesktop, mFallbackMessageHandler) == offsetof(SDesktopStorage, mFallbackMessageHandler));
		static_assert(offsetof(CDesktop, mMousePointerStorage) == offsetof(SDesktopStorage, mMousePointerStorage));
		static_assert(offsetof(CDesktop, mLeftButtonDown) == offsetof(SDesktopStorage, mLeftButtonDown));
		static_assert(offsetof(CDesktop, mRightButtonDown) == offsetof(SDesktopStorage, mRightButtonDown));
		static_assert(offsetof(CDesktop, mWorkResult) == offsetof(SDesktopStorage, mWorkResult));
		static_assert(offsetof(CDesktop, mSkipOrdinaryHitTestLists) == offsetof(SDesktopStorage, mSkipOrdinaryHitTestLists));

		l_ConstructLists();

		std::memset(static_cast<void*>(this), 0, sizeof(*this));
		mRectangle.SetVariables(0, 0, width, height);

		void* bitmapAllocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap), 1);
		mDisplayBitmap = bitmapAllocation == nullptr ? nullptr : ::new (bitmapAllocation) NXBasics::CBitmap(width, height, bitmapDepth);
		mDisplayBitmap->Fill(std::uint8_t{ 0 });

		sTheObjectPtr = this;
	}

	// Original function: FUN_00476f2f
	void CDesktop::l_ConstructLists()
	{
		auto* objectBytes = reinterpret_cast<std::byte*>(this);

		const auto constructElementList = [objectBytes](std::size_t offset)
		{
			new (static_cast<void*>(objectBytes + offset)) NXBasics::CList<CBaseElement>(false);
		};

		const auto constructWindowList = [objectBytes](std::size_t offset)
		{
			new (static_cast<void*>(objectBytes + offset)) NXBasics::CList<CBaseWindow>(false);
		};

		constructElementList(offsetof(CDesktop, mBackgroundElementDrawListStorage));
		constructElementList(offsetof(CDesktop, mBackgroundElementHitTestListStorage));
		constructElementList(offsetof(CDesktop, mOverlayElementDrawListStorage));
		constructElementList(offsetof(CDesktop, mOverlayElementHitTestListStorage));
		constructWindowList(offsetof(CDesktop, mWindowOrderListStorage));
		constructWindowList(offsetof(CDesktop, mWindowMessageListStorage));
		constructElementList(offsetof(CDesktop, mWindowDrawListStorage));
		constructElementList(offsetof(CDesktop, mWindowHitTestListStorage));
		constructWindowList(offsetof(CDesktop, mOverlayWindowOrderListStorage));
		constructWindowList(offsetof(CDesktop, mOverlayWindowMessageListStorage));
		constructElementList(offsetof(CDesktop, mOverlayWindowDrawListStorage));
		constructElementList(offsetof(CDesktop, mOverlayWindowHitTestListStorage));
		constructWindowList(offsetof(CDesktop, mWindowCloseListStorage));
	}

	// Original function: FUN_00477010
	CDesktop::~CDesktop()
	{
		Window_CloseAll();
		Element_DeleteAll();

		DestroyDesktopOwnedElement(mPrimaryMessageHandler);
		DestroyDesktopOwnedElement(mFallbackMessageHandler);

		if (mDisplayBitmap != nullptr)
		{
			delete mDisplayBitmap;
			mDisplayBitmap = nullptr;
		}

		if (mMousePointerStorage != nullptr)
			NXBasics::Runtime_Free(mMousePointerStorage);

		sTheObjectPtr = nullptr;

		std::memset(static_cast<void*>(this), 0, sizeof(*this));

		l_DestroyLists();
	}

	// Original function: FUN_00477082
	void CDesktop::l_DestroyLists()
	{
		auto* objectBytes = reinterpret_cast<std::byte*>(this);

		const auto destroyElementList = [objectBytes](std::size_t offset)
		{
			using TList = NXBasics::CList<CBaseElement>;

			auto* list = std::launder(reinterpret_cast<TList*>(objectBytes + offset));

			// Qualified destruction bypasses the cleared virtual-table pointer.
			TList::Delete(list, 0);
		};

		const auto destroyWindowList = [objectBytes](std::size_t offset)
		{
			using TList = NXBasics::CList<CBaseWindow>;

			auto* list = std::launder(reinterpret_cast<TList*>(objectBytes + offset));

			// Qualified destruction bypasses the cleared virtual-table pointer.
			TList::Delete(list, 0);
		};

		destroyWindowList(offsetof(CDesktop, mWindowCloseListStorage));
		destroyElementList(offsetof(CDesktop, mOverlayWindowHitTestListStorage));
		destroyElementList(offsetof(CDesktop, mOverlayWindowDrawListStorage));
		destroyWindowList(offsetof(CDesktop, mOverlayWindowMessageListStorage));
		destroyWindowList(offsetof(CDesktop, mOverlayWindowOrderListStorage));
		destroyElementList(offsetof(CDesktop, mWindowHitTestListStorage));
		destroyElementList(offsetof(CDesktop, mWindowDrawListStorage));
		destroyWindowList(offsetof(CDesktop, mWindowMessageListStorage));
		destroyWindowList(offsetof(CDesktop, mWindowOrderListStorage));
		destroyElementList(offsetof(CDesktop, mOverlayElementHitTestListStorage));
		destroyElementList(offsetof(CDesktop, mOverlayElementDrawListStorage));
		destroyElementList(offsetof(CDesktop, mBackgroundElementHitTestListStorage));
		destroyElementList(offsetof(CDesktop, mBackgroundElementDrawListStorage));
	}

	// Original function: FUN_00477811; deletion is deferred until the desktop
	// processes its close list. Ignore null and already queued windows.
	void CDesktop::Window_AddToCloseList(CBaseWindow* window)
	{
		if (window == nullptr)
			return;

		auto* list = std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(mWindowCloseListStorage));
		NXBasics::CListBase::Iterator iterator(*list);

		for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next())
		{
			if (entry == window)
			{

				return;
			}
		}

		list->InsertAtEnd(window);
	}

	// Original function: FUN_0047787b; four lists preserve drawing/input order.
	void CDesktop::Window_BringToFront(CBaseWindow* window)
	{
		auto* bytes = reinterpret_cast<std::byte*>(this);
		const bool foreground = (reinterpret_cast<const std::uint8_t*>(window)[NC2Layout::GuiBaseWindowOffset::DesktopListFlags] & 1u) != 0;
		const std::size_t firstOffset = foreground ? offsetof(CDesktop, mWindowOrderListStorage) : offsetof(CDesktop, mOverlayWindowOrderListStorage);
		auto listAt = [bytes](std::size_t offset)
		{

			return std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(bytes + offset));
		};

		for (std::uint32_t index = 0; index < 4; ++index)
		{
			listAt(firstOffset + static_cast<std::size_t>(index) * sizeof(NXBasics::CListBase))->l_Base_RemoveElement(window);
		}

		listAt(firstOffset)->InsertAtEnd(window);
		listAt(firstOffset + sizeof(NXBasics::CListBase))->InsertAtStart(window);
		listAt(firstOffset + 2 * sizeof(NXBasics::CListBase))->InsertAtEnd(window);
		listAt(firstOffset + 3 * sizeof(NXBasics::CListBase))->InsertAtStart(window);
	}

	// Original function: FUN_0047795d
	void CDesktop::PrimaryMessageHandler_Set(CBaseElement* handler)
	{
		mPrimaryMessageHandler = handler;
	}

	// Original function: FUN_0047728c
	void CDesktop::Window_Close(CBaseWindow* window)
	{
		if (window == nullptr)
			return;

		auto* objectBytes = reinterpret_cast<std::byte*>(this);

		const auto elementListAt = [objectBytes](std::size_t offset)
		{

			return std::launder(reinterpret_cast<NXBasics::CList<CBaseElement>*>(objectBytes + offset));
		};

		const auto windowListAt = [objectBytes](std::size_t offset)
		{

			return std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(objectBytes + offset));
		};

		auto* list0118 = windowListAt(offsetof(CDesktop, mWindowCloseListStorage));

		if (list0118->ContainsBeforeNullData(window))
		{

			return;
		}

		bool wasActive = false;

		if (static_cast<void*>(mActiveElement) == static_cast<void*>(window))
		{
			CallDesktopElementVirtual14(window);
			mActiveElement = nullptr;
			wasActive = true;
		}

		// The virtual call may have changed the list.

		if (list0118->ContainsBeforeNullData(window))
			list0118->l_Base_RemoveElement(window);

		const auto* windowBytes = reinterpret_cast<const unsigned char*>(window);

		if ((windowBytes[NC2Layout::GuiBaseWindowOffset::DesktopListFlags] & 1u) != 0)
		{
			elementListAt(offsetof(CDesktop, mWindowDrawListStorage))->l_Base_RemoveElement(window);
			elementListAt(offsetof(CDesktop, mWindowHitTestListStorage))->l_Base_RemoveElement(window);
			windowListAt(offsetof(CDesktop, mWindowMessageListStorage))->l_Base_RemoveElement(window);
			windowListAt(offsetof(CDesktop, mWindowOrderListStorage))->l_Base_RemoveElement(window);
		}
		else
		{
			elementListAt(offsetof(CDesktop, mOverlayWindowDrawListStorage))->l_Base_RemoveElement(window);
			elementListAt(offsetof(CDesktop, mOverlayWindowHitTestListStorage))->l_Base_RemoveElement(window);
			windowListAt(offsetof(CDesktop, mOverlayWindowMessageListStorage))->l_Base_RemoveElement(window);
			windowListAt(offsetof(CDesktop, mOverlayWindowOrderListStorage))->l_Base_RemoveElement(window);
		}

		DestroyDesktopOwnedElement(window);

		if (wasActive)
		{
			auto* nextActive = static_cast<CBaseElement*>(windowListAt(offsetof(CDesktop, mOverlayWindowMessageListStorage))->l_Base_GetStartElement());

			if (nextActive == nullptr)
				nextActive = static_cast<CBaseElement*>(windowListAt(offsetof(CDesktop, mWindowMessageListStorage))->l_Base_GetStartElement());

			if (nextActive != nullptr)
				Element_Activate(nextActive);
		}
	}

	// Original function: FUN_004773bc
	void CDesktop::Window_CloseAll()
	{
		auto* objectBytes = reinterpret_cast<std::byte*>(this);
		using TWindowList = NXBasics::CList<CBaseWindow>;

		auto* list00B4 = std::launder(reinterpret_cast<TWindowList*>(objectBytes + offsetof(CDesktop, mOverlayWindowOrderListStorage)));

		while (true)
		{
			auto* window = static_cast<CBaseWindow*>(list00B4->l_Base_GetStartElement());

			if (window == nullptr)
				break;

			Window_Close(window);
		}

		auto* list0064 = std::launder(reinterpret_cast<TWindowList*>(objectBytes + offsetof(CDesktop, mWindowOrderListStorage)));

		while (true)
		{
			auto* window = static_cast<CBaseWindow*>(list0064->l_Base_GetStartElement());

			if (window == nullptr)
				break;

			Window_Close(window);
		}
	}

	// Original functions: FUN_0047744a/FUN_0047745a/FUN_004774cd.
	void CDesktop::Window_UnHide(CBaseWindow* window)
	{
		if (window != nullptr)
			BaseWindow_InternalUnHide(window);
	}

	void CDesktop::Window_HideAll(CBaseWindow* except)
	{
		for (const auto offset : { offsetof(CDesktop, mOverlayWindowOrderListStorage), offsetof(CDesktop, mWindowOrderListStorage) })
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(reinterpret_cast<std::byte*>(this) + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (auto* item = iterator.First(); item != nullptr; item = iterator.Next())
			{
				if (item != except)
					Window_Hide(static_cast<CBaseWindow*>(item));
			}
		}
	}

	void CDesktop::Window_UnHideAll()
	{
		for (const auto offset : { offsetof(CDesktop, mOverlayWindowOrderListStorage), offsetof(CDesktop, mWindowOrderListStorage) })
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(reinterpret_cast<std::byte*>(this) + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (auto* item = iterator.First(); item != nullptr; item = iterator.Next())
			{
				BaseWindow_InternalUnHide(item);
			}
		}
	}

	// Original functions: FUN_00477574/FUN_004775e4.
	void CDesktop::Element_HideAll(CBaseElement* except)
	{
		for (const auto offset : { offsetof(CDesktop, mOverlayElementDrawListStorage), offsetof(CDesktop, mBackgroundElementDrawListStorage) })
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CList<CBaseElement>*>(reinterpret_cast<std::byte*>(this) + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (auto* item = iterator.First(); item != nullptr; item = iterator.Next())
			{
				if (item != except)
					Element_Hide(static_cast<CBaseElement*>(item));
			}
		}
	}

	void CDesktop::Element_UnHideAll()
	{
		for (const auto offset : { offsetof(CDesktop, mOverlayElementDrawListStorage), offsetof(CDesktop, mBackgroundElementDrawListStorage) })
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CList<CBaseElement>*>(reinterpret_cast<std::byte*>(this) + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (auto* item = iterator.First(); item != nullptr; item = iterator.Next())
			{
				BaseElement_InternalUnHide(item);
			}
		}
	}

	// Original function: FUN_0047793b.
	CBaseElement* CDesktop::Window_GetActive() const
	{
		auto* list = std::launder(reinterpret_cast<const NXBasics::CList<CBaseWindow>*>(reinterpret_cast<const std::byte*>(this) + offsetof(CDesktop, mWindowMessageListStorage)));

		return reinterpret_cast<CBaseElement*>(list->l_Base_GetStartElement());
	}

	// Original function: FUN_0047772f.
	void CDesktop::Element_Activate(CBaseElement* element)
	{
		if (element == nullptr || element == mActiveElement)
		{

			return;
		}

		if (mActiveElement != nullptr)
			CallDesktopElementVirtual14(mActiveElement);

		mActiveElement = element;
		CallDesktopElementVirtual10(element);

		// Reload the active element after the virtual call.
		CBaseElement* rootElement = mActiveElement;
		CBaseElement* parent = ReadBaseElementParent(rootElement);

		if (parent != nullptr)
			rootElement = l_Element_GetRootElementPtr(parent);

		std::uint32_t flags;
		const auto* bytes = reinterpret_cast<const std::byte*>(rootElement);
		std::memcpy(&flags, bytes + NC2Layout::GuiBaseElementOffset::Flags, sizeof(flags));

		mKeyboardTargetRoot = (flags & NXBasics::BitMask::Bit2) != 0 ? rootElement : nullptr;
	}

	// Original function: FUN_004777a0
	void CDesktop::Element_Deactivate(CBaseElement* element)
	{
		if (mActiveElement == element)
		{
			CallDesktopElementVirtual14(element);
			mActiveElement = nullptr;
		}

		if (mMouseOverElement == element)
		{
			BaseElement_MouseLeftArea(element);
			mMouseOverElement = nullptr;
		}

		if (mKeyboardTargetRoot == element)
			mKeyboardTargetRoot = nullptr;

		if (mPrimaryMessageHandler == element)
			mPrimaryMessageHandler = nullptr;

		if (mFallbackMessageHandler == element)
			mFallbackMessageHandler = nullptr;
	}

	// Original function: FUN_00477646
	void CDesktop::Element_Remove(CBaseElement* element)
	{
		auto* elementBytes = reinterpret_cast<unsigned char*>(element);
		elementBytes[NC2Layout::GuiBaseElementOffset::FlagsByte1] &= NXBasics::BitMask::ByteExceptBit3;
		Element_Deactivate(element);
		auto* objectBytes = reinterpret_cast<std::byte*>(this);
		constexpr std::size_t listOffsets[] = { offsetof(CDesktop, mBackgroundElementDrawListStorage), offsetof(CDesktop, mBackgroundElementHitTestListStorage), offsetof(CDesktop, mOverlayElementDrawListStorage), offsetof(CDesktop, mOverlayElementHitTestListStorage) };

		for (const std::size_t offset : listOffsets)
		{
			std::launder(reinterpret_cast<NXBasics::CList<CBaseElement>*>(objectBytes + offset))->l_Base_RemoveElement(element);
		}
	}

	// Original function: FUN_00477681
	void CDesktop::Element_Delete(CBaseElement* element)
	{
		Element_Remove(element);
		DestroyDesktopOwnedElement(element);
	}

	// Original function: FUN_0047769c
	void CDesktop::Element_DeleteAll()
	{
		auto* objectBytes = reinterpret_cast<std::byte*>(this);
		using TElementList = NXBasics::CList<CBaseElement>;

		const auto deleteListElements = [this, objectBytes](std::size_t firstOffset, std::size_t secondOffset)
		{
			auto* firstList = std::launder(reinterpret_cast<TElementList*>(objectBytes + firstOffset));
			auto* secondList = std::launder(reinterpret_cast<TElementList*>(objectBytes + secondOffset));

			while (true)
			{
				auto* element = static_cast<CBaseElement*>(firstList->l_Base_GetStartElement());

				if (element == nullptr)
					break;

				Element_Deactivate(element);
				firstList->l_Base_RemoveElement(element);
				secondList->l_Base_RemoveElement(element);
				DestroyDesktopOwnedElement(element);
			}
		};

		deleteListElements(offsetof(CDesktop, mOverlayElementHitTestListStorage), offsetof(CDesktop, mOverlayElementDrawListStorage));
		deleteListElements(offsetof(CDesktop, mBackgroundElementHitTestListStorage), offsetof(CDesktop, mBackgroundElementDrawListStorage));
	}

	// Original function: FUN_0047814f; preserves primary/overlay/element/window/default order and first-handled return.
	CBaseElement* CDesktop::Message_SendToAllElements(std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		const auto send = [=](void* element)
		{

			return NC2Lifetime::Read<TMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](element, 0x1000, command, param1, param2, param3);
		};

		if (mPrimaryMessageHandler != nullptr && send(mPrimaryMessageHandler))
			return mPrimaryMessageHandler;

		constexpr std::size_t offsets[4] = { offsetof(CDesktop, mOverlayWindowMessageListStorage), offsetof(CDesktop, mOverlayElementHitTestListStorage), offsetof(CDesktop, mWindowMessageListStorage), offsetof(CDesktop, mBackgroundElementHitTestListStorage) };

		for (auto offset : offsets)
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CListBase*>(reinterpret_cast<std::byte*>(this) + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (auto* element = iterator.First(); element != nullptr; element = iterator.Next())
			{
				if (offset == offsetof(CDesktop, mOverlayWindowMessageListStorage) || offset == offsetof(CDesktop, mWindowMessageListStorage))
					if (void* child = BaseWindow_Message_SendToChildElements(element, command, param1, param2, param3))
						return static_cast<CBaseElement*>(child);

				if (send(element))
					return static_cast<CBaseElement*>(element);
			}
		}

		if (mFallbackMessageHandler != nullptr && send(mFallbackMessageHandler))
			return static_cast<CBaseElement*>(mFallbackMessageHandler);

		return nullptr;
	}

	// Original function: FUN_004782fa; searches both original window lists.
	CBaseElement* CDesktop::Message_SendToAllWindows(std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		auto* bytes = reinterpret_cast<std::byte*>(this);
		constexpr std::size_t windowListOffsets[] = { offsetof(CDesktop, mOverlayWindowMessageListStorage), offsetof(CDesktop, mWindowMessageListStorage) };

		for (std::size_t offset : windowListOffsets)
		{
			auto* list = std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offset));
			NXBasics::CListBase::Iterator iterator(*list);

			for (void* window = iterator.First(); window != nullptr; window = iterator.Next())
			{
				TMessage* methods = nullptr;
				std::memcpy(&methods, window, sizeof(methods));

				if (methods[2](window, 0x1000, command, param1, param2, param3))
					return static_cast<CBaseElement*>(window);
			}
		}

		return nullptr;
	}

	// Original function: FUN_0047803b
	CBaseElement* CDesktop::l_Element_GetRootElementPtr(CBaseElement* element)
	{
		while (true)
		{
			CBaseElement* parent = ReadBaseElementParent(element);

			if (parent == nullptr)
			{

				return element;
			}

			element = parent;
		}
	}
}

namespace NXBaseGui
{
	// Original function: FUN_00477104; input, deferred closes, desktop and cursor draw.
	std::uint32_t CDesktop::Desktop_DoWork(bool clearBitmap, bool processInput)
	{
		std::uint32_t result = 0;
		std::memcpy(&mWorkResult, &result, sizeof(result));
		l_Message_HandleMouse(processInput);
		l_Message_HandleKeyBoard(processInput);
		l_WindowCloseList_Do();

		if (NXSys2DApp::IsRemasterHudBitmap(mDisplayBitmap))
		{
			mDisplayBitmap->Fill(std::uint32_t{ 0xFF000000 });
		}
		else if (clearBitmap)
		{
			mDisplayBitmap->Fill(std::uint8_t{ 0 });
		}

		l_DrawDesktop(mRectangle);

		if (mMousePointerStorage != nullptr)
		{
			using TDraw = void(__thiscall*)(void*, NXBasics::CBitmap*, const void*);
			TDraw* methods;
			std::memcpy(&methods, mMousePointerStorage, sizeof(methods));
			methods[0](mMousePointerStorage, mDisplayBitmap, NXSysMouseManager::CMouseManager::sTheObjectPtr);
		}

		std::memcpy(&result, &mWorkResult, sizeof(result));

		return result;
	}

	// Original function: FUN_00477859; close processing consumes the list tail.
	void CDesktop::l_WindowCloseList_Do()
	{
		auto& closeList = *std::launder(reinterpret_cast<NXBasics::CList<CBaseWindow>*>(mWindowCloseListStorage));

		while (auto* window = static_cast<CBaseWindow*>(closeList.l_Base_RemoveFromEnd()))
			Window_Close(window);
	}

	using namespace NC2Lifetime;

	static bool DesktopPointInside(const std::int32_t* point, void* element)
	{
		const auto area = BaseElement_GetRectangle(element);
		const auto right = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) + static_cast<std::uint32_t>(area.width) - 1u);
		const auto bottom = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) + static_cast<std::uint32_t>(area.height) - 1u);

		return point[0] >= area.x && point[0] <= right && point[1] >= area.y && point[1] <= bottom;
	}

	using TDesktopMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	static bool SendDesktopElementMessage(void* element, std::uint32_t message, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4)
	{

		return Read<TDesktopMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](element, message, param1, param2, param3, param4);
	}

	// Original function: FUN_0047796a.
	void CDesktop::l_DrawDesktop(const NXBasics::SRectangle& rectangle)
	{
		auto* bytes = reinterpret_cast<std::byte*>(this);

		for (const auto offset : { offsetof(CDesktop, mBackgroundElementDrawListStorage), offsetof(CDesktop, mWindowDrawListStorage), offsetof(CDesktop, mOverlayElementDrawListStorage), offsetof(CDesktop, mOverlayWindowDrawListStorage) })
		{
			l_DrawElementList(*std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offset)), rectangle);
		}
	}

	// Original function: FUN_004779ab; clipped bitmap lifetime spans drawing,
	// child traversal and overlays. Flags are reloaded after the draw callback.
	void CDesktop::l_DrawElementList(NXBasics::CListBase& list, const NXBasics::SRectangle& rectangle)
	{
		NXBasics::CListBase::Iterator iterator(list);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) != 0)
				continue;

			auto area = BaseElement_GetRectangle(element);

			if (!rectangle.IsTouching(area))
				continue;

			area.CutInside(rectangle);
			NXBasics::CBitmap clipped(mDisplayBitmap, area);
			area.x = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) - Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::PositionX));
			area.y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) - Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::PositionY));
			using TDraw = void(__thiscall*)(void*, NXBasics::CBitmap*, const NXBasics::SRectangle*);
			Read<TDraw*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[3](element, &clipped, &area);

			if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit6) != 0)
			{
				BaseWindow_DrawAllChildElements(element, &clipped, area);
				Read<TDraw*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[18](element, &clipped, &area);
			}
		}
	}

	// Original function: FUN_00477ec4; the modal byte suppresses ordinary lists.
	CBaseElement* CDesktop::l_Element_FindOnPosition(const std::int32_t* point, bool includeDisabled, std::uint32_t messageMask)
	{
		auto* bytes = reinterpret_cast<std::byte*>(this);
		auto findAt = [this, bytes, point, includeDisabled, messageMask](std::size_t offset)
		{

			return l_Element_FindInList(*std::launder(reinterpret_cast<NXBasics::CListBase*>(bytes + offset)), point, includeDisabled, messageMask);
		};

		if (auto* element = findAt(offsetof(CDesktop, mOverlayWindowHitTestListStorage)))
			return element;

		if (mSkipOrdinaryHitTestLists == 0)
			if (auto* element = findAt(offsetof(CDesktop, mOverlayElementHitTestListStorage)))
				return element;

		if (auto* element = findAt(offsetof(CDesktop, mWindowHitTestListStorage)))
			return element;

		return mSkipOrdinaryHitTestLists == 0 ? findAt(offsetof(CDesktop, mBackgroundElementHitTestListStorage)) : nullptr;
	}

	// Original function: FUN_00477f3d; BE_CanBeActivated precedes visibility,
	// message-mask, bounding rectangle, virtual coordinate conversion and hit test.
	CBaseElement* CDesktop::l_Element_FindInList(NXBasics::CListBase& list, const std::int32_t* point, bool includeDisabled, std::uint32_t messageMask)
	{
		NXBasics::CListBase::Iterator iterator(list);

		for (void* element = iterator.First(); element != nullptr; element = iterator.Next())
		{
			const auto flags = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags);

			if (!includeDisabled && !BaseElement_CanBeActivated(element))
				continue;

			if ((flags & NXBasics::BitMask::Bit9Unsigned) != 0 || (messageMask != 0 && (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::MessageMask) & messageMask) == 0))
				continue;

			if (!DesktopPointInside(point, element))
				continue;

			std::int32_t localPoint[2] = { point[0], point[1] };
			using TConvert = void(__thiscall*)(void*, std::int32_t*);
			Read<TConvert*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[11](element, localPoint);
			using THit = bool(__thiscall*)(void*, const std::int32_t*);

			if (Read<THit*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[8](element, localPoint))
				return static_cast<CBaseElement*>(element);
		}

		return nullptr;
	}

	// Original function: FUN_00477fd7.
	// Original function: FUN_004780fc.
	CBaseElement* CDesktop::Element_GetUnderMousePtr()
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));

		return l_Element_FindOnPositionDeep(point, true);
	}

	CBaseElement* CDesktop::l_Element_FindOnPositionDeep(const std::int32_t* point, bool includeDisabled)
	{
		auto* element = l_Element_FindOnPosition(point, includeDisabled, 0);

		if (element == nullptr)
			return nullptr;

		if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit6) != 0)
		{
			std::int32_t localPoint[2] = { point[0], point[1] };
			using TConvert = void(__thiscall*)(void*, std::int32_t*);
			Read<TConvert*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[11](element, localPoint);

			if (void* child = BaseWindow_FindElementOnPosition(element, localPoint, includeDisabled))
				return static_cast<CBaseElement*>(child);
		}

		return element;
	}

	// Original function: FUN_00478029.
	CBaseElement* CDesktop::l_Element_GetRealActivePtr() const
	{
		if (mActiveElement == nullptr)
			return nullptr;

		using TActive = CBaseElement*(__thiscall*)(void*);

		return Read<TActive*>(mActiveElement, NC2Layout::GuiBaseElementOffset::VirtualTable)[9](mActiveElement);
	}

	// Original function: FUN_0047804d; callbacks can change active/handler fields,
	// so each later target is read only after the previous dispatch returns.
	void CDesktop::l_Message_Send(std::uint32_t message, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4)
	{
		if (mPrimaryMessageHandler != nullptr && (Read<std::uint32_t>(mPrimaryMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
			if (SendDesktopElementMessage(mPrimaryMessageHandler, message, param1, param2, param3, param4))
				return;

		if (mActiveElement != nullptr && (Read<std::uint32_t>(mActiveElement, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit6) != 0)
		{
			void* child = Read<void*>(mActiveElement, NC2Layout::GuiBaseWindowOffset::ActiveChild);

			if (child != nullptr && (Read<std::uint32_t>(child, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
				if (SendDesktopElementMessage(child, message, param1, param2, param3, param4))
					return;
		}

		if (mActiveElement != nullptr && (Read<std::uint32_t>(mActiveElement, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
			if (SendDesktopElementMessage(mActiveElement, message, param1, param2, param3, param4))
			{

				return;
			}

		if (mFallbackMessageHandler != nullptr && (Read<std::uint32_t>(mFallbackMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
			SendDesktopElementMessage(mFallbackMessageHandler, message, param1, param2, param3, param4);
	}

	// Original function: FUN_00477d80.
	void CDesktop::l_Message_HandleKeyBoard(bool suppressInput)
	{
		auto* keyboard = NXSysKeyManager::CKeyManager::sTheObjectPtr;

		if (suppressInput)
		{
			keyboard->Init();

			return;
		}

		while (Read<std::uint32_t>(keyboard, NC2Layout::KeyManagerOffset::QueuedMessageCount) != 0)
		{
			NXSysKeyManager::SKeyMessage key;
			const auto kind = keyboard->GetKeyboardMessage(key);
			const std::uint32_t message = kind == 1 ? 0x200u : 0x400u;
			bool handled = false;

			if (mPrimaryMessageHandler != nullptr && (Read<std::uint32_t>(mPrimaryMessageHandler, NC2Layout::GuiBaseElementOffset::Flags) & 4u) != 0 && (Read<std::uint32_t>(mPrimaryMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
				handled = SendDesktopElementMessage(mPrimaryMessageHandler, message, key.keyCode, key.TextCharacter(), key.modifierFlags, 0);

			if (!handled && mKeyboardTargetRoot != nullptr && (Read<std::uint32_t>(mKeyboardTargetRoot, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
				handled = SendDesktopElementMessage(mKeyboardTargetRoot, message, key.keyCode, key.TextCharacter(), key.modifierFlags, 0);

			if (!handled && mFallbackMessageHandler != nullptr && (Read<std::uint32_t>(mFallbackMessageHandler, NC2Layout::GuiBaseElementOffset::Flags) & 4u) != 0 && (Read<std::uint32_t>(mFallbackMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & message) != 0)
				SendDesktopElementMessage(mFallbackMessageHandler, message, key.keyCode, key.TextCharacter(), key.modifierFlags, 0);

			keyboard = NXSysKeyManager::CKeyManager::sTheObjectPtr;
		}
	}

	// Original function: FUN_00477a82; mouse events are consumed before the
	// suppress-input return. Preserve left/double, release, middle/right, move
	// and wheel dispatch order, including callbacks that replace active elements.
	void CDesktop::l_Message_HandleMouse(bool suppressInput)
	{
		auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;

		if (Read<std::uint8_t>(mouse, NC2Layout::MouseManagerOffset::StateChanged) == 0)
			return;

		NXSysMouseManager::SMouseMessage event;
		mouse->GetNewMouseMessage(event);

		if (suppressInput)
		{

			return;
		}

		const std::int32_t point[2] = { event.mouseX, event.mouseY };
		using THasChildren = bool(__thiscall*)(void*);
		using TActivate = void(__thiscall*)(void*, const std::int32_t*);
		auto activateAtPoint = [this, &point]()
		{
			auto* element = l_Element_FindOnPosition(point, false, 0);

			if (element != nullptr)
			{
				Element_Activate(element);

				if (Read<THasChildren*>(mActiveElement, NC2Layout::GuiBaseElementOffset::VirtualTable)[1](mActiveElement))
					Read<TActivate*>(mActiveElement, NC2Layout::GuiBaseElementOffset::VirtualTable)[4](mActiveElement, point);
			}

			return element;
		};
		const auto x = static_cast<std::uint32_t>(event.mouseX), y = static_cast<std::uint32_t>(event.mouseY);

		if ((event.flags & NXBasics::BitMask::Bit2AndBit8) != 0)
		{
			mLeftButtonDown = 1;

			if (mPrimaryMessageHandler != nullptr)
			{
				if ((event.flags & 4u) != 0 && (Read<std::uint8_t>(mPrimaryMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & 1u) != 0)
					SendDesktopElementMessage(mPrimaryMessageHandler, 1, x, y, 0, 0);

				if ((event.flags & NXBasics::BitMask::Bit8) != 0 && (Read<std::uint8_t>(mPrimaryMessageHandler, NC2Layout::GuiBaseElementOffset::MessageMask) & 4u) != 0)
					SendDesktopElementMessage(mPrimaryMessageHandler, 4, x, y, 0, 0);
			}

			bool pressed = false;

			if (activateAtPoint() != nullptr)
			{
				const auto flags = Read<std::uint32_t>(mActiveElement, NC2Layout::GuiBaseElementOffset::Flags);

				if ((flags & NXBasics::BitMask::Bit10) == 0 && (flags & 2u) != 0)
				{
					using TPress = bool(__thiscall*)(void*, const std::int32_t*);
					pressed = Read<TPress*>(mActiveElement, NC2Layout::GuiBaseElementOffset::VirtualTable)[6](mActiveElement, point);
				}
			}

			if (!pressed)
			{
				if ((event.flags & 4u) != 0)
					l_Message_Send(1, x, y, 0, 0);

				if ((event.flags & NXBasics::BitMask::Bit8) != 0)
					l_Message_Send(4, x, y, 0, 0);
			}
		}

		if ((event.flags & 8u) != 0)
		{
			mLeftButtonDown = 0;
			using TRelease = bool(__thiscall*)(void*);

			if (mActiveElement == nullptr || !Read<TRelease*>(mActiveElement, NC2Layout::GuiBaseElementOffset::VirtualTable)[7](mActiveElement))
				l_Message_Send(2, x, y, 0, 0);
		}

		if ((event.flags & NXBasics::BitMask::Bit4AndBit6) != 0)
		{
			activateAtPoint();

			if ((event.flags & NXBasics::BitMask::Bit6) != 0)
			{
				mRightButtonDown = 1;
				l_Message_Send(0x20, x, y, 0, 0);
			}

			if ((event.flags & NXBasics::BitMask::Bit4) != 0)
				l_Message_Send(8, x, y, 0, 0);
		}

		if ((event.flags & NXBasics::BitMask::Bit7) != 0)
		{
			mRightButtonDown = 0;
			l_Message_Send(0x40, x, y, 0, 0);
		}

		if ((event.flags & NXBasics::BitMask::Bit5) != 0)
			l_Message_Send(0x10, x, y, 0, 0);

		if ((event.flags & 1u) != 0)
		{
			void* active = l_Element_GetRealActivePtr();

			if (active != nullptr && (Read<std::uint32_t>(active, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit4) != 0 && Read<std::uint32_t>(active, NC2Layout::GuiBaseElementOffset::InteractionState) == 2)
			{
				void* parent = Read<void*>(active, NC2Layout::GuiBaseElementOffset::ParentWindow);
				auto* areaOwner = parent != nullptr ? parent : active;
				auto area = BaseElement_GetRectangle(areaOwner);
				area.x = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) + static_cast<std::uint32_t>(event.deltaX));
				area.y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) + static_cast<std::uint32_t>(event.deltaY));
				Write(areaOwner, NC2Layout::GuiBaseElementOffset::Rectangle, area);
				area.PlaceInside(mRectangle);
				Write(areaOwner, NC2Layout::GuiBaseElementOffset::Rectangle, area);
			}
			else
			{
				auto* hit = l_Element_FindOnPositionDeep(point, false);

				if (hit != nullptr)
				{
					if ((Read<std::uint32_t>(hit, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) == 0)
					{
						if (mMouseOverElement != nullptr)
							BaseElement_MouseLeftArea(mMouseOverElement);

						mMouseOverElement = hit;
						BaseElement_MouseEnteredArea(hit);
					}
				}
				else if (mMouseOverElement != nullptr)
				{
					BaseElement_MouseLeftArea(mMouseOverElement);
					mMouseOverElement = nullptr;
				}
			}

			l_Message_Send(0x80, x, y, static_cast<std::uint32_t>(event.deltaX), static_cast<std::uint32_t>(event.deltaY));
		}

		if ((event.flags & 2u) != 0)
		{
			activateAtPoint();
			// 00477D67..00477D74: wheel handlers read GUI parameter 3.
			l_Message_Send(0x100, 0, 0, static_cast<std::uint32_t>(event.wheelDelta), 0);
		}
	}

	// Original function: FUN_00478122; Windows passes the rectangle by value and clamps it to the desktop.
	void CDesktop::Window_ChangeAreaRectangle(CBaseWindow* window, NXBasics::SRectangle rectangle)
	{
		rectangle.PlaceInside(mRectangle);
		std::memcpy(reinterpret_cast<std::byte*>(window) + NC2Layout::GuiBaseElementOffset::Rectangle, &rectangle, sizeof(rectangle));
		NC2Lifetime::Write(window, NC2Layout::GuiWindowClipSizeOffset::Width, rectangle.width);
		NC2Lifetime::Write(window, NC2Layout::GuiWindowClipSizeOffset::Height, rectangle.height);
	}

}