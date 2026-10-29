#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SPlayerGoodsCountStorage
	{
		std::uint32_t total;
		std::uint16_t amounts[66];
	};

	struct SPlayerStatisticSampleData
	{
		std::uint16_t humans[10];
		std::uint16_t houses[60];
		std::uint32_t goodTotal;
		std::uint16_t goods[66];
		std::uint32_t babiesBorn, humansDied, soldiersDied;
		std::byte unknown120[16];
		std::uint32_t wealth, happiness;
		std::int32_t GetNumber(std::int32_t type) const;
	};

	static_assert(sizeof(SPlayerStatisticSampleData) == 312);
	static_assert(offsetof(SPlayerStatisticSampleData, goodTotal) == 0x8C);
	static_assert(offsetof(SPlayerStatisticSampleData, babiesBorn) == 0x114);
	static_assert(offsetof(SPlayerStatisticSampleData, humansDied) == 0x118);
	static_assert(offsetof(SPlayerStatisticSampleData, soldiersDied) == 0x11C);
	static_assert(offsetof(SPlayerStatisticSampleData, wealth) == 0x130);
	static_assert(offsetof(SPlayerStatisticSampleData, happiness) == 0x134);
	void Tool_PlayerStatistics_CountHumans(std::uint32_t player, std::uint16_t* counts);
	std::int32_t Tool_PlayerStatistics_CountHumansOfJobExecuting(std::uint32_t player, std::uint32_t job);
	SPlayerStatisticSampleData* PlayerStatisticManager_GetStatistics(void* manager, std::int32_t time, std::uint32_t player);
	SPlayerStatisticSampleData* PlayerStatisticManager_GetCurrentStatistics(void* manager, std::uint32_t player);
	void Tool_PlayerStatistics_CountHouses(std::uint32_t player, std::uint16_t* counts);
	std::int32_t Tool_PlayerStatistics_CountHousesOfLogicType(std::uint32_t player, std::uint32_t type, std::int32_t* unfinished);
	void Tool_PlayerStatistics_CountGoods(std::uint32_t player, void* counts);
	void PlayerStatisticManager_CreateStatisticSamples(std::uint32_t player, SPlayerStatisticSampleData* sample);
	void PlayerStatisticManager_CreateStatistics(void* manager);
	void* __fastcall PlayerStatisticManager_Construct(void* manager, void* unusedEdx);
	void __fastcall PlayerStatisticManager_UpdateStatistics(void* manager, void* unusedEdx, std::uint32_t time);
}