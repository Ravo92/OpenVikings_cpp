#include "C2EngineObjectData.h"
#include "C2GameControlNetwork.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StringAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NC2InGameGuiMessages
{
	// Original function: FUN_004d21de.
	void Tool_CreateMessageString(const void* message, char* output, std::uint32_t capacity)
	{
		using namespace NC2Lifetime;
		using namespace NC2Logic;
		using NXBasics::Runtime_PrintString;
		const auto text = NC2GuiToolsBase::StringTool_GetGameGuiMessagesStringPtr;
		std::strcpy(output, "<UNKNOWN MESSAGE>");
		char name[200];
		std::strcpy(name, "<NAMELESS>");
		const auto humanId = Read<std::int32_t>(message, NC2Layout::ToUserMessageOffset::HumanId);

		if (humanId != -1)
		{
			const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanId) * CHumanArray::ElementSize;
			std::strcpy(name, reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries + static_cast<std::uint32_t>(humanId) * NC2Layout::RecordSize::DynamicHumanUnsigned + NC2Layout::DynamicHumanOffset::NameUnsigned));

			if (Human_IsAgeAdult(human) && Human_IsSexMale(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 0 && !Human_IsJobHero(human) && !(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32))
			{
				std::strcat(name, " (");
				std::strcat(name, StringAccess_GetJobNamePtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 1));
				std::strcat(name, ")");
			}
		}

		const auto type = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type);
		// Game.exe jump table 004d2a31..004d2b2c, recovered as one whole region.
		static constexpr std::uint32_t simpleText[47]{ 0, 15, 16, 18, 0, 0, 0, 22, 23, 24, 25, 17, 0, 0, 29, 30, 31, 32, 0, 0, 0, 0, 0, 0, 0, 44, 0, 46, 10, 12, 13, 14, 11, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 0, 0, 0, 61 };

		if (type < 47 && simpleText[type] != 0)
		{
			Runtime_PrintString(output, "%s %s", name, text(simpleText[type]));
			return;
		}

		const auto good = [&]()
		{

			return StringAccess_GetGoodNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem), 1);
		};

		switch (type)
		{
			case SwitchIds::UserMessage::CannotFindGood:
			case SwitchIds::UserMessage::CannotFindBuildGood:
			case SwitchIds::UserMessage::CannotFindHomeGood:
				Runtime_PrintString(output, "%s %s", name, text(type + 15));

				if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem) != 0)
				{
					std::strcat(output, " - ");
					std::strcat(output, good());
				}

				return;
			case SwitchIds::UserMessage::MustWaitForGood:
				Runtime_PrintString(output, "%s %s - %s", name, text(26), good());
				return;
			case SwitchIds::UserMessage::ProductionStockFull:
				if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem) == 0)
				{
					Runtime_PrintString(output, "%s %s", name, text(28));
				}
				else
				{
					char format[200];
					Runtime_PrintString(format, "%%s %s", text(27));
					Runtime_PrintString(output, format, name, good());
				}

				return;
			case SwitchIds::UserMessage::ExperienceUnlocks:
			{
				Runtime_PrintString(output, "%s %s:", name, text(33));
				const auto limit = capacity - 40u;
				const std::uint32_t offsets[4]{ 0x138, 0x2C, 0x1FC, 0x2DC };

				for (std::uint32_t category = 0; category < 4; ++category)
				{
					const auto offset = offsets[category], count = Read<std::uint32_t>(message, offset);

					if (count == 0)
						continue;

					std::strcat(output, "\r\n");
					std::strcat(output, text(34u + category));
					std::strcat(output, ": ");

					for (std::uint32_t i = 0; i < count; ++i)
					{
						if (i != 0)
							std::strcat(output, " - ");

						const auto id = Read<std::uint32_t>(message, offset + NC2Layout::CountedArrayOffset::EntriesUnsigned + i * NC2Layout::RecordSize::UserMessageItemUnsigned);
						const auto* item = category == 0 ? StringAccess_GetJobNamePtr(id, 1) : category == 1 ? StringAccess_GetGoodNamePtr(id, 1)
						                                                                   : category == 2   ? StringAccess_GetHouseNamePtr(id, 1)
						                                                                                     : StringAccess_GetVehicleTypeNamePtr(id);
						std::strcat(output, item);

						if (std::strlen(output) >= limit)
						{
							std::strcat(output, "...");
							return;
						}
					}
				}

				return;
			}
			case SwitchIds::UserMessage::GoodProductionUnlocked:
			case SwitchIds::UserMessage::EquipmentUnlocked:
				Runtime_PrintString(output, "%s %s - %s", name, text(type == 19 ? 38 : 42), good());
				return;
			case SwitchIds::UserMessage::JobUnlocked:
				Runtime_PrintString(output, "%s %s - %s", name, text(39), StringAccess_GetJobNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::JobItems), 1));
				return;
			case SwitchIds::UserMessage::HouseUnlocked:
				Runtime_PrintString(output, "%s %s - %s", name, text(40), StringAccess_GetHouseNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseItems), 1));
				return;
			case SwitchIds::UserMessage::VehicleUnlocked:
			case SwitchIds::UserMessage::CannotEnterVehicle:
				Runtime_PrintString(output, "%s %s - %s", name, text(type == 22 ? 41 : 57), StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::EquipmentItems)));
				return;
			case SwitchIds::UserMessage::ProductionAmountCompleted:
			case SwitchIds::UserMessage::CannotProduceWantedGood:
				Runtime_PrintString(output, "%s %s %s", name, text(type == 24 ? 43 : 45), good());
				return;
			case SwitchIds::UserMessage::CannotFindEquipment:
			case SwitchIds::UserMessage::BackpackFull:
				Runtime_PrintString(output, "%s %s: %s - %s", name, text(58), good(), text(type == 44 ? 59 : 60));
				return;
			case SwitchIds::UserMessage::HouseFinished:
			case SwitchIds::UserMessage::HouseUpgradeFinished:
			{
				const auto* house = CHouseArray::mStaticVars + Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseId) * CHouseArray::ElementSize;
				Runtime_PrintString(output, "%s %s", StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1), text(type == 47 ? 90 : 91));
				return;
			}
			case SwitchIds::UserMessage::HouseAttacked:
				Runtime_PrintString(output, "%s %s", StringAccess_GetHouseNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseItems), 1), text(92));
				return;
			case SwitchIds::UserMessage::VehicleNoWay:
			case SwitchIds::UserMessage::VehicleNoCommander:
			case SwitchIds::UserMessage::VehicleAttacked:
			case SwitchIds::UserMessage::VehicleNoDraggingAnimal:
			case SwitchIds::UserMessage::VehicleNoPassengerSpace:
			case SwitchIds::UserMessage::VehicleCannotAttach:
			case SwitchIds::UserMessage::VehicleCannotReachShip:
			case SwitchIds::UserMessage::VehicleCannotDetach:
			case SwitchIds::UserMessage::VehicleNeedsCarrier:
				Runtime_PrintString(output, "%s %s", StringAccess_GetVehicleTypeNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::EquipmentItems)), text(type + 50));
				return;
			case SwitchIds::UserMessage::HeroDied:
			{
				const auto* dead = static_cast<NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager*>(NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager)->DeadHuman_GetDataPtrByHumanUniqueId(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanUniqueId));

				if (dead != nullptr)
				{
					Runtime_PrintString(output, "%s %s", static_cast<const char*>(dead) + NC2Layout::DeadHumanRecordOffset::Name, text(120));
				}
				else
				{
					std::strcpy(output, text(121));
				}

				return;
			}
			case SwitchIds::UserMessage::PlayerSeen:
			case SwitchIds::UserMessage::DiplomacyChanged:
			case SwitchIds::UserMessage::PlayerDied:
			{
				const auto player = Read<std::int32_t>(message, NC2Layout::ToUserMessageOffset::RelatedPlayerId);
				const auto* playerName = GameControl_Names_GetPlayerTribeNamePtr(gGameControl, player);
				char tribeName[100];

				if (playerName != nullptr)
				{
					std::strcpy(tribeName, playerName);
				}
				else
				{
					Runtime_PrintString(tribeName, "%s %d", text(130), player);
				}

				if (type == 62)
				{
					Runtime_PrintString(output, "%s %s", tribeName, text(133));
				}
				else
				{
					const auto state = GameControl_Diplomacy_GetState(gGameControl, player, Read<std::int32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId));
					const auto* stateName = NC2GuiToolsBase::StringTool_GetLogicDiplomacyStateNamePtr(static_cast<std::uint32_t>(state));
					Runtime_PrintString(output, "%s %s %s", tribeName, text(type + 71u), stateName);
				}

				return;
			}
			case SwitchIds::UserMessage::SpecialItemFound:
			{
				char item[200];
				NC2GuiToolsBase::StringTool_GetLogicSpecialItemNamePtr(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::SpecialItemType), Read<std::int32_t>(message, NC2Layout::ToUserMessageOffset::SpecialItem), item);
				Runtime_PrintString(output, "%s - %s", text(134), item);
				return;
			}
			default:

				return;
		}
	}
}