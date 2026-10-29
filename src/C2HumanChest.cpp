#include "C2EngineLandscapeAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HumanChest.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticTribeTypeData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void Fail(void* human)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}

		const void* RealTarget(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
		}

		const void* WorkTarget(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
		}
	}

	// Original function: FUN_0043d34b.
	std::uint32_t Human_DoExecuteUserCommand_OpenChest(void* human, const void* point)
	{
		if (!Human_l_IsAtomicInterruptable(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			return 0;

		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1)
			return 1;

		const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);

		if ((landscape != 85 && landscape != 86) || !Human_IsAgeAdult(human))
			return 1;

		std::uint32_t work = 0;

		if (!GameMap_Misc_GetLandscapeWorkPosition(point, &work))
			return 1;

		Human_Target_Reset(human);
		Human_Target_SetTargetPositions(human, point, &work);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType, landscape);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 18);

		return 2;
	}

	// Original function: FUN_0044613f; cargo is dropped before retrying opcode 18.
	void Human_l_StartTask_OpenChest(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_DoAddNewUserCommand(human, 18, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), MapMigPoint_GetLong(RealTarget(human)), 0);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			{
				Human_StopCurrentGoodCirculationMode(human);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			}

			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}

		if (!Human_Target_IsValid(human))
		{
			Fail(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			Human_l_StartAtomicAnimation(human, 91, Human_Target_GetDirectionTo(human));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		const auto landscape = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 64);

		if (!Human_l_StartAtomicWalkAndFetch(human, RealTarget(human), WorkTarget(human), true))
		{
			Fail(human);

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2);
		Write(human, NC2Layout::HumanOffset::TargetLandscapeType, landscape);
	}

	// Original function: FUN_0043b00c; enable the good and its Windows producer job.
	void Tool_TechTree_EnableGoodProduction(std::uint32_t player, std::uint32_t tribe, std::uint32_t good)
	{
		auto* data = static_cast<std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
		const auto job = gStaticTribeTypeData[tribe].goodProduction[good].job;
		Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Layout::PlayerAllowanceStride::GoodsUnsigned + good, 1);

		if (job != 0)
			Write<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Layout::PlayerAllowanceStride::JobsAndEquipmentUnsigned + static_cast<std::uint32_t>(job), 1);
	}

	// Original function: FUN_0044cf1c; all Windows chest reward branches are connected.
	bool Tool_MapChest_UseChest(std::uint32_t humanId, const void* point)
	{
		auto* human = CHumanArray::mStaticVars + humanId * CHumanArray::ElementSize;
		const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
		const bool large = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 86;

		if (large && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 30 && !Human_IsJobHero(human))
			return false;

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, large ? 57 : 56, MapMigPoint_GetLong(point), 0);
		const auto reward = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		const auto goods = [&](std::uint32_t good, std::uint32_t amount)
		{
			auto drop = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorWrapped(&drop, 1);
			Map_GoodTool_PutOnMap(good, amount, drop, 5, false);
		};
		const auto special = [&](std::uint32_t type, std::uint32_t item)
		{
			GameControl_SpecialItem_Add(gGameControl, player, type, item, point);
		};
		const auto addHuman = [&](std::uint32_t job)
		{

			return CHumanArray::sTheObjectPtr->AllocateHuman(player, 1, job, point, 0, 0);
		};

		if (reward >= 1 && reward <= 12)
		{
			goods(43u + reward, large ? 3 : 1);
		}
		else
		{
			switch (reward)
			{
				case SwitchIds::ChestReward::Food:
					goods(16, large ? 48 : 30);
					break;
				case SwitchIds::ChestReward::LargeFoodPotions:
					goods(45, large ? 18 : 12);
					break;
				case SwitchIds::ChestReward::LargeStaminaPotions:
					goods(47, large ? 18 : 12);
					break;
				case SwitchIds::ChestReward::LargeHealingPotions:
					goods(49, large ? 18 : 12);
					break;
				case SwitchIds::ChestReward::ChainArmor:
					goods(35, large ? 9 : 6);
					break;
				case SwitchIds::ChestReward::PlateArmor:
					goods(36, large ? 9 : 6);
					break;
				case SwitchIds::ChestReward::Shoes:
					goods(30, large ? 9 : 6);
					break;
				case SwitchIds::ChestReward::LetterOfIndulgence:
				case SwitchIds::ChestReward::PlaceHouse:
					special(2, 0);
					break;
				case SwitchIds::ChestReward::PlaceWatchtower:
					special(3, 41);
					break;
				case SwitchIds::ChestReward::PlaceTemple:
					special(3, 37);
					break;
				case SwitchIds::ChestReward::PlaceBarracks:
					special(3, 39);
					break;
				case SwitchIds::ChestReward::PlaceStockhouse:
					special(3, 9);
					break;
				case SwitchIds::ChestReward::PlaceSmallHome:
					special(3, 3);
					break;
				case SwitchIds::ChestReward::PlaceLargeHome:
					special(3, 6);
					break;
				case SwitchIds::ChestReward::PlaceWell:
					special(3, 10);
					break;
				case SwitchIds::ChestReward::PlaceFarm:
					special(3, 12);
					break;
				case SwitchIds::ChestReward::PlaceMill:
					special(3, 13);
					break;
				case SwitchIds::ChestReward::PlaceBakery:
					special(3, 14);
					break;
				case SwitchIds::ChestReward::PlaceHeadquarters:
					special(3, 1);
					break;
				case SwitchIds::ChestReward::PlaceSchool:
					special(3, 38);
					break;
				case SwitchIds::ChestReward::SmithyAndSmith:
					Tool_TechTree_EnableGoodProduction(player, 1, 42);
					special(4, 32);
					addHuman(13);
					break;
				case SwitchIds::ChestReward::AlchemyHutAndDruid:
					Tool_TechTree_EnableGoodProduction(player, 1, 15);
					special(4, 35);
					addHuman(30);
					break;
				case SwitchIds::ChestReward::MintAndCoinMaker:
					Tool_TechTree_EnableGoodProduction(player, 1, 8);
					special(4, 33);
					addHuman(14);
					break;
				case SwitchIds::ChestReward::AdvancedAlchemyHutAndDruid:
				{
					for (const auto good : { 15u, 44u, 46u, 48u })
					{
						Tool_TechTree_EnableGoodProduction(player, 1, good);
					}

					special(4, 36);
					const auto id = addHuman(30);

					if (id != -1)
					{
						auto* added = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

						for (const auto offset : { 0x1A4u, 0x1C1u, 0x1C5u, 0x1C3u })
						{
							Write<std::uint8_t>(added, offset, 1);
						}

						Human_Production_Setup(added);
					}

					break;
				}
				case SwitchIds::ChestReward::WeaponsHutAndArmorer:
					Tool_TechTree_EnableGoodProduction(player, 1, 37);
					special(4, 27);
					addHuman(10);
					break;
				case SwitchIds::ChestReward::Catapult:
					Logic_EntryPoint_Add_Vehicle(player, 1, 5, point, 0);
					break;
				case SwitchIds::ChestReward::Wolves:
					for (std::uint32_t n = 0; n < 5; ++n)
					{
						Logic_EntryPoint_Add_Animal(20, 20, 49, point, 0, 0);
					}

					break;
				case SwitchIds::ChestReward::Civilians:
					for (std::uint32_t n = 0; n < 3; ++n)
					{
						addHuman(6);
					}

					break;
				case SwitchIds::ChestReward::Women:
					for (std::uint32_t n = 0; n < 3; ++n)
					{
						addHuman(5);
					}

					break;
				case SwitchIds::ChestReward::Swordsmen:
					for (std::uint32_t n = 0; n < 3; ++n)
					{
						addHuman(35);
					}

					break;
				case SwitchIds::ChestReward::Archers:
					for (std::uint32_t n = 0; n < 3; ++n)
					{
						addHuman(41);
					}

					break;
				case SwitchIds::ChestReward::Lions:
					for (std::uint32_t n = 0; n < 3; ++n)
					{
						Logic_EntryPoint_Add_Animal(20, 25, 49, point, 0, 0);
					}

					break;
				default:
					break;
			}
		}

		GameMap_MapMicroPoint_Landscape_ApplyTransition(point, 11);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, player, 9, 0);

		return true;
	}

	// Original function: FUN_0046a310; preserve reset and reservation-release order.
	void Human_l_PerformTaskEvent_OpenChest(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
			Human_Target_Reset(human);

		if (!Human_Target_IsValid(human) || !Tool_MapChest_UseChest(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), RealTarget(human)))
		{
			Fail(human);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);

			if (Human_IsJobHero(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Map_GoodTool_PutOnMap(Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), 1, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates), 5, false);
				Human_ClearCarriedGood(human);
			}
		}

		Human_Target_Reset(human);
		Human_StopCurrentGoodCirculationMode(human);
	}

	// Original function: FUN_0044D9C9; Game.exe 0044DA31 maps the eleven categories to these ranges.
	std::uint32_t Tool_MapChest_GetMapChestTypeFromRandomType(std::uint32_t type)
	{
		if (type > 10)
			return 0;

		static constexpr std::uint32_t minimum[11] = { 94, 52, 91, 20, 51, 0, 1, 7, 90, 24, 96 };
		static constexpr std::uint32_t maximum[11] = { 95, 52, 91, 20, 72, 0, 6, 12, 90, 25, 96 };

		return minimum[type] + Game_Random_GetNumber() % (maximum[type] - minimum[type] + 1u);
	}

	namespace
	{
		// Shared translation of the identical native category-selection loops; not a separate original entry.
		std::uint32_t SelectRandomChest(std::uint32_t mask)
		{
			std::uint32_t count = 0;

			for (std::uint32_t type = 0; type < 11; ++type)
			{
				if ((mask & (1u << type)) != 0)
					++count;
			}

			if (count == 0)
				return 0;

			auto selected = Game_Random_GetNumber() % count;

			for (std::uint32_t type = 0; type < 11; ++type)
			{
				if ((mask & (1u << type)) != 0)
				{
					if (selected == 0)
						return Tool_MapChest_GetMapChestTypeFromRandomType(type);

					--selected;
				}
			}

			return 0;
		}
	}

	// Original function: FUN_0044D79E; six attempts, Y randomization precedes X.
	void Tool_MapChest_SetRandomChestOnRandomPosition(std::uint32_t mask)
	{
		const auto engine = NC2LogicExternals::External_Landscape_GetLandscapeIdByNamePtr("chest wooden");
		const auto chest = SelectRandomChest(mask);

		if (chest == 0)
			return;

		for (std::int32_t attempt = 0; attempt < 6; ++attempt)
		{
			const auto height = GameMap_GetHeight(), width = GameMap_GetWidth();
			const auto y = Game_Random_GetNumber() % static_cast<std::uint32_t>(height);
			const auto x = Game_Random_GetNumber() % static_cast<std::uint32_t>(width);
			std::uint32_t point;
			MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), false);

			if (MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(&point)) != 1 || !Tool_Map_GetNotBlockedPosition(&point, true))
				continue;

			auto neighbor = point;
			MapMigPoint_MoveCursorUnwrapped(neighbor, 4);
			bool valid = true;

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				MapMigPoint_MoveCursorUnwrapped(neighbor, direction);

				if (!MapMigPoint_IsInsideMap(&neighbor) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&neighbor), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
					valid = false;
			}

			if (!valid)
				continue;

			Logic_EntryPoint_Edit_Start();
			Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 85, chest, engine, 1, -1);
			Logic_EntryPoint_Edit_Stop();

			return;
		}
	}

	// Original function: FUN_0044D8F5; all four attempts restart from the supplied point.
	void Tool_MapChest_SetRandomChestOnPosition(std::uint32_t mask, const void* source)
	{
		const auto engine = NC2LogicExternals::External_Landscape_GetLandscapeIdByNamePtr("chest wooden");
		const auto chest = SelectRandomChest(mask);

		if (chest == 0)
			return;

		for (std::int32_t attempt = 0; attempt < 4; ++attempt)
		{
			auto point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);

			if (MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(&point)) != 1 || !Tool_Map_GetNotBlockedPosition(&point, true))
				continue;

			Logic_EntryPoint_Edit_Start();
			Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 85, chest, engine, 1, -1);
			Logic_EntryPoint_Edit_Stop();

			return;
		}
	}
}