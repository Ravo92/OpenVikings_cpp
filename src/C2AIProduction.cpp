#include "C2AIBuildingAccess.h"
#include "CivilizationBuildings.h"
#include "C2AIHumanRequests.h"
#include "C2AIObjectAccess.h"
#include "C2AIProduction.h"
#include "C2AIResourceAccess.h"
#include "C2AIVillageAccess.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		std::byte* Producer(void* manager, std::uint32_t good, std::uint32_t producer)
		{
			return static_cast<std::byte*>(manager) + (producer + good * NC2Runtime::AIGoodsProducerAddressing::GoodIndexScale) * NC2Layout::RecordSize::AIGoodProducer + NC2Layout::AIGoodOffset::Producer;
		}
		void* Human(const void* producer)
		{
			return CHumanArray::mStaticVars + Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId) * CHumanArray::ElementSize;
		}
		void* House(const void* producer)
		{
			return CHouseArray::mStaticVars + Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId) * CHouseArray::ElementSize;
		}
		void Command(void* human, std::uint8_t command, std::uint32_t first, std::uint32_t second)
		{
			Human_DoAddNewUserCommand(human, command, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), first, second);
		}

		// Remaster: let native commands finish before changing a logistics assignment again.
		bool Remaster_HasPendingWorkAssignment(const void* human)
		{
			for (const auto opcode : { SwitchIds::UserCommand::ChangeJob, SwitchIds::UserCommand::ChangeProducedGood, SwitchIds::UserCommand::ChangeProduction, SwitchIds::UserCommand::AttachWorkHouse, SwitchIds::UserCommand::DetachWorkHouse, SwitchIds::UserCommand::SetWorkCenter })
				if (Human_UserControl_IsCommandTypeInQueue(human, static_cast<std::uint8_t>(opcode))) return true;
			return false;
		}

		bool Remaster_CollectorSiteNeedsCarrier(void* manager, std::uint32_t houseId, std::uint32_t point)
		{
			for (std::uint32_t good = 1; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
			{
				const auto* entry = static_cast<const std::byte*>(manager) + good * NC2Layout::RecordSize::AIGood;
				if (Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) == 0 || Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount) == 0) continue;
				auto* producer = Producer(manager, good, 0);
				if (Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId) != houseId || !AIGoodManager_Producer_IsHumanValidCheck(producer)) continue;
				const auto* human = Human(producer);
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == SwitchIds::HumanJob::Collector && Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter) == point && !House_Stock_IsFullSoon(House(producer), good)) return true;
			}
			return false;
		}

		// Remaster: warehouse attachments and queued assignments are the saved route state.
		void Remaster_EnsureCollectorCarrier(void* manager, const void* village, void* destination, std::uint32_t point)
		{
			const auto player = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId);
			const auto houseId = Read<std::uint32_t>(destination, NC2Layout::HouseOffset::HouseId);
			void* availableCarrier = nullptr;
			CHumanIterator iterator;
			for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
			{
				auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
				if (Human_UserControl_GetNextJobType(human) != SwitchIds::HumanJob::Carrier) continue;
				if (Remaster_HasPendingWorkAssignment(human)) return;
				if (!Human_IsAttachedToWorkHouse(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) != houseId || Human_IsAttachedToVehicle(human)) continue;
				const auto center = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);
				if (center == point) return;
				const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0 && (task == SwitchIds::HumanTask::Idle || task == SwitchIds::HumanTask::GoodieIdle || task == SwitchIds::HumanTask::ExecuteJob) && !Remaster_CollectorSiteNeedsCarrier(manager, houseId, center)) availableCarrier = human;
			}
			if (availableCarrier != nullptr)
			{
				Command(availableCarrier, SwitchIds::UserCommand::SetWorkCenter, point, 0);
				return;
			}
			if (!House_Inhabitants_IsFreeWorkSlotAvailable(destination, SwitchIds::HumanJob::Carrier)) return;
			const auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
			const auto* single = Read<void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
			void* candidate = nullptr;
			std::uint32_t bestDistance = NXBasics::IntegerRange::UInt32Maximum;
			for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
			{
				auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
				const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
				if (job != SwitchIds::HumanJob::Civilist && job != SwitchIds::HumanJob::Builder && job != SwitchIds::HumanJob::Carrier) continue;
				if (!Human_UserControl_CanChangeJob(human) || !Human_IsAbleToDoJob(human, SwitchIds::HumanJob::Carrier) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit5) != 0 || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToBuildSite(human) || Human_IsAttachedToVehicle(human) || Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human)) continue;
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Remaster_HasPendingWorkAssignment(human) || Human_UserControl_IsPriorityCommandInQueue(human) || AISinglePlayerHandler_Misc_Human_CheckRequestsForCreature(single, id)) continue;
				std::uint32_t target[3]{ 1, static_cast<std::uint32_t>(id), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) };
				if (AIHighLevelHandler_IsObjectUsed(high, target)) continue;
				const auto* position = Human_GetMapPositionRef(human);
				if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId)) continue;
				const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)));
				if (candidate == nullptr || (Human_IsJobCarrier(human) && !Human_IsJobCarrier(candidate)) || (Human_IsJobCarrier(human) == Human_IsJobCarrier(candidate) && distance < bestDistance)) { bestDistance = distance; candidate = human; }
			}
			if (candidate == nullptr) return;
			// Native slots execute by opcode, not insertion order. Set the flag after attachment.
			if (Read<std::uint32_t>(candidate, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Carrier) Command(candidate, SwitchIds::UserCommand::ChangeJob, SwitchIds::HumanJob::Carrier, 0);
			Command(candidate, SwitchIds::UserCommand::AttachWorkHouse, houseId, Read<std::uint32_t>(destination, NC2Layout::HouseOffset::UniqueId));
		}
	}

	// Remaster: keep dedicated and pending warehouse carriers out of other AI allocations.
	bool Remaster_AIGoods_IsWarehouseCarrier(const void* human)
	{
		if (Human_UserControl_GetNextJobType(human) != SwitchIds::HumanJob::Carrier) return false;
		if (Human_UserControl_IsCommandTypeInQueue(human, SwitchIds::UserCommand::ChangeJob) || Human_UserControl_IsCommandTypeInQueue(human, SwitchIds::UserCommand::AttachWorkHouse)) return true;
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);
		if (!Human_IsAttachedToWorkHouse(human) || CHouseArray::sTheObjectPtr == nullptr || id >= static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity)) return false;
		const auto* house = GetHousePtr(static_cast<std::int32_t>(id));
		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) && House_GetHouseMainType(house) == 1;
	}

	// Remaster: the single-player request fallback must not draft detached producers either.
	bool Remaster_AIGoods_IsFieldLogisticsHuman(const void* human)
	{
		if (Remaster_AIGoods_IsWarehouseCarrier(human)) return true;
		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		if (gAIManager == nullptr || player >= NC2Runtime::AIManagerLimits::Players || Human_UserControl_GetNextJobType(human) != SwitchIds::HumanJob::Collector) return false;
		const auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		if (high == nullptr) return false;
		std::uint32_t target[3]{ 1, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) };
		return AIVillagesData_IsObjectUsed(static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages, target);
	}

	// Remaster: stop through the native production-counter command, not unsupported good zero.
	void Remaster_AIGoods_PauseCollector(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Collector || good == 0 || good >= NC2Runtime::AIGoodsLimits::Goods || Remaster_HasPendingWorkAssignment(human)) return;
		const auto counter = Human_Production_GetCounter(human, good);
		if (counter > 0 && counter <= NC2Runtime::ProductionCounter::Infinite) Command(human, SwitchIds::UserCommand::ChangeProduction, good, 0u - static_cast<std::uint32_t>(NC2Runtime::ProductionCounter::Infinite));
	}
	// Original function: FUN_004571ce; fixed availability bytes come from Windows tribe branches.
	void AIGoodManager_Init(void* manager, std::uint32_t tribe)
	{
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			auto* entry = static_cast<std::byte*>(manager) + good * NC2Layout::RecordSize::AIGood;
			Write<std::int32_t>(entry, NC2Layout::AIGoodOffset::Producer, -1);
			Write<std::int32_t>(entry, NC2Layout::AIGoodOffset::ProducerHouseId, -1);
			Write<std::uint8_t>(entry, 36, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 0);
			Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 0);
			Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::TraderBringFlag, 0);
			Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount, 0);
			Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ForceProducingFlag, 0);
		}
		// Expanded civilizations establish real production instead of the original assumed-supply flags.
		if (NRemaster::CivilizationBuildings::IsExpandedTribe(tribe) || (tribe != 2 && tribe != 3 && tribe != 4 && tribe != 7)) return;
		const std::uint32_t sharedForceProducingFlagOffsets[] = {
			NC2Layout::RecordSize::AIGood * 1 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 4 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 8 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 11 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 12 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 13 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 15 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 20 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 25 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 27 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 28 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 29 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 35 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 36 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 43 + NC2Layout::AIGoodOffset::ForceProducingFlag
		};
		for (auto offset : sharedForceProducingFlagOffsets) Write<std::uint8_t>(manager, offset, 1);
		if (tribe == 2)
		{
			Write<std::uint8_t>(manager, NC2Layout::RecordSize::AIGood * 26 + NC2Layout::AIGoodOffset::ForceProducingFlag, 1);
			Write<std::uint8_t>(manager, NC2Layout::RecordSize::AIGood * 38 + NC2Layout::AIGoodOffset::ForceProducingFlag, 1);
			Write<std::uint8_t>(manager, NC2Layout::RecordSize::AIGood * 42 + NC2Layout::AIGoodOffset::ForceProducingFlag, 1);
		}
		else if (tribe == 3)
		{
			Write<std::uint8_t>(manager, NC2Layout::RecordSize::AIGood * 39 + NC2Layout::AIGoodOffset::ForceProducingFlag, 1);
			Write<std::uint8_t>(manager, NC2Layout::RecordSize::AIGood * 41 + NC2Layout::AIGoodOffset::ForceProducingFlag, 1);
		}
		else
		{
			const std::uint32_t extraForceProducingFlagOffsets[] = {
			NC2Layout::RecordSize::AIGood * 26 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 31 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 33 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 34 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 39 + NC2Layout::AIGoodOffset::ForceProducingFlag,
			NC2Layout::RecordSize::AIGood * 37 + NC2Layout::AIGoodOffset::ForceProducingFlag
			};
			for (auto offset : extraForceProducingFlagOffsets) Write<std::uint8_t>(manager, offset, 1);
		}
	}
	// Original function: FUN_004576b4.
	std::int32_t AIGoodManager_l_House_SearchHouse(void* manager, std::uint32_t good, std::uint32_t index, void* village)
	{
		auto* producer = Producer(manager, good, index);
		if (AIGoodManager_Producer_IsHouseValid(producer)) return 1;
		const auto id = AIVillageManager_Good_GetWorkHouseId(village, good);
		if (id == -2) return -2;
		if (id == -1)
		{
			Write<std::int32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId, -1);
			return 0;
		}
		Write(producer, NC2Layout::AIGoodProducerOffset::HouseId, id);
		Write(producer, NC2Layout::AIGoodProducerOffset::HouseRevision, Read<std::uint32_t>(House(producer), NC2Layout::HouseOffset::UniqueId));
		return 1;
	}
	// Original function: FUN_0045771e.
	bool AIGoodManager_l_GetWorkerForHouse(void* manager, std::uint32_t good, std::uint32_t index, const void* village)
	{
		auto* producer = Producer(manager, good, index);
		if (AIGoodManager_Producer_IsHumanValid(producer)) return true;
		const auto id = AIVillageManager_Good_GetWorkHumanId(village, House_Door_GetEntryPointRef(House(producer)));
		if (id == -1) return false;
		Write(producer, NC2Layout::AIGoodProducerOffset::HumanId, id);
		Write(producer, NC2Layout::AIGoodProducerOffset::HumanRevision, Read<std::uint32_t>(Human(producer), NC2Layout::HumanOffset::UniqueId));
		return true;
	}
	// Original function: FUN_0045778b.
	bool AIGoodManager_l_House_CheckHumanSettings(void* manager, std::uint32_t good, std::uint32_t index, const void* resources)
	{
		auto* producer = Producer(manager, good, index);
		auto* house = House(producer);
		const auto job = Read<std::uint32_t>(resources, (good + NC2Runtime::AIGoodsProducerAddressing::WorkerRequirementIndexBias) * NC2Layout::RecordSize::AIResourceGood);
		const auto id = Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId);
		if (!House_Inhabitants_IsFreeWorkSlotAvailable(house, job) && !House_Inhabitants_IsHumanAttachedAsWorker(house, id))
		{
			AIGoodManager_Producer_Clear(producer);
			return false;
		}
		auto* human = Human(producer);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == job)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) != Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId)) Command(human, 28, Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId), Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseRevision));
			else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != good) Command(human, 22, good, 0);
			else return House_Stock_GetAmountOfGoods(house, good, true) != 0;
		}
		else if (Human_IsJobBuilder(human) && Human_IsAttachedToBuildSite(human)) Write<std::int32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId, -1);
		else Command(human, 21, job, 0);
		return false;
	}
	// Original function: FUN_004578a3.
	bool AIGoodManager_l_Collect_CheckHumanSettings(void* manager, std::uint32_t good, std::uint32_t index, const void* village, const void* resources)
	{
		const auto* requirement = static_cast<const std::byte*>(resources) + NC2Layout::AIResourceManagerOffset::GoodRequirements + good * NC2Layout::RecordSize::AIResourceGood;
		if (Read<std::uint32_t>(requirement, NC2Layout::AIResourceGoodOffset::WorkerJob) == SwitchIds::HumanJob::Collector)
		{
			// Remaster: a producer's house is the delivery destination, not the collector's job binding.
			auto* producer = Producer(manager, good, index);
			auto* destination = House(producer);
			auto* human = Human(producer);
			std::uint32_t point = 0;
			if (!AIVillageManager_Good_GetWorkCenterPoint(village, good, &point) || !House_Stock_CanStoreGoodType(destination, good, true))
			{
				AIGoodManager_Producer_Clear(producer);
				return false;
			}
			const auto player = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId);
			if (AI_IsEnemyObjectAround(player, &point, 30)) { Remaster_AIGoods_PauseCollector(human); return false; }
			if (House_Stock_IsFullSoon(destination, good))
			{
				Remaster_AIGoods_PauseCollector(human);
				return House_Stock_GetAmountOfGoods(destination, good, true) != 0;
			}
			if (Remaster_HasPendingWorkAssignment(human)) return false;
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Collector)
			{
				if (Human_IsJobBuilder(human) && Human_IsAttachedToBuildSite(human)) AIGoodManager_Producer_Clear(producer);
				else if (Human_IsAbleToDoJob(human, SwitchIds::HumanJob::Collector)) Command(human, SwitchIds::UserCommand::ChangeJob, SwitchIds::HumanJob::Collector, 0);
				else AIGoodManager_Producer_Clear(producer);
				return false;
			}
			if (Human_IsAttachedToWorkHouse(human))
			{
				Command(human, SwitchIds::UserCommand::DetachWorkHouse, 0, 0);
				return false;
			}
			if (!Human_IsCapableToProduceGood(human, good)) { AIGoodManager_Producer_Clear(producer); return false; }
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != good || Human_Production_GetCounter(human, good) == 0) Command(human, SwitchIds::UserCommand::ChangeProducedGood, good, 0);
			else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter) != point) Command(human, SwitchIds::UserCommand::SetWorkCenter, point, 0);
			else Remaster_EnsureCollectorCarrier(manager, village, destination, point);
			return House_Stock_GetAmountOfGoods(destination, good, true) != 0;
		}
		if (!AIGoodManager_l_House_CheckHumanSettings(manager, good, index, resources)) return false;
		auto* producer = Producer(manager, good, index);
		std::uint32_t point = 0;
		if (!AIVillageManager_Good_GetWorkCenterPoint(village, good, &point))
		{
			Write<std::int32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId, -1);
			Write<std::int32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId, -1);
			return false;
		}
		auto* human = Human(producer);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter) == point) return true;
		Command(human, 17, MapMigPoint_GetLong(&point), 0);
		return false;
	}
	// Original function: FUN_0045754f.
	void AIGoodManager_DoOneLogicGameTick(void* manager, void* village, const void* resources)
	{
		const bool expanded = NRemaster::CivilizationBuildings::IsExpandedPlayer(Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId));
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			auto* entry = static_cast<std::byte*>(manager) + good * NC2Layout::RecordSize::AIGood;
			Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, expanded ? 0 : Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ForceProducingFlag));
			if (Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag) != 0) continue;
			const auto* data = static_cast<const std::byte*>(resources) + NC2Layout::AIResourceManagerOffset::GoodRequirements + good * NC2Layout::RecordSize::AIResourceGood;
			if (Read<std::uint32_t>(data, NC2Layout::AIResourceGoodOffset::Type) == 1 && AIResourceManager_WasResourceDataUpdated(resources, good) != 0)
			{
				std::uint32_t point = 0;
				if (AIResourceManager_GetResourcePosition(resources, good, &point) && AIVillage_IsPointInside(village, &point))
				{
					Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag, 0);
					Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::TraderBringFlag, 0);
				}
			}
			if (Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag) != 0)
			{
				Write(entry, NC2Layout::AIGoodOffset::ProducingFlag, Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::TraderBringFlag));
				if (Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount) > 0)
				{
					AIGoodManager_Producer_Clear(entry + NC2Layout::AIGoodOffset::Producer);
					Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 0);
				}
				continue;
			}
			if (Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) == 0)
			{
				AIGoodManager_Producer_Clear(entry + NC2Layout::AIGoodOffset::Producer);
				Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProductionConfirmationCount, 0);
				continue;
			}
			const auto house = AIGoodManager_l_House_SearchHouse(manager, good, 0, village);
			if (house == 0)
			{
				AIVillageManager_Good_AskForNewHouse(village, good, resources);
				continue;
			}
			if (house != 1) continue;
			if (Read<std::uint32_t>(data, NC2Layout::AIResourceGoodOffset::WorkerJob) == 0)
			{
				Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 1);
				Write(entry, NC2Layout::AIGoodOffset::NeedCount, Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) - 1);
				continue;
			}
			const auto kind = Read<std::uint32_t>(data, NC2Layout::AIResourceGoodOffset::Type);
			// Remaster: a full warehouse must not reserve an idle worker for a new field route.
			if (kind == 1 && Read<std::uint32_t>(data, NC2Layout::AIResourceGoodOffset::WorkerJob) == SwitchIds::HumanJob::Collector && !AIGoodManager_Producer_IsHumanValid(Producer(manager, good, 0)) && House_Stock_IsFullSoon(House(Producer(manager, good, 0)), good))
			{
				if (House_Stock_GetAmountOfGoods(House(Producer(manager, good, 0)), good, true) != 0)
				{
					Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 1);
					Write(entry, NC2Layout::AIGoodOffset::NeedCount, Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) - 1u);
				}
				Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 0);
				continue;
			}
			if (!AIGoodManager_l_GetWorkerForHouse(manager, good, 0, village)) continue;
			const auto producing = kind == 4 ? AIGoodManager_l_House_CheckHumanSettings(manager, good, 0, resources) : kind == 1 ? AIGoodManager_l_Collect_CheckHumanSettings(manager, good, 0, village, resources) : false;
			if (producing)
			{
				const auto count = Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProductionConfirmationCount) + 1;
				Write(entry, NC2Layout::AIGoodOffset::ProductionConfirmationCount, count);
				if (count > 5)
				{
					Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 1);
					Write(entry, NC2Layout::AIGoodOffset::NeedCount, Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) - 1);
				}
			}
			Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 1);
		}
	}
}
