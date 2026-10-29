#include "ApplicationGlobals.h"
#include "C2CallbackManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiGroupSelectionWindows.h"
#include "InGameGuiHouseWindow.h"
#include "InGameGuiKeyAssignmentManager.h"
#include "InGameGuiLandscapeSelectionWindow.h"
#include "InGameGuiManager.h"
#include "InGameGuiManager_MessageHandlers.h"
#include "InGameGuiMissionInfoWindow.h"
#include "InGameGuiMissionOnscreenInfoWindow.h"
#include "InGameGuiMousePointer.h"
#include "InGameGuiOverviewWindowAccess.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "InGameGuiSimpleSelectionWindows.h"
#include "InGameGuiSingleVehicleWindow.h"
#include "InGameGuiSupportAccess.h"
#include "InGameGuiWallRoadWindows.h"
#include "MasterPropertyManager.h"
#include "StringInputHandler.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiLists.h"
#include "XList.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstring>
#include <new>

// Untranslated Windows routines. The unused EDX argument keeps stack parameters
// on the original __thiscall stack; constructors retain their original vtables.
extern void* __fastcall InGameGuiManagerPrimaryMessageHandlerElement_Construct(void* object);
extern void __fastcall InGameGuiManager_InputMode_Changed(void* object);
extern void __fastcall InGameGuiManager_Selection_UpdateWindows(void* object);
extern bool __cdecl InGameGuiManager_ls_Logic_CallbackFunction(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);

extern void* __fastcall InGameGuiManager_Vars_Construct(void* object);

namespace
{
	// Original function: FUN_004a0069; SGuiElementPtr list constructors, in native order.
	void* InGameGuiManagerGuiElementPtr_Construct(void* object)
	{
		new (static_cast<std::byte*>(object) + 0x48) NXBasics::CList<NXBaseGui::CBaseElement>(false);
		new (static_cast<std::byte*>(object) + 0x5C) NXBasics::CList<NC2InGameGuiWorldElement::CWorldDisplayElement>(false);
		new (static_cast<std::byte*>(object) + 0x70) NXBasics::CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>(false);

		return object;
	}

	// Original function: FUN_004a0167; end all three placement-constructed list lifetimes in native order.
	void InGameGuiManagerGuiElementPtr_Destruct(void* object)
	{
		NXBasics::CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>::Delete(std::launder(reinterpret_cast<NXBasics::CList<NC2InGameGuiWorldWindow::CWorldObserveDisplayWindow>*>(static_cast<std::byte*>(object) + 0x70)), 0);
		NXBasics::CList<NC2InGameGuiWorldElement::CWorldDisplayElement>::Delete(std::launder(reinterpret_cast<NXBasics::CList<NC2InGameGuiWorldElement::CWorldDisplayElement>*>(static_cast<std::byte*>(object) + 0x5C)), 0);
		NXBasics::CList<NXBaseGui::CBaseElement>::Delete(std::launder(reinterpret_cast<NXBasics::CList<NXBaseGui::CBaseElement>*>(static_cast<std::byte*>(object) + 0x48)), 0);
	}

	// Original function: FUN_004a00a8; ten pairs of 16-bit map coordinates.
	void* GuiMarkers_Construct(void* object)
	{
		for (std::uint32_t index = 0; index < 10; ++index)
		{
			NC2Lifetime::Write<std::uint16_t>(object, index * NC2Layout::RecordSize::MapMigPointUnsigned + NC2Layout::MapCoordinatePairOffset::YUnsigned, 0);
			NC2Lifetime::Write<std::uint16_t>(object, index * NC2Layout::RecordSize::MapMigPointUnsigned, 0);
		}

		return object;
	}

	// Original function: FUN_00479943; original CFrameRateCalculator constructor storage.
	void* FrameRateCalculator_Construct(void* object)
	{
		std::memset(object, 0, 132);
		NC2Lifetime::Write(object, NC2Layout::FrameRateCalculatorOffset::PreviousTick, timeGetTime());

		return object;
	}

	// Original function: FUN_004b9407.
	void ToolGraphics_SetCenterGraphicsFlag(void* object, bool centered)
	{
		std::memcpy(static_cast<std::byte*>(object) + 0x34, &centered, sizeof(centered));
	}

	// Original function: FUN_004a0a90; Mac name: sl_IsMouseStoppedCallback.
	bool __cdecl sl_IsMouseStoppedCallback()
	{
		if (!NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->IsMiddleButtonScrollingEnabled())
			return false;

		return NXSysMouseManager::CMouseManager::sTheObjectPtr->IsMiddleButtonDown();
	}

	std::int32_t ReadIntegerProperty(const char* propertyName, std::int32_t defaultValue)
	{
		if (!NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists(propertyName))
		{
			return defaultValue;
		}

		return NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue(propertyName);
	}
}

