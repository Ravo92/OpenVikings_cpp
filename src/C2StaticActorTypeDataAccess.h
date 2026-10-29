#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SStaticAnimalTypeData
	{
		std::uint8_t aggressive;
		std::uint8_t getAngry;
		std::byte padding02[2];
		std::int32_t angryGameTime;
		std::uint8_t warrantable;
		std::byte padding09[3];
		std::int32_t maximumLeaderDistance;
		std::uint8_t searchForLeader;
		std::byte padding11[3];
		std::int32_t maximumDistanceToStayPoint;
		std::int32_t maximumDistanceToBirthPoint;
		std::int32_t maximumGroupSize;
		std::int32_t maximumCadaverSize;
		std::uint8_t catchable;
		std::byte padding25[3];
		std::int32_t moveSpeed;
		std::int32_t runSpeed;
		std::uint8_t ignoreHouses;
		std::byte padding31[3];
		std::int32_t hitPointsBaby;
		std::int32_t hitPointsAdult;
		std::uint8_t cannotBeAttacked;
		std::byte padding3D[3];
	};

	struct SStaticArmorTypeData
	{
		char name[30];
		std::byte padding1E[2];
		std::int32_t mainType;
		std::int32_t goodType;
		std::int32_t materialType;
		std::int32_t blockingValue;
		std::int32_t weight;
	};

	struct SStaticHumanJobExperienceTypeData
	{
		char name[30];
		std::byte padding1E[2];
		std::int32_t job;
		std::int32_t goods[2];
		std::int32_t experienceFactor;
		std::int32_t baseRepeatCounter;
	};

	struct SStaticWeaponTypeData
	{
		char name[30];
		std::byte padding1E[2];
		std::int32_t mainType, goodType, weight, minimumRange, maximumRange, jobType;
		std::int32_t damageValues[8];
		std::uint8_t hasMunition;
		std::byte padding59[3];
		std::int32_t munitionType, atomicActionType;
		std::int32_t soundTypesHit[8], soundTypesNoHit[11];
		std::int32_t damageType, speed;
		std::uint8_t createsSmoke;
		std::byte paddingB9[3];
		std::int32_t smokeLifetime;
		std::uint8_t hitSelf;
		std::byte paddingC1[3];
	};

	struct SStaticJobTypeData
	{
		char name[30];
		std::byte padding1E[2];
		std::int32_t baseAtomics;
		std::uint8_t allowedAtomics[92];
		std::uint8_t needsReligion, ignoresHomeHouse, canHaveWorkHouse, mustHaveWorkHouse;
		std::uint8_t mustHaveFinishedWorkHouse, userCanChangeProduction, userShouldAttachWorkPlaceAfterJobChange, canBeTrained;
	};

	struct SJobExperienceTypeList
	{
		std::uint32_t count;
		std::int32_t types[10];
	};

	static_assert(sizeof(SStaticAnimalTypeData) == 64);
	static_assert(offsetof(SStaticAnimalTypeData, hitPointsBaby) == 0x34);
	static_assert(sizeof(SStaticArmorTypeData) == 52);
	static_assert(sizeof(SStaticHumanJobExperienceTypeData) == 52);
	static_assert(sizeof(SJobExperienceTypeList) == 44);
	static_assert(sizeof(SStaticWeaponTypeData) == 196);
	static_assert(offsetof(SStaticWeaponTypeData, damageType) == 0xB0);
	static_assert(sizeof(SStaticJobTypeData) == 136);
	static_assert(offsetof(SStaticJobTypeData, needsReligion) == 0x80);
	void* __fastcall StaticAnimalTypeDataManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall StaticArmorTypeDataManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall StaticHumanJobExperienceTypeDataManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall StaticWeaponTypeDataManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall StaticJobTypeDataManager_Construct(void* manager, void* unusedEdx);
}