#pragma once

#include "XBasicTypes.h"
#include "XBTokenList.h"

#include <cstdint>

namespace NC2Logic
{
	// Logic IDs from the Windows house definitions; stored integer fields retain their native layouts.
	enum class EHouseType : std::uint32_t
	{
		None = 0,
		Headquarters = 1,
		Home00 = 2,
		Home01 = 3,
		Home02 = 4,
		Home03 = 5,
		Home04 = 6,
		Stock00 = 7,
		Stock01 = 8,
		Stock02 = 9,
		Well = 10,
		Hive = 11,
		Farm = 12,
		Mill = 13,
		Bakery00 = 14,
		Bakery01 = 15,
		Brewery = 16,
		Breedery = 17,
		Sewery00 = 18,
		Sewery01 = 19,
		Pottery00 = 20,
		Pottery01 = 21,
		Pottery02 = 22,
		Joinery00 = 23,
		Joinery01 = 24,
		Joinery02 = 25,
		Joinery03 = 26,
		Armory00 = 27,
		Armory01 = 28,
		MasonHut00 = 29,
		MasonHut01 = 30,
		Smithy00 = 31,
		Smithy01 = 32,
		CoinMint = 33,
		HerbHut = 34,
		DruidHut00 = 35,
		DruidHut01 = 36,
		Temple = 37,
		School = 38,
		Barracks = 39,
		Tower00 = 40,
		Tower01 = 41,
		VehicleCartHand = 42,
		VehicleCartOx = 43,
		VehicleShipSmall = 44,
		VehicleShipBig = 45,
		VehicleCatapult = 46,
		WonderLighthouseOfAlexandria = 47,
		WonderGardensOfSemiramis = 48,
		WonderRhodos = 49,
		WonderMausolos = 50,
		WonderArtemisTemple = 51,
		WonderZeusStatue = 52,
		WonderPyramidOfGizeh = 53,
		WonderEighthWonder = 54,
	};

	struct SStaticHouseWorker
	{
		std::int32_t jobType;
		std::int32_t maximumWorkers;
	};

	struct SStaticHouseStock
	{
		std::int32_t goodType;
		std::int32_t maximumAmount;
		std::uint8_t isOutputStock;
		std::byte padding09[3];
	};

	struct SStaticHouseTypeData
	{
		std::int32_t mainType;
		std::int32_t homeSize;
		std::int32_t schoolSize;
		std::int32_t workerCount;
		SStaticHouseWorker workers[10];
		std::uint32_t stockCount;
		SStaticHouseStock stock[66];
		std::int32_t stockIndices[66];
		std::uint32_t productionCount;
		std::int32_t productionGoods[20];
		std::int32_t productionIndices[66];
		std::uint8_t ignoreContinents;
		std::byte padding5E1[3];
		std::int32_t vehicleType;
		std::uint8_t canEnableDefenceMode;
		std::uint8_t buildOnBioPattern;
		std::byte padding5EA[2];
		char debugName[50];
		std::byte padding61E[2];
		NXBasics::SColorRGB debugColor;
	};

	class CStaticHouseTypeDataManager
	{
	public:
		CStaticHouseTypeDataManager();
		~CStaticHouseTypeDataManager();
		static std::int32_t GetHouseType(std::int32_t jobType, std::int32_t goodType);
		static std::int32_t GetHouseType_NoStock(std::int32_t goodType);
		CStaticHouseTypeDataManager(const CStaticHouseTypeDataManager&) = delete;
		CStaticHouseTypeDataManager& operator=(const CStaticHouseTypeDataManager&) = delete;

	private:
		std::uint8_t mObjectByte;
	};

	// Complete original static region: 005111B8..00511237.
	struct SStaticHouseTypeStorage
	{
		std::uint8_t tokenInitFlags;
		std::byte padding01[3];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		SStaticHouseTypeData* records;
		CStaticHouseTypeDataManager* owner;
		NXBasics::SToken tokens[14];
	};
	extern SStaticHouseTypeStorage gStaticHouseTypeStorage;

	static_assert(sizeof(SStaticHouseWorker) == 8);
	static_assert(offsetof(SStaticHouseWorker, maximumWorkers) == 4);
	static_assert(sizeof(SStaticHouseStock) == 12);
	static_assert(offsetof(SStaticHouseStock, maximumAmount) == 4);
	static_assert(offsetof(SStaticHouseStock, isOutputStock) == 8);
	static_assert(sizeof(SStaticHouseTypeData) == 1572);
	static_assert(offsetof(SStaticHouseTypeData, homeSize) == 4);
	static_assert(offsetof(SStaticHouseTypeData, schoolSize) == 8);
	static_assert(offsetof(SStaticHouseTypeData, workerCount) == 0x0C);
	static_assert(offsetof(SStaticHouseTypeData, workers) == 0x10);
	static_assert(offsetof(SStaticHouseTypeData, stockCount) == 0x60);
	static_assert(offsetof(SStaticHouseTypeData, stock) == 0x64);
	static_assert(offsetof(SStaticHouseTypeData, stockIndices) == 0x37C);
	static_assert(offsetof(SStaticHouseTypeData, productionCount) == 0x484);
	static_assert(offsetof(SStaticHouseTypeData, productionGoods) == 0x488);
	static_assert(offsetof(SStaticHouseTypeData, productionIndices) == 0x4D8);
	static_assert(offsetof(SStaticHouseTypeData, ignoreContinents) == 0x5E0);
	static_assert(offsetof(SStaticHouseTypeData, vehicleType) == 0x5E4);
	static_assert(offsetof(SStaticHouseTypeData, canEnableDefenceMode) == 0x5E8);
	static_assert(offsetof(SStaticHouseTypeData, buildOnBioPattern) == 0x5E9);
	static_assert(offsetof(SStaticHouseTypeData, debugName) == 0x5EC);
	static_assert(offsetof(SStaticHouseTypeData, debugColor) == 0x620);
	static_assert(sizeof(CStaticHouseTypeDataManager) == 1);
	static_assert(sizeof(SStaticHouseTypeStorage) == 128);
	static_assert(offsetof(SStaticHouseTypeStorage, tokenList) == 4);
	static_assert(offsetof(SStaticHouseTypeStorage, records) == 8);
	static_assert(offsetof(SStaticHouseTypeStorage, owner) == 0x0C);
	static_assert(offsetof(SStaticHouseTypeStorage, tokens) == 0x10);
}
