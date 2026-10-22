#pragma once
#include <cstddef>
#include <cstdint>
namespace NC2Logic
{
	// Physical Windows goods planning records; producer tail bytes remain unclassified.
	struct SAIGoodProducerStorage
	{
		std::int32_t humanId; std::uint32_t humanRevision; std::int32_t houseId; std::uint32_t houseRevision; std::byte unresolved10[4];
	};
	struct SAIGoodStorage
	{
		std::uint8_t producing;
		std::byte unresolved01[3];
		std::uint32_t productionConfirmationCount;
		std::uint8_t forceProducing, importing, traderBring;
		std::byte unresolved0B;
		std::uint32_t needCount, producerCount;
		SAIGoodProducerStorage producer;
	};
	void AIGoodManager_Init(void* manager, std::uint32_t tribe);
	bool Remaster_AIGoods_IsWarehouseCarrier(const void* human);
	bool Remaster_AIGoods_IsFieldLogisticsHuman(const void* human);
	void Remaster_AIGoods_PauseCollector(void* human);
	void Remaster_AIGoods_UpdateCollectors(void* high);
	void AIGoodManager_DoOneLogicGameTick(void* manager, void* village, const void* resources);
	std::int32_t AIGoodManager_l_House_SearchHouse(void* manager, std::uint32_t good, std::uint32_t producer, void* village);
	bool AIGoodManager_l_GetWorkerForHouse(void* manager, std::uint32_t good, std::uint32_t producer, const void* village);
	bool AIGoodManager_l_House_CheckHumanSettings(void* manager, std::uint32_t good, std::uint32_t producer, const void* resources);
	bool AIGoodManager_l_Collect_CheckHumanSettings(void* manager, std::uint32_t good, std::uint32_t producer, const void* village, const void* resources);
}
