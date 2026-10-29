#include "ApplicationGlobals.h"
#include "C2CallbackManager.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiMousePointer.h"
#include "InGameGuiStringOutputWindow.h"
#include "InGameGuiSupportAccess.h"
#include "InGameGuiWindowState.h"
#include "NetworkShutdown.h"
#include "WorldDisplayElementAccess.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XList.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstring>
#include <new>
#include <Windows.h>

// Compatibility callback binding used by the existing manager registration.
bool __cdecl InGameGuiManager_ls_Logic_CallbackFunction(unsigned int type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3)
{
	return NInGameGuiManager::CInGameGuiManager::ls_Logic_CallbackFunction(type, userData, param1, param2, param3);
}

// Original widget construction, desktop input/rendering and logic callback bodies.
using NC2InGameGuiWorldGameCursor::IGG_DrawWorldGameCursor_LogicCallback;
extern std::uint32_t __fastcall InGameGuiManager_l_Selection_ActionButtons_GetChecksum(void* object);
extern void __fastcall InGameGuiManager_l_Selection_ActionButtons_BringUp(void* object);
extern bool __fastcall InputManager_InputMode_Set(void* object, void* unusedEdx, std::int32_t mode);

namespace
{
	using namespace NC2Lifetime;
	using TMessage = std::uint32_t(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	void SendGuiMessage(void* element, std::uint32_t command, std::uint32_t param1)
	{
		TMessage* methods = Read<TMessage*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable);
		methods[2](element, 0x800, command, param1, 0, 0);
	}

	// Original function: FUN_00479975; CFrameRateCalculator::CalculateFrameRate(tick).
	double CalculateFrameRate(void* calculator, std::uint32_t tick)
	{
		const auto previous = Read<std::uint32_t>(calculator, NC2Layout::FrameRateCalculatorOffset::PreviousTick);
		if (tick < previous) tick = previous;
		const auto position = Read<std::uint32_t>(calculator, NC2Layout::FrameRateCalculatorOffset::NextIntervalIndex);
		Write(calculator, NC2Layout::FrameRateCalculatorOffset::FrameIntervalsUnsigned + position * NC2Layout::RecordSize::FrameIntervalUnsigned, tick - previous);
		const auto next = position + 1u;
		Write<std::uint32_t>(calculator, NC2Layout::FrameRateCalculatorOffset::NextIntervalIndex, next == 30 ? 0 : next);
		Write(calculator, NC2Layout::FrameRateCalculatorOffset::PreviousTick, tick);
		std::uint32_t total = 0;
		for (std::uint32_t index = 0; index < 30; ++index) total += Read<std::uint32_t>(calculator, NC2Layout::FrameRateCalculatorOffset::FrameIntervalsUnsigned + index * NC2Layout::RecordSize::FrameIntervalUnsigned);
		double interval = static_cast<double>(total) * static_cast<double>(0.03333333507180214f);
		interval *= static_cast<double>(0.0010000000474974513f);
		const double rate = 1.0 / interval;
		Write<float>(calculator, NC2Layout::FrameRateCalculatorOffset::FrameRate, static_cast<float>(rate));
		return rate;
	}

	// Original function: FUN_00479962; CFrameRateCalculator::CalculateFrameRate().
	double CalculateFrameRate(void* calculator)
	{
		return CalculateFrameRate(calculator, timeGetTime());
	}
}

