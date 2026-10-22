#include "C2AIObjectAccess.h"
#include "C2AIProduction.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanJobAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include "RemasterAIEconomy.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		void* Human(const void* record)
		{
			return CHumanArray::mStaticVars + Read<std::uint32_t>(record, NC2Layout::HumanReferenceOffset::HumanId) * CHumanArray::ElementSize;
		}
		void* Good(void* manager, std::uint32_t good)
		{
			return static_cast<std::byte*>(manager) + good * NC2Layout::RecordSize::AIGoodUnsigned;
		}
	}

	// Original function: FUN_00428b63.
	bool AIHighLevelHandler_IsObjectUsed(const void* handler, const void* target)
	{
		const auto* bytes = static_cast<const std::byte*>(handler);
		// Remaster logistics reservations survive save/load through jobs and command queues.
		if (Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) == 1)
		{
			const auto id = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint);
			if (CHumanArray::sTheObjectPtr != nullptr && id < static_cast<std::uint32_t>(CHumanArray::sTheObjectPtr->mCapacity))
			{
				const auto* human = GetHumanPtr(static_cast<std::int32_t>(id));
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) == Read<std::uint32_t>(handler, NC2Layout::AIHighLevelOffset::PlayerId) && (Remaster_AIGoods_IsWarehouseCarrier(human) || (RemasterAIEconomy::IsActive(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)) && Remaster_AIGoods_IsFieldLogisticsHuman(human)))) return true;
			}
		}
		if (AIScout_IsObjectUsed(bytes + 0x94E8, target)) return true;
		if (AIScout_IsObjectUsed(bytes + NC2Layout::AIHighLevelOffset::GuideManager, target)) return true;
		if (AITradeManager_IsObjectUsed(bytes + 0x74D4, target)) return true;
		return AIVillagesData_IsObjectUsed(bytes + 0x1C, target);
	}

	// Original function: FUN_0045360e.
	bool AITradeManager_IsObjectUsed(const void* manager, const void* target)
	{
		if (Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) != 1) return false;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(manager, NC2Layout::AITradeManagerOffset::Count); ++i)
			if (Read<std::uint32_t>(manager, NC2Layout::AITradeManagerOffset::FirstHumanId + i * NC2Layout::RecordSize::AITradeEntry) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint) && Read<std::uint32_t>(manager, NC2Layout::AITradeManagerOffset::FirstHumanUniqueId + i * NC2Layout::RecordSize::AITradeEntry) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision)) return true;
		return false;
	}

	// Original function: FUN_004536df.
	bool AIVillagesData_IsObjectUsed(const void* villages, const void* target)
	{
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			const auto* slot = static_cast<const std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlotUnsigned;
			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0 && AIVillageManager_IsObjectUsed(slot + NC2Layout::AIVillageSlotViewOffset::Village, target)) return true;
		}
		return false;
	}

	// Original function: FUN_00453cc3.
	void AIScout_Init(void* scout)
	{
		Write<std::uint32_t>(scout, NC2Layout::AIScoutOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum); Write<std::uint8_t>(scout, NC2Layout::AIScoutOffset::HumanRequestedFlag, 0);
	}

	// Original function: FUN_00453ccb.
	void AIScout_Set(void* scout, std::int32_t human)
	{
		Write(scout, NC2Layout::AIScoutOffset::HumanId, human);
		Write(scout, NC2Layout::AIScoutOffset::HumanRevision, Read<std::uint32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(human) * CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId));
		Write<std::uint8_t>(scout, NC2Layout::AIScoutOffset::HumanRequestedFlag, 0);
	}

	// Original function: FUN_00453ceb.
	bool AIScout_IsObjectUsed(const void* scout, const void* target)
	{
		return Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) == 1 && Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint) == Read<std::uint32_t>(scout, NC2Layout::AIScoutOffset::HumanId) && Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) == Read<std::uint32_t>(scout, NC2Layout::AIScoutOffset::HumanRevision);
	}

	// Original function: FUN_00453d0c.
	void AIScout_Clear(void* scout)
	{
		if (!AIGoodManager_Producer_IsHumanValid(scout)) return;
		auto* human = Human(scout);
		Human_DoAddNewUserCommand(human, 21, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, 6, 0);
		Write<std::uint32_t>(scout, NC2Layout::AIScoutOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_00453d3c.
	bool AIScout_CheckAndChangeJob(const void* scout)
	{
		auto* human = Human(scout);
		if (Human_IsJobScout(human)) return true;
		Human_DoAddNewUserCommand(human, 21, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, 27, 0);
		return false;
	}

	// Original function: FUN_00453d71; Windows static 004f4300 stores distance 50.
	bool AIScout_IsNearPos(const void* scout, const void* point)
	{
		const auto* human = Human(scout);
		const auto* position = Human_GetMapPositionRef(human);
		const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapX), Read<std::uint16_t>(position, NC2Layout::MapMigPointOffset::MapY)));
		return distance <= 50 && !Human_IsWalking(human);
	}

	// Original function: FUN_00453dc5.
	void AIScout_MoveToPos(const void* scout, const void* point)
	{
		auto* human = Human(scout);
		const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(player), id, revision, MapMigPoint_GetLong(point), 0);
	}

	// Original function: FUN_0045739b; every non-human target uses the house fields.
	bool AIGoodManager_IsObjectUsed(const void* manager, const void* target)
	{
		const auto offset = Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type) == 1 ? 0u : 8u;
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			const auto* entry = static_cast<const std::byte*>(manager) + good * NC2Layout::RecordSize::AIGoodUnsigned;
			for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount); ++i)
			{
				const auto* producer = entry + NC2Layout::AIGoodOffset::Producer + i * NC2Layout::RecordSize::AIGoodProducer;
				if (Read<std::uint32_t>(producer, offset) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint) && Read<std::uint32_t>(producer, offset + NC2Layout::ObjectReferenceOffset::Revision) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision)) return true;
			}
		}
		return false;
	}

	// Original function: FUN_004715fe.
	bool AIVillageManager_IsObjectUsed(const void* village, const void* target)
	{
		return AIGoodManager_IsObjectUsed(static_cast<const std::byte*>(village) + NC2Layout::AIVillageOffset::Goods, target);
	}

	// Original function: FUN_004574ac.
	std::uint8_t AIGoodManager_IsProducingGoodRealy(const void* manager, std::uint32_t good)
	{
		const auto* entry = static_cast<const std::byte*>(manager) + good * NC2Layout::RecordSize::AIGoodUnsigned;
		if (Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag) != 0 && Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::TraderBringFlag) != 0) return 0;
		return Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag);
	}

	// Original function: FUN_004574cc.
	bool AIGoodManager_Producer_IsHouseValid(void* producer)
	{
		const auto id = Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId);
		if (id == NXBasics::IntegerRange::UInt32Maximum) return false;
		const auto* house = CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseRevision) && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0) return true;
		Write<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId, NXBasics::IntegerRange::UInt32Maximum);
		return false;
	}

	// Original function: FUN_00457500.
	bool AIGoodManager_Producer_IsHumanValid(void* producer)
	{
		const auto id = Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId);
		if (id == NXBasics::IntegerRange::UInt32Maximum) return false;
		const auto* human = Human(producer);
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanRevision)) return true;
		Write<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum);
		return false;
	}

	// Original function: FUN_00457529.
	bool AIGoodManager_Producer_IsHumanValidCheck(const void* producer)
	{
		if (Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId) == NXBasics::IntegerRange::UInt32Maximum) return false;
		const auto* human = Human(producer);
		return Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanRevision);
	}

	// Windows shares the producer validity entries at 00457500 and 00457529 with scout records.
	bool AIScout_IsValid(void* scout)
	{
		return AIGoodManager_Producer_IsHumanValid(scout);
	}
	bool AIScout_IsValidCheck(const void* scout)
	{
		return AIGoodManager_Producer_IsHumanValidCheck(scout);
	}

	// Original function: FUN_00457958.
	void AIGoodManager_StopProduction(void* manager, std::uint32_t good)
	{
		auto* entry = Good(manager, good);
		Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount, 0);
		Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 0);
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount); ++i) AIGoodManager_Producer_Clear(static_cast<std::byte*>(entry) + NC2Layout::AIGoodOffset::Producer + i * NC2Layout::RecordSize::AIGoodProducer);
		Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 0);
	}

	// Original function: FUN_00457992.
	void AIGoodManager_Producer_Clear(void* producer)
	{
		Write<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HouseId, NXBasics::IntegerRange::UInt32Maximum);
		if (AIGoodManager_Producer_IsHumanValid(producer))
		{
			auto* human = Human(producer);
			// Remaster: retiring a flag collector must also cancel a pending collector job change.
			if (Human_UserControl_GetNextJobType(human) == SwitchIds::HumanJob::Collector && Human_IsAbleToDoJob(human, SwitchIds::HumanJob::Civilist)) Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), SwitchIds::HumanJob::Civilist, 0);
			else if (Human_IsAttachedToWorkHouse(human)) Human_DoAddNewUserCommand(human, 22, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
		}
		Write<std::uint32_t>(producer, NC2Layout::AIGoodProducerOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_004579d8.
	void AIGoodManager_ImportThisGoodType(void* manager, std::uint32_t good)
	{
		auto* entry = Good(manager, good);
		Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag, 1);
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount); ++i) AIGoodManager_Producer_Clear(static_cast<std::byte*>(entry) + NC2Layout::AIGoodOffset::Producer + i * NC2Layout::RecordSize::AIGoodProducer);
		Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount, 0);
	}

	// Original function: FUN_00457a0d.
	void AIGoodManager_SetTraderBringFlag(void* manager, std::uint32_t good, bool active)
	{
		Write<std::uint8_t>(manager, good * NC2Layout::RecordSize::AIGoodUnsigned + NC2Layout::AIGoodOffset::TraderBringFlag, active);
	}

	// Original function: FUN_00457a1f.
	std::uint32_t AIGoodManager_GetNumberOfUsedHumans(const void* manager)
	{
		std::uint32_t count = 0;
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			const auto* entry = static_cast<const std::byte*>(manager) + good * NC2Layout::RecordSize::AIGoodUnsigned;
			if (Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag) == 0) continue;
			for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount); ++i) if (AIGoodManager_Producer_IsHumanValidCheck(entry + NC2Layout::AIGoodOffset::Producer + i * NC2Layout::RecordSize::AIGoodProducer)) ++count;
		}
		return count;
	}
}
