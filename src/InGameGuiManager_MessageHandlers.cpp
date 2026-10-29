#include "C2CallbackManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameState.h"
#include "C2GameTime.h"
#include "C2HumanIterator.h"
#include "C2HumanNeeds.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2SavegameIO.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiHumanActionButtons.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiKeyAssignmentManager.h"
#include "InGameGuiManager.h"
#include "InGameGuiManager_MessageHandlers.h"
#include "InGameGuiSupportAccess.h"
#include "NetworkSessionAccess.h"
#include "StringInputHandler.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "XBitmap.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBitmapFileBMP.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"

#include <array>
#include <cstring>
#include <Windows.h>

namespace
{
	// Complete original table: 004F4DE0..004F4E27.
	const std::uintptr_t gInGameDefaultMessageHandlerVirtualTable[18] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&InGameGuiManagerDefaultMessageHandlerElement_Delete>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&InGameGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Draw>()),
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
	static_assert(sizeof(gInGameDefaultMessageHandlerVirtualTable) == 18 * sizeof(std::uintptr_t));
}

// Original function: FUN_004a0145; flags zero destroy the embedded handler without freeing it.
void* __fastcall InGameGuiManagerDefaultMessageHandlerElement_Delete(void* element, void* /*unusedEdx*/, std::uint32_t flags)
{
	NC2Lifetime::Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gInGameDefaultMessageHandlerVirtualTable + 0);
	NXBaseGui::BaseElement_Destruct(element);

	if ((flags & 1u) != 0)
		NXBasics::Runtime_Free(element);

	return element;
}

// Original function: FUN_004a3eab; CGuiManagerDefaultMessageHandlerElement constructor.
void* __fastcall InGameGuiManagerDefaultMessageHandlerElement_Construct(void* element)
{
	const NXBasics::SRectangle rectangle(0, 0, 0, 0);
	NXBaseGui::BaseElement_Construct(element, &rectangle, 0, 0x800);
	NC2Lifetime::Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gInGameDefaultMessageHandlerVirtualTable + 0);

	return element;
}

namespace
{
	// Complete Windows keyboard scratch region 00569454..00569477.
	struct SInGameKeyboardHistory
	{
		char characters[21];
		std::byte padding[3];
		std::uint32_t smallMapCounter;
		std::uint32_t largeMapCounter;
		std::uint32_t screenshotCounter;
	};

	static_assert(sizeof(SInGameKeyboardHistory) == 36);
	static SInGameKeyboardHistory gInGameKeyboardHistory{ };

