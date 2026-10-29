#include "XPalette.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GuiClockToolElementAccess.h"
#include "GuiToolsBase.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiElementAccess.h"
#include <Windows.h>
#include <cmath>

namespace NC2InGameGuiBase
{
	using namespace NC2Lifetime;
	using namespace NXBaseGui;
	static void __fastcall BaseToolElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall ToolClockElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall ToolClockElement_ToolTip(void* element, void*, const char** output);
	// Complete original base table 004F6BE4..004F6C2B.
	const std::uintptr_t gBaseToolElementVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseToolElement_Delete>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseToolElement_Draw>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Activate>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify)
	};
	// Complete original clock table 004F6F50..004F6F97.
	static const std::uintptr_t gClockVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseToolElement_Delete>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&ToolClockElement_Draw>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_Activate>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&ToolClockElement_ToolTip>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(NativeGuiVirtualEntry<&BaseElement_UnresolvedVirtual40>()),
		reinterpret_cast<std::uintptr_t>(&BaseElement_Notify)
	};
	// Original function: FUN_004d0823; shared CBaseToolElement deleting destructor.
	void* __fastcall BaseToolElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolElementVirtualTable + 0);
		BaseElement_Destruct(element);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004b8a76.
	static void __fastcall BaseToolElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		NC2GuiToolsBase::DrawTool_FillArea(element, bitmap, rectangle, 2);
		NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, 2);
	}
	// Original function: FUN_004b8a46; shared CBaseToolElement construction.
	void* BaseToolElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t elementId)
	{
		BaseElement_Construct(element, rectangle, flags, elementId);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + offsetof(SBaseToolElementStorage, unresolvedState), 0, 1);
		return element;
	}
	// Original function: FUN_004b99b8; CToolClockElement constructor.
	void* ToolClockElement_Construct(void* element, const NXBasics::SRectangle* rectangle)
	{
		BaseElement_Construct(element, rectangle, 0, 0);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolElementVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + offsetof(SBaseToolElementStorage, unresolvedState), 0, 1);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gClockVirtualTable + 0);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::ClockElementOffset::AnalogMode, 0, NC2Layout::RecordSize::ClockInitializedSuffix);
		return element;
	}
	// Original empty exit callbacks are distinct entries.
	// Original function: FUN_004b9c5f.
	static int __cdecl ClockColourExit255()
	{
		return 0;
	}
	// Original function: FUN_004b9c60.
	static int __cdecl ClockColourExit200()
	{
		return 0;
	}
	// Original function: FUN_004b9c61.
	static int __cdecl ClockColourExit150()
	{
		return 0;
	}
	// Original function: FUN_004b9c62.
	static int __cdecl ClockColourExit100()
	{
		return 0;
	}
	// Complete lazy colour region 00569438..0056944B.
	struct SClockColourState
	{
		NXBasics::SColorRGB colour100{ 0, 0, 0 };
		NXBasics::SColorRGB colour150{ 0, 0, 0 };
		NXBasics::SColorRGB colour255{ 0, 0, 0 };
		NXBasics::SColorRGB colour200{ 0, 0, 0 };
		std::uint8_t flags = 0;
		std::byte padding[3]{ };
	};
	static SClockColourState gClockColours;
	static_assert(sizeof(SClockColourState) == 20);
	static void ClockColours_Initialize()
	{
		if ((gClockColours.flags & 1u) == 0)
		{
			gClockColours.flags |= 1u;
			gClockColours.colour100.SetColors(100, 100, 100);
			NXBasics::Runtime_AtExit(ClockColourExit100);
		}
		if ((gClockColours.flags & 2u) == 0)
		{
			gClockColours.flags |= 2u;
			gClockColours.colour150.SetColors(150, 150, 150);
			NXBasics::Runtime_AtExit(ClockColourExit150);
		}
		if ((gClockColours.flags & 4u) == 0)
		{
			gClockColours.flags |= 4u;
			gClockColours.colour200.SetColors(200, 200, 200);
			NXBasics::Runtime_AtExit(ClockColourExit200);
		}
		if ((gClockColours.flags & 8u) == 0)
		{
			gClockColours.flags |= 8u;
			gClockColours.colour255.SetColors(255, 255, 255);
			NXBasics::Runtime_AtExit(ClockColourExit255);
		}
	}
	// Original function: FUN_004b9c71; l_DrawPointer.
	static void ToolClockElement_DrawPointer(void* element, NXBasics::CBitmap* bitmap, float angle, const NXBasics::SColorRGB* colour, float start, float end, std::int32_t offsetX, std::int32_t offsetY)
	{
		const auto halfWidth = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2;
		const auto halfHeight = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) / 2;
		const float x = static_cast<float>(std::sin(static_cast<double>(angle)) * halfWidth);
		const double y = -(std::cos(static_cast<double>(angle)) * halfHeight);
		const auto coordinate = [] (double value, std::int32_t center, std::int32_t offset)
		{
			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(value)) + static_cast<std::uint32_t>(center) + static_cast<std::uint32_t>(offset));
		};
		const auto y2 = coordinate(y * end, halfHeight, offsetY);
		const auto x2 = coordinate(static_cast<double>(x) * end, halfWidth, offsetX);
		const auto y1 = coordinate(y * start, halfHeight, offsetY);
		const auto x1 = coordinate(static_cast<double>(x) * start, halfWidth, offsetX);
		bitmap->Draw_Line(x1, y1, x2, y2, *colour);
	}
	// Original function: FUN_004b99f7.
	static void __fastcall ToolClockElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const auto offsetX = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->x));
		const auto offsetY = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->y));
		SYSTEMTIME time;
		::GetLocalTime(&time);
		char* text = static_cast<char*>(element) + NC2Layout::ClockElementOffset::TimeText;
		NXBasics::Runtime_PrintString(text, "%d:%2.2d:%2.2d", time.wHour, time.wMinute, time.wSecond);
		if (Read<std::uint8_t>(element, NC2Layout::ClockElementOffset::AnalogMode) != 0)
		{
			ClockColours_Initialize();
			constexpr float fullTurn = 6.283185482025146484375f;
			float angle = 0.0f;
			do
			{
				ToolClockElement_DrawPointer(element, bitmap, angle, &gClockColours.colour100, 0.800000011920928955f, 1.0f, offsetX, offsetY);
				angle = static_cast<float>(static_cast<double>(angle) + 0.52359879016876220703125f);
			} while (angle < fullTurn);
			constexpr float sixtieth = 0.0166666675359010696f;
			constexpr float twelfth = 0.083333335816860199f;
			const float seconds = static_cast<float>(static_cast<double>(time.wSecond) * sixtieth * fullTurn);
			const float minutes = static_cast<float>(static_cast<double>(time.wMinute) * sixtieth * fullTurn + static_cast<double>(seconds) * sixtieth);
			const float hours = static_cast<float>(static_cast<double>(time.wHour % 12) * twelfth * fullTurn + static_cast<double>(minutes) * twelfth);
			ToolClockElement_DrawPointer(element, bitmap, seconds, &gClockColours.colour150, 0.0f, 1.0f, offsetX, offsetY);
			ToolClockElement_DrawPointer(element, bitmap, minutes, &gClockColours.colour200, 0.0f, 1.0f, offsetX, offsetY);
			ToolClockElement_DrawPointer(element, bitmap, hours, &gClockColours.colour255, 0.0f, 0.75f, offsetX, offsetY);
		}
		else
		{
			const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			auto* font = resources.font10;
			const auto width = NXBasics::Font_GetPixelWidth(font, text);
			const auto x = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - static_cast<std::uint32_t>(width)) / 2) + static_cast<std::uint32_t>(offsetX));
			const auto y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - Read<std::uint32_t>(font, NC2Layout::FontOffset::Height)) / 2) + static_cast<std::uint32_t>(offsetY) - 3u);
			const auto* colour = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
			bitmap->Text_Print(font, colour, x, y, text);
		}
	}
	// Original function: FUN_004b9c63.
	static bool __fastcall ToolClockElement_ToolTip(void* element, void*, const char** output)
	{
		*output = static_cast<const char*>(element) + NC2Layout::ClockElementOffset::TimeText;
		return true;
	}
}
