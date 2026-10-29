#include "C2LifetimeAccess.h"
#include "XTextEncoding.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiClockToolElementAccess.h"
#include "GuiToolsBase.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiElementAccess.h"
#include "XNumericConstants.h"
#include "XStringMemory.h"

#include <cstring>
#include <new>

// Untranslated original base/widget interfaces. EDX is unused so the stack
// parameters and callee cleanup match Windows __thiscall dispatch.

namespace NC2InGameGuiBase
{
	template <class T>
	static T BitmapTool_Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

		return value;
	}

	template <typename T>
	static void BitmapTool_Write(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	// Original function: FUN_00478a79; l_BE_GlobalToLocalCoordinates.
	void BaseElement_GlobalToLocalCoordinates(const void* element, std::int32_t* point)
	{
		const auto subtract = [](std::int32_t value, std::int32_t offset)
		{

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) - static_cast<std::uint32_t>(offset));
		};

		if (const void* parent = BitmapTool_Read<void*>(element, NC2Layout::GuiBaseElementOffset::ParentWindow))
		{
			const std::int32_t y = BitmapTool_Read<std::int32_t>(parent, NC2Layout::GuiBaseElementOffset::PositionY);
			const std::int32_t x = BitmapTool_Read<std::int32_t>(parent, NC2Layout::GuiBaseElementOffset::PositionX);
			point[0] = subtract(point[0], x);
			point[1] = subtract(point[1], y);
		}

		const std::int32_t y = BitmapTool_Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::PositionY);
		const std::int32_t x = BitmapTool_Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::PositionX);
		point[0] = subtract(point[0], x);
		point[1] = subtract(point[1], y);
	}

	// Original virtual entry: FUN_004b8af7.
	static void* __fastcall BaseBitmapToolElement_DeletingDestructor(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void)unusedEdx;
		BaseBitmapToolElement_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// ABI adapter for FUN_004b8b33.
	static void __fastcall BaseBitmapToolElement_VirtualDraw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle)
	{
		(void)unusedEdx;
		BaseBitmapToolElement_Draw(element, bitmap, rectangle);
	}

	// Complete original base table: 004F6D58..004F6D9F.
	static const std::uintptr_t gBaseBitmapToolElementVirtualTable[] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseBitmapToolElement_DeletingDestructor>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseBitmapToolElement_VirtualDraw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Activate>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};
	static_assert(sizeof(gBaseBitmapToolElementVirtualTable) == 18 * sizeof(std::uintptr_t));

	// Original function: FUN_004b8aa0; CBaseBitmapToolElement constructor.
	void* BaseBitmapToolElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId, std::int32_t depth)
	{
		NXBaseGui::BaseElement_Construct(element, rectangle, flags, elementId);
		BitmapTool_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseBitmapToolElementVirtualTable + 0);
		BitmapTool_Write<void*>(element, sizeof(NXBaseGui::SBaseElementStorage), nullptr);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap), 1);
		NXBasics::CBitmap* bitmap = allocation == nullptr ? nullptr : ::new (allocation) NXBasics::CBitmap(rectangle->width, rectangle->height, depth);
		BitmapTool_Write(element, sizeof(NXBaseGui::SBaseElementStorage), bitmap);

		return element;
	}

	// Original function: FUN_004b8b13; CBaseBitmapToolElement destructor.
	void BaseBitmapToolElement_Destruct(void* element)
	{
		auto* bitmap = BitmapTool_Read<NXBasics::CBitmap*>(element, sizeof(NXBaseGui::SBaseElementStorage));
		BitmapTool_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseBitmapToolElementVirtualTable + 0);

		if (bitmap != nullptr)
			delete bitmap;

		NXBaseGui::BaseElement_Destruct(element);
	}

	// Original function: FUN_004b8b33.
	// Original method: NC2InGameGuiBase::CBaseBitmapToolElement::XGui_BE_Element_Draw.
	void BaseBitmapToolElement_Draw(const void* element, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		auto* source = BitmapTool_Read<NXBasics::CBitmap*>(element, sizeof(NXBaseGui::SBaseElementStorage));
		const std::int32_t x = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->x));
		const std::int32_t y = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->y));
		source->CopyIntoBitmap(bitmap, x, y);
	}

	using namespace NC2Lifetime;
	using namespace NXBaseGui;
	static void* __fastcall GfxButton_Delete(void* element, void*, std::uint32_t flags);
	static void* __fastcall CloserButton_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall GfxButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t param2, std::uintptr_t param3, std::uintptr_t param4);
	static void __fastcall GfxButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall GfxButton_IsHit(void* element, void*, const std::int32_t* point);
	static bool __fastcall GfxButton_ToolTip(void* element, void*, const char** output);
	static bool __fastcall GfxButton_ToolTipAtOnce(void* element);
	static void __fastcall GfxButton_DoPressedAction(void* element);
	// Complete Windows tables: ordinary graphics button 004F6E78..004F6EBF,
	// closer button 004F6F08..004F6F4F; only the deleting destructor differs.
	static const std::uintptr_t gGfxButtonVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_Delete>()), reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_Message>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_Draw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_Activate>()), reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()), reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_IsHit>()), reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_ToolTip>()),
		reinterpret_cast<std::uintptr_t>(&GfxButton_ToolTipAtOnce), reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()), reinterpret_cast<std::uintptr_t>(&GfxButton_DoPressedAction)
	};
	static const std::uintptr_t gCloserButtonVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&CloserButton_Delete>()), reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_Message>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_Draw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_Activate>()), reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()), reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_IsHit>()), reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxButton_ToolTip>()),
		reinterpret_cast<std::uintptr_t>(&GfxButton_ToolTipAtOnce), reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()), reinterpret_cast<std::uintptr_t>(&GfxButton_DoPressedAction)
	};

	static void* __fastcall GfxElement_Delete(void* element, void*, std::uint32_t flags);
	static void __fastcall GfxElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	// Complete eighteen-entry table 004F6E30..004F6E77.
	static const std::uintptr_t gGfxElementVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxElement_Delete>()), reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_Message_Handle>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GfxElement_Draw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_Activate>()), reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()), reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_IsHit>()), reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_AtOnce), reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()), reinterpret_cast<std::uintptr_t>(&BaseElement_Notify)
	};

	// Original function: FUN_004b93ad.
	void* __fastcall BaseToolGfxElement_Construct(void* element, void*, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId)
	{
		BaseElement_Construct(element, rectangle, 0, 0);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gGfxElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiGfxButtonOffset::GraphicsId, 0, NC2Layout::RecordSize::GuiGfxElementVars);
		Write(element, NC2Layout::GuiGfxButtonOffset::GraphicsId, graphicsId);
		Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered, 1);

		return element;
	}

	// Original function: FUN_004b93e5.
	static void* __fastcall GfxElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gGfxElementVirtualTable + 0);
		BaseElement_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004b9411.
	static void __fastcall GfxElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
	{
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* palette = Read<NXBasics::CPalette*>(element, NC2Layout::GuiGfxButtonOffset::Palette);

		if (palette == nullptr)
			palette = resources.backgroundPalette;

		const auto id = Read<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::GraphicsId);
		const auto type = NXBasics::BobManager_GetBobType(resources.windowBobManager, id);
		const bool centered = Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered) != 0;
		const auto x = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2 : 0;
		const auto y = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) / 2 : 0;

		if (type == 4)
		{
			NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, id, bitmap, x, y, palette, 128);
		}
		else
		{
			NXBasics::BobManager_PrintBob(resources.windowBobManager, id, bitmap, x, y, palette);
		}
	}

	// Original function: FUN_004b959a.
	void __fastcall BaseToolGfxButton_SetGraphicsId(void* element, void*, std::uint32_t graphicsId)
	{
		Write(element, NC2Layout::GuiGfxButtonOffset::GraphicsId, graphicsId);
	}

	// Original function: FUN_004b94b7.
	void* __fastcall BaseToolGfxButton_Construct(void* element, void*, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId, std::uint32_t command, void* handler)
	{
		BaseElement_Construct(element, rectangle, 2, 0x140);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gGfxButtonVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiGfxButtonOffset::GraphicsId, 0, NC2Layout::RecordSize::GuiGfxButtonVars);
		Write(element, NC2Layout::GuiGfxButtonOffset::GraphicsId, graphicsId);
		Write(element, NC2Layout::GuiGfxButtonOffset::Command, command);
		Write(element, NC2Layout::GuiGfxButtonOffset::Receiver, handler);
		Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered, 1);

		return element;
	}

	// Original function: FUN_004b9520; preserve its distinct tooltip allocation.
	void* __fastcall BaseToolGfxButton_ConstructWithToolTip(void* element, void*, const NXBasics::SRectangle* rectangle, std::uint32_t graphicsId, const char* tooltip, std::uint32_t command, void* handler)
	{
		BaseElement_Construct(element, rectangle, 2, 0x140);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gGfxButtonVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiGfxButtonOffset::GraphicsId, 0, NC2Layout::RecordSize::GuiGfxButtonVars);
		Write(element, NC2Layout::GuiGfxButtonOffset::GraphicsId, graphicsId);
		Write(element, NC2Layout::GuiGfxButtonOffset::Command, command);
		Write(element, NC2Layout::GuiGfxButtonOffset::Receiver, handler);

		if (tooltip != nullptr)
			Write(element, NC2Layout::GuiGfxButtonOffset::Tooltip, NXBasics::StringDuplicate(tooltip));

		Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered, 1);

		return element;
	}

	// Original function: FUN_004b95ae.
	void BaseToolGfxButton_SetToolTipString(void* element, const char* tooltip)
	{
		FreeAndClearField(element, NC2Layout::GuiGfxButtonOffset::Tooltip);

		if (tooltip != nullptr)
			Write(element, NC2Layout::GuiGfxButtonOffset::Tooltip, NXBasics::StringDuplicate(tooltip));
	}

	// Original function: FUN_004b9961.
	void* __fastcall WindowCloserButton_Construct(void* element, void*, const NXBasics::SRectangle* rectangle)
	{
		BaseToolGfxButton_Construct(element, nullptr, rectangle, 0x11, 0, nullptr);
		Write(element, NC2Layout::GuiBaseElementOffset::Flags, Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) | 8u);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gCloserButtonVirtualTable + 0);
		BaseToolGfxButton_SetToolTipString(element, NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(0x8D));

		return element;
	}

	// Original function: FUN_004b957b.
	static void GfxButton_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gGfxButtonVirtualTable + 0);
		FreeAndClearField(element, NC2Layout::GuiGfxButtonOffset::Tooltip);
		BaseElement_Destruct(element);
	}

	// Original function: FUN_004b9504.
	static void* __fastcall GfxButton_Delete(void* element, void*, std::uint32_t flags)
	{
		GfxButton_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004b9996.
	static void* __fastcall CloserButton_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gCloserButtonVirtualTable + 0);
		GfxButton_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004b95d6; the draw callback's rectangle is forwarded
	// to the background/frame helpers, while the Bob coordinates remain local.
	static void __fastcall GfxButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const bool highlighted = (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 && Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled) == 0;

		if (Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle) != 0)
		{
			const auto mode = highlighted ? 4 : (Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Selected) != 0 ? 3 : 2);
			NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, mode);
			NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		}

		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* palette = Read<NXBasics::CPalette*>(element, NC2Layout::GuiGfxButtonOffset::Palette);

		if (palette == nullptr)
			palette = resources.backgroundPalette;

		const auto bobId = Read<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::GraphicsId);
		const auto type = NXBasics::BobManager_GetBobType(resources.windowBobManager, bobId);
		const bool centered = Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered) != 0;
		const auto x = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2 : 0;
		const auto y = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) / 2 : 0;

		if (type == 4)
		{
			NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, bobId, bitmap, x, y, palette, highlighted ? 0xC0 : 0x80);
		}
		else if (highlighted)
		{
			NXBasics::BobManager_PrintBob_Shade_SSE2(resources.windowBobManager, bobId, bitmap, x, y, palette, 0xC0);
		}
		else
		{
			NXBasics::BobManager_PrintBob(resources.windowBobManager, bobId, bitmap, x, y, palette);
		}
	}

	// Original function: FUN_004b974c.
	static bool __fastcall GfxButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t param2, std::uintptr_t param3, std::uintptr_t param4)
	{
		if (message == 0x800 && command == 0x7F0)
		{
			if (param2 != 0)
				Write(reinterpret_cast<void*>(param2), 0, Read<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::Command));

			if (param3 != 0)
				Write<std::uintptr_t>(reinterpret_cast<void*>(param3), 0, Read<std::uintptr_t>(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1));

			if (param4 != 0)
				Write<std::uintptr_t>(reinterpret_cast<void*>(param4), 0, Read<std::uintptr_t>(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2));

			return true;
		}

		if (message == 0x100)
		{
			void* handler = Read<void*>(element, NC2Layout::GuiBaseElementOffset::MessageHandler);

			if (handler != nullptr)
			{
				using TMessage = NXBaseGui::TGuiMessage;
				return Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](handler, message, command, param2, param3, param4);
			}
		}
		else if (message == 0x40)
		{
			using TAction = void(__thiscall*)(void*);
			Read<TAction*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[17](element);
		}

		return false;
	}

	// Original function: FUN_004b97bf.
	static bool __fastcall GfxButton_IsHit(void* element, void*, const std::int32_t* point)
	{
		if (Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle) != 0)
			return true;

		const bool centered = Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered) != 0;
		const auto x = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2 : 0;
		const auto y = centered ? Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) / 2 : 0;
		const std::int32_t localPoint[2] = { NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[0]) - static_cast<std::uint32_t>(x)), NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(point[1]) - static_cast<std::uint32_t>(y)) };

		return NXBasics::BobManager_IsBobHit(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.windowBobManager, Read<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::GraphicsId), localPoint);
	}

	// Original function: FUN_004b982a.
	static void __fastcall GfxButton_DoPressedAction(void* element)
	{
		void* handler = Read<void*>(element, NC2Layout::GuiGfxButtonOffset::Receiver);

		if (handler == nullptr || Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled) != 0)
		{

			return;
		}

		using TMessage = NXBaseGui::TGuiMessage;
		Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](handler, 0x800, Read<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::Command), reinterpret_cast<std::uintptr_t>(element), Read<std::uintptr_t>(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1), Read<std::uintptr_t>(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2));
		NC2GuiToolsBase::FXTool_ClickConfirm();
	}

	// Original function: FUN_004b9853.
	static bool __fastcall GfxButton_ToolTip(void* element, void*, const char** output)
	{
		const char* tooltip = Read<char*>(element, NC2Layout::GuiGfxButtonOffset::Tooltip);

		if (tooltip == nullptr)
			return false;

		*output = tooltip;

		return true;
	}

	// Original function: FUN_004b9500.
	static bool __fastcall GfxButton_ToolTipAtOnce(void* element)
	{

		return Read<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip) != 0;
	}

	static void __fastcall WindowMoverButton_Draw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall WindowMoverButton_ToolTip(void* element, void* unusedEdx, const char** output);

	// Complete original table: 004F6EC0..004F6F07.
	static const std::uintptr_t gWindowMoverButtonVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&BaseToolElement_Delete>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WindowMoverButton_Draw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Activate>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WindowMoverButton_ToolTip>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify)
	};
	static_assert(sizeof(gWindowMoverButtonVirtualTable) == 18 * sizeof(std::uintptr_t));

	// Original function: FUN_004b9869; CWindowMoverButton constructor.
	void* WindowMoverButton_Construct(void* element, const NXBasics::SRectangle* rectangle, const char* caption)
	{
		NXBaseGui::BaseElement_Construct(element, rectangle, 0x12, 0);
		BitmapTool_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolElementVirtualTable + 0);
		BitmapTool_Write<std::uint8_t>(element, sizeof(NXBaseGui::SBaseElementStorage), 0);
		BitmapTool_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWindowMoverButtonVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + offsetof(SGuiWindowMoverStorage, caption), 0, 200);
		// Original CRT entry 004E66A0 is strncpy, including its zero padding.
		NXBasics::Text_Copy(static_cast<char*>(element) + offsetof(SGuiWindowMoverStorage, caption), 200, caption);

		return element;
	}

	// Original function: FUN_004b98bc.
	static void __fastcall WindowMoverButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		using namespace NC2Lifetime;
		const auto mode = (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 ? 4 : 2;
		NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, mode);
		NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		auto* font = NC2GuiToolsBase::DrawTool_GetFont10Ptr();
		const char* caption = static_cast<const char*>(element) + offsetof(SGuiWindowMoverStorage, caption);
		const auto textWidth = NXBasics::Font_GetPixelWidth(font, caption);
		const auto y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - Read<std::uint32_t>(font, NC2Layout::FontOffset::Height)) / 2) - 3u);
		const auto x = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - static_cast<std::uint32_t>(textWidth)) / 2;
		bitmap->Text_Print(font, NC2GuiToolsBase::DrawTool_GetFontColorRef(), x, y, caption);
	}

	// Original function: FUN_004b994b.
	static bool __fastcall WindowMoverButton_ToolTip(void*, void*, const char** output)
	{
		*output = NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(0x8C);

		return true;
	}
}