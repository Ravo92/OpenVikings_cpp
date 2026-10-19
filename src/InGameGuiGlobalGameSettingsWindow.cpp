#include "C2CallbackManager.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2UserNetworkCommandAccess.h"
#include "GameProfile.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiGlobalGameSettingsWindow.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"

#include <array>
#include <cstring>
#include <Windows.h>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	// Complete Windows static DWORD at 005084CC; persists across window instances.
	static std::uint32_t gGlobalGameSettingsPage = 0;
	static void GlobalGameSettingsWindow_Destruct(void* window);
	static void* __fastcall GlobalGameSettingsWindow_Delete(void* window, void* unusedEdx, std::uint32_t flags);
	// Complete Windows table 004F7458..004F74A3; inherited entries are unchanged.
	static const std::array<std::uintptr_t, 19> gGlobalGameSettingsWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> table{ };
		std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
		table[0] = reinterpret_cast<std::uintptr_t>(&GlobalGameSettingsWindow_Delete);
		table[2] = reinterpret_cast<std::uintptr_t>(&GlobalGameSettingsWindow_XGui_BE_Message_Handle);
		table[3] = reinterpret_cast<std::uintptr_t>(&GlobalGameSettingsWindow_XGui_BE_Element_Draw);
		return table;
	}();

	// Original function: FUN_004c1bf3; the Windows allocation is 0x74 bytes.
	void* GlobalGameSettingsWindow_Construct(void* window, bool specialItemsPage, bool assistantPage)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		Write<std::uint32_t>(window, NC2Layout::GlobalGameSettingsWindowOffset::LastRefreshTime, 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gGlobalGameSettingsWindowVirtualTable.data());
		// Cultures2.exe 004AF67E resets the per-window page to Letters.
		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2) gGlobalGameSettingsPage = 0;
		if (specialItemsPage) gGlobalGameSettingsPage = 0;
		if (assistantPage) gGlobalGameSettingsPage = 1;
		GlobalGameSettingsWindow_l_GuiButtons_Add(window);
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->FrameCall_Register(window);
		manager->Misc_GameTime_Stop_LargeWindowOpened();
		return window;
	}

	// Original function: FUN_004c1c8e.
	static void* __fastcall GlobalGameSettingsWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		GlobalGameSettingsWindow_Destruct(window);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(window);
		return window;
	}

	// Original function: FUN_004c1caa.
	static void GlobalGameSettingsWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gGlobalGameSettingsWindowVirtualTable.data());
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		manager->Misc_GameTime_Continue_LargeWindowClosed();
		manager->FrameCall_UnRegister(window);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004c1cd9; packet layout and update branches follow Windows.
	bool __fastcall GlobalGameSettingsWindow_XGui_BE_Message_Handle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		const auto update = [window]
		{
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			GlobalGameSettingsWindow_l_GuiButtons_Add(window);
		};
		if (message == 0x1000 && command == 0x7D9)
		{
			if (param1 != 0)
			{
				gGlobalGameSettingsPage = 0; update();
			}
			if (param2 != 0)
			{
				gGlobalGameSettingsPage = 1; update();
			}
			return true;
		}
		if (message != 0x800) return false;
		if (command == 0x7EE)
		{
			if (NRemaster::gGameVariant != NRemaster::EGameVariant::Cultures2 && timeGetTime() > Read<std::uint32_t>(window, NC2Layout::GlobalGameSettingsWindowOffset::LastRefreshTime) + 3000u) update();
			return true;
		}
		if (command == 10000 || command == 10002)
		{
			gGlobalGameSettingsPage = command == 10000 ? 0 : 1;
			update(); return true;
		}
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		if (command == 10100)
		{
			if (NXBasics::Runtime_TimeSigned32(param2) >= 2 && NXBasics::Runtime_TimeSigned32(param2) <= 4)
			{
				manager->MiscWindows_ConstructionSelectionWindow_Open(param2, param3);
				NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
			}
			return true;
		}
		std::byte packet[20]{ };
		packet[1] = static_cast<std::byte>(manager->GetLocalPlayerId());
		if (command >= 10200 && command <= 10203)
		{
			packet[0] = static_cast<std::byte>(0x81u + command - 10200u);
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, manager->GetLocalPlayerId(), 9, 0);
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
			return true;
		}
		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2) return true;
		std::uint32_t assistantCommand = 0;
		std::int32_t value = 0;
		bool refresh = false;
		switch (command)
		{
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestBoys: assistantCommand = 10; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestBoys: assistantCommand = 10; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestGirls: assistantCommand = 11; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestGirls: assistantCommand = 11; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestUnarmedSoldiers: assistantCommand = 20; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestUnarmedSoldiers: assistantCommand = 20; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestSpearmen: assistantCommand = 21; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestSpearmen: assistantCommand = 21; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestArchers: assistantCommand = 22; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestArchers: assistantCommand = 22; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DecreaseRequestSwordsmen: assistantCommand = 23; value = -1; break;
			case SwitchIds::GlobalGameSettingsCommand::IncreaseRequestSwordsmen: assistantCommand = 23; value = 1; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipCivilianShoes: assistantCommand = 30; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipCivilianShoes: assistantCommand = 30; value = 1; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipCivilianWoodenTools: assistantCommand = 31; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipCivilianWoodenTools: assistantCommand = 31; value = 1; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipCivilianIronTools: assistantCommand = 32; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipCivilianIronTools: assistantCommand = 32; value = 1; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipCivilianMead: assistantCommand = 33; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipCivilianMead: assistantCommand = 33; value = 1; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipSoldierShoes: assistantCommand = 40; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipSoldierShoes: assistantCommand = 40; value = 1; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::DisableEquipSoldierMead: assistantCommand = 41; refresh = true; break;
			case SwitchIds::GlobalGameSettingsCommand::EnableEquipSoldierMead: assistantCommand = 41; value = 1; refresh = true; break;
			default: return true;
		}
		packet[0] = std::byte{ 0x85 };
		Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, assistantCommand); Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectRevision, value);
		NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, packet, false, false);
		if (refresh) update();
		return true;
	}

	// Original function: FUN_004c2502; both native pages, all controls and periodic update.
	void __fastcall GlobalGameSettingsWindow_l_GuiButtons_Add(void* window)
	{
		using namespace NC2InGameGuiBase;
		const auto button = [window] (const NXBasics::SRectangle& rectangle, const char* caption, std::uint32_t command, bool toggle, bool selected, bool tooltip) -> void*
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			if (allocation == nullptr) return nullptr;
			if (tooltip) return BaseToolTextButton_ConstructToggleWithToolTip(allocation, nullptr, &rectangle, caption, nullptr, command, window, selected);
			return toggle ? BaseToolTextButton_ConstructToggle(allocation, nullptr, &rectangle, caption, command, window, selected) : BaseToolTextButton_Construct(allocation, nullptr, &rectangle, caption, command, window);
		};
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBasics::SRectangle rectangle(2, 2, NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22u), 14);
		void* element = allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(500));
		NXBaseGui::BaseWindow_Element_Add(window, element);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		rectangle = NXBasics::SRectangle(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18u), 2, 16, 14);
		element = allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &rectangle);
		NXBaseGui::BaseWindow_Element_Add(window, element);
		rectangle = NXBasics::SRectangle(4, 22, 244, 22);
		const bool cultures2 = NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2;
		if (cultures2)
		{
			// Cultures2.exe 004AF928..004AF98E: Letters, then Extras, from the base table.
			NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(501), 10000, true, gGlobalGameSettingsPage == 0, false));
			rectangle.x += rectangle.width + 2;
			NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(502), 10002, true, gGlobalGameSettingsPage == 1, false));
		}
		else
		{
			NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(10), 10002, true, gGlobalGameSettingsPage == 1, false));
			rectangle.x += rectangle.width + 2;
			NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(501), 10000, true, gGlobalGameSettingsPage == 0, false));
		}
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		if (cultures2 && gGlobalGameSettingsPage == 1)
		{
			// Cultures2.exe 004AF9C8..004AFC2C: four one-shot equipment requests.
			for (std::uint32_t index = 0; index < 4; ++index)
			{
				const auto y = 76 + static_cast<std::int32_t>(index) * 86;
				rectangle = NXBasics::SRectangle(4, y, 492, 22);
				NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(503 + index * 2), 10200 + index, false, false, false));
				rectangle = NXBasics::SRectangle(4, y + 26, 492, 40);
				allocation = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
				element = allocation == nullptr ? nullptr : BaseToolTextElement_Construct(allocation, nullptr, &rectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(504 + index * 2), false, true);
				if (element != nullptr) Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::SmallFontFlag, 1);
				NXBaseGui::BaseWindow_Element_Add(window, element);
			}
		}
		else if (gGlobalGameSettingsPage == 1)
		{
			const auto numeric = [&] (std::int32_t y, std::uint32_t command)
			{
				rectangle = NXBasics::SRectangle(330, y, 24, 18);
				NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, "-", command, false, false, false));
				rectangle.x += rectangle.width + 2;
				NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, "+", command + 1, false, false, false));
			};
			const auto flag = [&] (std::int32_t y, std::uint32_t command, std::uint32_t offset)
			{
				const bool selected = NC2Logic::GameAssistantManager_l_Assistant_IsPlayerIdOk(NC2Logic::gGameAssistantManager, player) && Read<std::uint8_t>(NC2Logic::gGameAssistantManager, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameAssistantPlayerUnsigned + offset) != 0;
				rectangle = NXBasics::SRectangle(330, y, 48, 18);
				NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(24), command, true, !selected, true));
				rectangle.x += rectangle.width + 2;
				NXBaseGui::BaseWindow_Element_Add(window, button(rectangle, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(23), command + 1, true, selected, true));
			};
			numeric(70, 10300); numeric(90, 10310);
			flag(120, 10340, 0x18); flag(140, 10350, 0x19); flag(160, 10360, 0x1A); flag(180, 10370, 0x1B);
			numeric(230, 10320); numeric(250, 10322); numeric(270, 10324); numeric(290, 10326);
			flag(320, 10440, 0x1C); flag(340, 10470, 0x1D);
		}
		else if (gGlobalGameSettingsPage == 0)
		{
			rectangle = NXBasics::SRectangle(4, 76, 468, 336);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
			void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &rectangle);
			NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 26);
			const auto base = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + 0x4624u;
			for (std::uint32_t index = 0; index < 100; ++index)
			{
				const auto type = Read<std::uint32_t>(NC2Logic::gGameControl, base + index * NC2Layout::RecordSize::PlayerSpecialItemUnsigned);
				const auto item = Read<std::uint32_t>(NC2Logic::gGameControl, base + index * NC2Layout::RecordSize::PlayerSpecialItemUnsigned + NC2Layout::PlayerSpecialItemOffset::ItemUnsigned);
				if (type == 0) continue;
				char caption[200];
				NC2GuiToolsBase::StringTool_GetLogicSpecialItemNamePtr(type, static_cast<std::int32_t>(item), caption);
				element = button(row, caption, 10100, true, false, false);
				Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, type); Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, item);
				Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, type != 2 && type != 3 && type != 4);
				BaseListBoxElement_Element_Add(list, nullptr, element);
				row.y += row.height + 2;
			}
			NXBaseGui::BaseWindow_Element_Add(window, list);
			rectangle.x = 476; rectangle.width = 20;
			allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
			element = allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &rectangle, list, false, true);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		}
		Write<std::uint32_t>(window, NC2Layout::GlobalGameSettingsWindowOffset::LastRefreshTime, timeGetTime());
	}

	// Original function: FUN_004c2f2b; assistant captions are drawn over the native base.
	void __fastcall GlobalGameSettingsWindow_XGui_BE_Element_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2 || gGlobalGameSettingsPage != 1 || NC2Logic::gGameAssistantManager == nullptr) return;
		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = resources.font08;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(resources.fontWhitePalette) + 0x400);
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
		const auto amount = [&] (std::uint32_t offset)
		{
			return NC2Logic::GameAssistantManager_l_Assistant_IsPlayerIdOk(NC2Logic::gGameAssistantManager, player) ? Read<std::uint32_t>(NC2Logic::gGameAssistantManager, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameAssistantPlayerUnsigned + offset) : 0u;
		};
		bitmap->Text_PrintV(font, color, 10, 72, "%s %d", NC2GuiToolsBase::StringTool_GetWonders001StringPtr(11), amount(0));
		bitmap->Text_PrintV(font, color, 10, 92, "%s %d", NC2GuiToolsBase::StringTool_GetWonders001StringPtr(12), amount(4));
		for (std::uint32_t index = 0; index < 4; ++index) bitmap->Text_Print(font, color, 10, 124 + index * 20, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(17 + index));
		for (std::uint32_t index = 0; index < 4; ++index) bitmap->Text_PrintV(font, color, 10, 232 + index * 20, "%s %d", NC2GuiToolsBase::StringTool_GetWonders001StringPtr(13 + index), amount(8 + index * 4));
		bitmap->Text_Print(font, color, 10, 324, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(21));
		bitmap->Text_Print(font, color, 10, 344, NC2GuiToolsBase::StringTool_GetWonders001StringPtr(22));
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a1e63.
	void CInGameGuiManager::MiscWindows_GlobalGameSettingsWindow_Toggle(bool specialItemsPage, bool assistantPage)
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7D9, specialItemsPage, assistantPage, 0);
		if (window != nullptr) desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		else MiscWindows_GlobalGameSettingsWindow_Open(specialItemsPage, assistantPage);
	}

	// Original function: FUN_004a1ea9.
	void CInGameGuiManager::MiscWindows_GlobalGameSettingsWindow_Open(bool specialItemsPage, bool assistantPage)
	{
		l_Selection_RemoveAllGuiElements();
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7D9, specialItemsPage, assistantPage, 0) != nullptr) return;
		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::GlobalGameSettingsWindow_Construct(allocation, specialItemsPage, assistantPage);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}