namespace NC2InGameGuiMiscWindows
{
	// Original function: FUN_004a59ab.
	void MissionInfoWindow_MissionBriefingHistory_Init()
	{
		gMissionBriefingHistory.count = 0;
		for (auto& entry : gMissionBriefingHistory.entries) entry = -1;
	}
	// Original function: FUN_004a5ff5.
	void TechTreeWindow_Statics_Init()
	{
		std::memset(gTechTreeWindowStatics, 0, sizeof(gTechTreeWindowStatics));
	}
	// Original function: FUN_004af057.
	void DiplomacyWindow_Statics_Init()
	{
		std::memset(gDiplomacyWindowStatics, 0, sizeof(gDiplomacyWindowStatics));
		NC2Lifetime::Write<std::uint32_t>(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page, 0);
	}
	// Original function: FUN_004b1093; CHumanListWindow::l_Statics_Reset.
	void HumanListWindow_l_Filter_Reset()
	{
		std::memset(gHumanListWindowStatics, 0, sizeof(gHumanListWindowStatics));
		NC2Lifetime::Write<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::JobFilter, 0);
		NC2Lifetime::Write<std::uint32_t>(gHumanListWindowStatics, NC2Layout::HumanListWindowStateOffset::PossibleJobFilter, 0);
	}
	// Original function: FUN_004b00da.
	void HumanListWindow_Statics_Init()
	{
		std::memset(gHumanListWindowStatics, 0, sizeof(gHumanListWindowStatics));
		HumanListWindow_l_Filter_Reset();
	}
	// Original function: FUN_004b1dde.
	void StatisticsWindow_Statics_Init()
	{
		std::memset(gStatisticsWindowStatics, 0, sizeof(gStatisticsWindowStatics));
		NC2Lifetime::Write<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::Page, 0);
		std::memset(gStatisticsWindowStatics + NC2Layout::StatisticsWindowStateOffset::TimeScaleHours, 0, 4);
		NC2Lifetime::Write<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::TimeScaleHours, 1);
	}
}

namespace NC2InGameGuiSelection
{
	// Original function: FUN_004a7142.
	void SelectionHouseWindow_Statics_Init()
	{
		std::memset(gSelectionHouseWindowStatics, 0, sizeof(gSelectionHouseWindowStatics));
	}
	// Original function: FUN_004a9946.
	void SelectionSingleVehicleWindow_Statics_Init()
	{
		std::memset(gSelectionSingleVehicleWindowStatics, 0, sizeof(gSelectionSingleVehicleWindowStatics));
	}
	// Original function: FUN_004aba22.
	void SelectionSingleHumanWindow_Statics_Init()
	{
		std::memset(gSelectionSingleHumanWindowStatics, 0, sizeof(gSelectionSingleHumanWindowStatics));
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2c0b; native registered GUI callback tree.
	bool __cdecl CInGameGuiManager::ls_Logic_CallbackFunction(std::uint32_t type, std::uintptr_t, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		auto* manager = sTheObjectPtr;
		if (type == 1)
		{
			manager->Selection_UpdateWindows(); manager->MiscButtons_SpeedButton_Update(false); manager->MiscButtons_MessagePriorityButton_Update(false); manager->l_UpdateElement0038();
		}
		else if (type == 0x14) l_IO_Savegame_Save(reinterpret_cast<NC2Logic::CIoHelper*>(callbackValueParam1));
		else if (type == 0x15) l_IO_Savegame_Load(reinterpret_cast<NC2Logic::CIoHelper*>(callbackValueParam1));
		else if (type == 0x16)
		{
			using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t); NXBasics::CListBase::Iterator iterator(*std::launder(reinterpret_cast<NXBasics::CListBase*>(manager->mLegacyState + 0x60)));
			for (auto* world = iterator.First(); world != nullptr; world = iterator.Next()) NC2Lifetime::Read<TMessage*>(world, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](world, 0x800, 0x7EF, 0, 0, 0);
			if (param1 != 0) NC2InGameGuiMiscWindows::MissionInfoWindow_MissionBriefingHistory_Init(); manager = sTheObjectPtr; if (manager != nullptr && manager->GetDesktop() != nullptr) NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(manager->GetDesktop());
		}
		else if (type == 0x1B) manager->MiscWindows_MissionInfoWindow_Open(param1, param2 != 0);
		else if (type == 0x2B)
		{
			NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition(); manager->MiscWindows_MissionInfoWindow_ReloadStrings(); manager->ObservationWindow_CloseAll();
		}
		else if (type == 0x2C)
		{
			manager->MiscWindows_MissionInfoWindow_ReloadStrings(); manager->ObservationWindow_CloseAll();
		}
		else if (type == 0x2D)
		{
			NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition(); manager->ObservationWindow_CloseAll();
		}
		else if (type == 0x2F) manager->MiscWindows_GameSafetyBoxWindow_Open(2);
		else if (type == 0x30) manager->MiscWindows_NetworkInfoWindow_Open();
		return true;
	}
	using namespace NC2Lifetime;