	// Native leading block of FUN_004bc36f, 004BC388..004BC50D.
	static void InGameKeyboard_ProcessHiddenCommands(std::uint8_t character)
	{
		if (character == 0)
		{

			return;
		}

		auto* history = gInGameKeyboardHistory.characters;
		std::memmove(history + 1, history, 19);
		history[20] = 0;
		history[0] = static_cast<char>(character);
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		bool matched = false;

		if (NC2Logic::gGameStaticVars.networkGame == 0 && std::strncmp(history, "nalamkcug", 9) == 0)
		{
			manager->Exploration_Toggle();
			matched = true;
		}

		if (std::strncmp(history, "pamagem", 7) == 0 && !NNetwork::Network_IsInSession())
		{
			NXBasics::CFile::FileSystem_Tool_CreateDirectory("mapshots");
			char filename[200];
			NXBasics::Runtime_PrintString(filename, "mapshots\\small_%4.4d.bmp", gInGameKeyboardHistory.smallMapCounter++);
			NC2E2::Tool_SaveWorldAsBMP(filename, 25);
			matched = true;
		}

		if (std::strncmp(history, "paminim", 7) == 0 && !NNetwork::Network_IsInSession())
		{
			NXBasics::CFile::FileSystem_Tool_CreateDirectory("mapshots");
			char filename[200];
			NXBasics::Runtime_PrintString(filename, "mapshots\\verybig_%4.4d.bmp", gInGameKeyboardHistory.largeMapCounter++);
			NC2E2::Tool_SaveWorldAsBMP(filename, 100);
			matched = true;
		}

		if (std::strncmp(history, "netsaklam", 9) == 0)
		{
			auto* world = manager->GetDesktop();
			auto variant = NC2Lifetime::Read<std::uint32_t>(world, NC2Layout::WorldGuiElementOffset::DarkenVariant) + 1u;

			if (variant == 4)
				variant = 0;

			NC2InGameGuiWorldElement::WorldDisplayElement_DarkenBitmap_Init(world, variant);
			matched = true;
		}

		if (matched)
		{
			NC2GuiToolsBase::FXTool_ClickConfirm();
			history[0] = 0;
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, manager->GetLocalPlayerId(), 9, 0);
		}
	}

	static bool SendElementMessage(void* element, std::uint32_t message, std::uint32_t command)
	{
		using TMessage = NXBaseGui::TGuiMessage;

		return NC2Lifetime::Read<TMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](element, message, command, 0, 0, 0);
	}

	static void CycleHumanJob(std::uint32_t job)
	{
		using namespace NC2Lifetime;
		using namespace NC2Logic;
		using namespace NC2InGameGuiInputManager;

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
		{

			return;
		}

		const auto capacity = CHumanArray::sTheObjectPtr->mCapacity;
		std::int32_t start = 0, stop = capacity - 1;

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 1)
		{
			const auto selected = Selection_GetSelectedSingleHumanId(gInputManager);

			if (Read<std::uint32_t>(GetHumanPtr(selected), NC2Layout::HumanOffset::JobType) == job)
			{
				start = selected + 1;

				if (start == capacity)
					start = 0;

				stop = selected;
			}
		}

		while (start != stop)
		{
			const auto* human = GetHumanPtr(start);

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) == static_cast<std::uint32_t>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId()) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == job)
			{
				Selection_SelectSingleHuman(gInputManager, start);
				break;
			}

			if (++start == capacity)
				start = 0;
		}
	}

	static void FilterSelectedHumans(bool heroes)
	{
		using namespace NC2Lifetime;
		using namespace NC2Logic;
		using namespace NC2InGameGuiInputManager;

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
		{

			return;
		}

		const auto matches = [=](std::int32_t id)
		{

			return heroes ? Human_IsJobHero(GetHumanPtr(id)) : Human_IsJobSoldier(GetHumanPtr(id));
		};

		while (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 2)
		{
			const auto count = Selection_GetSelectedMultipleHumanNumber(gInputManager);
			bool removed = false;

			for (std::uint32_t index = 0; index < count; ++index)
			{
				const auto id = Selection_GetSelectedMultipleHumanId(gInputManager, index);

				if (!matches(id))
				{
					UnSelect_Human(gInputManager, id);
					removed = true;
					break;
				}
			}

			if (!removed)
				break;
		}

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 1)
		{
			const auto id = Selection_GetSelectedSingleHumanId(gInputManager);

			if (!matches(id))
				UnSelect_Human(gInputManager, id);
		}
	}

	static void SelectOwnHumans(bool heroes)
	{
		using namespace NC2Logic;
		using namespace NC2InGameGuiInputManager;
		Selection_UnselectAll(gInputManager);
		InputMode_Cancel(gInputManager);
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId()); id != -1; id = iterator.GetNextHumanId())
		{
			const auto* human = GetHumanPtr(id);

			if (heroes ? Human_IsJobHero(human) : Human_IsJobSoldier(human) && !Human_IsRefreshingInWorld(human))
				Selection_SelectAddSingleHuman(gInputManager, id);
		}
	}

	// Original function: FUN_004bc36f; complete Windows keyboard dispatch, 004BC36F..004BD3C3.
	bool __fastcall InGameGuiManagerPrimaryMessageHandlerElement_MessageHandle(void*, void*, std::uint32_t message, std::uint32_t key, std::uintptr_t characterPayload, std::uintptr_t, std::uintptr_t)
	{
		const auto character = static_cast<std::uint32_t>(characterPayload);
		using namespace NC2Lifetime;
		using namespace NC2Logic;
		using namespace NC2InGameGuiInputManager;
		using namespace NC2GuiToolsBase;

		if (message != 0x200)
			return false;

		if (character < 128)
			InGameKeyboard_ProcessHiddenCommands(static_cast<std::uint8_t>(character));

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;

		if (manager->MiscWindows_HumanChangeNameWindow_SendKey(key, character))
			return true;

		if (character < 128 && manager->MiscWindows_ConstructionSelectionWindow_SendKey(key, static_cast<char>(character)))
			return true;

		if (gGameStaticVars.networkGame != 0 && NNetwork::Network_IsInSession())
		{
			auto* input = manager->GetStringInputHandler();
			const auto result = input->WorkOnKey(key, character);

			if (result == 3)
			{
				const auto* text = input->GetString();

				if (std::strlen(text) != 0)
				{
					char caption[100];
					NXBasics::Runtime_PrintString(caption, "%s> %s", NNetwork::Network_GetCallsign(), text);
					wchar_t record[128]{ };
					NNetwork::CopyNetworkUtf8ToWide(record + 1, caption, 127);
					record[0] = 1;

					if (gGameStaticVars.networkGame != 0 && NNetwork::Network_IsInSession())
						NNetwork::SendDynamicNetworkChatMessage(&gDynamicNetworkData, 0, record);

					FXTool_Chat();
				}

				input->SetString("");
			}

			if (input->mEditing != 0)
				manager->MiscWindows_StringOutputWindow_Open();

			if (result != 0 && result != 1)
				return true;
		}

		if (character >= 128)
			return true;

		if (manager->Selection_CheckKey_Action(key, static_cast<char>(character)))
			return true;

		const auto* assignments = NC2InGameGuiKeyAssignmentManager::CKeyAssignmentManager::sTheObjectPtr;
		const auto check = [=](std::int32_t assignment)
		{

			return assignments->CheckKey(assignment, key, static_cast<std::uint8_t>(character));
		};
		const auto confirm = []()
		{
			FXTool_ClickConfirm();

			return true;
		};

		if (check(1))
		{
			if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::InputMode) != 0)
			{
				InputMode_Cancel(gInputManager);
				FXTool_ClickConfirm();
				return true;
			}

			auto* window = desktop->Window_GetActive();

			if (window != nullptr && !SendElementMessage(window, 0x1000, 2000) && !SendElementMessage(window, 0x1000, 2001))
			{
				desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
				return confirm();
			}

			if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) != 0)
			{
				Selection_UnselectAll(gInputManager);
				return confirm();
			}
		}

		if (check(2))
		{
			manager->MiscWindows_HelpWindow_Open();
			return confirm();
		}

		if (check(3))
		{
			manager->MiscWindows_OptionsWindow_Open(false, false);
			return confirm();
		}

		if (check(13))
		{
			manager->MiscWindows_OptionsWindow_Open(true, false);
			return confirm();
		}

		if (check(14))
		{
			manager->MiscWindows_OptionsWindow_Open(false, true);
			return confirm();
		}

		if (check(4))
		{
			manager->MiscWindows_DiplomacyWindow_Open();
			return confirm();
		}

		if (check(5))
		{
			manager->MiscWindows_StatisticsWindow_Open();
			return confirm();
		}

		if (check(6))
		{
			manager->MiscWindows_HumanListWindow_Open(0, 0);
			return confirm();
		}

		if (check(7))
		{
			manager->MiscWindows_TechTreeWindow_Open();
			return confirm();
		}

		if (check(8))
		{
			manager->MiscWindows_GlobalGameSettingsWindow_Open(true, false);
			return confirm();
		}

		if (check(9))
		{
			manager->MiscWindows_GlobalGameSettingsWindow_Open(false, true);
			return confirm();
		}

		if (check(10))
		{
			manager->MiscWindows_OverviewWindow_Toggle();
			return confirm();
		}

		if (check(11))
		{
			manager->ObservationWindow_CreateNewOne();
			return confirm();
		}

		if (check(37))
		{
			manager->Selection_ActionButtons_BringUp();
			return confirm();
		}

		if (check(16))
		{
			manager->MiscWindows_ConstructionSelectionWindow_Open(0, 0);
			return confirm();
		}

		if (check(17))
		{
			InputMode_Set(gInputManager, 14);
			return confirm();
		}

		if (check(18))
		{
			InputMode_Set(gInputManager, 16);
			return confirm();
		}

		if (check(19))
		{
			InputMode_Set(gInputManager, 18);
			return confirm();
		}

		const auto queue = [](std::byte* packet)
		{
			UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, packet, false, true);
		};

		if (check(20))
		{
			std::byte packet[20]{ };
			packet[0] = std::byte{ 0x86 };
			packet[1] = static_cast<std::byte>(manager->GetLocalPlayerId());
			Write<std::uint32_t>(packet, NC2Layout::GameControlCommandOffset::Value, 1);
			queue(packet);
			return confirm();
		}

		if (check(21))
		{
			std::byte packet[20]{ };
			packet[0] = std::byte{ 0x86 };
			packet[1] = static_cast<std::byte>(manager->GetLocalPlayerId());
			queue(packet);
			return confirm();
		}

		if (check(35))
		{
			desktop->Message_SendToAllElements(2007, 0, 0, 0);
			return confirm();
		}

		if (check(36))
		{
			CHouseIterator iterator;

			for (auto id = iterator.GetFirstHouseId(manager->GetLocalPlayerId()); id != -1; id = iterator.GetNextHouseId())
			{
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
				const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

				if (type == 7 || type == 8 || type == 9 || type == 1)
				{
					NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(manager->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), reinterpret_cast<const std::uint32_t*>(house + NC2Layout::HouseOffset::MapPosition));
					return confirm();
				}
			}

			return true;
		}

		if (check(22))
		{
			std::byte packet[20]{ };
			packet[0] = std::byte{ 0x87 };
			Write<std::uint32_t>(packet, NC2Layout::GameSpeedCommandOffset::Speed, gGameTime.speed < 0.01 ? static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(gGameTime.retainedSpeed)) : 0);
			queue(packet);
			return confirm();
		}

		if (check(23))
		{
			const auto speed = Logic_EntryPoint_GameTime_GetSpeed();
			std::byte packet[20]{ };
			packet[0] = std::byte{ 0x87 };
			Write<std::uint32_t>(packet, NC2Layout::GameSpeedCommandOffset::Speed, speed < 12.0 ? 12 : speed < 24.0 ? 24 : speed < 36.0   ? 36 : 12);
			queue(packet);
			return confirm();
		}

		if (check(15))
		{
			GameIO_Savegame_Save("user_quicksave", nullptr, 0);
			manager->MiscWindows_StringOutputWindow_PrintString(StringTool_GetMiscMenuStringPtr(80));
			return confirm();
		}

		if (check(29))
			CycleHumanJob(6);

		if (check(30))
			CycleHumanJob(27);

		if (check(31))
			FilterSelectedHumans(false);

		if (check(32))
			FilterSelectedHumans(true);

		if (check(33))
			SelectOwnHumans(true);

		if (check(34))
			SelectOwnHumans(false);

		auto* groups = static_cast<NC2InGameGuiGroups::CObjectGroupManager*>(NC2InGameGuiGroups::gObjectGroupManager);

		for (std::uint32_t index = 0; index < 10; ++index)
		{
			if (check(41 + index))
				groups->Group_Choose(index);
		}

		for (std::uint32_t index = 0; index < 10; ++index)
		{
			if (check(51 + index))
				groups->Group_CreateNewGroup(index);
		}

		for (std::uint32_t index = 0; index < 10; ++index)
		{
			if (check(61 + index))
				groups->Group_AddSelectionToGroup(index);
		}

		if (key == 0 || (Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) == 0)
			return false;

		if (key == 'S')
		{
			const auto start = timeGetTime();
			GameIO_Savegame_Save("user_quicksave", nullptr, 0);
			const auto duration = timeGetTime() - start;
			manager->MiscWindows_StringOutputWindow_PrintStringV(StringTool_GetMiscMenuStringPtr(81), static_cast<double>(duration) * static_cast<double>(0.001f));
			return confirm();
		}

		if (key == 'B')
		{
			NXBasics::CFile::FileSystem_Tool_CreateDirectory("screenshots");
			char filename[200];
			NXBasics::Runtime_PrintString(filename, "screenshots\\c2_%4.4d.bmp", gInGameKeyboardHistory.screenshotCounter++);
			auto* source = desktop->GetDisplayBitmap();
			auto area = source->GetRectangle();
			auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();

			if (presentation->IsHudBitmap(source))
			{
				area.width = presentation->GetWidth();
				area.height = presentation->GetHeight();
			}

			{
				NXBasics::CBitmap bitmap(area.width, area.height, 2);

				if (presentation->IsHudBitmap(source))
				{
					presentation->CopyCompositedFrame(&bitmap);
				}
				else
				{
					source->CopyIntoBitmap(&bitmap, 0, 0);
				}

				NXBasics::CBitmapFileBMP writer;
				writer.Open(area.width, area.height, filename);
				writer.WriteBitmap(&bitmap);
				writer.Close();
				writer.Close();
			}
			manager->MiscWindows_StringOutputWindow_PrintStringV(StringTool_GetMiscMenuStringPtr(84));
			return confirm();
		}

		return false;
	}

	void* __fastcall InGameGuiManagerPrimaryMessageHandlerElement_Delete(void* element, void*, std::uint32_t flags);
	// Complete Windows table region 004F7038..004F707F; inherited folded entries retain their base semantics.
	const std::array<std::uintptr_t, 18> gInGamePrimaryMessageHandlerVirtualTable = []
	{
		std::array<std::uintptr_t, 18> table{ };
		std::memcpy(table.data(), gInGameDefaultMessageHandlerVirtualTable, sizeof(table));
		table[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&InGameGuiManagerPrimaryMessageHandlerElement_Delete>());
		table[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&InGameGuiManagerPrimaryMessageHandlerElement_MessageHandle>());

		return table;
	}();

	// Original function: FUN_004bc34d.
	void* __fastcall InGameGuiManagerPrimaryMessageHandlerElement_Delete(void* element, void*, std::uint32_t flags)
	{
		NC2Lifetime::Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gInGamePrimaryMessageHandlerVirtualTable.data());
		NXBaseGui::BaseElement_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}
}

