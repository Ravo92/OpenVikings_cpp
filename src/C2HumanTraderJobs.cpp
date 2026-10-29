#include "C2Debug.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMerchantAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTraderJobs.h"
#include "C2HumanWorkCommands.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticVehicleTypeData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleLifecycle.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Vehicle(const void* human)
		{

			return CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
		}

		void* House(const void* data)
		{

			return MerchantHouseData_IsValid(data) ? CHouseArray::mStaticVars + Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) * CHouseArray::ElementSize : nullptr;
		}

		void* Merchant(const void* human)
		{

			return HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));
		}

		bool IsFoodGroup16(std::uint32_t good)
		{

			return good == 16 || good == 18 || good == 19 || good == 21 || good == 56 || good == 22 || good == 23;
		}

		bool IsFoodGroup17(std::uint32_t good)
		{

			return good == 17 || good == 20;
		}

		std::uint32_t NormalizedGood(std::uint32_t good)
		{

			return TGoodTypes_IsFood(good) ? (IsFoodGroup16(good) ? 16u : 17u) : good;
		}

		bool StockOnly(const void* house)
		{
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

			return type == 1 || type == 7 || type == 8 || type == 9;
		}

		void Fail(void* human, std::uint32_t reason)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		void Start(void* human, std::uint32_t animation)
		{
			Human_l_StartAtomicAnimation(human, animation, 6);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}

		void TargetVehicle(void* human)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 6);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));
		}

		void TargetHouseData(void* human, const void* data)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 5);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseRevision));
		}

		void EnterVehicle(void* human)
		{
			TargetVehicle(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 21);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}

		bool IsCommander(const void* human)
		{

			return Human_IsAttachedToVehicle(human) && Vehicle_Passengers_IsCommander(Vehicle(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		}

		bool IsRestockForbidden(void* house, std::uint32_t good)
		{

			return (good == 15 || good == 28 || good == 29) && House_GetHouseMainType(house) == 2;
		}

		bool WalkToHouse(void* human, void* house, const void* data)
		{
			TargetHouseData(human, data);

			if (!Human_l_StartAtomicWalkAndBring(human, static_cast<std::byte*>(house) + 0x1C, static_cast<std::byte*>(house) + 0x24))
			{
				Fail(human, 1);
				return false;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 4);

			return true;
		}

		bool WalkToVehicle(void* human)
		{
			auto* vehicle = Vehicle(human);
			std::uint32_t door = 0;
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
			const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

			if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0) || !Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, revision))
			{
				Fail(human, 1);
				return false;
			}

			if (!Human_l_StartAtomicWalkAndBring(human, Vehicle_GetMapPositionRef(vehicle), &door))
			{
				Vehicle_Stock_ModifyFutureAmount(vehicle, good, -1, revision);
				Fail(human, 1);
				return false;
			}

			TargetVehicle(human);

			return true;
		}

		bool WalkToPile(void* human)
		{
			std::uint32_t real = 0, work = 0;

			if (!Human_FindPilePointNearby(human, &real, &work))
				return false;

			Human_Target_SetTargetPositions(human, &real, &work);

			return Human_l_StartAtomicWalk(human, &real, &work);
		}

		void ResetTradeAmounts(void* merchant)
		{
			Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsDelivered, 0);
			Write<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsReceived, 0);
		}

		// Original function: FUN_0046e941; original outlined helper name is unresolved.
		void TraderFetchFromHouse(void* human, void* house, std::uint32_t good)
		{
			if (IsFoodGroup16(good) || IsFoodGroup17(good))
				for (std::uint32_t candidate = 16; candidate < 24; ++candidate)
				{
					if ((IsFoodGroup16(good) ? IsFoodGroup16(candidate) : IsFoodGroup17(candidate)) && House_Stock_CanStoreGoodType(house, candidate, true))
					{
						good = candidate;
						break;
					}
				}

			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 5);
			Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
			Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));

			if (!Human_l_StartAtomicWalkAndFetch(human, static_cast<std::byte*>(house) + 0x1C, static_cast<std::byte*>(house) + 0x24, false))
				Fail(human, 1);
		}

		// Original function: FUN_0046ea2b; original outlined helper name is unresolved.
		void TraderFetchFromVehicle(void* human, std::uint32_t good)
		{
			auto* vehicle = Vehicle(human);
			std::uint32_t door = 0;
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

			if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0) || !Vehicle_Stock_ModifyFutureAmount(vehicle, good, -1, revision))
			{
				Fail(human, 1);
				return;
			}

			if (!Human_l_StartAtomicWalk(human, Vehicle_GetMapPositionRef(vehicle), &door))
			{
				Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, revision);
				Fail(human, 1);

				return;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 2);
			TargetVehicle(human);
		}
	}

	// Original function: FUN_00467b97.
	std::uint32_t Human_l_Trader_GetNextGoodFromVehicle(void* human)
	{
		auto* merchant = Merchant(human);
		auto* currentData = MerchantData_GetCurrentHouseDataPtr(merchant);
		auto* nextData = MerchantData_GetNextHouseDataPtr(merchant);
		auto* current = House(currentData);
		auto* next = House(nextData);
		auto* vehicle = Vehicle(human);
		const bool unrestricted = Read<std::uint32_t>(currentData, NC2Layout::MerchantHouseOffset::ImportCount) == 0 && Read<std::uint32_t>(nextData, NC2Layout::MerchantHouseOffset::ImportCount) == 0;
		std::int32_t bestRequests = 0, bestBalance = 0;
		std::uint32_t selected = 0;

		for (std::uint32_t good = 0; good <= 55; ++good)
		{
			const auto normalized = NormalizedGood(good);

			if (!House_Stock_CanStoreGoodType(current, normalized, false) || Vehicle_Stock_GetAmountOfGoods(vehicle, good) == 0 || House_Stock_IsFullSoon(current, normalized))
				continue;

			if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(currentData, normalized))
			{
				if (!MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized))
					return good;

				continue;
			}

			if (IsRestockForbidden(current, good))
				continue;

			auto requests = NXBasics::Runtime_TimeSigned32(House_Stock_GetGoodRequestCounter(current, normalized));

			if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(nextData, good) && !MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized))
			{
				if (bestRequests == 0 || requests > bestRequests)
				{
					selected = good;

					if (requests == 0)
					{
						bestRequests = 1;
					}
					else if (requests > bestRequests)
					{
						bestRequests = requests;
					}
				}

				continue;
			}

			if (TGoodTypes_IsFood(good) && House_GetHouseMainType(next) == 2)
				continue;

			std::uint32_t difference = 0u - House_Stock_GetAmountOfGoods(current, normalized, StockOnly(current));

			if (House_Stock_CanStoreGoodType(next, good, false))
			{
				difference += House_Stock_GetAmountOfGoods(next, good, StockOnly(next));
				requests = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(requests) - House_Stock_GetGoodRequestCounter(next, good));
			}
			else
			{
				difference = 1;
			}

			const auto balance = NXBasics::Runtime_TimeSigned32(Vehicle_Stock_GetAmountOfGoods(vehicle, good) + difference) / 2;

			if (balance > bestBalance)
			{
				if (bestRequests != 0 && requests <= bestRequests)
					continue;

				selected = good;
				bestBalance = balance;

				if (requests == 0)
				{
					bestRequests = 1;
				}
				else if (requests > bestRequests)
				{
					bestRequests = requests;
				}
			}
			else if (requests > bestRequests)
			{
				selected = good;
				bestBalance = balance;
				bestRequests = requests;
			}
		}

		return selected;
	}

	// Original function: FUN_00467e55.
	std::uint32_t Human_l_Trader_GetNextGoodForNextHouse(void* human)
	{
		auto* merchant = Merchant(human);
		auto* currentData = MerchantData_GetCurrentHouseDataPtr(merchant);
		auto* nextData = MerchantData_GetNextHouseDataPtr(merchant);
		auto* current = House(currentData);
		auto* next = House(nextData);
		auto* vehicle = Human_IsAttachedToVehicle(human) ? Vehicle(human) : nullptr;
		const bool unrestricted = Read<std::uint32_t>(currentData, NC2Layout::MerchantHouseOffset::ImportCount) == 0 && Read<std::uint32_t>(nextData, NC2Layout::MerchantHouseOffset::ImportCount) == 0;
		std::int32_t bestRequests = 0, bestBalance = 0;
		std::uint32_t selected = 0;

		for (std::uint32_t good = 0; good <= 55; ++good)
		{
			const auto normalized = NormalizedGood(good);

			if (!House_Stock_CanStoreGoodType(next, normalized, false) || !House_Stock_CanStoreGoodType(current, good, true) || House_Stock_IsFullSoon(next, normalized))
				continue;

			if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized))
				continue;

			if (IsRestockForbidden(next, good) || (TGoodTypes_IsFood(good) && House_GetHouseMainType(current) == 2))
				continue;

			const auto currentAmount = House_Stock_GetAmountOfGoods(current, good, StockOnly(current));
			const auto nextAmount = House_Stock_GetAmountOfGoods(next, normalized, StockOnly(next));
			std::uint32_t vehicleAmount = 0;

			if (vehicle != nullptr)
			{
				if (Vehicle_Stock_IsFull(vehicle) || !Vehicle_Stock_CanStoreGoodType(vehicle, good))
					continue;

				vehicleAmount = Vehicle_Stock_GetAmountOfGoods(vehicle, good);
				const auto maximum = House_Stock_GetMaximumAmount(next, normalized, false);

				if (NXBasics::Runtime_TimeSigned32(vehicleAmount + nextAmount) >= NXBasics::Runtime_TimeSigned32(maximum))
					continue;
			}

			if (NXBasics::Runtime_TimeSigned32(currentAmount) <= NXBasics::Runtime_TimeSigned32(House_Stock_GetMinimumValue(current, good)))
				continue;

			auto requests = NXBasics::Runtime_TimeSigned32(House_Stock_GetGoodRequestCounter(next, normalized));

			if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(currentData, good) && !MerchantHouseData_GetGoodTypeImportFlag(currentData, normalized))
			{
				if (House_Stock_GetAmountOfGoods(current, good, true) == 0 || (bestRequests != 0 && requests <= bestRequests))
					continue;

				selected = good;

				if (requests == 0)
				{
					bestRequests = 1;
				}
				else if (requests > bestRequests)
				{
					bestRequests = requests;
				}

				continue;
			}

			std::uint32_t difference;

			if (House_Stock_CanStoreGoodType(current, good, true))
			{
				difference = currentAmount - nextAmount;
				requests = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(requests) - House_Stock_GetGoodRequestCounter(current, good));
			}
			else
			{
				difference = vehicleAmount + 2u;
			}

			const auto balance = NXBasics::Runtime_TimeSigned32(difference - vehicleAmount) / 2;

			if (balance > bestBalance)
			{
				if (bestRequests != 0 && requests <= bestRequests)
					continue;

				selected = good;
				bestBalance = balance;

				if (requests == 0)
				{
					bestRequests = 1;
				}
				else if (requests > bestRequests)
				{
					bestRequests = requests;
				}
			}
			else if (requests > bestRequests)
			{
				selected = good;
				bestBalance = balance;
				bestRequests = requests;
			}
		}

		return selected;
	}

	// Original function: FUN_00468188.
	bool Human_l_Trader_GoodForCurrentHouse(void* human, std::uint32_t good)
	{
		auto* merchant = Merchant(human);
		auto* currentData = MerchantData_GetCurrentHouseDataPtr(merchant);
		auto* nextData = MerchantData_GetNextHouseDataPtr(merchant);
		auto* current = House(currentData);
		auto* next = House(nextData);
		auto* vehicle = Human_IsAttachedToVehicle(human) ? Vehicle(human) : nullptr;
		const bool unrestricted = Read<std::uint32_t>(currentData, NC2Layout::MerchantHouseOffset::ImportCount) == 0 && Read<std::uint32_t>(nextData, NC2Layout::MerchantHouseOffset::ImportCount) == 0;
		const auto normalized = NormalizedGood(good);

		if (!House_Stock_CanStoreGoodType(current, normalized, false) || House_Stock_IsFullSoon(current, normalized) || IsRestockForbidden(current, good) || (TGoodTypes_IsFood(good) && House_GetHouseMainType(next) == 2))
			return false;

		if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(currentData, normalized))
			return !MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized);

		const auto requests = House_Stock_GetGoodRequestCounter(current, normalized);

		if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(nextData, good) && !MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized))
			return true;

		std::uint32_t difference = 1;

		if (House_Stock_CanStoreGoodType(next, good, false))
			difference = House_Stock_GetAmountOfGoods(next, good, StockOnly(next)) - House_Stock_GetAmountOfGoods(current, good, StockOnly(current));

		const auto available = vehicle != nullptr && Vehicle_Stock_CanStoreGoodType(vehicle, good) ? Vehicle_Stock_GetAmountOfGoods(vehicle, good) + 1u : 1u;

		if (NXBasics::Runtime_TimeSigned32((available + difference) & NXBasics::BitMask::DwordExceptBit0) > 0)
			return true;

		return NXBasics::Runtime_TimeSigned32(requests) > NXBasics::Runtime_TimeSigned32(House_Stock_GetGoodRequestCounter(next, good));
	}

	// Original function: FUN_0046842e.
	bool Human_l_Trader_GoodForNextHouse(void* human, std::uint32_t good)
	{
		auto* merchant = Merchant(human);
		auto* currentData = MerchantData_GetCurrentHouseDataPtr(merchant);
		auto* nextData = MerchantData_GetNextHouseDataPtr(merchant);
		auto* current = House(currentData);
		auto* next = House(nextData);
		auto* vehicle = Human_IsAttachedToVehicle(human) ? Vehicle(human) : nullptr;
		const bool unrestricted = Read<std::uint32_t>(currentData, NC2Layout::MerchantHouseOffset::ImportCount) == 0 && Read<std::uint32_t>(nextData, NC2Layout::MerchantHouseOffset::ImportCount) == 0;
		const auto normalized = NormalizedGood(good);

		if (!House_Stock_CanStoreGoodType(next, normalized, false) || IsRestockForbidden(next, good) || (TGoodTypes_IsFood(good) && House_GetHouseMainType(current) == 2) || House_Stock_IsFullSoon(next, normalized))
			return false;

		if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(nextData, normalized))
			return false;

		const auto requests = House_Stock_GetGoodRequestCounter(next, normalized);

		if (!unrestricted && !MerchantHouseData_GetGoodTypeImportFlag(currentData, good) && !MerchantHouseData_GetGoodTypeImportFlag(currentData, normalized))
			return true;

		std::uint32_t difference = 1;

		if (House_Stock_CanStoreGoodType(next, good, false))
			difference = House_Stock_GetAmountOfGoods(current, good, StockOnly(current)) - House_Stock_GetAmountOfGoods(next, good, StockOnly(next));

		const auto available = vehicle != nullptr && Vehicle_Stock_CanStoreGoodType(vehicle, good) ? Vehicle_Stock_GetAmountOfGoods(vehicle, good) + 1u : 1u;

		if (NXBasics::Runtime_TimeSigned32((available + difference) & NXBasics::BitMask::DwordExceptBit0) > 0)
			return true;

		return NXBasics::Runtime_TimeSigned32(requests) > NXBasics::Runtime_TimeSigned32(House_Stock_GetGoodRequestCounter(current, good));
	}

	// Original function: FUN_0046de23.
	void Human_l_StartTask_ExecuteJob_Trader(void* human)
	{
		auto* merchant = Merchant(human);
		auto* currentData = MerchantData_GetCurrentHouseDataPtr(merchant);
		auto* nextData = MerchantData_GetNextHouseDataPtr(merchant);

		if (currentData == nextData || currentData == nullptr)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}

		auto* current = House(currentData);
		auto* next = House(nextData);

		if (current == nullptr || next == nullptr || Read<std::uint8_t>(current, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint8_t>(next, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint32_t>(current, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint32_t>(next, NC2Layout::HouseOffset::ConstructionState) != 3)
		{
			Fail(human, 2);
			MerchantData_NextHouse(merchant);
			return;
		}

		if (Read<std::uint8_t>(currentData, NC2Layout::MerchantHouseOffset::Foreign) == 0 && Read<std::uint8_t>(nextData, NC2Layout::MerchantHouseOffset::Foreign) == 0)
		{
			if (IsCommander(human) && (Human_l_Trader_GetNextGoodFromVehicle(human) != 0 || Human_l_Trader_GetNextGoodForNextHouse(human) != 0) && Human_l_Trader_MoveVehicleNearHouse(human, current))
				return;

			const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

			if (circulation == 4)
			{
				const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

				if (Human_IsAtTargetPoint(human))
				{
					const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

					if (target == 5)
					{
						auto* house = Human_GetTargetHousePtr(human);

						if (house == current && House_Stock_CanStoreGoodType(house, cargo, false))
						{
							Human_l_JobExperience_Gain(human);
							Start(human, 23);
							return;
						}
					}
					else if (target == 6)
					{
						auto* vehicle = Vehicle(human);

						if (Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && !Vehicle_Stock_IsFull(vehicle) && MerchantHouseData_GetGoodTypeImportFlag(nextData, cargo))
						{
							EnterVehicle(human);
						}
						else
						{
							Fail(human, 0);
						}

						return;
					}
				}
				else
				{
					if (Human_l_Trader_GoodForCurrentHouse(human, cargo))
					{
						WalkToHouse(human, current, currentData);
						return;
					}

					if (Human_l_Trader_GoodForNextHouse(human, cargo))
					{
						if (IsCommander(human))
						{
							auto* vehicle = Vehicle(human);

							if (!Vehicle_Stock_IsFull(vehicle) && Vehicle_Stock_CanStoreGoodType(vehicle, cargo))
							{
								WalkToVehicle(human);
								return;
							}
						}

						MerchantData_NextHouse(merchant);
						return;
					}
				}

				Human_StopCurrentGoodCirculationMode(human);
			}
			else if (circulation == 2 && Human_l_Trader_FetchForWork(human, current))
			{
				return;
			}

			Human_Target_Reset(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
				return;
			}

			Human_GetMapPositionRef(human);

			if (Human_IsAttachedToVehicle(human) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 && Vehicle_Passengers_IsCommander(Vehicle(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)))
			{
				const auto good = Human_l_Trader_GetNextGoodFromVehicle(human);

				if (good != 0)
				{
					TraderFetchFromVehicle(human, good);
					return;
				}
			}

			const auto good = Human_l_Trader_GetNextGoodForNextHouse(human);

			if (good != 0)
			{
				TraderFetchFromHouse(human, current, good);
				return;
			}

			if (IsCommander(human))
			{
				const auto vehicleGood = Human_l_Trader_GetNextGoodFromVehicle(human);

				if (vehicleGood != 0)
				{
					TraderFetchFromVehicle(human, vehicleGood);
					return;
				}
			}

			MerchantData_NextHouse(merchant);
			Fail(human, 2);
			return;
		}

		if (!MerchantData_IsTradeAgreementValid(merchant, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId)))
		{
			ResetTradeAmounts(merchant);
			Fail(human, 15);
			MerchantData_NextHouse(merchant);
			return;
		}

		const auto* agreement = static_cast<std::byte*>(gHumanMerchantArray) + NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned + Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::TradeAgreementId) * NC2Layout::RecordSize::TradeAgreementUnsigned;

		if (!IsCommander(human))
		{
			Fail(human, 14);
			ResetTradeAmounts(merchant);
			MerchantData_NextHouse(merchant);
			return;
		}

		const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

		if (Read<std::uint8_t>(nextData, NC2Layout::MerchantHouseOffset::Foreign) != 0)
		{
			if (circulation == 4)
			{
				const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

				if (Human_IsAtTargetPoint(human))
				{
					const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

					if (target == 6)
					{
						auto* vehicle = Vehicle(human);

						if (Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && !Vehicle_Stock_IsFull(vehicle) && Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood) == cargo)
						{
							EnterVehicle(human);
						}
						else
						{
							Fail(human, 0);
						}

						return;
					}

					if (target == 5)
					{
						auto* house = Human_GetTargetHousePtr(human);

						if (house == current && House_Stock_CanStoreGoodType(house, cargo, false))
						{
							Human_l_JobExperience_Gain(human);
							Start(human, 23);
							return;
						}
					}
					else if (target == 1)
					{
						if (Map_GoodTool_CanBePutOnMap(cargo, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition))
						{
							Start(human, 23);
							return;
						}

						if (WalkToPile(human))
							return;
					}
				}
				else
				{
					const auto outgoing = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood);

					if (cargo == outgoing || (IsFoodGroup16(cargo) && outgoing == 16) || (IsFoodGroup17(cargo) && outgoing == 17))
					{
						auto* vehicle = Vehicle(human);

						if (!Vehicle_Stock_IsFull(vehicle) && Vehicle_Stock_CanStoreGoodType(vehicle, cargo))
						{
							WalkToVehicle(human);
							return;
						}
					}
					else if (House_Stock_CanStoreGoodType(current, cargo, false))
					{
						WalkToHouse(human, current, currentData);
						return;
					}
					else if (WalkToPile(human))
					{
						return;
					}
				}
			}
			else if (circulation == 2 && Human_l_Trader_FetchForWork(human, current))
			{
				return;
			}

			std::uint32_t excess = 0;

			switch (Human_l_Trader_Foreign_CheckGoods(human, current, agreement, &excess))
			{
				case SwitchIds::TraderGoodsCheck::Unavailable:
					Write(human, NC2Layout::HumanOffset::RequestedGood, Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood));
					Fail(human, 3);
					ResetTradeAmounts(merchant);
					MerchantData_NextHouse(merchant);
					return;
				case SwitchIds::TraderGoodsCheck::FetchFromHouse:
					if (!Human_l_Trader_MoveVehicleNearHouse(human, current))
						TraderFetchFromHouse(human, current, Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood));

					return;
				case SwitchIds::TraderGoodsCheck::VisitNextHouse:
					MerchantData_NextHouse(merchant);
					return;
				case SwitchIds::TraderGoodsCheck::FetchExcessFromVehicle:
					if (!Human_l_Trader_MoveVehicleNearHouse(human, current))
						TraderFetchFromVehicle(human, excess);

					return;
				default:
					break;
			}
		}
		else
		{
			auto recordIncoming = [human, current, merchant]()
			{
				Write(merchant, NC2Layout::MerchantOffset::GoodsReceived, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsReceived) + 1u);
				const auto offset = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer + Read<std::uint32_t>(current, NC2Layout::HouseOffset::PlayerId) * 4u + NC2Layout::GameControlPlayerOffset::GoodsReceivedFromPlayersUnsigned;
				Write(gGameControl, offset, Read<std::uint32_t>(gGameControl, offset) + 1u);
			};

			if (circulation == 4)
			{
				const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

				if (Human_IsAtTargetPoint(human))
				{
					const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

					if (target == 5)
					{
						auto* house = Human_GetTargetHousePtr(human);

						if (house == current && House_Stock_CanStoreGoodType(house, cargo, false))
						{
							Human_l_JobExperience_Gain(human);
							Start(human, 23);
							Write(merchant, NC2Layout::MerchantOffset::GoodsDelivered, Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsDelivered) + 1u);
							return;
						}
					}
					else if (target == 6)
					{
						auto* vehicle = Vehicle(human);

						if (Vehicle_Stock_CanStoreGoodType(vehicle, cargo) && !Vehicle_Stock_IsFull(vehicle) && Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingGood) == cargo)
						{
							EnterVehicle(human);
							recordIncoming();
						}
						else
						{
							Fail(human, 0);
						}

						return;
					}
				}
				else if (cargo == Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood))
				{
					WalkToHouse(human, current, currentData);
					return;
				}
				else if (cargo == Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingGood))
				{
					auto* vehicle = Vehicle(human);

					if (!Vehicle_Stock_IsFull(vehicle) && Vehicle_Stock_CanStoreGoodType(vehicle, cargo))
					{
						if (WalkToVehicle(human))
							recordIncoming();

						return;
					}
				}
			}
			else if (circulation == 2)
			{
				if (Human_l_Trader_FetchForWork(human, current))
					return;
			}
			else if (circulation == 0)
			{
				auto* vehicle = Vehicle(human);
				const auto given = Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsDelivered);

				if (given >= Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount))
				{
					if (Read<std::uint32_t>(merchant, NC2Layout::MerchantOffset::GoodsReceived) >= Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingAmount))
					{
						ResetTradeAmounts(merchant);

						if (Vehicle_Stock_GetAmountOfGoods(vehicle, Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood)) < Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount))
							MerchantData_NextHouse(merchant);

						return;
					}

					if (Human_l_Trader_MoveVehicleNearHouse(human, current))
						return;

					if (Vehicle_Stock_IsFull(vehicle))
					{
						ResetTradeAmounts(merchant);
						return;
					}

					TraderFetchFromHouse(human, current, Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingGood));
					return;
				}

				const auto outgoing = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood);

				if (Vehicle_Stock_GetAmountOfGoods(vehicle, outgoing) + given >= Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount))
				{
					if (!Human_l_Trader_MoveVehicleNearHouse(human, current))
						TraderFetchFromVehicle(human, outgoing);

					return;
				}

				Write(human, NC2Layout::HumanOffset::RequestedGood, outgoing);
				Fail(human, 3);
				MerchantData_NextHouse(merchant);

				return;
			}
		}

		MerchantData_NextHouse(merchant);
		Fail(human, 2);
	}

	// Original function: FUN_0046ead1.
	bool Human_l_Trader_MoveVehicleNearHouse(void* human, void* house)
	{
		if (!IsCommander(human))
			return false;

		auto* vehicle = Vehicle(human);
		const auto* position = Vehicle_GetMapPositionRef(vehicle);
		const auto* work = static_cast<std::byte*>(house) + 0x24;

		if (!Vehicle_IsPriorityCommandInQueue(vehicle) && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) != 1 && VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(work, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(work, NC2Layout::MapCoordinatePairOffset::Y)) <= 5)
			return false;

		if (Vehicle_IsPriorityCommandInQueue(vehicle) || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 ? 1u : 21u);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return true;
		}

		auto point = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
		const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

		if (Tool_Vehicle_GetNextMovePointNearPosition(type, work, &point, 20, 2))
		{
			std::uint32_t target = 0;
			Vehicle_GetTargetWorkPosition(vehicle, &target);
			std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 101);
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));

			if (point != target || Tool_Vehicle_GetNextMovePointNearPosition(type, &point, &target, 20, 1))
			{
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(&point));
				Vehicle_DoAddNewUserCommand(vehicle, command);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				return true;
			}
		}

		Fail(human, 1);

		if (Human_DoExecuteUserCommand_DetachVehicle(human) == 0)
			Fail(human, 0);

		return true;
	}

	// Original function: FUN_0046ecb0.
	bool Human_l_Trader_FetchForWork(void* human, void* house)
	{
		if (Human_IsAtTargetPoint(human))
		{
			const auto type = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

			if (type == 6)
			{
				auto* vehicle = Vehicle(human);

				if (Vehicle_Stock_GetAmountOfGoods(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)) > 0)
				{
					std::uint32_t door = 0;

					if (!Vehicle_Door_GetEntryPoint(vehicle, &door, 0))
					{
						Fail(human, 1);
						return true;
					}

					if (Human_l_StartAtomicWalk(human, Vehicle_GetMapPositionRef(vehicle), &door))
					{
						TargetVehicle(human);
					}
					else
					{
						Fail(human, 1);
					}

					return true;
				}
			}
			else if (type == 5)
			{
				auto* target = Human_GetHousePtrAtTargetPosition(human);

				if (target == house)
				{
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) != Read<std::uint32_t>(target, NC2Layout::HouseOffset::DoorEntryPoint))
						C2Debug_WarningBox("standing at invalid position for interaction with house stock!");

					const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);

					if (House_Stock_CanStoreGoodType(target, good, true) && House_Stock_GetAmountOfGoods(target, good, true) > House_Stock_GetMinimumValue(house, good))
					{
						const auto houseType = Read<std::uint32_t>(target, NC2Layout::HouseOffset::LogicType);
						Start(human, houseType == 10 ? 44 : houseType == 11 ? 45 : 22);
						return true;
					}
				}
			}
			else if (type == 2)
			{
				Start(human, 22);
				return true;
			}
		}

		Human_StopCurrentGoodCirculationMode(human);

		return false;
	}

	// Original function: FUN_0046ee28.
	std::uint32_t Human_l_Trader_Foreign_CheckGoods(void* human, void* house, const void* agreement, std::uint32_t* excessGood)
	{
		auto* vehicle = Vehicle(human);
		const auto outgoing = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingGood);

		for (std::uint32_t good = 1; good < 66; ++good)
		{
			if (good == outgoing || (IsFoodGroup16(good) && outgoing == 16) || (IsFoodGroup17(good) && outgoing == 17))
				continue;

			if (Vehicle_Stock_GetAmountOfGoods(vehicle, good) > 0)
			{
				*excessGood = good;
				return 3;
			}
		}

		const auto available = static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->stockSlots) - Vehicle_Stock_GetTotalAmount(vehicle);
		const auto amount = Vehicle_Stock_GetAmountOfGoods(vehicle, outgoing);
		const auto required = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::OutgoingAmount);
		const auto incoming = Read<std::uint32_t>(agreement, NC2Layout::TradeAgreementOffset::IncomingAmount);
		auto difference = incoming - required;

		if (NXBasics::Runtime_TimeSigned32(difference) < 0)
			difference = 0;

		const auto remaining = required - amount % required;
		const auto capacityNeeded = (amount / required + 1u) * difference + remaining;

		if (NXBasics::Runtime_TimeSigned32(capacityNeeded) <= NXBasics::Runtime_TimeSigned32(available) && (!TGoodTypes_IsFood(outgoing) || House_GetHouseMainType(house) != 2))
		{
			if (IsFoodGroup16(outgoing) || IsFoodGroup17(outgoing))
			{
				for (std::uint32_t good = 16; good < 24; ++good)
				{
					if (!(IsFoodGroup16(outgoing) ? IsFoodGroup16(good) : IsFoodGroup17(good)) || !House_Stock_CanStoreGoodType(house, good, true))
						continue;

					const auto stock = House_Stock_RequestAmountOfGoods(house, good, true);

					if (NXBasics::Runtime_TimeSigned32(stock - House_Stock_GetMinimumValue(house, good)) >= NXBasics::Runtime_TimeSigned32(remaining))
						return 1;
				}
			}
			else if (House_Stock_CanStoreGoodType(house, outgoing, true))
			{
				const auto stock = House_Stock_RequestAmountOfGoods(house, outgoing, true);

				if (NXBasics::Runtime_TimeSigned32(stock - House_Stock_GetMinimumValue(house, outgoing)) >= NXBasics::Runtime_TimeSigned32(remaining))
					return 1;
			}
		}

		return amount / required != 0 ? 2u : 0u;
	}
}