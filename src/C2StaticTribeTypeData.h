#pragma once

#include "XBTokenList.h"

namespace NC2Logic
{
	struct STribeExperienceRequirement
	{
		std::uint32_t requiredExperience;
		std::uint32_t jobCount;
		std::int32_t jobs[3];
	};

	struct STribeJobEnables
	{
		std::uint32_t jobCount;
		std::uint32_t goodCount;
		std::uint32_t houseCount;
		std::uint32_t vehicleCount;
		std::int32_t jobs[50];
		std::int32_t goods[50];
		std::int32_t houses[50];
		std::int32_t vehicles[50];
		std::int32_t defaultGood;
		std::uint32_t equipmentCount;
		std::int32_t equipment[50];
	};

	struct STribeGoodProduction
	{
		std::int32_t job;
		std::int32_t house;
	};

	struct STribeBuildRequirement
	{
		std::uint32_t jobCount;
		std::uint32_t goodCount;
		std::int32_t jobs[50];
		std::int32_t goods[50];
	};

	struct SStaticTribeTypeData
	{
		char name[30];
		std::byte unknown1E[2];
		std::uint32_t atomicIds[56][92];
		std::uint8_t allowedJobs[56];
		std::uint8_t allowedGoods[66];
		std::uint8_t allowedEquipment[66];
		std::uint8_t allowedHouses[55];
		std::uint8_t allowedVehicles[7];
		std::byte padding519A[2];
		STribeExperienceRequirement jobNeeds[56];
		STribeExperienceRequirement goodNeeds[66];
		STribeExperienceRequirement houseNeeds[55];
		STribeExperienceRequirement vehicleNeeds[7];
		STribeExperienceRequirement equipmentNeeds[66];
		STribeExperienceRequirement armorNeeds[5];
		STribeExperienceRequirement weaponNeeds[22];
		std::uint32_t jobTraining[56];
		std::uint32_t goodTraining[66];
		STribeJobEnables jobEnables[56];
		STribeGoodProduction goodProduction[66];
		STribeBuildRequirement houseBuildNeeds[55];
		STribeBuildRequirement vehicleBuildNeeds[7];
	};

	class CStaticTribeTypeDataManager
	{
	public:
		CStaticTribeTypeDataManager();
		~CStaticTribeTypeDataManager();
		CStaticTribeTypeDataManager(const CStaticTribeTypeDataManager&) = delete;
		CStaticTribeTypeDataManager& operator=(const CStaticTribeTypeDataManager&) = delete;

	private:
		std::uint8_t mObjectByte;
	};

	bool Tool_AllowedItems_IsTribeTypeAllowed(std::uint32_t tribeType);

	// Complete original writable region: 00511018..0051110F.
	struct SStaticTribeTypeStorage
	{
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		std::uint8_t tokenInitFlags;
		std::byte padding05[sizeof(void*) - 1];
		SStaticTribeTypeData* records;
		CStaticTribeTypeDataManager* owner;
		NXBasics::SToken tokens[29];
	};
	extern SStaticTribeTypeStorage gStaticTribeTypeStorage;

	static_assert(sizeof(STribeExperienceRequirement) == 20);
	static_assert(offsetof(STribeExperienceRequirement, jobCount) == 4);
	static_assert(offsetof(STribeExperienceRequirement, jobs) == 8);
	static_assert(sizeof(STribeJobEnables) == 1024);
	static_assert(offsetof(STribeJobEnables, jobs) == 0x10);
	static_assert(offsetof(STribeJobEnables, goods) == 0xD8);
	static_assert(offsetof(STribeJobEnables, houses) == 0x1A0);
	static_assert(offsetof(STribeJobEnables, vehicles) == 0x268);
	static_assert(offsetof(STribeJobEnables, defaultGood) == 0x330);
	static_assert(offsetof(STribeJobEnables, equipmentCount) == 0x334);
	static_assert(offsetof(STribeJobEnables, equipment) == 0x338);
	static_assert(sizeof(STribeGoodProduction) == 8);
	static_assert(sizeof(STribeBuildRequirement) == 408);
	static_assert(offsetof(STribeBuildRequirement, jobs) == 8);
	static_assert(offsetof(STribeBuildRequirement, goods) == 0xD0);
	static_assert(sizeof(SStaticTribeTypeData) == 110088);
	static_assert(offsetof(SStaticTribeTypeData, atomicIds) == 0x20);
	static_assert(sizeof(SStaticTribeTypeData::atomicIds) == 20608);
	static_assert(offsetof(SStaticTribeTypeData, allowedJobs) == 0x50A0);
	static_assert(offsetof(SStaticTribeTypeData, allowedGoods) == 0x50D8);
	static_assert(offsetof(SStaticTribeTypeData, allowedEquipment) == 0x511A);
	static_assert(offsetof(SStaticTribeTypeData, allowedHouses) == 0x515C);
	static_assert(offsetof(SStaticTribeTypeData, allowedVehicles) == 0x5193);
	static_assert(offsetof(SStaticTribeTypeData, jobNeeds) == 0x519C);
	static_assert(offsetof(SStaticTribeTypeData, goodNeeds) == 0x55FC);
	static_assert(offsetof(SStaticTribeTypeData, houseNeeds) == 0x5B24);
	static_assert(offsetof(SStaticTribeTypeData, vehicleNeeds) == 0x5F70);
	static_assert(offsetof(SStaticTribeTypeData, equipmentNeeds) == 0x5FFC);
	static_assert(offsetof(SStaticTribeTypeData, armorNeeds) == 0x6524);
	static_assert(offsetof(SStaticTribeTypeData, weaponNeeds) == 0x6588);
	static_assert(offsetof(SStaticTribeTypeData, jobTraining) == 0x6740);
	static_assert(offsetof(SStaticTribeTypeData, goodTraining) == 0x6820);
	static_assert(offsetof(SStaticTribeTypeData, jobEnables) == 0x6928);
	static_assert(offsetof(SStaticTribeTypeData, goodProduction) == 0x14928);
	static_assert(offsetof(SStaticTribeTypeData, houseBuildNeeds) == 0x14B38);
	static_assert(offsetof(SStaticTribeTypeData, vehicleBuildNeeds) == 0x1A2E0);
	static_assert(sizeof(SStaticTribeTypeStorage) == (sizeof(void*) == 8 ? 496 : 248));
	static_assert(offsetof(SStaticTribeTypeStorage, tokenInitFlags) == sizeof(void*));
	static_assert(offsetof(SStaticTribeTypeStorage, records) == 2 * sizeof(void*));
	static_assert(offsetof(SStaticTribeTypeStorage, owner) == 3 * sizeof(void*));
	static_assert(offsetof(SStaticTribeTypeStorage, tokens) == 4 * sizeof(void*));
	static_assert(sizeof(CStaticTribeTypeDataManager) == 1);
	const char* StaticTribeTypeData_GetNamePtr(std::int32_t tribe);
}
