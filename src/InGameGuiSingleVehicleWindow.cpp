#include "XPalette.h"
#include "C2DefaultInteraction.h"
#include "C2EngineDisplayAccess.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StaticVehicleTypeData.h"
#include "C2StringAccess.h"
#include "C2VehicleAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiSingleVehicleWindow.h"
#include "InGameGuiTraderConfigWindow.h"
#include "InGameGuiVehicleInhabitantsElements.h"
#include "InGameGuiWindowState.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include <array>
#include <cstring>
#include <new>

extern void __fastcall Display_DE_MapMoveableToGlobalWorldPixelCoordinates(void* display, void* unusedEdx, std::uint32_t moveableId, std::int32_t* point);

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2GuiToolsBase;
	using NXBasics::SRectangle;

	// Complete Windows data region 004F6500..004F670F; all numbers come from Game.exe.
	struct SSubTabDescriptor
	{
		std::int32_t width, height;
		std::uint32_t title, field0C;
	};

	constexpr SSubTabDescriptor gSubTabs[5] = { { 240, 160, 1, 0 }, { 200, 84, 1, 0 }, { 220, 76, 3, 0 }, { 326, 90, 4, 0 }, { 326, 254, 5, 0 } };
	constexpr std::uint32_t gStockPageButtons[7][2] = { { 74, 30 }, { 73, 31 }, { 72, 32 }, { 75, 33 }, { 76, 34 }, { 71, 35 }, { 74, 36 } };
	constexpr std::uint32_t gStockPages[7][14] = { { }, { 1, 4, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23, 43 }, { 2, 3, 5, 24, 25, 26, 27 }, { 6, 7, 8, 9, 10, 13, 14, 15 }, { 33, 34, 35, 36, 37, 38, 39, 40, 41, 42 }, { 31, 32, 28, 29, 30, 45, 44, 47, 46, 49, 48 }, { 50, 51, 52, 53, 54, 55 } };

	struct SSelectionVehicleWindowMostStockGoodsData
	{
		std::uint32_t count;

		struct SEntry
		{
			std::uint32_t good, amount;
		} entries[68];
	};

	static const char* VehicleString(std::uint32_t id)
	{

		return StringTool_GetGameGuiVehicleSelectedWindowStringPtr(id);
	}

	static const char* HumanString(std::uint32_t id)
	{

		return StringTool_GetGameGuiHumanSelectedWindowStringPtr(id);
	}

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	static CStaticToolTipHandler& Tooltips(void* window)
	{

		return *std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::Tooltips));
	}

	static CDynamicSubButtonsHandler& Buttons(void* window)
	{

		return *std::launder(reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::DynamicButtons));
	}

	static SRectangle Section(void* window, std::uint32_t index)
	{
		SRectangle area(0, 0, 0, 0);
		std::memcpy(&area, static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabRectangles + index * 16, 16);

		return area;
	}

	static void* Vehicle(std::int32_t id)
	{

		return NC2Logic::CVehicleArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CVehicleArray::ElementSize;
	}

	static void* House(std::int32_t id)
	{

		return NC2Logic::CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHouseArray::ElementSize;
	}

	// Original function: FUN_004aa941.
	static void* l_Misc_GetSelectedVehiclePtr()
	{
		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) != 4)
			return nullptr;

		const auto id = Selection_GetSelectedSingleVehicleId(gInputManager);

		if (id == -1)
			return nullptr;

		auto* vehicle = Vehicle(id);

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) ? vehicle : nullptr;
	}

	static void* Merchant(void* vehicle)
	{
		const auto id = NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle);

		if (id == -1)
			return nullptr;

		const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::ElementSize;

		return NC2Logic::Human_IsJobTrader(human) ? NC2Logic::HumanMerchantArray_GetMerchantDataPtr(NC2Logic::gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)) : nullptr;
	}

	// Original function: FUN_004aa96f.
	static void l_Misc_CenterWorldDisplay(void* window)
	{
		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (vehicle == nullptr)
			return;

		auto* preview = Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::PreviewElement);

		if (preview == nullptr)
		{

			return;
		}

		auto* display = Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display);
		std::int32_t point[2];
		Display_DE_MapMoveableToGlobalWorldPixelCoordinates(display, nullptr, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId), point);
		point[1] -= 25;
		NC2E2::DE_SetWantedPositionToPixelPoint(display, point);
	}

	// Original function: FUN_004ab2e3.
	static std::uint32_t l_SubTabs_GetEnabledChecksum(void* window)
	{
		std::uint32_t result = 0;

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				result |= 1u << (index * 2);

				if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index))
					result |= 2u << (index * 2);
			}
		}

		return result;
	}

	// Original function: FUN_004ab314.
	static void l_SubTabs_OverwriteHeight(void* window, std::uint32_t index, std::int32_t height)
	{
		if (!Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index) || Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index))
		{

			return;
		}

		if (height < 16)
			height = 16;

		if (Read<std::int32_t>(window, NC2Layout::SingleVehicleWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::VehicleWindowValue) != height)
		{
			Write(window, NC2Layout::SingleVehicleWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::VehicleWindowValue, height);
			Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::LayoutInvalidFlag, 1);
		}
	}

	// Original function: FUN_004ab4bf.
	int __cdecl lsSortGoodsCompareFunction(const void* first, const void* second)
	{

		return StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(first, NC2Layout::MapCoordinatePairOffset::Coordinates), 1), NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(second, NC2Layout::MapCoordinatePairOffset::Coordinates), 1));
	}

	// Original function: FUN_004ab4eb; stable descending signed amounts, then ascending names.
	static void l_SubTabs_StockVehicleGoods_GetMostStockGoods(void*, SSelectionVehicleWindowMostStockGoodsData* output)
	{
		output->count = 0;
		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (vehicle == nullptr)
		{

			return;
		}

		for (std::uint32_t good = 1; good <= 55; ++good)
		{
			if (NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedGoodTypes[good])
			{
				const auto amount = NC2Logic::Vehicle_Stock_GetAmountOfGoods(vehicle, good);

				if (amount)
					output->entries[output->count++] = { good, amount };
			}
		}

		for (std::uint32_t pass = 1; pass < output->count; ++pass)
		{
			for (std::uint32_t index = 1; index < output->count; ++index)
			{
				auto& a = output->entries[index - 1];
				auto& b = output->entries[index];

				if (static_cast<std::int32_t>(a.amount) < static_cast<std::int32_t>(b.amount) || (a.amount == b.amount && StringToolMisc_StringSortCompare(NC2Logic::StringAccess_GetGoodNamePtr(a.good, 1), NC2Logic::StringAccess_GetGoodNamePtr(b.good, 1)) > 0))
				{
					const auto temporary = a;
					a = b;
					b = temporary;
				}
			}
		}
	}

	// Original function: FUN_004ab34e.
	static void l_SubTabs_StockVehicleGoods_Init(void* window)
	{
		if (!Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabEnabled))
			return;

		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (vehicle == nullptr)
		{

			return;
		}

		for (std::uint32_t page = 0; page < 7; ++page)
		{
			std::uint32_t index = 0;

			for (const auto good : gStockPages[page])
			{
				if (NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedGoodTypes[good])
					Write(window, NC2Layout::SingleVehicleWindowOffset::StockGoods + (page * NC2Layout::VehicleWindowLimits::StockGoodsPerPage + index++) * NC2Layout::RecordSize::VehicleWindowValue, good);
			}
		}

		for (std::uint32_t good = 1; good <= 55; ++good)
		{
			if (NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->allowedGoodTypes[good])
			{
				bool found = false;

				for (std::uint32_t page = 0; page < 7; ++page)
				{
					for (std::uint32_t index = 0; index < 14; ++index)
					{
						if (Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::StockGoods + (page * NC2Layout::VehicleWindowLimits::StockGoodsPerPage + index) * NC2Layout::RecordSize::VehicleWindowValue) == good)
						{
							found = true;
							break;
						}
					}
				}

				if (!found)
					for (std::uint32_t index = 0; index < 14; ++index)
					{
						if (!Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::UncategorizedStockGoods + index * NC2Layout::RecordSize::GoodId))
						{
							Write(window, NC2Layout::SingleVehicleWindowOffset::UncategorizedStockGoods + index * NC2Layout::RecordSize::GoodId, good);
							break;
						}
					}
			}
		}

		SSelectionVehicleWindowMostStockGoodsData most;
		l_SubTabs_StockVehicleGoods_GetMostStockGoods(window, &most);

		if (most.count > 14)
			most.count = 14;

		for (std::uint32_t index = 0; index < 14; ++index)
		{
			Write(window, NC2Layout::SingleVehicleWindowOffset::StockGoods + index * NC2Layout::RecordSize::VehicleWindowValue, index < most.count ? most.entries[index].good : 0u);
		}

		for (std::uint32_t page = 1; page < 7; ++page)
		{
			auto* data = static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::StockGoods + page * 56;
			std::uint32_t count = 0;

			for (std::uint32_t index = 0; index < 14; ++index)
			{
				if (Read<std::uint32_t>(data, index * NC2Layout::RecordSize::GoodId))
					++count;
			}

			if (count > 1)
				NXBasics::Runtime_QuickSort(data, count, 4, &lsSortGoodsCompareFunction);
		}
	}

	// Original function: FUN_004ab5b8.
	static void l_SubTabs_Trader_UpdateButtons(void* window)
	{
		if (Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigButton) == nullptr)
			return;

		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		const auto hide = [&](std::uint32_t offset)
		{
			auto* element = Read<void*>(window, offset);
			desktop->Element_Hide(static_cast<NXBaseGui::CBaseElement*>(element));
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled, 1);
		};
		const auto show = [&](std::uint32_t offset)
		{
			auto* element = Read<void*>(window, offset);
			desktop->Element_UnHide(static_cast<NXBaseGui::CBaseElement*>(element));
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Disabled, 0);
		};
		hide(NC2Layout::SingleVehicleWindowOffset::RouteSelectButtons);
		hide(NC2Layout::SingleVehicleWindowOffset::SecondRouteSelectButton);
		show(NC2Layout::SingleVehicleWindowOffset::RouteAssignButtons);
		show(NC2Layout::SingleVehicleWindowOffset::SecondRouteAssignButton);
		hide(NC2Layout::SingleVehicleWindowOffset::RouteRemoveButtons);
		hide(NC2Layout::SingleVehicleWindowOffset::SecondRouteRemoveButton);
		BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::RouteLabels), HumanString(112));
		BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::SecondRouteLabel), HumanString(113));
		Write<std::uint8_t>(Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigButton), NC2Layout::GuiTextButtonOffset::DisabledFlag, 1);
		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (vehicle == nullptr)
			return;

		auto* merchant = Merchant(vehicle);

		if (merchant == nullptr)
		{

			return;
		}

		std::uint32_t row = 0;

		for (std::uint32_t index = 0; index < 2; ++index)
		{
			const auto* data = NC2Logic::MerchantData_GetHouseDataPtr(merchant, index);

			if (!NC2Logic::MerchantHouseData_IsValid(data))
				continue;

			const auto* house = House(Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId));
			BaseToolTextElement_SetCaptionString(Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::RouteLabels + row * NC2Layout::RecordSize::VehicleRouteControls), NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1));

			for (auto offset : { NC2Layout::SingleVehicleWindowOffset::RouteSelectButtonsUnsigned, NC2Layout::SingleVehicleWindowOffset::RouteRemoveButtonsUnsigned })
			{
				auto* element = Read<void*>(window, offset + row * NC2Layout::RecordSize::VehicleRouteControls);
				Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
				Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				show(offset + row * 16);

				if (offset == NC2Layout::SingleVehicleWindowOffset::RouteSelectButtons)
					hide(NC2Layout::SingleVehicleWindowOffset::RouteAssignButtons + row * NC2Layout::RecordSize::VehicleRouteControls);
			}

			if (++row == 2)
				break;
		}

		Write<std::uint8_t>(Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigButton), NC2Layout::GuiTextButtonOffset::DisabledFlag, row < 2);
	}

	static void* AddGraphic(void* window, const SRectangle& area, std::uint32_t graphic, const char* tooltip, std::uint32_t command)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		auto* element = memory == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(memory, nullptr, &area, graphic, tooltip, command, window);
		NXBaseGui::BaseWindow_Element_Add(window, element);

		return element;
	}

	// Original function: FUN_004aa9b5; native child creation order and bottom-right anchoring.
	static void l_SubTabs_UpdateAll(void* window, bool force)
	{
		const auto checksum = l_SubTabs_GetEnabledChecksum(window);

		if (!force && Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::TabStateChecksum) == checksum && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::LayoutInvalidFlag))
		{

			return;
		}

		Write(window, NC2Layout::SingleVehicleWindowOffset::TabStateChecksum, checksum);
		Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::LayoutInvalidFlag, 0);
		std::int32_t width = 0, height = 0;

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				if (width < gSubTabs[index].width)
					width = gSubTabs[index].width;

				const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SingleVehicleWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::VehicleWindowValue);
				height += Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index) ? 16 : 16 + (overrideHeight ? overrideHeight : gSubTabs[index].height);
			}
		}

		Write(window, NC2Layout::SingleVehicleWindowOffset::ComputedWidth, width);
		Write(window, NC2Layout::SingleVehicleWindowOffset::ComputedHeight, height);
		std::int32_t y = 0;

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			SRectangle area(-1, -1, 0, 0);

			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SingleVehicleWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::VehicleWindowValue);
				area = SRectangle(width - gSubTabs[index].width, y, gSubTabs[index].width, Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index) ? 16 : 16 + (overrideHeight ? overrideHeight : gSubTabs[index].height));
				y += area.height;
			}

			std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabRectangles + index * 16, &area, 16);
		}

		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(GetDisplayWidth() - width, GetDisplayHeight() - height, width, height);
		NC2InGameGuiBase::GetDesktopPtr()->Window_ChangeAreaRectangle(static_cast<NXBaseGui::CBaseWindow*>(window), area);
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::PreviewElement, 0, 40);
		Buttons(window).AllButtonsHaveBeenRemoved();

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				const auto section = Section(window, index);
				const bool collapsed = Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index) != 0;
				auto* button = AddGraphic(window, SRectangle(section.x + section.width - 19, section.y + 2, 16, 12), collapsed ? 19 : 20, VehicleString(collapsed ? 6 : 7), 10000);
				Write(button, NC2Layout::GuiGfxButtonOffset::MessageParameter1, index);
				Write<std::uint32_t>(button, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0);
			}
		}

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags))
		{
			const auto section = Section(window, 0);
			const SRectangle previewArea(section.x + 4, section.y + 20, 120, Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabCollapsed) ? 100 : 152);
			auto* memory = NXBasics::Runtime_AllocateWithNewHandler(580, 1);
			auto* preview = memory == nullptr ? nullptr : NC2InGameGuiWorldElement::WorldDisplayElement_Construct(memory, &previewArea, false, false);
			Write(window, NC2Layout::SingleVehicleWindowOffset::PreviewElement, preview);
			NC2InGameGuiWorldElement::WorldDisplayElement_DrawFrame_SetMode(preview, 2);
			NC2InGameGuiWorldElement::WorldDisplayElement_Misc_LeftMouseClick_HandlerSet(preview, window, 10001);
			NC2InGameGuiWorldElement::WorldDisplayElement_Misc_LeftMouseDoubleClick_HandlerSet(preview, window, 10002);
			NXBaseGui::BaseWindow_Element_Add(window, preview);
			NC2E2::DE_ControllingPlayer_SetId(Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display), NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId());
			NC2E2::DE_Zoom_SetCounter(Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display), -1);
			l_Misc_CenterWorldDisplay(window);
			AddGraphic(window, SRectangle(previewArea.x + 2, previewArea.y + 2, 18, 16), 18, VehicleString(14), 10003);
			AddGraphic(window, SRectangle(previewArea.x + previewArea.width - 20, previewArea.y + 2, 18, 16), 28, VehicleString(15), 10004);
			AddGraphic(window, SRectangle(previewArea.x + previewArea.width - 20, previewArea.y + previewArea.height - 20, 18, 18), 142, VehicleString(16), 10005);
		}

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabCollapsed))
		{
			auto* vehicle = l_Misc_GetSelectedVehiclePtr();

			if (vehicle != nullptr && Merchant(vehicle) != nullptr)
			{
				const auto section = Section(window, 1);

				for (std::uint32_t row = 0; row < 2; ++row)
				{
					const auto rowY = section.y + 24 + row * 24;
					Write(window, NC2Layout::SingleVehicleWindowOffset::RouteSelectButtons + row * NC2Layout::RecordSize::VehicleRouteControls, AddGraphic(window, SRectangle(section.x, rowY, 20, 18), 18, VehicleString(26), 10019));
					Write(window, NC2Layout::SingleVehicleWindowOffset::RouteAssignButtons + row * NC2Layout::RecordSize::VehicleRouteControls, AddGraphic(window, SRectangle(section.x, rowY, 20, 30), 30, VehicleString(27), 10020));
					Write(window, NC2Layout::SingleVehicleWindowOffset::RouteRemoveButtons + row * NC2Layout::RecordSize::VehicleRouteControls, AddGraphic(window, SRectangle(section.x + 20, rowY, 20, 30), 29, VehicleString(28), 10021));
					auto* memory = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
					const SRectangle labelArea(section.x + 38, rowY, 154, 20);
					auto* label = memory == nullptr ? nullptr : BaseToolTextElement_Construct(memory, nullptr, &labelArea, "", false, false);
					Write(window, NC2Layout::SingleVehicleWindowOffset::RouteLabels + row * NC2Layout::RecordSize::VehicleRouteControls, label);
					Write<std::uint8_t>(label, NC2Layout::GuiTextElementOffset::SmallFontFlag, 1);
					NXBaseGui::BaseWindow_Element_Add(window, label);
				}

				const SRectangle config(section.x + 8, section.y + 76, 184, 20);
				auto* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
				auto* button = memory == nullptr ? nullptr : BaseToolTextButton_Construct(memory, nullptr, &config, HumanString(110), 10022, window);
				Write(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigButton, button);
				NXBaseGui::BaseWindow_Element_Add(window, button);
				l_SubTabs_Trader_UpdateButtons(window);
			}
		}

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::HumanPassengersTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::HumanPassengersTabCollapsed))
		{
			const auto section = Section(window, 2);
			const SRectangle child(section.x + 4, section.y + 20, section.width - 8, section.height - 24);
			auto* memory = NXBasics::Runtime_AllocateWithNewHandler(756, 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : SelectionVehicleAttachedHumansElement_Construct(memory, &child, window, 10012, 10013, Selection_GetSelectedSingleVehicleId(gInputManager)));
		}

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::HumanAndVehiclePassengersTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::HumanAndVehiclePassengersTabCollapsed))
		{
			const auto section = Section(window, 3);
			SRectangle child(section.x + 4, section.y + 20, section.width - 112, section.height - 24);
			auto* memory = NXBasics::Runtime_AllocateWithNewHandler(756, 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : SelectionVehicleAttachedHumansElement_Construct(memory, &child, window, 10012, 10013, Selection_GetSelectedSingleVehicleId(gInputManager)));
			child.x += child.width + 4;
			child.width = 100;
			memory = NXBasics::Runtime_AllocateWithNewHandler(756, 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : SelectionVehicleAttachedVehiclesElement_Construct(memory, &child, window, 10014, 10015, Selection_GetSelectedSingleVehicleId(gInputManager)));
		}

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabCollapsed))
		{
			const auto section = Section(window, 4);

			for (std::uint32_t page = 0; page < 7; ++page)
			{
				auto* button = AddGraphic(window, SRectangle(section.x + 8 + page * 46, section.y + 24, 44, 18), gStockPageButtons[page][0], CGuiBaseStringsManager::GetStringPtr(8, gStockPageButtons[page][1]), 10016);
				Write(button, NC2Layout::GuiGfxButtonOffset::MessageParameter1, page);
				Write<std::uint32_t>(button, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0);
				Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::Selected, page == Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::StockPage));
				Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
				Write<std::uint8_t>(button, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
			}
		}
	}

	static void __fastcall SelectionSingleVehicleWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);

	// Original function: FUN_004a9958.
	static bool __fastcall SelectionSingleVehicleWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t first, std::uint32_t second, std::uint32_t third)
	{
		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		switch (command)
		{
			case SwitchIds::VehicleWindowCommand::ToggleSubTab:
				if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + second))
				{
					Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + second, !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + second));
					l_SubTabs_UpdateAll(window, false);
				}

				break;
			case SwitchIds::VehicleWindowCommand::CenterVehicle:
			case SwitchIds::VehicleWindowCommand::CenterVehicleAndStopFollowing:
				if (vehicle != nullptr)
				{
					NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(manager->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), static_cast<const std::uint32_t*>(NC2Logic::Vehicle_GetMapPositionRef(vehicle)));
					NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
				}

				break;
			case SwitchIds::VehicleWindowCommand::ShowActionButtons:
				manager->Selection_ActionButtons_BringUp();
				break;
			case SwitchIds::VehicleWindowCommand::OpenObservation:
				manager->ObservationWindow_CreateNewOne();
				break;
			case SwitchIds::VehicleWindowCommand::Update:
			{
				if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabCollapsed))
				{
					SSelectionVehicleWindowMostStockGoodsData most;
					l_SubTabs_StockVehicleGoods_GetMostStockGoods(window, &most);

					if (most.count > 14)
						most.count = 14;

					for (std::uint32_t index = 0; index < most.count; ++index)
					{
						if (Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::StockGoods + index * NC2Layout::RecordSize::VehicleWindowValue) != most.entries[index].good)
						{
							l_SubTabs_StockVehicleGoods_Init(window);
							l_SubTabs_UpdateAll(window, true);
							break;
						}
					}
				}

				Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabEnabled, vehicle != nullptr && NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle) != -1);

				if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabCollapsed))
				{
					l_SubTabs_Trader_UpdateButtons(window);
				}
				else if (auto* popup = Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigWindow))
				{
					NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(static_cast<NXBaseGui::CBaseWindow*>(popup));
				}

				l_Misc_CenterWorldDisplay(window);
				l_SubTabs_UpdateAll(window, false);
				break;
			}
			case SwitchIds::VehicleWindowCommand::ToggleFollowVehicle:
				if (vehicle != nullptr)
				{
					const auto id = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId);

					if (NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_GetVehicleId(manager->GetDesktop()) == id)
					{
						NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
					}
					else
					{
						NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_FollowVehicle(manager->GetDesktop(), id);
					}
				}

				break;
			case SwitchIds::VehicleWindowCommand::StopVehicle:
			case SwitchIds::VehicleWindowCommand::ResetWantedAmounts:
			case SwitchIds::VehicleWindowCommand::DetachAttachedVehicle:
				if (vehicle != nullptr && (command != 10011 || NC2Logic::Vehicle_Attachment_IsValid(vehicle)))
				{
					std::byte packet[20]{ };
					Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, command == 10006 ? 0x66 : command == 10007 ? 0x6E : 0x72);
					Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, packet);
				}

				break;
			case SwitchIds::VehicleWindowCommand::AssignWorkCenter:
			case SwitchIds::VehicleWindowCommand::AttachVehicle:
				if (vehicle != nullptr)
					InputMode_Set(gInputManager, command == 10008 ? 19 : 27);

				break;
			case SwitchIds::VehicleWindowCommand::SelectAttachedVehicle:
				if (vehicle != nullptr && NC2Logic::Vehicle_Attachment_IsValid(vehicle))
					Selection_SelectSingleVehicle(gInputManager, Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId));

				break;
			case SwitchIds::VehicleWindowCommand::SelectHuman:
			case SwitchIds::VehicleWindowCommand::SelectHumanAndShowActions:
				Selection_SelectSingleHuman(gInputManager, first);

				if (command == 10013)
					manager->Selection_ActionButtons_BringUp();

				break;
			case SwitchIds::VehicleWindowCommand::SelectVehicle:
			case SwitchIds::VehicleWindowCommand::SelectVehicleAndShowActions:
				Selection_SelectSingleVehicle(gInputManager, first);

				if (command == 10015)
					manager->Selection_ActionButtons_BringUp();

				break;
			case SwitchIds::VehicleWindowCommand::SelectStockView:
				Write(window, NC2Layout::SingleVehicleWindowOffset::StockPage, second);
				l_SubTabs_UpdateAll(window, true);
				break;
			case SwitchIds::VehicleWindowCommand::DecreaseWantedAmount:
			case SwitchIds::VehicleWindowCommand::IncreaseWantedAmount:
				if (vehicle != nullptr)
				{
					auto amount = static_cast<std::int32_t>(NC2Logic::Vehicle_Stock_GetAmountOfWantedGoods(vehicle, second));
					const auto step = (Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) ? 10 : 1;

					if (command == 10017)
					{
						amount -= step;

						if (amount < 0)
							amount = 0;
					}
					else
					{
						amount += step;

						if (amount > NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots)
							amount = NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots;
					}

					std::byte packet[20]{ };
					Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x6D);
					Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter1, second);
					Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter2, amount);
					NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, packet);
				}

				break;
			case SwitchIds::VehicleWindowCommand::SelectTradeHouse:
				if (Read<std::uint8_t>(House(second), NC2Layout::HouseOffset::AllocatedFlag) && Read<std::uint32_t>(House(second), NC2Layout::HouseOffset::UniqueId) == third)
					Selection_SelectHouse(gInputManager, second);

				break;
			case SwitchIds::VehicleWindowCommand::AddTradeHouse:
				if (vehicle != nullptr && NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle) != -1)
				{
					Selection_SelectSingleHuman(gInputManager, NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle));
					InputMode_Set(gInputManager, 8);
				}

				break;
			case SwitchIds::VehicleWindowCommand::RemoveTradeHouse:
				if (Read<std::uint8_t>(House(second), NC2Layout::HouseOffset::AllocatedFlag) && Read<std::uint32_t>(House(second), NC2Layout::HouseOffset::UniqueId) == third && vehicle != nullptr && NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle) != -1)
				{
					const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle)) * NC2Logic::CHumanArray::ElementSize;
					std::byte packet[20]{ };
					Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x25);
					Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
					Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter1, second);
					Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter2, third);
					NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, packet);
				}

				break;
			case SwitchIds::VehicleWindowCommand::OpenTraderConfiguration:
				if (Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigWindow) == nullptr)
				{
					auto* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::TraderConfigWindow, 1);
					auto* popup = memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedTraderConfigWindow_Construct(memory, window, 10023);
					Write(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigWindow, popup);
					BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(popup), false);
				}

				break;
			case SwitchIds::VehicleWindowCommand::TraderConfigurationClosed:
				Write<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigWindow, nullptr);
				break;
		}

		return true;
	}

	// Original function: FUN_004aa8f5; the base foods remain counted while their substitutes are excluded.
	static std::uint32_t l_TransportGoods_GetCurrentTotalAmount(void*, const void* vehicle)
	{
		std::uint32_t total = 0;

		for (std::uint32_t good = 0; good < 66; ++good)
		{
			if (good == 18 || good == 19 || good == 21 || good == 56 || good == 22 || good == 23 || good == 20)
				continue;

			total += NC2Logic::Vehicle_Stock_GetAmountOfGoods(vehicle, good);
		}

		return total;
	}

	// Original function: FUN_004a9ecf; complete native section drawing and dynamic command controls.
	static void __fastcall SelectionSingleVehicleWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		Tooltips(window).ClearAll();

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				auto header = Section(window, index);
				header.x += rectangle->x;
				header.y += rectangle->y;
				header.height = 16;

				if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index))
				{
					header.x += header.width - 200;
					header.width = 200;
				}

				SelectionTool_FillRectangle(bitmap, header, 5);
				SelectionTool_PrintCenteredString(bitmap, header, true, VehicleString(gSubTabs[index].title));

				if (!Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags + index))
				{
					auto body = Section(window, index);
					body.x += rectangle->x;
					body.y += rectangle->y + 16;
					body.height -= 16;
					SelectionTool_FillRectangle(bitmap, body, 1);
				}
			}
		}

		auto* vehicle = l_Misc_GetSelectedVehiclePtr();

		if (vehicle == nullptr)
			return;

		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		const auto* color = White();

		if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags) && !Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags))
		{
			const auto section = Section(window, 0);
			auto x = section.x + 128, y = section.y + 20;
			const auto width = section.width - 132;
			const auto centered = [&](const char* text)
			{
				bitmap->Text_Print(font, color, x + (width - NXBasics::Font_GetPixelWidth(font, text)) / 2, y, text);
			};
			centered(NC2Logic::StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType)));
			y += 16;
			const auto type = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);
			const std::uint32_t maximum = type == 3 || type == 4 ? 5000u : type == 5 ? 3000u
			                                                                         : 1000u;
			const auto health = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::Hitpoints);
			DrawTool_DrawIcon(34, bitmap, x + 6, y + 6);
			SRectangle bar(x + 16, y + 2, width - 16, 8);
			DrawTool_DrawFrame(bitmap, bar, 2);
			bar.Inflate(-2, -2);
			bar.width = static_cast<std::int32_t>(static_cast<std::uint32_t>(bar.width) * health / maximum);
			auto shade = health * 255u / maximum;

			if (shade > 255)
				shade = 255;

			bitmap->Draw_Box(bar, *reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.hitpointsBarPalette) + NXBasics::PaletteOffset::Colors + shade * 4));
			char text[300];
			NXBasics::Runtime_PrintStringN(text, 100, "%s: %d/%d", VehicleString(9), health, maximum);
			text[99] = 0;
			Tooltips(window).Add(SRectangle(x, y, width, 12), text);
			y += 12;

			if (NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots)
			{
				NXBasics::Runtime_PrintString(text, "%s: %d", VehicleString(8), NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots);
				centered(text);
				y += 12;
			}

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 54)
			{
				centered(NC2Logic::StringAccess_GetMilitaryModeNamePtr(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MilitaryMode)));
				y += 12;
			}

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CurrentTask))
			{
				centered(NC2Logic::StringAccess_GetVehicleTaskNamePtr(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::CurrentTask)));
				y += 12;
			}

			y += 10;
			NC2Logic::SUserCommandsForVehicleHandler commands;

			if (NC2Logic::Logic_UserControl_GetVehicleCommands(Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId), &commands, false))
			{
				constexpr std::uint32_t actions[3]{ 3, 14, 4 };
				// Windows 004AA2A3..004AA354 uses string IDs 0x11..0x16 for disembark, unload and dock.
				constexpr std::uint32_t captions[3]{ 17, 19, 21 };
				constexpr std::uint32_t tips[3]{ 18, 20, 22 };

				for (std::uint32_t index = 0; index < 3; ++index)
				{
					const auto command = 10006 + index;

					if (commands.IsCommandAllowed(actions[index]))
					{
						Buttons(window).UpdateTextButton(command, 0, x, y, width, 18, VehicleString(captions[index]), true, VehicleString(tips[index]));
						y += 22;
					}
					else
					{
						Buttons(window).RemoveButton(command, 0);
					}
				}

				const bool attached = NC2Logic::Vehicle_Attachment_IsValid(vehicle);

				if (!attached && !commands.IsCommandAllowed(12) && !commands.IsCommandAllowed(13))
				{
					Buttons(window).RemoveButton(10009, 0);
					Buttons(window).RemoveButton(10010, 0);
					Buttons(window).RemoveButton(10011, 0);
				}
				else
				{
					y += 6;
					DrawTool_DrawIcon(144, bitmap, x + 6, y + 6);
					x += 20;

					if (NC2Logic::Vehicle_Attachment_IsValid(vehicle))
					{
						Buttons(window).UpdateGfxButton(10009, 0, x, y, 18, VehicleString(23));
						x += 20;
					}
					else
					{
						Buttons(window).RemoveButton(10009, 0);
					}

					if (commands.IsCommandAllowed(12))
					{
						Buttons(window).UpdateGfxButton(10010, 0, x, y, 30, VehicleString(24));
						x += 20;
					}
					else
					{
						Buttons(window).RemoveButton(10010, 0);
					}

					if (commands.IsCommandAllowed(13))
					{
						Buttons(window).UpdateGfxButton(10011, 0, x, y, 29, VehicleString(25));
						x += 20;
					}
					else
					{
						Buttons(window).RemoveButton(10011, 0);
					}

					if (NC2Logic::Vehicle_Attachment_IsValid(vehicle))
						bitmap->Text_Print(font, color, x, y, NC2Logic::StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(Vehicle(Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::CarrierVehicleId)), NC2Layout::VehicleOffset::LogicType)));

					y += 12;
				}
			}

			auto height = y - section.y - 10;
			const auto* preview = Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::PreviewElement);
			const auto minimum = preview == nullptr ? 8 : Read<std::int32_t>(preview, NC2Layout::GuiBaseElementOffset::Height) + 8;

			if (height < minimum)
				height = minimum;

			l_SubTabs_OverwriteHeight(window, 0, height);
		}
		else
		{
			Buttons(window).RemoveButton(10009, 0);
			Buttons(window).RemoveButton(10010, 0);
			Buttons(window).RemoveButton(10011, 0);
		}

		if (!Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabEnabled) || Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabCollapsed))
		{

			return;
		}

		const auto stock = Section(window, 4);
		const auto x = stock.x + 4;
		auto y = stock.y + 46;
		auto buttonY = y + 1;
		const auto totalWanted = NC2Logic::Vehicle_Stock_GetTotalWantedAmount(vehicle);
		const auto slots = NC2Logic::Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots;

		for (std::uint32_t index = 0; index < 14; ++index)
		{
			const auto good = Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::StockGoods + (Read<std::uint32_t>(window, NC2Layout::SingleVehicleWindowOffset::StockPage) * NC2Layout::VehicleWindowLimits::StockGoodsPerPage + index) * NC2Layout::RecordSize::VehicleWindowValue);

			if (!good)
				break;

			if (!NC2Logic::Vehicle_Stock_CanStoreGoodType(vehicle, good))
				continue;

			const auto amount = NC2Logic::Vehicle_Stock_GetAmountOfGoods(vehicle, good);
			const auto wanted = NC2Logic::Vehicle_Stock_GetAmountOfWantedGoods(vehicle, good);
			const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, amount);
			DrawTool_PrintStringXV(bitmap, x, y, font, color, "{gfxgood:%d,%d} %s {setx:180}%d", good, amount ? amount : 1, name, amount);
			char tooltip[100];
			NXBasics::Runtime_PrintStringN(tooltip, 100, "%d %s", amount, name);
			tooltip[99] = 0;
			Tooltips(window).Add(SRectangle(x, y, 34, 14), tooltip);

			if (NC2Logic::Vehicle_Passengers_IsCarrierAttached(vehicle))
			{
				DrawTool_PrintStringXV(bitmap, x + 224, y, font, color, "{gfxgood:%d,%d}", good, wanted ? wanted : 1);
				NXBasics::Runtime_PrintStringN(tooltip, 100, "%d %s", wanted, name);
				tooltip[99] = 0;
				Tooltips(window).Add(SRectangle(x + 224, y, 34, 14), tooltip);
				char number[300];
				NXBasics::Runtime_PrintString(number, "%d", wanted);
				bitmap->Text_Print(font, color, x + 290 - NXBasics::Font_GetPixelWidth(font, number), y, number);
				const auto* singular = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
				NXBasics::Runtime_PrintString(number, VehicleString(10), singular);
				Buttons(window).UpdateGfxButton(10017, good, x + 255, buttonY, 27, number);
				Buttons(window).SetButtonMiscCommandData(10017, good, good, 0);
				NXBasics::Runtime_PrintString(number, VehicleString(11), singular);
				Buttons(window).UpdateGfxButton(10018, good, x + 295, buttonY, 26, number);
				Buttons(window).SetButtonMiscCommandData(10018, good, good, 0);
			}
			else
			{
				Buttons(window).RemoveButton(10017, good);
				Buttons(window).RemoveButton(10018, good);
			}

			y += 16;
			buttonY += 16;
		}

		const auto footerY = stock.y + stock.height - 24;
		bitmap->Text_PrintV(font, color, x + 2, footerY, VehicleString(12), l_TransportGoods_GetCurrentTotalAmount(window, vehicle));

		if (NC2Logic::Vehicle_Passengers_IsCarrierAttached(vehicle))
			bitmap->Text_PrintV(font, color, x + 144, footerY, VehicleString(13), totalWanted, slots);
	}

	// Original function: FUN_004aa87a.
	static bool __fastcall SelectionSingleVehicleWindow_IsHit(void* window, void*, const std::int32_t* point)
	{
		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags + index))
			{
				const auto area = Section(window, index);
				const auto right = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) + static_cast<std::uint32_t>(area.width) - 1u);
				const auto bottom = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) + static_cast<std::uint32_t>(area.height) - 1u);

				if (point[0] >= area.x && point[0] <= right && point[1] >= area.y && point[1] <= bottom)
					return true;
			}
		}

		return false;
	}

	// Original function: FUN_004aa8b1.
	static bool __fastcall SelectionSingleVehicleWindow_ToolTip_Available(void* window, void*, const char** output)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, 8);
		BaseElement_GlobalToLocalCoordinates(window, point);
		const auto* text = Tooltips(window).Get(point);

		if (text == nullptr)
			return false;

		*output = text;

		return true;
	}

	// Original function: FUN_004a98ad, deleting wrapper for 004A98DB.
	static void* __fastcall SelectionSingleVehicleWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		if (auto* popup = Read<void*>(window, NC2Layout::SingleVehicleWindowOffset::TraderConfigWindow))
			NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(static_cast<NXBaseGui::CBaseWindow*>(popup));

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
		manager->FrameCall_UnRegister(window);
		gSelectionSingleVehicleWindowStatics[0] = std::byte{ 1 };
		std::memcpy(gSelectionSingleVehicleWindowStatics + 1, static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags, 5);
		Tooltips(window).~CStaticToolTipHandler();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable);
		NXBaseGui::BaseWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Complete native window table 004F6710..004F675B.
	static const std::array<std::uintptr_t, 19> gSingleVehicleTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&SelectionSingleVehicleWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&SelectionSingleVehicleWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&SelectionSingleVehicleWindow_Draw);
		t[8] = reinterpret_cast<std::uintptr_t>(&SelectionSingleVehicleWindow_IsHit);
		t[13] = reinterpret_cast<std::uintptr_t>(&SelectionSingleVehicleWindow_ToolTip_Available);
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);

		return t;
	}();

	// Original function: FUN_004a98c9; the receiver is the embedded SVars at window +0x70.
	static void* SelectionSingleVehicleWindow_Vars_Construct(void* vars)
	{
		::new (static_cast<std::byte*>(vars) + NC2Layout::SingleVehicleWindowVarsOffset::Tooltips) NC2InGameGuiBase::CStaticToolTipHandler;

		return vars;
	}

	// Original function: FUN_004a9786; native allocation 0x3C80.
	void* SelectionSingleVehicleWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(GetDisplayWidth() - 140, GetDisplayHeight() - 600, 140, 600);
		BaseToolWindow_Construct(window, &area);
		SelectionSingleVehicleWindow_Vars_Construct(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags);
		new (static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::DynamicButtons) CDynamicSubButtonsHandler;
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags, 0, NC2Layout::RecordSize::SingleVehicleWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSingleVehicleTable.data());
		Buttons(window).SetWindow(window);

		if (auto* vehicle = l_Misc_GetSelectedVehiclePtr())
		{
			Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TabEnabledFlags, 1);

			if (NC2Logic::Vehicle_Passengers_GetWorkTraderHumanId(vehicle) != -1)
				Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::TraderTabEnabled, 1);

			const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);
			Write<std::uint8_t>(window, type == 3 || type == 4 ? NC2Layout::SingleVehicleWindowOffset::HumanAndVehiclePassengersTabEnabled : NC2Layout::SingleVehicleWindowOffset::HumanPassengersTabEnabled, 1);

			if (type != 5)
				Write<std::uint8_t>(window, NC2Layout::SingleVehicleWindowOffset::StockTabEnabled, 1);
		}

		if (gSelectionSingleVehicleWindowStatics[0] != std::byte{ 0 })
			std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SingleVehicleWindowOffset::TabCollapsedFlags, gSelectionSingleVehicleWindowStatics + 1, 5);

		l_SubTabs_StockVehicleGoods_Init(window);
		{
			NXBasics::CBitmap scratch(10, 10, 1);
			const SRectangle bounds(0, 0, 10, 10);
			SelectionSingleVehicleWindow_Draw(window, nullptr, &scratch, &bounds);
		}
		l_SubTabs_UpdateAll(window, true);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);

		return window;
	}
}