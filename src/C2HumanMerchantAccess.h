#pragma once
#include "C2LogicObjectArrays.h"
#include <cstddef>
#include <cstdint>
namespace NXBasics
{
	class CIniFileReader;
}
namespace NC2Logic
{
	// Physical Windows merchant routes; borrowed house IDs are validated by revision.
	struct SMerchantHouseStorage
	{
		std::int32_t houseId; std::uint32_t houseRevision;
		std::uint8_t foreign; std::byte padding09[3];
		std::uint32_t importCount;
		std::uint8_t importFlags[66]; std::byte unresolved52[2];
	};
	struct SMerchantStorage
	{
		std::uint8_t allocated; std::byte padding01[3];
		std::int32_t tradeAgreementId;
		std::uint32_t foreignHouseCount, goodsDelivered, goodsReceived, houseCount;
		std::int32_t currentHouseIndex;
		SMerchantHouseStorage houses[2];
	};
	struct STradeAgreementStorage
	{
		std::int32_t houseId; std::uint32_t houseRevision, outgoingGood, outgoingAmount, incomingGood, incomingAmount;
	};
	struct SMerchantManagerStorage
	{
		alignas(CArrayHandler) std::byte arrayHandler[sizeof(CArrayHandler)]; void* entries; std::uint32_t tradeAgreementCount; STradeAgreementStorage tradeAgreements[60];
	};
	static_assert(sizeof(SMerchantHouseStorage) == 84);
	static_assert(sizeof(SMerchantStorage) == 196);
	static_assert(sizeof(STradeAgreementStorage) == 24);
	static_assert(sizeof(SMerchantManagerStorage) == (sizeof(void*) == 8 ? 1528 : 1496));
	static_assert(offsetof(SMerchantManagerStorage, entries) == sizeof(CArrayHandler));
	static_assert(offsetof(SMerchantManagerStorage, tradeAgreementCount) == sizeof(CArrayHandler) + sizeof(void*));
	static_assert(offsetof(SMerchantManagerStorage, tradeAgreements) == sizeof(CArrayHandler) + sizeof(void*) + 4);
	static_assert(offsetof(SMerchantManagerStorage, tradeAgreements) + sizeof(SMerchantManagerStorage::tradeAgreements) - offsetof(SMerchantManagerStorage, tradeAgreementCount) == 1444);
	bool MerchantData_AddHouse(void* merchant, std::int32_t house, std::uint32_t revision, bool foreign);
	void MerchantData_ClearHouse(void* merchant, std::int32_t house, std::uint32_t revision);
	void MerchantData_NextHouse(void* merchant);
	bool MerchantData_IsHouseInArray(const void* merchant, std::int32_t house, std::uint32_t revision);
	void* MerchantData_GetHouseDataPtr(void* merchant, std::int32_t house, std::uint32_t revision);
	void* MerchantData_GetCurrentHouseDataPtr(void* merchant);
	void* MerchantData_GetNextHouseDataPtr(void* merchant);
	bool MerchantData_IsTradeAgreementValid(void* merchant, std::int32_t player);
	bool MerchantData_Tool_CanHouseBeAdded(const void* merchant, std::int32_t human, std::int32_t house);
	void MerchantHouseData_Init(void* data, const void* house, bool foreign);
	void MerchantHouseData_SetGoodTypeImportFlag(void* data, std::uint32_t good, bool enabled);
	bool MerchantHouseData_GetGoodTypeImportFlag(const void* data, std::uint32_t good);
	bool TradeAgreement_IsValid(const void* agreement);
	std::uint32_t HumanMerchantArray_GetNextTradeAgreementId(const void* manager, const void* house, std::uint32_t previous);
	void HumanMerchantArray_AddTradeAgreement(void* manager, std::uint32_t missionCheck, std::uint32_t firstGood, std::uint32_t firstAmount, std::uint32_t secondGood, std::uint32_t secondAmount);
	void __fastcall HumanMerchantArray_CleanMap_LoadTradeAgreements(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	bool __cdecl HumanMerchantArray_l_LogicCallback(std::uint32_t type, std::uintptr_t callbackId, std::uintptr_t firstPlayer, std::uintptr_t secondPlayer, std::uintptr_t state);
	void Human_l_Trader_ResetImportFlags(void* human);
	std::uint32_t Human_DoExecuteUserCommand_AttachTradeHouse(void* human, std::int32_t house, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_DetachTradeHouse(void* human, std::int32_t house, std::uint32_t revision);
}
