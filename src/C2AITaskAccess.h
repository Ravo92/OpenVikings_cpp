#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Windows task storage models; the description payload depends on its type.
	struct SAITaskSimpleDescriptionStorage
	{
		std::uint32_t type;
		std::byte payload[28];
	};

	// Windows no-way correction payload: byte flag, mode and two packed map points.
	struct SAIMainTaskNoWayCorrectionStorage
	{
		std::uint8_t pending;
		std::byte padding[3];
		std::uint32_t mode, sourcePoint, targetPoint;
	};

	struct SAIMainTaskStorage
	{
		std::uint8_t disabled;
		std::byte padding[3];
		std::uint32_t activePriority;
		SAITaskSimpleDescriptionStorage description;
		std::uint32_t configuredPriority, conditionId, createdTick;
		SAIMainTaskNoWayCorrectionStorage calculationState;
	};

	struct SAIActiveCombinedMainTasksStorage
	{
		std::uint32_t type, priority;
		double priorityPercent;
		std::uint32_t mainTaskCount, mainTaskIndices[10], minimumSoldiers, maximumSoldiers, radius, attackPhase, formationRadius, formationPosition;
		std::byte trailingPadding[4];
	};

	static_assert(sizeof(SAITaskSimpleDescriptionStorage) == 32);
	static_assert(sizeof(SAIMainTaskStorage) == 68);
	static_assert(sizeof(SAIActiveCombinedMainTasksStorage) == 88);

	bool AIManager_EntryPoint_MainTask_Add(void* manager, std::uint32_t player, const void* task);
	bool AIManager_EntryPoint_SingleSoldier_SetDefaultPosition(void* manager, std::uint32_t player, const void* point, std::uint32_t radius);
	std::uint32_t AISinglePlayerHandler_l_MainTask_GetFreeArraySlot(void* handler);
	std::uint32_t AISinglePlayerHandler_MainTask_Add(void* handler, const void* task);
	bool AISinglePlayerHandler_EntryPoint_SingleSoldier_SetDefaultPosition(void* handler, const void* point, std::uint32_t radius);
	bool AITaskSimpleDescription_IsEqualTo(const void* description, const void* other);
	bool AITaskSimpleDescription_Tool_GetMapPosition(const void* description, void* point);
	void AIMainTask_Init(void* task, std::uint32_t type, std::int32_t priority, std::uint32_t condition);
	void AIActiveCombinedMainTasks_Init(void* task, std::uint32_t mainTask, std::int32_t priority, const void* description);
	bool AIActiveCombinedMainTasks_AddMainTask(void* task, std::uint32_t mainTask, std::int32_t priority, const void* description);
	void AIConditionToConditionCheck_Tool_Init(void* check, std::uint32_t operation);
	void* AIActiveCombinedMainTasks_CopyConstruct(void* task, const void* other);
	bool AISinglePlayerHandler_l_BuildHouses_AddBuildHouse(void* handler, std::uint32_t type, const void* point, std::uint32_t stage, std::uint32_t requested);
	void AISinglePlayerHandler_l_CombinedActiveMainTask_UpdateList(void* handler);
	void AISinglePlayerHandler_MainTask_RecheckAll(void* handler);
	void AISinglePlayerHandler_l_BuildHouses_SaveAllHousesIntoArray(void* handler);
}