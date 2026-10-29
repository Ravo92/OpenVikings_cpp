#include "InGameGuiStorage.h"
#include "XPalette.h"
#include "C2E2HumanSoundAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameControlNetwork.h"
#include "C2HouseAccess.h"
#include "C2HouseUserControl.h"
#include "C2HumanMerchantAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHouseGoodTypeElement.h"
#include "InGameGuiHouseInhabitantsElement.h"
#include "InGameGuiHouseWindow.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
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
#include "XNumericConstants.h"
#include <array>
#include <cmath>
#include <cstring>
#include <new>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2GuiToolsBase;
	using NXBasics::SRectangle;

	// Complete Windows descriptor/page region 004F61F8..004F6467.
	struct SSubTabDescriptor
	{
		std::int32_t width, height;
		std::uint32_t title, field0C;
	};

	static constexpr SSubTabDescriptor gSubTabs[11] = { { 320, 200, 1, 0 }, { 320, 36, 2, 0 }, { 300, 80, 3, 0 }, { 300, 70, 4, 0 }, { 340, 190, 5, 0 }, { 300, 100, 6, 0 }, { 340, 270, 5, 0 }, { 300, 140, 7, 0 }, { 220, 76, 8, 0 }, { 310, 180, 10, 0 }, { 310, 120, 11, 0 } };
	static constexpr std::uint32_t gStockPageButtons[7][2] = { { 74, 30 }, { 73, 31 }, { 72, 32 }, { 75, 33 }, { 76, 34 }, { 71, 35 }, { 157, 36 } };
	static constexpr std::uint32_t gStockPages[7][14] = { { }, { 1, 4, 11, 12, 16, 17, 18, 19, 20, 21, 22, 23, 43 }, { 2, 3, 5, 24, 25, 26, 27 }, { 6, 7, 8, 9, 10, 13, 14, 15 }, { 33, 34, 35, 36, 37, 38, 39, 40, 41, 42 }, { 31, 32, 28, 29, 30, 45, 44, 47, 46, 49, 48 }, { 50, 51, 52, 53, 54, 55 } };

	struct SSelectionHouseWindowMostStockGoodsData
	{
		std::uint32_t count;

		struct SEntry
		{
			std::uint32_t good;
			float amount;
		} entries[68];
	};

	static_assert(sizeof(SSelectionHouseWindowMostStockGoodsData) == 548);

	static const char* HouseString(std::uint32_t id)
	{

		return StringTool_GetGameGuiHouseSelectedWindowStringPtr(id);
	}

	static CStaticToolTipHandler& Tooltips(void* window)
	{

		return *std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::Tooltips));
	}

	static CDynamicSubButtonsHandler& Buttons(void* window)
	{

		return *std::launder(reinterpret_cast<CDynamicSubButtonsHandler*>(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::DynamicButtons));
	}

	static SRectangle Section(void* window, std::uint32_t index)
	{
		SRectangle area(0, 0, 0, 0);
		std::memcpy(&area, static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabRectangles + index * 16, 16);

		return area;
	}

	// Original function: FUN_004a85a6.
	static void* l_Misc_GetSelectedHousePtr()
	{
		const auto id = Selection_GetSelectedHouseId(gInputManager);

		if (id == -1)
			return nullptr;

		auto* house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHouseArray::ElementSize;

		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) ? house : nullptr;
	}

	// Original function: FUN_004a85ca.
	static bool l_Misc_IsOpponentTradeHouseSelected()
	{
		auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
			return false;

		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();

		if (Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId) == player)
			return false;

		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

		if (type != 7 && type != 8 && type != 9 && type != 1)
			return false;

		if (NC2Logic::GameControl_Diplomacy_GetState(NC2Logic::gGameControl, Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId), player) != 1)
			return false;

		return NC2Logic::HumanMerchantArray_GetNextTradeAgreementId(NC2Logic::gHumanMerchantArray, house, NXBasics::IntegerRange::UInt32Maximum) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_004a8627.
	static std::uint32_t l_Misc_GetMainWorkerJobType()
	{
		const auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
			return 0;

		const auto* type = NC2Logic::House_GetHouseTypeDataPtr(house);

		return type->workerCount ? type->workers[0].jobType : 0;
	}

	// Original function: FUN_004a91f8.
	static std::uint32_t l_SubTabs_GetEnabledChecksum(void* window)
	{
		std::uint32_t result = 0;

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
			{
				result |= 1u << (index * 2);

				if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
					result |= 2u << (index * 2);
			}
		}

		return result;
	}

	// Original function: FUN_004a94d2.
	static void l_SubTabs_OverwriteHeight(void* window, std::uint32_t index, std::int32_t height)
	{
		if (!Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index) || Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
		{

			return;
		}

		if (height < 16)
			height = 16;

		if (Read<std::int32_t>(window, NC2Layout::SelectionHouseWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::HouseWindowTabHeight) != height)
		{
			Write(window, NC2Layout::SelectionHouseWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::HouseWindowTabHeight, height);
			Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::LayoutInvalidFlag, 1);
		}
	}

	static bool IsGoodAllowed(const void* house, std::uint32_t good)
	{
		const auto* type = NC2Logic::House_GetHouseTypeDataPtr(house);

		for (std::uint32_t index = 0; index < type->stockCount; ++index)
		{
			if (static_cast<std::uint32_t>(type->stock[index].goodType) == good)
				return true;
		}

		return false;
	}

	static bool IncludesFutureStock(const void* house)
	{
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

		return type == 7 || type == 8 || type == 9 || type == 1;
	}

	// Original function: FUN_004a93c0; unordered or zero amounts are excluded by the native x87 flags.
	static void l_SubTabs_StockHouseGoods_GetMostStockGoods(void*, SSelectionHouseWindowMostStockGoodsData* output)
	{
		output->count = 0;
		const auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
		{

			return;
		}

		for (std::uint32_t good = 1; good <= 55; ++good)
		{
			if (IsGoodAllowed(house, good))
			{
				const auto amount = NC2Logic::House_Stock_GetAmountOfGoodX(house, good, IncludesFutureStock(house));

				if (amount != 0.0f && !std::isnan(amount))
					output->entries[output->count++] = { good, amount };
			}
		}

		for (std::uint32_t pass = 1; pass < output->count; ++pass)
		{
			for (std::uint32_t index = 1; index < output->count; ++index)
			{
				auto& a = output->entries[index - 1];
				auto& b = output->entries[index];

				if (a.amount < b.amount || (a.amount == b.amount && lsSortGoodsCompareFunction(&a.good, &b.good) > 0))
				{
					const auto temporary = a;
					a = b;
					b = temporary;
				}
			}
		}
	}

	// Original function: FUN_004a9229.
	static void l_SubTabs_StockHouseGoods_Init(void* window)
	{
		if (!Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::StockHouseTabEnabled))
			return;

		const auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
		{

			return;
		}

		for (std::uint32_t page = 0; page < 7; ++page)
		{
			std::uint32_t index = 0;

			for (const auto good : gStockPages[page])
			{
				if (IsGoodAllowed(house, good))
					Write(window, NC2Layout::SelectionHouseWindowOffset::StockGoods + (page * NC2Layout::HouseWindowLimits::StockGoodsPerPage + index++) * NC2Layout::RecordSize::HouseWindowStockValue, good);
			}
		}

		for (std::uint32_t good = 1; good <= 55; ++good)
		{
			if (IsGoodAllowed(house, good))
			{
				bool found = false;

				for (std::uint32_t page = 0; page < 7; ++page)
				{
					for (std::uint32_t index = 0; index < 14; ++index)
					{
						if (Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::StockGoods + (page * NC2Layout::HouseWindowLimits::StockGoodsPerPage + index) * NC2Layout::RecordSize::HouseWindowStockValue) == good)
							found = true;
					}
				}

				if (!found)
					for (std::uint32_t index = 0; index < 14; ++index)
					{
						if (!Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::UncategorizedStockGoods + index * NC2Layout::RecordSize::HouseWindowStockValue))
						{
							Write(window, NC2Layout::SelectionHouseWindowOffset::UncategorizedStockGoods + index * NC2Layout::RecordSize::HouseWindowStockValue, good);
							break;
						}
					}
			}
		}

		SSelectionHouseWindowMostStockGoodsData most;
		l_SubTabs_StockHouseGoods_GetMostStockGoods(window, &most);

		if (most.count > 14)
			most.count = 14;

		for (std::uint32_t index = 0; index < 14; ++index)
		{
			Write(window, NC2Layout::SelectionHouseWindowOffset::StockGoods + index * NC2Layout::RecordSize::HouseWindowStockValue, index < most.count ? most.entries[index].good : 0u);
		}

		for (std::uint32_t page = 1; page < 7; ++page)
		{
			auto* data = static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::StockGoods + page * 56;
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

	static void* AddGraphic(void* window, const SRectangle& area, std::uint32_t graphic, const char* tooltip, std::uint32_t command)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
		auto* element = memory == nullptr ? nullptr : tooltip == nullptr ? BaseToolGfxButton_Construct(memory, nullptr, &area, graphic, command, window)
		                                                                 : BaseToolGfxButton_ConstructWithToolTip(memory, nullptr, &area, graphic, tooltip, command, window);
		NXBaseGui::BaseWindow_Element_Add(window, element);

		return element;
	}

	static void* AddText(void* window, const SRectangle& area, const char* caption, const char* tooltip, std::uint32_t command)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		auto* element = memory == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &area, caption, tooltip, command, window);
		NXBaseGui::BaseWindow_Element_Add(window, element);

		return element;
	}

	static void AddGood(void* window, const SRectangle& area, const void* house, std::uint32_t good)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelection::SHouseGoodTypeElementStorage), 1);
		auto* element = memory == nullptr ? nullptr : SelectionHouseGoodTypeElement_Construct(memory, &area, window, 10014, 0, Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId), good);
		NXBaseGui::BaseWindow_Element_Add(window, element);
	}

	static void* AddPreview(void* window, const SRectangle& area, const void* house, bool commands)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiWorldElement::SWorldGuiElementStorage), 1);
		auto* preview = memory == nullptr ? nullptr : NC2InGameGuiWorldElement::WorldDisplayElement_Construct(memory, &area, false, false);
		auto* display = Read<void*>(preview, NC2Layout::WorldGuiElementOffset::Display);
		NC2E2::DE_Zoom_SetCounter(display, -1);
		NC2E2::DE_SetWantedPositionToMapPoint(display, reinterpret_cast<const std::uint32_t*>(static_cast<const std::byte*>(house) + 0x1C));
		NC2E2::DE_ControllingPlayer_SetId(display, NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId());
		NC2InGameGuiWorldElement::WorldDisplayElement_DrawFrame_SetMode(preview, 2);

		if (commands)
		{
			NC2InGameGuiWorldElement::WorldDisplayElement_Misc_LeftMouseClick_HandlerSet(preview, window, 10023);
			NC2InGameGuiWorldElement::WorldDisplayElement_Misc_LeftMouseDoubleClick_HandlerSet(preview, window, 10024);
		}

		NXBaseGui::BaseWindow_Element_Add(window, preview);

		return preview;
	}

	static bool Allowed(const NC2Logic::SUserCommandsForHouseHandler& commands, std::uint32_t command)
	{
		for (std::int32_t index = 0; index < commands.count; ++index)
		{
			if (commands.commands[index] == command)
				return true;
		}

		return false;
	}

	// Original function: FUN_004a8644; bottom-right layout and native child creation order.
	static void l_SubTabs_UpdateAll(void* window, bool force)
	{
		const auto checksum = l_SubTabs_GetEnabledChecksum(window);

		if (!force && Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::TabStateChecksum) == checksum && !Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::LayoutInvalidFlag))
			return;

		Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::LayoutInvalidFlag, 0);
		Write(window, NC2Layout::SelectionHouseWindowOffset::TabStateChecksum, checksum);
		std::int32_t width = 0, height = 0;

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
			{
				if (width < gSubTabs[index].width)
					width = gSubTabs[index].width;

				const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SelectionHouseWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::HouseWindowTabHeight);
				height += Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index) ? 16 : 16 + (overrideHeight ? overrideHeight : gSubTabs[index].height);
			}
		}

		Write(window, NC2Layout::SelectionHouseWindowOffset::ComputedWidth, width);
		Write(window, NC2Layout::SelectionHouseWindowOffset::ComputedHeight, height);
		std::int32_t y = 0;

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			SRectangle area(-1, -1, 0, 0);

			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
			{
				const auto overrideHeight = Read<std::int32_t>(window, NC2Layout::SelectionHouseWindowOffset::TabHeightOverrides + index * NC2Layout::RecordSize::HouseWindowTabHeight);
				area = SRectangle(width - gSubTabs[index].width, y, gSubTabs[index].width, Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index) ? 16 : 16 + (overrideHeight ? overrideHeight : gSubTabs[index].height));
				y += area.height;
			}

			std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabRectangles + index * 16, &area, 16);
		}

		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(GetDisplayWidth() - width, GetDisplayHeight() - height, width, height);
		NC2InGameGuiBase::GetDesktopPtr()->Window_ChangeAreaRectangle(static_cast<NXBaseGui::CBaseWindow*>(window), area);
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		Buttons(window).AllButtonsHaveBeenRemoved();

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
			{
				const auto section = Section(window, index);
				const bool collapsed = Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index) != 0;
				auto* button = AddGraphic(window, SRectangle(section.x + section.width - 19, section.y + 2, 18, 12), collapsed ? 19 : 20, HouseString(collapsed ? 100 : 101), 10000);
				Write<std::uintptr_t>(button, NC2Layout::GuiGfxButtonOffset::MessageParameter1, index);
				Write<std::uintptr_t>(button, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0);
			}
		}

		auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
		{

			return;
		}

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index) && !Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
			{
				const auto section = Section(window, index);

				if (index == 0)
				{
					Write(window, NC2Layout::SelectionHouseWindowOffset::PreviewElement, AddPreview(window, SRectangle(section.x + 8, section.y + 24, 180, section.height - 32), house, true));
					SRectangle button(section.x + 192, section.y + 80, 122, 18);
					NC2Logic::SUserCommandsForHouseHandler commands;
					NC2Logic::House_UserControl_GetCommands(house, NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId(), &commands);

					if (Allowed(commands, 2))
					{
						AddText(window, button, HouseString(110), HouseString(111), 10001);
						button.y += button.height + 2;
						button.height = 32;
						char caption[1024]{ }, tooltip[1024]{ };
						NC2Logic::SHouseConstructionGoods goods;
						NC2Logic::House_Construction_CalculateGoodsNeeded(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) + 1u, true, &goods);

						for (std::int32_t item = 0; item < goods.count; ++item)
						{
							const auto good = goods.entries[item].good, amount = goods.entries[item].requiredAmount;

							if (*tooltip)
								std::strcat(tooltip, " - ");

							char text[100];
							NXBasics::Runtime_PrintString(text, "%d %s", amount, NC2Logic::StringAccess_GetGoodNamePtr(good, amount));
							std::strcat(tooltip, text);
							auto remaining = amount;

							while (remaining)
							{
								const auto group = remaining > 5 ? 5u : remaining;
								NXBasics::Runtime_PrintString(text, "{gfxgood:%d,%d}", good, group);
								std::strcat(caption, text);
								remaining -= group;
							}
						}

						auto* costs = AddText(window, button, caption, tooltip, 10001);
						Write<std::uint8_t>(costs, NC2Layout::ToolTextButtonOffset::CenteredFlag, 0);
						button.y += button.height + 2;
						button.height = 18;
					}

					for (auto command : { 3u, 1u })
					{
						if (Allowed(commands, command))
						{
							AddText(window, button, HouseString(command == 3 ? 112 : 114), HouseString(command == 3 ? 113 : 115), command == 3 ? 10002 : 10003);
							button.y += button.height + 2;
						}
					}

					AddText(window, button, HouseString(116), HouseString(117), 10004);
					button.y += button.height + 2;

					if (l_Misc_GetMainWorkerJobType())
					{
						char text[200];
						NXBasics::Runtime_PrintString(text, "%s - %s", HouseString(119), NC2Logic::StringAccess_GetJobNamePtr(l_Misc_GetMainWorkerJobType(), 1));
						AddText(window, button, HouseString(118), text, 10005);
						button.y += button.height + 2;
					}

					AddText(window, button, HouseString(120), HouseString(121), 10006);
				}

				if (index == 4)
				{
					SRectangle button(section.x + 8, section.y + 24, 44, 18);

					for (std::uint32_t page = 0; page < 7; ++page)
					{
						auto* child = AddGraphic(window, button, gStockPageButtons[page][0], HouseString(gStockPageButtons[page][1]), 10013);
						Write<std::uintptr_t>(child, NC2Layout::ToolGfxButtonOffset::CommandData1, page);
						Write<std::uintptr_t>(child, NC2Layout::ToolGfxButtonOffset::CommandData2, 0);
						Write<std::uint8_t>(child, NC2Layout::GuiGfxButtonOffset::Selected, page == Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::StockPage));
						Write<std::uint8_t>(child, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
						Write<std::uint8_t>(child, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
						button.x += button.width + 2;
					}

					SRectangle goodArea(section.x + 8, section.y + 48, 156, 20);
					const auto start = goodArea.y;

					for (std::uint32_t slot = 0; slot < 14; ++slot)
					{
						const auto good = Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::StockGoods + (Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::StockPage) * NC2Layout::HouseWindowLimits::StockGoodsPerPage + slot) * NC2Layout::RecordSize::HouseWindowStockValue);

						if (!good)
							break;

						AddGood(window, goodArea, house, good);
						goodArea.y += 22;

						if (goodArea.y > section.y + section.height - 37)
						{
							goodArea.x += 164;
							goodArea.y = start;
						}
					}
				}

				if (index == 5 || index == 6)
				{
					const auto* type = NC2Logic::House_GetHouseTypeDataPtr(house);
					std::int32_t row = 0, column = 0;

					for (std::uint32_t stock = 0; stock < type->stockCount; ++stock)
					{
						const auto& entry = type->stock[stock];

						if (entry.goodType > 55 || entry.isOutputStock)
							continue;

						const SRectangle goodArea(index == 5 ? section.x + 124 : section.x + 8 + column * 162, section.y + (index == 5 ? 36 : 44) + row * 18, index == 5 ? width - section.x - 132 : 158, 16);
						AddGood(window, goodArea, house, entry.goodType);

						if (index == 5)
						{
							++row;
						}
						else if (++column == 2)
						{
							column = 0;
							++row;
						}
					}
				}

				if ((index == 4 || index == 5 || index == 6) && Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood))
				{
					SRectangle button(section.x + section.width - 49, section.y + section.height - 21, 20, 16);
					AddGraphic(window, button, 27, nullptr, 10015);
					button.x += button.width;
					AddGraphic(window, button, 26, nullptr, 10016);
				}

				if (index == 8)
				{
					const SRectangle child(section.x + 4, section.y + 20, section.width - 8, section.height - 24);
					auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelection::SHouseInhabitantsSelectionStorage), 1);
					NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : SelectionHouseInhabitantsElement_Construct(memory, &child, window, 10007, 10008, Selection_GetSelectedHouseId(gInputManager)));
				}

				if (index == 9)
				{
					AddPreview(window, SRectangle(section.x + 8, section.y + 24, 180, section.height - 32), house, false);
					AddText(window, SRectangle(section.x + 200, section.y + section.height - 39, 30, 30), "->", HouseString(117), 10004);
				}
			}
		}
	}

	// Original function: FUN_004a7154; command IDs and packet fields come from the Windows branches.
	static bool __fastcall SelectionHouseWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t firstPayload, std::uintptr_t secondPayload, std::uintptr_t)
	{
		const auto first = static_cast<std::uint32_t>(firstPayload);
		const auto second = static_cast<std::uint32_t>(secondPayload);
		auto* house = l_Misc_GetSelectedHousePtr();

		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		const auto send = [&](std::uint8_t type, std::uint32_t value1 = 0, std::uint32_t value2 = 0)
		{
			if (house == nullptr)
			{

				return;
			}

			std::byte packet[20]{ };
			Write(packet, NC2Layout::UserNetworkCommandOffset::Opcode, type);
			Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
			Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter1, value1);
			Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter2, value2);
			NC2Logic::UserNetworkCommand_AddHouseCommand(NC2Logic::gUserNetworkCommandManager, packet);
		};

		switch (command)
		{
			case SwitchIds::HouseWindowCommand::Update:
				if (house == nullptr || Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId) == manager->GetLocalPlayerId())
				{
					const bool construction = house != nullptr && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3;

					if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::ConstructionTabEnabled) != static_cast<std::uint8_t>(construction))
					{
						Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::ConstructionTabEnabled, construction);

						if (construction)
							Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::ConstructionTabCollapsed, 0);
					}

					if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::StockHouseTabEnabled) && !Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::StockHouseTabCollapsed))
					{
						SSelectionHouseWindowMostStockGoodsData most;
						l_SubTabs_StockHouseGoods_GetMostStockGoods(window, &most);

						if (most.count > 14)
							most.count = 14;

						for (std::uint32_t index = 0; index < most.count; ++index)
						{
							if (Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::StockGoods + index * NC2Layout::RecordSize::HouseWindowStockValue) != most.entries[index].good)
							{
								l_SubTabs_StockHouseGoods_Init(window);
								l_SubTabs_UpdateAll(window, true);
								break;
							}
						}
					}

					house = l_Misc_GetSelectedHousePtr();
					Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::DefenceTabEnabled, house != nullptr && NC2Logic::House_CanEnableDefenceMode(house));
				}

				l_SubTabs_UpdateAll(window, false);
				break;
			case SwitchIds::HouseWindowCommand::ToggleSubTab:
				if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + second))
				{
					Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + second, !Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + second));
					l_SubTabs_UpdateAll(window, false);
				}

				break;
			case SwitchIds::HouseWindowCommand::StartUpgrade:
				send(0x5B);
				break;
			case SwitchIds::HouseWindowCommand::StopUpgrade:
				send(0x5C);
				break;
			case SwitchIds::HouseWindowCommand::ConfirmRemove:
				if (house != nullptr)
					manager->MiscWindows_HouseRemoveWindow_Open();

				break;
			case SwitchIds::HouseWindowCommand::CenterHouse:
			case SwitchIds::HouseWindowCommand::CenterHouseAndStopFollowing:
				if (house != nullptr)
				{
					NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(manager->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), reinterpret_cast<const std::uint32_t*>(static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition));

					if (command == 10023)
						NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
				}

				break;
			case SwitchIds::HouseWindowCommand::OpenWorkerList:
				if (house != nullptr && l_Misc_GetMainWorkerJobType())
					manager->MiscWindows_HumanListWindow_Open(0, l_Misc_GetMainWorkerJobType());

				break;
			case SwitchIds::HouseWindowCommand::OpenHouseHelp:
				if (house != nullptr)
					manager->MiscWindows_HelpWindow_Open_HouseInfo(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage));

				break;
			case SwitchIds::HouseWindowCommand::SelectHuman:
			case SwitchIds::HouseWindowCommand::SelectHumanAndShowActions:
				Selection_SelectSingleHuman(gInputManager, first);

				if (command == 10008)
					manager->Selection_ActionButtons_BringUp();

				break;
			case SwitchIds::HouseWindowCommand::DecreaseProductionCounter:
			case SwitchIds::HouseWindowCommand::IncreaseProductionCounter:
				send(0x5F, command == 10010, 1);
				break;
			case SwitchIds::HouseWindowCommand::StopProduction:
			case SwitchIds::HouseWindowCommand::ResumeProduction:
				send(0x5E, command == 10011);
				break;
			case SwitchIds::HouseWindowCommand::SelectStockView:
				Write(window, NC2Layout::SelectionHouseWindowOffset::StockPage, second);
				l_SubTabs_UpdateAll(window, true);
				break;
			case SwitchIds::HouseWindowCommand::SelectStockGood:
				Write(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood, first);
				l_SubTabs_UpdateAll(window, true);
				break;
			case SwitchIds::HouseWindowCommand::DecreaseStockMinimum:
			case SwitchIds::HouseWindowCommand::IncreaseStockMinimum:
				if (house != nullptr && Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood))
				{
					const auto good = Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood);
					const auto step = (Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) ? 10u : 1u;
					const auto previous = NC2Logic::House_Stock_GetMinimumValue(house, good);
					auto amount = NXBasics::Runtime_TimeSigned32(command == 10015 ? previous - step : previous + step);

					if (command == 10015 && amount < 0)
						amount = 0;

					if (command == 10016 && amount > 100)
						amount = 100;

					send(0x60, good, amount);
				}

				break;
			case SwitchIds::HouseWindowCommand::DisableFoodEnhancer:
			case SwitchIds::HouseWindowCommand::EnableFoodEnhancer:
			case SwitchIds::HouseWindowCommand::DisableSocialEnhancer:
			case SwitchIds::HouseWindowCommand::EnableSocialEnhancer:
			case SwitchIds::HouseWindowCommand::DisableReligionEnhancer:
			case SwitchIds::HouseWindowCommand::EnableReligionEnhancer:
				send(0x5D, (command - 10017) / 2, (command - 10017) % 2);
				break;
			case SwitchIds::HouseWindowCommand::ShowActionButtons:
				manager->Selection_ActionButtons_BringUp();
				break;
			case SwitchIds::HouseWindowCommand::EnableDefence:
			case SwitchIds::HouseWindowCommand::DisableDefence:
				send(0x61, command == 10025);
				break;
			case SwitchIds::HouseWindowCommand::SelectSoldiers:
				if (house != nullptr)
				{
					std::int32_t soldiers[10], count = 0;

					for (std::uint32_t slot = 0; slot < 20; ++slot)
					{
						const auto* attachment = NC2Logic::House_Inhabitants_GetAttachedHumanSlotPtrBySlotId(house, slot);
						const auto flags = Read<std::uint8_t>(attachment, NC2Layout::HouseAttachmentOffset::Flags);

						if (!flags || !(flags & 2u))
							continue;

						const auto id = Read<std::int32_t>(attachment, NC2Layout::HouseAttachmentOffset::HumanId);
						const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::ElementSize;

						if (NC2Logic::Human_IsJobSoldier(human))
						{
							soldiers[count++] = id;

							if (count == 10)
								break;
						}
					}

					for (std::int32_t index = 0; index < count; ++index)
					{
						std::byte packet[20]{ };
						Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x1D);
						Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, soldiers[index]);
						NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, packet);
						NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, soldiers[index]);
					}
				}

				break;
		}

		return true;
	}

	// Original function: FUN_004a7895; all eleven sections retain the native drawing order.
	static void __fastcall SelectionHouseWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		Tooltips(window).ClearAll();

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
			{
				auto header = Section(window, index);
				header.x += rectangle->x;
				header.y += rectangle->y;
				header.height = 16;

				if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
				{
					header.x += header.width - 200;
					header.width = 200;
				}

				SelectionTool_FillRectangle(bitmap, header, 5);
				SelectionTool_PrintCenteredString(bitmap, header, true, HouseString(gSubTabs[index].title));

				if (!Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
				{
					auto body = Section(window, index);
					body.x += rectangle->x;
					body.y += rectangle->y + 16;
					body.height -= 16;
					SelectionTool_FillRectangle(bitmap, body, 1);
				}
			}
		}

		auto* house = l_Misc_GetSelectedHousePtr();

		if (house == nullptr)
		{

			return;
		}

		const auto* type = NC2Logic::House_GetHouseTypeDataPtr(house);
		const auto* development = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);

		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index) && !Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags + index))
			{
				const auto section = Section(window, index);

				if (index == 0)
				{
					const auto x = section.x + 192, width = section.width - 196;
					auto y = section.y + 22;
					const auto centered = [&](const char* text)
					{
						bitmap->Text_Print(font, color, x + (width - NXBasics::Font_GetPixelWidth(font, text)) / 2, y, text);
					};
					centered(NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1));
					y += 16;
					const auto health = Read<std::uint32_t>(house, NC2Layout::HouseOffset::Hitpoints), maximum = Read<std::uint32_t>(development, NC2Layout::EngineHouseLogicViewOffset::MaximumHitpoints + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValue);
					SRectangle bar(x, y + 2, width, 8);
					DrawTool_DrawFrame(bitmap, bar, 2);
					bar.Inflate(-2, -2);
					bar.width = static_cast<std::int32_t>(static_cast<std::uint32_t>(bar.width) * health / maximum);
					auto shade = health * 255u / maximum;

					if (shade > 255)
						shade = 255;

					bitmap->Draw_Box(bar, *reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.hitpointsBarPalette) + NXBasics::PaletteOffset::Colors + shade * 4));
					char text[300];
					NXBasics::Runtime_PrintStringN(text, 100, "%s: %d%%", HouseString(20), health * 100u / maximum);
					text[99] = 0;
					Tooltips(window).Add(SRectangle(x, y, width, 12), text);
					y += 11;

					if (NC2Logic::House_GetHouseMainType(house) == 1)
					{
						const auto difficulty = Read<std::uint32_t>(NC2Logic::gGameControl, NC2Layout::GameControlOffset::DifficultyLevel);
						const auto factor = difficulty == 1 ? 2u : difficulty == 2 ? 5u
						                                                           : 1u;
						NXBasics::Runtime_PrintString(text, "%s: %d", HouseString(21), static_cast<std::uint32_t>(type->stock[0].maximumAmount) * factor);
						centered(text);
					}
				}

				if (index == 1)
				{
					const auto startX = section.x + 8;
					auto x = startX, y = section.y + 22;
					NC2Logic::SUserCommandsForHouseHandler commands;

					if (NC2Logic::Logic_UserControl_GetHouseCommands(Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId), NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId(), &commands))
					{
						if (Allowed(commands, 14))
						{
							Buttons(window).UpdateGfxButton(10025, 0, x, y + 2, 30, HouseString(140));
							x += 30;
						}
						else
						{
							Buttons(window).RemoveButton(10025, 0);
						}

						if (Allowed(commands, 15))
						{
							Buttons(window).UpdateGfxButton(10026, 0, x, y + 2, 29, HouseString(141));
							x += 30;
						}
						else
						{
							Buttons(window).RemoveButton(10026, 0);
						}

						bitmap->Text_Print(font, color, x, y, HouseString(Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) ? 142 : 143));
						y += 20;

						if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 1)
						{
							Buttons(window).UpdateGfxButton(10027, 0, startX, y + 2, 29, HouseString(144));
							y += 18;
						}
						else
						{
							Buttons(window).RemoveButton(10027, 0);
						}
					}

					l_SubTabs_OverwriteHeight(window, 1, y - section.y - 8);
				}

				if (index == 2)
				{
					const auto x = section.x + 8;
					auto y = section.y + 18;
					const auto state = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState);

					if (state == 1 || state == 2)
					{
						bitmap->Text_PrintV(font, color, x, y, "%s (%d%%)", HouseString(state == 1 ? 22 : 23), NC2Logic::House_Construction_GetPercentage(house));
						y += 16;
					}

					for (std::int32_t goodIndex = 0; goodIndex < Read<std::int32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount); ++goodIndex)
					{
						const auto* entry = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::ConstructionGoodEntriesSigned + goodIndex * 20;
						const auto good = Read<std::uint32_t>(entry, NC2Layout::HouseConstructionGoodOffset::GoodType);
						DrawTool_PrintStringXV(bitmap, x, y, font, DrawTool_GetFontColorRef(), "{gfxgood:%d,1} %s   %d - %d - %d", good, NC2Logic::StringAccess_GetGoodNamePtr(good, 1), Read<std::uint32_t>(entry, NC2Layout::HouseConstructionGoodOffset::RequiredAmount), Read<std::uint32_t>(entry, NC2Layout::HouseConstructionGoodOffset::DeliveredAmount), Read<std::uint32_t>(entry, NC2Layout::HouseConstructionGoodOffset::ConsumedAmount));
						y += 12;
					}

					l_SubTabs_OverwriteHeight(window, 2, y - section.y - 8);
				}

				if (index == 3)
				{
					const auto x = section.x + 8;
					auto y = section.y + 22;
					NC2Logic::SUserCommandsForHouseHandler commands;
					NC2Logic::House_UserControl_GetCommands(house, NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId(), &commands);

					for (std::uint32_t enhancer = 0; enhancer < 3; ++enhancer)
					{
						const auto offset = NC2Layout::HouseOffset::HomeQualityEnhancers + enhancer * 8;
						const auto* state = HouseString(Read<std::uint8_t>(house, offset) ? 50 : 51);
						const auto* name = HouseString(enhancer == 0 ? 41 : enhancer == 1 ? 44 : 47);
						const auto* prefix = HouseString(40);
						DrawTool_PrintStringXV(bitmap, x + 20, y, font, color, "%s %s (%s){setx:%d}%d", prefix, name, state, x + 120, Read<std::uint32_t>(house, offset + NC2Layout::HomeQualityEnhancerOffset::Value));
						const auto enable = 10018u + enhancer * 2, disable = 10017u + enhancer * 2;

						if (Allowed(commands, 4 + enhancer * 2))
						{
							Buttons(window).UpdateGfxButton(enable, 0, x, y, 30, HouseString(42 + enhancer * 3));
						}
						else
						{
							Buttons(window).RemoveButton(enable, 0);
						}

						if (Allowed(commands, 5 + enhancer * 2))
						{
							Buttons(window).UpdateGfxButton(disable, 0, x, y, 29, HouseString(43 + enhancer * 3));
						}
						else
						{
							Buttons(window).RemoveButton(disable, 0);
						}

						if (enhancer != 2)
							y += 16;
					}

					std::uint32_t families[20];
					y += 20;
					const auto count = NC2Logic::House_Inhabitants_GetNumberOfFamiliesLivingInHome(house, families);
					bitmap->Text_PrintV(font, color, x, y, "%s: %d/%d", HouseString(52), count, type->homeSize);
					l_SubTabs_OverwriteHeight(window, 3, y - section.y + 4);
				}

				if ((index == 4 || index == 5 || index == 6) && Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood))
				{
					const auto good = Read<std::uint32_t>(window, NC2Layout::SelectionHouseWindowOffset::SelectedStockGood);
					const auto* name = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
					const auto x = section.x + 8, y = section.y + section.height - 21;
					DrawTool_PrintStringXV(bitmap, x, y, font, color, "{gfxgood:%d,1} %s: %d", good, HouseString(61), NC2Logic::House_Stock_GetMinimumValue(house, good));
					Tooltips(window).Add(SRectangle(x, y, 40, 16), name);
				}

				if (index == 5)
				{
					const auto x = section.x + 10;
					const auto y = section.y + 18;
					bitmap->Text_PrintV(font, color, x, y, "%s:", HouseString(62));
					bitmap->Text_PrintV(font, color, section.x + 124, y, "%s:", HouseString(63));
					auto outputY = y + 18, inputY = y + 12;

					for (std::uint32_t stock = 0; stock < type->stockCount; ++stock)
					{
						const auto& entry = type->stock[stock];

						if (entry.goodType > 55)
							continue;

						const auto amount = NC2Logic::House_Stock_GetAmountOfGoodX(house, entry.goodType, IncludesFutureStock(house));

						if (entry.isOutputStock)
						{
							DrawTool_PrintStringXV(bitmap, x, outputY, font, DrawTool_GetFontColorRef(), "{gfxgood:%d,1}    %.1f", entry.goodType, static_cast<double>(amount));
							Tooltips(window).Add(SRectangle(x, outputY, 100, 12), NC2Logic::StringAccess_GetGoodNamePtr(entry.goodType, 1));
							outputY += 14;
						}
						else
						{
							inputY += 18;
						}
					}

					if (outputY < inputY)
						outputY = inputY;

					l_SubTabs_OverwriteHeight(window, 5, outputY - section.y + 12);
				}

				if (index == 6)
				{
					const auto x = section.x + 8;
					auto y = section.y + 20;

					for (std::uint32_t stock = 0; stock < type->stockCount; ++stock)
					{
						const auto& entry = type->stock[stock];

						if (entry.goodType > 55)
							continue;

						const auto amount = NC2Logic::House_Stock_GetAmountOfGoodX(house, entry.goodType, IncludesFutureStock(house));

						if (!entry.isOutputStock)
							continue;

						DrawTool_PrintStringXV(bitmap, x, y, font, DrawTool_GetFontColorRef(), "{gfxgood:%d,1}    %.1f", entry.goodType, static_cast<double>(amount));
						Tooltips(window).Add(SRectangle(x, y, 100, 12), NC2Logic::StringAccess_GetGoodNamePtr(entry.goodType, 1));
						y += 14;
					}
				}
				// The opponent preview names every visible building, not only warehouses.

				if (index == 9)
					bitmap->Text_Print(font, color, section.x + 196, section.y + 24, NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1));

				if (index == 10)
				{
					auto y = section.y + 18;

					for (auto agreement = NC2Logic::HumanMerchantArray_GetNextTradeAgreementId(NC2Logic::gHumanMerchantArray, house, NXBasics::IntegerRange::UInt32Maximum); agreement != NXBasics::IntegerRange::UInt32Maximum; agreement = NC2Logic::HumanMerchantArray_GetNextTradeAgreementId(NC2Logic::gHumanMerchantArray, house, agreement))
					{
						const auto* data = static_cast<const std::byte*>(NC2Logic::gHumanMerchantArray) + NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + agreement * NC2Layout::RecordSize::TradeAgreementUnsigned;
						const auto* secondName = NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::OutgoingGood), Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::OutgoingAmount));
						const auto* secondPrefix = HouseString(91);
						const auto* firstName = NC2Logic::StringAccess_GetGoodNamePtr(Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::IncomingGood), Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::IncomingAmount));
						const auto* firstPrefix = HouseString(90);
						bitmap->Text_PrintV(font, color, section.x + 4, y, "%s %d %s - %s %d %s", firstPrefix, Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::IncomingAmount), firstName, secondPrefix, Read<std::uint32_t>(data, NC2Layout::TradeAgreementOffset::OutgoingAmount), secondName);
						y += 12;
					}
				}
			}
		}
	}

	// Original function: FUN_004a852b; native inclusive far-edge hit convention.
	static bool __fastcall SelectionHouseWindow_IsHit(void* window, void*, const std::int32_t* point)
	{
		for (std::uint32_t index = 0; index < 11; ++index)
		{
			if (Read<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags + index))
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

	// Original function: FUN_004a8562.
	static bool __fastcall SelectionHouseWindow_ToolTip_Available(void* window, void*, const char** output)
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

	// Original function: FUN_004a70cc, deleting wrapper for 004A70FA.
	static void* __fastcall SelectionHouseWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_UnRegister(window);
		gSelectionHouseWindowStatics[0] = std::byte{ 1 };
		std::memcpy(gSelectionHouseWindowStatics + 1, static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags, 11);
		Tooltips(window).~CStaticToolTipHandler();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable);
		NXBaseGui::BaseWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Complete native table 004F6468..004F64B3.
	static const std::array<std::uintptr_t, 19> gHouseWindowTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseWindow_Delete>());
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseWindow_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseWindow_Draw>());
		t[8] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseWindow_IsHit>());
		t[13] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseWindow_ToolTip_Available>());
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);

		return t;
	}();

	// Original function: FUN_004a70e8; the receiver is the embedded SVars at window +0x70.
	static void* SelectionHouseWindow_Vars_Construct(void* vars)
	{
		::new (static_cast<std::byte*>(vars) + NC2Layout::SelectionHouseWindowVarsOffset::Tooltips) NC2InGameGuiBase::CStaticToolTipHandler;

		return vars;
	}

	// Original function: FUN_004a6f10; native allocation 0x3CE0.
	void* SelectionHouseWindow_Construct(void* window)
	{
		const SRectangle area(0, 0, 1, 1);
		BaseToolWindow_Construct(window, &area);
		SelectionHouseWindow_Vars_Construct(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags);
		new (static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::DynamicButtons) CDynamicSubButtonsHandler;
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags, 0, NC2Layout::RecordSize::SelectionHouseWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHouseWindowTable.data());
		Buttons(window).SetWindow(window);
		auto* house = l_Misc_GetSelectedHousePtr();

		if (house != nullptr)
		{
			if (Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId) == NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId())
			{
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::TabEnabledFlags, 1);
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::DefenceTabEnabled, NC2Logic::House_CanEnableDefenceMode(house));
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::ConstructionTabEnabled, Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3);
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::HomeTabEnabled, NC2Logic::House_GetHouseMainType(house) == 2);

				if (NC2Logic::House_GetHouseMainType(house) == 1)
				{
					Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::StockHouseTabEnabled, 1);
				}
				else
				{
					Write<std::uint8_t>(window, Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 39 ? NC2Layout::SelectionHouseWindowOffset::BarrackTabEnabled : NC2Layout::SelectionHouseWindowOffset::ProductionTabEnabled, 1);
				}

				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::InhabitantsTabEnabled, 1);
			}
			else
			{
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::OpponentTabEnabled, 1);
				Write<std::uint8_t>(window, NC2Layout::SelectionHouseWindowOffset::OpponentTradeTabEnabled, l_Misc_IsOpponentTradeHouseSelected());
			}
		}

		if (gSelectionHouseWindowStatics[0] != std::byte{ 0 })
			std::memcpy(static_cast<std::byte*>(window) + NC2Layout::SelectionHouseWindowOffset::TabCollapsedFlags, gSelectionHouseWindowStatics + 1, 11);

		l_SubTabs_StockHouseGoods_Init(window);
		{
			NXBasics::CBitmap scratch(10, 10, 1);
			const SRectangle bounds(0, 0, 10, 10);
			SelectionHouseWindow_Draw(window, nullptr, &scratch, &bounds);
		}
		l_SubTabs_UpdateAll(window, true);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);

		return window;
	}
}