// Original function: FUN_004bc31c.
void* __fastcall InGameGuiManagerPrimaryMessageHandlerElement_Construct(void* element)
{
	const NXBasics::SRectangle rectangle(0, 0, 0, 0);
	NXBaseGui::BaseElement_Construct(element, &rectangle, 4, 0xA01);
	NC2Lifetime::Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gInGamePrimaryMessageHandlerVirtualTable.data());

	return element;
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a1b29; complete command/assignment table 00507350..00507377.
	bool CInGameGuiManager::Selection_CheckKey_Action(std::uint32_t key, char character)
	{
		using namespace NC2InGameGuiInputManager;
		using namespace NC2Logic;

		if (NC2Lifetime::Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) != 1)
			return false;

		SUserCommandsForHumanHandler commands;

		if (!Logic_UserControl_GetHumanCommands(Selection_GetSelectedSingleHumanId(gInputManager), &commands, false, false))
			return false;

		constexpr std::uint32_t mapping[5][2] = { { 24, 38 }, { 16, 39 }, { 20, 39 }, { 10, 40 }, { 0, 0 } };

		for (const auto& entry : mapping)
		{
			if (entry[0] == 0)
				break;

			if (commands.IsCommandAllowed(entry[0]) && NC2InGameGuiKeyAssignmentManager::CKeyAssignmentManager::sTheObjectPtr->CheckKey(entry[1], key, static_cast<std::uint8_t>(character)))
			{
				using TMessage = NXBaseGui::TGuiMessage;
				auto* handler = mDisplayObject;
				NC2Lifetime::Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](handler, 0x800, entry[0] + 3000u, 0, 0, 0);
				return true;
			}
		}

		return false;
	}
}