// Original function: FUN_004a002d; outer manager construction subsequently resets its complete storage.
void* __fastcall InGameGuiManager_Vars_Construct(void* object)
{
	InGameGuiManagerGuiElementPtr_Construct(static_cast<std::byte*>(object) + 4);
	new (static_cast<std::byte*>(object) + 0xB8) NXBasics::CList<NXBaseGui::CBaseElement>(false);
	GuiMarkers_Construct(static_cast<std::byte*>(object) + 0xE8);
	FrameRateCalculator_Construct(static_cast<std::byte*>(object) + 0x110);

	return object;
}

namespace NInGameGuiManager
{
	// Complete contiguous cache pair 00507378..0050737F, initial PE values -2/-1.
	struct SMiscButtonUpdateCache
	{
		std::uint32_t speed, priority;
	};

	static SMiscButtonUpdateCache gMiscButtonUpdateCache{ 0xFFFFFFFEu, NXBasics::IntegerRange::UInt32Maximum };

	// Original function: FUN_004a299b.
	void CInGameGuiManager::MiscButtons_SpeedButton_Update(bool update)
	{
		using namespace NC2Lifetime;
		void* button = Read<void*>(this, NC2Layout::InGameGuiManagerOffset::SpeedButton);

		if (button == nullptr)
			return;

		const auto speed = NC2Logic::gGameTime.maximizeSpeed != 0 ? NXBasics::IntegerRange::UInt32Maximum : static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(NC2Logic::gGameTime.speed)) / 12u;
		const auto graphicsId = speed == 2 ? 0x34u : speed == 3 ? 0x35u
		                                         : speed == 0   ? 0x36u
		                                                        : 0x31u;
		NC2InGameGuiBase::BaseToolGfxButton_SetGraphicsId(button, nullptr, graphicsId);

		if (gMiscButtonUpdateCache.speed == speed && !update)
		{

			return;
		}

		gMiscButtonUpdateCache.speed = speed;
		char text[100];

		if (speed == NXBasics::IntegerRange::UInt32Maximum)
		{
			std::strcpy(text, "MAX!");
		}
		else
		{
			NXBasics::Runtime_PrintString(text, "%s (*%d)", NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(13), speed);
		}

		button = Read<void*>(this, NC2Layout::InGameGuiManagerOffset::SpeedButton);
		NC2InGameGuiBase::BaseToolGfxButton_SetToolTipString(button, text);
		Write<std::uint8_t>(Read<void*>(this, NC2Layout::InGameGuiManagerOffset::SpeedButton), NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
	}

