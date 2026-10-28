#include "C2AIAssistantAccess.h"
#include "C2CallbackManager.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanUserControl.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2VehicleAccess.h"
#include "XNumericConstants.h"
#include <initializer_list>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		std::byte* Human(std::uint32_t id)
		{
			return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
		}
		std::byte* House(std::uint32_t id)
		{
			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}
		std::int32_t Distance(const void* a, const void* b)
		{
			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y));
		}
		void Shuffle(std::uint32_t* ids, std::uint32_t count)
		{
			if (count <= 1) return;
			for (std::uint32_t i = 0; i < count * 4; ++i)
			{
				const auto first = Game_Random_GetNumber() % count;
				const auto second = Game_Random_GetNumber() % count;
				const auto id = ids[first]; ids[first] = ids[second]; ids[second] = id;
			}
		}
		void DecrementIfNonzero(void* data, std::uint32_t offset)
		{
			const auto value = Read<std::uint32_t>(data, offset);
			if (value != 0) Write(data, offset, value - 1);
		}
	}
	// Original function: FUN_00418ced.
	void GameAssistantManager_l_Human_Command_Add(void*, std::uint32_t id, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2, std::uint32_t duration)
	{
		auto* human = Human(id);
		Human_DoAddNewUserCommand(human, static_cast<std::uint8_t>(command), Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), id, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), parameter1, parameter2);
		if (duration != 0) GameAssistantSingleHumanData_CommandHistory_Add(human + NC2Layout::HumanOffset::AssistantData, command, parameter1, parameter2, duration);
	}
	// Original function: FUN_00418d3b.
	bool __cdecl GameAssistantManager_ls_LogicCallback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueParameter1, std::uintptr_t, std::uintptr_t)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		if (type == 1 && parameter1 % 12 == 0)
			for (std::int32_t player = 0; player < 20; ++player) GameAssistantManager_l_Assistant_DoWork(gGameAssistantManager, player, parameter1 / 12);
		return true;
	}
	// Original function: FUN_00418f65.
	bool GameAssistantManager_ls_Tool_GetHQPosition(std::uint32_t player, void* point)
	{
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = House(id);
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 1) continue;
			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint)); return true;
		}
		return false;
	}
	// Original function: FUN_00418faf.
	bool GameAssistantManager_ls_Tool_GetCenteredStockPosition(std::uint32_t player, void* point)
	{
		std::int32_t x = 0, y = 0, count = 0;
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = House(id);
			if (House_GetHouseMainType(house) != 1) continue;
			x += Read<std::int16_t>(house, NC2Layout::HouseOffset::DoorEntryPointX); y += Read<std::int16_t>(house, NC2Layout::HouseOffset::DoorEntryPointY); ++count;
		}
		if (count == 0) return false;
		MapMigPoint_SetPosition(point, static_cast<std::int16_t>(x / count), static_cast<std::int16_t>(y / count), false); return true;
	}
	// Original function: FUN_00418d79.
	std::int32_t GameAssistantManager_ls_Tool_FindWomanToGetChild(std::uint32_t player)
	{
		std::uint32_t candidates[10], count = 0;
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
		{
			const auto* human = Human(id);
			if (!Human_IsAgeAdult(human) || !Human_IsSexFemale(human) || Human_IsJobHero(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) == NXBasics::IntegerRange::UInt32Maximum || Human_UserControl_DoesWantToGetChild(human)) continue;
			SUserCommandsForHumanHandler commands;
			Human_UserControl_GetCommands(human, &commands, false, false);
			if (!commands.IsCommandAllowed(8) && !commands.IsCommandAllowed(9)) continue;
			candidates[count++] = id; if (count == 10) break;
		}
		if (count == 0) return -1;
		if (count > 1)
		{
			std::uint32_t center = 0;
			if (GameAssistantManager_ls_Tool_GetHQPosition(player, &center) || GameAssistantManager_ls_Tool_GetCenteredStockPosition(player, &center))
				for (std::uint32_t pass = count - 1; pass != 0; --pass)
					for (std::uint32_t i = 0; i < count - 1; ++i)
					{
						const auto* first = House(Read<std::uint32_t>(Human(candidates[i]), NC2Layout::HumanOffset::HomeHouseId)) + 36;
						const auto* second = House(Read<std::uint32_t>(Human(candidates[i + 1]), NC2Layout::HumanOffset::HomeHouseId)) + 36;
						const auto firstDistance = Distance(&center, first), secondDistance = Distance(&center, second);
						if (firstDistance > secondDistance)
						{
							const auto id = candidates[i]; candidates[i] = candidates[i + 1]; candidates[i + 1] = id;
						}
					}
		}
		return candidates[0];
	}
	// Original function: FUN_004176c8; all native assistant phases run in their original order.
	void GameAssistantManager_l_Assistant_DoWork(void* manager, std::int32_t player, std::uint32_t tick)
	{
		if (!GameAssistantManager_l_Assistant_IsPlayerIdOk(manager, player)) return;
		auto* data = static_cast<std::byte*>(manager) + static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameAssistantPlayer;
		const auto desiredBirths = [&] ()
		{
			return Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedBoys) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedUnarmedSoldiers) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSpearmen) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedArchers) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSwordsmen);
		};
		const auto desiredRecruits = [&] ()
		{
			return Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedUnarmedSoldiers) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSpearmen) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedArchers) + Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSwordsmen);
		};
		if (tick % 180 == 0)
		{
			const auto births = Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget);
			if (static_cast<std::int32_t>(births) < static_cast<std::int32_t>(desiredBirths())) Write(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget, births + 1);
			const auto recruits = Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget);
			if (static_cast<std::int32_t>(recruits) < static_cast<std::int32_t>(desiredRecruits())) Write(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget, recruits + 1);
		}
		const std::uint32_t targets[6] = { desiredBirths(), Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedGirls), desiredRecruits(), Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSpearmen), Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedArchers), Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RequestedSwordsmen) };
		const std::int32_t caps[6] = { 100, 20, 80, 20, 20, 20 };
		for (std::uint32_t i = 0; i < 6; ++i)
		{
			auto value = Read<std::int32_t>(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget + i * NC2Layout::RecordSize::AssistantRequestValue);
			if (value >= static_cast<std::int32_t>(targets[i])) value = static_cast<std::int32_t>(targets[i]);
			if (value <= 0) value = 0;
			if (value >= caps[i]) value = caps[i];
			Write(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget + i * NC2Layout::RecordSize::AssistantRequestValue, value);
		}
		if (tick % 15 == 0)
		{
			if (Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget) != 0)
			{
				const auto id = GameAssistantManager_ls_Tool_FindWomanToGetChild(player);
				if (id != -1)
				{
					GameAssistantManager_l_Human_Command_Add(manager, id, 15, 0, 0, 3); Write(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget, Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget) - 1); DecrementIfNonzero(data, NC2Layout::GameAssistantPlayerOffset::RequestedBoys);
				}
			}
			if (Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::GirlRequestBudget) != 0)
			{
				const auto id = GameAssistantManager_ls_Tool_FindWomanToGetChild(player);
				if (id != -1)
				{
					GameAssistantManager_l_Human_Command_Add(manager, id, 16, 0, 0, 3); Write(data, NC2Layout::GameAssistantPlayerOffset::GirlRequestBudget, Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::GirlRequestBudget) - 1); DecrementIfNonzero(data, NC2Layout::GameAssistantPlayerOffset::RequestedGirls);
				}
			}
		}
		const auto equipItems = [&] (std::uint32_t good, bool soldiers, bool tool, bool amulet)
		{
			std::uint32_t candidates[100], count = 0;
			CHumanIterator iterator;
			for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
			{
				const auto* human = Human(id);
				if (!Human_IsAgeAdult(human) || Human_IsJobHero(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0) continue;
				if (amulet)
				{
					if (Human_IsSexFemale(human) || Human_IsJobSoldier(human) != soldiers || Human_HasEquippedItemType(human, 43, -1)) continue;
				}
				else if (tool)
				{
					if (!Human_CanEquipToolIron(human) || Human_HasEquippedTools(human)) continue;
				}
				else if (!Human_CanEquipShoes(human) || Human_IsJobSoldier(human) != soldiers || Human_HasEquippedShoes(human)) continue;
				if (GameAssistantSingleHumanData_CommandHistory_IsInArray(const_cast<std::byte*>(human) + NC2Layout::HumanOffset::AssistantData, 83, good, 0)) continue;
				candidates[count++] = id; if (count == 100) break;
			}
			Shuffle(candidates, count);
			for (std::uint32_t i = 0; i < count && i < 6; ++i)
				if (Human_IsAbleToEquipGoodNow(Human(candidates[i]), good, true)) GameAssistantManager_l_Human_Command_Add(manager, candidates[i], 83, good, 0, 3);
		};
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipCivilianShoes) != 0 && (tick - 2u) % 30 == 0) equipItems(30, false, false, false);
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipCivilianWoodenTools) != 0 && (tick - 4u) % 30 == 0) equipItems(31, false, true, false);
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipCivilianIronTools) != 0 && (tick - 6u) % 30 == 0) equipItems(32, false, true, false);
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipCivilianMead) != 0 && (tick - 8u) % 30 == 0) equipItems(43, false, false, true);
		if ((tick - 10u) % 30 == 0 && Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget) != 0)
		{
			std::uint32_t barracks[5], barrackCount = 0;
			CHouseIterator houses;
			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				const auto* house = House(id);
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 39) continue;
				barracks[barrackCount++] = id; if (barrackCount == 5) break;
			}
			if (barrackCount != 0)
			{
				std::uint32_t candidates[50], count = 0;
				CHumanIterator humans;
				for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
				{
					const auto* human = Human(id);
					if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human) || Human_IsJobHero(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 6 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) != NXBasics::IntegerRange::UInt32Maximum) continue;
					const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId);
					std::uint32_t b = 0;
					while (b < barrackCount && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(House(barracks[b]) + 28), NC2Layout::MapMicroPointOffset::ContinentId) != continent) ++b;
					if (b == barrackCount) continue;
					candidates[count++] = id; if (count == 50) break;
				}
				struct SRecruit
				{
					std::uint32_t human, house; std::int32_t distance;
				} recruits[50];
				std::uint32_t recruitCount = 0;
				for (std::uint32_t i = 0; i < count; ++i)
				{
					const auto* position = Human_GetMapPositionRef(Human(candidates[i]));
					const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
					std::int32_t best = 10000000, house = -1;
					for (std::uint32_t b = 0; b < barrackCount; ++b)
					{
						const auto* door = House(barracks[b]) + 36;
						if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(door), NC2Layout::MapMicroPointOffset::ContinentId) != continent) continue;
						const auto distance = Distance(position, door);
						if (distance <= best)
						{
							best = distance; house = barracks[b];
						}
					}
					if (house != -1) recruits[recruitCount++] = { candidates[i], static_cast<std::uint32_t>(house), best };
				}
				if (recruitCount > 1)
					for (std::uint32_t pass = recruitCount - 1; pass != 0; --pass)
						for (std::uint32_t i = 0; i < recruitCount - 1; ++i)
							if (recruits[i].distance > recruits[i + 1].distance)
							{
								const auto entry = recruits[i]; recruits[i] = recruits[i + 1]; recruits[i + 1] = entry;
							}
				for (std::uint32_t i = 0; i < recruitCount; ++i)
				{
					const auto& entry = recruits[i];
					GameAssistantManager_l_Human_Command_Add(manager, entry.human, 32, entry.house, Read<std::uint32_t>(House(entry.house), NC2Layout::HouseOffset::UniqueId), 1);
					DecrementIfNonzero(data, NC2Layout::GameAssistantPlayerOffset::RequestedUnarmedSoldiers); Write(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget, Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget) - 1); if (Read<std::uint32_t>(data, NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget) == 0) break;
				}
			}
		}
		const auto equipSoldiers = [&] (std::uint32_t desired, std::uint32_t counter, std::uint32_t firstWeapon, std::uint32_t secondWeapon)
		{
			if (Read<std::uint32_t>(data, counter) == 0) return;
			std::uint32_t candidates[100], count = 0;
			CHumanIterator iterator;
			for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
			{
				const auto* human = Human(id);
				if (!Human_IsAgeAdult(human) || Human_IsJobHero(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 || !Human_IsJobSoldier(human) || Human_HasEquippedWeapon(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) != 1 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 31 || GameAssistantSingleHumanData_CommandHistory_IsInArray(const_cast<std::byte*>(human) + NC2Layout::HumanOffset::AssistantData, 81)) continue;
				candidates[count++] = id; if (count == 100) break;
			}
			Shuffle(candidates, count);
			for (std::uint32_t i = 0; i < count && i < 6; ++i)
			{
				auto* human = Human(candidates[i]); std::uint32_t weapon;
				if (Human_IsAbleToEquipGoodNow(human, firstWeapon, true)) weapon = firstWeapon;
				else if (Human_IsAbleToEquipGoodNow(human, secondWeapon, true)) weapon = secondWeapon;
				else continue;
				GameAssistantManager_l_Human_Command_Add(manager, candidates[i], 81, weapon, 0, 3);
				if (!Human_HasEquippedArmor(human))
					for (const auto armor : { 36u, 35u, 34u, 33u })
						if (Human_IsAbleToEquipGoodNow(human, armor, true))
						{
							GameAssistantManager_l_Human_Command_Add(manager, candidates[i], 82, armor, 0, 3); break;
						}
				DecrementIfNonzero(data, desired);
				const auto value = Read<std::uint32_t>(data, counter);
				if (value != 0)
				{
					Write(data, counter, value - 1); if (value - 1 == 0) break;
				}
			}
		};
		if ((tick - 12u) % 30 == 0) equipSoldiers(NC2Layout::GameAssistantPlayerOffset::RequestedSwordsmen, NC2Layout::GameAssistantPlayerOffset::SwordsmanRequestBudget, 42, 41);
		if ((tick - 14u) % 30 == 0) equipSoldiers(NC2Layout::GameAssistantPlayerOffset::RequestedArchers, NC2Layout::GameAssistantPlayerOffset::ArcherRequestBudget, 38, 37);
		if ((tick - 16u) % 30 == 0) equipSoldiers(NC2Layout::GameAssistantPlayerOffset::RequestedSpearmen, NC2Layout::GameAssistantPlayerOffset::SpearmanRequestBudget, 40, 39);
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipSoldierShoes) != 0 && (tick - 18u) % 30 == 0) equipItems(30, true, false, false);
		if (Read<std::uint8_t>(data, NC2Layout::GameAssistantPlayerOffset::EquipSoldierMead) != 0 && (tick - 20u) % 30 == 0) equipItems(43, true, false, true);
	}
}