// Vehicle branch of FUN_004a3edb; the single and group paths retain distinct native action sets.
static void DefaultHandler_VehicleCommand(std::uint32_t action)
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiInputManager;
	auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
	std::int32_t mode = -1;

	switch (action)
	{
		case SwitchIds::VehicleGuiAction::SetWorkCenter:
			mode = 20;
			break;
		case SwitchIds::VehicleGuiAction::AttackHuman:
			mode = 24;
			break;
		case SwitchIds::VehicleGuiAction::AttackHouse:
			mode = 11;
			break;
		case SwitchIds::VehicleGuiAction::AttackVehicle:
			mode = 28;
			break;
		case SwitchIds::VehicleGuiAction::AttackMapPosition:
			mode = 21;
			break;
	}

	if (mode != -1)
	{
		InputMode_Set(gInputManager, mode);
		manager->l_Selection_RemoveAllGuiElements();

		return;
	}

	const auto queue = [&](std::int32_t id, bool single)
	{
		std::uint8_t opcode = 0;

		switch (action)
		{
			case SwitchIds::VehicleGuiAction::GotoAndWait:
				opcode = 0x64;
				break;
			case SwitchIds::VehicleGuiAction::Stop:
				opcode = 0x66;
				break;
			case SwitchIds::VehicleGuiAction::MilitaryAggressive:
			case SwitchIds::VehicleGuiAction::MilitaryDefensive:
			case SwitchIds::VehicleGuiAction::MilitaryIgnore:
				opcode = 0x6F;
				break;
			case SwitchIds::VehicleGuiAction::DetachVehicle:
				if (single)
					opcode = 0x72;

				break;
			case SwitchIds::VehicleGuiAction::ResetWantedAmounts:
				if (single)
					opcode = 0x6E;

				break;
		}

		if (opcode == 0)
		{

			return;
		}

		std::byte packet[20]{ };
		Write(packet, NC2Layout::UserNetworkCommandOffset::Opcode, opcode);
		Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, id);

		if (opcode == 0x6F)
			Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter1, action - 8);

		NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, packet);
		manager->l_Selection_RemoveAllGuiElements();
	};

	if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 4)
	{
		const auto id = Selection_GetSelectedSingleVehicleId(gInputManager);

		if (action == 4)
		{
			InputMode_Set(gInputManager, 19);
			manager->l_Selection_RemoveAllGuiElements();
		}
		else if (action == 12)
		{
			InputMode_Set(gInputManager, 27);
		}
		else
		{
			queue(id, true);
		}
	}

	if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 5)
	{
		const auto count = Selection_GetSelectedMultipleVehiclesNumber(gInputManager);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			queue(Selection_GetSelectedMultipleVehicleId(gInputManager, index), false);
		}
	}

	manager->l_Selection_RemoveAllGuiElements();
}

