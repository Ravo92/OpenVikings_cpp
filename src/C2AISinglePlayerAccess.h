#pragma once
#include <cstddef>
#include <cstdint>
namespace NC2Logic
{
	// Documentary Windows storage views; callers still access the original byte records.
	struct SAISinglePlayerPrefixStorage
	{
		std::uint32_t playerId; std::uint8_t enabled, initializationPending, mainTasksChanged; std::byte unclassified07;
	};
	// Two-byte coordinates preserve the six-byte wall stride without packed DWORD members.
	struct SAISingleWallStorage
	{
		std::uint8_t workPending; std::byte unclassified01; std::int16_t x, y;
	};
	struct SAISingleGateStorage
	{
		std::uint8_t repairPending; std::byte unclassified01; std::int16_t x, y; std::uint8_t stateFlag; std::byte unclassified07;
	};
	struct SAISingleWallArrayStorage
	{
		std::uint8_t workPending; std::byte unclassified01[3]; std::uint32_t count; SAISingleWallStorage entries[1000];
	};
	struct SAISingleGateArrayStorage
	{
		std::uint8_t repairPending; std::byte unclassified01[3]; std::uint32_t count; SAISingleGateStorage entries[50];
	};
	struct SAIBuildHouseStorage
	{
		std::uint32_t engineType, mapPosition, developmentStage, missionCheckId;
	};
	struct SAIBuildHouseArrayStorage
	{
		std::uint32_t count; SAIBuildHouseStorage entries[100];
	};

	void AISinglePlayerHandler_WorkOnAI(void* handler, std::uint32_t tick);
	void AISinglePlayerHandler_l_Misc_CheckAndAddDefaultTasks(void* handler);
	void AISinglePlayerHandler_l_Women_WorkOn(void* handler);
	void AISinglePlayerHandler_l_BuildWallGates_SaveAllIntoArray(void* handler);
	void AISinglePlayerHandler_l_BuildWall_CheckForBuildOrRepairWall(void* handler);
	void AISinglePlayerHandler_l_BuildGate_CheckForDamagedGate(void* handler);
	void AISinglePlayerHandler_l_CheckDefenceMode(void* handler);
	bool AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(void* handler, const void* point);
	void AISinglePlayerHandler_l_OpenCloseGates(void* handler);
	void AISinglePlayerHandler_l_Messages_TryToRepairAllMainTasks(void* handler);
	void AISinglePlayerHandler_l_BuildHouses_CheckForBuildHouse(void* handler);
}
