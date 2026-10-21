#include "C2AIAnimalDiplomacy.h"
#include "C2AIBuildingAccess.h"
#include "C2AIGlobalGoods.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIResourceAccess.h"
#include "C2AITaskAccess.h"
#include "C2AIVillageAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MapWalkSectorManager.h"
#include "C2MilitaryTargetSearch.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerCenterPosition.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "RemasterAIEconomy.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	// Remaster policy: only autonomous enemy expeditions use the wave controller.
	bool Remaster_AIMilitary_IsWaveTask(const void* handler, const void* task)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		if (!RemasterAIEconomy::IsActive(player) || Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::TaskType) != 2) return false;
		const auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		const auto* manager = static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::Military;
		const auto index = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex);
		if (index < 0 || index >= 100) return false;
		const auto* slot = manager + index * NC2Layout::RecordSize::AIMilitarySlot;
		if (Read<std::uint8_t>(slot, NC2Layout::AIMilitarySlotAccessOffset::Active) == 0) return false;
		// Do not take ownership of a combined task which also contains authored mission orders.
		return Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskCount) == 1 && Read<std::int32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices) == Read<std::int32_t>(slot, NC2Layout::AIMilitarySlotAccessOffset::MainTaskIndex);
	}
	std::uint32_t Remaster_AIMilitary_WantedArchers(const void* handler, std::uint32_t count)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto tribe = Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType);
		for (const auto weapon : { 16u, 17u })
		{
			const auto good = Read<std::uint32_t>(gStaticWeaponTypeArrays[tribe], weapon * NC2Layout::RecordSize::StaticWeaponType + NC2Layout::StaticWeaponTypeOffset::GoodType);
			if (good != 0 && Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Layout::RecordSize::TribeEquipmentEnables + good + NC2Layout::GameControlPlayerOffset::AllowedEquipment) != 0) return (count * 2 + 4) / 5;
		}
		return 0;
	}
	// Original function: FUN_00453efb; equipment tables retain tribe-specific records.
	void AIMilitaryManager_Init(void* manager, std::uint32_t player, const void* center, std::uint32_t radius)
	{
		for (std::uint32_t i = 0; i < 100; ++i) Write<std::uint8_t>(manager, NC2Layout::AIMilitarySlotAccessOffset::Active + i * NC2Layout::RecordSize::AIMilitarySlot, 0);
		Write<std::int32_t>(manager, 0x1454, -1);
		Write(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId, player);
		const auto tribe = static_cast<std::int32_t>(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
		for (std::uint32_t t = 0; t < NC2Runtime::AIEquipmentLimits::Tribes; ++t)
			for (std::uint32_t weapon = 0; weapon < NC2Runtime::AIEquipmentLimits::Weapons; ++weapon)
			{
				const auto* data = static_cast<const std::byte*>(gStaticWeaponTypeArrays[t]) + weapon * NC2Layout::RecordSize::StaticWeaponType;
				const auto offset = NC2Layout::AIMilitaryManagerOffset::WeaponGoods + (t * NC2Runtime::AIEquipmentLimits::Weapons + weapon) * NC2Layout::RecordSize::AIEquipmentRow;
				if (Read<std::uint32_t>(data, NC2Layout::StaticWeaponTypeOffset::MaximumRange) == 0)
				{
					Write<std::uint32_t>(manager, offset, 0); continue;
				}
				std::uint32_t best = 0; std::int32_t damage = 100000;
				for (std::uint32_t armor = 0; armor < NC2Runtime::AIEquipmentLimits::Armors; ++armor)
				{
					const auto* armorData = static_cast<const std::byte*>(gStaticArmorTypeData) + armor * NC2Layout::RecordSize::ArmorType;
					const auto candidate = Read<std::int32_t>(data, NC2Layout::StaticWeaponTypeOffset::DamageValues + Read<std::uint32_t>(armorData, NC2Layout::StaticArmorTypeOffset::MaterialType) * NC2Layout::RecordSize::WeaponDamageValue);
					if (candidate >= damage || Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Layout::StaticWeaponTypeOffset::DamageValues + Read<std::uint32_t>(armorData, NC2Layout::StaticArmorTypeOffset::GoodType) + NC2Layout::GameControlPlayerOffset::AllowedEquipment) == 0) continue;
					best = armor; damage = candidate;
				}
				Write(manager, offset + NC2Layout::AIEquipmentGoodViewOffset::PreferredType, best);
				if (weapon == 4 || weapon == 5 || weapon == 7 || weapon == 8 || weapon == 16 || weapon == 17) Write(manager, offset, Read<std::uint32_t>(data, NC2Layout::StaticWeaponTypeOffset::GoodType));
			}
		for (std::uint32_t armor = 0; armor < NC2Runtime::AIEquipmentLimits::Armors; ++armor)
		{
			const auto* data = static_cast<const std::byte*>(gStaticArmorTypeData) + armor * NC2Layout::RecordSize::ArmorType;
			Write(manager, NC2Layout::AIMilitaryManagerOffset::ArmorGoods + armor * NC2Layout::RecordSize::AIEquipmentRow, Read<std::uint32_t>(data, NC2Layout::StaticArmorTypeOffset::GoodType));
			std::uint32_t best = 0; std::int32_t damage = 0;
			for (std::uint32_t weapon = 0; weapon < NC2Runtime::AIEquipmentLimits::Weapons; ++weapon)
			{
				if (weapon != 4 && weapon != 5 && weapon != 7 && weapon != 8 && weapon != 16 && weapon != 17) continue;
				const auto* weaponData = static_cast<const std::byte*>(gStaticWeaponTypeArrays[tribe]) + weapon * NC2Layout::RecordSize::StaticWeaponType;
				if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Layout::StaticWeaponTypeOffset::DamageValues + Read<std::uint32_t>(weaponData, NC2Layout::StaticWeaponTypeOffset::GoodType) + NC2Layout::GameControlPlayerOffset::AllowedEquipment) == 0) continue;
				const auto candidate = Read<std::int32_t>(weaponData, NC2Layout::StaticWeaponTypeOffset::DamageValues + Read<std::uint32_t>(data, NC2Layout::StaticArmorTypeOffset::MaterialType) * NC2Layout::RecordSize::WeaponDamageValue);
				if (candidate > damage)
				{
					damage = candidate; best = weapon;
				}
			}
			Write(manager, NC2Layout::AIMilitaryManagerOffset::ArmorPreferredWeapon + armor * NC2Layout::RecordSize::AIEquipmentRow, best);
		}
		auto* single = Read<void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		if (Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription) == 1 && Read<std::uint16_t>(single, NC2Layout::AISinglePlayerOffset::FirstMainTaskPosition) == Read<std::uint16_t>(center, NC2Layout::MapMigPointOffset::MapX))
		{
			Write<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription, 0);
			AISinglePlayerHandler_MainTask_RecheckAll(single);
		}
		AISinglePlayerHandler_EntryPoint_SingleSoldier_SetDefaultPosition(single, center, radius * 2);
		for (std::uint32_t i = 0; i < 20; ++i) AIMilitaryPoint_Clear(static_cast<std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::EnemyTargets + i * NC2Layout::RecordSize::AIMilitaryPoint);
		Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor, -1);
		Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer, -1);
		Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex, -1);
	}
	// Original function: FUN_004545d4.
	void AIMilitaryManager_DoOneLogicGameTick(void* manager, void* high, std::uint32_t tick)
	{
		const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId);
		const bool balanced = RemasterAIEconomy::IsActive(player);
		auto* villages = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages;
		bool foodAvailable = AIVillagesData_Good_IsProducingGood(villages, 19, false);
		if (!foodAvailable && RemasterAIEconomy::IsActive(Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId)))
		{
			const auto workforce = RemasterAIEconomy::Inspect(Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId));
			// A stocked, staffed food chain may pause production while its output is being consumed.
			foodAvailable = workforce.foodVacancies == 0 && workforce.foodStock >= (workforce.population + 3u) / 4u && workforce.foodStock != 0;
		}
		if (foodAvailable || balanced)
		{
			if (AIVillagesData_Military_IsBarrackBuild(villages))
			{
				if (foodAvailable) AIMilitaryManager_l_Soldier_Create(manager, high);
				AIMilitaryManager_l_Soldier_CheckOthersEquipment(manager);
				AIMilitaryManager_l_Soldier_SetupCorrectEquipment(manager, high);
				AIMilitaryManager_l_EnemyTarget_CheckNext(manager, static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::FirstVillageCenter);
				if (tick % 10 == 0)
				{
					AIMilitaryManager_l_EnemyTarget_SelectCurrentTargetedEnemyPlayer(manager, static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::FirstVillageCenter);
					AIMilitaryManager_l_EnemyTarget_CheckForAttack(manager);
				}
			}
			else if (foodAvailable) AIVillagesData_House_BuildHouse(villages, 39, true, nullptr, 0);
		}
		if (tick % 10 == 0)
		{
			AIMilitaryManager_l_MilitaryPoints_Tower_Refresh(manager, high);
			AIMilitaryManager_l_MilitaryPoints_Villages_Refresh(manager, high);
		}
		if (tick % 5 == 0) AIMilitaryManager_l_MilitaryPoints_Update(manager, high);
	}
	// Original function: FUN_00454687; reservations remain queued until the human changes job.
	void AIMilitaryManager_l_Soldier_Create(void* manager, const void* high)
	{
		const auto* villages = static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages;
		const auto houseId = AIVillagesData_Military_GetBarrackHouseId(villages);
		if (houseId == -1) return;
		auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::ElementSize;
		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) return;
		const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId);
		auto count = Read<std::uint16_t>(PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player), NC2Layout::PlayerStatisticSampleOffset::SoldierCount);
		if (RemasterAIEconomy::IsActive(player))
		{
			const auto workforce = RemasterAIEconomy::Inspect(player);
			// Staff every completed workplace before converting genuinely spare adults.
			if (workforce.recruitmentVacancies != 0 || workforce.workers <= 12u || workforce.soldiers >= RemasterAIEconomy::MilitaryLimit()) return;
			const auto id = RemasterAIEconomy::FindWorker(player, House_Door_GetEntryPointRef(house), 0, true);
			if (id == -1 || count >= NC2Runtime::AIEquipmentLimits::RecruitmentLimit) return;
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			Human_DoAddNewUserCommand(human, 32, static_cast<std::uint8_t>(player), id, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			return;
		}
		if (count > NC2Runtime::AIEquipmentLimits::RecruitmentLimit) return;
		auto needed = static_cast<std::int32_t>(AIVillagesData_GetNumberOfUsedHumans(villages) * 120) / 100;
		CHumanIterator humans;
		for (auto id = humans.GetFirstSectorHumanId(player, false); id != -1; id = humans.GetNextSectorHumanId())
		{
			const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			if (Human_IsJobBuilder(human) && Human_IsAttachedToBuildSite(human)) continue;
			if (Human_IsAgeAdult(human) && Human_IsSexMale(human)) --needed;
		}
		while (needed < 0)
		{
			const auto id = AIHighLevelHandler_Tool_GetUnusedHuman(high, house + NC2Layout::HouseOffset::DoorEntryPoint);
			if (id == -1) return;
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			Human_DoAddNewUserCommand(human, 32, static_cast<std::uint8_t>(player), id, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			if (++count > NC2Runtime::AIEquipmentLimits::RecruitmentLimit) return;
			++needed;
		}
	}
	// Original function: FUN_0045479c.
	void AIMilitaryManager_l_Soldier_CheckOthersEquipment(void* manager)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Tribes * NC2Runtime::AIEquipmentLimits::Weapons; ++i) Write<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponWeights + i * NC2Layout::RecordSize::AIEquipmentRow, 0);
		for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Armors; ++i) Write<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorWeights + i * NC2Layout::RecordSize::AIEquipmentRow, 0);
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned) == 0 || player == Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId)) continue;
			const auto state = GameControl_Diplomacy_GetState(gGameControl, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId), player);
			if (state == 1) continue;
			const std::uint32_t weight = state == 3 ? 5 : 1;
			CHumanIterator humans;
			for (auto id = humans.GetFirstSectorHumanId(player, true); id != -1; id = humans.GetNextSectorHumanId())
			{
				const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
				const auto weapon = NC2Layout::AIMilitaryManagerOffset::WeaponWeights + (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::AIEquipmentLimits::Weapons + Human_Equipment_GetWeaponType(human)) * NC2Layout::RecordSize::AIEquipmentRow;
				Write(manager, weapon, Read<std::uint32_t>(manager, weapon) + weight);
				const auto armor = NC2Layout::AIMilitaryManagerOffset::ArmorWeights + Human_Equipment_GetArmorType(human) * NC2Layout::RecordSize::AIEquipmentRow;
				Write(manager, armor, Read<std::uint32_t>(manager, armor) + weight);
			}
		}
	}
	// Original function: FUN_004548aa.
	std::uint32_t AIMilitaryManager_l_Soldier_GetBestPossibleArmorType(void* manager, std::uint32_t armor, void* high)
	{
		auto* villages = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages; const auto* catcher = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::AnimalCatcher;
		if ((armor == 4 || armor == 3) && AIVillagesData_Good_IsProducingGood(villages, 32, true) && AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorGoods + armor * NC2Layout::RecordSize::AIEquipmentRow), true)) return armor;
		if ((!AIAnimalCatcher_IsType19Available(catcher) || armor == 2) && AIAnimalCatcher_IsType10Available(catcher) && AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorTwoGood), true)) return 2;
		if (AIAnimalCatcher_IsType19Available(catcher) && AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorOneGood), true)) return 1;
		return 0;
	}
	// Original function: FUN_00454958.
	std::uint32_t AIMilitaryManager_l_Soldier_GetBestPossibleWeaponType(void* manager, std::uint32_t weapon, void* high)
	{
		const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId);
		const auto tribe = static_cast<std::int32_t>(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
		auto* villages = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages;
		if ((weapon == 5 || weapon == 7 || weapon == 8) && AIVillagesData_Good_IsProducingGood(villages, 32, true) && AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponGoods + (tribe * NC2Runtime::AIEquipmentLimits::Weapons + weapon) * NC2Layout::RecordSize::AIEquipmentRow), true)) return weapon;
		if ((weapon == 16 || weapon == 17) && AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponGoods + (tribe * NC2Runtime::AIEquipmentLimits::Weapons + weapon) * NC2Layout::RecordSize::AIEquipmentRow), true)) return weapon;
		const auto good = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponFourGood + tribe * NC2Layout::RecordSize::AIMilitaryTribeEquipment);
		if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Layout::RecordSize::TribeEquipmentEnables + good + NC2Layout::GameControlPlayerOffset::AllowedEquipment) != 0 && AIVillagesData_Good_IsProducingGood(villages, good, true)) return 4;
		return 0;
	}
	// Original function: FUN_004557ed.
	void AIMilitaryManager_l_MilitaryPoints_Villages_Refresh(void* manager, const void* high)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Tribes; ++i)
		{
			const auto* entry = static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::FirstVillageActive + i * NC2Layout::RecordSize::AIVillageSlot;
			if (Read<std::uint8_t>(entry, NC2Layout::AIVillagePhysicalSlotOffset::Active) == 0) continue;
			std::byte target[12]{ };
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 5);
			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, Read<std::uint32_t>(entry, NC2Layout::AIVillagePhysicalSlotOffset::Center));
			Write<std::uint16_t>(target, NC2Layout::TargetDataOffset::ObjectRevisionHighWord, 0);
			AIMilitaryManager_MilitaryPoint_Add(manager, 1, 0, target, Read<std::uint32_t>(entry, NC2Layout::AIVillagePhysicalSlotOffset::Radius) + 30, 1, 5);
		}
	}
	// Original function: FUN_00455860; process one eligible enemy per invocation.
	void AIMilitaryManager_l_EnemyTarget_CheckNext(void* manager, const void* center)
	{
		Write(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor) + 1);
		for (;;)
		{
			const auto enemy = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor);
			if (enemy == 20)
			{
				Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor, -1); return;
			}
			const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId);
			if (static_cast<std::uint32_t>(enemy) != player && enemy < 20 && Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::GameControlPlayerSigned) != 0 && Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + enemy + NC2Layout::GameControlPlayerOffset::PlayerSeenFlags) != 0 && Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::DeadFlag) == 0)
			{
				auto* target = static_cast<std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::EnemyTargets + static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::AIMilitaryPoint;
				if (GameControl_Diplomacy_GetState(gGameControl, player, enemy) != 3) AIMilitaryPoint_Clear(target);
				else
				{
					std::uint32_t enemyCenter = 0; Tool_Misc_GetPlayerCenterPosition(enemy, &enemyCenter);
					if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(center), NC2Layout::MapMicroPointOffset::ContinentId) == Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&enemyCenter), NC2Layout::MapMicroPointOffset::ContinentId))
					{
						std::uint32_t form = 0;
						if (!CMapWalkSectorManager::AI_Military_GetFormPoint(player, center, &enemyCenter, &form)) form = Read<std::uint32_t>(center, NC2Layout::MapMigPointOffset::Coordinates);
						std::uint32_t candidate[3]{ };
						if (AIMilitaryManager_l_EnemyTarget_GetBestTarget(manager, enemy, &form, candidate)) AIMilitaryPoint_SetTarget(target, player, candidate, 60, 1, 3);
						else AIMilitaryPoint_Clear(target);
						return;
					}
				}
			}
			Write(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::EnemyScanCursor) + 1);
		}
	}
	// Original function: FUN_004559de; Windows repeats its first house-sector query on failure.
	bool AIMilitaryManager_l_EnemyTarget_GetBestTarget(const void* manager, std::uint32_t enemy, const void* point, void* target)
	{
		Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0); std::int32_t best = 100000;
		const auto player = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId); CHouseIterator houses;
		const auto searchHouses = [&] ()
		{
			for (auto id = houses.GetFirstSectorHouseId(enemy, point, 10000, false); id != -1; id = houses.GetNextSectorHouseId())
			{
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
				const auto distance = VE_HexagonDirection_GetDistance(Read<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointX), Read<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointY), Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapY));
				const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(house + NC2Layout::HouseOffset::DoorEntryPoint, player, 5);
				const auto score = static_cast<std::int32_t>(static_cast<std::uint32_t>(distance) + static_cast<std::uint32_t>(danger));
				if (score >= best) continue;
				best = score;
				Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, id);
				Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 4);
			}
		};
		searchHouses();
		if (!TargetData_IsValid(target)) searchHouses();
		if (!TargetData_IsValid(target))
		{
			CHumanIterator humans;
			for (auto id = humans.GetFirstSectorHumanId(enemy, false); id != -1; id = humans.GetNextSectorHumanId())
			{
				const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
				if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human)) continue;
				const auto* position = Human_GetMapPositionRef(human);
				const auto distance = VE_HexagonDirection_GetDistance(Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapY), Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapY));
				const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(Human_GetMapPositionRef(human), player, 5);
				const auto score = static_cast<std::int32_t>(static_cast<std::uint32_t>(distance) + static_cast<std::uint32_t>(danger));
				if (score >= best) continue;
				best = score;
				Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, id);
				Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 1);
			}
		}
		return TargetData_IsValid(target);
	}
	// Original function: FUN_00455449.
	void AIMilitaryManager_l_MilitaryPoints_Update(void* manager, void* high)
	{
		const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId); auto* single = Read<void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		const auto tribe = static_cast<std::int32_t>(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
		auto* villages = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages;
		for (std::uint32_t index = 0; index < 100; ++index)
		{
			auto* entry = static_cast<std::byte*>(manager) + index * NC2Layout::RecordSize::AIMilitarySlot;
			if (Read<std::uint8_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Active) == 0) continue;
			if (!AIMilitaryPoint_IsValid(entry + NC2Layout::AIMilitarySlotAccessOffset::Point))
			{
				AIMilitaryManager_MilitaryPoint_Clear(manager, index); continue;
			}
			std::uint32_t point = 0; TargetData_GetMapPosition(entry + NC2Layout::AIMilitarySlotAccessOffset::Point, &point); Tool_Map_GetNotBlockedPosition(&point, true);
			std::byte task[68]{ }; std::uint32_t maximum = 0, minimum = 0, priority = 0;
			const auto attack = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Role) == 2;
			const bool wave = attack && RemasterAIEconomy::IsActive(player) && Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::SecondaryType) == 1;
			if (attack)
			{
				auto danger = AIMilitaryManager_Military_GetDangerValueForPosition(&point, player);
				if (wave)
				{
					maximum = RemasterAIEconomy::WaveSize(RemasterAIEconomy::Inspect(player).population);
					minimum = maximum; priority = 100;
				}
				else if (danger > 0)
				{
					maximum = static_cast<std::int32_t>(static_cast<std::uint32_t>(danger) + 5) / 10;
					if (AIVillagesData_Good_IsProducingGood(villages, 6, false) && maximum > 1) maximum >>= 1;
					if (AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, tribe * NC2Layout::RecordSize::AIMilitaryTribeEquipment + NC2Layout::AIMilitaryManagerOffset::WeaponSeventeenGood), false) || AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, tribe * NC2Layout::RecordSize::AIMilitaryTribeEquipment + NC2Layout::AIMilitaryManagerOffset::WeaponFourGood), false) || AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorTwoGood), false) || AIVillagesData_Good_IsProducingGood(villages, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorOneGood), false)) if (maximum > 1) maximum >>= 1;
					minimum = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::TargetPolicy) == 3 ? maximum : (maximum >> 1) + (maximum & 1);
					if (Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MaximumSoldiers) != 0)
					{
						if (Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MaximumSoldiers) > maximum) maximum = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MaximumSoldiers);
						if (minimum > Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MinimumSoldiers)) minimum = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MinimumSoldiers) + 1;
					}
					Write(entry, NC2Layout::AIMilitarySlotAccessOffset::MaximumSoldiers, maximum);
					Write(entry, NC2Layout::AIMilitarySlotAccessOffset::MinimumSoldiers, minimum);
					const auto count = Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::SoldierCount);
					if (count != 0) priority = maximum * 100 / count;
				}
				AIMainTask_Init(task, 2, priority, 100000); Write(task, NC2Layout::AIMainTaskOffset::PrimaryPosition, point);
				std::uint32_t form = 0;
				if (wave || !CMapWalkSectorManager::AI_Military_GetFormPoint(player, static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::FirstVillageCenter, &point, &form)) form = Read<std::uint32_t>(high, NC2Layout::AIHighLevelOffset::FirstVillageCenter);
				if (wave) Tool_Map_GetNotBlockedPosition(&form, true);
				Write(task, NC2Layout::AIMainTaskOffset::SecondaryPosition, form);
				Write<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::MilitaryMode, 1);
			}
			else
			{
				const auto type = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::TargetPolicy);
				if (type == 2)
				{
					maximum = 2; priority = 5;
				}
				else if (type == 1 || type == 4)
				{
					const auto danger = AIMilitaryManager_Military_GetDangerValueForPosition(&point, player);
					if (danger > 5)
					{
						maximum = static_cast<std::int32_t>(static_cast<std::uint32_t>(danger) + 5) / 10; minimum = (maximum >> 1) + (maximum & 1);
						const auto count = Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::SoldierCount); if (count != 0) priority = maximum * 50 / count;
					}
				}
				AIMainTask_Init(task, 1, priority, 100000); Write(task, NC2Layout::AIMainTaskOffset::PrimaryPosition, point);
			}
			Write(task, NC2Layout::AIMainTaskOffset::Radius, wave ? 15u : Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Radius));
			Write(task, NC2Layout::AIMainTaskOffset::MinimumSoldiers, minimum);
			Write(task, NC2Layout::AIMainTaskOffset::MaximumSoldiers, maximum);
			const auto taskId = Read<std::int32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MainTaskIndex);
			if (taskId == -1)
			{
				const auto added = AISinglePlayerHandler_MainTask_Add(single, task); if (added != NXBasics::IntegerRange::UInt32Maximum) Write(entry, NC2Layout::AIMilitarySlotAccessOffset::MainTaskIndex, added);
				continue;
			}
			auto* old = static_cast<std::byte*>(single) + NC2Layout::AISinglePlayerOffset::MainTasks + static_cast<std::uint32_t>(taskId) * NC2Layout::RecordSize::AIMainTask;
			// Freeze a wave's target, size and rally point until the native target expires.
			if (wave && Read<std::uint32_t>(old, NC2Layout::AIMainTaskOffset::ConfiguredPriority) == 100 && Read<std::uint32_t>(old, NC2Layout::AIMainTaskOffset::Radius) == 15) continue;
			if (Read<std::uint32_t>(old, NC2Layout::AIMainTaskOffset::ConfiguredPriority) == priority && AITaskSimpleDescription_IsEqualTo(old + NC2Layout::AIMainTaskOffset::Description, task + 8)) continue;
			if (attack && !wave && Read<std::uint32_t>(old, NC2Layout::AIMainTaskOffset::ConfiguredPriority) != 0)
			{
				const auto formDistance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(old, NC2Layout::AIMainTaskOffset::SecondaryPosition), Read<std::uint16_t>(old, NC2Layout::AIMainTaskOffset::SecondaryPositionY), Read<std::uint16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPosition), Read<std::uint16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPositionY)));
				if (formDistance <= 60)
				{
					const auto targetDistance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(old, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::uint16_t>(old, NC2Layout::AIMainTaskOffset::PrimaryPositionY), Read<std::uint16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::uint16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPositionY)));
					if (targetDistance <= 60) continue;
				}
			}
			if (wave)
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::CombinedTaskCount); ++i)
				{
					auto* combined = static_cast<std::byte*>(single) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
					if (Remaster_AIMilitary_IsWaveTask(single, combined)) Write<std::uint32_t>(combined, NC2Layout::AICombinedTaskOffset::FormationRadius, 0);
				}
			for (std::uint32_t offset = 0; offset < 68; offset += 4) Write(old, offset, Read<std::uint32_t>(task, offset));
			AISinglePlayerHandler_MainTask_RecheckAll(single);
		}
	}
	// Original function: FUN_00454a3e; adjust queued soldiers before choosing equipment quotas.
	void AIMilitaryManager_l_Soldier_SetupCorrectEquipment(void* manager, void* high)
	{
		const auto player = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId); auto* single = Read<void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		const auto tribe = static_cast<std::int32_t>(player) < 20 ? Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
		auto count = Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::SoldierCount); if (count == 0) return;
		auto remaining = count; auto* villages = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages;
		const auto available = [&] (std::uint32_t good)
		{
			return Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Layout::RecordSize::TribeEquipmentEnables + good + NC2Layout::GameControlPlayerOffset::AllowedEquipment) != 0;
		};
		const auto weaponGood = [&] (std::uint32_t weapon)
		{
			return Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponGoods + (tribe * NC2Runtime::AIEquipmentLimits::Weapons + weapon) * NC2Layout::RecordSize::AIEquipmentRow);
		};
		const auto armorGood = [&] (std::uint32_t armor)
		{
			return Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorGoods + armor * NC2Layout::RecordSize::AIEquipmentRow);
		};
		const auto soldierAt = [&] (std::uint32_t i)
		{
			return static_cast<std::byte*>(single) + NC2Layout::AISinglePlayerOffset::Soldiers + i * NC2Layout::RecordSize::AISoldier;
		};
		const auto humanAt = [&] (const void* soldier)
		{
			return CHumanArray::mStaticVars + Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId) * CHumanArray::ElementSize;
		};
		const auto inVillage = [&] (const void* human)
		{
			return AIVillagesData_GetVillageId(villages, Human_GetMapPositionRef(human)) != -1;
		};
		const auto equip = [&] (void* human, std::uint8_t opcode, std::uint32_t good)
		{
			Human_DoAddNewUserCommand(human, opcode, static_cast<std::uint8_t>(player), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), good, 0);
		};
		if (RemasterAIEconomy::IsActive(player))
		{
			// One common 60:40 melee/bow policy, using each tribe's real goods and permissions.
			std::uint32_t melee = 0, bow = 0;
			for (const auto weapon : { 4u, 7u, 5u, 8u })
				if (weaponGood(weapon) != 0 && available(weaponGood(weapon))) { melee = weapon; break; }
			for (const auto weapon : { 16u, 17u })
				if (weaponGood(weapon) != 0 && available(weaponGood(weapon))) { bow = weapon; break; }
			// Request both chains independently, including their native buildings, workers and inputs.
			if (melee != 0) AIVillagesData_Good_IsProducingGood(villages, weaponGood(melee), true);
			if (bow != 0) AIVillagesData_Good_IsProducingGood(villages, weaponGood(bow), true);
			std::uint32_t rangedCount = 0;
			for (std::uint32_t i = 0; i < count; ++i)
			{
				const auto* soldier = soldierAt(i); const auto* human = humanAt(soldier);
				const auto weapon = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::ChangingEquipment ? Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType) : Human_Equipment_GetWeaponType(human);
				if (weapon == 16 || weapon == 17) ++rangedCount;
			}
			const auto wanted = bow == 0 ? 0u : (count * 2 + 4) / 5;
			for (std::uint32_t i = 0; i < count; ++i)
			{
				auto* soldier = soldierAt(i); auto* human = humanAt(soldier);
				if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::ChangingEquipment || !inVillage(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Human_UserControl_IsPriorityCommandInQueue(human) || Human_UserControl_IsCommandTypeInQueue(human, 81) || Human_UserControl_IsCommandTypeInQueue(human, 82)) continue;
				// Do not recall an expedition or interrupt fighting to rebalance its equipment.
				if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking) continue;
				if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) == 2)
				{
					const auto index = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex);
					if (index >= Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::CombinedTaskCount)) continue;
					const auto* task = static_cast<const std::byte*>(single) + NC2Layout::AISinglePlayerOffset::CombinedTasks + index * NC2Layout::RecordSize::AICombinedTask;
					if (!Remaster_AIMilitary_IsWaveTask(single, task) || Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) == 2) continue;
				}
				const auto current = Human_Equipment_GetWeaponType(human); const bool rangedNow = current == 16 || current == 17;
				const auto desired = rangedCount < wanted && !rangedNow ? bow : (rangedNow && rangedCount > wanted ? melee : (!Human_HasEquippedWeapon(human) ? melee : 0u));
				if (desired == 0 || !Human_CanEquipGoodType(human, weaponGood(desired)) || !Human_IsAbleToEquipGoodNow(human, weaponGood(desired), false)) continue;
				equip(human, 81, weaponGood(desired));
				if (rangedNow) --rangedCount;
				if (desired == 16 || desired == 17) ++rangedCount;
				Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
				Write(soldier, NC2Layout::AISoldierOffset::WeaponType, desired);
				Write(soldier, NC2Layout::AISoldierOffset::ArmorType, Human_Equipment_GetArmorType(human));
			}
			return; // Armor is optional; never request it as a prerequisite for a remaster army.
		}
		std::int32_t percentage = 0; std::uint32_t ranged = 0;
		if (available(weaponGood(17)) && AIVillagesData_Good_IsProducingGood(villages, weaponGood(17), true))
		{
			percentage = 30; ranged = 17;
		}
		else if (available(weaponGood(16)) && AIVillagesData_Good_IsProducingGood(villages, weaponGood(16), true))
		{
			percentage = 40; ranged = 16;
		}
		std::uint32_t weaponWeight = 0, armorWeight = 0;
		for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Tribes * NC2Runtime::AIEquipmentLimits::Weapons; ++i) weaponWeight += Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponWeights + i * NC2Layout::RecordSize::AIEquipmentRow);
		for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Armors; ++i) armorWeight += Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorWeights + i * NC2Layout::RecordSize::AIEquipmentRow);
		std::int32_t weapons[NC2Runtime::AIEquipmentLimits::Weapons]{ }, armors[5]{ };
		for (std::uint32_t i = 0; i < count; ++i)
		{
			auto* soldier = soldierAt(i); const auto state = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction);
			if (state == SwitchIds::AISoldierAction::ChangingEquipment || state == SwitchIds::AISoldierAction::Attacking)
			{
				--remaining; continue;
			}
			auto* human = humanAt(soldier); const auto weapon = Human_Equipment_GetWeaponType(human);
			Write(soldier, NC2Layout::AISoldierOffset::WeaponType, weapon); ++weapons[weapon];
			const auto armor = Human_Equipment_GetArmorType(human);
			Write(soldier, NC2Layout::AISoldierOffset::ArmorType, armor); ++armors[armor];
		}
		const auto wantedRanged = static_cast<std::int32_t>(static_cast<std::uint32_t>(percentage) * remaining) / 100;
		if (weapons[ranged] < wantedRanged)
		{
			auto needed = wantedRanged - weapons[ranged]; if (ranged == 16) needed -= weapons[17];
			if (needed > 0)
			{
				auto reserved = ranged == 17 ? (weapons[16] <= needed ? weapons[16] : needed) : 0;
				auto fresh = reserved < needed ? needed - reserved : 0;
				for (std::uint32_t i = 0; i < count && needed != 0; ++i)
				{
					auto* soldier = soldierAt(i); const auto state = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction);
					if (state == SwitchIds::AISoldierAction::ChangingEquipment || state == SwitchIds::AISoldierAction::Attacking) continue;
					const auto current = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType);
					if (current == 16)
					{
						if (ranged == 17)
						{
							auto* human = humanAt(soldier);
							if (inVillage(human))
							{
								equip(human, 81, weaponGood(17)); --weapons[current]; --remaining; --needed;
								Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
								Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType, 17);
							}
							else
							{
								++reserved; if (weapons[16] < reserved) ++fresh;
							}
						}
						continue;
					}
					if (current == 17 || fresh == 0) continue;
					auto* human = humanAt(soldier); if (!inVillage(human)) continue;
					equip(human, 81, weaponGood(ranged)); --weapons[current]; --remaining; --needed; --fresh;
					Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
					Write(soldier, NC2Layout::AISoldierOffset::WeaponType, ranged);
				}
			}
		}
		else remaining -= static_cast<std::uint32_t>(wantedRanged);
		if (remaining != 0)
		{
			std::uint32_t wantedWeapons[NC2Runtime::AIEquipmentLimits::Weapons]{ }, wantedArmors[5]{ };
			std::uint32_t possibleWeapons[NC2Runtime::AIEquipmentLimits::Weapons], possibleArmors[5];
			if (weaponWeight != 0)
				for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Tribes * NC2Runtime::AIEquipmentLimits::Weapons; ++i)
				{
					const auto amount = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponWeights + i * NC2Layout::RecordSize::AIEquipmentRow) * remaining / weaponWeight;
					if (amount != 0) wantedArmors[Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::WeaponPreferredArmor + i * NC2Layout::RecordSize::AIEquipmentRow)] += amount;
				}
			if (armorWeight != 0)
				for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Armors; ++i)
				{
					const auto amount = Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorWeights + i * NC2Layout::RecordSize::AIEquipmentRow) * remaining / armorWeight;
					if (amount != 0) wantedWeapons[Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::ArmorPreferredWeapon + i * NC2Layout::RecordSize::AIEquipmentRow)] += amount;
				}
			for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Weapons; ++i) if (wantedWeapons[i] != 0) possibleWeapons[i] = AIMilitaryManager_l_Soldier_GetBestPossibleWeaponType(manager, i, high);
			for (std::uint32_t i = 0; i < NC2Runtime::AIEquipmentLimits::Armors; ++i) if (wantedArmors[i] != 0) possibleArmors[i] = AIMilitaryManager_l_Soldier_GetBestPossibleArmorType(manager, i, high);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				auto* soldier = soldierAt(i); const auto state = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction);
				if (state == SwitchIds::AISoldierAction::ChangingEquipment || state == SwitchIds::AISoldierAction::Attacking) continue;
				auto* human = humanAt(soldier); if (!inVillage(human)) continue;
				const auto weapon = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType);
				if (weapon != 17 && weapon != 16 && weapons[weapon] > static_cast<std::int32_t>(wantedWeapons[weapon]) && weaponWeight != 0)
					for (std::uint32_t next = 1; next < NC2Runtime::AIEquipmentLimits::Weapons; ++next)
					{
						if (weapons[next] >= static_cast<std::int32_t>(wantedWeapons[next]) || possibleWeapons[next] == 0) continue;
						equip(human, 81, weaponGood(possibleWeapons[next])); ++weapons[next]; --weapons[weapon];
						Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
						Write(soldier, NC2Layout::AISoldierOffset::WeaponType, next); break;
					}
				const auto armor = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::ArmorType);
				if (armors[armor] > static_cast<std::int32_t>(wantedArmors[armor]) && armorWeight != 0)
					for (std::uint32_t next = 1; next < 5; ++next)
					{
						if (armors[next] >= static_cast<std::int32_t>(wantedArmors[next]) || possibleArmors[next] == 0) continue;
						equip(human, 82, armorGood(possibleArmors[next])); ++armors[next]; --armors[armor];
						Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
						Write(soldier, NC2Layout::AISoldierOffset::ArmorType, next); break;
					}
			}
		}
		std::uint32_t defaultWeapon = 1, defaultArmor = 0;
		if (available(weaponGood(4))) defaultWeapon = AIVillagesData_Good_IsProducingGood(villages, weaponGood(4), true) ? 4 : 1;
		else if (available(weaponGood(7)) && AIVillagesData_Good_IsProducingGood(villages, weaponGood(7), true) && AIVillagesData_Good_IsProducingGood(villages, 32, true)) defaultWeapon = 7;
		const auto* catcher = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::AnimalCatcher;
		if (AIAnimalCatcher_IsType10Available(catcher))
		{
			if (AIVillagesData_Good_IsProducingGood(villages, armorGood(2), true)) defaultArmor = 2;
		}
		else if (AIAnimalCatcher_IsType19Available(catcher) && AIVillagesData_Good_IsProducingGood(villages, armorGood(1), true)) defaultArmor = 1;
		for (std::uint32_t i = 0; i < count; ++i)
		{
			auto* soldier = soldierAt(i); const auto state = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction);
			if (state == SwitchIds::AISoldierAction::ChangingEquipment || state == SwitchIds::AISoldierAction::Attacking) continue;
			if ((Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType) != 1 || defaultWeapon == 1) && (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::ArmorType) != 0 || defaultArmor == 0)) continue;
			auto* human = humanAt(soldier); if (!inVillage(human)) continue;
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType) == 1 && defaultWeapon != 1)
			{
				equip(human, 81, weaponGood(defaultWeapon));
				Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
				Write(soldier, NC2Layout::AISoldierOffset::WeaponType, defaultWeapon);
			}
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::ArmorType) == 0 && defaultArmor != 0)
			{
				equip(human, 82, armorGood(defaultArmor));
				Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::ChangingEquipment);
				Write(soldier, NC2Layout::AISoldierOffset::ArmorType, defaultArmor);
			}
		}
	}
	namespace
	{
		// Original function: FUN_00455398; only the first four DWORDs participate.
		bool AIMilitaryPoint_IsEqualTo(const void* point, const void* other)
		{
			return Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::TargetUniqueId) == Read<std::uint32_t>(other, NC2Layout::AIMilitaryPointOffset::TargetUniqueId) && Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::TargetIdOrMapPoint) == Read<std::uint32_t>(other, NC2Layout::AIMilitaryPointOffset::TargetIdOrMapPoint) && Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::Target) == Read<std::uint32_t>(other, NC2Layout::AIMilitaryPointOffset::Target) && Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::TargetPolicy) == Read<std::uint32_t>(other, NC2Layout::AIMilitaryPointOffset::TargetPolicy);
		}
	}
	// Original function: FUN_00454578.
	bool AIMilitaryManager_Military_IsAnyTargetInRange(std::uint32_t player, const void* point, std::uint32_t radius)
	{
		if (AI_IsEnemyObjectAround(player, point, radius)) return true;
		if (Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, false, 1, 0, true, false) != -1) return true;
		return Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, false, 1, 0) != -1;
	}
	// Original function: FUN_00472a8e.
	void AIMilitaryPoint_Clear(void* point)
	{
		Write<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::TargetPolicy, 0);
		Write<std::int32_t>(point, NC2Layout::AIMilitaryPointOffset::MainTaskIndex, -1);
	}
	// Original function: FUN_00472a97.
	bool AIMilitaryPoint_l_IsValid(const void* point)
	{
		const auto type = Read<std::int32_t>(point, NC2Layout::AIMilitaryPointOffset::TargetPolicy);
		if (type <= 0) return false;
		if (type <= 3) return TargetData_IsValid(point);
		if (type != 4) return false;
		std::uint32_t position = 0;
		TargetData_GetMapPosition(point, &position);
		return Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&position), NC2Layout::MapMicroPointOffset::ObjectType) != 3;
	}
	// Original function: FUN_00472ae7.
	bool AIMilitaryPoint_IsValid(const void* point)
	{
		if (!AIMilitaryPoint_l_IsValid(point)) return false;
		const auto lifetime = Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::LifetimePolicy);
		if (lifetime == 0) return false;
		if (lifetime != 2 && lifetime != 4 && lifetime != 5) return true;
		std::uint32_t position = 0;
		TargetData_GetMapPosition(point, &position);
		const auto player = Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::PlayerId);
		const auto radius = Read<std::uint32_t>(point, NC2Layout::AIMilitaryPointOffset::Radius);
		if (lifetime == 2) return AI_IsEnemyObjectAround(player, &position, radius);
		if (lifetime == 4) return AIMilitaryManager_Military_IsAnyTargetInRange(player, &position, radius);
		CHouseIterator iterator;
		if (iterator.GetFirstSectorHouseId(player, &position, radius, false) != -1) return true;
		return iterator.GetFirstSectorHouseId(player, &position, radius, true) != -1;
	}
	// Original function: FUN_00472bd7.
	void AIMilitaryPoint_SetTarget(void* point, std::uint32_t player, const void* target, std::uint32_t radius, std::uint32_t type, std::uint32_t lifetime)
	{
		Write(point, NC2Layout::AIMilitaryPointOffset::PlayerId, player);
		Write(point, NC2Layout::AIMilitaryPointOffset::Radius, radius);
		for (std::uint32_t i = 0; i < 12; i += 4) Write(point, i, Read<std::uint32_t>(target, i));
		Write(point, NC2Layout::AIMilitaryPointOffset::TargetPolicy, type);
		Write(point, NC2Layout::AIMilitaryPointOffset::LifetimePolicy, lifetime);
	}
	// Original function: FUN_004552f1; slot accesses use a base four bytes before the physical 52-byte record.
	std::int32_t AIMilitaryManager_MilitaryPoint_Add(void* manager, std::uint32_t type, std::uint32_t secondaryType, const void* target, std::uint32_t radius, std::uint32_t targetType, std::uint32_t lifetime)
	{
		std::byte point[32]{ };
		AIMilitaryPoint_Clear(point);
		AIMilitaryPoint_SetTarget(point, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId), target, radius, targetType, lifetime);
		std::int32_t free = -1;
		for (std::int32_t i = 0; i < 100; ++i)
		{
			const auto* entry = static_cast<const std::byte*>(manager) + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::AIMilitarySlot;
			if (Read<std::uint8_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Active) != 0)
			{
				if (AIMilitaryPoint_IsEqualTo(point, entry + NC2Layout::AIMilitarySlotAccessOffset::Point)) return i;
			}
			else if (free == -1) free = i;
		}
		if (free == -1) return free;
		auto* entry = static_cast<std::byte*>(manager) + static_cast<std::uint32_t>(free) * NC2Layout::RecordSize::AIMilitarySlot;
		for (std::uint32_t i = 0; i < 32; i += 4) Write(entry, NC2Layout::AIMilitarySlotAccessOffset::Point + i, Read<std::uint32_t>(point, i));
		Write(entry, NC2Layout::AIMilitarySlotAccessOffset::Role, type);
		Write<std::uint8_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Active, 1);
		Write(entry, NC2Layout::AIMilitarySlotAccessOffset::SecondaryType, secondaryType);
		Write<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MinimumSoldiers, 0);
		Write<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MaximumSoldiers, 0);
		return free;
	}
	// Original function: FUN_004553c4.
	void AIMilitaryManager_MilitaryPoint_Clear(void* manager, std::int32_t index)
	{
		auto* entry = static_cast<std::byte*>(manager) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::AIMilitarySlot;
		if (Read<std::uint8_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Active) != 0)
		{
			const auto task = Read<std::int32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::MainTaskIndex);
			if (task != -1)
			{
				void* single = Read<void*>(gAIManager, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId) * NC2Layout::RecordSize::AIPlayerHandlerPointer);
				Write<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::FirstMainTaskDescriptionUnsigned + static_cast<std::uint32_t>(task) * NC2Layout::RecordSize::AIMainTask, 0);
				AISinglePlayerHandler_MainTask_RecheckAll(single);
				AIMilitaryPoint_Clear(entry + NC2Layout::AIMilitarySlotAccessOffset::Point);
			}
			Write<std::uint8_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::Active, 0);
		}
		const auto secondary = Read<std::uint32_t>(entry, NC2Layout::AIMilitarySlotAccessOffset::SecondaryType);
		if (secondary == 1 && Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex) == index) AIMilitaryManager_l_EnemyTarget_ClearCurrentTargetedEnemyPlayer(manager);
		else if (secondary == 2)
		{
			MapMigPoint_Construct(static_cast<std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::AttackedPosition, -1, -1, false);
			Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::AttackedPositionMilitaryPointIndex, -1);
		}
	}
	// Original function: FUN_00455cc2; clear the reference only after the original nested clear.
	void AIMilitaryManager_l_EnemyTarget_ClearCurrentTargetedEnemyPlayer(void* manager)
	{
		const auto index = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex);
		if (index != -1 && Read<std::uint8_t>(manager, static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::AIMilitarySlot + NC2Layout::AIMilitarySlotAccessOffset::Active) != 0) AIMilitaryManager_MilitaryPoint_Clear(manager, index);
		Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex, -1);
		Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer, -1);
	}
	// Original function: FUN_00455c0c.
	void AIMilitaryManager_l_EnemyTarget_SelectCurrentTargetedEnemyPlayer(void* manager, const void* center)
	{
		if (Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer) != -1) return;
		std::int32_t best = -1;
		std::uint32_t score = 100000;
		for (std::int32_t i = 0; i < 20; ++i)
		{
			const auto* target = static_cast<const std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::EnemyTargets + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::AIMilitaryPoint;
			if (!AIMilitaryPoint_IsValid(target)) continue;
			std::uint32_t position = 0;
			TargetData_GetMapPosition(target, &position);
			Tool_Map_GetNotBlockedPosition(&position, true);
			const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(&position, Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId), 5);
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::int16_t>(center, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(center, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(&position, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(&position, NC2Layout::MapMigPointOffset::MapY));
			const auto candidate = static_cast<std::uint32_t>(danger) + static_cast<std::uint32_t>(distance);
			if (static_cast<std::int32_t>(candidate) >= static_cast<std::int32_t>(score)) continue;
			score = candidate; best = i;
		}
		Write(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer, best);
	}
	// Original function: FUN_00455cf4.
	void AIMilitaryManager_l_EnemyTarget_CheckForAttack(void* manager)
	{
		const auto selected = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex);
		if (RemasterAIEconomy::IsActive(Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId)) && selected >= 0 && selected < 100)
		{
			const auto* slot = static_cast<const std::byte*>(manager) + selected * NC2Layout::RecordSize::AIMilitarySlot;
			const auto* previous = slot + NC2Layout::AIMilitarySlotAccessOffset::Point;
			if (Read<std::uint8_t>(slot, NC2Layout::AIMilitarySlotAccessOffset::Active) != 0 && AIMilitaryPoint_IsValid(previous) && GameControl_Diplomacy_GetState(gGameControl, Read<std::uint32_t>(manager, NC2Layout::AIMilitaryManagerOffset::PlayerId), TargetData_GetPlayerId(previous)) == 3) return;
		}
		const auto enemy = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer);
		if (enemy == -1) return;
		const auto* target = static_cast<const std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::EnemyTargets + static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::AIMilitaryPoint;
		if (!AIMilitaryPoint_IsValid(target))
		{
			AIMilitaryManager_l_EnemyTarget_ClearCurrentTargetedEnemyPlayer(manager); return;
		}
		const auto index = Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex);
		if (index != -1)
		{
			const auto* previous = static_cast<const std::byte*>(manager) + static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::AIMilitarySlot + NC2Layout::AIMilitarySlotAccessOffset::Point;
			// Windows compares the current enemy index here, rather than the military-point index.
			if (Read<std::uint32_t>(previous, NC2Layout::AIMilitaryPointOffset::TargetUniqueId) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) && Read<std::uint32_t>(previous, NC2Layout::AIMilitaryPointOffset::Target) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type)) return;
			Write<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex, -1);
			AIMilitaryManager_MilitaryPoint_Clear(manager, index);
		}
		Write(manager, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex, AIMilitaryManager_MilitaryPoint_Add(manager, 2, 1, target, 60, 3, 3));
	}
	// Original function: FUN_00455da5; the Windows branch handles an animal target (type 3).
	void AIMilitaryManager_Message_Soldier_Attacked(void* manager, const void* target)
	{
		if (TargetData_GetTargetType(target) != 3 || Read<std::int32_t>(manager, NC2Layout::AIMilitaryManagerOffset::AttackedPositionMilitaryPointIndex) != -1) return;
		auto* position = static_cast<std::byte*>(manager) + NC2Layout::AIMilitaryManagerOffset::AttackedPosition;
		TargetData_GetMapPosition(target, position);
		std::byte mapTarget[12]{ };
		Write<std::uint32_t>(mapTarget, NC2Layout::TargetDataOffset::Type, 5);
		Write(mapTarget, NC2Layout::MapTargetDataOffset::MapX, Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapX)); Write(mapTarget, NC2Layout::MapTargetDataOffset::MapY, Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapY));
		Write(manager, NC2Layout::AIMilitaryManagerOffset::AttackedPositionMilitaryPointIndex, AIMilitaryManager_MilitaryPoint_Add(manager, 2, 2, mapTarget, 30, 1, 2));
	}
	// Original function: FUN_0045522f.
	void AIMilitaryManager_l_MilitaryPoints_Tower_Refresh(void* manager, const void* high)
	{
		for (std::uint32_t village = 0; village < 8; ++village)
		{
			const auto* entry = static_cast<const std::byte*>(high) + village * NC2Layout::RecordSize::AIVillageSlot;
			if (Read<std::uint8_t>(entry, NC2Layout::AIHighLevelVillageViewOffset::Active) == 0) continue;
			std::byte target[12]{ };
			for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIHighLevelVillageViewOffset::DefendPointCount); ++i)
			{
				const auto* micro = GameMap_GetConstMapMicroPointPtr(entry + NC2Layout::AIHighLevelVillageViewOffset::DefendPoints + i * NC2Layout::RecordSize::MapMigPoint);
				const auto houseId = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? static_cast<std::int32_t>(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId)) : -1;
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::ElementSize;
				if (House_GetHouseMainType(house) != 5) continue;
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 4);
				Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, houseId);
				Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				AIMilitaryManager_MilitaryPoint_Add(manager, 1, 0, target, 30, 2, 3);
			}
		}
	}
	// Original function: FUN_004293fc.
	void __fastcall AIHighLevelHandler_Message_Soldier_Attacked(void* handler, void* /*unusedEdx*/, const void* target)
	{
		AIMilitaryManager_Message_Soldier_Attacked(static_cast<std::byte*>(handler) + NC2Layout::AIHighLevelOffset::Military, target);
	}
}