// House branch of FUN_004a3edb; complete native command table 004A479E..004A47D9.
static void DefaultHandler_HouseCommand(std::uint32_t command)
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiInputManager;

	if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) != 6)
		return;

	const auto id = Selection_GetSelectedHouseId(gInputManager);
	const auto index = command - 3301u;

	if (index > 14)
	{

		return;
	}

	auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

	if (index == 0)
	{
		manager->MiscWindows_HouseRemoveWindow_Open();
	}
	else
	{
		constexpr std::uint8_t opcode[14] = { 0x5B, 0x5C, 0x5D, 0x5D, 0x5D, 0x5D, 0x5D, 0x5D, 0x5E, 0x5E, 0x5F, 0x5F, 0x61, 0x61 };
		constexpr std::uint32_t first[14] = { 0, 0, 0, 0, 1, 1, 2, 2, 1, 0, 1, 0, 1, 0 };
		constexpr std::uint32_t second[14] = { 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0 };
		std::byte packet[20]{ };
		Write(packet, NC2Layout::UserNetworkCommandOffset::Opcode, opcode[index - 1]);
		Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
		Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter1, first[index - 1]);
		Write(packet, NC2Layout::UserNetworkCommandOffset::Parameter2, second[index - 1]);
		NC2Logic::UserNetworkCommand_AddHouseCommand(NC2Logic::gUserNetworkCommandManager, packet);
	}

	manager->l_Selection_RemoveAllGuiElements();
}

