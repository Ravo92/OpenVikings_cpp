#include "ApplicationGlobals.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapIO.h"
#include "C2GameTime.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2PlayerCenterPosition.h"
#include "C2RuntimeConstants.h"
#include "InGameGuiManager.h"
#include "MasterControlProgram.h"
#include "NetworkCivilizationBuildings.h"
#include "MasterPropertyManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_00408f18; Mac CAIManager::EnableAI.
	static void AIManager_EnableAI(void* manager, std::int32_t playerId)
	{
		if (playerId >= NC2Runtime::AIManagerLimits::Players) return;
		const std::uint32_t index = static_cast<std::uint32_t>(playerId);
		if (NC2Lifetime::Read<std::uint8_t>(gGameControl, index * NC2Layout::RecordSize::GameControlPlayerSigned) == 0) return;
		NC2Lifetime::Write<std::uint8_t>(NC2Lifetime::Read<void*>(manager, index * NC2Layout::RecordSize::AIPlayerHandlerPointer), NC2Layout::AISinglePlayerOffset::Enabled, 1);
		NC2Lifetime::Write<std::uint8_t>(NC2Lifetime::Read<void*>(manager, NC2Layout::AIManagerOffset::HighLevelHandlers + index * NC2Layout::RecordSize::AIPlayerHandlerPointer), NC2Layout::AIHighLevelOffset::Enabled, 1);
	}
}

namespace NMasterControlProgram
{
	using namespace NC2Lifetime;

	// Original function: FUN_00401985
	void CMasterControlProgram::GameEngine_StartUpEngine()
	{
		if (mEngineInitialized == 0)
		{
			if (void* game = NXBasics::Runtime_AllocateWithNewHandler(1, 1)) NC2Logic::Game_Construct(game);
			if (void* manager = NXBasics::Runtime_AllocateWithNewHandler(1, 1)) NC2E2::E2Manager_Construct(manager);
			NC2Logic::Misc_EngineIsInitialised();
			mEngineInitialized = 1;
		}

		if (mGameInitialized == 0)
		{
			NC2Logic::Game_StartUp(20, 20, false, nullptr, false);
			mGameInitialized = 1;
		}
	}

	// Original function: FUN_004019da
	void CMasterControlProgram::GameEngine_StartUpGame()
	{
		if (mGameInitialized == 0)
		{
			NC2Logic::Game_StartUp(20, 20, false, nullptr, false);
			mGameInitialized = 1;
		}

		const char* startMap = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetStringValuePtr("start_map");
		if (startMap != nullptr)
		{
			NC2Logic::GameIO_Cleanmap_Load(startMap, true, true, false);
			NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition();
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("start_map");
			return;
		}

		const bool suppressCentering = Read<std::uint32_t>(&mGameControlStartupData, NC2Layout::GameControlStartupOffset::Mode) == 2;
		NC2Logic::gGameControlStartupData.Reset();
		if (Read<std::uint32_t>(&mGameControlStartupData, NC2Layout::GameControlStartupOffset::Mode) != 0)
		{
			std::memcpy(&NC2Logic::gGameControlStartupData, &mGameControlStartupData, sizeof(mGameControlStartupData));
			mGameControlStartupData.Reset();
		}

		std::byte savedStartupData[288];
		std::memcpy(savedStartupData, &NC2Logic::gGameControlStartupData, sizeof(savedStartupData));
		NC2Logic::Game_WorkOnStartUpData();
		std::memcpy(&NC2Logic::gGameControlStartupData, savedStartupData, sizeof(savedStartupData));

		std::int32_t controllingPlayer = 0;
		const std::uint32_t startupType = Read<std::uint32_t>(savedStartupData, NC2Layout::GameControlStartupOffset::Mode);
		if (startupType == SwitchIds::GameStartupMode::MultiplayerMap || startupType == SwitchIds::GameStartupMode::SkirmishMap)
		{
			const std::uint32_t localId = Read<std::uint32_t>(savedStartupData, NC2Layout::GameControlStartupOffset::LocalNetworkPlayerId);
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				if (Read<std::uint32_t>(&NC2Logic::gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerNetworkId + player * NC2Layout::RecordSize::GameControlStartupPlayer) == localId)
				{
					controllingPlayer = static_cast<std::int32_t>(player);
					break;
				}
			}
		}

		if (startupType == SwitchIds::GameStartupMode::MultiplayerContinue)
		{
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				const std::uint32_t type = Read<std::uint32_t>(&NC2Logic::gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerType + player * NC2Layout::RecordSize::GameControlStartupPlayer);
				if (type == 0) continue;
				void* playerData = static_cast<std::byte*>(NC2Logic::gGameControl) + player * NC2Layout::RecordSize::GameControlPlayerSigned;
				if (type == 1)
				{
					Write<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::NetworkPlayerId, Read<std::uint32_t>(&NC2Logic::gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerNetworkId + player * NC2Layout::RecordSize::GameControlStartupPlayer));
					Write<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::Type, 1);
				}
				else if (Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::Type) == 1)
				{
					Write<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::NetworkPlayerId, 0);
					Write<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::Type, 2);
					NC2Logic::AIManager_EnableAI(NC2Logic::gAIManager, static_cast<std::int32_t>(player));
				}
			}
			NC2Logic::GameTime_ResetNetworkSynchronization();
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				if (Read<std::uint32_t>(&NC2Logic::gGameControlStartupData, NC2Layout::GameControlStartupOffset::FirstPlayerNetworkId + player * NC2Layout::RecordSize::GameControlStartupPlayer) == Read<std::uint32_t>(&NC2Logic::gGameControlStartupData, NC2Layout::GameControlStartupOffset::LocalNetworkPlayerId))
				{
					controllingPlayer = static_cast<std::int32_t>(player);
					break;
				}
			}
		}
		if (startupType == SwitchIds::GameStartupMode::Savegame && NC2Logic::gGameStaticVars.networkGame == 0 && NNetwork::CivilizationBuildings::IsActive())
		{
			// Local skirmish saves may put the sole human in a slot other than zero.
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				const auto* data = static_cast<const std::byte*>(NC2Logic::gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
				if (Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 1) { controllingPlayer = static_cast<std::int32_t>(player); break; }
			}
		}

		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->ControllingPlayer_Set(controllingPlayer, true);
		if (!suppressCentering) NC2InGameGuiEntryPoints::C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition();
	}
}
