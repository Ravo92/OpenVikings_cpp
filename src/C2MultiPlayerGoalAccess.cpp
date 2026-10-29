#include "C2ChecksumTools.h"
#include "C2DataIds.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MultiPlayerGoalAccess.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2UserNetworkCommandAccess.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>

namespace
{
	// Complete Windows token region 004F3820..004F3847.
	const NXBasics::SToken gMultiPlayerGoalTokens[] = { { "goalwonwhengoods", NC2DataIds::MultiPlayerGoalToken::GoalWonWhenGoods }, { "goalwonwheninhabitants", NC2DataIds::MultiPlayerGoalToken::GoalWonWhenInhabitants }, { "goalwonbymission", NC2DataIds::MultiPlayerGoalToken::GoalWonByMission }, { "goallostbymission", NC2DataIds::MultiPlayerGoalToken::GoalLostByMission }, { nullptr, 0 } };
	const NXBasics::SToken* gMultiPlayerGoalTokenList = nullptr; // Original 005114F8.
	std::uint8_t gMultiPlayerGoalTokenInitFlags = 0;             // Original 005114FC.
	static_assert(sizeof(gMultiPlayerGoalTokens) == 40);

	// Original function: FUN_00419303; the original exit callback is a bare RET.
	int __cdecl DestroyMultiPlayerGoalTokenList()
	{

		return 0;
	}
}

namespace NC2Logic
{
	// Original function: FUN_004190FD.
	std::uint32_t MultiPlayerGoalManager_Checksum_Calculate(const void* manager)
	{

		return Tool_Checksum_Calculate(manager, NC2Layout::ObjectSize::MultiPlayerGoalManager);
	}

	// Original function: FUN_0041910B; existing goal payloads are not cleared here.
	void __fastcall MultiPlayerGoalManager_IO_LoadGoals(void* manager, void* /*unusedEdx*/, NXBasics::CIniFileReader* reader)
	{
		using namespace NC2Lifetime;
		Write<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::Enabled, gGameStaticVars.mapType == SwitchIds::GameMapType::Multiplayer);

