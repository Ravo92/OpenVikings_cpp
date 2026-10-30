#include "InGameGuiStorage.h"
#include "C2DefaultInteraction.h"
#include "C2E2HumanSoundAccess.h"
#include "C2HouseUserControl.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHumanActionButtons.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XList.h"
#include "XMemoryChecksum.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"

using namespace NC2Lifetime;

namespace
{
	void* Input()
	{

		return NC2InGameGuiInputManager::gInputManager;
	}
}

// Original function: FUN_004a1bf0; Windows deliberately rehashes the first list in both group branches.
std::uint32_t __fastcall InGameGuiManager_l_Selection_ActionButtons_GetChecksum(void*)
{
	using namespace NC2InGameGuiInputManager;
	const auto type = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);

	if (type == 6)
	{
		NC2Logic::SUserCommandsForHouseHandler house;

		if (!NC2Logic::Logic_UserControl_GetHouseCommands(Selection_GetSelectedHouseId(Input()), NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId(), &house))
			return 0;

		return NXBasics::XB_GetMemoryChecksum(&house, sizeof(house));
	}

	if (type == 4 || type == 5)
	{
		NC2Logic::SUserCommandsForVehicleHandler vehicle;

		if (!NC2Logic::Logic_UserControl_GetVehicleCommands(type == 4 ? Selection_GetSelectedSingleVehicleId(Input()) : Selection_GetSelectedMultipleVehicleId(Input(), 0), &vehicle, type == 5))
			return 0;

		auto checksum = NXBasics::XB_GetMemoryChecksum(&vehicle, sizeof(vehicle));

		if (type == 4)
			return checksum;

		const auto count = Selection_GetSelectedMultipleVehiclesNumber(Input());

		for (std::uint32_t index = 1; index < count; ++index)
		{
			NC2Logic::SUserCommandsForVehicleHandler other;

			if (!NC2Logic::Logic_UserControl_GetVehicleCommands(Selection_GetSelectedMultipleVehicleId(Input(), index), &other, true))
				break;

			checksum ^= NXBasics::XB_GetMemoryChecksum(&vehicle, sizeof(vehicle)) + 10u;
		}

		return checksum;
	}

	NC2Logic::SUserCommandsForHumanHandler commands;

	if (type == 1)
	{
		if (!NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedSingleHumanId(Input()), &commands, false, false))
			return 0;

		return NXBasics::XB_GetMemoryChecksum(&commands, sizeof(commands));
	}

	if (type != 2 || !NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedMultipleHumanId(Input(), 0), &commands, true, false))
		return 0;

	auto checksum = NXBasics::XB_GetMemoryChecksum(&commands, sizeof(commands));
	const auto count = Selection_GetSelectedMultipleHumanNumber(Input());

	for (std::uint32_t index = 1; index < count; ++index)
	{
		NC2Logic::SUserCommandsForHumanHandler other;

		if (!NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedMultipleHumanId(Input(), index), &other, true, false))
			break;
		// Windows hashes the first command list again, rather than the newly filled list.
		checksum ^= NXBasics::XB_GetMemoryChecksum(&commands, sizeof(commands)) + 10u;
	}

	return checksum;
}

