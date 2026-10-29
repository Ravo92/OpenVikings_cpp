#include "C2AIConstruction.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "XCrtFileAccess.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Windows member constructors below initialize only their point fields.
		void ConstructPoints(void* array, std::uint32_t offset, std::uint32_t stride, std::uint32_t count)
		{
			for (std::uint32_t i = 0; i < count; ++i)
			{
				Write<std::uint16_t>(array, offset + stride * i + NC2Layout::MapCoordinatePairOffset::Y, 0);
				Write<std::uint16_t>(array, offset + stride * i, 0);
			}
		}

		// Original function: FUN_00408fa6; main-task calculation fields.
		void* AIMainTask_CalcVars_Construct(void* fields)
		{
			Write<std::uint32_t>(fields, NC2Layout::AIMainTaskCalcVarsOffset::SourcePoint, 0);
			Write<std::uint32_t>(fields, NC2Layout::AIMainTaskCalcVarsOffset::TargetPoint, 0);

			return fields;
		}

		// Original function: FUN_00407e28.
		void* AIMainTaskArray_Construct(void* array)
		{
			for (std::uint32_t i = 0; i < 50; ++i)
			{
				auto* task = static_cast<std::byte*>(array) + 4 + i * 0x44;
				AIMainTask_CalcVars_Construct(task + 0x34);
				NXBasics::Runtime_FillMemory(task, 0, 0x44);
			}

			return array;
		}

		// Original function: FUN_00407e5b.
		void* AIActiveCombinedMainTasksArray_Construct(void* array)
		{
			ConstructPoints(array, 0x58, 0x58, 50);

			return array;
		}

		// Original function: FUN_00407e73.
		void* AISingleWallArray_Construct(void* array)
		{
			ConstructPoints(array, NC2Layout::AISingleWallArrayOffset::FirstPosition, NC2Layout::RecordSize::AISingleWall, NC2Runtime::AISinglePlayerLimits::Walls);

			return array;
		}

		// Original function: FUN_00407e8d.
		void* AISingleGateArray_Construct(void* array)
		{
			ConstructPoints(array, NC2Layout::AISingleGateArrayOffset::FirstPosition, NC2Layout::RecordSize::AISingleGate, NC2Runtime::AISinglePlayerLimits::Gates);

			return array;
		}

		// Original function: FUN_00407ec7.
		void* AIBuildHouseArray_Construct(void* array)
		{
			ConstructPoints(array, NC2Layout::AIBuildHouseArrayOffset::FirstPosition, NC2Layout::RecordSize::AIBuildHouse, NC2Runtime::AISinglePlayerLimits::BuildHouses);

			return array;
		}

		// Original function: FUN_00407edf.
		void* AISingleVehicleArray_Construct(void* array)
		{
			for (std::uint32_t i = 0; i < 30; ++i)
			{
				NXBasics::Runtime_FillMemory(static_cast<std::byte*>(array) + 20 + i * 28, 0, 12);
			}

			return array;
		}

		// Original function: FUN_00407f3e.
		void* AISingleSoldier_Construct(void* soldier)
		{
			NXBasics::Runtime_FillMemory(static_cast<std::byte*>(soldier) + 0x48, 0, 12);

			return soldier;
		}

		// Original function: FUN_00407f03.
		void* AISingleSoldierArray_Construct(void* array)
		{
			for (std::uint32_t i = 0; i < 200; ++i)
			{
				AISingleSoldier_Construct(static_cast<std::byte*>(array) + 4 + i * 0x54);
			}

			return array;
		}

		// Original function: FUN_00407ea5.
		void* AISingleSoldiers_Construct(void* soldiers)
		{
			AISingleSoldierArray_Construct(soldiers);
			Write<std::uint16_t>(soldiers, NC2Layout::AISingleSoldiersOffset::DefaultPositionY, 0);
			Write<std::uint16_t>(soldiers, NC2Layout::AISingleSoldiersOffset::DefaultPositionX, 0);

			return soldiers;
		}

		// Original function: FUN_00407f23.
		void* AISingleSoldierCalcArray_Construct(void* array)
		{
			ConstructPoints(array, 0, 4, 200);

			return array;
		}

		// Original function: FUN_00407dd4.
		void* AISinglePlayerHandler_Vars_Construct(void* handler)
		{
			auto* bytes = static_cast<std::byte*>(handler);
			AIMainTaskArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::LastMainTaskIndex);
			AIActiveCombinedMainTasksArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
			AIBuildHouseArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::BuildHouseCount);
			AISingleWallArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::WallWorkPendingFlag);
			AISingleGateArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::GateRepairPendingFlag);
			AISingleVehicleArray_Construct(bytes + NC2Layout::AISinglePlayerOffset::VehicleCount);
			AISingleSoldiers_Construct(bytes + NC2Layout::AISinglePlayerOffset::SoldierCount);

			return handler;
		}

		// Original function: FUN_0042940e; the two original fills overlap.
		void* AITarget_Construct(void* target)
		{
			NXBasics::Runtime_FillMemory(target, 0, 12);
			NXBasics::Runtime_FillMemory(target, 0, 32);

			return target;
		}

		// Original function: FUN_00428a7a.
		void* AIMilitaryEnemyTargetArray_Construct(void* targets)
		{
			for (std::uint32_t i = 0; i < 20; ++i)
			{
				AITarget_Construct(static_cast<std::byte*>(targets) + i * 32);
			}

			return targets;
		}

		// Original function: FUN_00428a07.
		void* AIMilitaryManager_Construct(void* manager)
		{
			for (std::uint32_t i = 0; i < 100; ++i)
			{
				AITarget_Construct(static_cast<std::byte*>(manager) + 12 + i * 52);
			}

			AIMilitaryEnemyTargetArray_Construct(static_cast<std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::EnemyTargets);
			Write<std::uint16_t>(manager, NC2Layout::AIMilitaryManagerOffset::AttackedPositionY, 0);
			Write<std::uint16_t>(manager, NC2Layout::AIMilitaryManagerOffset::AttackedPosition, 0);

			return manager;
		}

		// Original function: FUN_00428a46.
		void* AIAnimalCatcher_Construct(void* manager)
		{
			Write<std::uint32_t>(manager, NC2Layout::AIAnimalCatcherOffset::TypeCount, 0);
			NXBasics::Runtime_FillMemory(static_cast<std::byte*>(manager) + NC2Layout::AIAnimalCatcherOffset::Target, 0, NC2Layout::RecordSize::AITargetReference);

			return manager;
		}

		// Original function: FUN_00428a61.
		void* AIResourcePointArray_Construct(void* array)
		{
			ConstructPoints(array, 0, 20, 20);

			return array;
		}

		// Original function: FUN_004535f7.
		void* AITradeManager_Construct(void* manager)
		{
			NXBasics::Runtime_FillMemory(manager, 0, 0xA4);

			return manager;
		}

		// Original function: FUN_00429328.
		void* AIHouseHumanArray_Construct(void* array)
		{
			ConstructPoints(array, 4, 4, 100);

			return array;
		}

		// Original function: FUN_00429301.
		void* AIHouseManager_Construct(void* manager)
		{
			ConstructPoints(manager, NC2Layout::AIHouseManagerOffset::Stocks, 12, 10);
			AIHouseHumanArray_Construct(static_cast<std::byte*>(manager) + NC2Layout::AIHouseManagerOffset::DefendPointCount);

			return manager;
		}

		// Original function: FUN_004292e8.
		void* AIVillageManager_Construct(void* manager)
		{
			Write<std::uint16_t>(manager, NC2Layout::AIVillageOffset::CenterY, 0);
			Write<std::uint16_t>(manager, NC2Layout::AIVillageOffset::CenterPoint, 0);
			AIHouseManager_Construct(static_cast<std::byte*>(manager) + NC2Layout::AIVillageOffset::Houses);

			return manager;
		}

		// Original function: FUN_004289cd.
		void* AIVillagesData_Construct(void* villages)
		{
			for (std::uint32_t i = 0; i < 8; ++i)
			{
				auto* village = static_cast<std::byte*>(villages) + NC2Layout::AIVillagesOffset::FirstVillage + i * NC2Layout::RecordSize::AIVillageSlot;
				AIVillageManager_Construct(village);
				Write<std::uint8_t>(village, NC2Layout::AIVillageOffset::AllocatedFlag, 0);
			}

			Write<std::uint16_t>(villages, NC2Layout::AIVillagesOffset::PendingVillageY, 0);
			Write<std::uint16_t>(villages, NC2Layout::AIVillagesOffset::PendingVillagePoint, 0);

			return villages;
		}
	}

	// Original function: FUN_00407dbd.
	void* AISinglePlayerHandler_Construct(void* handler)
	{
		AISinglePlayerHandler_Vars_Construct(handler);
		AISingleSoldierCalcArray_Construct(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOrigins);

		return handler;
	}

	// Original function: FUN_00428982.
	void* AIHighLevelHandler_Vars_Construct(void* handler)
	{
		auto* bytes = static_cast<std::byte*>(handler);
		NXBasics::Runtime_FillMemory(bytes + 5, 0, 21);
		AIVillagesData_Construct(bytes + NC2Layout::AIHighLevelOffset::Villages);
		AIResourcePointArray_Construct(bytes + NC2Layout::AIHighLevelOffset::ResourcePoints);
		AITradeManager_Construct(bytes + NC2Layout::AIHighLevelOffset::Trade);
		AIMilitaryManager_Construct(bytes + NC2Layout::AIHighLevelOffset::Military);
		AIAnimalCatcher_Construct(bytes + NC2Layout::AIHighLevelOffset::AnimalCatcher);

		return handler;
	}

	// Original function: FUN_0042951b.
	void* AIGuideManager_Construct(void* manager)
	{
		Write<std::uint16_t>(manager, NC2Layout::AIGuideManagerOffset::DestinationY, 0);
		Write<std::uint16_t>(manager, NC2Layout::AIGuideManagerOffset::Destination, 0);
		NXBasics::Runtime_FillMemory(manager, 0, NC2Runtime::AIGuideLimits::SerializedBytes);
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(manager) + NC2Layout::AIGuideManagerOffset::CalculationGrid, 0, NC2Layout::RecordSize::AIGuideGrid);

		return manager;
	}

	// Original function: FUN_00408fbb; the shared Windows vector-constructor loop.
	void __stdcall ConstructOriginalMemberArray(void* array, std::uint32_t stride, std::uint32_t count, void(__thiscall* constructor)(void*))
	{
		if (static_cast<std::int32_t>(count - 1) < 0)
		{

			return;
		}

		auto* element = static_cast<std::byte*>(array);
		do
		{
			constructor(element);
			element += stride;
		} while (--count != 0);
	}

	// Original function: FUN_0044e640; shared fish-swarm member construction.
	void* MapFishManager_ConstructMembers(void* manager)
	{
		for (std::uint32_t i = 0; i < 500; ++i)
		{
			Write<std::uint16_t>(manager, i * NC2Layout::RecordSize::FishSwarm + NC2Layout::FishSwarmOffset::MapPositionY, 0);
			Write<std::uint16_t>(manager, i * NC2Layout::RecordSize::FishSwarm, 0);
		}

		return manager;
	}
}