		if (Read<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::Enabled) == 0)
			return;

		Write<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::GoalCount, 1);
		Write<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::FirstGoalType, SwitchIds::MultiplayerGoal::Survive);

		if (!reader->SearchGroup("misc_multiplayer_goals"))
		{

			return;
		}

		while (reader->GetNextCommandLineInGroup() != nullptr)
		{
			if ((gMultiPlayerGoalTokenInitFlags & 1u) == 0)
			{
				gMultiPlayerGoalTokenInitFlags |= 1u;
				gMultiPlayerGoalTokenList = gMultiPlayerGoalTokens;
				NXBasics::Runtime_AtExit(DestroyMultiPlayerGoalTokenList);
			}

			NXBasics::CTokenList tokens(gMultiPlayerGoalTokenList);
			const auto command = tokens.FindToken(reader->GetCurrentName());

			if (command != NC2DataIds::MultiPlayerGoalToken::GoalWonWhenGoods && command != NC2DataIds::MultiPlayerGoalToken::GoalWonWhenInhabitants && command != NC2DataIds::MultiPlayerGoalToken::GoalWonByMission && command != NC2DataIds::MultiPlayerGoalToken::GoalLostByMission)
				continue;

			const auto count = Read<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::GoalCount);

			if (count == NC2Runtime::MultiPlayerGoalLimits::Goals)
				continue;

			auto* goal = static_cast<std::byte*>(manager) + NC2Layout::MultiPlayerGoalManagerOffset::Goals + count * NC2Layout::RecordSize::MultiPlayerGoalUnsigned;

			if (command == NC2DataIds::MultiPlayerGoalToken::GoalWonWhenGoods)
			{
				Write<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type, SwitchIds::MultiplayerGoal::CollectGoods);

				while (reader->GetTypeOfNextParameter() == SwitchIds::IniParameterType::Integer && Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount) != NC2Runtime::MultiPlayerGoalLimits::GoodsRequirements)
				{
					const auto good = reader->GetInt();
					const auto amount = reader->GetInt();
					const auto pair = Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount);
					Write(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstGoodId + pair * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned, good);
					Write(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstRequiredAmount + pair * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned, amount);
					Write(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount, pair + 1u);
				}
			}
			else if (command == NC2DataIds::MultiPlayerGoalToken::GoalWonWhenInhabitants)
			{
				const auto amount = reader->GetInt();
				const auto mode = reader->GetInt();
				Write<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type, SwitchIds::MultiplayerGoal::Population);
				Write(goal, NC2Layout::MultiPlayerPopulationGoalOffset::RequiredCount, amount);
				Write<std::uint8_t>(goal, NC2Layout::MultiPlayerPopulationGoalOffset::SoldiersOnly, mode != 0);
			}
			else
			{
				Write<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type, command == NC2DataIds::MultiPlayerGoalToken::GoalWonByMission ? SwitchIds::MultiplayerGoal::PlayersWin : SwitchIds::MultiplayerGoal::PlayersLose);
			}

			Write(manager, NC2Layout::MultiPlayerGoalManagerOffset::GoalCount, count + 1u);
		}
	}

	// Original function: FUN_0041950B; no player bounds check exists in Windows.
	bool MultiPlayerGoalManager_l_IsWonOrLost(const void* manager, std::uint32_t player)
	{

		return NC2Lifetime::Read<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned) != 0 || NC2Lifetime::Read<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned) != 0;
	}

	// Original function: FUN_00419344; all twenty goal slots are scanned.
	void MultiPlayerGoalManager_l_Goals_Check(void* manager)
	{
		using namespace NC2Lifetime;

		for (std::uint32_t player = 0; player < NC2Runtime::MultiPlayerGoalLimits::Players; ++player)
		{
			if (MultiPlayerGoalManager_l_IsWonOrLost(manager, player))
				continue;

			if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0 || MultiPlayerGoalManager_l_IsWonOrLost(manager, player))
				continue;

			for (std::uint32_t index = 0; index < NC2Runtime::MultiPlayerGoalLimits::Goals; ++index)
			{
				const auto* goal = static_cast<const std::byte*>(manager) + NC2Layout::MultiPlayerGoalManagerOffset::Goals + index * NC2Layout::RecordSize::MultiPlayerGoalUnsigned;
				bool won = false;
				bool lost = false;

				switch (Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type))
				{
					case SwitchIds::MultiplayerGoal::Survive:
						lost = Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::DeadFlag) != 0;
						break;
					case SwitchIds::MultiplayerGoal::CollectGoods:
					{
						std::byte goods[NC2Layout::RecordSize::PlayerGoodsCounts]{ };
						Tool_PlayerStatistics_CountGoods(player, goods);
						std::int32_t pair = 0;

						for (; pair < Read<std::int32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount); ++pair)
						{
							const auto good = Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstGoodId + static_cast<std::uint32_t>(pair) * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned);

							if (Read<std::uint16_t>(goods, NC2Layout::PlayerGoodsCountOffset::Amounts + good * NC2Layout::RecordSize::PlayerGoodCountUnsigned) < Read<std::int32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstRequiredAmount + static_cast<std::uint32_t>(pair) * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned))
								break;
						}

						won = pair == Read<std::int32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount);
						break;
					}
					case SwitchIds::MultiplayerGoal::Population:
					{
						std::uint16_t counts[NC2Runtime::HumanStatisticIndex::Entries]{ };
						Tool_PlayerStatistics_CountHumans(player, counts);
						won = static_cast<std::uint32_t>(counts[Read<std::uint8_t>(goal, NC2Layout::MultiPlayerPopulationGoalOffset::SoldiersOnly) != 0 ? NC2Runtime::HumanStatisticIndex::Soldiers : NC2Runtime::HumanStatisticIndex::All]) >= Read<std::uint32_t>(goal, NC2Layout::MultiPlayerPopulationGoalOffset::RequiredCount);
						break;
					}
					case SwitchIds::MultiplayerGoal::PlayersWin:
						won = Read<std::uint8_t>(goal, NC2Layout::MultiPlayerMissionGoalOffset::PlayerFlags + player) != 0;
						break;
					case SwitchIds::MultiplayerGoal::PlayersLose:
						lost = Read<std::uint8_t>(goal, NC2Layout::MultiPlayerMissionGoalOffset::PlayerFlags + player) != 0;
						break;
				}

				if (won)
				{
					Write<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, 1);
					const auto order = Read<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::NextWinOrder);
					Write(manager, NC2Layout::MultiPlayerGoalManagerOffset::WinOrders + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, order);
					Write(manager, NC2Layout::MultiPlayerGoalManagerOffset::NextWinOrder, order + 1u);
					CCallbackManager::sTheObjectPtr->ExecuteCallback(SwitchIds::CallbackType::PlayerWonOrLost, true, player, 1, 0);
					break;
				}

				if (lost)
				{
					Write<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, 1);
					const auto order = Read<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::NextLossOrder);
					Write(manager, NC2Layout::MultiPlayerGoalManagerOffset::LossOrders + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, order);
					Write(manager, NC2Layout::MultiPlayerGoalManagerOffset::NextLossOrder, order + 1u);
					CCallbackManager::sTheObjectPtr->ExecuteCallback(SwitchIds::CallbackType::PlayerWonOrLost, true, player, 0, 1);
					Write<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::PendingRemovalFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, 1);
					break;
				}
			}
		}
	}

	// Original function: FUN_0041952C; clear pending flags before checking activity.
	void MultiPlayerGoalManager_l_KickFlags_Execute(void* manager)
	{
		using namespace NC2Lifetime;

		for (std::uint32_t player = 0; player < NC2Runtime::MultiPlayerGoalLimits::Players; ++player)
		{
			if (Read<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::PendingRemovalFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned) == 0)
				continue;

			Write<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::PendingRemovalFlags + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned, 0);

			if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0)
				continue;

			std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
			Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::ModifyObjectsAfterPlayerLeaves);
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, player);
			Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, 1);
			UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, command, true, false);
		}
	}

	// Original function: FUN_004195A4; queued departures precede this pass's goals.
	bool __cdecl MultiPlayerGoalManager_l_LogicCallback(unsigned int type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueTick, std::uintptr_t /*param2*/, std::uintptr_t /*param3*/)
	{
		std::uint32_t tick = static_cast<std::uint32_t>(callbackValueTick);

		if (type == SwitchIds::CallbackType::GameTick && tick % NC2Runtime::MultiPlayerGoalTiming::CheckIntervalTicksUnsigned == 0)
		{
			MultiPlayerGoalManager_l_KickFlags_Execute(gMultiPlayerGoalManager);
			MultiPlayerGoalManager_l_Goals_Check(gMultiPlayerGoalManager);
		}

		return true;
	}

	// Original function: FUN_00419304; the player index is unchecked in Windows.
	void MultiPlayerGoalManager_ExternalTrigger_WonByMissionManager(void* manager, std::uint32_t player)
	{
		for (std::uint32_t index = 0; index < NC2Runtime::MultiPlayerGoalLimits::Goals; ++index)
		{
			auto* goal = static_cast<std::byte*>(manager) + NC2Layout::MultiPlayerGoalManagerOffset::Goals + index * NC2Layout::RecordSize::MultiPlayerGoalUnsigned;

			if (NC2Lifetime::Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type) == SwitchIds::MultiplayerGoal::PlayersWin)
				NC2Lifetime::Write<std::uint8_t>(goal, NC2Layout::MultiPlayerMissionGoalOffset::PlayerFlagsUnsigned + player, 1);
		}
	}

	// Original function: FUN_00419324; losing conditions are distinct original records.
	void MultiPlayerGoalManager_ExternalTrigger_LostByMissionManager(void* manager, std::uint32_t player)
	{
		for (std::uint32_t index = 0; index < NC2Runtime::MultiPlayerGoalLimits::Goals; ++index)
		{
			auto* goal = static_cast<std::byte*>(manager) + NC2Layout::MultiPlayerGoalManagerOffset::Goals + index * NC2Layout::RecordSize::MultiPlayerGoalUnsigned;

			if (NC2Lifetime::Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type) == SwitchIds::MultiplayerGoal::PlayersLose)
				NC2Lifetime::Write<std::uint8_t>(goal, NC2Layout::MultiPlayerMissionGoalOffset::PlayerFlagsUnsigned + player, 1);
		}
	}
}