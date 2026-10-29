#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include <cstring>
namespace NC2Logic
{
	using NC2Lifetime::Read;
	using NC2Lifetime::Write;
	// Original function: FUN_0041acf8.
	SPlayerStatisticSampleData* PlayerStatisticManager_GetStatistics(void* manager, std::int32_t time, std::uint32_t player)
	{
		if (time < 0) return nullptr;
		auto* history = Read<SPlayerStatisticSampleData*>(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
		if (history == nullptr) return Read<SPlayerStatisticSampleData*>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
		const auto sampleTime = Read<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::LastSampleTime);
		const auto current = Read<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentSampleIndex);
		if (static_cast<std::uint32_t>(time) >= sampleTime) return history + current;
		const auto distance = (sampleTime - static_cast<std::uint32_t>(time) + 59u) / 60u;
		if (distance > 598) return nullptr;
		auto index = static_cast<std::int32_t>(current - distance);
		if (index < 0) index += 600;
		return history + index;
	}
	// Original function: FUN_0041ad54.
	SPlayerStatisticSampleData* PlayerStatisticManager_GetCurrentStatistics(void* manager, std::uint32_t player)
	{
		return PlayerStatisticManager_GetStatistics(manager, static_cast<std::int32_t>(gGameTime.tick / 12u), player);
	}
	// Original function: FUN_00440b4c.
	std::int32_t Tool_PlayerStatistics_CountHumansOfJobExecuting(std::uint32_t player, std::uint32_t job)
	{
		std::int32_t count = 0; CHumanIterator iterator;
		for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId()) if (Read<std::uint32_t>(GetHumanPtr(id), NC2Layout::HumanOffset::JobType) == job) ++count;
		return count;
	}
	// Original function: FUN_00440b92; complete Windows jump table 00440C6D..00440CAC.
	std::int32_t SPlayerStatisticSampleData::GetNumber(std::int32_t type) const
	{
		if (type >= 16 && type <= 81) return goods[type - 16];
		if (type >= 0 && type <= 14) return Read<std::uint16_t>(this, static_cast<std::uint32_t>(type) * NC2Layout::RecordSize::StatisticCountUnsigned);
		if (type == 15) return static_cast<std::int32_t>(goodTotal);
		if (type >= 82 && type <= 84) return static_cast<std::int32_t>(Read<std::uint32_t>(this, offsetof(SPlayerStatisticSampleData, babiesBorn) + static_cast<std::uint32_t>(type - 82) * sizeof(std::uint32_t)));
		if (type >= 85 && type <= 86) return static_cast<std::int32_t>(Read<std::uint32_t>(this, offsetof(SPlayerStatisticSampleData, wealth) + static_cast<std::uint32_t>(type - 85) * sizeof(std::uint32_t)));
		return 0;
	}
	// Original function: FUN_00440af8.
	std::int32_t Tool_PlayerStatistics_CountHousesOfLogicType(std::uint32_t player, std::uint32_t type, std::int32_t* unfinished)
	{
		*unfinished = 0;
		std::int32_t total = 0;
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = static_cast<const std::byte*>(CHouseArray::mStaticVars) + static_cast<std::uint32_t>(id) * 0x7E0u;
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != type) continue;
			++total;
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) ++*unfinished;
		}
		return total;
	}
	// Original function: FUN_0041aa59.
	void* __fastcall PlayerStatisticManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gPlayerStatisticManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::PlayerStatisticManager);
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			const auto* playerData = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
			if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0) continue;
			if (Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::Type) == 1)
			{
				void* history = NXBasics::Runtime_AllocateWithNewHandler(187200, 1);
				Write<void*>(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned, history);
				std::memset(history, 0, 187200);
			}
			else
			{
				void* sample = NXBasics::Runtime_AllocateWithNewHandler(312, 1);
				Write<void*>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned, sample);
				std::memset(sample, 0, 312);
			}
		}
		PlayerStatisticManager_CreateStatistics(manager);
		return manager;
	}
	// Original function: FUN_004408ee.
	void Tool_PlayerStatistics_CountHumans(std::uint32_t player, std::uint16_t* counts)
	{
		std::memset(counts, 0, 20);
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
		{
			const auto* human = GetHumanPtr(id);
			++counts[NC2Runtime::HumanStatisticIndex::All];
			if (Human_IsAgeAdult(human))
			{
				if (Human_IsSexMale(human))
				{
					++counts[NC2Runtime::HumanStatisticIndex::AdultMale];
					if (!Human_IsAttachedToHomeHouse(human))
					{
						++counts[NC2Runtime::HumanStatisticIndex::AdultMaleWithoutHome]; ++counts[NC2Runtime::HumanStatisticIndex::WithoutHome];
					}
					if (!Human_IsAttachedToWorkHouse(human)) ++counts[NC2Runtime::HumanStatisticIndex::AdultMaleWithoutWorkHouse];
					if (Human_IsJobSoldier(human)) ++counts[NC2Runtime::HumanStatisticIndex::Soldiers];
				}
				else
				{
					++counts[NC2Runtime::HumanStatisticIndex::AdultFemale];
					if (!Human_IsAttachedToHomeHouse(human))
					{
						++counts[NC2Runtime::HumanStatisticIndex::AdultFemaleWithoutHome]; ++counts[NC2Runtime::HumanStatisticIndex::WithoutHome];
					}
				}
			}
			else
			{
				++counts[NC2Runtime::HumanStatisticIndex::Children];
				if (!Human_IsAttachedToHomeHouse(human))
				{
					++counts[NC2Runtime::HumanStatisticIndex::ChildrenWithoutHome]; ++counts[NC2Runtime::HumanStatisticIndex::WithoutHome];
				}
			}
		}
	}
	// Original function: FUN_004409c1.
	void Tool_PlayerStatistics_CountHouses(std::uint32_t player, std::uint16_t* counts)
	{
		std::memset(counts, 0, 120);
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = reinterpret_cast<const std::byte*>(CHouseArray::mStaticVars) + static_cast<std::uint32_t>(id) * 0x7E0u;
			++counts[0];
			const bool completed = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3;
			if (completed) ++counts[5u + Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType)];
			if (House_GetHouseMainType(house) == 2) ++counts[completed ? 1 : 2];
			else ++counts[completed ? 3 : 4];
		}
	}
	// Original function: FUN_00440a53.
	void Tool_PlayerStatistics_CountGoods(std::uint32_t player, void* counts)
	{
		std::memset(counts, 0, NC2Layout::RecordSize::PlayerGoodsCounts);
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = reinterpret_cast<const std::byte*>(CHouseArray::mStaticVars) + static_cast<std::uint32_t>(id) * 0x7E0u;
			const auto* type = House_GetHouseTypeDataPtr(house);
			for (std::uint32_t stock = 0; stock < type->stockCount; ++stock)
			{
				const auto good = static_cast<std::uint32_t>(type->stock[stock].goodType);
				const auto amount = House_Stock_GetAmountOfGoods(house, good, false);
				Write<std::uint32_t>(counts, NC2Layout::PlayerGoodsCountOffset::Total, Read<std::uint32_t>(counts, NC2Layout::PlayerGoodsCountOffset::Total) + amount);
				auto value = static_cast<std::uint32_t>(Read<std::uint16_t>(counts, NC2Layout::PlayerGoodsCountOffset::AmountsUnsigned + good * NC2Layout::RecordSize::PlayerGoodCountUnsigned)) + amount;
				if (value > NXBasics::IntegerRange::UInt16Maximum) value = NXBasics::IntegerRange::UInt16Maximum;
				Write<std::uint16_t>(counts, NC2Layout::PlayerGoodsCountOffset::AmountsUnsigned + good * NC2Layout::RecordSize::PlayerGoodCountUnsigned, static_cast<std::uint16_t>(value));
			}
		}
	}
	// Original function: FUN_0041adaf.
	void PlayerStatisticManager_CreateStatisticSamples(std::uint32_t player, SPlayerStatisticSampleData* sample)
	{
		std::memset(sample, 0, 312);
		Tool_PlayerStatistics_CountHumans(player, sample->humans);
		Tool_PlayerStatistics_CountHouses(player, sample->houses);
		Tool_PlayerStatistics_CountGoods(player, &sample->goodTotal);
		const auto* playerData = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
		sample->babiesBorn = Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::BabiesBorn);
		sample->humansDied = Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::HumansDied);
		sample->soldiersDied = Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::SoldiersDied);
		sample->wealth = Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::Wealth);
		sample->happiness = Read<std::uint32_t>(playerData, NC2Layout::GameControlPlayerOffset::HappinessValue);
	}
	// Original function: FUN_0041ad6f.
	void PlayerStatisticManager_CreateStatistics(void* manager)
	{
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			auto* history = Read<std::byte*>(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
			if (history != nullptr) PlayerStatisticManager_CreateStatisticSamples(player, reinterpret_cast<SPlayerStatisticSampleData*>(history + Read<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentSampleIndex) * 0x138u));
			auto* current = Read<SPlayerStatisticSampleData*>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayerUnsigned + player * NC2Layout::RecordSize::StatisticPlayerPointerUnsigned);
			if (current != nullptr) PlayerStatisticManager_CreateStatisticSamples(player, current);
		}
	}
	// Original function: FUN_0041acd6.
	void __fastcall PlayerStatisticManager_UpdateStatistics(void* manager, void* /*unusedEdx*/, std::uint32_t time)
	{
		const auto index = Read<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentSampleIndex) + 1u;
		Write<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentSampleIndex, index == 600 ? 0 : index);
		PlayerStatisticManager_CreateStatistics(manager);
		Write<std::uint32_t>(manager, NC2Layout::PlayerStatisticManagerOffset::LastSampleTime, time);
	}
}
