#include "InGameGuiStorage.h"
#include "C2E2HumanSoundAccess.h"
#include "C2EngineObjectData.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHumanEquipmentWindow.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelectedAction
{
	using namespace NC2Lifetime;
	using NXBasics::SRectangle;

	namespace
	{
		void* Input()
		{

			return NC2InGameGuiInputManager::gInputManager;
		}

		void* Human(std::int32_t id)
		{

			return NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::ElementSize;
		}

		const char* MiscString(std::uint32_t id)
		{

			return NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(id);
		}

		void AddText(void* window, const SRectangle& rectangle, const char* caption)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextElementStorage), 1);
			void* element = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextElement_Construct(allocation, nullptr, &rectangle, caption, false, false);
			NXBaseGui::BaseWindow_Element_Add(window, element);
		}

		// Original function: FUN_004c85ad; only a zero checksum accepts a new selection.
		void l_System_Validate(void* window)
		{
			const auto type = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);
			const auto saved = Read<std::uint32_t>(window, NC2Layout::HumanEquipmentWindowOffset::SelectionChangeCount);
			const auto current = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionChangeCount);

			if ((type != 1 && type != 2) || (saved != 0 && saved != current))
			{
				Write<std::uint32_t>(window, NC2Layout::HumanEquipmentWindowOffset::SelectionChangeCount, 0);
				Write<std::uint8_t>(window, NC2Layout::HumanEquipmentWindowOffset::ValidSelection, 0);
				NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));

				return;
			}

			Write(window, NC2Layout::HumanEquipmentWindowOffset::SelectionChangeCount, current);
			Write<std::uint8_t>(window, NC2Layout::HumanEquipmentWindowOffset::ValidSelection, 1);
		}

		void AddRow(void* list, void* window, SRectangle& rectangle, const char* caption, bool button, std::uint32_t command, std::uint32_t good, bool spacing)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(button ? sizeof(NC2InGameGuiBase::STextButtonStorage) : sizeof(NC2InGameGuiBase::STextElementStorage), 1);
			void* element = nullptr;

			if (allocation != nullptr)
				element = button ? NC2InGameGuiBase::BaseToolTextButton_Construct(allocation, nullptr, &rectangle, caption, command, window) : NC2InGameGuiBase::BaseToolTextElement_Construct(allocation, nullptr, &rectangle, caption, false, false);

			if (button && element != nullptr)
			{
				Write<std::uintptr_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, good);
				Write<std::uintptr_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, 0);
			}

			BaseListBoxElement_Element_Add(list, nullptr, element);
			rectangle.y += rectangle.height + (spacing ? 2 : 0);
		}

		// Original function: FUN_004c85f1; list order, availability checks and row classes.
		void l_GuiButtons_Add(void* window)
		{
			using namespace NC2InGameGuiBase;
			const auto width = Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width);
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiWindowMoverStorage), 1);
			const SRectangle mover(2, 2, width - 22, 14);
			void* element = allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &mover, MiscString(300));
			NXBaseGui::BaseWindow_Element_Add(window, element);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
			const SRectangle closer(width - 18, 2, 16, 14);
			element = allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &closer);
			NXBaseGui::BaseWindow_Element_Add(window, element);
			l_System_Validate(window);

			if (Read<std::uint8_t>(window, NC2Layout::HumanEquipmentWindowOffset::ValidSelection) == 0)
			{

				return;
			}

			const auto category = Read<std::uint32_t>(window, NC2Layout::HumanEquipmentWindowOffset::Category);
			const auto slot = Read<std::int32_t>(window, NC2Layout::HumanEquipmentWindowOffset::Slot);
			char title[100];
			const char* typeName = NC2GuiToolsBase::StringTool_GetLogicGoodEquipmentTypeNamePtr(category);

			if (slot == -1)
			{
				NXBasics::Runtime_PrintStringN(title, sizeof(title), "%s", typeName);
			}
			else
			{
				NXBasics::Runtime_PrintStringN(title, sizeof(title), "%s #%d", typeName, slot);
			}

			title[sizeof(title) - 1] = '\0';
			AddText(window, SRectangle(4, 22, 412, 16), title);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBaseListBox::SListBoxStorage), 1);
			const SRectangle listRect(4, 44, 388, 234);
			void* list = allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &listRect);
			SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

			if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) == 1)
			{
				const void* human = Human(NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(Input()));

				for (std::uint32_t good = 30; good <= 55; ++good)
				{
					if (category != 6 && NC2Logic::Tool_GoodEquipmentType_Convert(good) != category)
						continue;

					if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + good) == 0)
						continue;

					const auto equipped = (slot != -1 || category != 6) && NC2Logic::Human_HasEquippedItemType(human, good, slot);

					if (!equipped && !NC2Logic::Human_IsAbleToEquipGoodNow(human, good, true))
						continue;

					const auto label = equipped ? (good >= 33 ? 301u : 302u) : 303u;
					char caption[200];
					NXBasics::Runtime_PrintStringN(caption, sizeof(caption), "%s (%s)", NC2Logic::StringAccess_GetGoodNamePtr(good, 1), MiscString(label));
					caption[sizeof(caption) - 1] = '\0';
					AddRow(list, window, row, caption, !equipped || good >= 33, equipped ? 10001u : 10000u, good, true);
				}
			}
			else if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) == 2)
			{
				bool allowed[66]{ };

				for (std::uint32_t good = 30; good <= 55; ++good)
				{
					allowed[good] = true;
				}

				for (std::uint32_t index = 0; index < NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanNumber(Input()); ++index)
				{
					const void* human = Human(NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanId(Input(), index));

					for (std::uint32_t good = 30; good <= 55; ++good)
					{
						if (!allowed[good])
							continue;

						if ((category != 6 && NC2Logic::Tool_GoodEquipmentType_Convert(good) != category) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + good) == 0 || !NC2Logic::Human_IsAbleToEquipGoodNow(human, good, true))
							allowed[good] = false;
					}
				}

				for (std::uint32_t good = 30; good <= 55; ++good)
				{
					if (allowed[good])
						AddRow(list, window, row, NC2Logic::StringAccess_GetGoodNamePtr(good, 1), true, 10000, good, false);
				}
			}

			NXBaseGui::BaseWindow_Element_Add(window, list);
			allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBaseListBox::SListBoxControllerStorage), 1);
			const SRectangle controllerRect(396, 44, 20, 234);
			element = allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &controllerRect, list, false, true);
			NXBaseGui::BaseWindow_Element_Add(window, element);
			const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);

			if (selection == 1)
			{
				const auto id = NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(Input());
				AddText(window, SRectangle(4, 282, 412, 16), reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::Name);
			}
			else if (selection == 2)
			{
				AddText(window, SRectangle(4, 282, 412, 16), MiscString(305));
			}
		}

		// Inlined single-human opcode selector in FUN_004c82e9.
		std::uint8_t EquipmentCommand(std::uint32_t category, std::int32_t slot, bool drop)
		{
			if (slot != -1)
				return static_cast<std::uint8_t>(static_cast<std::uint32_t>(slot) + (drop ? 0x4Bu : 0x54u));

			switch (category)
			{
				case SwitchIds::GoodEquipmentCategory::Weapon:
					return drop ? 0x48 : 0x51;
				case SwitchIds::GoodEquipmentCategory::Armor:
					return drop ? 0x49 : 0x52;
				case SwitchIds::GoodEquipmentCategory::Tool:
					return drop ? 0x47 : 0x50;
				case SwitchIds::GoodEquipmentCategory::Shoes:
					return drop ? 0x46 : 0x4F;
				case SwitchIds::GoodEquipmentCategory::Item:
					return drop ? 0x4A : 0x53;
				default:

					return 0;
			}
		}

		void QueueEquipmentCommand(std::int32_t id, std::uint8_t opcode, std::uint32_t good, std::int32_t slot)
		{
			std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, opcode);
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, good);
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, slot);
			NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
			NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, id);
		}

		// Original function: FUN_004c82e9.
		bool __fastcall SelectedSingleHumanChangeEquippedGoodsWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t firstPayload, std::uintptr_t secondPayload, std::uintptr_t)
		{
			const auto first = static_cast<std::uint32_t>(firstPayload);
			const auto second = static_cast<std::uint32_t>(secondPayload);

			if (message == 0x1000)
			{
				if (command == 2029)
				{
					l_System_Validate(window);
					return false;
				}

				if (command != 2016)
					return false;

				if (first != 0)
				{
					Write(window, NC2Layout::HumanEquipmentWindowOffset::Category, first);
					Write(window, NC2Layout::HumanEquipmentWindowOffset::Slot, second);
					NXBaseGui::BaseWindow_Element_DeleteAll(window);
					l_GuiButtons_Add(window);
				}

				return true;
			}

			if (message != 0x800)
				return false;

			if (command != 10000 && command != 10001)
				return true;

			const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);
			const auto category = Read<std::uint32_t>(window, NC2Layout::HumanEquipmentWindowOffset::Category) == 6 ? NC2Logic::Tool_GoodEquipmentType_Convert(second) : Read<std::uint32_t>(window, NC2Layout::HumanEquipmentWindowOffset::Category);
			const auto slot = Read<std::int32_t>(window, NC2Layout::HumanEquipmentWindowOffset::Slot);

			if (selection == 1)
			{
				const auto id = NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(Input());
				QueueEquipmentCommand(id, EquipmentCommand(category, slot, command == 10001), second, slot);
			}
			else if (selection == 2 && command == 10000)
			{
				for (std::uint32_t index = 0; index < NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanNumber(Input()); ++index)
				{
					const auto id = NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanId(Input(), index);
					// The native group branch ignores slot-specific opcodes and skips matching gear.

					if (category >= 1 && category <= 4 && NC2Logic::Human_HasEquippedItemType(Human(id), second, -1))
						continue;

					const auto opcode = EquipmentCommand(category, -1, false);

					if (opcode != 0)
						QueueEquipmentCommand(id, opcode, second, slot);
				}
			}

			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));

			return true;
		}

		// Original function: FUN_004c859d; this window inherits the native base drawing.
		void __fastcall SelectedSingleHumanChangeEquippedGoodsWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		}

		// Complete nineteen-slot Windows region 004F77A8..004F77F3.
		const std::array<std::uintptr_t, 19> gEquipmentWindowVirtualTable = []
		{
			std::array<std::uintptr_t, 19> table{ };
			std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
			table[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeEquippedGoodsWindow_MessageHandle>());
			table[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeEquippedGoodsWindow_Draw>());

			return table;
		}();
	}

	// Original function: FUN_004c8275; full 0x80-byte allocation and centered rectangle.
	void* SelectedSingleHumanChangeEquippedGoodsWindow_Construct(void* window, std::uint32_t type, std::int32_t slot)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle((state.displayWidth - 420) / 2, (state.displayHeight - 300) / 2, 420, 300);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + offsetof(SHumanEquipmentWindowStorage, category), 0, sizeof(SHumanEquipmentWindowStorage) - offsetof(SHumanEquipmentWindowStorage, category));
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gEquipmentWindowVirtualTable.data());
		Write(window, NC2Layout::HumanEquipmentWindowOffset::Category, type);
		Write(window, NC2Layout::HumanEquipmentWindowOffset::Slot, slot);
		l_GuiButtons_Add(window);

		return window;
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a279d; reuse/retarget the identity-2016 window.
	void CInGameGuiManager::MiscWindows_HumanChangeEquippedGoodsWindow_Open(std::uint32_t type, std::int32_t slot)
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(2016, type, static_cast<std::uint32_t>(slot), 0) != nullptr)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiSelectedAction::SHumanEquipmentWindowStorage), 1);
		void* window = allocation == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedSingleHumanChangeEquippedGoodsWindow_Construct(allocation, type, slot);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(window), false);
	}
}