#include "C2GameState.h"
#include "C2HappinessEvents.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2VehicleIterator.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00419818; active players are checked again for each iteration.
	void HappinessManager_RecalculateAllPlayer(void* manager)
	{
		for (std::uint32_t player = 0; player < 20; ++player)
		{
			if (Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) != 0) HappinessManager_l_RecalculatePlayer(manager, player);
		}
	}

	// Original function: FUN_0041984e; components, event decay and attack penalty preserve native update order.
	void HappinessManager_l_RecalculatePlayer(void* manager, std::uint32_t player)
	{
		auto* happiness = static_cast<std::byte*>(manager) + player * NC2Layout::RecordSize::HappinessPlayerUnsigned;
		Happiness_UpdateEvents(happiness);
		if (Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::AttackExpiresAtTick) < gGameTime.tick) Write<std::uint8_t>(happiness, NC2Layout::HappinessPlayerOffset::UnderAttack, 0);
		Write(happiness, NC2Layout::HappinessPlayerOffset::Value, HappinessManager_l_GetGoodHappinessValue(manager, player));
		Write(happiness, NC2Layout::HappinessPlayerOffset::Value, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value) + static_cast<std::uint32_t>(HappinessManager_l_GetBuildingHappinessValue(manager, player)));
		Write(happiness, NC2Layout::HappinessPlayerOffset::Value, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value) + static_cast<std::uint32_t>(HappinessManager_l_GetHumanHappinessValue(manager, player)));
		Write(happiness, NC2Layout::HappinessPlayerOffset::Value, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value) + HappinessManager_l_GetMilitaryHappinessValue(manager, player));
		Write(happiness, NC2Layout::HappinessPlayerOffset::Value, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value) + static_cast<std::uint32_t>(Happiness_GetEventValue(happiness)));
		if (Read<std::uint8_t>(happiness, NC2Layout::HappinessPlayerOffset::UnderAttack) != 0) Write(happiness, NC2Layout::HappinessPlayerOffset::Value, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value) - 10u);
		Write(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::HappinessValueUnsigned, Read<std::uint32_t>(happiness, NC2Layout::HappinessPlayerOffset::Value));
	}

	// Original function: FUN_004198d3; statistics are read as unsigned WORDs before signed arithmetic.
	std::int32_t HappinessManager_l_GetGoodHappinessValue(const void*, std::uint32_t player)
	{
		const void* statistics = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		const auto population = static_cast<std::int32_t>(Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::HumanCount));
		const auto goods16And17 = static_cast<std::int32_t>(Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Goods + 16 * NC2Layout::RecordSize::StatisticCount)) + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Goods + 17 * NC2Layout::RecordSize::StatisticCount);
		const auto cappedGoods = goods16And17 > population * 2 ? population * 2 : goods16And17;
		const auto goods15And28And29 = static_cast<std::int32_t>(Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Goods + 28 * NC2Layout::RecordSize::StatisticCount)) + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Goods + 29 * NC2Layout::RecordSize::StatisticCount) + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Goods + 15 * NC2Layout::RecordSize::StatisticCount);
		return cappedGoods + goods15And28And29 / 3 - population;
	}

	// Original function: FUN_0041992c; retain all independent building terms and the zero-count penalty.
	std::int32_t HappinessManager_l_GetBuildingHappinessValue(const void*, std::uint32_t player)
	{
		const void* statistics = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		std::uint32_t value = Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 42 * NC2Layout::RecordSize::StatisticCount) * 5u + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 6 * NC2Layout::RecordSize::StatisticCount) * 3u + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 18 * NC2Layout::RecordSize::StatisticCount) + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 17 * NC2Layout::RecordSize::StatisticCount);
		value = value * 5u + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 46 * NC2Layout::RecordSize::StatisticCount) * 12u + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 45 * NC2Layout::RecordSize::StatisticCount) * 8u;
		const auto count = Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 19 * NC2Layout::RecordSize::StatisticCount);
		if (count == 0) value -= 10u;
		else value += (static_cast<std::uint32_t>(count) + Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 20 * NC2Layout::RecordSize::StatisticCount)) * 20u;
		value += Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 22 * NC2Layout::RecordSize::StatisticCount) * 20u - Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::Houses + 0 * NC2Layout::RecordSize::StatisticCount);
		return NXBasics::Runtime_TimeSigned32(value);
	}

	// Original function: FUN_004199a1; children take the separate native branch after the food check.
	std::int32_t HappinessManager_l_GetHumanHappinessValue(const void*, std::uint32_t player)
	{
		std::uint32_t value = 0;
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(player, false); id != -1; id = iterator.GetNextSectorHumanId())
		{
			const void* human = GetHumanPtr(id);
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) < 1000) --value;
			if (!Human_IsAgeAdult(human)) ++value;
			else
			{
				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) < 8000) --value;
				if (Human_IsMarried(human)) value += 2u;
				if (!Human_IsAttachedToHomeHouse(human)) --value;
			}
		}
		return NXBasics::Runtime_TimeSigned32(value);
	}

	// Original function: FUN_00419a27; the second vehicle list adds two per returned vehicle.
	std::uint32_t HappinessManager_l_GetMilitaryHappinessValue(const void*, std::uint32_t player)
	{
		const void* statistics = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		std::uint32_t value = Read<std::uint16_t>(statistics, NC2Layout::PlayerStatisticSampleOffset::SoldierCount);
		CVehicleIterator iterator;
		for (auto id = iterator.GetFirstSectorVehicleId(player, 0, 10000, true); id != -1; id = iterator.GetNextSectorVehicleId()) value += 2u;
		return value;
	}

	// Original function: FUN_00419a7f; every branch preserves unconditional callback propagation.
	bool __cdecl HappinessManager_l_Happiness_Callback(TCallbackTypes type, std::uintptr_t, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		if (type == 1)
		{
			if (param1 % 720u == 0) HappinessManager_RecalculateAllPlayer(gHappinessManager);
		}
		else if (type == 0x27)
		{
			const void* event = reinterpret_cast<const void*>(callbackValueParam1);
			const void* human = Read<const void*>(event, NC2Layout::DirectionalParticleCallbackOffset::Human);
			if (!Human_IsJobSoldier(human)) Happiness_AddEventType(static_cast<std::byte*>(gHappinessManager) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::HappinessPlayerUnsigned, 1);
		}
		else if (type == 0x28) HappinessManager_Messages_PlayerAttackedByPlayer(gHappinessManager, param1, param2);
		return true;
	}
}
