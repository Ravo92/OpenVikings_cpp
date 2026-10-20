#include "C2CallbackManager.h"
#include "C2DelayedActionManager.h"
#include "C2GameChecksum.h"
#include "C2GameState.h"
#include "C2GameTime.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapMoveableAccess.h"
#include "C2MapResourceManagers.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2SavegameIO.h"
#include "C2UserNetworkCommandAccess.h"
#include "NetworkShutdown.h"
#include "ModernNetwork.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"

#include <algorithm>
#include <cstring>
#include <Windows.h>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Intentional network modernization: host policy is local; only its frequency command is synchronized.
	static std::uint32_t gSynchronizationPoll = 0, gSynchronizationBudget = 0, gPendingFrequency = 0, gFrequencyCandidate = 0, gCandidateSamples = 0;

	std::uint32_t GameTime_Network_GetSynchronizationBudget() { return gSynchronizationBudget; }

	std::uint32_t GameTime_Network_GetTicksPerCycle()
	{
		const auto frequency = (std::clamp)(gGameTime.networkFrequency, 1u, 12u);
		const auto speed = gGameTime.speed >= 0.01 ? static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(gGameTime.speed)) : 0u;
		return (std::max)(1u, speed / frequency);
	}

	static void UpdateAutomaticSynchronization(std::uint32_t now)
	{
		gGameTime.autoAdjustNetworkFrequency = 1;
		if (!NNetwork::Network_IsHost() || now - gSynchronizationPoll < 1000u) return;
		gSynchronizationPoll = now;
		if (gPendingFrequency != 0)
		{
			if (gGameTime.networkFrequency != gPendingFrequency) return;
			gPendingFrequency = 0;
		}
		std::uint32_t first = 0, second = 0, highestLatency = 0;
		const auto* players = static_cast<const NNetwork::SPlayerInfo*>(gDynamicNetworkData.playerInfoRecords);
		auto* peer = GetNetworkInstance()->peer;
		bool missing = false;
		for (std::uint32_t index = 0; index < gDynamicNetworkData.playerCount; ++index)
		{
			if (players[index].playerId == 0 || players[index].isLocal) continue;
			NNetwork::ModernConnectionQuality quality;
			if (peer == nullptr || !peer->ConnectionQuality(players[index].playerId, quality) || !quality.measured || quality.age >= 3000u) { missing = true; continue; }
			highestLatency = (std::max)(highestLatency, quality.latency);
			std::uint32_t peak = quality.latency;
			for (auto sample = quality.count > 8 ? quality.count - 8 : 0; sample < quality.count; ++sample)
				if (quality.samples[sample] != UINT32_MAX) peak = (std::max)(peak, quality.samples[sample]);
			const auto budget = peak + quality.jitter * 2u;
			if (budget >= first) { second = first; first = budget; } else second = (std::max)(second, budget);
		}
		// Client-to-client commands travel through the host: reserve both slowest legs.
		gSynchronizationBudget = missing ? 1000u : first == 0 && second == 0 ? 0u : first + second + 20u;
		const auto sampleIndex = gGameTime.latencySampleIndex % 10u;
		gGameTime.latencySamples[sampleIndex] = highestLatency;
		gGameTime.latencySampleIndex = (sampleIndex + 1u) % 10u;
		gGameTime.latencySampleTime = now;
		auto frequency = GameTime_Network_LatencyToFrequency(gSynchronizationBudget);
		if (gGameTime.speed >= 0.01 && gGameTime.speed < static_cast<double>(frequency)) frequency = 1;
		if (frequency != gFrequencyCandidate) { gFrequencyCandidate = frequency; gCandidateSamples = 1; }
		else if (gCandidateSamples < 3) ++gCandidateSamples;
		// Slow down promptly; require three stable observations before speeding up.
		if (frequency == gGameTime.networkFrequency || (frequency > gGameTime.networkFrequency && gCandidateSamples < 3)) return;
		std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{};
		command[0] = std::byte{ SwitchIds::UserCommand::SetNetworkFrequency };
		Write(command, NC2Layout::NetworkFrequencyCommandOffset::Frequency, frequency);
		auto* queue = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(gUserNetworkCommandManager) + NC2Layout::UserNetworkCommandManagerOffset::SystemQueue);
		const auto queued = queue->GetCount();
		UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, command, true, false);
		if (queue->GetCount() > queued) gPendingFrequency = frequency;
	}

	// Original function: FUN_0040aeb7.
	std::uint32_t Logic_EntryPoint_GameTime_GetTick()
	{
		return gGameTime.tick;
	}

	// Original function: FUN_0040aece; Windows returns the stored double in ST(0).
	double Logic_EntryPoint_GameTime_GetSpeed()
	{
		return gGameTime.speed;
	}

	extern void __fastcall HouseArray_DoOneGameTick(void*, void* unusedEdx);
	extern void __fastcall HouseArray_DoOneGameSecond(void*, void* unusedEdx);
	extern void __fastcall RoadBuildManager_DoOneGameSecond(void*, void* unusedEdx);
	extern void __fastcall WallBuildManager_DoOneGameSecond(void*, void* unusedEdx);
	extern void __fastcall GameControl_System_DoOneGameTick(void*, void* unusedEdx, std::uint32_t);
	extern void Game_l_Autosave_Update();
	// Original function: FUN_0040B183; query only nonzero, non-local player IDs.
	std::uint32_t GameTime_Network_GetHighestPlayerLatency()
	{
		const auto* players = static_cast<const std::byte*>(gDynamicNetworkData.playerInfoRecords);
		auto* network = GetNetworkInstance();
		std::uint32_t maximum = 0;
		for (std::uint32_t index = 0; index < gDynamicNetworkData.playerCount; ++index)
		{
			const auto player = NC2Lifetime::Read<NetworkPlayerId>(players, index * NC2Layout::RecordSize::NetworkPlayerUnsigned);
			if (player == 0 || player == NNetwork::Network_GetLocalPlayerId()) continue;
			NetworkConnectionInfo connection;
			if (network->GetConnectionData(NC2Lifetime::Read<NetworkPlayerId>(players, index * NC2Layout::RecordSize::NetworkPlayerUnsigned), &connection) && connection.dwRoundTripLatencyMS > maximum) maximum = connection.dwRoundTripLatencyMS;
		}
		return maximum;
	}

	static bool GameTime_IsATickToGo();

	// Original function: FUN_0040b1eb; maximum of the complete ten-sample region.
	std::uint32_t GameTime_Network_GetHighestStoredLatency()
	{
		std::uint32_t maximum = 0;
		for (std::uint32_t sample : gGameTime.latencySamples) if (sample > maximum) maximum = sample;
		return maximum;
	}

	// Original function: FUN_0040b206. Modern policy consumes RTT + measured variation + scheduling reserve.
	std::uint32_t GameTime_Network_LatencyToFrequency(std::uint32_t latency)
	{
		for (const auto frequency : { 12u, 6u, 4u, 3u, 2u, 1u }) if (latency <= 1000u / frequency) return frequency;
		return 1;
	}

	// Original function: FUN_004077b8.
	void GameTime_Update()
	{
		if (gGameTime.maximizeSpeed != 0)
		{
			const std::uint32_t startTime = timeGetTime();
			while (timeGetTime() - startTime < 500u) GameTime_DoOneGameTick();
			gGameTime.lastRealTime = static_cast<double>(timeGetTime());
			return;
		}
		for (std::int32_t count = 0; count < 50; ++count)
		{
			if (!GameTime_IsATickToGo()) break;
			GameTime_DoOneGameTick();
		}
	}

	// Original function: FUN_00407812.
	void GameTime_DoOneGameTick()
	{
		if (gGameInitialized == 0) return;
		if (gGameStaticVars.networkGame != 0)
		{
			const std::uint32_t speed = static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(gGameTime.speed));
			if (gGameTime.tick % (speed * 30u) == 0)
			{
				std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
				command[0] = std::byte{ SwitchIds::UserCommand::SubmitGameChecksum };
				Write<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Checksum, Game_Checksum_Calculate());
				Write<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::Tick, gGameTime.tick);
				Write<std::uint32_t>(command, NC2Layout::GameChecksumCommandOffset::NetworkPlayerId, NNetwork::GetDynamicNetworkLocalPlayerId(&gDynamicNetworkData));
				UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, command, true, false);
			}
		}
		++gGameTime.tick;
		gGameTime.lastRealTime += gGameTime.tickDuration;
		if (gGameTime.tick % 720u == 0) PlayerStatisticManager_UpdateStatistics(gPlayerStatisticManager, nullptr, gGameTime.tick / 12u);
		MapMoveableArray_DoOneGameTick(gMapMoveableArrayStorage.manager, nullptr);
		HouseArray_DoOneGameTick(CHouseArray::sTheObjectPtr, nullptr);
		if (gGameTime.tick % 12u == 0)
		{
			HouseArray_DoOneGameSecond(CHouseArray::sTheObjectPtr, nullptr);
			RoadBuildManager_DoOneGameSecond(gRoadBuildManager, nullptr);
			WallBuildManager_DoOneGameSecond(gWallBuildManager, nullptr);
		}
		Map_BioSystem_DoOneGameTick();
		static_cast<CMapLandscapeAutoTransitionManager*>(gMapLandscapeAutoTransitionManager)->System_DoOneGameTick();
		static_cast<CMapBioRegrowManager*>(gMapBioRegrowManager)->System_DoOneGameTick();
		DelayedActionManager_System_DoOneGameTick(gDelayedActionManager, nullptr);
		GameControl_System_DoOneGameTick(gGameControl, nullptr, gGameTime.tick);
		Game_l_Autosave_Update();
		CCallbackManager::sTheObjectPtr->ExecuteCallback(1, true, gGameTime.tick, 0, 0);
	}

	// Original function: FUN_004079cf; separate initialization entry.
	void GameTime_ResetNetworkSynchronization()
	{
		if (gGameStaticVars.networkGame != 0)
		{
			NNetwork::AdvanceDynamicNetworkTick(&gDynamicNetworkData);
			UserNetworkCommandManager_SendCommands(gUserNetworkCommandManager, nullptr);
			GameTime_SetNetworkWaitFlag(false);
		}
		gGameTime.networkBaseTick = 0;
		gGameTime.networkBaseRealTime = 0;
		gGameTime.networkFrequency = 3;
		gGameTime.latencySampleTime = timeGetTime();
		gGameTime.autoAdjustNetworkFrequency = 1;
		for (auto& sample : gGameTime.latencySamples) sample = 400;
		gGameTime.latencySampleIndex = 0;
		gSynchronizationPoll = timeGetTime();
		gSynchronizationBudget = 0; gPendingFrequency = 0; gFrequencyCandidate = 0; gCandidateSamples = 0;
	}

	// Original function: FUN_00407a36; Mac l_GameTime_IsATickToGo.
	static bool GameTime_IsATickToGo()
	{
		const std::uint32_t now = timeGetTime();
		if (gGameStaticVars.networkGame != 0)
		{
			UpdateAutomaticSynchronization(now);
			const double speed = gGameTime.speed;
			if (!(speed >= 0.01) && now - gGameTime.networkBaseRealTime < 1000u)
			{
				gGameTime.lastRealTime = static_cast<double>(now);
				return false;
			}
			const std::uint32_t targetTick = gGameTime.networkBaseTick + GameTime_Network_GetTicksPerCycle();
			if (!(gGameTime.tick < targetTick && speed >= 0.01))
			{
				if (!UserNetworkCommandManager_ReceiveCommands(gUserNetworkCommandManager, nullptr))
				{
					gGameTime.lastRealTime = static_cast<double>(now);
					return false;
				}
				gGameTime.networkBaseTick = gGameTime.tick;
				gGameTime.networkBaseRealTime = now;
				NNetwork::AdvanceDynamicNetworkTick(&gDynamicNetworkData);
				UserNetworkCommandManager_SendCommands(gUserNetworkCommandManager, nullptr);
				GameTime_SetNetworkWaitFlag(false);
			}
			else
				return static_cast<double>(now) - gGameTime.lastRealTime >= gGameTime.tickDuration;
		}
		if (!(gGameTime.speed >= 0.01))
		{
			gGameTime.lastRealTime = static_cast<double>(now);
			return false;
		}
		return static_cast<double>(now) - gGameTime.lastRealTime >= gGameTime.tickDuration;
	}

	// Original function: FUN_0040aeb2; distinct tail-forwarding entry.
	void Logic_EntryPoint_GameTime_Update()
	{
		GameTime_Update();
	}

	// Original function: FUN_0040b246.
	void GameTime_SetNetworkWaitFlag(bool waiting)
	{
		gGameStaticVars.networkWait = static_cast<std::uint8_t>(waiting);
	}
	// Original function: FUN_004074e5.
	void Autosave_Start(std::uint32_t intervalTicks)
	{
		if (intervalTicks == 0)
		{
			gGameStaticVars.autosave.intervalTicks = 0; return;
		}
		gGameStaticVars.autosave.sequence = 0;
		gGameStaticVars.autosave.intervalTicks = intervalTicks;
		l_Autosave_Do();
	}

	// Original function: FUN_00407524.
	void l_Autosave_Do()
	{
		gGameStaticVars.autosave.lastSaveTick = gGameTime.tick;
		const auto slot = gGameStaticVars.autosave.sequence % 10;
		char filename[20];
		NXBasics::Runtime_PrintString(filename, "user_autosave_%2.2d", slot);
		GameIO_Savegame_Save(filename, nullptr, static_cast<std::int32_t>(slot + 2000));
		++gGameStaticVars.autosave.sequence;
	}

	// Original function: FUN_00407505; both operands use wrapping unsigned arithmetic.
	void Game_l_Autosave_Update()
	{
		const auto& autosave = gGameStaticVars.autosave;
		if (autosave.intervalTicks != 0 && gGameTime.tick - autosave.lastSaveTick >= autosave.intervalTicks) l_Autosave_Do();
	}

}
