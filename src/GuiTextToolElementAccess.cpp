#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GuiClockToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager_Access.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XFontMetrics.h"
#include "XGuiElementAccess.h"
#include "XNumericConstants.h"
#include "XStringMemory.h"
#include "XTextEncoding.h"
#include <cstring>
#include <new>

namespace NC2InGameGuiBase
{
	using namespace NC2Lifetime;
	using namespace NXBaseGui;
	static void* __fastcall TextButton_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall TextButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4);
	static void __fastcall TextButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall TextButton_ToolTip(void* element, void*, const char** output);
	static bool __fastcall TextButton_ToolTipAtOnce(void* element);
	static void __fastcall TextButton_DoPressedAction(void* element);
	// Complete eighteen-entry Windows table 004F6DE8..004F6E2F.
	const std::uintptr_t gTextButtonVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(&TextButton_Delete),
		reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&TextButton_Message),
		reinterpret_cast<std::uintptr_t>(&TextButton_Draw),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&TextButton_ToolTip),
		reinterpret_cast<std::uintptr_t>(&TextButton_ToolTipAtOnce),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&TextButton_DoPressedAction)
	};

	static void __fastcall TextElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	// Complete table 004F6DA0..004F6DE7.
	static const std::uintptr_t gTextElementVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(&BaseToolElement_Delete),
		reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Message_Handle),
		reinterpret_cast<std::uintptr_t>(&TextElement_Draw),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Activate),
		reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify)
	};

	// Original function: FUN_004b8b51.
	void* __fastcall BaseToolTextElement_Construct(void* element, void*, const NXBasics::SRectangle* rectangle, const char* caption, bool fillBackground, bool wrapText)
	{
		BaseElement_Construct(element, rectangle, 0, 0);
		Write<std::uint8_t>(element, 0x2C, 0);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTextElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiTextElementOffset::Caption, 0, NC2Layout::RecordSize::GuiTextElementVars);
		NXBasics::Text_Copy(static_cast<char*>(element) + NC2Layout::GuiTextElementOffset::Caption, NC2Runtime::TextElementLimits::CaptionCharacters + 1u, caption);
		Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::FillBackgroundFlag, fillBackground);
		Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::WrapTextFlag, wrapText);
		Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CaptionTerminator, 0);
		Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CenteredFlag, 1);
		return element;
	}

	// Original function: FUN_004b8bc5; the fixed caption buffer is explicitly terminated.
	void BaseToolTextElement_SetCaptionString(void* element, const char* caption)
	{
		NXBasics::Text_Copy(static_cast<char*>(element) + NC2Layout::GuiTextElementOffset::Caption, NC2Runtime::TextElementLimits::CaptionCharacters + 1u, caption);
		Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CaptionTerminator, 0);
	}

	// Original function: FUN_004b8be8.
	static void __fastcall TextElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		if (Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::FillBackgroundFlag) != 0) NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, 2);
		if (Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::DrawFrameFlag) != 0) NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, 2);
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::SmallFontFlag) != 0 ? resources.font08 : resources.font10;
		const auto* color = Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::DarkFontFlag) != 0 ? NC2GuiToolsBase::DrawTool_GetDarkFontColorRef() : NC2GuiToolsBase::DrawTool_GetFontColorRef();
		const char* caption = static_cast<const char*>(element) + NC2Layout::GuiTextElementOffset::Caption;
		if (Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::WrapTextFlag) != 0)
		{
			NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 2, 2, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width), font, color, caption);
		}
		else if (Read<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CenteredFlag) != 0)
		{
			const auto textWidth = static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, caption));
			const auto fontHeight = Read<std::uint32_t>(font, NC2Layout::FontOffset::Height);
			const auto x = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - textWidth) / 2) - static_cast<std::uint32_t>(rectangle->x));
			const auto y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - fontHeight) / 2) - static_cast<std::uint32_t>(rectangle->y) - 3u);
			bitmap->Text_Print(font, color, x, y, caption);
		}
		else NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, NXBasics::Runtime_TimeSigned32(2u - static_cast<std::uint32_t>(rectangle->x)), NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->y)), font, color, caption);
	}

	// Shared constructor sequence established separately in all four Windows entries.
	static void TextButton_ConstructMembers(void* element, const NXBasics::SRectangle* rectangle, const char* caption)
	{
		BaseElement_Construct(element, rectangle, 2, 0x140);
		Write<std::uint8_t>(element, 0x2C, 0);
		new (static_cast<std::byte*>(element) + NC2Layout::GuiTextButtonOffset::SolidColor) NXBasics::SColorRGB();
		std::memset(static_cast<std::byte*>(element) + NC2Layout::GuiTextButtonOffset::Caption, 0, NC2Layout::RecordSize::GuiTextButtonVars);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTextButtonVirtualTable + 0);
		BaseToolTextButton_SetCaptionString(element, caption);
	}

	// Original function: FUN_004b8da2.
	void* __fastcall BaseToolTextButton_Construct(void* element, void*, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler)
	{
		TextButton_ConstructMembers(element, rectangle, caption);
		Write(element, NC2Layout::GuiTextButtonOffset::Command, command);
		Write(element, NC2Layout::GuiTextButtonOffset::Receiver, handler);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 0);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 1);
		return element;
	}

	// Original function: FUN_004b8e2c.
	void* __fastcall BaseToolTextButton_ConstructWithToolTip(void* element, void*, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler)
	{
		TextButton_ConstructMembers(element, rectangle, caption);
		Write(element, NC2Layout::GuiTextButtonOffset::Command, command);
		Write(element, NC2Layout::GuiTextButtonOffset::Receiver, handler);
		if (tooltip != nullptr) Write(element, NC2Layout::GuiTextButtonOffset::Tooltip, NXBasics::StringDuplicate(tooltip));
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 0);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 1);
		return element;
	}

	// Original function: FUN_004b8eac.
	void* __fastcall BaseToolTextButton_ConstructToggle(void* element, void*, const NXBasics::SRectangle* rectangle, const char* caption, std::uint32_t command, void* handler, bool selected)
	{
		TextButton_ConstructMembers(element, rectangle, caption);
		Write(element, NC2Layout::GuiTextButtonOffset::Command, command);
		Write(element, NC2Layout::GuiTextButtonOffset::Receiver, handler);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, selected);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 1);
		return element;
	}

	// Original function: FUN_004b8f1c.
	void* __fastcall BaseToolTextButton_ConstructToggleWithToolTip(void* element, void*, const NXBasics::SRectangle* rectangle, const char* caption, const char* tooltip, std::uint32_t command, void* handler, bool selected)
	{
		TextButton_ConstructMembers(element, rectangle, caption);
		Write(element, NC2Layout::GuiTextButtonOffset::Command, command);
		Write(element, NC2Layout::GuiTextButtonOffset::Receiver, handler);
		if (tooltip != nullptr) Write(element, NC2Layout::GuiTextButtonOffset::Tooltip, NXBasics::StringDuplicate(tooltip));
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, selected);
		Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 1);
		return element;
	}

	// Original function: FUN_004b8f9e.
	void BaseToolTextButton_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gTextButtonVirtualTable + 0);
		FreeAndClearField(element, NC2Layout::GuiTextButtonOffset::Caption);
		FreeAndClearField(element, NC2Layout::GuiTextButtonOffset::Tooltip);
		BaseElement_Destruct(element);
	}

	// Original function: FUN_004b8e10.
	static void* __fastcall TextButton_Delete(void* element, void*, std::uint32_t flags)
	{
		BaseToolTextButton_Destruct(element);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(element);
		return element;
	}

	static std::int32_t Wrapped(std::uint32_t value)
	{
		return NXBasics::Runtime_TimeSigned32(value);
	}
	static const NXBasics::SColorRGB* FontColor()
	{
		// Original getter 004E264B returns the last color entry of the white palette.
		const auto* palette = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette;
		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(palette) + NC2Layout::PaletteOffset::LastColor);
	}

	// Original function: FUN_004b907a.
	static void __fastcall TextButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const bool disabled = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag) != 0;
		const auto mode = ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 && !disabled) ? 4 : (Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag) != 0 ? 3 : 2);
		if (Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::FillSolidColorFlag) != 0) bitmap->Fill(Read<NXBasics::SColorRGB>(element, NC2Layout::GuiTextButtonOffset::SolidColor));
		else NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, mode);
		NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, mode);
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SmallFontFlag) != 0 ? resources.font08 : resources.font10;
		const auto fontHeight = Read<std::uint32_t>(font, NC2Layout::FontOffset::Height);
		const auto width = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Width), height = Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Height);
		char* caption = Read<char*>(element, NC2Layout::GuiTextButtonOffset::Caption);
		const bool centered = Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag) != 0;
		if (Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::MultilineFlag) == 0)
		{
			if (Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SkipOverwideCaptionFlag) != 0 && NXBasics::Font_GetPixelWidth(font, caption) > Wrapped(width - 6u)) return;
			const auto y = Wrapped(static_cast<std::uint32_t>(Wrapped(height - fontHeight) / 2) - static_cast<std::uint32_t>(rectangle->y) - 3u);
			if (centered)
			{
				const auto textWidth = static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, caption));
				const auto x = Wrapped(static_cast<std::uint32_t>(Wrapped(width - textWidth) / 2) - static_cast<std::uint32_t>(rectangle->x));
				bitmap->Text_Print(font, FontColor(), x, y, caption);
			}
			else NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, Wrapped(2u - static_cast<std::uint32_t>(rectangle->x)), y, font, FontColor(), caption);
		}
		else
		{
			const auto spacing = fontHeight + 4u;
			const auto count = Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::LineCount);
			std::uint32_t y = static_cast<std::uint32_t>(Wrapped(height - spacing) / 2) - (count - 1u) * static_cast<std::uint32_t>(Wrapped(spacing) / 2) - static_cast<std::uint32_t>(rectangle->y) - 1u;
			for (std::uint32_t index = 0; index < count; ++index)
			{
				char* end = std::strstr(caption, "\r\n");
				if (end != nullptr) *end = '\0';
				const auto x = centered ? Wrapped(width - static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, caption))) / 2 : 2;
				bitmap->Text_Print(font, FontColor(), Wrapped(static_cast<std::uint32_t>(x) - static_cast<std::uint32_t>(rectangle->x)), Wrapped(y), caption);
				y += spacing;
				if (end != nullptr) *end = '\r';
				caption = reinterpret_cast<char*>(reinterpret_cast<std::uintptr_t>(end) + 2u);
			}
		}
		if (disabled)
		{
			NXBasics::SRectangle area = bitmap->GetRectangle();
			area.Inflate(-1, -1);
			bitmap->Tool_Darken(area, NXBasics::IntegerRange::Int8MaximumSigned, NXBasics::IntegerRange::Int8MaximumSigned, NXBasics::IntegerRange::Int8MaximumSigned);
		}
	}

	// Original function: FUN_004b92a4.
	static bool __fastcall TextButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4)
	{
		if (message == SwitchIds::TextButtonMessage::Command)
		{
			if (command == SwitchIds::TextButtonCommand::GetCommandParameters)
			{
				if (param2 != 0) Write(reinterpret_cast<void*>(param2), 0, Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::Command));
				if (param3 != 0) Write(reinterpret_cast<void*>(param3), 0, Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter1));
				if (param4 != 0) Write(reinterpret_cast<void*>(param4), 0, Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2));
				return true;
			}
			if (command == SwitchIds::TextButtonCommand::GetPlainCaption)
			{
				auto* output = reinterpret_cast<char*>(param2);
				if (output == nullptr || param3 == 0) return false;
				*output = '\0';
				const char* caption = Read<char*>(element, NC2Layout::GuiTextButtonOffset::Caption);
				if (caption == nullptr) return true;
				while (*caption == '{')
				{
					do
					{
						++caption;
					} while (*caption != '}' && *caption != '\0');
					if (*caption == '\0') return true;
					++caption;
				}
				NXBasics::Text_Copy(output, param3, caption);
				output[param3 - 1u] = '\0';
				if (char* brace = std::strchr(output, '{')) *brace = '\0';
				return true;
			}
		}
		else if (message == SwitchIds::TextButtonMessage::ForwardToParentHandler)
		{
			void* handler = Read<void*>(element, NC2Layout::GuiBaseElementOffset::MessageHandler);
			if (handler != nullptr)
			{
				using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
				return Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[NC2Runtime::GuiVirtualSlot::MessageHandle](handler, message, command, param2, param3, param4);
			}
		}
		else if (message == SwitchIds::TextButtonMessage::DoPressedAction)
		{
			using TAction = void(__thiscall*)(void*);
			Read<TAction*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[NC2Runtime::GuiVirtualSlot::DoPressedAction](element);
		}
		return false;
	}

	// Original function: FUN_004b936e.
	static void __fastcall TextButton_DoPressedAction(void* element)
	{
		void* handler = Read<void*>(element, NC2Layout::GuiTextButtonOffset::Receiver);
		if (handler == nullptr || Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag) != 0) return;
		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[NC2Runtime::GuiVirtualSlot::MessageHandle](handler, SwitchIds::TextButtonMessage::Command, Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::Command), reinterpret_cast<std::uint32_t>(element), Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter1), Read<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2));
		NC2GuiToolsBase::FXTool_ClickConfirm();
	}

	// Original function: FUN_004b9397.
	static bool __fastcall TextButton_ToolTip(void* element, void*, const char** output)
	{
		const char* tooltip = Read<char*>(element, NC2Layout::GuiTextButtonOffset::Tooltip);
		if (tooltip == nullptr) return false;
		*output = tooltip;
		return true;
	}

	// Original function: FUN_004a5ac6; shared tooltip-immediacy byte getter.
	static bool __fastcall TextButton_ToolTipAtOnce(void* element)
	{
		return Read<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::ImmediateTooltipFlag) != 0;
	}
}
