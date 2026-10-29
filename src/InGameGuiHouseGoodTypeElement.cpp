#include "XPalette.h"
#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiHouseGoodTypeElement.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	static void* __fastcall SelectionHouseGoodTypeElement_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall SelectionHouseGoodTypeElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	static void __fastcall SelectionHouseGoodTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*);
	static void __fastcall SelectionHouseGoodTypeElement_DoPressedAction(void* element);
	static bool __fastcall SelectionHouseGoodTypeElement_ToolTip_Available(void* element, void*, const char** output);
	// Complete native table 004F64B4..004F64FB.
	static const std::array<std::uintptr_t, 18> gHouseGoodTypeTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), NXBaseGui::gBaseElementVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&SelectionHouseGoodTypeElement_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionHouseGoodTypeElement_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionHouseGoodTypeElement_Draw);
		t[13] = reinterpret_cast<std::uintptr_t>(&SelectionHouseGoodTypeElement_ToolTip_Available);
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);
		t[17] = reinterpret_cast<std::uintptr_t>(&SelectionHouseGoodTypeElement_DoPressedAction);

		return t;
	}();

	// Original function: FUN_004a950c; native allocation 0x10C.
	void* SelectionHouseGoodTypeElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t houseId, std::uint32_t good)
	{
		NXBaseGui::BaseElement_Construct(element, rectangle, 3, 0x20);
		std::memset(static_cast<std::byte*>(element) + 0x2C, 0, 224);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gHouseGoodTypeTable.data());
		Write(element, NC2Layout::HouseGoodElementOffset::HouseId, houseId);

		if (houseId != -1)
		{
			const auto* house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::intptr_t>(houseId) * NC2Logic::CHouseArray::ElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag))
			{
				Write(element, NC2Layout::HouseGoodElementOffset::HouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			}
			else
			{
				Write<std::int32_t>(element, NC2Layout::HouseGoodElementOffset::HouseId, -1);
			}
		}

		Write(element, NC2Layout::HouseGoodElementOffset::GoodType, good);
		Write(element, NC2Layout::HouseGoodElementOffset::PressedCommand, selectCommand);
		Write(element, NC2Layout::HouseGoodElementOffset::HoverCommand, actionCommand);
		Write(element, NC2Layout::HouseGoodElementOffset::Receiver, receiver);

		return element;
	}

	// Original function: FUN_004a9587.
	static void* __fastcall SelectionHouseGoodTypeElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gHouseGoodTypeTable.data());
		NXBaseGui::BaseElement_Destruct(element);

		if (flags & 1u)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004a95a9.
	static bool __fastcall SelectionHouseGoodTypeElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		if (message != 0x20)
			return false;

		auto* receiver = Read<void*>(element, NC2Layout::HouseGoodElementOffset::Receiver);
		Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::HoverCommand), Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::GoodType), 0, 0);

		return true;
	}

	// Original function: FUN_004a975e.
	static const void* l_ValidateHouse(void* element)
	{
		const auto id = Read<std::int32_t>(element, NC2Layout::HouseGoodElementOffset::HouseId);

		if (id == -1)
			return nullptr;

		const auto* house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHouseArray::ElementSize;

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::HouseRevision))
			return house;

		Write<std::int32_t>(element, NC2Layout::HouseGoodElementOffset::HouseId, -1);

		return nullptr;
	}

	// Original function: FUN_004a95d8; request/minimum suffixes are kept in native order.
	static void __fastcall SelectionHouseGoodTypeElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
	{
		const auto* house = l_ValidateHouse(element);

		if (house == nullptr)
		{

			return;
		}

		const auto good = Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::GoodType);
		NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
		const auto amount = NC2Logic::House_Stock_GetAmountOfGoodX(house, good, type == 7 || type == 8 || type == 9 || type == 1);
		char text[200], suffix[200];
		NXBasics::Runtime_PrintString(text, "{gfxgood:%d,1} %.1f", good, static_cast<double>(amount));

		if (NC2Logic::House_Stock_GetMinimumValue(house, good))
		{
			NXBasics::Runtime_PrintString(suffix, " - %s %d", NC2GuiToolsBase::CGuiBaseStringsManager::GetStringPtr(8, 131), NC2Logic::House_Stock_GetMinimumValue(house, good));
			std::strcat(text, suffix);
		}

		if (NC2Logic::House_Stock_GetGoodRequestCounter(house, good))
		{
			NXBasics::Runtime_PrintString(suffix, " - %s %d", NC2GuiToolsBase::CGuiBaseStringsManager::GetStringPtr(8, 130), NC2Logic::House_Stock_GetGoodRequestCounter(house, good));
			std::strcat(text, suffix);
		}

		SelectionTool_FillRectangle(bitmap, bitmap->GetRectangle(), 2);
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		NC2GuiToolsBase::DrawTool_PrintStringX(bitmap, 2, 1, resources.font08, color, text);
	}

	// Original function: FUN_004a9714.
	static void __fastcall SelectionHouseGoodTypeElement_DoPressedAction(void* element)
	{
		auto* receiver = Read<void*>(element, NC2Layout::HouseGoodElementOffset::Receiver);
		Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::PressedCommand), Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::GoodType), 0, 0);
		NC2GuiToolsBase::FXTool_ClickConfirm();
	}

	// Original function: FUN_004a9738; tooltip remains available independently of revision validation.
	static bool __fastcall SelectionHouseGoodTypeElement_ToolTip_Available(void* element, void*, const char** output)
	{
		auto* text = reinterpret_cast<char*>(element) + 0x38;
		std::strcpy(text, NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(element, NC2Layout::HouseGoodElementOffset::GoodType), 1));
		*output = text;

		return true;
	}
}