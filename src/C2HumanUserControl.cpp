#include "C2HumanCommandAccess.h"
#include "C2HumanFamily.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanUserControl.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00436A68; Windows command-24 predicate, no separate Mac export established.
	bool Human_UserControl_CanChangeJob(const void* human)
	{

		return (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit6) == 0 && Human_IsSexMale(human) && Human_IsAgeAdult(human);
	}

	// Original function: FUN_00436A97; Windows command-26 predicate, no separate Mac export established.
	bool Human_UserControl_CanChooseJob(const void* human)
	{

		return !(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::JobSelectionEnabledFlag) != 0;
	}

	// Original function: FUN_0040b01f; failed allocation checks leave the output untouched.
	bool Logic_UserControl_GetHumanCommands(std::int32_t id, SUserCommandsForHumanHandler* commands, bool parameter2, bool parameter3)
	{
		const void* human = CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * CHumanArray::ElementSize;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0)
			return false;

		Human_UserControl_GetCommands(human, commands, parameter2, parameter3);

		return true;
	}

	// Original function: FUN_0042c8b8.
	bool Human_IsAbleToHaveWorkHouse(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::CanHaveWorkHouseUnsigned) != 0;
	}

	// Original function: FUN_00434f37; action ids are distinct from network opcodes.
	void Human_UserControl_GetCommands(const void* human, SUserCommandsForHumanHandler* commands, bool parameter2, bool parameter3)
	{
		commands->Init();

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0)
			return;

		const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
		const auto playerTribe = player < 20 ? Read<std::uint32_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeTypeUnsigned) : 1u;
		const bool sameTribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == playerTribe;

		if (!parameter2)
		{
			if (Human_IsJobTrader(human))
			{
				const void* merchant = HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));

				if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) < 2)
					commands->AddCommand(22);

				if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) != 0)
					commands->AddCommand(23);
			}

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
				goto passenger;

			if (Human_IsAttachedToVehicle(human))
				commands->AddCommand(13);

			if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
			{
				if (Human_GetMilitaryMode(human) != 1)
					commands->AddCommand(35);

				if (Human_GetMilitaryMode(human) != 2)
					commands->AddCommand(36);

				if (Human_GetMilitaryMode(human) != 3)
					commands->AddCommand(37);

				if (Human_IsJobSoldier(human))
					commands->AddCommand(Human_IsRefreshingInWorld(human) ? 40 : 39);
			}

			if (Human_IsAgeAdult(human))
			{
				commands->AddCommand(41);

				if (Human_IsSexMale(human) || Human_IsJobHero(human))
				{
					commands->AddCommand(42);
					commands->AddCommand(44);
					commands->AddCommand(43);

					if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
						commands->AddCommand(45);
				}
			}

			commands->AddCommand(2);
			commands->AddCommand(12);

			if (Human_IsJobHero(human))
				return;

			if (Human_UserControl_CanChooseJob(human))
				commands->AddCommand(26);

			commands->AddCommand(3);
			commands->AddCommand(4);

			if (Human_IsAbleToTalk(human))
				commands->AddCommand(5);

			if (Human_IsAbleToPray(human))
				commands->AddCommand(6);

			if (!Human_IsAgeAdult(human))
				return;

			if (sameTribe)
			{
				if (!Human_UserControl_IsCommandTypeInQueue(human, 13) && !Human_UserControl_IsCommandTypeInQueue(human, 14) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 5 && Human_IsAbleToMarryNow(human))
					commands->AddCommand(7);

				if (!Human_UserControl_IsCommandTypeInQueue(human, 15) && !Human_UserControl_IsCommandTypeInQueue(human, 16) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 11 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 12 && Human_IsAbleToGetPregnant(human))
				{
					commands->AddCommand(8);
					commands->AddCommand(9);
				}
			}

			commands->AddCommand(10);

			if (Human_IsAttachedToHomeHouse(human))
				commands->AddCommand(11);

			if (sameTribe && Human_UserControl_CanChangeJob(human))
				commands->AddCommand(24);

			if (Human_IsAbleToHaveWorkHouse(human))
			{
				if (Human_UserControl_GetNextJobType(human) == 7)
				{
					commands->AddCommand(20);
				}
				else if (Human_UserControl_GetNextJobType(human) != 25)
				{
					commands->AddCommand(16);
				}
			}

			if (Human_IsAttachedToWorkHouse(human))
			{
				if (Human_IsAttachedToLearnHouse(human))
				{
					commands->AddCommand(19);
				}
				else if (Human_UserControl_GetNextJobType(human) == 7)
				{
					commands->AddCommand(21);
				}
				else if (Human_UserControl_GetNextJobType(human) != 25)
				{
					commands->AddCommand(17);
				}
			}

			if (sameTribe && Human_IsAgeAdult(human) && Human_IsSexMale(human))
				commands->AddCommand(18);

			if (Human_WorkCenterMarker_IsAllowed(human) && !Human_IsJobSoldier(human) && !Human_IsJobBuilder(human))
			{
				commands->AddCommand(28);
				commands->AddCommand(29);
			}

			if (Human_UserControl_GetNextJobType(human) == 27)
			{
				commands->AddCommand(31);
				commands->AddCommand(32);
			}

			return;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
		passenger:
			const void* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if ((type != 3 && type != 4) || Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0)
				commands->AddCommand(13);

			return;
		}

		if (Human_IsAttachedToVehicle(human))
			commands->AddCommand(13);

		commands->AddCommand(12);

		if (Human_IsAgeAdult(human))
		{
			commands->AddCommand(41);

			if (Human_IsSexMale(human) || Human_IsJobHero(human))
			{
				commands->AddCommand(42);
				commands->AddCommand(44);
				commands->AddCommand(43);

				if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
					commands->AddCommand(45);
			}
		}

		if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
		{
			commands->AddCommand(35);
			commands->AddCommand(36);
			commands->AddCommand(37);
		}

		if (Human_IsJobSoldier(human))
		{
			commands->AddCommand(39);
			commands->AddCommand(40);
		}

		if (Human_IsJobHero(human))
			return;

		commands->AddCommand(3);
		commands->AddCommand(4);

		if (Human_IsAbleToTalk(human))
			commands->AddCommand(5);

		if (Human_IsAbleToPray(human))
			commands->AddCommand(6);

		if (Human_UserControl_CanChooseJob(human))
			commands->AddCommand(26);

		if (sameTribe && Human_UserControl_CanChangeJob(human))
			commands->AddCommand(24);

		if (Human_IsAgeAdult(human) && sameTribe)
		{
			if (!Human_UserControl_IsCommandTypeInQueue(human, 13) && !Human_UserControl_IsCommandTypeInQueue(human, 14) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 5 && Human_IsAbleToMarryNow(human))
				commands->AddCommand(7);
			// Windows 0043550B repeats opcode 15; it does not check opcode 16 here.

			if (!Human_UserControl_IsCommandTypeInQueue(human, 15) && !Human_UserControl_IsCommandTypeInQueue(human, 15) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 11 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 12 && Human_IsAbleToGetPregnant(human))
			{
				commands->AddCommand(8);
				commands->AddCommand(9);
			}
		}

		if (!parameter3 || !Human_IsAgeAdult(human))
		{

			return;
		}

		commands->AddCommand(10);

		if (Human_IsAbleToHaveWorkHouse(human))
		{
			if (Human_UserControl_GetNextJobType(human) == 7)
			{
				commands->AddCommand(20);
			}
			else if (Human_UserControl_GetNextJobType(human) != 25)
			{
				commands->AddCommand(16);
			}
		}

		if (sameTribe && Human_IsSexMale(human))
			commands->AddCommand(18);
	}

	// Original function: FUN_0044071b.
	void SUserCommandsForHumanHandler::Init()
	{
		count = 0;
		std::memset(commands, 0, sizeof(commands));
	}

	// Original function: FUN_0044072b; capacity is checked for equality, duplicates are retained.
	void SUserCommandsForHumanHandler::AddCommand(std::uint32_t command)
	{
		if (count != 25)
			commands[count++] = command;
	}

	// Original function: FUN_0044073f.
	bool SUserCommandsForHumanHandler::IsCommandAllowed(std::int32_t command) const
	{
		for (std::int32_t i = 0; i < count; ++i)
		{
			if (commands[i] == static_cast<std::uint32_t>(command))
				return true;
		}

		return false;
	}

	// Original function: FUN_00440765; descending unsigned priority, descending signed id on ties.
	void SUserCommandsForHumanHandler::SortCommandsByPriority()
	{
		if (count <= 1)
		{

			return;
		}

		std::uint32_t priorities[25];

		for (std::int32_t i = 0; i < count; ++i)
		{
			priorities[i] = Tool_UserHumanCommandTypes_GetPriority(commands[i]);
		}

		for (std::int32_t pass = 0; pass < count - 1; ++pass)
		{
			for (std::int32_t i = 0; i < count - 1; ++i)
			{
				if (priorities[i] < priorities[i + 1] || (priorities[i] == priorities[i + 1] && NXBasics::Runtime_TimeSigned32(commands[i]) < NXBasics::Runtime_TimeSigned32(commands[i + 1])))
				{
					const auto priority = priorities[i], command = commands[i];
					priorities[i] = priorities[i + 1];
					priorities[i + 1] = priority;
					commands[i] = commands[i + 1];
					commands[i + 1] = command;
				}
			}
		}
	}

	// Original function: FUN_00440808.
	std::uint32_t Tool_UserHumanCommandTypes_GetPriority(std::uint32_t command)
	{
		const auto value = NXBasics::Runtime_TimeSigned32(command);
		auto priority = command;

		if (value > 34 && value < 46)
			priority = command + 400u;

		if (value > 15 && value < 33)
			priority += 300u;

		if (value > 1 && value < 14)
			priority += 200u;

		if (value > 47 && value < 56)
			priority += 100u;

		return priority + Tool_UserHumanCommandTypes_GetGroupType(value) * 1000u;
	}

	// Original function: FUN_0044085d.
	std::uint32_t Tool_UserHumanCommandTypes_GetGroupType(std::int32_t command)
	{
		if (command <= 32)
		{
			if (command >= 31)
				return 1;

			if (command < 10)
				return 0;

			if (command <= 13)
				return 2;

			if (command <= 15)
				return 0;

			if (command <= 23)
				return 2;

			if (command <= 29)
				return 1;

			return 0;
		}

		if (command < 35)
			return 0;

		if (command <= 40)
			return 4;

		return command <= 45 ? 3 : 0;
	}
}