	// Original function: FUN_004a2a55.
	void CInGameGuiManager::MiscButtons_MessagePriorityButton_Update(bool update)
	{
		using namespace NC2Lifetime;
		void* button = Read<void*>(this, NC2Layout::InGameGuiManagerOffset::MessagePriorityButton);

		if (button == nullptr)
			return;

		const auto priority = Read<std::uint32_t>(NC2InGameGuiMessages::gMessageManager, NC2Layout::GuiMessageManagerOffset::MinimumPriority);
		const auto graphicsId = priority == 1 ? 0x41u : priority == 2 ? 0x42u
		                                                              : 0x40u;
		NC2InGameGuiBase::BaseToolGfxButton_SetGraphicsId(button, nullptr, graphicsId);

		if (gMiscButtonUpdateCache.priority == priority && !update)
		{

			return;
		}

		gMiscButtonUpdateCache.priority = priority;
		const char* text = NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(14);

		if (priority == 1)
			text = NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(15);

		if (priority == 2)
			text = NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(16);

		button = Read<void*>(this, NC2Layout::InGameGuiManagerOffset::MessagePriorityButton);
		NC2InGameGuiBase::BaseToolGfxButton_SetToolTipString(button, text);
		Write<std::uint8_t>(Read<void*>(this, NC2Layout::InGameGuiManagerOffset::MessagePriorityButton), NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
	}

	void* CInGameGuiManager::GetDesktop() const
	{

		return *reinterpret_cast<void* const*>(mLegacyState + 0x04);
	}

	std::int32_t CInGameGuiManager::GetLocalPlayerId() const
	{

		return *reinterpret_cast<const std::int32_t*>(mLegacyState);
	}

	bool CInGameGuiManager::AreCheatsAllowed() const
	{

		return mLegacyState[0xE5] != std::byte{ 0 };
	}

	NC2InGameGuiBaseTextInputElement::CStringInputHandler* CInGameGuiManager::GetStringInputHandler()
	{

		return std::launder(reinterpret_cast<NC2InGameGuiBaseTextInputElement::CStringInputHandler*>(mStringInputStorage));
	}

	// Original function: FUN_0049fd79
	CInGameGuiManager::CInGameGuiManager()
	{
		InGameGuiManager_Vars_Construct(this);
		InGameGuiManagerDefaultMessageHandlerElement_Construct(mDisplayObject);
		static_assert(sizeof(mStringInputStorage) == sizeof(NC2InGameGuiBaseTextInputElement::CStringInputHandler));
		static_assert(offsetof(CInGameGuiManager, mStringInputStorage) % alignof(NC2InGameGuiBaseTextInputElement::CStringInputHandler) == 0);
		auto* stringInput = new (mStringInputStorage) NC2InGameGuiBaseTextInputElement::CStringInputHandler;

		sTheObjectPtr = this;

		std::memset(mLegacyState, 0, sizeof(mLegacyState));
		std::memset(&mStaticVars, 0, sizeof(mStaticVars));

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(1, 1);

		if (allocation != nullptr)
			new (allocation) NC2GuiToolsBase::CGuiBaseStringsManager;

		allocation = NXBasics::Runtime_AllocateWithNewHandler(1, 1);

		if (allocation != nullptr)
			new (allocation) NC2GuiToolsBase::CGuiBaseDataManager;

		void* guiDefinitionTable = NXBasics::Runtime_AllocateWithNewHandler(2568, 1);

		if (guiDefinitionTable != nullptr)
		{
			::new (guiDefinitionTable) NC2InGameGuiKeyAssignmentManager::CKeyAssignmentManager;
		}

		*reinterpret_cast<std::int32_t*>(mLegacyState + NC2Layout::InGameGuiManagerOffset::ScrollSpeed) = ReadIntegerProperty("gui_scroll_speed", 2);
		*reinterpret_cast<std::int32_t*>(mLegacyState + NC2Layout::InGameGuiManagerOffset::MainMode) = ReadIntegerProperty("gui_main_mode", 1);
		mLegacyState[NC2Layout::InGameGuiManagerOffset::ExpertMode] = ReadIntegerProperty("gui_expert_flag", 0) != 0 ? std::byte{ 1 } : std::byte{ 0 };
		mLegacyState[NC2Layout::InGameGuiManagerOffset::TooltipsDisabled] = ReadIntegerProperty("gui_tooltipsoff_flag", 0) != 0 ? std::byte{ 1 } : std::byte{ 0 };
		mLegacyState[NC2Layout::InGameGuiManagerOffset::ThirdButtonScroll] = ReadIntegerProperty("gui_scroll_on_third_button", 1) != 0 ? std::byte{ 1 } : std::byte{ 0 };
		mLegacyState[NC2Layout::InGameGuiManagerOffset::BorderScroll] = ReadIntegerProperty("gui_scroll_on_border", 0) != 0 ? std::byte{ 1 } : std::byte{ 0 };
		mLegacyState[NC2Layout::InGameGuiManagerOffset::SoftwareMouse] = ReadIntegerProperty("gui_mouse_software", 0) != 0 ? std::byte{ 1 } : std::byte{ 0 };

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("allow_cheats") && NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("allow_cheats") != 0)
		{
			mLegacyState[0xE5] = std::byte{ 1 };
		}

		stringInput->Init(60, false, false, 13, 0);
	}

	// Original function: FUN_004a00c1
	CInGameGuiManager::~CInGameGuiManager()
	{
		auto* guiDefinitionTable = NC2InGameGuiKeyAssignmentManager::CKeyAssignmentManager::sTheObjectPtr;

		if (guiDefinitionTable != nullptr)
		{
			guiDefinitionTable->~CKeyAssignmentManager();
			NXBasics::Runtime_Free(guiDefinitionTable);
		}

		auto* guiResourceManager = NC2GuiToolsBase::CGuiBaseDataManager::sTheObjectPtr;

		if (guiResourceManager != nullptr)
		{
			guiResourceManager->~CGuiBaseDataManager();
			NXBasics::Runtime_Free(guiResourceManager);
		}

		auto* textResourceManager = NC2GuiToolsBase::CGuiBaseStringsManager::sTheObjectPtr;

		if (textResourceManager != nullptr)
		{
			textResourceManager->~CGuiBaseStringsManager();
			NXBasics::Runtime_Free(textResourceManager);
		}

		sTheObjectPtr = nullptr;

		std::launder(reinterpret_cast<NC2InGameGuiBaseTextInputElement::CStringInputHandler*>(mStringInputStorage))->Exit();

		// Flags zero reproduce the original inline destructor without freeing
		// the embedded element; its original base-element vtable is retained.
		InGameGuiManagerDefaultMessageHandlerElement_Delete(mDisplayObject, nullptr, 0);

		NXBasics::CList<NXBaseGui::CBaseElement>::Delete(std::launder(reinterpret_cast<NXBasics::CList<NXBaseGui::CBaseElement>*>(mLegacyState + 0xB8)), 0);
		InGameGuiManagerGuiElementPtr_Destruct(mLegacyState + 4);
	}