	// Original function: FUN_004a0184.
	void CInGameGuiManager::System_UpdateGui()
	{
		FrameCall_Do();
		l_Selection_ActionButtons_CheckUpdate();
		CalculateFrameRate(reinterpret_cast<std::byte*>(this) + 0x110);
		sTheObjectPtr->MiscButtons_SpeedButton_Update(false);
		sTheObjectPtr->MiscButtons_MessagePriorityButton_Update(false);
		sTheObjectPtr->l_UpdateElement0038();
		NXBaseGui::CDesktop::sTheObjectPtr->Desktop_DoWork(false, mStaticVars.redrawRequested);
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		mStaticVars.redrawRequested = false;
		auto* underMouse = desktop->Element_GetUnderMousePtr();
		if (underMouse != nullptr && underMouse == Read<NXBaseGui::CBaseElement*>(this, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement))
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Activate(Read<NXBaseGui::CBaseElement*>(this, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement));
	}

	// Original function: FUN_004a0205; System_StartGame does not use ECX.
	void CInGameGuiManager::System_StartGame()
	{
		using namespace NC2InGameGuiInputManager;
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(3964, 1);
		if (allocation != nullptr) new (allocation) CInputManager;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiMessages::CMessageManager), 1);
		if (allocation != nullptr) new (allocation) NC2InGameGuiMessages::CMessageManager;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::ObjectGroupManager, 1);
		if (allocation != nullptr) new (allocation) NC2InGameGuiGroups::CObjectGroupManager;
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager), 1);
		if (allocation != nullptr) new (allocation) NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager;
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x19, IGG_DrawWorldGameCursor_LogicCallback, 0, 10);
		NC2InGameGuiMiscWindows::StatisticsWindow_Statics_Init();
		NC2InGameGuiMiscWindows::HumanListWindow_Statics_Init();
		NC2InGameGuiMiscWindows::DiplomacyWindow_Statics_Init();
		NC2InGameGuiSelection::SelectionSingleHumanWindow_Statics_Init();
		NC2InGameGuiSelection::SelectionSingleVehicleWindow_Statics_Init();
		NC2InGameGuiSelection::SelectionHouseWindow_Statics_Init();
		NC2InGameGuiMiscWindows::TechTreeWindow_Statics_Init();
		NC2InGameGuiMiscWindows::MissionInfoWindow_MissionBriefingHistory_Init();
	}

	// Original function: FUN_004a0295; reverse owner destruction order.
	void CInGameGuiManager::System_EndGame()
	{
		NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(0x19, IGG_DrawWorldGameCursor_LogicCallback, 0);
		auto* deadHumans = static_cast<NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager*>(NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager);
		if (deadHumans != nullptr)
		{
			deadHumans->~CDeadHumanDataManager();
			NXBasics::Runtime_Free(deadHumans);
		}
		auto* groups = static_cast<NC2InGameGuiGroups::CObjectGroupManager*>(NC2InGameGuiGroups::gObjectGroupManager);
		if (groups != nullptr)
		{
			groups->~CObjectGroupManager();
			NXBasics::Runtime_Free(groups);
		}
		auto* messages = static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager);
		if (messages != nullptr)
		{
			messages->~CMessageManager();
			NXBasics::Runtime_Free(messages);
		}
		auto* input = static_cast<NC2InGameGuiInputManager::CInputManager*>(NC2InGameGuiInputManager::gInputManager);
		if (input != nullptr)
		{
			input->~CInputManager();
			NXBasics::Runtime_Free(input);
		}
	}

	// Original function: FUN_004a1bc3.
	void CInGameGuiManager::l_Selection_ActionButtons_CheckUpdate()
	{
		if (Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ActionButtonsChecksum) == 0) return;
		const auto checksum = InGameGuiManager_l_Selection_ActionButtons_GetChecksum(this);
		if (checksum == Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ActionButtonsChecksum)) return;
		l_Selection_RemoveAllGuiElements();
		InGameGuiManager_l_Selection_ActionButtons_BringUp(this);
	}

	// Original function: FUN_004a2436; element role remains unresolved.
	bool CInGameGuiManager::l_IsElement0038Required() const
	{
		if (NNetwork::Network_IsInSession()) return true;
		return Read<std::uint8_t>(NC2Logic::gMultiPlayerGoalManager, NC2Layout::MultiPlayerGoalManagerOffset::Enabled) != 0;
	}

	// Original function: FUN_004a244e; complete table 004F4DA0..004F4DCF.
	void CInGameGuiManager::MiscWindows_CloseAllLargeOnes(bool resetInputMode)
	{
		static constexpr std::uint32_t commands[] = { 0x7D8, 0x7D9, 0x7E3, 0x7E4, 0x7E5, 0x7E6, 0x7E7, 0x7E8, 0x7E9, 0x7EC, 0x7DA, NXBasics::IntegerRange::UInt32Maximum };
		for (auto command : commands)
		{
			if (command == NXBasics::IntegerRange::UInt32Maximum) break;
			auto* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(command, 0, 0, 0);
			if (window != nullptr) NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		if (resetInputMode) InputManager_InputMode_Set(NC2InGameGuiInputManager::gInputManager, nullptr, 0);
	}

	// Original function: FUN_004a2598.
	void CInGameGuiManager::MiscWindows_StringOutputWindow_Open()
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7DC, 0, 0, 0) != nullptr) return;
		MiscWindows_CloseAllLargeOnes(false);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::StringOutputWindow_Construct(allocation);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}

	// Original function: FUN_004a25e0.
	void CInGameGuiManager::MiscWindows_StringOutputWindow_PrintString(const char* text)
	{
		MiscWindows_StringOutputWindow_Open();
		void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7DC, 0, 0, 0);
		if (window != nullptr) SendGuiMessage(window, 0x4E20, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(text)));
	}

	// Original function: FUN_004a261a; native variadic adapter uses a 1000-byte buffer.
	void __cdecl CInGameGuiManager::MiscWindows_StringOutputWindow_PrintStringV(const char* format, ...)
	{
		char text[1000]; va_list arguments; va_start(arguments, format); NXBasics::Runtime_FormatString(text, 1000, format, arguments); va_end(arguments); text[999] = 0; MiscWindows_StringOutputWindow_PrintString(text);
	}
	// Original function: FUN_004a2653.
	void CInGameGuiManager::MiscWindows_StringOutputWindow_ClearAll()
	{
		void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7DC, 0, 0, 0);
		if (window != nullptr) SendGuiMessage(window, 0x4E21, 0);
	}

	// Original function: FUN_004a2acd.
	void CInGameGuiManager::l_UpdateElement0038()
	{
		if (Read<void*>(this, NC2Layout::InGameGuiManagerOffset::NetworkInfoButton) == nullptr || l_IsElement0038Required()) return;
		NXBaseGui::CDesktop::sTheObjectPtr->Element_Delete(Read<NXBaseGui::CBaseElement*>(this, NC2Layout::InGameGuiManagerOffset::NetworkInfoButton));
		Write<void*>(this, NC2Layout::InGameGuiManagerOffset::NetworkInfoButton, nullptr);
	}

	// Original function: FUN_004a2d80; callback mutation follows the original iterator.
	void CInGameGuiManager::FrameCall_Do()
	{
		auto* list = std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0x4C));
		NXBasics::CListBase::Iterator iterator(*list);
		for (void* element = iterator.First(); element != nullptr; element = iterator.Next()) SendGuiMessage(element, 0x7EE, 0);
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2ea7; complete ten-position region +0xE8..+0x10F.
	void CInGameGuiManager::GuiMarker_Set(std::uint32_t index, const std::uint32_t* point)
	{
		if (index < 10) NC2Lifetime::Write(this, NC2Layout::InGameGuiManagerOffset::GuiMarkers + index * NC2Layout::RecordSize::PackedMapPointUnsigned, *point);
	}
	// Original function: FUN_004a2ec0.
	void CInGameGuiManager::GuiMarker_Get(std::uint32_t index, std::uint32_t* point) const
	{
		*point = 0; if (index < 10) *point = NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::GuiMarkers + index * NC2Layout::RecordSize::PackedMapPointUnsigned);
	}
}
