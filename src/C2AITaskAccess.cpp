#include "C2AIConditions.h"
#include "C2AIMilitaryAccess.h"
#include "C2AITaskAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0044dd7f; task counts store the last occupied index.
	void AISinglePlayerHandler_MainTask_RecheckAll(void* handler)
	{
		const auto last = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex);
		for (std::uint32_t i = 0; i <= last; ++i)
		{
			auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + i * NC2Layout::RecordSize::AIMainTask;
			const auto type = Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description);
			if (type == 0) continue;
			const auto priority = AISinglePlayerHandler_Condition_IsActive(handler, Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ConditionId)) != 0 && Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::Disabled) == 0 ? Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ConfiguredPriority) : 0u;
			Write(task, NC2Layout::AIMainTaskOffset::ActivePriority, priority);
			if (priority == 0) continue;
			bool disable = false;
			if (type == 3)
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, Read<std::int16_t>(task, NC2Layout::AIBuildHouseTaskOffset::Position), Read<std::int16_t>(task, NC2Layout::AIBuildHouseTaskOffset::PositionY), false);
				disable = AISinglePlayerHandler_l_BuildHouses_AddBuildHouse(handler, Read<std::uint32_t>(task, NC2Layout::AIBuildHouseTaskOffset::HouseType), &point, Read<std::uint32_t>(task, NC2Layout::AIBuildHouseTaskOffset::DevelopmentStage), Read<std::uint32_t>(task, NC2Layout::AIBuildHouseTaskOffset::RequestedCount));
			}
			else if (type == 4)
			{
				GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(task, NC2Layout::AIDiplomacyTaskOffset::OtherPlayerId), Read<std::uint32_t>(task, NC2Layout::AIDiplomacyTaskOffset::DiplomacyState));
				disable = true;
			}
			else if (type == 5)
			{
				for (std::uint32_t count = 0; count < Read<std::uint32_t>(task, NC2Layout::AISpawnHumansTaskOffset::HumanCount); ++count)
				{
					std::uint32_t point;
					MapMigPoint_SetPosition(&point, Read<std::int16_t>(task, NC2Layout::AISpawnHumansTaskOffset::Position), Read<std::int16_t>(task, NC2Layout::AISpawnHumansTaskOffset::PositionY), false);
					Logic_EntryPoint_Add_Human(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(task, NC2Layout::AISpawnHumansTaskOffset::TribeType), Read<std::uint32_t>(task, NC2Layout::AISpawnHumansTaskOffset::JobType), &point, Read<std::uint32_t>(task, NC2Layout::AISpawnHumansTaskOffset::Height), 0);
				}
				disable = Read<std::uint8_t>(task, NC2Layout::AISpawnHumansTaskOffset::DisableAfterSpawn) != 0;
			}
			else if (type == 6)
			{
				CHumanIterator iterator;
				for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)); id != -1; id = iterator.GetNextHumanId()) CHumanArray::sTheObjectPtr->FreeHuman(id);
				disable = true;
			}
			else if (type == 7 || type == 8) disable = true;
			else continue;
			if (disable) Write<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::Disabled, 1);
			Write<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ActivePriority, 0);
		}
		std::uint32_t priorities[64];
		std::uint32_t count = 0;
		Write<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskCount, 0);
		for (std::uint32_t i = 0; i <= last; ++i)
		{
			const auto* task = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + i * NC2Layout::RecordSize::AIMainTask;
			if (Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description) == 0 || Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ActivePriority) == 0) continue;
			Write(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskIndices + count * NC2Layout::RecordSize::AITaskIndexValue, i);
			priorities[count++] = Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ActivePriority);
			Write(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskCount, count);
		}
		if (count > 1)
			for (std::uint32_t end = count - 1; end != 0; --end)
				for (std::uint32_t i = 0; i < end; ++i)
					if (priorities[i] < priorities[i + 1])
					{
						const auto priority = priorities[i]; priorities[i] = priorities[i + 1]; priorities[i + 1] = priority;
						const auto index = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskIndices + i * NC2Layout::RecordSize::AITaskIndexValue);
						Write(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskIndices + i * NC2Layout::RecordSize::AITaskIndexValue, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SecondActiveMainTaskIndex + i * NC2Layout::RecordSize::AITaskIndexValue));
						Write(handler, NC2Layout::AISinglePlayerOffset::SecondActiveMainTaskIndex + i * NC2Layout::RecordSize::AITaskIndexValue, index);
					}
		AISinglePlayerHandler_l_CombinedActiveMainTask_UpdateList(handler);
	}

	// Original function: FUN_0044e2c9; unlike recalculation, this appends without deduplication.
	void AISinglePlayerHandler_l_BuildHouses_SaveAllHousesIntoArray(void* handler)
	{
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)); id != -1; id = iterator.GetNextHouseId())
		{
			const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount);
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			if (count == 100) break;
			Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseTypes + count * NC2Layout::RecordSize::AIBuildHouse, Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
			Write(handler, NC2Layout::AISinglePlayerOffset::BuildHousePositions + count * NC2Layout::RecordSize::AIBuildHouse, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition));
			Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseStages + count * NC2Layout::RecordSize::AIBuildHouse, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage));
			Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseMissionCheckIds + count * NC2Layout::RecordSize::AIBuildHouse, Read<std::uint32_t>(house, NC2Layout::HouseOffset::MissionCheckId));
			Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount, count + 1);
		}
	}

	// Original function: FUN_0042b41e.
	bool AITaskSimpleDescription_IsEqualTo(const void* description, const void* other)
	{
		const auto type = Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Type);
		if (type != Read<std::uint32_t>(other, NC2Layout::AITaskDescriptionOffset::Type) || (type != 1 && type != 2)) return false;
		return NXBasics::Runtime_CompareMemory(static_cast<const std::byte*>(description) + 4, static_cast<const std::byte*>(other) + 4, type == 1 ? 16 : 24) == 0;
	}

	// Original function: FUN_0042b450.
	bool AITaskSimpleDescription_Tool_GetMapPosition(const void* description, void* point)
	{
		const auto type = Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Type);
		if (type != 1 && type != 2 && type != 3 && type != 5 && type != 8) return false;
		MapMigPoint_Construct(point, Read<std::uint16_t>(description, NC2Layout::AITaskDescriptionOffset::MapPoint), Read<std::uint16_t>(description, NC2Layout::AITaskDescriptionOffset::MapPointY), false);
		return true;
	}

	// Original function: FUN_0042b489.
	void AIMainTask_Init(void* task, std::uint32_t type, std::int32_t priority, std::uint32_t condition)
	{
		NXBasics::Runtime_FillMemory(task, 0, NC2Layout::RecordSize::AIMainTaskPrefix);
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(task) + NC2Layout::AIMainTaskOffset::NoWayCorrection, 0, NC2Layout::RecordSize::AIMainTaskNoWayCorrection);
		Write(task, NC2Layout::AIMainTaskOffset::Description, type);
		Write(task, NC2Layout::AIMainTaskOffset::ConfiguredPriority, priority);
		Write(task, NC2Layout::AIMainTaskOffset::ConditionId, condition);
		Write(task, NC2Layout::AIMainTaskOffset::CreatedTick, gGameTime.tick);
	}

	// Original function: FUN_0042b4c7.
	void AIActiveCombinedMainTasks_Init(void* task, std::uint32_t mainTask, std::int32_t priority, const void* description)
	{
		NXBasics::Runtime_FillMemory(task, 0, 0x58);
		const auto type = Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Type);
		Write(task, NC2Layout::AICombinedTaskOffset::TaskType, type);
		Write(task, NC2Layout::AICombinedTaskOffset::Priority, priority);
		Write(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices, mainTask);
		Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskCount, 1);
		if (type == 1 || type == 2)
		{
			Write(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::MinimumSoldiers));
			Write(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers, Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::MaximumSoldiers));
			Write(task, NC2Layout::AICombinedTaskOffset::Radius, Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Radius));
			if (type == 2)
			{
				MapMigPoint_Construct(static_cast<std::byte*>(task) + NC2Layout::AICombinedTaskOffset::FormationPosition, Read<std::uint16_t>(description, NC2Layout::AITaskDescriptionOffset::FormationPosition), Read<std::uint16_t>(description, NC2Layout::AITaskDescriptionOffset::FormationPositionY), false);
				Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 2);
			}
		}
	}

	// Original function: FUN_0042b543; the secondary center is averaged with truncation toward zero.
	bool AIActiveCombinedMainTasks_AddMainTask(void* task, std::uint32_t mainTask, std::int32_t priority, const void* description)
	{
		const auto count = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskCount);
		if (count == 10) return false;
		Write(task, NC2Layout::AICombinedTaskOffset::Priority, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::Priority) + static_cast<std::uint32_t>(priority));
		Write(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices + count * NC2Layout::RecordSize::AITaskIndexValue, mainTask);
		Write(task, NC2Layout::AICombinedTaskOffset::MainTaskCount, count + 1);
		const auto type = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::TaskType);
		if (type == 1 || type == 2)
		{
			Write(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers) + Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::MinimumSoldiers));
			Write(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers) + Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::MaximumSoldiers));
			const auto radius = Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Radius);
			if (radius > Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::Radius)) Write(task, NC2Layout::AICombinedTaskOffset::Radius, radius);
			if (type == 2)
			{
				const auto x = (static_cast<std::int32_t>(Read<std::int16_t>(task, NC2Layout::AICombinedTaskOffset::FormationPosition)) + Read<std::int16_t>(description, NC2Layout::AITaskDescriptionOffset::FormationPosition)) / 2;
				const auto y = (static_cast<std::int32_t>(Read<std::int16_t>(task, NC2Layout::AICombinedTaskOffset::FormationPositionY)) + Read<std::int16_t>(description, NC2Layout::AITaskDescriptionOffset::FormationPositionY)) / 2;
				MapMigPoint_Construct(static_cast<std::byte*>(task) + NC2Layout::AICombinedTaskOffset::FormationPosition, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), false);
			}
		}
		return true;
	}

	// Original function: FUN_0042b5d3.
	void AIConditionToConditionCheck_Tool_Init(void* check, std::uint32_t operation)
	{
		Write<std::uint32_t>(check, NC2Layout::AIConditionCheckOffset::OperandCount, 0); Write(check, NC2Layout::AIConditionCheckOffset::Operation, operation);
		for (std::uint32_t i = 0; i < NC2Runtime::AIConditionLimits::Operands; ++i) Write<std::uint32_t>(check, NC2Layout::AIConditionCheckOffset::OperandIds + i * NC2Layout::RecordSize::AIConditionOperand, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_0044e09a; Windows combines matching type-2 centers only.
	void AISinglePlayerHandler_l_CombinedActiveMainTask_UpdateList(void* handler)
	{
		// The native rebuild resets AttackPhase. Retain autonomous wave progress by main-task ID.
		struct SWave { std::uint32_t main, phase, position; } waves[50]{};
		std::uint32_t waveCount = 0;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount) && i < 50; ++i)
		{
			const auto* task = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
			if (Remaster_AIMilitary_IsWaveTask(handler, task) && Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius) == 8)
				waves[waveCount++] = { Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices), Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase), Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationPosition) };
		}
		Write<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount, 0);
		const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskCount);
		if (count == 0) return;
		for (std::uint32_t i = 0; i < count; ++i)
		{
			const auto id = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::ActiveMainTaskIndices + i * NC2Layout::RecordSize::AITaskIndexValue);
			const auto* main = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + id * NC2Layout::RecordSize::AIMainTask;
			const auto type = Read<std::uint32_t>(main, NC2Layout::AIMainTaskOffset::Description);
			bool add = true;
			if (type == 1 || type == 2)
				for (std::uint32_t j = 0; j < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount); ++j)
				{
					auto* combined = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + j * NC2Layout::RecordSize::AICombinedTask;
					if (Read<std::uint32_t>(combined, NC2Layout::AICombinedTaskOffset::TaskType) != type) continue;
					const auto* first = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + Read<std::uint32_t>(combined, NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask;
					if (type == 2 && Read<std::uint16_t>(main, NC2Layout::AIMainTaskOffset::PrimaryPosition) == Read<std::uint16_t>(first, NC2Layout::AIMainTaskOffset::PrimaryPosition) && Read<std::uint16_t>(main, NC2Layout::AIMainTaskOffset::PrimaryPositionY) == Read<std::uint16_t>(first, NC2Layout::AIMainTaskOffset::PrimaryPositionY) && AIActiveCombinedMainTasks_AddMainTask(combined, id, Read<std::int32_t>(main, NC2Layout::AIMainTaskOffset::ActivePriority), main + NC2Layout::AIMainTaskOffset::Description))
					{
						add = false; break;
					}
				}
			if (add)
			{
				const auto slot = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
				if (slot == 50) break;
				AIActiveCombinedMainTasks_Init(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + slot * NC2Layout::RecordSize::AICombinedTask, id, Read<std::int32_t>(main, NC2Layout::AIMainTaskOffset::ActivePriority), main + NC2Layout::AIMainTaskOffset::Description);
				Write(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount, slot + 1);
			}
		}
		const auto combinedCount = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
		for (std::uint32_t i = 0; i < combinedCount; ++i)
		{
			auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
			if (!Remaster_AIMilitary_IsWaveTask(handler, task)) continue;
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 1);
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius, 8);
			for (std::uint32_t j = 0; j < waveCount; ++j)
				if (waves[j].main == Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices))
				{
					Write(task, NC2Layout::AICombinedTaskOffset::AttackPhase, waves[j].phase);
					Write(task, NC2Layout::AICombinedTaskOffset::FormationPosition, waves[j].position);
					break;
				}
			// Gather existing soldiers immediately; MaximumSoldiers remains the launch target.
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, 1);
		}
		std::uint32_t total = 0;
		for (std::uint32_t i = 0; i < combinedCount; ++i) total += Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstCombinedTaskPriority + i * NC2Layout::RecordSize::AICombinedTask);
		if (total != 0)
			for (std::uint32_t i = 0; i < combinedCount; ++i) Write(handler, NC2Layout::AISinglePlayerOffset::FirstCombinedTaskPriorityPercent + i * NC2Layout::RecordSize::AICombinedTask, static_cast<double>(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstCombinedTaskPriority + i * NC2Layout::RecordSize::AICombinedTask)) * 100.0 / total);
		for (std::uint32_t end = combinedCount; end > 1; --end)
			for (std::uint32_t i = 0; i + 1 < end; ++i)
			{
				auto* first = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
				if (Read<std::int32_t>(first, NC2Layout::AICombinedTaskOffset::Priority) >= Read<std::int32_t>(first, NC2Layout::AICombinedTaskOffset::NextTaskPriority)) continue;
				alignas(8) std::byte temporary[88];
				AIActiveCombinedMainTasks_CopyConstruct(temporary, first);
				for (std::uint32_t offset = 0; offset < 0x58; offset += 4) Write(first, offset, Read<std::uint32_t>(first, offset + NC2Layout::RecordSize::AICombinedTask));
				for (std::uint32_t offset = 0; offset < 0x58; offset += 4) Write(first, offset + NC2Layout::RecordSize::AICombinedTask, Read<std::uint32_t>(temporary, offset));
			}
	}

	// Original function: FUN_0044e26d; the native copy constructor leaves trailing padding untouched.
	void* AIActiveCombinedMainTasks_CopyConstruct(void* task, const void* other)
	{
		Write(task, NC2Layout::AICombinedTaskOffset::TaskType, Read<std::uint32_t>(other, NC2Layout::AICombinedTaskOffset::TaskType));
		Write(task, NC2Layout::AICombinedTaskOffset::Priority, Read<std::uint32_t>(other, NC2Layout::AICombinedTaskOffset::Priority));
		Write(task, NC2Layout::AICombinedTaskOffset::PriorityPercent, Read<double>(other, NC2Layout::AICombinedTaskOffset::PriorityPercent));
		for (std::uint32_t offset = 0x10; offset < 0x50; offset += 4) Write(task, offset, Read<std::uint32_t>(other, offset));
		Write(task, NC2Layout::AICombinedTaskOffset::FormationPosition, Read<std::uint16_t>(other, NC2Layout::AICombinedTaskOffset::FormationPosition));
		Write(task, NC2Layout::AICombinedTaskOffset::FormationPositionY, Read<std::uint16_t>(other, NC2Layout::AICombinedTaskOffset::FormationPositionY));
		return task;
	}

	// Original function: FUN_0044e433; each store reloads the native count, retaining alias behavior.
	bool AISinglePlayerHandler_l_BuildHouses_AddBuildHouse(void* handler, std::uint32_t type, const void* point, std::uint32_t stage, std::uint32_t requested)
	{
		if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) == 100) return false;
		Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseTypes + Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) * NC2Layout::RecordSize::AIBuildHouse, type);
		Write(handler, NC2Layout::AISinglePlayerOffset::BuildHousePositions + Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) * NC2Layout::RecordSize::AIBuildHouse, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseStages + Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) * NC2Layout::RecordSize::AIBuildHouse, stage);
		Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseMissionCheckIds + Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) * NC2Layout::RecordSize::AIBuildHouse, requested);
		Write(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) + 1);
		return true;
	}

	// Original function: FUN_0042a53f; the player index is unchecked in Windows.
	bool AIManager_EntryPoint_MainTask_Add(void* manager, std::uint32_t player, const void* task)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);
		return handler != nullptr && AISinglePlayerHandler_MainTask_Add(handler, task) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0042a5bf.
	bool AIManager_EntryPoint_SingleSoldier_SetDefaultPosition(void* manager, std::uint32_t player, const void* point, std::uint32_t radius)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);
		return handler != nullptr && AISinglePlayerHandler_EntryPoint_SingleSoldier_SetDefaultPosition(handler, point, radius);
	}

	// Original function: FUN_0044df73; the highest slot is updated before task validation.
	std::uint32_t AISinglePlayerHandler_l_MainTask_GetFreeArraySlot(void* handler)
	{
		for (std::uint32_t slot = 0; slot < 50; ++slot)
		{
			if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstMainTaskDescriptionUnsigned + slot * NC2Layout::RecordSize::AIMainTaskUnsigned) != 0) continue;
			if (slot > Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex)) Write(handler, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex, slot);
			return slot;
		}
		return NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0044df9a; distinct primary/secondary border checks precede a forward DWORD copy.
	std::uint32_t AISinglePlayerHandler_MainTask_Add(void* handler, const void* task)
	{
		const auto slot = AISinglePlayerHandler_l_MainTask_GetFreeArraySlot(handler);
		if (slot == NXBasics::IntegerRange::UInt32Maximum) return slot;
		const auto type = Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description);
		if (type == 1 || type == 2 || type == 3 || type == 5)
		{
			std::uint32_t point;
			MapMigPoint_SetPosition(&point, Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPositionY), false);
			if (MapMigPoint_IsBorderPoint(&point)) return NXBasics::IntegerRange::UInt32Maximum;
			if (type == 2)
			{
				MapMigPoint_Construct(&point, Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPositionY), false);
				if (MapMigPoint_IsBorderPoint(&point)) return NXBasics::IntegerRange::UInt32Maximum;
			}
		}
		void* destination = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasksUnsigned + slot * NC2Layout::RecordSize::AIMainTaskUnsigned;
		for (std::uint32_t offset = 0; offset < 0x44; offset += 4) Write(destination, offset, Read<std::uint32_t>(task, offset));
		Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::MainTasksChanged, 1);
		return slot;
	}

	// Original function: FUN_0045afe1; only a non-border point on a nonzero continent type is accepted.
	bool AISinglePlayerHandler_EntryPoint_SingleSoldier_SetDefaultPosition(void* handler, const void* point, std::uint32_t radius)
	{
		if (MapMigPoint_IsBorderPoint(point)) return false;
		if (MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(point)) == 0) return false;
		Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPositionEnabled, 1);
		Write(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPosition, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierRadius, radius);
		return true;
	}
}
