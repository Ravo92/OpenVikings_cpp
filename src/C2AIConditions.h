#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows condition records. The union models type-dependent payloads only.
	struct SAIConditionToConditionCheckStorage
	{
		std::uint32_t operation, operandCount, operandIds[10];
	};

	struct SAIConditionDelayedStorage
	{
		std::uint32_t conditionId;
		std::uint8_t watchActive;
		std::byte padding05[3];
		std::uint32_t delayTicks;
	};

	struct SAIConditionDiplomacyStorage
	{
		std::int32_t firstPlayerId, secondPlayerId;
		std::uint32_t state;
	};

	struct SAIConditionMoveableRangeStorage
	{
		std::int16_t x, y;
		std::uint32_t radius, playerId;
		std::uint8_t opponentsOnly, militaryOnly;
		std::byte padding0E[2];
	};

	struct SAIConditionHouseRangeStorage
	{
		std::int16_t x, y;
		std::uint32_t radius, playerId;
		std::uint8_t opponentsOnly;
		std::byte padding0D[3];
		std::uint32_t houseType;
		std::uint8_t completedOnly;
		std::byte padding15[3];
	};

	struct SAIConditionPlayerSeenStorage
	{
		std::uint32_t playerId, seenPlayerId;
	};

	struct SAIConditionSoldierCountStorage
	{
		std::uint32_t minimumCount, playerId;
	};

	struct SAIConditionPeriodicStorage
	{
		std::uint32_t initialDelayTicks, activeTicks, inactiveTicks;
	};

	union SAIConditionPayloadStorage
	{
		std::byte opaque[48];
		std::uint32_t requiredTick, playerId;
		std::uint8_t activatedFlag;
		SAIConditionToConditionCheckStorage logical;
		SAIConditionDelayedStorage delayed;
		SAIConditionDiplomacyStorage diplomacy;
		SAIConditionMoveableRangeStorage moveableRange;
		SAIConditionHouseRangeStorage houseRange;
		SAIConditionPlayerSeenStorage playerSeen;
		SAIConditionSoldierCountStorage soldierCount;
		SAIConditionPeriodicStorage periodic;
	};

	struct SAIConditionStorage
	{
		std::uint8_t active;
		std::byte padding01[3];
		std::uint32_t activatedTick, deactivatedTick, type;
		std::uint8_t keepActive;
		std::byte padding11[3];
		std::uint32_t createdTick;
		SAIConditionPayloadStorage payload;
	};

	bool AIConditionToConditionCheck_Tool_CheckNumberOfConditions(const void* check);
	void AICondition_Init(void* condition, std::uint32_t type, std::uint8_t flag);
	bool AICondition_SetCondition(void* condition, std::uint8_t active);
	std::uint8_t AISinglePlayerHandler_Condition_IsActive(const void* handler, std::uint32_t index);
	bool AISinglePlayerHandler_Condition_RecheckAll(void* handler, std::uint32_t tick);
	bool AISinglePlayerHandler_l_Condition_CheckForConditionChange(void* handler, void* condition);
	bool AISinglePlayerHandler_Condition_Set(void* handler, std::uint32_t index, const void* condition);
	bool AIManager_EntryPoint_Condition_Set(void* manager, std::uint32_t player, std::uint32_t index, const void* condition);
	bool AIManager_EntryPoint_SetUnitLimit(void* manager, std::uint32_t player, std::uint32_t limit);
	bool AIManager_EntryPoint_SetMaxUnitLimit(void* manager, std::uint32_t player, std::uint32_t limit);
	bool AIManager_EntryPoint_Condition_ExternalActivate_SetFlag(void* manager, std::uint32_t player, std::uint32_t index, bool active);
	bool AISinglePlayerHandler_EntryPoint_Condition_ExternalActivate_SetFlag(void* handler, std::uint32_t index, bool active);
}