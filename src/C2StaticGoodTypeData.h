#pragma once

#include "XBTokenList.h"

namespace NC2Logic
{
	struct SStaticGoodTypeData
	{
		char name[30];
		std::byte unknown1E[2];
		std::int32_t landscapeType;
		std::int32_t engineLandscapeId;
		std::int32_t weaponType;
		std::int32_t armorType;
		std::uint8_t isInputGood;
		std::uint8_t isProducedInHouse;
		std::uint8_t isProducedOnMap;
		std::uint8_t isBioLandscape;
		std::int32_t landscapeToHarvest;
		std::int32_t landscapeToPickup;
		std::int32_t landscapeToStore;
		std::int32_t atomicForHarvesting;
		std::int32_t atomicForCultivating;
		std::int32_t atomicForPlanting;
		std::int32_t atomicForProduction;
		std::uint32_t productionInputCount;
		std::uint32_t distinctProductionInputCount;
		std::int32_t distinctProductionInputs[10];
		std::uint32_t productionInputAmounts[10];
		std::int32_t productionInputs[10];
	};

	class CStaticGoodTypeDataManager
	{
	  public:
		CStaticGoodTypeDataManager();
		~CStaticGoodTypeDataManager();
		static void Misc_GetEngineLandscapeIds();
		CStaticGoodTypeDataManager(const CStaticGoodTypeDataManager&) = delete;
		CStaticGoodTypeDataManager& operator=(const CStaticGoodTypeDataManager&) = delete;

	  private:
		std::uint8_t mObjectByte;
	};

	// Complete original region: 00511238..005112D7, including all token rows.
	struct SStaticGoodTypeStorage
	{
		std::uint8_t tokenInitFlags;
		std::byte padding01[sizeof(void*) - 1];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		SStaticGoodTypeData* records;
		CStaticGoodTypeDataManager* owner;
		NXBasics::SToken tokens[18];
	};

	extern SStaticGoodTypeStorage gStaticGoodTypeStorage;

	static_assert(sizeof(SStaticGoodTypeData) == 208);
	static_assert(offsetof(SStaticGoodTypeData, landscapeType) == 0x20);
	static_assert(offsetof(SStaticGoodTypeData, engineLandscapeId) == 0x24);
	static_assert(offsetof(SStaticGoodTypeData, weaponType) == 0x28);
	static_assert(offsetof(SStaticGoodTypeData, armorType) == 0x2C);
	static_assert(offsetof(SStaticGoodTypeData, isInputGood) == 0x30);
	static_assert(offsetof(SStaticGoodTypeData, isProducedInHouse) == 0x31);
	static_assert(offsetof(SStaticGoodTypeData, isProducedOnMap) == 0x32);
	static_assert(offsetof(SStaticGoodTypeData, isBioLandscape) == 0x33);
	static_assert(offsetof(SStaticGoodTypeData, landscapeToHarvest) == 0x34);
	static_assert(offsetof(SStaticGoodTypeData, landscapeToPickup) == 0x38);
	static_assert(offsetof(SStaticGoodTypeData, landscapeToStore) == 0x3C);
	static_assert(offsetof(SStaticGoodTypeData, atomicForHarvesting) == 0x40);
	static_assert(offsetof(SStaticGoodTypeData, atomicForCultivating) == 0x44);
	static_assert(offsetof(SStaticGoodTypeData, atomicForPlanting) == 0x48);
	static_assert(offsetof(SStaticGoodTypeData, atomicForProduction) == 0x4C);
	static_assert(offsetof(SStaticGoodTypeData, productionInputCount) == 0x50);
	static_assert(offsetof(SStaticGoodTypeData, distinctProductionInputCount) == 0x54);
	static_assert(offsetof(SStaticGoodTypeData, distinctProductionInputs) == 0x58);
	static_assert(offsetof(SStaticGoodTypeData, productionInputAmounts) == 0x80);
	static_assert(offsetof(SStaticGoodTypeData, productionInputs) == 0xA8);
	static_assert(sizeof(CStaticGoodTypeDataManager) == 1);
	static_assert(sizeof(SStaticGoodTypeStorage) == (sizeof(void*) == 8 ? 320 : 160));
	static_assert(offsetof(SStaticGoodTypeStorage, tokenList) == sizeof(void*));
	static_assert(offsetof(SStaticGoodTypeStorage, records) == 2 * sizeof(void*));
	static_assert(offsetof(SStaticGoodTypeStorage, owner) == 3 * sizeof(void*));
	static_assert(offsetof(SStaticGoodTypeStorage, tokens) == 4 * sizeof(void*));
	const SStaticGoodTypeData* StaticGoodTypeData_GetPtr(std::uint32_t good);
}