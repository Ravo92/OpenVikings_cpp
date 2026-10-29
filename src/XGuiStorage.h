#pragma once

#include "XBasicTypes.h"
#include "XList.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
}

namespace NXBaseGui
{
	class CBaseElement;

	// Windows physical GUI prefixes; these models do not own or construct child lists.
	struct SBaseElementStorage
	{
		const void* virtualTable;
		NXBasics::SRectangle rectangle;
		std::uint32_t interactionState;
		std::uint32_t flags;
		std::uint32_t messageMask;
		void* parentWindow;
		void* messageHandler;
		std::uint32_t serial;
	};

	struct SBaseWindowStorage
	{
		SBaseElementStorage element;
		NXBasics::SRectangle clippingRectangle;
		alignas(NXBasics::CListBase) std::byte drawChildren[sizeof(NXBasics::CListBase)];
		alignas(NXBasics::CListBase) std::byte inputChildren[sizeof(NXBasics::CListBase)];
		void* activeChild;
		std::uint8_t desktopListFlags;
		std::byte unresolved69[3];
	};

	// The desktop owns the lifetimes of its embedded lists; offsets are runtime-only.
	struct SDesktopStorage
	{
		NXBasics::SRectangle mRectangle;                                                                          // +0x000
		NXBasics::CBitmap* mDisplayBitmap;                                                                        // +0x010
		alignas(NXBasics::CListBase) std::byte mBackgroundElementDrawListStorage[sizeof(NXBasics::CListBase)];    // +0x014
		alignas(NXBasics::CListBase) std::byte mBackgroundElementHitTestListStorage[sizeof(NXBasics::CListBase)]; // +0x028
		alignas(NXBasics::CListBase) std::byte mOverlayElementDrawListStorage[sizeof(NXBasics::CListBase)];       // +0x03C
		alignas(NXBasics::CListBase) std::byte mOverlayElementHitTestListStorage[sizeof(NXBasics::CListBase)];    // +0x050
		alignas(NXBasics::CListBase) std::byte mWindowOrderListStorage[sizeof(NXBasics::CListBase)];              // +0x064
		alignas(NXBasics::CListBase) std::byte mWindowMessageListStorage[sizeof(NXBasics::CListBase)];            // +0x078
		alignas(NXBasics::CListBase) std::byte mWindowDrawListStorage[sizeof(NXBasics::CListBase)];               // +0x08C
		alignas(NXBasics::CListBase) std::byte mWindowHitTestListStorage[sizeof(NXBasics::CListBase)];            // +0x0A0
		alignas(NXBasics::CListBase) std::byte mOverlayWindowOrderListStorage[sizeof(NXBasics::CListBase)];       // +0x0B4
		alignas(NXBasics::CListBase) std::byte mOverlayWindowMessageListStorage[sizeof(NXBasics::CListBase)];     // +0x0C8
		alignas(NXBasics::CListBase) std::byte mOverlayWindowDrawListStorage[sizeof(NXBasics::CListBase)];        // +0x0DC
		alignas(NXBasics::CListBase) std::byte mOverlayWindowHitTestListStorage[sizeof(NXBasics::CListBase)];     // +0x0F0
		CBaseElement* mActiveElement;                                                                             // +0x104
		CBaseElement* mMouseOverElement;                                                                          // +0x108
		CBaseElement* mKeyboardTargetRoot;                                                                        // +0x10C; root selected by element flag 0x04.
		CBaseElement* mPrimaryMessageHandler;                                                                     // +0x110
		void* mFallbackMessageHandler;                                                                            // +0x114; owned fallback element for unhandled messages.
		alignas(NXBasics::CListBase) std::byte mWindowCloseListStorage[sizeof(NXBasics::CListBase)];              // +0x118; deferred window-close list.
		std::uint8_t mLeftButtonDown;                                                                             // +0x12C
		std::uint8_t mRightButtonDown;                                                                            // +0x12D
		std::byte mUnclassified012E[2];
		std::uint32_t mWorkResult;              // +0x130; cleared and returned by Desktop_DoWork.
		void* mMousePointerStorage;             // +0x134
		std::uint8_t mSkipOrdinaryHitTestLists; // +0x138; skips lists +0x50/+0x28 during hit testing.
		std::byte mUnclassified0139[3];
	};

	static_assert(sizeof(SBaseElementStorage) == (sizeof(void*) == 8 ? 64 : 44));
	static_assert(sizeof(SBaseWindowStorage) == (sizeof(void*) == 8 ? 176 : 108));
	static_assert(sizeof(SDesktopStorage) == (sizeof(void*) == 8 ? 608 : 316));
}

namespace NC2InGameGuiBase
{
	struct SBaseToolElementStorage
	{
		NXBaseGui::SBaseElementStorage element;
		std::byte unresolvedState;
	};

	struct SBaseToolWindowStorage
	{
		NXBaseGui::SBaseWindowStorage window;
		std::uint32_t backgroundStyle;
	};

	static_assert(sizeof(SBaseToolWindowStorage) == (sizeof(void*) == 8 ? 184 : 112));
	static_assert(sizeof(SBaseToolElementStorage) == (sizeof(void*) == 8 ? 72 : 48));
}