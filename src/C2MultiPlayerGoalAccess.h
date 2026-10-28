#pragma once
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
	// Documentary Windows manager and tagged goal views; serialization still uses the native byte span.
	struct SMultiPlayerGoalPlayerResultStorage
	{
		std::uint8_t won; std::byte unclassified01[3];
		std::uint32_t winOrder;
		std::uint8_t lost; std::byte unclassified09[3];
		std::uint32_t lossOrder;
		std::uint8_t pendingRemoval; std::byte unclassified11[3];
	};
	struct SMultiPlayerGoalGoodRequirementStorage
	{
		std::uint32_t good; std::int32_t requiredAmount;
	};
	struct SMultiPlayerGoalStorage
	{
		std::uint32_t type; std::byte payload[68];
	};
	struct SMultiPlayerGoodsGoalStorage
	{
		std::uint32_t type; std::int32_t requirementCount; SMultiPlayerGoalGoodRequirementStorage requirements[8];
	};
	struct SMultiPlayerPopulationGoalStorage
	{
		std::uint32_t type, requiredCount; std::uint8_t soldiersOnly; std::byte unclassified09[63];
	};
	struct SMultiPlayerMissionGoalStorage
	{
		std::uint32_t type; std::uint8_t playerFlags[20]; std::byte unclassified18[48];
	};
	struct SMultiPlayerGoalManagerStorage
	{
		std::uint8_t enabled; std::byte unclassified01[3];
		SMultiPlayerGoalPlayerResultStorage players[20];
		std::uint32_t nextWinOrder, nextLossOrder, goalCount;
		SMultiPlayerGoalStorage goals[20];
	};

	std::uint32_t MultiPlayerGoalManager_Checksum_Calculate(const void* manager);
	void __fastcall MultiPlayerGoalManager_IO_LoadGoals(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	bool MultiPlayerGoalManager_l_IsWonOrLost(const void* manager, std::uint32_t player);
	void MultiPlayerGoalManager_l_Goals_Check(void* manager);
	void MultiPlayerGoalManager_l_KickFlags_Execute(void* manager);
	bool __cdecl MultiPlayerGoalManager_l_LogicCallback(unsigned int type, std::uintptr_t userData, std::uintptr_t tick, std::uintptr_t param2, std::uintptr_t param3);
	void MultiPlayerGoalManager_ExternalTrigger_WonByMissionManager(void* manager, std::uint32_t player);
	void MultiPlayerGoalManager_ExternalTrigger_LostByMissionManager(void* manager, std::uint32_t player);
}
