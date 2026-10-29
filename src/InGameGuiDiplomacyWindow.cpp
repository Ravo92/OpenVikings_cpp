#include "XPalette.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerCenterPosition.h"
#include "C2StringAccess.h"
#include "C2StringIO.h"
#include "C2TributeAccess.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiDiplomacyWindow.h"
#include "InGameGuiManager.h"
#include "InGameGuiOverviewDisplayAccess.h"
#include "InGameGuiWindowState.h"
#include "MainMenuGuiManager_Access.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include "XStringArray.h"

#include <array>
#include <cstring>
#include <Windows.h>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	static void* __fastcall DiplomacyWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall DiplomacyWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall DiplomacyWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall DiplomacyWindow_DrawOverlays(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	// Complete native virtual table 004F6AF0..004F6B3B.
	static const std::array<std::uintptr_t, 19> gDiplomacyWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> table{ };
		std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
		table[0] = reinterpret_cast<std::uintptr_t>(&DiplomacyWindow_Delete);
		table[2] = reinterpret_cast<std::uintptr_t>(&DiplomacyWindow_Message);
		table[3] = reinterpret_cast<std::uintptr_t>(&DiplomacyWindow_Draw);
		table[18] = reinterpret_cast<std::uintptr_t>(&DiplomacyWindow_DrawOverlays);

		return table;
	}();

	static std::uint32_t SelectedPage()
	{

		return Read<std::uint32_t>(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page);
	}

	static NXBasics::SRectangle OverviewArea(const void* window)
	{
		NXBasics::SRectangle area(0, 0, 0, 0);
		std::memcpy(&area, static_cast<const std::byte*>(window) + NC2Layout::DiplomacyWindowOffset::OverviewRectangle, sizeof(area));

		return area;
	}

	static std::uint32_t Player()
	{

		return NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
	}

	static std::uint32_t OtherPlayer(const void* window)
	{

		return Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerPageSlotsUnsigned + SelectedPage() * NC2Layout::RecordSize::DiplomacyPlayerSlotUnsigned);
	}

	static void Add(void* window, void* element)
	{
		NXBaseGui::BaseWindow_Element_Add(window, element);
	}

	static void* TextButton(void* window, NXBasics::SRectangle rectangle, const char* caption, std::uint32_t command, bool toggle, bool selected = false)
	{
		void* memory = NXBasics::Runtime_Allocate(96);
		void* button = memory == nullptr ? nullptr : toggle ? NC2InGameGuiBase::BaseToolTextButton_ConstructToggle(memory, nullptr, &rectangle, caption, command, window, selected)
		                                                    : NC2InGameGuiBase::BaseToolTextButton_Construct(memory, nullptr, &rectangle, caption, command, window);

		return button;
	}

	static void* TextButton(void* window, NXBasics::SRectangle rectangle, std::uint32_t stringId, std::uint32_t command, bool toggle, bool selected = false)
	{

		return TextButton(window, rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(stringId), command, toggle, selected);
	}

	// Original function: FUN_004aef6a; the native constructor resets the selected page.
	void* DiplomacyWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::DiplomacyWindowOffset::PlayerCount, 0, NC2Layout::RecordSize::DiplomacyWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gDiplomacyWindowVirtualTable.data());
		Write<std::uint32_t>(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page, 0);
		Write(window, NC2Layout::DiplomacyWindowOffset::MissionStrings, NC2Logic::StringIO_GetMissionStringArrayPtr());
		DiplomacyWindow_l_GuiButtons_Add(window);
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->FrameCall_Register(window);
		manager->Misc_GameTime_Stop_LargeWindowOpened();

		return window;
	}

	// Original function: FUN_004af017.
	static void DiplomacyWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gDiplomacyWindowVirtualTable.data());
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->Misc_GameTime_Continue_LargeWindowClosed();
		manager->FrameCall_UnRegister(window);
		delete Read<NXBasics::CStringArray*>(window, NC2Layout::DiplomacyWindowOffset::MissionStrings);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004aeffb.
	static void* __fastcall DiplomacyWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		DiplomacyWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004af0e5; diplomacy and tribute commands use 20-byte packets.
	static bool __fastcall DiplomacyWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t param2, std::uint32_t param3)
	{
		if (message == 0x1000)
			return command == 0x7E3;

		if (message != 0x800)
			return false;

		if (command >= 10000 && command < 10100)
		{
			Write(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page, command - 10000);
			DiplomacyWindow_l_GuiButtons_Update(window);
			return true;
		}

		std::byte packet[20]{ };
		Write<std::uint8_t>(packet, NC2Layout::DiplomacyCommandOffset::PlayerId, static_cast<std::uint8_t>(Player()));

		if (command >= 10100 && command < 10200)
		{
			Write<std::uint8_t>(packet, NC2Layout::DiplomacyCommandOffset::Opcode, NXBasics::IntegerRange::Int8MaximumSigned);
			Write(packet, NC2Layout::DiplomacyCommandOffset::FirstPlayer, Player());
			Write(packet, NC2Layout::DiplomacyCommandOffset::SecondPlayer, OtherPlayer(window));
			Write<std::uint32_t>(packet, NC2Layout::DiplomacyCommandOffset::Parameter, command == 10100 ? 3 : command == 10102 ? 1 : 2);
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
			DiplomacyWindow_l_GuiButtons_Update(window);
		}

		if (command == 11000)
		{
			Write<std::uint8_t>(packet, NC2Layout::DiplomacyCommandOffset::Opcode, 0x80);
			Write(packet, NC2Layout::DiplomacyCommandOffset::FirstPlayer, Player());
			Write(packet, NC2Layout::DiplomacyCommandOffset::SecondPlayer, OtherPlayer(window));
			Write(packet, NC2Layout::DiplomacyCommandOffset::Parameter, param2);
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
		}
		else if (command == 12000)
		{
			const auto* house = NC2Logic::CHouseArray::mStaticVars + param2 * NC2Logic::CHouseArray::ElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == param3)
				NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), reinterpret_cast<const std::uint32_t*>(house + NC2Layout::HouseOffset::MapPosition));
		}
		else
		{
			return true;
		}

		NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));

		return true;
	}

	// Original function: FUN_004af278.
	void DiplomacyWindow_l_GuiButtons_Update(void* window)
	{
		NXBaseGui::BaseWindow_Element_DeleteAll(window);
		Write<void*>(window, NC2Layout::DiplomacyWindowOffset::OverviewElement, nullptr);
		DiplomacyWindow_l_GuiButtons_Add(window);
	}

	// Original function: FUN_004af29c; tabs, tributes, bilateral trade offers and overview.
	void DiplomacyWindow_l_GuiButtons_Add(void* window)
	{
		const NXBasics::SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14), closer(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		void* memory = NXBasics::Runtime_Allocate(248);
		Add(window, memory == nullptr ? nullptr : NC2InGameGuiBase::WindowMoverButton_Construct(memory, &title, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(350)));
		memory = NXBasics::Runtime_Allocate(84);
		Add(window, memory == nullptr ? nullptr : NC2InGameGuiBase::WindowCloserButton_Construct(memory, nullptr, &closer));
		std::memset(static_cast<std::byte*>(window) + NC2Layout::DiplomacyWindowOffset::PlayerCount, 0, NC2Layout::RecordSize::DiplomacyPlayerTabs);
		const auto player = Player(), playerBase = player * NC2Layout::RecordSize::GameControlPlayer;

		for (std::uint32_t other = 0; other < 20; ++other)
		{
			if (Read<std::uint8_t>(NC2Logic::gGameControl, other * NC2Layout::RecordSize::GameControlPlayer) == 0 || (Read<std::uint8_t>(NC2Logic::gGameControl, playerBase + NC2Layout::GameControlPlayerOffset::PlayerSeenFlagsUnsigned + other) == 0 && player != other) || Read<std::uint8_t>(NC2Logic::gGameControl, playerBase + NC2Layout::GameControlPlayerOffset::HideDiplomacyChangesUnsigned + other) != 0)
				continue;

			const auto count = Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerCount);
			const auto hidden = Read<std::uint8_t>(NC2Logic::gGameControl, playerBase + NC2Layout::GameControlPlayerOffset::HideDiplomacyDetailsUnsigned + other);
			Write(window, NC2Layout::DiplomacyWindowOffset::PlayerIdsUnsigned + count * NC2Layout::RecordSize::DiplomacyPlayerSlotUnsigned, other);
			Write(window, NC2Layout::DiplomacyWindowOffset::HiddenPlayerFlagsUnsigned + count, hidden);
			Write(window, NC2Layout::DiplomacyWindowOffset::PlayerCount, count + 1);

			if (hidden == 0)
				Write(window, NC2Layout::DiplomacyWindowOffset::VisiblePlayerCount, Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::VisiblePlayerCount) + 1u);
		}

		NXBasics::SRectangle tab(4, 22, 492, 22);
		const auto visible = Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::VisiblePlayerCount);

		if (visible != 0)
		{
			tab.width = 492 / visible - 2;
			Add(window, TextButton(window, tab, 351, 10000, true, SelectedPage() == 0));
			tab.x += tab.width + 2;

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerCount); ++i)
			{
				const auto other = Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerIdsUnsigned + i * NC2Layout::RecordSize::DiplomacyPlayerSlotUnsigned);

				if (Read<std::uint8_t>(window, NC2Layout::DiplomacyWindowOffset::HiddenPlayerFlagsUnsigned + i) != 0 || other == player)
					continue;

				char name[260];
				NC2Logic::GameControl_Misc_GetPlayerName(NC2Logic::gGameControl, other, true, name);
				auto* button = TextButton(window, tab, name, 10001 + i, true, SelectedPage() == i + 1);
				NC2InGameGuiBase::BaseToolTextButton_SetBackgroundColor(button, NC2E2::Tool_PlayerColors_GetColorRef(other < 20 ? Read<std::uint32_t>(NC2Logic::gGameControl, other * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::ColorIdUnsigned) : 9));
				Write<std::uint8_t>(button, NC2Layout::GuiTextButtonOffset::SkipOverwideCaptionFlag, 1);
				Add(window, button);
				tab.x += tab.width + 2;
			}
		}

		if (SelectedPage() == 0)
		{
			NXBasics::SRectangle area(4, tab.y + tab.height + 8, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 8, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Height) - tab.y - tab.height - 12);
			area.Inflate(-2, -2);
			Write(window, NC2Layout::DiplomacyWindowOffset::OverviewRectangle, area);
			memory = NXBasics::Runtime_Allocate(33132);
			auto* overview = memory == nullptr ? nullptr : NC2InGameGuiOverviewDisplay::WorldOverviewElement_Construct(memory, &area, 2);
			Write(window, NC2Layout::DiplomacyWindowOffset::OverviewElement, overview);
			Add(window, overview);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawCivilians, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawMilitaryHumans, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawAnimals, 0);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawVehicles, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawHouses, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawGuides, 0);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::DrawWalls, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::FullRedrawRequested, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::UsePatternDebugColors, 0);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::FullRedrawRequested, 1);
			Write<std::uint8_t>(overview, NC2Layout::OverviewElementOffset::MouseWheelZoomEnabled, 0);
		}
		else if (OtherPlayer(window) != player)
		{
			const auto other = OtherPlayer(window);

			if (Read<std::uint8_t>(NC2Logic::gGameControl, playerBase + NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned + other) == 0)
			{
				NXBasics::SRectangle choice(4, 122, 162, 16);
				NC2Logic::Logic_EntryPoint_GetDiplomacyState(other, player);
				Add(window, TextButton(window, choice, 352, 10102, true));
				choice.x += choice.width + 2;
				Add(window, TextButton(window, choice, 353, 10101, true));
				choice.x += choice.width + 2;
				Add(window, TextButton(window, choice, 354, 10100, true));
			}

			NXBasics::SRectangle demand(4, 140, 492, 36);
			NC2Logic::CTributeIterator tributes(player, other);

			for (tributes++; tributes.GetCurrent() != nullptr; tributes++)
			{
				const auto* tribute = tributes.GetCurrent();

				if (Read<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Allocated) != 0 && Read<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid) != 0)
					continue;

				auto* strings = Read<NXBasics::CStringArray*>(window, NC2Layout::DiplomacyWindowOffset::MissionStrings);
				const auto* description = strings == nullptr ? "<MISSION STRINGS NOT LOADED>" : strings->GetStringPtr(Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::DescriptionStringId));

				if (description == nullptr)
					description = "<MISSION STRING DOES NOT EXIST>";

				char goods[300]{ }, entry[200], caption[300];

				for (std::uint32_t i = 0; i < Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount); ++i)
				{
					const auto good = Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::FirstGoodUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned), amount = Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::FirstAmountUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned);
					NXBasics::Runtime_PrintString(entry, "%d %s (%d %s)", amount, NC2Logic::StringAccess_GetGoodNamePtr(good, amount), NC2Logic::TributeManager_CountGoodAmountForTribute(player, good), NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(355));

					if (goods[0] != '\0')
						std::strcat(goods, " - ");

					std::strcat(goods, entry);
				}

				std::strcpy(caption, description);
				std::strcat(caption, "\r\n");
				std::strcat(caption, goods);
				auto* button = TextButton(window, demand, caption, 11000, false);
				Write<std::uint8_t>(button, NC2Layout::GuiTextButtonOffset::DisabledFlag, !NC2Logic::TributeData_Possible(tribute));
				NC2InGameGuiBase::BaseToolTextButton_SetToolTipString(button, goods);
				Write(button, NC2Layout::ToolTextButtonOffset::CommandData1, tributes.GetIndex());
				Write<std::uint32_t>(button, NC2Layout::ToolTextButtonOffset::CommandData2, 0);
				Write<std::uint8_t>(button, NC2Layout::GuiTextButtonOffset::SmallFontFlag, 1);
				Add(window, button);
				demand.y += demand.height;
			}

			if (NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, other) == 1 && NC2Logic::Logic_EntryPoint_GetDiplomacyState(other, player) == 1)
			{
				NXBasics::SRectangle listArea(4, 320, 468, 82), row(0, 0, 468, 18);
				memory = NXBasics::Runtime_Allocate(116);
				auto* list = memory == nullptr ? nullptr : ToolListBoxElement_Construct(memory, nullptr, &listArea);
				row.width = Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width);
				NC2Logic::CHouseIterator houses;

				for (auto id = houses.GetFirstHouseId(other); id != -1; id = houses.GetNextHouseId())
				{
					const auto* house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHouseArray::ElementSize;
					const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

					if (type != 7 && type != 8 && type != 9 && type != 1)
						continue;

					if (NC2Logic::GameControl_Diplomacy_GetState(NC2Logic::gGameControl, Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId), player) != 1)
						continue;

					if (player < 16 && (Read<std::uint16_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(house + NC2Layout::HouseOffset::MapPosition), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0)
						continue;

					for (auto offer = NC2Logic::HumanMerchantArray_GetNextTradeAgreementId(NC2Logic::gHumanMerchantArray, house, NXBasics::IntegerRange::UInt32Maximum); offer != NXBasics::IntegerRange::UInt32Maximum; offer = NC2Logic::HumanMerchantArray_GetNextTradeAgreementId(NC2Logic::gHumanMerchantArray, house, offer))
					{
						const auto* agreement = static_cast<const std::byte*>(NC2Logic::gHumanMerchantArray) + NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + offer * NC2Layout::RecordSize::TradeAgreementUnsigned;
						const auto firstGood = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood), firstAmount = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount), secondGood = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingGood), secondAmount = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingAmount);
						char caption[260];
						NXBasics::Runtime_PrintString(caption, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(356), secondAmount, NC2Logic::StringAccess_GetGoodNamePtr(secondGood, secondAmount), firstAmount, NC2Logic::StringAccess_GetGoodNamePtr(firstGood, firstAmount));
						auto* button = TextButton(window, row, caption, 12000, false);
						Write(button, NC2Layout::ToolTextButtonOffset::CommandData1, id);
						Write(button, NC2Layout::ToolTextButtonOffset::CommandData2, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
						BaseListBoxElement_Element_Add(list, nullptr, button);
						row.y += row.height + 2;
					}
				}

				Add(window, list);
				const NXBasics::SRectangle scroll(476, 320, 20, 82);
				memory = NXBasics::Runtime_Allocate(60);
				Add(window, memory == nullptr ? nullptr : ToolListBoxControllerElement_Construct(memory, nullptr, &scroll, list, false, true));
			}
		}

		Write(window, NC2Layout::DiplomacyWindowOffset::LastButtonsUpdateTick, timeGetTime());
	}

	// Original function: FUN_004afb38.
	static void __fastcall DiplomacyWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);

		if (SelectedPage() == 0)
		{
			auto area = OverviewArea(window);
			area.Inflate(2, 2);
			NC2GuiToolsBase::DrawTool_DrawFrame(bitmap, area, 2);
			return;
		}

		const auto player = Player(), other = OtherPlayer(window);

		if (player == other)
		{

			return;
		}

		const auto ourState = NC2Logic::Logic_EntryPoint_GetDiplomacyState(player, other), theirState = NC2Logic::Logic_EntryPoint_GetDiplomacyState(other, player);
		char name[200];
		NC2Logic::GameControl_Misc_GetPlayerName(NC2Logic::gGameControl, other, false, name);
		bitmap->Text_Print(resources.font10, color, 4, 50, name);
		std::int32_t y = 74;

		if (other < 20 && Read<std::uint8_t>(NC2Logic::gGameControl, other * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::DeadFlagUnsigned) != 0)
		{
			bitmap->Text_Print(resources.font08, color, 4, y, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(357));
			y = 92;
		}

		bitmap->Text_PrintV(resources.font08, color, 4, y, "%s: %s", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(358), NC2GuiToolsBase::StringTool_GetLogicDiplomacyStateNamePtr(theirState));
		bitmap->Text_PrintV(resources.font08, color, 4, y + 18, "%s: %s", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(359), NC2GuiToolsBase::StringTool_GetLogicDiplomacyStateNamePtr(ourState));

		if (ourState == 1 && theirState == 1)
			bitmap->Text_PrintV(resources.font08, color, 10, 402, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(360), Read<std::uint32_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + other * NC2Layout::RecordSize::PlayerTradeCounterUnsigned + NC2Layout::GameControlPlayerOffset::GoodsReceivedFromPlayersUnsigned));
	}

	// Original function: FUN_004afcea; player names and diplomacy labels overlay the overview.
	static void __fastcall DiplomacyWindow_DrawOverlays(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
	{
		if (SelectedPage() != 0)
		{

			return;
		}

		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		const auto area = OverviewArea(window);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerCount); ++i)
		{
			const auto other = Read<std::uint32_t>(window, NC2Layout::DiplomacyWindowOffset::PlayerIdsUnsigned + i * NC2Layout::RecordSize::DiplomacyPlayerSlotUnsigned);

			if (other == Player())
				continue;

			std::uint32_t position = 0;

			if (!NC2InGameGuiEntryPoints::C2IGG_EntryPoint_PlayerCenterPosition_Get(other, &position))
				continue;

			std::int32_t point[2];
			NC2InGameGuiOverviewDisplay::WorldOverviewElement_TransformToOverViewDisplayCoord(Read<void*>(window, NC2Layout::DiplomacyWindowOffset::OverviewElement), point, reinterpret_cast<const std::int16_t*>(&position));
			const auto x = area.x + point[0], fontHeight = NXBasics::Font_GetPixelHeight(resources.font08, " ");
			const auto maximumY = area.y + area.height - fontHeight * 2 - 7;
			const auto y = area.y + point[1] < maximumY ? area.y + point[1] : maximumY;
			char name[400];
			NC2Logic::GameControl_Misc_GetPlayerName(NC2Logic::gGameControl, other, false, name);
			const auto drawLabel = [&](const char* text, std::int32_t top, bool fill)
			{
				const auto width = NXBasics::Font_GetPixelWidth(resources.font08, text), height = NXBasics::Font_GetPixelHeight(resources.font08, text);
				NXBasics::SRectangle frame(x - width / 2 - 2, top, width + 6, height + 3);
				frame.PlaceInside(area);

				if (fill)
					bitmap->Draw_Box(frame, *NC2E2::Tool_PlayerColors_GetColorRef(other < 20 ? Read<std::uint32_t>(NC2Logic::gGameControl, other * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::ColorIdUnsigned) : 9));

				bitmap->Tool_Darken(frame);
				NXBasics::CBitmap sub(bitmap, frame);
				NC2GuiToolsBase::DrawTool_DrawFrame(nullptr, &sub, &frame, 2);
				bitmap->Text_Print(resources.font08, color, frame.x + 3, frame.y + (fill ? 2 : 1), text);

				return frame.height;
			};
			const auto height = drawLabel(name, y, true);
			drawLabel(NC2GuiToolsBase::StringTool_GetLogicDiplomacyStateNamePtr(NC2Logic::Logic_EntryPoint_GetDiplomacyState(other, Player())), y + height, false);
		}
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a1f09.
	void CInGameGuiManager::MiscWindows_DiplomacyWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7E3, 0, 0, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_DiplomacyWindow_Open();
		}
	}

	// Original function: FUN_004a1f3d.
	void CInGameGuiManager::MiscWindows_DiplomacyWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E3, 0, 0, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_Allocate(248);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::DiplomacyWindow_Construct(allocation);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}