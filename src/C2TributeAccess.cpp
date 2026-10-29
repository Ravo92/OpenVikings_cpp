#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2TributeAccess.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstring>
#include <initializer_list>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00419e8e; the script-provided slot is unchecked.
	void TributeManager_CreateTribute(void* manager, std::uint32_t sourcePlayer, std::uint32_t targetPlayer, std::uint32_t slot)
	{
		TributeData_Init(static_cast<STributeData*>(manager) + slot, sourcePlayer, targetPlayer);
	}

	// Original function: FUN_00419ea9; inactive records do not accept goods.
	void TributeManager_AddTribute(void* manager, std::uint32_t slot, std::uint32_t good, std::uint32_t amount)
	{
		auto* tribute = static_cast<STributeData*>(manager) + slot;

		if (tribute->allocated != 0)
			TributeData_AddTribute(tribute, good, amount);
	}

	// Original function: FUN_00419ed5.
	std::uint32_t TributeManager_CountGoodAmountForTribute(std::uint32_t player, std::uint32_t good)
	{
		std::uint32_t total = 0;
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			const auto type = House_GetHouseMainType(house);

			if (type == 1 || type == 3)
				total += Tool_House_GetAmountOfGoods(house, &good, true);
		}

		return total;
	}

	// Original function: FUN_0043e55c; unused goods and padding are deliberately retained.
	void TributeData_Init(STributeData* tribute, std::uint32_t sourcePlayer, std::uint32_t targetPlayer)
	{
		tribute->goodCount = 0;
		tribute->descriptionStringId = NXBasics::IntegerRange::UInt32Maximum;
		tribute->sourcePlayer = sourcePlayer;
		tribute->paid = 1;
		tribute->targetPlayer = targetPlayer;
		tribute->allocated = 1;
	}

	// Original function: FUN_0043e57c; matching goods merge even when all five slots are used.
	bool TributeData_AddTribute(STributeData* tribute, std::uint32_t good, std::uint32_t amount)
	{
		std::uint32_t index = 0;

		while (index < tribute->goodCount && tribute->goods[index].good != good)
			++index;

		if (index == tribute->goodCount)
		{
			if (tribute->goodCount >= 5)
				return false;

			tribute->goods[index].good = good;
			tribute->goods[index].amount = amount;
			++tribute->goodCount;
		}
		else
		{
			tribute->goods[index].amount += amount;
		}

		tribute->paid = 0;

		return true;
	}

	// Original function: FUN_0043e5cc; the original demand is copied before aggregation.
	bool TributeData_Possible(const void* tribute)
	{
		std::byte remaining[60];
		std::memcpy(remaining, tribute, sizeof(remaining));
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::SourcePlayer)); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			const auto type = House_GetHouseMainType(house);

			if (type != 1 && type != 3)
				continue;

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount); ++i)
			{
				auto good = Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::FirstGoodUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned);
				const auto amount = Tool_House_GetAmountOfGoods(house, &good, true);
				Write(remaining, NC2Layout::TributeDataOffset::FirstAmountUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned, Read<std::uint32_t>(remaining, NC2Layout::TributeDataOffset::FirstAmountUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned) - amount);
			}

			std::uint32_t satisfied = 0;

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount); ++i)
			{
				if (Read<std::int32_t>(remaining, NC2Layout::TributeDataOffset::FirstAmountUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned) <= 0)
					++satisfied;
			}

			if (satisfied == Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount))
				return true;
		}

		return false;
	}

	// Original function: FUN_0043e694; the selected concrete food type is the one removed.
	bool TributeData_GetTributeFromHouse(void* tribute, std::int32_t id)
	{
		auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
		Write<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid, 1);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount); ++i)
		{
			const auto offset = 0x18u + i * 8u;
			const auto wanted = Read<std::uint32_t>(tribute, offset);

			if (wanted == 0)
				continue;

			auto good = Read<std::uint32_t>(tribute, offset - 4);
			auto amount = Tool_House_GetAmountOfGoods(house, &good, true);

			if (amount == 0)
				continue;

			auto remaining = wanted - amount;

			if (NXBasics::Runtime_TimeSigned32(remaining) < 0)
			{
				amount += remaining;
				remaining = 0;
			}

			Write(tribute, offset, remaining);
			House_Stock_ModifyAmount(house, nullptr, good, 0u - amount, false);
		}

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::GoodCount); ++i)
		{
			if (Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::FirstAmountUnsigned + i * NC2Layout::RecordSize::TributeGoodAmountUnsigned) != 0)
			{
				Write<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid, 0);
				break;
			}
		}

		return Read<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid) != 0;
	}

	// Original function: FUN_0043e72e; preferred stock, all stocks, then production houses.
	bool __fastcall TributeData_Pay(void* tribute, void*)
	{
		if (!TributeData_Possible(tribute))
			return false;

		std::byte remaining[60];
		std::memcpy(remaining, tribute, sizeof(remaining));
		const auto player = Read<std::uint32_t>(tribute, NC2Layout::TributeDataOffset::SourcePlayer);
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 1)
				continue;

			const bool paid = TributeData_GetTributeFromHouse(remaining, id);
			Write<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid, paid);

			if (paid)
				return true;

			break;
		}

		for (std::uint32_t type : { 1u, 3u })
		{
			for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
			{
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

				if (House_GetHouseMainType(house) != type)
					continue;

				const bool paid = TributeData_GetTributeFromHouse(remaining, id);
				Write<std::uint8_t>(tribute, NC2Layout::TributeDataOffset::Paid, paid);

				if (paid)
					return true;
			}
		}

		return false;
	}

	// Original function: FUN_004f288c; complete native array of forty 0x3C-byte records.
	void CTributeIterator::operator++(int)
	{
		++mIndex;

		while (mIndex < 40)
		{
			mCurrent = static_cast<std::byte*>(gTributeManager) + mIndex * 0x3Cu;

			if (Read<std::uint8_t>(mCurrent, NC2Layout::TributeDataOffset::Allocated) != 0 && (mSource == NXBasics::IntegerRange::UInt32Maximum || (Read<std::uint32_t>(mCurrent, NC2Layout::TributeDataOffset::SourcePlayer) == mSource && Read<std::uint32_t>(mCurrent, NC2Layout::TributeDataOffset::TargetPlayer) == mTarget)))
			{

				return;
			}

			++mIndex;
		}

		mCurrent = nullptr;
	}
}