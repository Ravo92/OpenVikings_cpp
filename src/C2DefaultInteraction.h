#pragma once
#include <cstdint>

namespace NC2Logic
{
	struct SUserCommandsForVehicleHandler
	{
		std::int32_t count;
		std::uint32_t commands[10];
		void Init();
		void AddCommand(std::uint32_t command);
		bool IsCommandAllowed(std::int32_t command) const;
	};

	static_assert(sizeof(SUserCommandsForVehicleHandler) == 44);
	bool StaticDependencies_Item_IsChest(std::int32_t landscape);
	bool StaticDependencies_Item_IsAbleToOpenChest(std::int32_t job, std::int32_t landscape);
	bool RoadBuildManager_IsRoadBuildFlagSetForPlayer(const void* manager, std::int32_t player, const void* point);
	std::int32_t RoadBuildManager_GetRoadBuildFlagPlayerId(const void* manager, const void* point);
	std::uint32_t House_JobMisc_GetPrimaryWorkerJobType(const void* house);
	bool House_JobMisc_CanJobBeAttachedAsWorker(const void* house, std::uint32_t job);
	const void* Animal_GetConstMapMicroPointPtr(const void* animal);
	bool Vehicle_Attachment_IsValid(const void* vehicle);
	void Vehicle_UserControl_GetCommands(const void* vehicle, SUserCommandsForVehicleHandler* commands, bool multiple);
	bool Logic_UserControl_GetVehicleCommands(std::int32_t id, SUserCommandsForVehicleHandler* commands, bool multiple);
	std::uint32_t Human_UserControl_GetDefaultInteractionCommand_SingleHuman(const void* human, const void* target, void* command, void* secondary, bool modifier);
	bool Human_UserControl_GetDefaultInteractionCommand_MultipleHuman(const void* human, std::uint32_t* action, const void* target, void* command, std::uint32_t index, std::uint32_t count, bool modifier);
	std::uint32_t Vehicle_UserControl_GetDefaultInteractionCommand_SingleVehicle(const void* vehicle, const void* target, void* command, void* secondary, bool modifier);
	std::uint32_t Vehicle_UserControl_GetDefaultInteractionCommand_TrySendTo(const void* vehicle, void* command, const void* point, const SUserCommandsForVehicleHandler* commands, bool modifier);
	bool Vehicle_UserControl_GetDefaultInteractionCommand_MultipleVehicles(const void* vehicle, std::uint32_t* action, const void* target, void* command, std::uint32_t index, std::uint32_t count, bool modifier);
}