// Original function: FUN_004a10cb; native house/vehicle text lists and human radial actions.
void __fastcall InGameGuiManager_l_Selection_ActionButtons_BringUp(void* manager)
{
	using namespace NC2InGameGuiInputManager;
	const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);

	if (selection == 4 || selection == 5 || selection == 6)
	{
		NC2Logic::SUserCommandsForHouseHandler house;
		NC2Logic::SUserCommandsForVehicleHandler vehicle;
		std::int32_t count;
		const std::uint32_t* commands;

		if (selection == 6)
		{
			if (!NC2Logic::Logic_UserControl_GetHouseCommands(Selection_GetSelectedHouseId(Input()), NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId(), &house))
				return;

			count = house.count;
			commands = house.commands;
		}
		else
		{
			if (!NC2Logic::Logic_UserControl_GetVehicleCommands(selection == 4 ? Selection_GetSelectedSingleVehicleId(Input()) : Selection_GetSelectedMultipleVehicleId(Input(), 0), &vehicle, selection == 5))
				return;

			if (selection == 5)
			{
				const auto members = Selection_GetSelectedMultipleVehiclesNumber(Input());

				for (std::uint32_t index = 1; index < members; ++index)
				{
					NC2Logic::SUserCommandsForVehicleHandler other;

					if (!NC2Logic::Logic_UserControl_GetVehicleCommands(Selection_GetSelectedMultipleVehicleId(Input(), index), &other, true))
						return;

					for (std::int32_t command = 0; command < vehicle.count; ++command)
					{
						if (other.IsCommandAllowed(vehicle.commands[command]))
							continue;

						for (auto next = command; next < vehicle.count - 1; ++next)
						{
							vehicle.commands[next] = vehicle.commands[next + 1];
						}

						--vehicle.count;
						--command;
					}
				}

				if (vehicle.count == 0)
					return;
			}

			count = vehicle.count;
			commands = vehicle.commands;
		}

		NXBasics::SRectangle area(Read<std::int32_t>(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginX) - 260, Read<std::int32_t>(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginY) - count * 22 / 2, 240, count * 22);
		NXBasics::SRectangle bounds(0, 0, 0, 0);
		std::memcpy(&bounds, NXBaseGui::CDesktop::sTheObjectPtr, sizeof(bounds));
		area.PlaceInside(bounds);
		area.height = 22;

		for (std::int32_t index = 0; index < count; ++index)
		{
			const auto action = commands[index];
			const auto* caption = selection == 6 ? NC2GuiToolsBase::StringTool_GetLogicUserCommandsForHouseTypeNamePtr(action) : NC2GuiToolsBase::StringTool_GetLogicUserCommandsForVehicleTypeNamePtr(action);
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
			void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_Construct(memory, nullptr, &area, caption, (selection == 6 ? 3300u : 3600u) + action, static_cast<std::byte*>(manager) + NC2Layout::InGameGuiManagerOffset::DefaultMessageHandler);
			NC2InGameGuiBase::BaseToolDesktop_AddOverlayElement(static_cast<NXBaseGui::CBaseElement*>(element));
			reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + NC2Layout::InGameGuiManagerOffset::ActionButtonsList)->InsertAtEnd(element);
			area.y += 22;
		}

		Write(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsChecksum, InGameGuiManager_l_Selection_ActionButtons_GetChecksum(manager));
		return;
	}

	if (selection != 1 && selection != 2)
	{
		Write(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsChecksum, InGameGuiManager_l_Selection_ActionButtons_GetChecksum(manager));
		return;
	}

	NC2Logic::SUserCommandsForHumanHandler commands;

	if (selection == 1)
	{
		if (!NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedSingleHumanId(Input()), &commands, false, false))
			return;
	}
	else
	{
		if (!NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedMultipleHumanId(Input(), 0), &commands, true, false))
			return;

		const auto count = Selection_GetSelectedMultipleHumanNumber(Input());

		if (count > 1)
		{
			for (std::uint32_t index = 1; index < count; ++index)
			{
				NC2Logic::SUserCommandsForHumanHandler other;

				if (!NC2Logic::Logic_UserControl_GetHumanCommands(Selection_GetSelectedMultipleHumanId(Input(), index), &other, true, false))
					return;

				for (std::int32_t command = 0; command < commands.count; ++command)
				{
					if (other.IsCommandAllowed(commands.commands[command]))
						continue;

					for (auto next = command; next < commands.count - 1; ++next)
					{
						commands.commands[next] = commands.commands[next + 1];
					}

					--commands.count;
					--command;
				}
			}

			for (std::int32_t command = 0; command < commands.count; ++command)
			{
				if (commands.commands[command] != 12)
					continue;

				for (auto next = command; next < commands.count - 1; ++next)
				{
					commands.commands[next] = commands.commands[next + 1];
				}

				--commands.count;
				--command;
			}
		}
	}

	if (commands.count == 0)
	{

		return;
	}

	commands.SortCommandsByPriority();
	std::uint32_t groups[5]{ };

	for (std::int32_t index = 0; index < commands.count; ++index)
	{
		++groups[NC2Logic::Tool_UserHumanCommandTypes_GetGroupType(commands.commands[index])];
	}

	NXBasics::SRectangle area(Read<std::int32_t>(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginX) - 116, Read<std::int32_t>(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginY) - 116, 232, 232);
	NXBasics::SRectangle desktopBounds(0, 0, 0, 0);
	// CDesktop stores its rectangle at +0, unlike CBaseElement's rectangle at +4.
	std::memcpy(&desktopBounds, NXBaseGui::CDesktop::sTheObjectPtr, sizeof(desktopBounds));
	area.PlaceInside(desktopBounds);
	// Complete graphic switch 004A18E6..004A1995, covering native actions 2..45.
	constexpr std::uint32_t graphics[44]{ 107, 109, 108, 125, 126, 104, 105, 106, 101, 102, 127, 128, 107, 107, 96, 97, 129, 130, 103, 133, 131, 132, 112, 135, 134, 107, 100, 99, 107, 111, 136, 107, 107, 114, 115, 116, 117, 123, 124, 118, 119, 120, 121, 122 };

	for (std::uint32_t group = 0; group < 5; ++group)
	{
		auto x = area.x + area.width / 2, y = area.y + area.height / 2;
		std::int32_t dx = 0, dy = 0, bendX = 0, bendY = 0;
		const auto span = static_cast<std::int32_t>(groups[group] * 16);

		switch (group)
		{
			case SwitchIds::ActionButtonArc::Bottom:
				x += 16 - span;
				y += 100;
				dx = 32;
				bendY = -5;
				break;
			case SwitchIds::ActionButtonArc::Top:
				x += span - 16;
				y -= 100;
				dx = -32;
				bendY = 5;
				break;
			case SwitchIds::ActionButtonArc::Right:
				x += 100;
				y += 16 - span;
				dy = 32;
				bendX = -5;
				break;
			case SwitchIds::ActionButtonArc::Left:
				x -= 100;
				y += span - 16;
				dy = -32;
				bendX = 5;
				break;
			case SwitchIds::ActionButtonArc::InnerLeft:
				x -= 68;
				y += span - 16;
				dy = -32;
				bendX = 5;
				break;
		}

		std::uint32_t ordinal = 0;

		for (std::int32_t index = 0; index < commands.count; ++index)
		{
			const auto action = commands.commands[index];

			if (NC2Logic::Tool_UserHumanCommandTypes_GetGroupType(action) != group)
				continue;

			const char* tooltip = NC2GuiToolsBase::StringTool_GetLogicUserCommandsForHumanTypeNamePtr(action);
			const auto graphic = action >= 2 && action <= 45 ? graphics[action - 2] : 107u;
			NXBasics::SRectangle rectangle(x - 16, y - 16, 32, 32);

			if (ordinal == 0 || ordinal == groups[group] - 1)
			{
				rectangle.x += bendX;
				rectangle.y += bendY;
			}

			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
			void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(memory, nullptr, &rectangle, graphic, tooltip, 3000 + action, static_cast<std::byte*>(manager) + NC2Layout::InGameGuiManagerOffset::DefaultMessageHandler);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::Centered, 1);
			Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
			NC2InGameGuiBase::ToolGraphics_SetPalettePtr(element, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.contextPalette);
			NC2InGameGuiBase::BaseToolDesktop_AddOverlayElement(static_cast<NXBaseGui::CBaseElement*>(element));
			reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + NC2Layout::InGameGuiManagerOffset::ActionButtonsList)->InsertAtEnd(element);
			x += dx;
			y += dy;
			++ordinal;
		}
	}

	Write(manager, NC2Layout::InGameGuiManagerOffset::ActionButtonsChecksum, InGameGuiManager_l_Selection_ActionButtons_GetChecksum(manager));
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a10a4.
	void CInGameGuiManager::Selection_ActionButtons_BringUp()
	{
		l_Selection_RemoveAllGuiElements();
		auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
		Write(this, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginX, Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::X));
		Write(this, NC2Layout::InGameGuiManagerOffset::ActionButtonsOriginY, Read<std::int32_t>(mouse, NC2Layout::MouseManagerPositionOffset::Y));
		InGameGuiManager_l_Selection_ActionButtons_BringUp(this);
	}
}

