#include "ApplicationGlobals.h"
#include "XTextEncoding.h"
#include "C2DataIds.h"
#include "C2GameControlNetwork.h"
#include "C2GameControlStartup.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticTribeTypeData.h"
#include "NetworkLobbySettings.h"
#include "NetworkCivilizationBuildings.h"
#include "C2StringIO.h"
#include "C2UserMessageAccess.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"
#include "XStringArray.h"

#include <cstring>

namespace
{
	// Complete Windows token tables 004F3558..004F360F, including ignored tokens.
	const NXBasics::SToken gPlayerTokens[] = { { "player", NC2DataIds::GameControlToken::Player },{ "diplomacy", NC2DataIds::GameControlToken::Diplomacy },{ nullptr, 0 } };
	const NXBasics::SToken gMiscTokens[] = {
		{ "nametribe", NC2DataIds::GameControlToken::NameTribe },{ "nametribeshort", NC2DataIds::GameControlToken::NameTribeShort },
		{ "relationnotchangeable", NC2DataIds::GameControlToken::RelationNotChangeable },{ "relationhide", NC2DataIds::GameControlToken::RelationHide },
		{ "relationhidedetails", NC2DataIds::GameControlToken::RelationHideDetails },{ "noseenfirstmessage", NC2DataIds::GameControlToken::NoSeenFirstMessage },
		{ "playerneverdies", NC2DataIds::GameControlToken::PlayerNeverDies },{ "playeroption", NC2DataIds::GameControlToken::PlayerOption },
		{ "playerhideinmenu", NC2DataIds::GameControlToken::PlayerHideInMenu },{ "playerfixcolors", NC2DataIds::GameControlToken::PlayerFixColors },
		{ nullptr, 0 }
	};
	const NXBasics::SToken gAllowedTokens[] = {
		{ "allowhouse", NC2DataIds::GameControlToken::AllowHouse },{ "allowjob", NC2DataIds::GameControlToken::AllowJob },{ "forbidjob", NC2DataIds::GameControlToken::ForbidJob },
		{ "forbidhouse", NC2DataIds::GameControlToken::ForbidHouse },{ "allowgood", NC2DataIds::GameControlToken::AllowGood },{ "forbidgood", NC2DataIds::GameControlToken::ForbidGood },{ nullptr, 0 }
	};
	const NXBasics::SToken gSpecialItemTokens[] = { { "add", NC2DataIds::GameControlToken::Add },{ nullptr, 0 } };
	static_assert(sizeof(gPlayerTokens) == 24);
	static_assert(sizeof(gMiscTokens) == 88);
	static_assert(sizeof(gAllowedTokens) == 56);
	static_assert(sizeof(gSpecialItemTokens) == 16);

	// Original function: FUN_00409a9d.
	int __cdecl DestroyPlayerTokenList()
	{
		return 0;
	}
	// Original function: FUN_00409a9c.
	int __cdecl DestroyMiscTokenList()
	{
		return 0;
	}
	// Original function: FUN_00409a9b.
	int __cdecl DestroyAllowedTokenList()
	{
		return 0;
	}
	// Original function: FUN_00409a9a.
	int __cdecl DestroySpecialItemTokenList()
	{
		return 0;
	}