	bool CInGameGuiManager::IsMiddleButtonScrollingEnabled() const
	{

		return mLegacyState[NC2Layout::InGameGuiManagerOffset::ThirdButtonScroll] != std::byte{ 0 };
	}

	// Original function: FUN_004a030c.
	void CInGameGuiManager::Desktop_Open(std::int32_t width, std::int32_t height, std::uint32_t depth)
	{
		if (mStaticVars.desktopInitialized)
		{

			return;
		}

		// Remaster: GUI coordinates are independent of the native world bitmap.
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
		presentation->ConfigureHud(width, height);
		mStaticVars.displayWidth = width;
		mStaticVars.displayHeight = height;
		mStaticVars.displayDepth = depth;
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBaseGui::CDesktop), 1);

		if (allocation != nullptr)
			::new (allocation) NXBaseGui::CDesktop(width, height, NXBasics::ConvertBitsToTDepth(static_cast<std::int32_t>(depth)));

		if (presentation->GetWorldComposition() != nullptr)
			presentation->SetHudBitmap(NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap());

		NC2GuiToolsBase::CGuiBaseDataManager::DynamicData_Load();
		NXSysMouseManager::CMouseManager::sTheObjectPtr->SetPositionOverrideCallback(sl_IsMouseStoppedCallback);

		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBaseGui::SBaseElementStorage), 1);
		void* primaryHandler = allocation == nullptr ? nullptr : InGameGuiManagerPrimaryMessageHandlerElement_Construct(allocation);
		NXBaseGui::CDesktop::sTheObjectPtr->PrimaryMessageHandler_Set(static_cast<NXBaseGui::CBaseElement*>(primaryHandler));
		allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiManagerMousePointer, 1);
		void* mousePointer = allocation == nullptr ? nullptr : InGameGuiManagerMousePointer_Construct(allocation);
		NXBaseGui::CDesktop::sTheObjectPtr->SetMousePointerStorage(mousePointer);

		constexpr std::uint32_t callbackTypes[] = { 1, 0x14, 0x15, 0x16, 0x1B, 0x2B, 0x2C, 0x2D, 0x2F, 0x30 };

		for (const std::uint32_t type : callbackTypes)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, InGameGuiManager_ls_Logic_CallbackFunction, 0, type == 1 ? 10 : 5);
		}

		NC2E2::Inform_DesktopXTructed(width, height, depth);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler), 1);

		if (allocation != nullptr)
			::new (allocation) NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler();

		allocation = NXBasics::Runtime_AllocateWithNewHandler(580, 1);
		void* worldElement = nullptr;

		if (allocation != nullptr)
		{
			const NXBasics::SRectangle rectangle(0, 0, width, height);
			worldElement = NC2InGameGuiWorldElement::WorldDisplayElement_Construct(allocation, &rectangle, true, true);
		}

		std::memcpy(mLegacyState + 0x04, &worldElement, sizeof(worldElement));
		NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(worldElement));

		const auto addGraphics = [this](std::size_t offset, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::uint32_t graphicsId)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(56, 1);
			void* element = nullptr;

			if (memory != nullptr)
			{
				const NXBasics::SRectangle rectangle(x, y, w, h);
				element = NC2InGameGuiBase::BaseToolGfxElement_Construct(memory, nullptr, &rectangle, graphicsId);
			}

			ToolGraphics_SetCenterGraphicsFlag(element, false);
			NC2InGameGuiBase::ToolGraphics_SetPalettePtr(element, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.iconsLeftPalette);
			std::memcpy(mLegacyState + offset, &element, sizeof(element));
			NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(element));
		};

		const auto addButton = [this](std::size_t offset, std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, std::uint32_t graphicsId, std::int32_t stringId, std::uint32_t tooltipId, bool clearCenterFlag)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
			void* element = nullptr;

			if (memory != nullptr)
			{
				if (stringId >= 0)
				{
					const char* text = NC2GuiToolsBase::CGuiBaseStringsManager::GetStringPtr(1, static_cast<std::uint32_t>(stringId));
					const NXBasics::SRectangle rectangle(x, y, w, h);
					element = NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(memory, nullptr, &rectangle, graphicsId, text, tooltipId, mDisplayObject);
				}
				else
				{
					const NXBasics::SRectangle rectangle(x, y, w, h);
					element = NC2InGameGuiBase::BaseToolGfxButton_Construct(memory, nullptr, &rectangle, graphicsId, tooltipId, mDisplayObject);
				}
			}

			if (clearCenterFlag)
			{
				ToolGraphics_SetCenterGraphicsFlag(element, false);
				NC2InGameGuiBase::ToolGraphics_SetPalettePtr(element, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.iconsLeftPalette);
				std::memcpy(mLegacyState + offset, &element, sizeof(element));
				NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(element));
			}
			else
			{
				// The button at +0x38 is published before its palette is assigned.
				std::memcpy(mLegacyState + offset, &element, sizeof(element));
				NC2InGameGuiBase::ToolGraphics_SetPalettePtr(element, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.iconsLeftPalette);
				std::memcpy(&element, mLegacyState + offset, sizeof(element));
				NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(element));
			}
		};

		addGraphics(0x0C, 0, 10, 50, 433, 0x33);
		addButton(0x14, 0, 41, 40, 35, 0x2A, 2, 0xF3E, true);
		addButton(0x18, 0, 73, 40, 35, 0x2D, 5, 0xF3D, true);
		addButton(0x28, 0, 117, 40, 35, 0x2E, 3, 0xF42, true);
		addButton(0x1C, 0, 151, 40, 35, 0x2C, 4, 0xF3F, true);
		addButton(0x24, 0, 176, 40, 35, 0x32, 7, 0xF41, true);
		addButton(0x20, 0, 204, 40, 35, 0x2B, 6, 0xF40, true);
		addButton(0x2C, 0, 238, 40, 35, 0x38, 8, 0xF43, true);
		addButton(0x10, 0, 295, 40, 35, 0x2F, 1, 0xF3C, true);
		addButton(0x30, 0, 329, 40, 35, 0x30, 0, 0xF44, true);
		addButton(0x34, 0, 373, 40, 35, 0x31, -1, 0xF46, true);
		MiscButtons_SpeedButton_Update(true);
		addButton(0x38, 0, 420, 40, 35, 0xB3, 10, 0xF48, false);
		addGraphics(0x3C, 24, 0, 126, 41, 0x3F);
		addButton(0x40, 106, 3, 37, 31, 0x40, -1, 0xF47, true);
		MiscButtons_MessagePriorityButton_Update(true);
		MiscWindows_OverviewWindow_Open();

		void* window;
		std::memcpy(&window, mLegacyState + NC2Layout::InGameGuiManagerOffset::MissionOnscreenInfoWindow, sizeof(window));

		if (window == nullptr)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(10160, 1);
			window = allocation == nullptr ? nullptr : MissionOnscreenInfoStaticGuiWindow_Construct(allocation);
			std::memcpy(mLegacyState + NC2Layout::InGameGuiManagerOffset::MissionOnscreenInfoWindow, &window, sizeof(window));
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
		}

		mStaticVars.desktopInitialized = true;
		mStaticVars.redrawRequested = true;
		InGameGuiManager_InputMode_Changed(this);
		const std::uint32_t zero = 0;
		std::memcpy(mLegacyState + 0x88, &zero, sizeof(zero));
		InGameGuiManager_Selection_UpdateWindows(this);
		NC2E2::Inform_ControllingPlayerChanged(GetLocalPlayerId());
		NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::sTheObjectPtr->System_ControllingPlayerChanged(GetLocalPlayerId());
		static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->Message_RemoveAll(false);
	}

	// Original function: FUN_004a0c6d.
	void CInGameGuiManager::ControllingPlayer_Set(std::int32_t playerId, bool force)
	{
		if (GetLocalPlayerId() == playerId && !force)
		{

			return;
		}

		NC2Lifetime::Write<std::int32_t>(this, NC2Layout::InGameGuiManagerOffset::LocalPlayerId, playerId);
		NC2E2::Inform_ControllingPlayerChanged(playerId);
		NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::sTheObjectPtr->System_ControllingPlayerChanged(playerId);
		static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->Message_RemoveAll(false);
	}

	// Original function: FUN_004a0aaf.
	void CInGameGuiManager::Desktop_Close()
	{
		if (!mStaticVars.desktopInitialized)
		{

			return;
		}

		l_RemoveAllGuiElements();
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;

		if (desktop != nullptr)
		{
			desktop->~CDesktop();
			NXBasics::Runtime_Free(desktop);
		}

		std::memset(mLegacyState + 0x04, 0, 132);
		auto* overviewHandler = NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::sTheObjectPtr;

		if (overviewHandler != nullptr)
		{
			overviewHandler->~CWorldOverviewElementHandler();
			NXBasics::Runtime_Free(overviewHandler);
		}

		NC2E2::Inform_DesktopXTructed(0, 0, 0);
		constexpr std::uint32_t callbackTypes[] = { 0x30, 0x2F, 0x2D, 0x2C, 0x2B, 0x1B, 0x16, 0x15, 0x14, 1 };

		for (const std::uint32_t type : callbackTypes)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(type, InGameGuiManager_ls_Logic_CallbackFunction, 0);
		}

		if (NXSysMouseManager::CMouseManager::sTheObjectPtr != nullptr)
			NXSysMouseManager::CMouseManager::sTheObjectPtr->SetPositionOverrideCallback(nullptr);

		NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay()->DisableHud();
		mStaticVars.desktopInitialized = false;
	}

	// Original function: FUN_004a0bd4.
	void CInGameGuiManager::EngineWorldDisplayGuiElement_Register(void* element)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0x60))->InsertAtEnd(element);
	}

	// Original function: FUN_004a0be3.
	void CInGameGuiManager::EngineWorldDisplayGuiElement_UnRegister(void* element)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0x60))->l_Base_RemoveElement(element);
	}

	// Original function: FUN_004a0ca6; both the main displays and registered overviews are informed.
	void CInGameGuiManager::Exploration_Set(bool enabled)
	{
		NC2E2::C2DEngineDisplayManager::sTheObjectPtr->InformSetExplorationState(enabled);
		NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::sTheObjectPtr->System_SetExplorationState(enabled);
	}

	// Original function: FUN_004a0cf2.
	bool CInGameGuiManager::Exploration_GetFlag() const
	{
		void* display;
		std::memcpy(&display, static_cast<const std::byte*>(GetDesktop()) + 0x30, sizeof(display));
		std::uint32_t flags;
		std::memcpy(&flags, static_cast<const std::byte*>(display) + 0x70, sizeof(flags));

		return (flags & 4u) != 0;
	}

	// Original function: FUN_004a2d62.
	void CInGameGuiManager::FrameCall_Register(void* element)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0x4C))->InsertAtEnd(element);
	}

	// Original function: FUN_004a2d71.
	void CInGameGuiManager::FrameCall_UnRegister(void* element)
	{
		std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0x4C))->l_Base_RemoveElement(element);
	}

	// Original function: FUN_004a0e9d; all native selection window families are connected.
	void CInGameGuiManager::Selection_UpdateWindows()
	{
		using namespace NC2Lifetime;
		auto* input = NC2InGameGuiInputManager::gInputManager;
		const auto checksum = Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionChangeCount);

		if (Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::SelectionChangeCount) == checksum)
			return;

		Write(this, NC2Layout::InGameGuiManagerOffset::SelectionChangeCount, checksum);
		l_RemoveAllGuiElements();
		NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7ED, 0, 0, 0);
		const auto selection = Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType);

		if (selection == 6)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(15584, 1);
			void* window = allocation == nullptr ? nullptr : NC2InGameGuiSelection::SelectionHouseWindow_Construct(allocation);
			Write(this, NC2Layout::InGameGuiManagerOffset::HouseWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection == 4)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(15488, 1);
			void* window = allocation == nullptr ? nullptr : NC2InGameGuiSelection::SelectionSingleVehicleWindow_Construct(allocation);
			Write(this, NC2Layout::InGameGuiManagerOffset::SingleVehicleWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection == 2 || selection == 5)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
			void* window = allocation == nullptr ? nullptr : selection == 2 ? NC2InGameGuiSelection::SelectionMultipleHumanWindow_Construct(allocation)
			                                                                : NC2InGameGuiSelection::SelectionMultipleVehiclesWindow_Construct(allocation);
			Write(this, selection == 2 ? NC2Layout::InGameGuiManagerOffset::MultipleHumansWindow : NC2Layout::InGameGuiManagerOffset::MultipleVehiclesWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection == 9)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(136, 1);
			void* window = allocation == nullptr ? nullptr : NC2InGameGuiSelection::SelectionLandscapeTypeWindow_Construct(allocation);
			Write(this, NC2Layout::InGameGuiManagerOffset::LandscapeWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection == 3 || selection == 7 || selection == 8)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(selection == 8 ? 0x2F54 : 0x74, 1);
			void* window = allocation == nullptr ? nullptr : selection == 3 ? NC2InGameGuiSelection::SelectionSingleAnimalWindow_Construct(allocation)
			                                             : selection == 7   ? NC2InGameGuiSelection::SelectionHouseTypeWindow_Construct(allocation)
			                                                                : NC2InGameGuiSelection::SelectionGuideWindow_Construct(allocation);
			Write(this, selection == 3 ? NC2Layout::InGameGuiManagerOffset::SingleAnimalWindow : selection == 7 ? NC2Layout::InGameGuiManagerOffset::HouseTypeWindow : NC2Layout::InGameGuiManagerOffset::GuideWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection == 10 || selection == 11)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
			void* window = allocation == nullptr ? nullptr : selection == 10 ? NC2InGameGuiSelection::SelectionRoadBuildWindow_Construct(allocation)
			                                                                 : NC2InGameGuiSelection::SelectionWallBuildWindow_Construct(allocation);
			Write(this, selection == 10 ? NC2Layout::InGameGuiManagerOffset::RoadBuildWindow : NC2Layout::InGameGuiManagerOffset::WallBuildWindow, window);
			NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
			return;
		}

		if (selection != 1)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(15736, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiSelection::SelectionSingleHumanWindow_Construct(allocation);
		Write(this, NC2Layout::InGameGuiManagerOffset::SingleHumanWindow, window);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}

	// Original function: FUN_004a1996; entry bytes recovered from Game.exe.
	void CInGameGuiManager::l_RemoveAllGuiElements()
	{
		l_Selection_RemoveAllGuiElements();

		for (std::size_t offset = 0x8C; offset <= 0xB4; offset += 4)
		{
			NXBaseGui::CBaseWindow* window;
			std::memcpy(&window, mLegacyState + offset, sizeof(window));

			if (window != nullptr)
			{
				NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(window);
				window = nullptr;
				std::memcpy(mLegacyState + offset, &window, sizeof(window));
			}
		}
	}

	// Original function: FUN_004a1af0.
	void CInGameGuiManager::l_Selection_RemoveAllGuiElements()
	{
		auto* list = std::launder(reinterpret_cast<NXBasics::CListBase*>(mLegacyState + 0xB8));

		while (auto* element = static_cast<NXBaseGui::CBaseElement*>(list->l_Base_GetStartElement()))
		{
			list->l_Base_RemoveElement(element);
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Delete(element);
		}

		const std::uint32_t zero = 0;
		std::memcpy(mLegacyState + 0xCC, &zero, sizeof(zero));
	}

	// Original function: FUN_004a20b4.
	void CInGameGuiManager::MiscWindows_MissionInfoWindow_Toggle(std::uint32_t briefingId, bool goalsPage)
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(SwitchIds::MissionInfoCommand::FindMissionInfoWindow, briefingId, goalsPage ? 1u : 0u, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_MissionInfoWindow_Open(briefingId, goalsPage);
		}
	}

	// Original function: FUN_004a20f8.
	void CInGameGuiManager::MiscWindows_MissionInfoWindow_Open(std::uint32_t briefingId, bool goalsPage)
	{
		l_Selection_RemoveAllGuiElements();
		auto* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::MissionInfoCommand::FindMissionInfoWindow, 0, 0, 0);

		if (window != nullptr)
		{
			using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
			NC2Lifetime::Read<TMessage*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](window, SwitchIds::TextButtonMessage::Command, SwitchIds::MissionInfoCommand::SetBriefing, briefingId, goalsPage ? 1u : 0u, 0);

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::MissionInfoWindow, 1);
		void* created = allocation == nullptr ? nullptr : NC2InGameGuiMiscWindows::MissionInfoWindow_Construct(allocation, briefingId, goalsPage);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(created), false);
	}

	// Original function: FUN_004a2171.
	void CInGameGuiManager::MiscWindows_MissionInfoWindow_ReloadStrings()
	{
		auto* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::MissionInfoCommand::FindMissionInfoWindow, 0, 0, 0);

		if (window == nullptr)
		{

			return;
		}

		using TMessage = bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		NC2Lifetime::Read<TMessage*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](window, SwitchIds::TextButtonMessage::Command, SwitchIds::MissionInfoCommand::ClearBriefing, 0, 0, 0);
	}

	// Original function: FUN_004a28a6.
	void CInGameGuiManager::MiscWindows_OverviewWindow_Open()
	{
		void* window;
		std::memcpy(&window, mLegacyState + 0x44, sizeof(window));

		if (window != nullptr)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(124, 1);
		window = allocation == nullptr ? nullptr : NC2InGameGuiOverviewDisplay::WorldOverviewStaticWindow_Construct(allocation);
		std::memcpy(mLegacyState + 0x44, &window, sizeof(window));
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
		NXBaseGui::CBaseElement* button;
		std::memcpy(&button, mLegacyState + 0x48, sizeof(button));

		if (button != nullptr)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Delete(button);
			button = nullptr;
			std::memcpy(mLegacyState + 0x48, &button, sizeof(button));
		}
	}

	// Original function: FUN_004a291b.
	void CInGameGuiManager::MiscWindows_OverviewWindow_Closed()
	{
		using namespace NC2Lifetime;
		Write<void*>(this, NC2Layout::InGameGuiManagerOffset::OverviewWindow, nullptr);

		if (Read<void*>(this, NC2Layout::InGameGuiManagerOffset::StatisticsWindow) != nullptr)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		void* button = nullptr;

		if (allocation != nullptr)
		{
			const NXBasics::SRectangle rectangle(0, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mStaticVars.displayHeight) - 38u), 38, 38);
			const char* tooltip = NC2GuiToolsBase::StringTool_GetGameGuiMainStringPtr(12);
			button = NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, 0x91, tooltip, 0xF45, mDisplayObject);
		}

		Write(this, NC2Layout::InGameGuiManagerOffset::StatisticsWindow, button);
		NC2InGameGuiBase::ToolGraphics_SetPalettePtr(button, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.iconsLeftPalette);
		NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(button));
	}

	// Original function: FUN_004a2322.
	void CInGameGuiManager::MiscWindows_LargeOverviewWindow_Toggle()
	{
		auto* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E9, 0, 0, 0);

		if (window != nullptr)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_LargeOverviewWindow_Open(false);
		}
	}

	// Original function: FUN_004a2358.
	void CInGameGuiManager::MiscWindows_LargeOverviewWindow_Open(bool topRight)
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7E9, 0, 0, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(184, 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiOverviewDisplay::WorldOverviewLargeWindow_Construct(allocation, topRight);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), topRight);
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2af3.
	void CInGameGuiManager::Options_ScrollSpeed_Set(std::uint32_t speed, bool save)
	{
		if (NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ScrollSpeed) == speed)
		{

			return;
		}

		NC2Lifetime::Write<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ScrollSpeed, speed);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2b2d.
	void CInGameGuiManager::Options_GuiMisc_SetMainMode(std::uint32_t mode, bool save)
	{
		if (NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::MainMode) == mode)
		{

			return;
		}

		NC2Lifetime::Write<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::MainMode, mode);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2b52.
	void CInGameGuiManager::Options_GuiMisc_SetExpertFlag(bool enabled, bool save)
	{
		if (NC2Lifetime::Read<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::ExpertMode) == static_cast<std::uint8_t>(enabled))
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::ExpertMode, enabled);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2b77.
	void CInGameGuiManager::Options_GuiMisc_SetTooltipsOffFlag(bool disabled, bool save)
	{
		if (NC2Lifetime::Read<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::TooltipsDisabled) == static_cast<std::uint8_t>(disabled))
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::TooltipsDisabled, disabled);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2b9c.
	void CInGameGuiManager::Options_Scrolling_SetThirdMouseButtonScrollFlag(bool enabled, bool save)
	{
		if (NC2Lifetime::Read<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::ThirdButtonScroll) == static_cast<std::uint8_t>(enabled))
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::ThirdButtonScroll, enabled);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2bc1.
	void CInGameGuiManager::Options_Scrolling_SetBorderScrollFlag(bool enabled, bool save)
	{
		if (NC2Lifetime::Read<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::BorderScroll) == static_cast<std::uint8_t>(enabled))
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::BorderScroll, enabled);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2be6.
	void CInGameGuiManager::Options_Scrolling_SetSoftwareMouseFlag(bool enabled, bool save)
	{
		if (NC2Lifetime::Read<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::SoftwareMouse) == static_cast<std::uint8_t>(enabled))
		{

			return;
		}

		NC2Lifetime::Write<std::uint8_t>(this, NC2Layout::InGameGuiManagerOffset::SoftwareMouse, enabled);

		if (save)
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_004a2b18.
	std::uint32_t CInGameGuiManager::Options_ScrollSpeed_Get() const
	{

		return NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ScrollSpeed);
	}

	// Original function: FUN_004a2b1f; complete three-DWORD table at 00507380..0050738B.
	std::uint32_t CInGameGuiManager::Options_ScrollSpeed_GetInPixelPerSecond() const
	{
		static constexpr std::uint32_t speeds[3]{ 300, 600, 1200 };

		return speeds[NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::InGameGuiManagerOffset::ScrollSpeed)];
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004a3126; the GUI desktop owns the main display.
	void External_FX_Earthquake_Start(std::uint32_t durationMilliseconds)
	{
		const auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (manager == nullptr)
			return;

		const void* desktop = NC2Lifetime::Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);

		if (desktop == nullptr)
		{

			return;
		}

		void* display = NC2Lifetime::Read<void*>(desktop, NC2Layout::WorldGuiElementOffset::Display);

		if (display != nullptr)
			NC2E2::DE_SpecialFX_Earthquake_Start(display, durationMilliseconds);
	}
}

// ABI adapter for original Windows member 004A0E9D.
void __fastcall InGameGuiManager_Selection_UpdateWindows(void* manager)
{
	static_cast<NInGameGuiManager::CInGameGuiManager*>(manager)->Selection_UpdateWindows();
}