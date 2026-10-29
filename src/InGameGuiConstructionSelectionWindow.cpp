#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiConstructionSelectionWindow.h"
#include "InGameGuiHelpWindow.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"

#include <array>
#include <cstring>
#include <Windows.h>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	static void* __fastcall ConstructionSelectionWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags);
	// Complete Windows table 004F75C0..004F760B.
	static const std::array<std::uintptr_t, 19> gConstructionSelectionWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> table{ };
		std::memcpy(table.data(), gBaseToolWindowVirtualTable, sizeof(table));
		table[NC2Runtime::GuiVirtualSlot::DeletingDestructor] = reinterpret_cast<std::uintptr_t>(&ConstructionSelectionWindow_Delete);
		table[NC2Runtime::GuiVirtualSlot::MessageHandle] = reinterpret_cast<std::uintptr_t>(&ConstructionSelectionWindow_XGui_BE_Message_Handle);

		return table;
	}();

	// Original function: FUN_004c647e; Windows allocation 0x3A8.
	void* ConstructionSelectionWindow_Construct(void* window, std::uint32_t specialItemType, std::uint32_t specialItem)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - NC2Runtime::ConstructionSelectionDimensions::WidthUnsigned) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - NC2Runtime::ConstructionSelectionDimensions::HeightUnsigned) / 2, NC2Runtime::ConstructionSelectionDimensions::Width, NC2Runtime::ConstructionSelectionDimensions::Height);
		BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::ConstructionSelectionWindowOffset::Page, 0, NC2Layout::RecordSize::ConstructionSelectionWindowState);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gConstructionSelectionWindowVirtualTable.data());
		Write(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType, specialItemType);
		Write(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItem, specialItem);
		ConstructionSelectionWindow_l_GuiButtons_Add(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);
		ConstructionSelectionWindow_XGui_BE_Message_Handle(window, nullptr, SwitchIds::GuiWindowMessage::Command, SwitchIds::ConstructionSelectionCommand::Refresh, 0, 0, 0);

		return window;
	}

	// Original function: FUN_004c651e; combined destructor/deleting-destructor entry.
	static void* __fastcall ConstructionSelectionWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gConstructionSelectionWindowVirtualTable.data());
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_UnRegister(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004c6554; both native query IDs have distinct semantics.
	bool __fastcall ConstructionSelectionWindow_XGui_BE_Message_Handle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		const auto rebuild = [window]
		{
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			Write<void*>(window, NC2Layout::ConstructionSelectionWindowOffset::HouseList, nullptr);
			ConstructionSelectionWindow_l_GuiButtons_Add(window);
		};

		if (message == SwitchIds::GuiWindowMessage::Query)
		{
			if (command == SwitchIds::GuiWindowQuery::ConstructionSelection)
				return true;

			if (command != SwitchIds::GuiWindowQuery::ConstructionSelectionWithItem)
				return false;

			if (param1 != Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType) || param2 != Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItem))
			{
				Write(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType, param1);
				Write(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItem, param2);
				rebuild();
			}

			return true;
		}

		if (message != SwitchIds::GuiWindowMessage::Command)
			return false;

		if (command >= SwitchIds::ConstructionSelectionCommand::FirstPage && command < SwitchIds::ConstructionSelectionCommand::PageLimit)
		{
			Write(window, NC2Layout::ConstructionSelectionWindowOffset::Page, command - SwitchIds::ConstructionSelectionCommand::FirstPage);
			rebuild();
			return true;
		}

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		auto* input = NC2InGameGuiInputManager::gInputManager;
		const auto select = [&](std::uint32_t type, std::uint32_t stage)
		{
			NC2InGameGuiInputManager::Selection_SelectEngineHouseType(input, type, stage, Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType), Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItem));
			NC2InGameGuiInputManager::InputMode_Set(input, SwitchIds::InputMode::HousePlacement);
		};

		if (command == SwitchIds::ConstructionSelectionCommand::SelectHouse)
		{
			select(param2, param3);
			manager->MiscWindows_ConstructionSelectionWindow_Close();
		}
		else if (command == SwitchIds::ConstructionSelectionCommand::HouseHelp)
		{
			manager->MiscWindows_HelpWindow_Open_HouseInfo(param2, param3);
		}
		else if (command == SwitchIds::ConstructionSelectionCommand::GoodHelp)
		{
			manager->MiscWindows_HelpWindow_Open_GoodInfo(param2);
		}
		else if (command == SwitchIds::ConstructionSelectionCommand::Refresh)
		{
			if (timeGetTime() > Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::LastRefreshTime) + NC2Runtime::ConstructionSelectionLimits::RefreshIntervalMillisecondsUnsigned)
				ConstructionSelectionWindow_l_GuiButtons_UpdateListBox(window);

			if (Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType) != 0 && Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseCount) == 1)
			{
				select(Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseEngineTypes), Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseDevelopmentStages));
				NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
			}
		}
		else if (command >= SwitchIds::ConstructionSelectionCommand::BuildRoad && command <= SwitchIds::ConstructionSelectionCommand::BuildGate)
		{
			const std::int32_t modes[3]{ SwitchIds::InputMode::RoadStart, SwitchIds::InputMode::WallStart, SwitchIds::InputMode::GateStart };
			NC2InGameGuiInputManager::InputMode_Set(input, modes[command - SwitchIds::ConstructionSelectionCommand::BuildRoad]);
			manager->MiscWindows_ConstructionSelectionWindow_Close();
		}
		else if (command == SwitchIds::ConstructionSelectionCommand::GameSettings)
		{
			manager->MiscWindows_GlobalGameSettingsWindow_Open(true, false);
		}
		else if (command == SwitchIds::ConstructionSelectionCommand::NavigateByLetter)
		{
			return Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::InteractionState) == 1 && static_cast<std::uint8_t>(param2) != 0 && NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_CenterOnEntry_StartingWithLetter(Read<void*>(window, NC2Layout::ConstructionSelectionWindowOffset::HouseList), static_cast<char>(param2));
		}

		return true;
	}

	// Original function: FUN_004c672b; five filters, list viewport and normal-mode footer.
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_Add(void* window)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiWindowMover, 1);
		NXBasics::SRectangle rectangle(2, 2, NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22u), 14);
		void* element = allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType) == 0 ? 0 : 1));
		NXBaseGui::BaseWindow_Element_Add(window, element);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiWindowCloser, 1);
		rectangle = NXBasics::SRectangle(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18u), 2, 16, 14);
		element = allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &rectangle);
		NXBaseGui::BaseWindow_Element_Add(window, element);
		const auto button = [window](const NXBasics::SRectangle& area, std::uint32_t caption, std::uint32_t command, bool selected)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
			void* child = memory == nullptr ? nullptr : BaseToolTextButton_ConstructToggle(memory, nullptr, &area, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(caption), command, window, selected);
			NXBaseGui::BaseWindow_Element_Add(window, child);
		};
		rectangle = NXBasics::SRectangle(4, 22, 492 / NC2Runtime::ConstructionSelectionLimits::Pages - 2, 22);

		for (std::uint32_t page = 0; page < NC2Runtime::ConstructionSelectionLimits::Pages; ++page)
		{
			button(rectangle, 2 + page, SwitchIds::ConstructionSelectionCommand::FirstPage + page, Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::Page) == page);
			rectangle.x += rectangle.width + 2;
		}

		rectangle = NXBasics::SRectangle(4, 54, 468, 354 - (Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType) == 0 ? 26 : 0));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiListBox, 1);
		void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &rectangle);
		Write(window, NC2Layout::ConstructionSelectionWindowOffset::HouseList, list);
		NXBaseGui::BaseWindow_Element_Add(window, list);
		ConstructionSelectionWindow_l_GuiButtons_UpdateListBox(window);
		rectangle.x = 476;
		rectangle.width = 20;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiListBoxController, 1);
		element = allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &rectangle, list, false, true);
		NXBaseGui::BaseWindow_Element_Add(window, element);

		if (Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType) != 0)
		{

			return;
		}

		rectangle = NXBasics::SRectangle(4, rectangle.y + rectangle.height + 10, 121, 20);

		for (std::uint32_t index = 0; index < 3; ++index)
		{
			button(rectangle, 7 + index, SwitchIds::ConstructionSelectionCommand::BuildRoad + index, false);
			rectangle.x += rectangle.width + 2;
		}

		rectangle.x += 2;
		button(rectangle, 501, SwitchIds::ConstructionSelectionCommand::GameSettings, false);
	}

	// Original function: FUN_004c6af6; availability, sorting, costs and single-result special items.
	void __fastcall ConstructionSelectionWindow_l_GuiButtons_UpdateListBox(void* window)
	{
		Write<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseCount, 0);
		void* list = Read<void*>(window, NC2Layout::ConstructionSelectionWindowOffset::HouseList);

		if (list == nullptr)
		{

			return;
		}

		const auto scroll = Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ScrollY);
		NC2InGameGuiBaseListBox::BaseListBoxElement_Element_DeleteAll(list);
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		const auto* playerData = static_cast<const std::byte*>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;

		struct SHouseEntry
		{
			std::uint32_t type, stage;
		};

		auto* entries = static_cast<SHouseEntry*>(NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(NC2E2::gStaticHouseCount) * NC2Layout::RecordSize::ConstructionSelectionEntriesPerEngineUnsigned, 1));
		std::uint32_t count = 0;
		const auto page = Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::Page), specialType = Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItemType);

		for (std::int32_t type = 0; type < NC2E2::gStaticHouseCount; ++type)
		{
			const auto* engine = static_cast<const std::byte*>(NC2E2::gStaticHouseEntries) + static_cast<std::uint32_t>(type) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
			const auto tribe = Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::TribeType);

			for (std::uint32_t stage = 0; stage < Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount); ++stage)
			{
				const auto logicType = Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned);

				if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + logicType) == 0)
					continue;

				const auto mainType = NC2Logic::gStaticHouseTypeStorage.records[logicType].mainType;
				const bool buildable = logicType != 1 && mainType != 6 && mainType != 7 && Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + logicType) != 0;
				bool include = false;

				if (specialType != 0)
				{
					if (tribe == 1)
					{
						if (specialType == 2)
						{
							include = buildable;
						}
						else if (specialType > 2 && specialType <= 4)
						{
							include = logicType == Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::SpecialItem);
						}
					}
				}
				else if (buildable)
				{
					include = page == SwitchIds::ConstructionSelectionPage::Work ? mainType == 3 : page == SwitchIds::ConstructionSelectionPage::Stock  ? mainType == 1
					                                                                           : page == SwitchIds::ConstructionSelectionPage::Home     ? mainType == 2
					                                                                           : page == SwitchIds::ConstructionSelectionPage::Military ? mainType == 5 || logicType == 39
					                                                                                                                                    : true;
				}

				if (!include)
					continue;

				entries[count++] = { static_cast<std::uint32_t>(type), stage };
				const auto cachedHouseCount = Read<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseCount);

				if (cachedHouseCount < NC2Runtime::ConstructionSelectionLimits::CachedHouses)
				{
					Write(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseEngineTypesUnsigned + cachedHouseCount * NC2Layout::RecordSize::ConstructionSelectionHouseEntryUnsigned, type);
					Write(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseDevelopmentStagesUnsigned + cachedHouseCount * NC2Layout::RecordSize::ConstructionSelectionHouseEntryUnsigned, stage);
					Write(window, NC2Layout::ConstructionSelectionWindowOffset::CachedHouseCount, cachedHouseCount + 1);
				}
			}
		}

		if (count != 0)
			NXBasics::Runtime_QuickSort(entries, count, sizeof(SHouseEntry), lsSortHouseTypeCompareFunction);

		const bool expandedRows = page != SwitchIds::ConstructionSelectionPage::All;
		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), expandedRows ? 240 : 32);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto entry = entries[index];
			const auto* engine = static_cast<const std::byte*>(NC2E2::gStaticHouseEntries) + entry.type * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
			const auto* name = NC2Logic::StringAccess_GetHouseNamePtr(entry.type, entry.stage, 1);
			std::int32_t unfinished = 0;
			const auto total = NC2Logic::Tool_PlayerStatistics_CountHousesOfLogicType(player, Read<std::uint32_t>(engine, NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned + entry.stage * NC2Layout::RecordSize::HouseStageValueUnsigned), &unfinished);
			char caption[600], text[180];
			caption[0] = '\0';

			if (expandedRows)
			{
				NXBasics::Runtime_PrintString(text, "{setx:240}{sety:70}{gfxhouse:%d,%d}", entry.type, entry.stage);
				std::strcat(caption, text);
			}

			NXBasics::Runtime_PrintString(text, "{setx:4}{sety:%d}%s - (%d+%d)", expandedRows ? -100 : 0, name, total - unfinished, unfinished);
			std::strcat(caption, text);
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &row, caption, SwitchIds::ConstructionSelectionCommand::SelectHouse, window);
			Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 0);
			Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, entry.type);
			Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, entry.stage);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			NXBasics::SRectangle control(row.width, row.y + 2, 0, 28);
			NC2Logic::SHouseConstructionGoods goods{ };
			NC2Logic::House_Construction_CalculateGoodsNeeded(entry.type, entry.stage, false, &goods);

			for (std::int32_t slot = 0; slot < goods.count; ++slot)
			{
				const auto good = goods.entries[slot].good, amount = goods.entries[slot].requiredAmount;
				char tooltip[132], graphic[32];
				NXBasics::Runtime_PrintString(graphic, "{gfxgood:%d,%d}", good, amount);
				NXBasics::Runtime_PrintString(tooltip, "%d %s", amount, NC2Logic::StringAccess_GetGoodNamePtr(good, amount));
				control.x -= 30;
				control.width = 28;
				allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
				element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &control, graphic, SwitchIds::ConstructionSelectionCommand::GoodHelp, window);
				Write<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, 0);
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::CenteredFlag, 0);
				Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, good);
				BaseToolTextButton_SetToolTipString(element, tooltip);
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::ImmediateTooltipFlag, 1);
				BaseListBoxElement_Element_Add(list, nullptr, element);
			}

			control.x -= 22;
			control.width = 20;
			allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiGfxButton, 1);
			element = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &control, 143, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(10), SwitchIds::ConstructionSelectionCommand::HouseHelp, window);
			Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1, entry.type);
			Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2, entry.stage);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height + 2;
		}

		if (count == 0)
		{
			const NXBasics::SRectangle empty(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 28);
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextElement, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextElement_Construct(allocation, nullptr, &empty, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(12), false, false);
			BaseListBoxElement_Element_Add(list, nullptr, element);
		}

		NXBasics::Runtime_Free(entries);
		NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(list, 0, scroll);
		Write<std::uint32_t>(window, NC2Layout::ConstructionSelectionWindowOffset::LastRefreshTime, timeGetTime());
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a24a4.
	void CInGameGuiManager::MiscWindows_ConstructionSelectionWindow_Toggle(std::uint32_t type, std::uint32_t item)
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::ConstructionSelectionWithItem, type, item, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_ConstructionSelectionWindow_Open(type, item);
		}
	}

	// Original function: FUN_004a24e6; this window does not close other large windows.
	void CInGameGuiManager::MiscWindows_ConstructionSelectionWindow_Open(std::uint32_t type, std::uint32_t item)
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::ConstructionSelectionWithItem, type, item, 0) != nullptr)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::ConstructionSelectionWindow, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::ConstructionSelectionWindow_Construct(allocation, type, item);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}

	// Original function: FUN_004a2533.
	void CInGameGuiManager::MiscWindows_ConstructionSelectionWindow_Close()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::ConstructionSelection, 0, 0, 0);

		if (window != nullptr)
			desktop->Window_Close(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
	}

	// Original function: FUN_004a255a; sign-extend the Windows char parameter.
	bool CInGameGuiManager::MiscWindows_ConstructionSelectionWindow_SendKey(std::uint32_t key, char character)
	{
		void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::ConstructionSelection, 0, 0, 0);

		if (window == nullptr)
			return false;

		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		auto* methods = NC2Lifetime::Read<TMessage*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable);

		return methods[NC2Runtime::GuiVirtualSlot::MessageHandle](window, SwitchIds::GuiWindowMessage::Command, SwitchIds::ConstructionSelectionCommand::NavigateByLetter, key, static_cast<std::uint32_t>(static_cast<std::int32_t>(character)), 0);
	}
}