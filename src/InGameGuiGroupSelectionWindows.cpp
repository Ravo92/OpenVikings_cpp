#include "XGuiElementAccess.h"
#include "InGameGuiStorage.h"
#include "XPalette.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineObjectData.h"
#include "C2GameState.h"
#include "C2HumanEquipment.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiGroupSelectionWindows.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include <array>
#include <cstring>
#include <new>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiInputManager;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using NXBasics::SRectangle;
	using TMessage = NXBaseGui::TGuiMessage;

	static const char* Text(std::uint32_t id)
	{

		return StringTool_GetGameGuiMiscWindowStringPtr(id);
	}

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	static CStaticToolTipHandler* Tooltips(void* element)
	{

		return std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(element) + offsetof(SMultipleHumanSelectionElementStorage, tooltips)));
	}

	static const void* Human(std::int32_t id)
	{

		return NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::ElementSize;
	}

	static const void* Vehicle(std::int32_t id)
	{

		return NC2Logic::CVehicleArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CVehicleArray::ElementSize;
	}

	static bool Inside(const std::int32_t* point, const SRectangle& area)
	{

		return point[0] >= area.x && point[1] >= area.y && point[0] <= NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.x) + static_cast<std::uint32_t>(area.width)) && point[1] <= NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(area.y) + static_cast<std::uint32_t>(area.height));
	}

	static void* __fastcall SelectionMultipleVehiclesElement_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall SelectionMultipleVehiclesWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t idPayload, std::uintptr_t, std::uintptr_t);
	static void __fastcall SelectionMultipleVehiclesWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	static void __fastcall SelectionMultipleVehiclesElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const SRectangle*);
	static bool __fastcall SelectionMultipleVehiclesElement_IsHit(void* element, void*, const std::int32_t* point);
	static bool __fastcall SelectionMultipleVehiclesElement_ToolTip_Available(void* element, void*, const char** output);
	static bool __fastcall SelectionMultipleHumanWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t idPayload, std::uintptr_t, std::uintptr_t);
	static void __fastcall SelectionMultipleHumanWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle);
	static void* SelectionMultipleHumanElement_Construct(void* element, const SRectangle& rectangle, void* receiver, std::uint32_t select, std::uint32_t action);
	static void* __fastcall SelectionMultipleHumanElement_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall SelectionMultipleHumanElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	static void __fastcall SelectionMultipleHumanElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const SRectangle*);
	static bool __fastcall SelectionMultipleHumanElement_IsHit(void* element, void*, const std::int32_t* point);
	static bool __fastcall SelectionMultipleHumanElement_ToolTip_Available(void* element, void*, const char** output);
	static void __fastcall SelectionMultipleVehiclesElement_DoPressedAction(void* element);
	// Complete native tables 004F7248..004F728F, 004F7290..004F72DB, 004F7328..004F7373 and 004F7374..004F73BB.
	static const std::array<std::uintptr_t, 18> gMultipleVehiclesElementVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), NXBaseGui::gBaseElementVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesElement_Delete>());
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesElement_Draw>());
		t[8] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesElement_IsHit>());
		t[13] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesElement_ToolTip_Available>());
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);
		t[17] = reinterpret_cast<std::uintptr_t>(&SelectionMultipleVehiclesElement_DoPressedAction);

		return t;
	}();
	static const std::array<std::uintptr_t, 19> gMultipleVehiclesWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesWindow_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleVehiclesWindow_Draw>());

		return t;
	}();
	static const std::array<std::uintptr_t, 19> gMultipleHumanWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanWindow_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanWindow_Draw>());

		return t;
	}();
	static const std::array<std::uintptr_t, 18> gMultipleHumanElementVirtualTable = []
	{
		std::array<std::uintptr_t, 18> t{ };
		std::memcpy(t.data(), NXBaseGui::gBaseElementVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_Delete>());
		t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_Message>());
		t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_Draw>());
		t[8] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_IsHit>());
		t[13] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionMultipleHumanElement_ToolTip_Available>());
		t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);
		t[17] = reinterpret_cast<std::uintptr_t>(&SelectionMultipleVehiclesElement_DoPressedAction);

		return t;
	}();

	// Original function: FUN_004beadb; native window allocation 0x74, child allocation 0x104.
	void* SelectionMultipleVehiclesWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 160, state.displayHeight - 300, 160, 300);
		BaseToolWindow_Construct(window, &area);
		Write<std::uint8_t>(window, offsetof(SMultipleVehicleSelectionWindowStorage, unresolved70), 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleVehiclesWindowVirtualTable.data());
		const SRectangle child(4, 20, 152, 262);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelection::SGroupSelectionElementStorage), 1);

		if (memory != nullptr)
		{
			NXBaseGui::BaseElement_Construct(memory, &child, 3, 0x20);
			std::memset(static_cast<std::byte*>(memory) + offsetof(SGroupSelectionElementStorage, hoveredObjectId), 0, sizeof(SGroupSelectionElementStorage) - offsetof(SGroupSelectionElementStorage, hoveredObjectId));
			Write(memory, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleVehiclesElementVirtualTable.data());
			Write<std::int32_t>(memory, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, -1);
			Write<std::uint32_t>(memory, NC2Layout::GroupSelectionElementOffset::PressedCommand, 10000);
			Write<std::uint32_t>(memory, NC2Layout::GroupSelectionElementOffset::ActionCommand, 10001);
			Write(memory, NC2Layout::GroupSelectionElementOffset::Receiver, window);
		}

		NXBaseGui::BaseWindow_Element_Add(window, memory);

		if (NC2Logic::gGameStaticVars.networkGame == 0)
		{
			const SRectangle remove(4, 282, 152, 14);
			memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_Construct(memory, nullptr, &remove, "Debug: remove", 20000, window));
		}

		return window;
	}

	// Original function: FUN_004bebf6.
	static bool __fastcall SelectionMultipleVehiclesWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t idPayload, std::uintptr_t, std::uintptr_t)
	{
		const auto id = static_cast<std::uint32_t>(idPayload);

		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		if (command == 10000)
		{
			if ((Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) != 0)
			{
				UnSelect_Vehicle(gInputManager, id);
			}
			else
			{
				Selection_SelectSingleVehicle(gInputManager, id);
			}
		}
		else if (command == 10001)
		{
			Selection_SelectSingleVehicle(gInputManager, id);
			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();
		}
		else if (command == 20000)
		{
			std::int32_t ids[200];
			std::uint32_t count = 0;

			for (std::uint32_t index = 0; index < Selection_GetSelectedMultipleVehiclesNumber(gInputManager); ++index)
			{
				const auto selected = Selection_GetSelectedMultipleVehicleId(gInputManager, index);

				if (selected != -1)
				{
					ids[count++] = selected;

					if (count == 200)
						break;
				}
			}

			for (std::uint32_t index = 0; index < count; ++index)
			{
				std::byte packet[20]{ };
				Write<std::uint8_t>(packet, NC2Layout::UserNetworkCommandOffset::Opcode, 0x64);
				Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, ids[index]);
				NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, packet);
			}
		}

		return true;
	}

	// Original function: FUN_004bed13.
	static void __fastcall SelectionMultipleVehiclesWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 160, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(471));
		bitmap->Text_PrintV(CGuiBaseDataManager::mStaticVars.resources.font08, White(), 4, 16, "%d %s", Selection_GetSelectedMultipleVehiclesNumber(gInputManager), Text(470));
	}

	// Original function: FUN_004beda9.
	static void* __fastcall SelectionMultipleVehiclesElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleVehiclesElementVirtualTable.data());
		NXBaseGui::BaseElement_Destruct(element);

		if (flags & 1u)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004bef29.
	static void SelectionMultipleVehiclesElement_l_GetVehicleSlotBasePosition(void* element, std::int32_t index, std::int32_t* point)
	{
		const auto count = Selection_GetSelectedMultipleVehiclesNumber(gInputManager);
		auto step = count == 0 ? 0 : (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 100) / static_cast<std::int32_t>(count);

		if (step > 90)
			step = 90;

		point[0] = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) / 2;
		point[1] = 50 + step * index;
	}

	// Original function: FUN_004bedcb.
	static void __fastcall SelectionMultipleVehiclesElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const SRectangle*)
	{
		for (std::uint32_t index = 0; index < Selection_GetSelectedMultipleVehiclesNumber(gInputManager); ++index)
		{
			const auto id = Selection_GetSelectedMultipleVehicleId(gInputManager, index);

			if (id == -1)
				continue;

			std::int32_t point[2];
			SelectionMultipleVehiclesElement_l_GetVehicleSlotBasePosition(element, index, point);
			NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::MapMoveableId), bitmap, point[0], point[1], nullptr, true, -1);
		}
	}

	// Original function: FUN_004bee45; the native human element uses the same folded pressed-action body.
	static void __fastcall SelectionMultipleVehiclesElement_DoPressedAction(void* element)
	{
		const auto id = Read<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId);

		if (id == -1)
		{

			return;
		}

		auto* receiver = Read<void*>(element, NC2Layout::GroupSelectionElementOffset::Receiver);
		Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::GroupSelectionElementOffset::PressedCommand), id, 0, 0);
		FXTool_ClickConfirm();
	}

	// Original function: FUN_004bee72.
	static bool __fastcall SelectionMultipleVehiclesElement_IsHit(void* element, void*, const std::int32_t* point)
	{
		Write<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, -1);

		for (auto index = static_cast<std::int32_t>(Selection_GetSelectedMultipleVehiclesNumber(gInputManager)) - 1; index >= 0; --index)
		{
			const auto id = Selection_GetSelectedMultipleVehicleId(gInputManager, index);

			if (id == -1)
				continue;

			std::int32_t slot[2];
			SelectionMultipleVehiclesElement_l_GetVehicleSlotBasePosition(element, index, slot);

			if (Inside(point, SRectangle(slot[0] - 30, slot[1] - 50, 60, 60)))
			{
				Write(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, id);
				return true;
			}
		}

		return false;
	}

	// Original function: FUN_004beeea.
	static bool __fastcall SelectionMultipleVehiclesElement_ToolTip_Available(void* element, void*, const char** output)
	{
		const auto id = Read<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId);

		if (id == -1)
			return false;

		auto* text = reinterpret_cast<char*>(element) + offsetof(SGroupSelectionElementStorage, caption);
		std::strcpy(text, NC2Logic::StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(Vehicle(id), NC2Layout::VehicleOffset::LogicType)));
		*output = text;

		return true;
	}

	// Original function: FUN_004bf097; native window allocation 0x74 and child allocation 0x2FE8.
	void* SelectionMultipleHumanWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle area(state.displayWidth - 160, state.displayHeight - 360, 160, 360);
		BaseToolWindow_Construct(window, &area);
		Write<std::uint8_t>(window, offsetof(SMultipleHumanSelectionWindowStorage, unresolved70), 0);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleHumanWindowVirtualTable.data());
		const SRectangle actions(4, 20, 152, 18);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : BaseToolTextButton_Construct(memory, nullptr, &actions, Text(461), 11000, window));
		const SRectangle child(4, 62, 152, 294);
		memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelection::SMultipleHumanSelectionElementStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : SelectionMultipleHumanElement_Construct(memory, child, window, 10000, 10001));

		return window;
	}

	// Original function: FUN_004bf186.
	static bool __fastcall SelectionMultipleHumanWindow_Message(void*, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t idPayload, std::uintptr_t, std::uintptr_t)
	{
		const auto id = static_cast<std::uint32_t>(idPayload);

		if (message != 0x800)
			return message == 0x1000 && command == 2001;

		if (command == 10000)
		{
			if ((Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2u) != 0)
			{
				UnSelect_Human(gInputManager, id);
			}
			else
			{
				Selection_SelectSingleHuman(gInputManager, id);
			}
		}
		else if (command == 10001 || command == 11000)
		{
			if (command == 10001)
				Selection_SelectSingleHuman(gInputManager, id);

			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Selection_ActionButtons_BringUp();
		}

		return true;
	}

	// Original function: FUN_004bf201.
	static void __fastcall SelectionMultipleHumanWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		const SRectangle title(0, 0, 160, 16);
		SelectionTool_FillRectangle(bitmap, title, 5);
		SelectionTool_PrintCenteredString(bitmap, title, true, Text(462));
		bitmap->Text_PrintV(CGuiBaseDataManager::mStaticVars.resources.font08, White(), 8, 42, "%d %s", Selection_GetSelectedMultipleHumanNumber(gInputManager), Text(460));
	}

	// Original function: FUN_004bf297.
	static void* SelectionMultipleHumanElement_Construct(void* element, const SRectangle& rectangle, void* receiver, std::uint32_t select, std::uint32_t action)
	{
		NXBaseGui::BaseElement_Construct(element, &rectangle, 3, 0x20);
		new (static_cast<std::byte*>(element) + offsetof(SMultipleHumanSelectionElementStorage, tooltips)) CStaticToolTipHandler;
		std::memset(static_cast<std::byte*>(element) + offsetof(SGroupSelectionElementStorage, hoveredObjectId), 0, sizeof(SMultipleHumanSelectionElementStorage) - offsetof(SGroupSelectionElementStorage, hoveredObjectId));
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleHumanElementVirtualTable.data());
		Write<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, -1);
		Write(element, NC2Layout::GroupSelectionElementOffset::PressedCommand, select);
		Write(element, NC2Layout::GroupSelectionElementOffset::ActionCommand, action);
		Write(element, NC2Layout::GroupSelectionElementOffset::Receiver, receiver);

		return element;
	}

	// Original function: FUN_004bf2f7.
	static void* __fastcall SelectionMultipleHumanElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gMultipleHumanElementVirtualTable.data());
		Tooltips(element)->~CStaticToolTipHandler();
		NXBaseGui::BaseElement_Destruct(element);

		if (flags & 1u)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004bf326; also the native multiple-vehicle message slot.
	static bool __fastcall SelectionMultipleHumanElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (message != 0x20)
			return false;

		const auto id = Read<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId);

		if (id != -1)
		{
			auto* receiver = Read<void*>(element, NC2Layout::GroupSelectionElementOffset::Receiver);
			Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::GroupSelectionElementOffset::ActionCommand), id, 0, 0);
		}

		return true;
	}

	// Original function: FUN_004bf8b8; dense groups retain the original staggered rows and integer divisions.
	static void SelectionMultipleHumanElement_l_GetHumanSlotBasePosition(void* element, std::int32_t index, std::int32_t* point)
	{
		const auto count = Selection_GetSelectedMultipleHumanNumber(gInputManager);

		if (count <= 6)
		{
			auto step = static_cast<std::int32_t>((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 70u) / count);

			if (step > 50)
				step = 50;

			point[0] = 20;
			point[1] = 50 + step * index;

			return;
		}

		const auto columns = (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - 30) / 30;
		const auto rows = count / static_cast<std::uint32_t>(columns);
		const auto row = index / columns, column = index % columns;
		auto step = rows == 0 ? 0 : (Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height) - 70) / static_cast<std::int32_t>(rows);

		if (step > 50)
			step = 50;

		point[0] = 20 + ((row & 1) + column * 2) * 15;
		point[1] = 60 + step * row;
	}

	static void DrawStatusBar(void* element, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t percent, std::uint32_t icon, std::uint32_t label, std::int32_t current, std::int32_t maximum, bool hitpoints)
	{
		DrawTool_DrawIcon(icon, bitmap, x + 6, y + 6);
		SRectangle bar(x + 16, y + 2, width - 16, 8);
		DrawTool_DrawFrame(bitmap, bar, 2);
		bar.Inflate(-2, -2);
		const auto fill = static_cast<std::uint32_t>(percent) > 100u ? 100u : static_cast<std::uint32_t>(percent);
		bar.width = static_cast<std::int32_t>(static_cast<std::uint32_t>(bar.width) * fill / 100u);
		std::uint32_t color = hitpoints ? static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(percent) * 255u) / 100) : static_cast<std::uint32_t>(percent) * 255u / 100u;

		if (color > 255u)
			color = 255u;

		auto* palette = hitpoints ? CGuiBaseDataManager::mStaticVars.resources.hitpointsBarPalette : CGuiBaseDataManager::mStaticVars.resources.standardBarPalette;
		bitmap->Draw_Box(bar, *reinterpret_cast<const NXBasics::SColorRGB*>(palette->GetColorData() + color * 4u));
		char text[100];

		if (hitpoints)
		{
			NXBasics::Runtime_PrintStringN(text, 100, "%s %d/%d", CGuiBaseStringsManager::GetStringPtr(7, label), current, maximum);
		}
		else
		{
			NXBasics::Runtime_PrintStringN(text, 100, "%s %d%%", CGuiBaseStringsManager::GetStringPtr(7, label), percent);
		}

		text[99] = 0;
		Tooltips(element)->Add(SRectangle(x, y, width, 12), text);
	}

	// Original function: FUN_004bf35d; all native small-group bars and dense-group health signs.
	static void __fastcall SelectionMultipleHumanElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const SRectangle*)
	{
		Tooltips(element)->ClearAll();

		for (std::uint32_t index = 0; index < Selection_GetSelectedMultipleHumanNumber(gInputManager); ++index)
		{
			const auto id = Selection_GetSelectedMultipleHumanId(gInputManager, index);

			if (id == -1)
				continue;

			std::int32_t point[2];
			SelectionMultipleHumanElement_l_GetHumanSlotBasePosition(element, index, point);
			const auto* human = Human(id);
			NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), bitmap, point[0], point[1], nullptr, true, -1);
			const auto current = Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints), maximum = static_cast<std::int32_t>(NC2Logic::Human_GetHumanAttributeMaximum_Hitpoints(human));
			auto health = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(current) * 100u) / maximum;

			if (health < 0)
				health = 0;

			if (health > 100)
				health = 100;

			if (Selection_GetSelectedMultipleHumanNumber(gInputManager) <= 6)
			{
				const auto x = point[0] + 24, width = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - x - 4;
				auto y = point[1] - 30;
				DrawStatusBar(element, bitmap, x, y, width, health, 34, 11, current, maximum, true);
				y += 11;

				if (!NC2Logic::Human_IsAgeAdult(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 1u) != 0)
					continue;

				const auto food = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::Food) * 100u) / 10000, stamina = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::Stamina) * 100u) / 10000;
				DrawStatusBar(element, bitmap, x, y, width, food, 38, 12, 0, 0, false);
				y += 11;
				DrawStatusBar(element, bitmap, x, y, width, stamina, 35, 13, 0, 0, false);
			}
			else
			{
				NXBasics::BobManager_PrintBob(Read<void*>(NC2E2::gE2StaticDataBobManagersManager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager), 21 - health / 10, bitmap, point[0], point[1] - 40, NC2E2::EnginePalette_GetPlayerPalette(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId)));
			}
		}
	}

	// Original function: FUN_004bf73f; native hit testing returns true even when no human slot matched.
	static bool __fastcall SelectionMultipleHumanElement_IsHit(void* element, void*, const std::int32_t* point)
	{
		Write<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, -1);

		for (auto index = static_cast<std::int32_t>(Selection_GetSelectedMultipleHumanNumber(gInputManager)) - 1; index >= 0; --index)
		{
			const auto id = Selection_GetSelectedMultipleHumanId(gInputManager, index);

			if (id == -1)
				continue;

			std::int32_t slot[2];
			SelectionMultipleHumanElement_l_GetHumanSlotBasePosition(element, index, slot);

			if (Inside(point, SRectangle(slot[0] - 15, slot[1] - 45, 30, 50)))
			{
				Write(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId, id);
				break;
			}
		}

		return true;
	}

	// Original function: FUN_004bf7b3; status-bar tips take precedence over the hovered human's name.
	static bool __fastcall SelectionMultipleHumanElement_ToolTip_Available(void* element, void*, const char** output)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
		BaseElement_GlobalToLocalCoordinates(element, point);
		const auto* status = Tooltips(element)->Get(point);

		if (status != nullptr)
		{
			*output = status;
			return true;
		}

		const auto id = Read<std::int32_t>(element, NC2Layout::GroupSelectionElementOffset::HoveredObjectId);

		if (id == -1)
			return false;

		const auto* human = Human(id);
		auto* caption = reinterpret_cast<char*>(element) + offsetof(SGroupSelectionElementStorage, caption);
		std::strcpy(caption, reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::DynamicHuman + NC2Layout::DynamicHumanOffset::Name);

		if (!NC2Logic::Human_IsJobHero(human))
		{
			std::strcat(caption, " - ");
			std::strcat(caption, Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 47 ? CGuiBaseStringsManager::GetStringPtr(12, 31) : NC2Logic::StringAccess_GetJobNamePtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 1));
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != 0)
		{
			std::strcat(caption, " (");
			std::strcat(caption, NC2Logic::Human_GetReallyProducedGoodType_GetGuiNamePtr(human));
			std::strcat(caption, ")");
		}

		*output = caption;

		return true;
	}
}