// Original function: FUN_004a3edb; complete native command dispatch.
// Naming reference: NC2InGameGuiManager::CGuiManagerDefaultMessageHandlerElement::XGui_BE_Message_Handle.
bool __fastcall InGameGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
{
	if (message == 0x800)
	{
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (command >= 3000 && command < 3299)
			return NC2InGameGuiManager::HumanActionButtons_HandleCommand(command - 3000);

		if (command >= 3600 && command < 3899)
		{
			DefaultHandler_VehicleCommand(command - 3600);
			return true;
		}

		if (command >= 3300 && command < 3599)
		{
			DefaultHandler_HouseCommand(command);
			return true;
		}

		if (command == 0xF3C)
		{
			manager->MiscWindows_OptionsWindow_Toggle(false, false);
		}
		else if (command == 0xF3D)
		{
			manager->MiscWindows_GlobalGameSettingsWindow_Toggle(false, false);
		}
		else if (command == 0xF3E)
		{
			manager->MiscWindows_ConstructionSelectionWindow_Toggle(0, 0);
		}
		else if (command == 0xF3F)
		{
			manager->MiscWindows_DiplomacyWindow_Toggle();
		}
		else if (command == 0xF40)
		{
			manager->MiscWindows_HumanListWindow_Toggle(0, 0);
		}
		else if (command == 0xF41)
		{
			manager->MiscWindows_StatisticsWindow_Toggle();
		}
		else if (command == 0xF42)
		{
			manager->MiscWindows_MissionInfoWindow_Toggle(NXBasics::IntegerRange::UInt32Maximum, true);
		}
		else if (command == 0xF43)
		{
			manager->MiscWindows_TechTreeWindow_Toggle();
		}
		else if (command == 0xF44)
		{
			manager->MiscWindows_HelpWindow_Toggle();
		}
		else if (command == 0xF45)
		{
			manager->MiscWindows_OverviewWindow_Open();
		}
		else if (command == 0xF46)
		{
			const double speed = NC2Logic::Logic_EntryPoint_GameTime_GetSpeed();
			std::uint32_t nextSpeed = 12;

			if (speed >= 12.0 && speed < 24.0)
			{
				nextSpeed = 24;
			}
			else if (speed >= 24.0 && speed < 36.0)
			{
				nextSpeed = 36;
			}

			std::byte networkCommand[20]{ };
			networkCommand[0] = std::byte{ 0x87 };
			std::memcpy(networkCommand + 4, &nextSpeed, sizeof(nextSpeed));
			NC2Logic::UserNetworkCommandManager_AddCommand(NC2Logic::gUserNetworkCommandManager, nullptr, networkCommand, false, true);
		}
		else if (command == 0xF47)
		{
			static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->MessagePriority_Toggle();
			manager->MiscButtons_MessagePriorityButton_Update(true);
		}
		else if (command == 0xF48)
		{
			manager->MiscWindows_NetworkInfoWindow_Toggle();
		}
	}

	return true;
}