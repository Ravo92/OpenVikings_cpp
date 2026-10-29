#include "XPalette.h"
#include "C2CallbackManager.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineObjectData.h"
#include "C2HumanEquipment.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "C2StringAccess.h"
#include "C2VehicleAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiClockToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHumanListWindow.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiWindowState.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"
#include "XMemoryChecksum.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using namespace NC2Logic;
	using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	static const void* Human(std::uint32_t id)
	{

		return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
	}

	static const char* Name(std::uint32_t id)
	{

		return reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries + id * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::Name);
	}

	static char* Text(std::uint32_t id)
	{

		return StringTool_GetGameGuiHumanListWindowStringPtr(id);
	}

	static bool Filter(std::uint32_t index)
	{

		return Read<std::uint8_t>(gHumanListWindowStatics, index) != 0;
	}

	static std::uint32_t Modifiers()
	{

		return Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags);
	}

	static void* __fastcall HumanListWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall HumanListWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall HumanListWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __cdecl HumanListWindow_ls_Callback_Logic(std::uint32_t type, std::uintptr_t userData, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	static void* __fastcall HumanListJobSelectionWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall HumanListJobSelectionWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void HumanListJobSelectionWindow_l_GuiButtons_Add(void* window);
	static bool HumanListJobSelectionWindow_l_IsJobToChoose(void* window, std::uint32_t tribe, std::uint32_t job, std::uint32_t player);
	static bool HumanListButton_l_ValidateHuman(void* element);
	static bool __fastcall HumanListButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall HumanListButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static void __fastcall HumanListButton_DoPressedAction(void* element);
	// Complete Windows tables 004F6B50..004F6BE3 and 004F6C2C..004F6C77.
	static const std::array<std::uintptr_t, 19> gHumanListWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&HumanListWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&HumanListWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&HumanListWindow_Draw);

		return t;
	}();
	static const std::array<std::uintptr_t, 18> gHumanListButtonVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), gBaseToolElementVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(&HumanListButton_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&HumanListButton_Draw);
		t[17] = reinterpret_cast<std::uintptr_t>(&HumanListButton_DoPressedAction);

		return t;
	}();
	static const std::array<std::uintptr_t, 19> gHumanListJobSelectionVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&HumanListJobSelectionWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&HumanListJobSelectionWindow_Message);

		return t;
	}();

	// Original function: FUN_004affa9; Windows CHumanListWindow allocation 0x1018.
	void* HumanListWindow_Construct(void* window, std::uint32_t job, std::uint32_t possibleJob)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::HumanListWindowOffset::HumanCount, 0, NC2Layout::RecordSize::HumanListWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHumanListWindowVirtualTable.data());
		CCallbackManager::sTheObjectPtr->RegisterCallback(1, HumanListWindow_ls_Callback_Logic, reinterpret_cast<std::uintptr_t>(window), 10);

		if (job != 0)
		{
			HumanListWindow_l_Filter_Reset();
			Write(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::JobFilter, job);
			HumanListWindow_l_UpdateHumanListElements(window, true);
		}

		if (possibleJob != 0)
		{
			HumanListWindow_l_Filter_Reset();
			Write(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter, possibleJob);
			HumanListWindow_l_UpdateHumanListElements(window, true);
		}

		HumanListWindow_l_UpdateHumanDataArray(window);
		HumanListWindow_l_GuiButtons_Add(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Stop_LargeWindowOpened();

		return window;
	}

	// Original function: FUN_004b008e.
	static void HumanListWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHumanListWindowVirtualTable.data());
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Continue_LargeWindowClosed();
		auto* subwindow = Read<NXBaseGui::CBaseWindow*>(window, NC2Layout::HumanListWindowOffset::JobSelectionWindow);

		if (subwindow != nullptr)
			NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(subwindow);

		CCallbackManager::sTheObjectPtr->UnRegisterCallback(1, HumanListWindow_ls_Callback_Logic, reinterpret_cast<std::uintptr_t>(window));
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004b0072.
	static void* __fastcall HumanListWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		HumanListWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004b0155.
	static bool __fastcall HumanListWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t first, std::uint32_t data, std::uint32_t revision)
	{
		if (message == 0x1000 && command == 0x7E4)
		{
			// Both native query branches store the possible-job field, including the first parameter.

			if (first != 0)
			{
				HumanListWindow_l_Filter_Reset();
				Write(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter, first);
				HumanListWindow_l_UpdateHumanListElements(window, true);
			}

			if (data != 0)
			{
				HumanListWindow_l_Filter_Reset();
				Write(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter, data);
				HumanListWindow_l_UpdateHumanListElements(window, true);
			}

			return true;
		}

		if (message != 0x800)
			return false;

		auto* input = NC2InGameGuiInputManager::gInputManager;

		if (command == 10000 || command == 10001)
		{
			const auto* human = Human(data);

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != revision)
				return true;

			if (Modifiers() & 1u)
			{
				NC2InGameGuiInputManager::Selection_SelectAddSingleHuman(input, data);
				return true;
			}

			if (Modifiers() & 2u)
			{
				NC2InGameGuiInputManager::UnSelect_Human(input, data);
			}
			else
			{
				NC2InGameGuiInputManager::Selection_SelectSingleHuman(input, data);
			}

			if ((Modifiers() & 2u) == 0)
				NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetToMapPosition(Human_GetMapPositionRef(human));

			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));

			if (command == 10001)
				NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();
		}
		else if (command >= 10002 && command <= 10014)
		{
			constexpr std::int32_t filters[13]{ -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13 };
			HumanListWindow_l_Filter_Reset();
			const auto index = filters[command - 10002];

			if (index >= 0)
				Write<std::uint8_t>(gHumanListWindowStatics, index, 1);

			HumanListWindow_l_UpdateHumanListElements(window, true);
		}
		else if (command == 10015 || command == 10016)
		{
			HumanListWindow_l_JobSelectionWindow_Open(window, command == 10015);
		}
		else if (command == 10017)
		{
			HumanListWindow_l_UpdateHumanDataArray(window);

			if ((Modifiers() & 3u) == 0)
				NC2InGameGuiInputManager::Selection_UnselectAll(input);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanCount); ++index)
			{
				const auto id = Read<std::int32_t>(window, NC2Layout::HumanListWindowOffset::HumanReferencesUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned);

				if ((Modifiers() & 1u) != 0 || (Modifiers() & 2u) == 0)
				{
					NC2InGameGuiInputManager::Selection_SelectAddSingleHuman(input, id);
				}
				else
				{
					NC2InGameGuiInputManager::UnSelect_Human(input, id);
				}
			}
		}

		return true;
	}

	// Original function: FUN_004b0426.
	static void __fastcall HumanListWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(static_cast<const std::byte*>(static_cast<const void*>(resources.fontWhitePalette)) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		bitmap->Text_PrintV(resources.font08, color, 8, 72, "%s:", Text(1));
		bitmap->Text_PrintV(resources.font08, color, 8, 398, Text(2), Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanCount));
	}

	// Original function: FUN_004b0fd3; heroes sort first, then native strcmp on engine names.
	static bool HumanListWindow_ls_HumanSortFunction(std::uint32_t first, std::uint32_t second)
	{
		if (!Human_IsJobHero(Human(first)) && Human_IsJobHero(Human(second)))
			return true;

		if (Human_IsJobHero(Human(first)) && !Human_IsJobHero(Human(second)))
			return false;

		return std::strcmp(Name(first), Name(second)) > 0;
	}

	// Original function: FUN_004b0ba9; preserve the native 500-entry cap, filter order and bubble sort.
	bool HumanListWindow_l_UpdateHumanDataArray(void* window)
	{
		std::uint32_t ids[500], count = 0;
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId()); id != -1; id = iterator.GetNextHumanId())
		{
			const auto* human = Human(id);

			if (Filter(0) && (!Human_IsSexMale(human) || !Human_IsAgeAdult(human)))
				continue;

			if (Filter(1) && (!Human_IsSexFemale(human) || !Human_IsAgeAdult(human) || Human_IsJobHero(human)))
				continue;

			if (Filter(2) && Human_IsAgeAdult(human))
				continue;

			if (Filter(3) && !Human_IsJobSoldier(human))
				continue;

			if (Filter(4) && !Human_IsJobHero(human))
				continue;

			if (Filter(5) && (!Human_IsSexMale(human) || !Human_IsAgeAdult(human) || Human_IsJobSoldier(human) || Human_IsJobHero(human) || Human_UserControl_GetNextJobType(human) == 6))
				continue;

			if (Filter(6) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) != NXBasics::IntegerRange::UInt32Maximum || Human_IsJobSoldier(human) || Human_IsJobHero(human)))
				continue;

			if (Filter(7))
			{
				if (!Human_IsSexMale(human) || !Human_IsAgeAdult(human) || Human_IsJobSoldier(human) || Human_IsJobHero(human))
					continue;

				const auto good = Human_GetEquipmentType_Tool(human);
				const auto percentage = good == 31 || good == 32 ? NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) * 100u) / 10000 : 0;

				if (percentage > 0)
					continue;
			}

			if (Filter(8) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId) != NXBasics::IntegerRange::UInt32Maximum || Human_IsJobSoldier(human) || Human_IsJobHero(human) || !Human_IsAgeAdult(human)))
				continue;

			if (Filter(9) || Filter(10))
				continue;

			if (Filter(11) && (!Human_IsAgeAdult(human) || !Human_IsSexFemale(human) || Human_IsJobHero(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId) != NXBasics::IntegerRange::UInt32Maximum))
				continue;

			if (Filter(12) && (!Human_IsAgeAdult(human) || Human_IsJobHero(human) || Human_HasEquippedShoes(human)))
				continue;

			if (Filter(13) && (!Human_IsAgeAdult(human) || !Human_IsJobSoldier(human) || Human_IsJobHero(human) || Read<std::uint32_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)], Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon) * NC2Layout::RecordSize::StaticWeaponTypeUnsigned + NC2Layout::StaticWeaponTypeOffset::GoodType) != 0))
				continue;

			const auto job = Read<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::JobFilter), possible = Read<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter);

			if (job != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != job)
				continue;

			if (possible != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != possible && (!Human_UserControl_CanChangeJob(human) || !Human_IsAllowedToDoJob(human, possible) || !Human_IsAbleToDoJob(human, possible)))
				continue;

			ids[count++] = id;

			if (count == 500)
				break;
		}

		if (count > 1)
			for (auto end = static_cast<std::int32_t>(count) - 1; end >= 0; --end)
			{
				for (std::int32_t index = 0; index < end; ++index)
				{
					if (HumanListWindow_ls_HumanSortFunction(ids[index], ids[index + 1]))
					{
						const auto old = ids[index];
						ids[index] = ids[index + 1];
						ids[index + 1] = old;
					}
				}
			}

		bool same = Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanCount) == count;

		if (same)
			for (std::uint32_t index = 0; index < count; ++index)
			{
				if (Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanReferencesUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned) != ids[index] || Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::FirstHumanUniqueIdUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned) != Read<std::uint32_t>(Human(ids[index]), NC2Layout::HumanOffset::UniqueId))
				{
					same = false;
					break;
				}
			}

		if (same)
			return false;

		Write(window, NC2Layout::HumanListWindowOffset::HumanCount, count);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			Write(window, NC2Layout::HumanListWindowOffset::HumanReferencesUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned, ids[index]);
			Write(window, NC2Layout::HumanListWindowOffset::FirstHumanUniqueIdUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned, Read<std::uint32_t>(Human(ids[index]), NC2Layout::HumanOffset::UniqueId));
		}

		return true;
	}

	// Original function: FUN_004b106f.
	void HumanListWindow_l_UpdateHumanListElements(void* window, bool force)
	{
		if (HumanListWindow_l_UpdateHumanDataArray(window) || force)
		{
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			HumanListWindow_l_GuiButtons_Add(window);
		}
	}

	// Original function: FUN_004b111f.
	static bool __cdecl HumanListWindow_ls_Callback_Logic(std::uint32_t type, std::uintptr_t userData, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (type == 1)
			HumanListWindow_l_UpdateHumanListElements(reinterpret_cast<void*>(userData), false);

		return true;
	}

	// Original function: FUN_004b1134; CHumanListButton stores its allocation revision.
	static void* HumanListButton_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t human, std::uint32_t pressed, std::uint32_t doubleClicked, void* handler, bool debug)
	{
		BaseToolElement_Construct(element, rectangle, 3, 0x120);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::HumanListButtonOffset::HumanId, 0, NC2Layout::RecordSize::HumanListButtonVars);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gHumanListButtonVirtualTable.data());
		Write(element, NC2Layout::HumanListButtonOffset::HumanId, human);
		Write(element, NC2Layout::HumanListButtonOffset::HumanUniqueId, Read<std::uint32_t>(Human(human), NC2Layout::HumanOffset::UniqueId));
		Write(element, NC2Layout::HumanListButtonOffset::PressedCommand, pressed);
		Write(element, NC2Layout::HumanListButtonOffset::DoubleClickCommand, doubleClicked);
		Write(element, NC2Layout::HumanListButtonOffset::Handler, handler);
		Write<std::uint8_t>(element, NC2Layout::HumanListButtonOffset::DebugFlag, debug);

		return element;
	}

	// Original function: FUN_004b1729.
	static bool HumanListButton_l_ValidateHuman(void* element)
	{
		const auto id = Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* human = Human(id);

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanUniqueId))
			return true;

		Write<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanUniqueId, 0);

		return false;
	}

	// Original function: FUN_004b1691; 0x100 messages are forwarded to the parent list.
	static bool __fastcall HumanListButton_Message(void* element, void*, std::uint32_t message, std::uint32_t command, std::uint32_t first, std::uint32_t second, std::uint32_t third)
	{
		if (message == 0x100)
		{
			auto* parent = Read<void*>(element, NC2Layout::GuiBaseElementOffset::MessageHandler);
			return parent == nullptr ? true : Read<TMessage*>(parent, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](parent, message, command, first, second, third);
		}

		if (message != 0x20)
			return false;

		auto* handler = Read<void*>(element, NC2Layout::HumanListButtonOffset::Handler);

		if (handler != nullptr)
		{
			HumanListButton_l_ValidateHuman(element);
			const auto id = Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanId);

			if (id != NXBasics::IntegerRange::UInt32Maximum)
				Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](handler, 0x800, Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::DoubleClickCommand), reinterpret_cast<std::uint32_t>(element), id, Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanUniqueId));
		}

		return true;
	}

	// Original function: FUN_004b16f7; native validation precedes the pressed command.
	static void __fastcall HumanListButton_DoPressedAction(void* element)
	{
		auto* handler = Read<void*>(element, NC2Layout::HumanListButtonOffset::Handler);

		if (handler == nullptr)
			return;

		HumanListButton_l_ValidateHuman(element);
		const auto id = Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		Read<TMessage*>(handler, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](handler, 0x800, Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::PressedCommand), reinterpret_cast<std::uint32_t>(element), id, Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanUniqueId));
		FXTool_ClickConfirm();
	}

	// Original function: FUN_004b1198; row details are rebuilt from live native object data.
	static void __fastcall HumanListButton_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		const auto mode = (Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8) != 0 ? 4 : 2;
		DrawTool_FillArea(element, bitmap, rectangle, mode);
		DrawTool_DrawFrame(element, bitmap, rectangle, mode);

		if (!HumanListButton_l_ValidateHuman(element))
			return;

		const auto id = Read<std::uint32_t>(element, NC2Layout::HumanListButtonOffset::HumanId);
		const auto* human = Human(id);
		NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), bitmap, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - 30, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 4, nullptr, true, -1);
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(static_cast<const std::byte*>(static_cast<const void*>(resources.fontWhitePalette)) + NXBasics::PaletteOffset::Colors + 255u * 4u);
		bitmap->Text_Print(resources.font10, color, 4, 4, Name(id));
		char caption[200]{ };

		if (Human_IsJobHero(human))
		{
			std::strcat(caption, CGuiBaseStringsManager::GetStringPtr(7, 120));
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 47)
		{
			std::strcat(caption, CGuiBaseStringsManager::GetStringPtr(12, 31));
		}
		else
		{
			std::strcat(caption, StringAccess_GetJobNamePtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 1));
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != 0)
		{
			std::strcat(caption, " (");
			std::strcat(caption, NC2Logic::Human_GetReallyProducedGoodType_GetGuiNamePtr(human));
			std::strcat(caption, ")");
		}

		if (Human_IsAgeAdult(human) && Human_IsSexMale(human) && !Human_IsJobHero(human) && !(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32))
		{
			char detail[100];

			if (Human_IsJobSoldier(human))
			{
				NXBasics::Runtime_PrintString(detail, " - %s: %d%%", CGuiBaseStringsManager::GetStringPtr(7, 83), Human_Tool_Gui_GetExperienceForEquippedWeapon(human));
			}
			else
			{
				NXBasics::Runtime_PrintString(detail, " - %s: %d%%", Text(40), Human_JobEfficiency_CalculateExperienceFactor(human));
			}

			std::strcat(caption, detail);
		}

		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		// Native IsJobWoman (0042C5AA) tests job 5 alone.

		if (job == 5 && !Human_IsMarried(human))
		{
			std::strcat(caption, " - ");
			std::strcat(caption, Text(41));
		}

		if (job == 5 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId) != NXBasics::IntegerRange::UInt32Maximum)
		{
			std::strcat(caption, " - ");
			std::strcat(caption, Text(42));
			std::strcat(caption, ": ");
			std::strcat(caption, Name(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)));
		}

		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;

			if (house != nullptr)
			{
				std::strcat(caption, " - ");
				std::strcat(caption, StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1));
			}
		}

		if (!Human_IsJobSoldier(human) && !Human_IsAttachedToHomeHouse(human) && !Human_IsJobHero(human))
		{
			std::strcat(caption, " - ");
			std::strcat(caption, Text(43));
		}

		bitmap->Text_Print(resources.font08, color, 10, 18, caption);

		if (Read<std::uint8_t>(element, NC2Layout::HumanListButtonOffset::DebugFlag) != 0)
		{
			char debug[100];
			NXBasics::Runtime_PrintString(debug, "#%d - %d - %8.8x", Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), NXBasics::XB_GetMemoryChecksum(human, 0xC58));
			bitmap->Text_Print(resources.font08, color, 4, Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 18, debug);
			return;
		}

		if (Human_IsJobHero(human))
		{

			return;
		}

		caption[0] = '\0';
		const auto appendGood = [&](std::uint32_t good)
		{
			if (good != 0)
			{
				char fragment[100];
				NXBasics::Runtime_PrintString(fragment, "{gfxgood:%d,1}", good);
				std::strcat(caption, fragment);
			}
		};

		if (Human_IsJobSoldier(human))
		{
			appendGood(Human_GetEquipmentType_Weapon(human));
			appendGood(Human_GetEquipmentType_Armor(human));
		}

		if (Human_IsAgeAdult(human) && Human_IsSexMale(human))
		{
			appendGood(Human_GetEquipmentType_Shoes(human));
			appendGood(Human_GetEquipmentType_Tool(human));
		}

		for (std::uint32_t index = 0; index < 4; ++index)
		{
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::InventoryConditions + index * NC2Layout::RecordSize::HumanInventorySlot) > 0)
				appendGood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventorySlots + index * NC2Layout::RecordSize::HumanInventorySlot));
		}

		DrawTool_PrintStringX(bitmap, 10, 32, resources.font08, color, caption);
	}

	// Original function: FUN_004b0490; controls follow the native list/filter order.
	void HumanListWindow_l_GuiButtons_Add(void* window)
	{
		const NXBasics::SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14), close(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &title, Text(0)));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &close));
		const auto textButton = [&](const NXBasics::SRectangle& area, const char* caption, const char* tooltip, std::uint32_t command, bool toggle, bool selected)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			void* element = memory == nullptr ? nullptr : toggle ? BaseToolTextButton_ConstructToggleWithToolTip(memory, nullptr, &area, caption, tooltip, command, window, selected)
			                                                     : BaseToolTextButton_ConstructWithToolTip(memory, nullptr, &area, caption, tooltip, command, window);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		};
		textButton(NXBasics::SRectangle(4, 30, 132, 18), Text(10), Text(11), 10002, false, false);
		const auto job = Read<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::JobFilter), possible = Read<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter);
		textButton(NXBasics::SRectangle(138, 30, 162, 18), job == 0 ? Text(12) : StringAccess_GetJobNamePtr(job, 1), Text(13), 10015, true, job != 0);
		textButton(NXBasics::SRectangle(302, 30, 162, 18), possible == 0 ? Text(14) : StringAccess_GetJobNamePtr(possible, 1), Text(15), 10016, true, possible != 0);
		const auto graphicButton = [&](const NXBasics::SRectangle& area, std::uint32_t graphic, const char* tooltip, std::uint32_t command, std::uint32_t flag)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
			void* element = memory == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(memory, nullptr, &area, graphic, tooltip, command, window);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Selected, Filter(flag));
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::RectangularStyle, 1);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		};
		graphicButton(NXBasics::SRectangle(4, 50, 80, 18), 168, Text(16), 10003, 0);
		graphicButton(NXBasics::SRectangle(86, 50, 80, 18), 166, Text(17), 10004, 1);
		graphicButton(NXBasics::SRectangle(168, 50, 80, 18), 167, Text(18), 10005, 2);
		graphicButton(NXBasics::SRectangle(250, 50, 80, 18), 192, CGuiBaseStringsManager::GetStringPtr(12, 32), 10007, 4);
		graphicButton(NXBasics::SRectangle(332, 50, 80, 18), 169, Text(19), 10006, 3);
		graphicButton(NXBasics::SRectangle(414, 50, 80, 18), 165, Text(20), 10008, 5);
		graphicButton(NXBasics::SRectangle(184, 70, 50, 18), 171, Text(21), 10009, 6);
		graphicButton(NXBasics::SRectangle(236, 70, 50, 18), 174, Text(23), 10011, 8);
		graphicButton(NXBasics::SRectangle(288, 70, 50, 18), 173, Text(24), 10012, 11);
		graphicButton(NXBasics::SRectangle(340, 70, 50, 18), 175, Text(25), 10013, 12);
		graphicButton(NXBasics::SRectangle(392, 70, 50, 18), 177, Text(22), 10010, 7);
		graphicButton(NXBasics::SRectangle(444, 70, 50, 18), 176, Text(26), 10014, 13);
		NXBasics::SRectangle area(4, 90, 468, 300);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &area);
		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 56);

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanCount); ++index)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(72, 1);
			void* element = allocation == nullptr ? nullptr : HumanListButton_Construct(allocation, &row, Read<std::uint32_t>(window, NC2Layout::HumanListWindowOffset::HumanReferencesUnsigned + index * NC2Layout::RecordSize::HumanReferenceUnsigned), 10000, 10001, window, false);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height + 2;
		}

		NXBaseGui::BaseWindow_Element_Add(window, list);
		area.x = 476;
		area.width = 20;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &area, list, false, true));
		textButton(NXBasics::SRectangle(332, 396, 164, 18), Text(3), Text(4), 10017, false, false);
	}

	// Original function: FUN_004b1758; native job chooser allocation is 0x78 bytes.
	static void* HumanListJobSelectionWindow_Construct(void* window, void* owner, bool currentJob)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle rectangle(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 340u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 260u) / 2, 340, 260);
		BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::HumanListJobSelectionWindowOffset::OwnerWindow, 0, NC2Layout::RecordSize::HumanListJobSelectionWindowVars);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gHumanListJobSelectionVirtualTable.data());
		Write(window, NC2Layout::HumanListJobSelectionWindowOffset::OwnerWindow, owner);
		Write<std::uint8_t>(window, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag, currentJob);
		HumanListJobSelectionWindow_l_GuiButtons_Add(window);

		return window;
	}

	// Original function: FUN_004b10b3; an existing job chooser is reused and brought to front.
	void HumanListWindow_l_JobSelectionWindow_Open(void* window, bool currentJob)
	{
		auto* existing = Read<void*>(window, NC2Layout::HumanListWindowOffset::JobSelectionWindow);

		if (existing != nullptr)
		{
			if ((Read<std::uint8_t>(existing, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag) != 0) != currentJob)
			{
				Write<std::uint8_t>(existing, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag, currentJob);
				NXBaseGui::BaseWindow_Element_DeleteAll(existing);
				HumanListJobSelectionWindow_l_GuiButtons_Add(existing);
			}

			NXBaseGui::CDesktop::sTheObjectPtr->Window_BringToFront(reinterpret_cast<NXBaseGui::CBaseWindow*>(existing));

			return;
		}

		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(120, 1);
		void* subwindow = nullptr;

		if (memory != nullptr)
		{
			subwindow = HumanListJobSelectionWindow_Construct(memory, window, currentJob);
		}

		Write(window, NC2Layout::HumanListWindowOffset::JobSelectionWindow, subwindow);
		BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(subwindow), false);
	}

	// Original functions: FUN_004b17cc and FUN_004b17e8.
	static void* __fastcall HumanListJobSelectionWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		Write<std::uint32_t>(Read<void*>(window, NC2Layout::HumanListJobSelectionWindowOffset::OwnerWindow), NC2Layout::HumanListWindowOffset::JobSelectionWindow, 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004b17fd.
	static bool __fastcall HumanListJobSelectionWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t job, std::uint32_t)
	{
		if (message != 0x800)
			return false;

		if (command == 10000)
		{
			const bool current = Read<std::uint8_t>(window, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag) != 0;
			auto* owner = Read<void*>(window, NC2Layout::HumanListJobSelectionWindowOffset::OwnerWindow);
			HumanListWindow_l_Filter_Reset();
			Write(gHumanListWindowStatics, current ? NC2Layout::HumanListWindowStateOffset::JobFilter : NC2Layout::HumanListWindowStateOffset::PossibleJobFilter, job);
			HumanListWindow_l_UpdateHumanListElements(owner, true);
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}

		return true;
	}

	// Original function: FUN_004b1a6d; the tribe is not used to break equal-name ties.
	static std::int32_t __cdecl HumanListJobSelectionWindow_ls_SortFunction(const void* first, const void* second)
	{

		return StringToolMisc_StringSortCompare(StringAccess_GetJobNamePtr(Read<std::uint32_t>(first, NC2Layout::MapCoordinatePairOffset::Coordinates), 1), StringAccess_GetJobNamePtr(Read<std::uint32_t>(second, NC2Layout::MapCoordinatePairOffset::Coordinates), 1));
	}

	// Original function: FUN_004b1a99; one matching inhabitant makes the pair visible.
	static bool HumanListJobSelectionWindow_l_IsJobToChoose(void* window, std::uint32_t tribe, std::uint32_t job, std::uint32_t player)
	{
		const auto* state = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;

		if (Read<std::uint8_t>(state, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Layout::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) == 0)
			return false;

		if (Read<std::uint8_t>(window, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag) == 0 && tribe != Read<std::uint32_t>(state, NC2Layout::GameControlPlayerOffset::TribeType))
			return false;

		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
		{
			const auto* human = Human(id);

			if (Human_IsJobHero(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) != tribe)
				continue;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == job)
				return true;

			if (Read<std::uint8_t>(window, NC2Layout::HumanListJobSelectionWindowOffset::CurrentJobFlag) == 0 && Human_UserControl_CanChangeJob(human) && Human_IsAllowedToDoJob(human, job))
				return true;
		}

		return false;
	}

	// Original function: FUN_004b1856; all allowed tribe/job pairs are collected and name-sorted.
	static void HumanListJobSelectionWindow_l_GuiButtons_Add(void* window)
	{
		const NXBasics::SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14), close(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &title, Text(50)));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &close));
		NXBasics::SRectangle area(4, 22, 308, 234);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &area);
		auto* pairs = static_cast<std::uint32_t*>(NXBasics::Runtime_AllocateWithNewHandler(18816, 1));
		std::uint32_t count = 0;
		const auto player = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();

		for (std::uint32_t tribe = 0; tribe < 42; ++tribe)
		{
			for (std::uint32_t job = 6; job < 48; ++job)
			{
				if (HumanListJobSelectionWindow_l_IsJobToChoose(window, tribe, job, player))
				{
					pairs[count * 2] = job;
					pairs[count * 2 + 1] = tribe;
					++count;
				}
			}
		}

		if (count > 1)
			NXBasics::Runtime_QuickSort(pairs, count, 8, HumanListJobSelectionWindow_ls_SortFunction);

		NXBasics::SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 18);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto job = pairs[index * 2];
			char caption[200];
			NXBasics::Runtime_PrintString(caption, "%s - %s", StringAccess_GetJobNamePtr(job, 1), StringAccess_GetTribeNamePtr(pairs[index * 2 + 1], 0));
			allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			void* element = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &row, caption, 10000, window);
			Write(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, job);
			Write<std::uint32_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, 0);
			BaseListBoxElement_Element_Add(list, nullptr, element);
			row.y += row.height;
		}

		NXBasics::Runtime_Free(pairs);
		NXBaseGui::BaseWindow_Element_Add(window, list);
		area.x = 316;
		area.width = 20;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
		NXBaseGui::BaseWindow_Element_Add(window, allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &area, list, false, true));
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a1f8e; 004A1F8A belongs to the preceding function epilogue.
	void CInGameGuiManager::MiscWindows_HumanListWindow_Toggle(std::uint32_t job, std::uint32_t possibleJob)
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7E4, job, possibleJob, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_HumanListWindow_Open(job, possibleJob);
		}
	}

	// Original function: FUN_004a1fd0.
	void CInGameGuiManager::MiscWindows_HumanListWindow_Open(std::uint32_t job, std::uint32_t possibleJob)
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E4, job, possibleJob, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		auto* allocation = NXBasics::Runtime_AllocateWithNewHandler(4120, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::HumanListWindow_Construct(allocation, job, possibleJob);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}