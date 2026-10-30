#include "C2DataIds.h"
#include "C2GameControlNetwork.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::int32_t id)
		{

			return CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * CHouseArray::RuntimeElementSize;
		}

		void* Human(std::int32_t id)
		{

			return CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * CHumanArray::RuntimeElementSize;
		}
	}

	// Original function: FUN_0041c61c.
	bool MerchantData_AddHouse(void* merchant, std::int32_t id, std::uint32_t revision, bool foreign)
	{
		if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) >= NC2Runtime::MerchantLimits::Houses)
			return false;

		const void* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != revision || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
			return false;

		std::int32_t freeSlot = -1;

		for (std::uint32_t slot = 0; slot < NC2Runtime::MerchantLimits::Houses; ++slot)
		{
			const void* data = MerchantData_GetHouseDataPtr(merchant, slot);

			if (Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) == id && Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId))
				return false;

			if (!MerchantHouseData_IsValid(data) && freeSlot == -1)
				freeSlot = static_cast<std::int32_t>(slot);
		}

		if (freeSlot == -1)
			return false;

		MerchantHouseData_Init(MerchantData_GetHouseDataPtr(merchant, static_cast<std::uint32_t>(freeSlot)), house, foreign);
		Write(merchant, NC2Layout::MerchantOffset::HouseCount, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) + 1u);

		if (foreign)
			Write(merchant, NC2Layout::MerchantOffset::ForeignHouseCount, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount) + 1u);

		return true;
	}

	// Original function: FUN_0041c6c1.
	void MerchantData_ClearHouse(void* merchant, std::int32_t id, std::uint32_t revision)
	{
		if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) == 0)
			return;

		for (std::uint32_t slot = 0; slot < NC2Runtime::MerchantLimits::Houses; ++slot)
		{
			void* data = MerchantData_GetHouseDataPtr(merchant, slot);

			if (Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) != id || Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision) != revision)
				continue;

			if (Read<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::Foreign) != 0)
				Write(merchant, NC2Layout::MerchantOffset::ForeignHouseCount, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount) - 1u);

			if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount) == 0)
				Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::TradeAgreementId, -1);

			Write<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId, -1);
			Write(merchant, NC2Layout::MerchantOffset::HouseCount, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) - 1u);

			if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex) == slot)
				MerchantData_NextHouse(merchant);

			return;
		}
	}

	// Original function: FUN_0041c718.
	bool MerchantData_IsHouseInArray(const void* merchant, std::int32_t id, std::uint32_t revision)
	{
		if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) == 0)
			return false;

		for (std::uint32_t slot = 0; slot < NC2Runtime::MerchantLimits::Houses; ++slot)
		{
			const auto offset = NC2Layout::MerchantOffset::HousesUnsigned + slot * NC2Layout::RecordSize::MerchantHouseUnsigned;

			if (Read<std::int32_t>(merchant, offset) == id && Read<std::uint32_t>(merchant, offset + NC2Layout::MerchantHouseOffset::HouseRevision) == revision)
				return true;
		}

		return false;
	}

	// Original function: FUN_0041c745; invalid matches are removed while searching.
	void* MerchantData_GetHouseDataPtr(void* merchant, std::int32_t id, std::uint32_t revision)
	{
		if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) == 0)
			return nullptr;

		for (std::uint32_t slot = 0; slot < NC2Runtime::MerchantLimits::Houses; ++slot)
		{
			void* data = MerchantData_GetHouseDataPtr(merchant, slot);

			if (Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) != id || Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision) != revision)
				continue;

			if (MerchantHouseData_IsValid(data))
				return data;

			MerchantData_ClearHouse(merchant, id, revision);
		}

		return nullptr;
	}

	// Original function: FUN_0041c7a7.
	void* MerchantData_GetCurrentHouseDataPtr(void* merchant)
	{
		if (Read<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex) == -1)
			MerchantData_NextHouse(merchant);

		const auto slot = Read<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex);

		if (slot == -1)
			return nullptr;

		const void* data = MerchantData_GetHouseDataPtr(merchant, static_cast<std::uint32_t>(slot));

		return MerchantData_GetHouseDataPtr(merchant, Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId), Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision));
	}

	// Original function: FUN_0041c7d6; lookup side effects survive restoring the index.
	void* MerchantData_GetNextHouseDataPtr(void* merchant)
	{
		const auto saved = Read<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex);
		MerchantData_NextHouse(merchant);
		void* result = nullptr;
		const auto slot = Read<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex);

		if (slot != -1)
		{
			const void* data = MerchantData_GetHouseDataPtr(merchant, static_cast<std::uint32_t>(slot));
			result = MerchantData_GetHouseDataPtr(merchant, Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId), Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision));
		}

		Write(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, saved);

		return result;
	}

	// Original function: FUN_0041c807; ClearHouse can recursively advance the index.
	void MerchantData_NextHouse(void* merchant)
	{
		while (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) != 0)
		{
			auto slot = Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex) + 1u;
			Write(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, slot);

			for (; slot < NC2Runtime::MerchantLimits::Houses; ++slot)
			{
				void* data = MerchantData_GetHouseDataPtr(merchant, slot);

				if (Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) == -1)
					continue;

				if (MerchantHouseData_IsValid(data))
				{
					Write(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, slot);

					return;
				}

				MerchantData_ClearHouse(merchant, Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId), Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision));
			}

			Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, -1);
		}

		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::CurrentHouseIndex, -1);
	}

	// Original function: FUN_0041c85e.
	bool MerchantData_IsTradeAgreementValid(void* merchant, std::int32_t player)
	{
		const auto id = Read<std::int32_t>(merchant, NC2Layout::MerchantOffset::TradeAgreementId);

		if (id == -1)
			return false;

		const void* agreement = static_cast<const std::byte*>(gHumanMerchantArray) + NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + id * NC2Layout::RecordSize::TradeAgreementUnsigned;

		if (TradeAgreement_IsValid(agreement) && GameControl_Diplomacy_GetState(gGameControl, player, Read<std::int32_t>(House(Read<std::int32_t>(agreement, NC2Layout::TradeAgreementOffset::HouseId)), NC2Layout::HouseOffset::PlayerId)) == 1)
			return true;

		Write<std::int32_t>(merchant, NC2Layout::MerchantOffset::TradeAgreementId, -1);

		return false;
	}

	// Original function: FUN_0041c8b7; the caller validates allocation/revision.
	bool MerchantData_Tool_CanHouseBeAdded(const void* merchant, std::int32_t humanId, std::int32_t houseId)
	{
		if (houseId == -1)
			return false;

		const void* human = Human(humanId);
		const void* house = House(houseId);

		if (MerchantData_IsHouseInArray(merchant, houseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId)) || Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount) >= NC2Runtime::MerchantLimits::Houses)
			return false;

		const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), owner = Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId);

		if (player == owner)
			return true;

		if (HumanMerchantArray_GetNextTradeAgreementId(gHumanMerchantArray, house, NXBasics::IntegerRange::UInt32Maximum) == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		if (GameControl_Diplomacy_GetState(gGameControl, player, owner) != 1 || GameControl_Diplomacy_GetState(gGameControl, owner, player) != 1)
			return false;

		return Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount) == 0;
	}

	// Original function: FUN_0041c956; clear exactly the house stock-slot count.
	void MerchantHouseData_Init(void* data, const void* house, bool foreign)
	{
		Write(data, NC2Layout::MerchantHouseOffset::HouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
		Write(data, NC2Layout::MerchantHouseOffset::HouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
		Write<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::ImportCount, 0);
		Write<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::Foreign, foreign);
		std::memset(static_cast<std::byte*>(data) + NC2Layout::MerchantHouseOffset::ImportFlags, 0, House_GetHouseTypeDataPtr(house)->stockCount);
	}

	// Original function: FUN_0041c9b7; imported-good count changes only on transition.
	void MerchantHouseData_SetGoodTypeImportFlag(void* data, std::uint32_t good, bool enabled)
	{
		const auto slot = House_GetHouseTypeDataPtr(House(Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId)))->stockIndices[good];

		if (slot == -1 || Read<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::ImportFlagsUnsigned + slot) == static_cast<std::uint8_t>(enabled))
		{

			return;
		}

		Write(data, NC2Layout::MerchantHouseOffset::ImportCount, Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::ImportCount) + (enabled ? 1u : NXBasics::IntegerRange::UInt32Maximum));
		Write<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::ImportFlagsUnsigned + slot, enabled);
	}

	// Original function: FUN_0041c9fd.
	bool MerchantHouseData_GetGoodTypeImportFlag(const void* data, std::uint32_t good)
	{
		const auto slot = House_GetHouseTypeDataPtr(House(Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId)))->stockIndices[good];

		return slot != -1 && Read<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::ImportFlagsUnsigned + slot) != 0;
	}

	// Original function: FUN_0041ca2f.
	bool TradeAgreement_IsValid(const void* agreement)
	{
		if (Read<std::int32_t>(agreement, NC2Layout::TradeAgreementOffset::HouseId) == -1)
			return false;

		const void* house = House(Read<std::int32_t>(agreement, NC2Layout::TradeAgreementOffset::HouseId));

		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::HouseRevision) && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0;
	}

	// Original function: FUN_0041ca5e.
	void HumanMerchantArray_AddTradeAgreement(void* manager, std::uint32_t missionCheck, std::uint32_t firstGood, std::uint32_t firstAmount, std::uint32_t secondGood, std::uint32_t secondAmount)
	{
		for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(missionCheck); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(missionCheck, id))
		{
			const void* house = House(id);
			const auto owner = Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId);

			if (owner < 20 && Read<std::uint32_t>(gGameControl, owner * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::Type) == 1)
				continue;

			const auto count = Read<std::uint32_t>(manager, NC2Layout::MerchantManagerOffset::TradeAgreementCount);

			if (count == NC2Runtime::MerchantLimits::TradeAgreements)
			{

				return;
			}

			void* agreement = static_cast<std::byte*>(manager) + NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + count * NC2Layout::RecordSize::TradeAgreementUnsigned;
			Write(manager, NC2Layout::MerchantManagerOffset::TradeAgreementCount, count + 1u);
			Write(agreement, NC2Layout::TradeAgreementOffset::HouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
			Write(agreement, NC2Layout::TradeAgreementOffset::HouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			Write(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood, firstGood);
			Write(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount, firstAmount);
			Write(agreement, NC2Layout::TradeAgreementOffset::IncomingGood, secondGood);
			Write(agreement, NC2Layout::TradeAgreementOffset::IncomingAmount, secondAmount);
		}
	}

	// Original function: FUN_0041cae4; unsigned wrap makes -1 start at zero.
	std::uint32_t HumanMerchantArray_GetNextTradeAgreementId(const void* manager, const void* house, std::uint32_t previous)
	{
		for (auto id = previous + 1u; id < Read<std::uint32_t>(manager, NC2Layout::MerchantManagerOffset::TradeAgreementCount); ++id)
		{
			const auto offset = NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + id * NC2Layout::RecordSize::TradeAgreementUnsigned;

			if (Read<std::uint32_t>(manager, offset) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId) && Read<std::uint32_t>(manager, offset + NC2Layout::TradeAgreementOffset::HouseRevision) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId))
				return id;
		}

		return NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0041cb1e; complete token region 004F3898..004F38A7.
	void __fastcall HumanMerchantArray_CleanMap_LoadTradeAgreements(void* manager, void*, NXBasics::CIniFileReader* reader)
	{
		static const NXBasics::SToken tokens[] = { { "tradeagreement", NC2DataIds::HumanMerchantToken::TradeAgreement }, { nullptr, 0 } };

		for (auto* group = reader->GetFirstGroup("misc_tradeagreement"); group != nullptr; group = reader->GetNextGroup("misc_tradeagreement"))
		{
			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				if (NXBasics::XB_Token_FindTokenInArray(tokens, reader->GetCurrentName()) != NC2DataIds::HumanMerchantToken::TradeAgreement)
					continue;

				const auto missionCheck = reader->GetInt(), firstGood = reader->GetInt(), firstAmount = reader->GetInt(), secondGood = reader->GetInt(), secondAmount = reader->GetInt();
				HumanMerchantArray_AddTradeAgreement(manager, missionCheck, firstGood, firstAmount, secondGood, secondAmount);
			}
		}
	}

	// Original function: FUN_0041cbb8; preserve the count-changing traversal.
	bool __cdecl HumanMerchantArray_l_LogicCallback(std::uint32_t type, std::uintptr_t, std::uintptr_t callbackValueFirstPlayer, std::uintptr_t callbackValueSecondPlayer, std::uintptr_t callbackValueState)
	{
		std::uint32_t firstPlayer = static_cast<std::uint32_t>(callbackValueFirstPlayer);
		std::uint32_t secondPlayer = static_cast<std::uint32_t>(callbackValueSecondPlayer);
		std::uint32_t state = static_cast<std::uint32_t>(callbackValueState);

		if (type != 0x1A || state == 1)
			return true;

		CHumanIterator iterator;
		const auto removeForeignHouses = [&](std::uint32_t player, std::uint32_t other)
		{
			for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
			{
				const void* human = Human(id);

				if (!Human_IsJobTrader(human))
					continue;

				void* merchant = HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));

				for (std::uint32_t slot = 0; slot < Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount); ++slot)
				{
					void* data = MerchantData_GetHouseDataPtr(merchant, slot);

					if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::ForeignHouseCount) == 0 || Read<std::uint8_t>(data, NC2Layout::MerchantHouseOffset::Foreign) == 0)
						continue;

					const void* house = MerchantHouseData_IsValid(data) ? House(Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId)) : nullptr;

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == other)
						MerchantData_ClearHouse(merchant, Read<std::int32_t>(data, NC2Layout::MerchantHouseOffset::HouseId), Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision));
				}
			}
		};
		removeForeignHouses(firstPlayer, secondPlayer);
		removeForeignHouses(secondPlayer, firstPlayer);

		return true;
	}

	// Original function: FUN_004683ec; cycles every route entry, clearing all 66 goods.
	void Human_l_Trader_ResetImportFlags(void* human)
	{
		if (!Human_IsJobTrader(human))
		{

			return;
		}

		void* merchant = HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));
		auto remaining = Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::HouseCount);

		while (remaining != 0)
		{
			void* data = MerchantData_GetCurrentHouseDataPtr(merchant);

			for (std::uint32_t good = 0; good < NC2Runtime::MerchantLimits::Goods; ++good)
			{
				MerchantHouseData_SetGoodTypeImportFlag(data, good, false);
			}

			MerchantData_NextHouse(merchant);
			--remaining;
		}
	}

	// Original function: FUN_0043cfc4; no atomic-interruptibility prerequisite.
	std::uint32_t Human_DoExecuteUserCommand_AttachTradeHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (!Human_IsJobTrader(human))
			return 1;

		void* merchant = HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));

		if (!MerchantData_AddHouse(merchant, id, revision, Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)))
			return 1;

		Human_l_Trader_ResetImportFlags(human);

		return 2;
	}

	// Original function: FUN_0043d021; reset imports even when no entry matched.
	std::uint32_t Human_DoExecuteUserCommand_DetachTradeHouse(void* human, std::int32_t id, std::uint32_t revision)
	{
		if (!Human_IsJobTrader(human))
			return 1;

		MerchantData_ClearHouse(HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)), id, revision);
		Human_l_Trader_ResetImportFlags(human);

		return 2;
	}
}