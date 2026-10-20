#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "C2UserMessageAccess.h"
#include "C2UserNetworkCommandAccess.h"
#include "C2VehicleAccess.h"
#include "NetworkShutdown.h"
#include "NetworkCivilizationBuildings.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_0040A295.
	void __fastcall GameControl_Player_LeavesGame(void* gameControl, void* /*unusedEdx*/, unsigned int player, bool originalFlag, bool removeObjects)
	{
		using namespace NC2Lifetime;
		const auto offset = player * NC2Layout::RecordSize::GameControlPlayer;
		if (gGameStaticVars.networkGame != 0 && Read<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type) == 1 && Read<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::NetworkPlayerId) != 0)
		{
			Write<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::NetworkPlayerId, 0);
			Write<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type, 2);
		}
		GameControl_Player_LeftGameModifyObjects(gameControl, nullptr, player, originalFlag, removeObjects);
	}

	// Original function: FUN_0040A2D4; the middle flag is unused by Windows.
	void __fastcall GameControl_Player_LeftGameModifyObjects(void* /*gameControl*/, void* /*unusedEdx*/, unsigned int player, bool /*originalFlag*/, bool removeObjects)
	{
		using namespace NC2Lifetime;
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDestructionLandscapeFlag, 0);
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag, 1);
		if (removeObjects)
		{
			for (std::int32_t id = 0; id < CHouseArray::sTheObjectPtr->mCapacity; ++id)
			{
				const auto* house = CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == player) CHouseArray::sTheObjectPtr->FreeHouse(id);
			}
			for (std::int32_t id = 0; id < CHumanArray::sTheObjectPtr->mCapacity; ++id)
			{
				const auto* human = CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != player) continue;
				std::uint32_t position = 0;
				bool createAnimal = false;
				const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 && MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human))) == 1)
				{
					createAnimal = true;
					position = Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
				}
				CHumanArray::sTheObjectPtr->FreeHuman(id);
				if (createAnimal) CAnimalArray::sTheObjectPtr->AllocateAnimal(20, 0x1E, 0x31, &position, 0, 0);
			}
			for (std::int32_t id = 0; id < CAnimalArray::sTheObjectPtr->mCapacity; ++id)
			{
				auto* animal = CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize;
				if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) == player) Animal_ChangePlayerId(animal, 20);
			}
			for (std::int32_t id = 0; id < CVehicleArray::sTheObjectPtr->mCapacity; ++id)
			{
				const auto* vehicle = CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize;
				if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) == player) CVehicleArray::sTheObjectPtr->FreeVehicle(id);
			}
			CGuideIterator iterator;
			for (auto id = iterator.GetFirstGuideId(player); id != -1; id = iterator.GetNextGuideId()) GuideManager_RemoveGuide(gGuideManager, nullptr, static_cast<std::uint32_t>(id));
		}
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag, 0);
		Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDestructionLandscapeFlag, 0);
	}

	// Original function: FUN_0040a263.
	const char* GameControl_Names_GetPlayerTribeShortNamePtr(const void* gameControl, std::int32_t player)
	{
		if (player >= 20) return nullptr;
		const auto offset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (NC2Lifetime::Read<std::uint8_t>(gameControl, offset) == 0) return nullptr;
		const auto* name = static_cast<const char*>(gameControl) + offset + 0x49E0u;
		return std::strlen(name) != 0 ? name : nullptr;
	}

	// Original function: FUN_0040a58c; the network name takes precedence over tribe names.
	void GameControl_Misc_GetPlayerName(const void* gameControl, std::int32_t player, bool shortName, char* output)
	{
		if (GameControl_Player_IsNetworkPlayer(gameControl, player) && NNetwork::Network_GetPlayerName(NC2Lifetime::Read<std::uint32_t>(gameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::NetworkPlayerIdUnsigned), output)) return;
		const auto* name = shortName ? GameControl_Names_GetPlayerTribeShortNamePtr(gameControl, player) : GameControl_Names_GetPlayerTribeNamePtr(gameControl, player);
		if (name != nullptr) std::strcpy(output, name);
		else if (shortName) NXBasics::Runtime_PrintString(output, "P%d", player + 1);
		else NXBasics::Runtime_PrintString(output, "%s %d", StringAccess_GetMiscStringPtr(0), player + 1);
	}
	// Original function: FUN_0040b13a; same player checks and matrix access as 0040A12B.
	std::int32_t Logic_EntryPoint_GetDiplomacyState(std::int32_t player, std::int32_t otherPlayer)
	{
		return GameControl_Diplomacy_GetState(gGameControl, player, otherPlayer);
	}

	// Original function: FUN_0040a231. The signed upper bound has no lower check.
	const char* GameControl_Names_GetPlayerTribeNamePtr(const void* gameControl, std::int32_t player)
	{
		if (player >= 20) return nullptr;
		const auto offset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (NC2Lifetime::Read<std::uint8_t>(gameControl, offset) == 0) return nullptr;
		const auto* name = static_cast<const char*>(gameControl) + offset + 0x49ACu;
		return std::strlen(name) != 0 ? name : nullptr;
	}
	// Original function: FUN_0040a034; CGameControl::SpecialItem_Add.
	bool GameControl_SpecialItem_Add(void* gameControl, std::int32_t player, std::uint32_t type, std::uint32_t item, const void* position)
	{
		using namespace NC2Lifetime;
		if (player >= 20) return false;
		const std::uint32_t base = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (Read<std::uint8_t>(gameControl, base) == 0) return false;
		for (std::uint32_t index = 0; index < 100; ++index)
		{
			const std::uint32_t offset = base + 0x4624u + index * 8u;
			if (Read<std::uint32_t>(gameControl, offset) != 0) continue;
			Write(gameControl, offset, type);
			Write(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type, item);
			std::byte message[772];
			ToUserMessage_Construct(message);
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 0x3F);
			Write(message, NC2Layout::ToUserMessageOffset::PlayerId, player);
			Write(message, NC2Layout::ToUserMessageOffset::SpecialItemType, type);
			Write(message, NC2Layout::ToUserMessageOffset::SpecialItem, item);
			if (position != nullptr) Write(message, NC2Layout::ToUserMessageOffset::MapPositionX, Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates));
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
			return true;
		}
		return false;
	}

	// Original function: FUN_0040a0d7; matching entries are consumed immediately.
	bool GameControl_SpecialItem_Use(void* gameControl, std::int32_t player, std::uint32_t type, std::uint32_t item)
	{
		using namespace NC2Lifetime;
		if (player >= 20) return false;
		const auto base = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (Read<std::uint8_t>(gameControl, base) == 0) return false;
		for (std::uint32_t index = 0; index < 100; ++index)
		{
			const auto offset = base + 0x4624u + index * 8u;
			if (Read<std::uint32_t>(gameControl, offset) != type || Read<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type) != item) continue;
			Write<std::uint32_t>(gameControl, offset, 0);
			Write<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type, 0);
			return true;
		}
		return false;
	}
	// Original function: FUN_0040a4da; zero network IDs never match.
	std::int32_t GameControl_GetPlayerByNetworkId(const void* gameControl, std::uint32_t networkId)
	{
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			const auto id = NC2Lifetime::Read<std::uint32_t>(gameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::NetworkPlayerIdUnsigned);
			if (id != 0 && id == networkId) return static_cast<std::int32_t>(player);
		}
		return -1;
	}

	// Original function: FUN_0040b250.
	std::int32_t GameTime_Network_GetPlayerByNetworkId(std::uint32_t networkId)
	{
		return gGameStaticVars.networkGame != 0 ? GameControl_GetPlayerByNetworkId(gGameControl, networkId) : -1;
	}
	// Original function: FUN_0040a55f; the signed upper bound has no lower-bound check.
	bool GameControl_Player_IsNetworkPlayer(const void* gameControl, std::int32_t player)
	{
		if (gGameStaticVars.networkGame == 0 || player >= 20) return false;
		const auto offset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		return NC2Lifetime::Read<std::uint8_t>(gameControl, offset) != 0 && NC2Lifetime::Read<std::uint32_t>(gameControl, offset + NC2Layout::GameControlPlayerOffset::Type) == 1;
	}
	bool Remaster_GameControl_Player_HasDifficultyBenefits(const void* gameControl, std::uint32_t player)
	{
		if (player >= 20) return false;
		if (gGameStaticVars.networkGame == 0 && NNetwork::CivilizationBuildings::IsActive())
		{
			const auto* data = static_cast<const std::byte*>(gameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
			return NC2Lifetime::Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && NC2Lifetime::Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 1;
		}
		return player == 0 || GameControl_Player_IsNetworkPlayer(gameControl, static_cast<std::int32_t>(player));
	}
	// Original function: FUN_0040a12b; CGameControl::Diplomacy_GetState.
	std::int32_t GameControl_Diplomacy_GetState(const void* gameControl, std::int32_t player, std::int32_t otherPlayer)
	{
		if (player >= 20 || NC2Lifetime::Read<std::uint8_t>(gameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0) return 0;
		if (otherPlayer >= 20 || NC2Lifetime::Read<std::uint8_t>(gameControl, static_cast<std::uint32_t>(otherPlayer) * NC2Layout::RecordSize::GameControlPlayer) == 0) return 0;
		return NC2Lifetime::Read<std::int32_t>(gameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + static_cast<std::uint32_t>(otherPlayer) * NC2Layout::RecordSize::DiplomacyStateUnsigned + NC2Layout::GameControlPlayerOffset::DiplomacyStatesUnsigned);
	}
	// Original function: FUN_0040a171; CGameControl::Diplomacy_SetState.
	bool __fastcall GameControl_Diplomacy_SetState(void* gameControl, void* /*unusedEdx*/, std::uint32_t player, std::uint32_t otherPlayer, std::uint32_t state)
	{
		using namespace NC2Lifetime;
		// Signed upper bounds deliberately retain the original missing lower checks.
		if (static_cast<std::int32_t>(player) >= 20 || Read<std::uint8_t>(gameControl, player * NC2Layout::RecordSize::GameControlPlayer) == 0) return false;
		if (static_cast<std::int32_t>(otherPlayer) >= 20 || Read<std::uint8_t>(gameControl, otherPlayer * NC2Layout::RecordSize::GameControlPlayer) == 0) return false;
		const std::uint32_t offset = player * NC2Layout::RecordSize::GameControlPlayer + otherPlayer * 4u + 0x45A8u;
		if (Read<std::uint32_t>(gameControl, offset) == state) return true;
		Write(gameControl, offset, state);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1A, true, player, otherPlayer, state);
		if (Read<std::uint8_t>(gameControl, player * NC2Layout::RecordSize::GameControlPlayer + otherPlayer + NC2Layout::GameControlPlayerOffset::HideDiplomacyChangesUnsigned) == 0)
		{
			std::byte message[772];
			ToUserMessage_Construct(message);
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 0x3D);
			Write(message, NC2Layout::ToUserMessageOffset::PlayerId, otherPlayer);
			Write(message, NC2Layout::ToUserMessageOffset::RelatedPlayerId, player);
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
		}
		return true;
	}


	// Original function: FUN_0040a4fd; Windows network-ID command adapter.
	// The Mac Player_LeavesGame name does not identify this Windows-only adapter.
	void GameControl_QueueNetworkPlayerDeparture(void* gameControl, std::uint32_t networkPlayerId, bool originalFlag)
	{
		(void) originalFlag; // The original stack argument is never read.
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			const auto id = NC2Lifetime::Read<std::uint32_t>(gameControl, NC2Layout::GameControlPlayerOffset::NetworkPlayerIdUnsigned + player * NC2Layout::RecordSize::GameControlPlayer);
			if (id == 0 || id != networkPlayerId) continue;
			std::byte command[20]{ };
			NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, 0);
			NC2Lifetime::Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, 1);
			command[0] = std::byte{ SwitchIds::UserCommand::PlayerLeavesGame };
			NC2Lifetime::Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, player);
			UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, command, true, false);
			return;
		}
	}
}