namespace NC2InGameGuiManager
{
	// Original function: FUN_004a480e; action IDs are distinct from network opcodes.
	bool ls_Tool_SendSimpleHumanNetworkCommand(std::uint32_t action, std::int32_t id, std::uint32_t first, std::uint32_t second)
	{
		std::uint8_t opcode;

		switch (action)
		{
			case SwitchIds::HumanGuiAction::Eat:
				opcode = 7;
				break;
			case SwitchIds::HumanGuiAction::Sleep:
				opcode = 8;
				break;
			case SwitchIds::HumanGuiAction::Talk:
				opcode = 9;
				break;
			case SwitchIds::HumanGuiAction::Pray:
				opcode = 10;
				break;
			case SwitchIds::HumanGuiAction::Marry:
				opcode = 13;
				break;
			case SwitchIds::HumanGuiAction::GetBoy:
				opcode = 15;
				break;
			case SwitchIds::HumanGuiAction::GetGirl:
				opcode = 16;
				break;
			case SwitchIds::HumanGuiAction::DetachHomeHouse:
				opcode = 27;
				break;
			case SwitchIds::HumanGuiAction::DetachVehicle:
				opcode = 39;
				break;
			case SwitchIds::HumanGuiAction::DetachWorkHouse:
				opcode = 29;
				break;
			case SwitchIds::HumanGuiAction::DetachLearnHouse:
				opcode = 33;
				break;
			case SwitchIds::HumanGuiAction::DetachBuildHouse:
				opcode = 31;
				break;
			case SwitchIds::HumanGuiAction::ExploreAreaAndReset:
				opcode = 20;
				break;
			case SwitchIds::HumanGuiAction::MilitaryAggressive:
			case SwitchIds::HumanGuiAction::MilitaryDefensive:
			case SwitchIds::HumanGuiAction::MilitaryIgnore:
			case SwitchIds::HumanGuiAction::MilitaryFlee:
				opcode = 41;
				break;
			case SwitchIds::HumanGuiAction::AggressionOn:
			case SwitchIds::HumanGuiAction::AggressionOff:
				opcode = 42;
				break;
			case SwitchIds::HumanGuiAction::GoAway:
				opcode = 5;
				break;
			case SwitchIds::HumanGuiAction::GoHome:
				opcode = 6;
				break;
			case SwitchIds::HumanGuiAction::Cheer:
				opcode = 12;
				break;
			case SwitchIds::HumanGuiAction::Goodie:
				opcode = 11;
				break;
			default:
				return false;
		}

		std::byte command[20]{ };
		Write(command, NC2Layout::UserNetworkCommandOffset::Opcode, opcode);
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, first);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, second);
		NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
		NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, id);

		return true;
	}

	// Human branch of FUN_004a3edb; native input modes remain separate implementation boundaries.
	bool HumanActionButtons_HandleCommand(std::uint32_t action)
	{
		using namespace NC2InGameGuiInputManager;
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		std::uint32_t first = action >= 35 && action <= 38 ? action - 34 : action == 39 ? 1
		                                                                                : 0;
		std::int32_t mode = -1;

		switch (action)
		{
			case SwitchIds::HumanGuiAction::AttackHuman:
				mode = 23;
				break;
			case SwitchIds::HumanGuiAction::AttackHouse:
				mode = 10;
				break;
			case SwitchIds::HumanGuiAction::AttackAnimal:
				mode = 25;
				break;
			case SwitchIds::HumanGuiAction::AttackVehicle:
				mode = 26;
				break;
			case SwitchIds::HumanGuiAction::AttackMapPosition:
				mode = 22;
				break;
		}

		bool remove = true;

		if (mode != -1)
		{
			InputMode_Set(Input(), mode);
		}
		else if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) == 1)
		{
			const auto id = Selection_GetSelectedSingleHumanId(Input());

			switch (action)
			{
				case SwitchIds::HumanGuiAction::GotoAndWait:
					mode = 1;
					break;
				case SwitchIds::HumanGuiAction::AttachHomeHouse:
					mode = 4;
					break;
				case SwitchIds::HumanGuiAction::AttachVehicle:
					mode = 27;
					break;
				case SwitchIds::HumanGuiAction::AttachWorkHouse:
					mode = 5;
					break;
				case SwitchIds::HumanGuiAction::AttachLearnHouse:
					mode = 6;
					break;
				case SwitchIds::HumanGuiAction::AttachBuildHouse:
					mode = 7;
					break;
				case SwitchIds::HumanGuiAction::AttachTradeHouse:
					mode = 8;
					break;
				case SwitchIds::HumanGuiAction::DetachTradeHouse:
					mode = 9;
					break;
				case SwitchIds::HumanGuiAction::ChangeJob:
					manager->MiscWindows_HumanChangeJobWindow_Open();
					break;
				case SwitchIds::HumanGuiAction::ChangeProducedGood:
					manager->MiscWindows_HumanChangeProducedGoodWindow_Open();
					break;
				case SwitchIds::HumanGuiAction::ChangeEquippedGoods:
					manager->MiscWindows_HumanChangeEquippedGoodsWindow_Open(6, -1);
					break;
				case SwitchIds::HumanGuiAction::LearnJob:
					manager->MiscWindows_HumanLearnJobWindow_Open();
					break;
				case SwitchIds::HumanGuiAction::SetWorkCenter:
					mode = 2;
					break;
				case SwitchIds::HumanGuiAction::SetWorkArea:
					mode = 3;
					break;
				case SwitchIds::HumanGuiAction::ExploreArea:
					mode = 13;
					break;
				case SwitchIds::HumanGuiAction::ExploreAreaAndReset:
					remove = ls_Tool_SendSimpleHumanNetworkCommand(32, id, NXBasics::IntegerRange::UInt32Maximum, 1);
					break;
				default:
					remove = ls_Tool_SendSimpleHumanNetworkCommand(action, id, first, 0);
					break;
			}

			if (mode != -1)
				InputMode_Set(Input(), mode);
		}
		else if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) == 2)
		{
			if (action == 24)
			{
				manager->MiscWindows_HumanChangeJobWindow_Open();
			}
			else if (action == 26)
			{
				manager->MiscWindows_HumanChangeEquippedGoodsWindow_Open(6, -1);
			}
			else
			{
				for (std::uint32_t index = 0; index < Selection_GetSelectedMultipleHumanNumber(Input()); ++index)
				{
					remove = ls_Tool_SendSimpleHumanNetworkCommand(action, Selection_GetSelectedMultipleHumanId(Input(), index), first, 0);
				}
			}
		}

		if (remove)
			manager->l_Selection_RemoveAllGuiElements();

		return true;
	}
}