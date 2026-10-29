#include "InGameGuiStorage.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHouseRemoveWindow.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelectedAction
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2InGameGuiInputManager;
	using NXBasics::SRectangle;
	static void SelectedHouseRemoveWindow_l_System_Validate(void* window);
	static bool __fastcall SelectedHouseRemoveWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	static void __fastcall SelectedHouseRemoveWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	// Complete native table 004F7854..004F789F.
	static const std::array<std::uintptr_t, 19> gSelectedHouseRemoveVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedHouseRemoveWindow_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedHouseRemoveWindow_Draw>());

		return t;
	}();

	static const char* Text(std::uint32_t id)
	{

		return NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(id);
	}

	// Original function: FUN_004c900c; allocation 0x78 and original 320x140 confirmation layout.
	void* SelectedHouseRemoveWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 320u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 140u) / 2, 320, 140);
		BaseToolWindow_Construct(window, &area);
		std::memset(static_cast<std::byte*>(window) + offsetof(SHouseRemoveWindowStorage, selectionValidFlag), 0, sizeof(SHouseRemoveWindowStorage) - offsetof(SHouseRemoveWindowStorage, selectionValidFlag));
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gSelectedHouseRemoveVirtualTable.data());
		const SRectangle title(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiWindowMoverStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowMoverButton_Construct(memory, &title, Text(220)));
		const SRectangle close(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowCloserButton_Construct(memory, nullptr, &close));
		SelectedHouseRemoveWindow_l_System_Validate(window);

		if (Read<std::uint8_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionValidFlag) != 0)
		{
			SRectangle button(4, 20, 154, 116);
			memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_ConstructToggle(memory, nullptr, &button, Text(221), 10000, window, false));
			button.x += button.width + 4;
			memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_ConstructToggle(memory, nullptr, &button, Text(222), 10001, window, false));
		}

		return window;
	}

	// Original function: FUN_004c918c.
	static bool __fastcall SelectedHouseRemoveWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (message != 0x800)
		{
			if (message != 0x1000)
				return false;

			if (command == 0x7EA)
				return true;

			if (command == 0x7ED)
				SelectedHouseRemoveWindow_l_System_Validate(window);

			return false;
		}

		if (command == 10000)
		{
			SelectedHouseRemoveWindow_l_System_Validate(window);

			if (Read<std::uint8_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionValidFlag) != 0)
			{
				const auto id = Selection_GetSelectedHouseId(gInputManager);

				if (id != -1)
				{
					std::byte packet[20]{ };
					Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x5A);
					Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
					NC2Logic::UserNetworkCommand_AddHouseCommand(NC2Logic::gUserNetworkCommandManager, packet);
				}
			}

			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else if (command == 10001)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));
		}

		return true;
	}

	// Original function: FUN_004c923c; the native draw body only validates and queries the selected house.
	static void __fastcall SelectedHouseRemoveWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		SelectedHouseRemoveWindow_l_System_Validate(window);

		if (Read<std::uint8_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionValidFlag) != 0)
			Selection_GetSelectedHouseId(gInputManager);
	}

	// Original function: FUN_004c9268; checksum zero permits the first selected-house revision.
	static void SelectedHouseRemoveWindow_l_System_Validate(void* window)
	{
		const auto checksum = Read<std::uint32_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionChangeCount);

		if (Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionType) == 6 && (checksum == 0 || checksum == Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionChangeCount)))
		{
			Write(window, NC2Layout::HouseRemoveWindowOffset::SelectionChangeCount, Read<std::uint32_t>(gInputManager, NC2Layout::InputManagerOffset::SelectionChangeCount));
			Write<std::uint8_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionValidFlag, 1);

			return;
		}

		Write<std::uint32_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionChangeCount, 0);
		Write<std::uint8_t>(window, NC2Layout::HouseRemoveWindowOffset::SelectionValidFlag, 0);
		NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2826.
	void CInGameGuiManager::MiscWindows_HouseRemoveWindow_Open()
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7EA, 0, 0, 0) != nullptr)
		{

			return;
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelectedAction::SHouseRemoveWindowStorage), 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedHouseRemoveWindow_Construct(memory)), false);
	}
}