#include "C2AIBuildingAccess.h"
#include "C2AIGlobalGoods.h"
#include "C2AIObjectAccess.h"
#include "C2AIResourceAccess.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Good(void* manager, std::uint32_t good)
		{

			return static_cast<std::byte*>(manager) + good * NC2Layout::RecordSize::AIGlobalGood;
		}

		const void* Good(const void* manager, std::uint32_t good)
		{

			return static_cast<const std::byte*>(manager) + good * NC2Layout::RecordSize::AIGlobalGood;
		}

		void* VillageGoods(void* villages, std::int32_t id)
		{

			return static_cast<std::byte*>(AIVillagesData_GetVillageDataById(villages, id)) + NC2Layout::AIVillageOffset::Goods;
		}
	}

	// Original function: FUN_004538b5.
	bool AIVillagesData_House_CanBuildAnyFurther(const void* villages, std::uint32_t house)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::AIGoodsLimits::Villages; ++i)
		{
			const auto* slot = static_cast<const std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;

			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0 && Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::HouseBuildAllowedFlags + house) != 0)
				return true;
		}

		return false;
	}

	// Original function: FUN_00453b68.
	std::uint32_t AIVillagesData_GetNumberOfUsedHumans(const void* villages)
	{
		std::uint32_t count = 0;

		for (std::uint32_t i = 0; i < NC2Runtime::AIGoodsLimits::Villages; ++i)
		{
			const auto* slot = static_cast<const std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;

			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0)
				count += AIGoodManager_GetNumberOfUsedHumans(slot + NC2Layout::AIVillageSlotViewOffset::Goods);
		}

		return count;
	}

	// Original function: FUN_00453b97; recursive need increments stop at the first producing village.
	bool AIVillagesData_Good_IsProducingGood(void* villages, std::uint32_t good, bool includeInputs)
	{
		for (std::uint32_t i = 0; i < NC2Runtime::AIGoodsLimits::Villages; ++i)
		{
			auto* slot = static_cast<std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;

			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) != 0 && AIGoodManager_IsProducingGood(slot + NC2Layout::AIVillageSlotViewOffset::Goods, good, includeInputs))
				return true;
		}

		return false;
	}

	// Original function: FUN_00456da5.
	void AIGlobalGoods_Init(void* manager)
	{
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			auto* entry = Good(manager, good);
			Write<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId, -1);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, 0);
		}
	}

	// Original function: FUN_00456dc0; the skip byte is read from the first village throughout the outer loop.
	void AIGlobalGoods_DoOneLogicGameTick(void* manager, void* villages)
	{
		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			auto* entry = Good(manager, good);
			Write<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId, -1);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer, 0);
			Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, 0);
		}

		for (std::uint32_t i = 0; i < NC2Runtime::AIGoodsLimits::Villages; ++i)
		{
			auto* slot = static_cast<std::byte*>(villages) + i * NC2Layout::RecordSize::AIVillageSlot;

			if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) == 0)
				continue;

			const auto id = Read<std::int32_t>(slot, NC2Layout::AIVillageSlotViewOffset::VillageId);
			auto* goods = slot + NC2Layout::AIVillageSlotViewOffset::Goods;

			for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
			{
				if (Read<std::uint8_t>(villages, NC2Layout::AIVillagesGoodsViewOffset::FirstForceProducingFlag + good * NC2Layout::RecordSize::AIGood) != 0 || good == 19 || good == 4 || good == 1 || good == 11)
					continue;

				auto* local = goods + good * NC2Layout::RecordSize::AIGood;
				std::uint32_t action = 0;

				if (AIGoodManager_IsProducingGoodRealy(goods, good) != 0)
				{
					action = AIGlobalGoods_l_AddProducer(manager, id, good, villages);
				}
				else if (Read<std::uint8_t>(local, NC2Layout::AIGoodOffset::ImportingFlag) != 0)
				{
					action = AIGlobalGoods_l_AddImporting(manager, id, good, villages);
				}
				else if (Read<std::uint32_t>(local, NC2Layout::AIGoodOffset::NeedCount) != 0)
				{
					action = AIGlobalGoods_l_AddNeed(manager, id, good, villages);
				}
				else if (Read<std::int32_t>(Good(manager, good), NC2Layout::AIGlobalGoodOffset::SelectedVillageId) == -1)
				{
					Write(Good(manager, good), NC2Layout::AIGlobalGoodOffset::SelectedVillageId, id);
				}

				switch (action)
				{
					case SwitchIds::AIGoodAction::ImportAndBring:
						AIGoodManager_ImportThisGoodType(goods, good);
						AIGoodManager_SetTraderBringFlag(goods, good, true);
						break;
					case SwitchIds::AIGoodAction::Bring:
						AIGoodManager_SetTraderBringFlag(goods, good, true);
						break;
					case SwitchIds::AIGoodAction::StopProduction:
						AIGoodManager_StopProduction(goods, good);
						break;
					case SwitchIds::AIGoodAction::StartProduction:
						AIGoodManager_IsProducingGood(goods, good, true);
						break;
					case SwitchIds::AIGoodAction::StopBringing:
						AIGoodManager_SetTraderBringFlag(goods, good, false);
						break;
					default:
						break;
				}
			}
		}

		for (std::uint32_t good = 0; good < NC2Runtime::AIGoodsLimits::Goods; ++good)
		{
			auto* entry = Good(manager, good);
			const auto producer = Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer);

			if (producer == 0)
			{
				if (Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount) == 0)
					continue;

				for (std::uint32_t i = 1; i < Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount); ++i)
				{
					AIGoodManager_StopProduction(VillageGoods(villages, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + i * NC2Layout::RecordSize::AIVillageId)), good);
				}
			}
			else if (Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount) != 0 && producer != 1 && Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId) != -1)
			{
				const auto id = Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId);
				AIGoodManager_IsProducingGood(VillageGoods(villages, id), good, true);
				const auto count = Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount);

				if (count < NC2Runtime::AIGoodsLimits::VillageIds)
				{
					Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + count * NC2Layout::RecordSize::AIVillageId, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId));
					Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, count + 1);
				}
			}
		}
	}

	// Original function: FUN_00456fb8.
	bool AIGlobalGoods_IsNeedingVillage(const void* manager, std::uint32_t good)
	{
		const auto* entry = Good(manager, good);

		return Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount) != 0 && Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer) != 1 && Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount) == 0;
	}

	// Original function: FUN_00456fe0.
	std::uint32_t AIGlobalGoods_l_AddNeed(void* manager, std::int32_t village, std::uint32_t good, void* villages)
	{
		auto* entry = Good(manager, good);

		if (Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer) == 1)
			return 1;

		if (Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId) != -1)
		{
			AIGoodManager_IsProducingGood(VillageGoods(villages, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId)), good, true);
			const auto count = Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount);

			if (count < NC2Runtime::AIGoodsLimits::VillageIds)
			{
				Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + count * NC2Layout::RecordSize::AIVillageId, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId));
				Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, count + 1);
			}

			Write<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId, -1);
			return 3;
		}

		const auto count = Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount);

		if (count < NC2Runtime::AIGoodsLimits::VillageIds)
		{
			Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + count * NC2Layout::RecordSize::AIVillageId, village);
			Write(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, count + 1);
		}

		return 0;
	}

	// Original function: FUN_0045704b.
	std::uint32_t AIGlobalGoods_l_AddProducer(void* manager, std::int32_t village, std::uint32_t good, void* villages)
	{
		auto* entry = Good(manager, good);

		if (Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer) == 1)
		{
			AIGoodManager_StopProduction(VillageGoods(villages, village), good);
			return 1;
		}

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount); ++i)
		{
			const auto id = Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + i * NC2Layout::RecordSize::AIVillageId);
			AIGoodManager_ImportThisGoodType(VillageGoods(villages, id), good);
			AIGoodManager_SetTraderBringFlag(VillageGoods(villages, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageIds + i * NC2Layout::RecordSize::AIVillageId)), good, true);
		}

		Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer, 1);
		Write<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::CandidateVillageCount, 0);
		Write(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId, village);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount); ++i)
		{
			AIGoodManager_SetTraderBringFlag(VillageGoods(villages, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageIds + i * NC2Layout::RecordSize::AIVillageId)), good, true);
		}

		return 4;
	}

	// Original function: FUN_00457110.
	std::uint32_t AIGlobalGoods_l_AddImporting(void* manager, std::int32_t village, std::uint32_t good, void* villages)
	{
		auto* entry = Good(manager, good);
		const auto count = Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount);

		if (count < NC2Runtime::AIGoodsLimits::VillageIds)
		{
			Write(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageIds + count * NC2Layout::RecordSize::AIVillageId, village);
			Write(entry, NC2Layout::AIGlobalGoodOffset::ImportVillageCount, count + 1);
		}

		if (Read<std::uint32_t>(entry, NC2Layout::AIGlobalGoodOffset::HasProducer) != 1)
			return 5;

		AIGoodManager_IsProducingGood(VillageGoods(villages, Read<std::int32_t>(entry, NC2Layout::AIGlobalGoodOffset::SelectedVillageId)), good, true);

		return 2;
	}
}