	bool IsPlayerIndex(std::int32_t player)
	{
		return player >= 0 && player < NC2Runtime::GameControlLimits::Players;
	}
	std::byte* PlayerData(void* gameControl, std::uint32_t player)
	{
		return static_cast<std::byte*>(gameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
	}
}

namespace NC2Logic
{
	// Original function: FUN_0040a005; signed sign testing follows DWORD wrap.
	bool GameControl_Wealth_ChangeAmount(void* gameControl, std::int32_t player, std::int32_t amount)
	{
		using namespace NC2Lifetime;
		if (player >= NC2Runtime::GameControlLimits::Players) return false;
		auto* data = static_cast<std::byte*>(gameControl) + static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) return false;
		const auto changed = Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Wealth) + static_cast<std::uint32_t>(amount);
		if (NXBasics::Runtime_TimeSigned32(changed) < 0) return false;
		Write(data, NC2Layout::GameControlPlayerOffset::Wealth, changed);
		return true;
	}

	// Original function: FUN_00409CE7. The original rejects exactly two, not every value above two.
	bool GameControl_System_PathfinderLimit_IsSearchAllowed(void* gameControl, std::int32_t playerId)
	{
		if (playerId == -1 || playerId == NC2Runtime::GameControlLimits::Players) return true;
		if (playerId < 0 || playerId >= NC2Runtime::GameControlLimits::Players) return false;
		const auto offset = static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::PathfinderStartsThisTickUnsigned;
		const auto count = NC2Lifetime::Read<std::uint32_t>(gameControl, offset);
		if (count == NC2Runtime::GameControlLimits::RejectedPathfinderStartCountUnsigned) return false;
		NC2Lifetime::Write(gameControl, offset, count + 1u);
		return true;
	}

	// Original function: FUN_00409BCC.
	void __fastcall GameControl_System_DoOneGameTick(void* gameControl, void* /*unusedEdx*/, std::uint32_t tick)
	{
		using namespace NC2Lifetime;
		for (std::uint32_t player = 0; player < NC2Runtime::GameControlLimits::Players; ++player) NC2Lifetime::Write<std::uint32_t>(gameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::PathfinderStartsThisTickUnsigned, 0);
		if (tick < NC2Runtime::GameControlTiming::PlayerDeathFirstCheckTickUnsigned || tick % NC2Runtime::GameControlTiming::PlayerDeathCheckIntervalUnsigned != 0) return;
		for (std::uint32_t player = 0; player < NC2Runtime::GameControlLimits::Players; ++player)
		{
			auto* data = PlayerData(gameControl, player);
			if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::DeadFlag) != 0 || Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::NeverDiesFlag) != 0) continue;
			CHumanIterator iterator;
			auto id = iterator.GetFirstHumanId();
			for (; id != -1; id = iterator.GetNextHumanId())
			{
				const auto* human = CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) == player && Human_IsSexMale(human) && Human_IsAgeAdult(human)) break;
			}
			if (id != -1) continue;
			Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::DeadFlag, 1);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(SwitchIds::CallbackType::PlayerDied, true, player, 0, 0);
			std::byte message[NC2Layout::RecordSize::ToUserMessage];
			ToUserMessage_Construct(message);
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, SwitchIds::UserMessage::PlayerDied);
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId, NXBasics::IntegerRange::UInt32Maximum);
			Write(message, NC2Layout::ToUserMessageOffset::RelatedPlayerId, player);
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
		}
	}

	using namespace NC2Lifetime;

	// Original function: FUN_00409a3b. Inlined in the named Mac initializer;
	// the helper name describes the confirmed Windows writes, not an exported symbol.
	void PlayerAllowedThings_MarkHouseAvailable(void* allowedThings, std::uint32_t tribe, std::uint32_t house)
	{
		using NC2Lifetime::Write;
		Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableHouses + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house, 1);
		if (house == 10) Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableGood1Flag + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
		if (house == 11) Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableGood12Flag + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
		if (house == 17)
		{
			Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableGood21Flag + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
			Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableGood9Flag + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
			Write<std::uint8_t>(allowedThings, NC2Layout::PlayerAllowedThingsOffset::AvailableGood10Flag + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
		}
	}

	// Original function: FUN_00408fdd; CGameControl::CGameControl(CIniFileReader*).
	void* __fastcall GameControl_Construct(void* gameControl, void* /*unusedEdx*/, NXBasics::CIniFileReader* reader)
	{
		gGameControl = gameControl;
		std::memset(gameControl, 0, NC2Layout::ObjectSize::GameControl);
		auto& runtime = gGameControlRuntimeStorage;
		if (reader != nullptr)
		{
			if (reader->SearchGroup("playerdata"))
			{
				while (reader->GetNextCommandLineInGroup() != nullptr)
				{
					if ((runtime.tokenInitFlags & 1u) == 0)
					{
						runtime.tokenInitFlags |= 1u;
						runtime.playerTokens = gPlayerTokens;
						NXBasics::Runtime_AtExit(DestroyPlayerTokenList);
					}
					NXBasics::CTokenList tokens(runtime.playerTokens);
					const auto command = tokens.FindToken(reader->GetCurrentName());
					if (command == NC2DataIds::GameControlToken::Player)
					{
						const auto player = reader->GetInt();
						const auto type = reader->GetInt();
						const auto tribe = reader->GetInt();
						const auto color = reader->GetInt();
						if (!IsPlayerIndex(player) || type <= 0 || type >= 3 || tribe <= 0 || tribe >= 8 || color < 0 || color >= 10) continue;
						auto* data = PlayerData(gameControl, static_cast<std::uint32_t>(player));
						Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag, 1);
						Write(data, NC2Layout::GameControlPlayerOffset::Type, type);
						Write(data, NC2Layout::GameControlPlayerOffset::TribeType, tribe);
						Write(data, NC2Layout::GameControlPlayerOffset::ColorId, color);
					}
					else if (command == NC2DataIds::GameControlToken::Diplomacy)
					{
						const auto player = reader->GetInt();
						const auto other = reader->GetInt();
						const auto state = reader->GetInt();
						if (!IsPlayerIndex(player) || !IsPlayerIndex(other) || state < 1 || state > 3) continue;
						auto* data = PlayerData(gameControl, static_cast<std::uint32_t>(player));
						if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint8_t>(PlayerData(gameControl, static_cast<std::uint32_t>(other)), NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
						Write(data, NC2Layout::GameControlPlayerOffset::DiplomacyStatesUnsigned + static_cast<std::uint32_t>(other) * NC2Layout::RecordSize::PlayerDiplomacyStateUnsigned, state);
					}
				}
			}
			if (reader->SearchGroup("playermisc"))
			{
				NXBasics::CStringArray* strings = nullptr;
				while (reader->GetNextCommandLineInGroup() != nullptr)
				{
					if ((runtime.tokenInitFlags & 2u) == 0)
					{
						runtime.tokenInitFlags |= 2u;
						runtime.miscTokens = gMiscTokens;
						NXBasics::Runtime_AtExit(DestroyMiscTokenList);
					}
					NXBasics::CTokenList tokens(runtime.miscTokens);
					const auto command = tokens.FindToken(reader->GetCurrentName());
					if (command == NC2DataIds::GameControlToken::NameTribe || command == NC2DataIds::GameControlToken::NameTribeShort)
					{
						// Retry the original string loader after an earlier null result.
						if (strings == nullptr) strings = StringIO_GetMissionStringArrayPtr("$maproot$");
						if (strings == nullptr) continue;
						const auto player = reader->GetInt();
						if (!IsPlayerIndex(player)) continue;
						auto* data = PlayerData(gameControl, static_cast<std::uint32_t>(player));
						if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
						const char* name = strings->GetStringPtr(static_cast<std::uint32_t>(reader->GetInt()));
						if (name != nullptr) NXBasics::Text_Copy(reinterpret_cast<char*>(data + (command == NC2DataIds::GameControlToken::NameTribe ? NC2Layout::GameControlPlayerOffset::TribeName : NC2Layout::GameControlPlayerOffset::TribeShortName)), NC2Runtime::GameControlLimits::NameCopyCharacters + 1u, name);
					}
					else if (command == NC2DataIds::GameControlToken::PlayerNeverDies)
					{
						const auto player = reader->GetInt();
						if (!IsPlayerIndex(player)) continue;
						auto* data = PlayerData(gameControl, static_cast<std::uint32_t>(player));
						if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::NeverDiesFlag, 1);
					}
					else if (command == NC2DataIds::GameControlToken::RelationNotChangeable || command == NC2DataIds::GameControlToken::RelationHide || command == NC2DataIds::GameControlToken::RelationHideDetails || command == NC2DataIds::GameControlToken::NoSeenFirstMessage)
					{
						const auto player = reader->GetInt();
						const auto other = reader->GetInt();
						if (!IsPlayerIndex(player) || !IsPlayerIndex(other)) continue;
						auto* first = PlayerData(gameControl, static_cast<std::uint32_t>(player));
						auto* second = PlayerData(gameControl, static_cast<std::uint32_t>(other));
						if (Read<std::uint8_t>(first, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint8_t>(second, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
						const std::uint32_t offset = command == NC2DataIds::GameControlToken::RelationNotChangeable ? NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned : command == NC2DataIds::GameControlToken::RelationHide ? NC2Layout::GameControlPlayerOffset::HideDiplomacyChangesUnsigned : command == NC2DataIds::GameControlToken::RelationHideDetails ? NC2Layout::GameControlPlayerOffset::HideDiplomacyDetailsUnsigned : NC2Layout::GameControlPlayerOffset::SuppressPlayerSeenMessagesUnsigned;
						Write<std::uint8_t>(first, offset + static_cast<std::uint32_t>(other), 1);
						Write<std::uint8_t>(second, offset + static_cast<std::uint32_t>(player), 1);
						if (command == NC2DataIds::GameControlToken::RelationHide)
						{
							Write<std::uint8_t>(first, NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned + static_cast<std::uint32_t>(other), 1);
							Write<std::uint8_t>(second, NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned + static_cast<std::uint32_t>(player), 1);
						}
					}
				}
				delete strings;
			}
		}
		else
		{
			Write<std::uint8_t>(gameControl, NC2Layout::GameControlPlayerOffset::ActiveFlag, 1);
			Write<std::uint32_t>(gameControl, NC2Layout::GameControlPlayerOffset::Type, 1);
			Write<std::uint32_t>(gameControl, NC2Layout::GameControlPlayerOffset::TribeType, 1);
			Write<std::uint32_t>(gameControl, NC2Layout::GameControlPlayerOffset::ColorId, 0);
		}

		const auto startupMode = Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::Mode);
		if (startupMode == SwitchIds::GameStartupMode::SingleMap && reader != nullptr && reader->SearchGroup("misc_maptype") && reader->SearchCommand("maptype") && reader->GetInt() == SwitchIds::GameMapType::Multiplayer)
		{
			// Remaster: the single-player frontend controls slot zero; AI fills the other human slots on multiplayer maps.
			for (std::uint32_t player = 1; player < NC2Runtime::GameControlLimits::Players; ++player)
			{
				auto* data = PlayerData(gameControl, player);
				if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 1) Write<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type, 2);
			}
		}
		if (startupMode != SwitchIds::GameStartupMode::None)
		{
			if (startupMode == SwitchIds::GameStartupMode::MultiplayerMap || startupMode == SwitchIds::GameStartupMode::MultiplayerContinue || startupMode == SwitchIds::GameStartupMode::SkirmishMap)
			{
				for (std::uint32_t player = 0; player < NC2Runtime::GameControlLimits::Players; ++player)
				{
					auto* data = PlayerData(gameControl, player);
					const auto type = Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerTypeUnsigned + player * NC2Layout::RecordSize::GameControlStartupPlayerUnsigned);
					Write(data, NC2Layout::GameControlPlayerOffset::Type, type);
					if ((type == 2 && startupMode == SwitchIds::GameStartupMode::MultiplayerMap) || ((type == 1 || type == 2) && startupMode == SwitchIds::GameStartupMode::SkirmishMap)) Write(data, NC2Layout::GameControlPlayerOffset::TribeType, NNetwork::Lobby::ResolveStartupTribe(player, Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::TribeType)));
					Write(data, NC2Layout::GameControlPlayerOffset::ColorId, Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerColorUnsigned + player * NC2Layout::RecordSize::GameControlStartupPlayerUnsigned));
					Write(data, NC2Layout::GameControlPlayerOffset::NetworkPlayerId, Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerNetworkIdUnsigned + player * NC2Layout::RecordSize::GameControlStartupPlayerUnsigned));
					Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag, type == 1 || type == 2);
				}
			}
			Write(gameControl, NC2Layout::GameControlOffset::DifficultyLevel, Read<std::uint32_t>(&gGameControlStartupData, NC2Layout::GameControlStartupOffset::DifficultyLevel));
		}
		if (runtime.pathfinderLimitOverridePending != 0)
		{
			Write(gameControl, NC2Layout::GameControlOffset::DifficultyLevel, runtime.pathfinderLimitOverride);
			runtime.pathfinderLimitOverridePending = 0;
		}
		for (std::uint32_t player = 0; player < NC2Runtime::GameControlLimits::Players; ++player)
		{
			auto* data = PlayerData(gameControl, player);
			if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
			for (std::uint32_t other = 0; other < NC2Runtime::GameControlLimits::Players; ++other)
			{
				if (Read<std::uint8_t>(PlayerData(gameControl, other), NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
				if (Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::DiplomacyStatesUnsigned + other * NC2Layout::RecordSize::PlayerDiplomacyStateUnsigned) == 0) Write<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::DiplomacyStatesUnsigned + other * NC2Layout::RecordSize::PlayerDiplomacyStateUnsigned, player == other ? 4u : 2u);
			}
		}
		GameControl_MiscSystem_PlayerDynamicData_InitStructures(gameControl);

		if (reader != nullptr)
		{
			if (reader->SearchGroup("allowedthings"))
			{
				while (reader->GetNextCommandLineInGroup() != nullptr)
				{
					if ((runtime.tokenInitFlags & 4u) == 0)
					{
						runtime.tokenInitFlags |= 4u;
						runtime.allowedTokens = gAllowedTokens;
						NXBasics::Runtime_AtExit(DestroyAllowedTokenList);
					}
					NXBasics::CTokenList tokens(runtime.allowedTokens);
					const auto command = tokens.FindToken(reader->GetCurrentName());
					if (command != NC2DataIds::GameControlToken::AllowHouse && command != NC2DataIds::GameControlToken::AllowJob && command != NC2DataIds::GameControlToken::ForbidJob && command != NC2DataIds::GameControlToken::ForbidHouse && command != NC2DataIds::GameControlToken::AllowGood && command != NC2DataIds::GameControlToken::ForbidGood) continue;
					const auto player = reader->GetInt();
					const auto tribe = reader->GetInt();
					const auto item = reader->GetInt();
					const bool house = command == NC2DataIds::GameControlToken::AllowHouse || command == NC2DataIds::GameControlToken::ForbidHouse;
					const bool job = command == NC2DataIds::GameControlToken::AllowJob || command == NC2DataIds::GameControlToken::ForbidJob;
					if (!IsPlayerIndex(player) || tribe <= 0 || tribe >= 8 || item < (house ? 1 : 0) || item >= (house ? 55 : job ? 56 : 66)) continue;
					auto* data = PlayerData(gameControl, static_cast<std::uint32_t>(player));
					if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
					if (house && command == NC2DataIds::GameControlToken::AllowHouse)
					{
						PlayerAllowedThings_MarkHouseAvailable(data + NC2Layout::GameControlPlayerOffset::AllowedThings, static_cast<std::uint32_t>(tribe), static_cast<std::uint32_t>(item));
						continue;
					}
					const std::uint32_t stride = house ? NC2Runtime::PlayerAllowanceStride::HousesUnsigned : job ? NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned : NC2Runtime::PlayerAllowanceStride::GoodsUnsigned;
					const std::uint32_t index = static_cast<std::uint32_t>(tribe) * stride + static_cast<std::uint32_t>(item);
					const std::uint32_t allowed = house ? NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned : job ? NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned : NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned;
					const std::uint32_t available = house ? NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned : job ? NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned : NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned;
					if (command == NC2DataIds::GameControlToken::ForbidHouse || command == NC2DataIds::GameControlToken::ForbidJob || command == NC2DataIds::GameControlToken::ForbidGood)
					{
						Write<std::uint8_t>(data, allowed + index, 0);
						Write<std::uint8_t>(data, available + index, 0);
					}
					else if (Read<std::uint8_t>(data, allowed + index) != 0) Write<std::uint8_t>(data, available + index, 1);
				}
			}
			if (reader->SearchGroup("specialitems"))
			{
				while (reader->GetNextCommandLineInGroup() != nullptr)
				{
					if ((runtime.tokenInitFlags & 8u) == 0)
					{
						runtime.tokenInitFlags |= 8u;
						runtime.specialItemTokens = gSpecialItemTokens;
						NXBasics::Runtime_AtExit(DestroySpecialItemTokenList);
					}
					NXBasics::CTokenList tokens(runtime.specialItemTokens);
					if (tokens.FindToken(reader->GetCurrentName()) != NC2DataIds::GameControlToken::Add) continue;
					const auto player = reader->GetInt();
					const auto type = reader->GetInt();
					const auto item = reader->GetInt();
					if (!IsPlayerIndex(player) || type < 0 || type >= 8) continue;
					GameControl_SpecialItem_Add(gameControl, player, static_cast<std::uint32_t>(type), static_cast<std::uint32_t>(item), nullptr);
				}
			}
		}
		CCallbackManager::sTheObjectPtr->RegisterCallback(SwitchIds::CallbackType::PlayerAttackedByPlayer, GameControl_ls_LogicCallback, 0, 10);
		return gameControl;
	}

	// Original function: FUN_00409d42; named Mac initialization method.
	void GameControl_MiscSystem_PlayerDynamicData_InitStructures(void* gameControl)
	{
		for (std::uint32_t player = 0; player < NC2Runtime::GameControlLimits::Players; ++player)
		{
			auto* data = PlayerData(gameControl, player);
			if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
			const auto playerTribe = Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::TribeType);
			const bool expanded = NNetwork::CivilizationBuildings::IsExpandedPlayer(player);
			for (std::uint32_t tribe = 1; tribe < NC2Runtime::PlayerAllowanceSlots::Tribes; ++tribe)
			{
				if (!Tool_AllowedItems_IsTribeTypeAllowed(tribe)) continue;
				const auto& definition = gStaticTribeTypeData[tribe];
				for (std::uint32_t job = 0; job < NC2Runtime::GameControlLimits::Jobs; ++job)
					if (definition.allowedJobs[job] != 0) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job, 1);
				for (std::uint32_t good = 0; good < NC2Runtime::GameControlLimits::GoodsAndEquipment; ++good)
					if (definition.allowedGoods[good] != 0 || (expanded && tribe == playerTribe && gStaticTribeTypeData[1].allowedGoods[good] != 0)) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good, 1);
				// Windows uses a 56-byte tribe stride for 66 equipment flags.
				for (std::uint32_t equipment = 0; equipment < NC2Runtime::GameControlLimits::GoodsAndEquipment; ++equipment)
					if (definition.allowedEquipment[equipment] != 0) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + equipment, 1);
				for (std::uint32_t house = 0; house < NC2Runtime::GameControlLimits::Houses; ++house)
					if (definition.allowedHouses[house] != 0 || (expanded && tribe == playerTribe && NNetwork::CivilizationBuildings::GetHouseId(tribe, house, nullptr) != -1)) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house, 1);
				for (std::uint32_t vehicle = 0; vehicle < NC2Runtime::GameControlLimits::Vehicles; ++vehicle)
					if (definition.allowedVehicles[vehicle] != 0) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedVehiclesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::VehiclesUnsigned + vehicle, 1);
				if (tribe != playerTribe) continue;
				for (std::uint32_t job = 0; job < NC2Runtime::GameControlLimits::Jobs; ++job)
				{
					const auto index = tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job;
					if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + index) == 0 || Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + index) != 0) continue;
					STribeExperienceRequirement requirement;
					std::memcpy(&requirement, &definition.jobNeeds[job], sizeof(requirement));
					bool needsExperience = requirement.requiredExperience != 0 || requirement.jobCount != 0;
					if ((job >= 31 && job <= 41) || Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 2) needsExperience = false;
					if (!needsExperience) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + index, 1);
				}
				for (std::uint32_t house = 0; house < NC2Runtime::GameControlLimits::Houses; ++house)
				{
					const auto index = tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house;
					if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + index) == 0 || Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + index) != 0) continue;
					STribeBuildRequirement requirement;
					std::memcpy(&requirement, &definition.houseBuildNeeds[house], sizeof(requirement));
					bool available = true;
					for (std::uint32_t needed = 0; needed < requirement.jobCount; ++needed)
						if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + static_cast<std::uint32_t>(Read<std::int32_t>(&requirement, NC2Layout::TribeBuildRequirementOffset::JobsUnsigned + needed * NC2Layout::RecordSize::TribeBuildRequirementItemUnsigned))) == 0)
						{
							available = false;
							break;
						}
					if (available)
						for (std::uint32_t needed = 0; needed < requirement.goodCount; ++needed)
							if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + static_cast<std::uint32_t>(Read<std::int32_t>(&requirement, NC2Layout::TribeBuildRequirementOffset::GoodsUnsigned + needed * NC2Layout::RecordSize::TribeBuildRequirementItemUnsigned))) == 0)
							{
								available = false;
								break;
							}
					if (Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 2) available = true;
					if (available) PlayerAllowedThings_MarkHouseAvailable(data + NC2Layout::GameControlPlayerOffset::AllowedThings, tribe, house);
				}
				if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::Good16AllowedFlagUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned) != 0) Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::Good16AvailableFlagUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned, 1);
			}
		}
	}

	// Original function: FUN_0040a612; CGameControl::ls_LogicCallback.
	bool __cdecl GameControl_ls_LogicCallback(TCallbackTypes type, std::uint32_t /*userData*/, std::uint32_t player, std::uint32_t other, std::uint32_t /*param3*/)
	{
		if (type == SwitchIds::CallbackType::PlayerAttackedByPlayer && player != NC2Runtime::GameControlLimits::Players && other != NC2Runtime::GameControlLimits::Players)
		{
			if (GameControl_Diplomacy_GetState(gGameControl, static_cast<std::int32_t>(other), static_cast<std::int32_t>(player)) == 3)
			{
				const auto state = GameControl_Diplomacy_GetState(gGameControl, static_cast<std::int32_t>(player), static_cast<std::int32_t>(other));
				if (state == 2 || (state != 2 && GameControl_Diplomacy_GetState(gGameControl, static_cast<std::int32_t>(player), static_cast<std::int32_t>(other)) == 1)) GameControl_Diplomacy_SetState(gGameControl, nullptr, player, other, 3);
			}
			Write<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + other + NC2Layout::GameControlPlayerOffset::AttackedByPlayersUnsigned, 1);
		}
		return true;
	}
}
