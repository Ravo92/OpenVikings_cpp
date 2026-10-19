#include "C2DataIds.h"
#include "C2Debug.h"
#include "C2LifetimeGlobals.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticTribeTypeData.h"
#include "GameProfile.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include <cstring>
#include <new>

namespace NC2Logic
{
	// Complete original writable command arrays, including all null terminators.
	static NXBasics::SToken gAnimalTypeTokens[19]{ }; // 00510D58..00510DEF.
	static std::uint8_t gAnimalTypeTokenFlags = 0; // 00510D48.
	static NXBasics::SToken gArmorTypeTokens[8]{ }; // 00510E00..00510E3F.
	static std::uint8_t gArmorTypeTokenFlags = 0; // 00510DF4.
	alignas(NXBasics::CTokenList) static std::byte gArmorTypeTokenList[4]{ }; // 00510DF0.
	static NXBasics::SToken gExperienceTypeTokens[7]{ }; // 00510A48..00510A7F.
	static std::uint8_t gExperienceTypeTokenFlags = 0; // 00510A34.
	alignas(NXBasics::CTokenList) static std::byte gExperienceTypeTokenList[4]{ }; // 00510A30.
	// Original functions: FUN_00412a41 and FUN_0041217a; empty token-list cleanup.
	static int __cdecl ArmorTokenList_Exit()
	{
		return 0;
	}
	static int __cdecl ExperienceTokenList_Exit()
	{
		return 0;
	}

	// Original function: FUN_00412299; CStaticAnimalTypeDataManager constructor.
	void* __fastcall StaticAnimalTypeDataManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticAnimalTypeDataManager = manager;
		std::memset(manager, 0, 1);
		gStaticAnimalTypeData = nullptr;
		gStaticAnimalTypeData = NXBasics::Runtime_Allocate(2176);
		std::memset(gStaticAnimalTypeData, 0, 2176);
		NXBasics::CIniFileReader reader("data\\logic\\animaltypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return manager;
		for (char* group = reader.GetFirstGroup("animaltype"); group != nullptr; group = reader.GetNextGroup("animaltype"))
		{
			if (!reader.SearchCommand("tribetype")) continue;
			const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
			const auto firstAnimalTribe = NRemaster::GetFirstAnimalTribe();
			if (tribe < firstAnimalTribe || tribe >= firstAnimalTribe + 34u || !Tool_AllowedItems_IsTribeTypeAllowed(tribe)) continue;
			auto* record = reinterpret_cast<SStaticAnimalTypeData*>(gStaticAnimalTypeData) + tribe - firstAnimalTribe;
			record->moveSpeed = 8;
			record->hitPointsAdult = 1000;
			record->hitPointsBaby = 500;
			reader.SetCurrentLineToGroupStart();
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gAnimalTypeTokenFlags & 1u) == 0)
				{
					gAnimalTypeTokenFlags |= 1u;
					static constexpr NXBasics::SToken tokens[19] = {
						{ "tribetype", NC2DataIds::AnimalTypeToken::TribeType },{ "aggressive", SwitchIds::AnimalTypeToken::Aggressive },{ "warrantable", SwitchIds::AnimalTypeToken::Warrantable },
						{ "maximumleaderdistance", SwitchIds::AnimalTypeToken::MaximumLeaderDistance },{ "searchforleader", SwitchIds::AnimalTypeToken::SearchForLeader },
						{ "maximumdistancetostaypoint", SwitchIds::AnimalTypeToken::MaximumDistanceToStayPoint },{ "maximumdistancetobirthpoint", SwitchIds::AnimalTypeToken::MaximumDistanceToBirthPoint },
						{ "maximumgroupsize", SwitchIds::AnimalTypeToken::MaximumGroupSize },{ "maximumcadaversize", SwitchIds::AnimalTypeToken::Maximumcadaversize },{ "catchable", SwitchIds::AnimalTypeToken::Catchable },
						{ "movespeed", SwitchIds::AnimalTypeToken::MoveSpeed },{ "hitpoints_baby", SwitchIds::AnimalTypeToken::HitpointsBABY },{ "hitpoints_adult", SwitchIds::AnimalTypeToken::HitpointsAdult },
						{ "getangry", NC2DataIds::AnimalTypeToken::GetAngry },{ "angryGameTime", SwitchIds::AnimalTypeToken::AngryGameTime },{ "runspeed", SwitchIds::AnimalTypeToken::Runspeed },
						{ "ignorehouses", NC2DataIds::AnimalTypeToken::IgnoreHouses },{ "cannotbeattacked", SwitchIds::AnimalTypeToken::Cannotbeattacked },{ nullptr, 0 }
					};
					std::memcpy(gAnimalTypeTokens, tokens, sizeof(tokens));
				}
				switch (NXBasics::XB_Token_FindTokenInArray(gAnimalTypeTokens, reader.GetCurrentName()))
				{
					case SwitchIds::AnimalTypeToken::Aggressive: record->aggressive = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::GetAngry: record->getAngry = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::AngryGameTime: record->angryGameTime = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::Warrantable: record->warrantable = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::MaximumLeaderDistance: record->maximumLeaderDistance = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::SearchForLeader: record->searchForLeader = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::MaximumDistanceToStayPoint: record->maximumDistanceToStayPoint = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::MaximumDistanceToBirthPoint: record->maximumDistanceToBirthPoint = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::MaximumGroupSize: record->maximumGroupSize = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::Maximumcadaversize: record->maximumCadaverSize = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::Catchable: record->catchable = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::MoveSpeed: record->moveSpeed = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::Runspeed: record->runSpeed = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::IgnoreHouses: record->ignoreHouses = reader.GetInt() != 0; break;
					case SwitchIds::AnimalTypeToken::HitpointsBABY: record->hitPointsBaby = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::HitpointsAdult: record->hitPointsAdult = reader.GetInt(); break;
					case SwitchIds::AnimalTypeToken::Cannotbeattacked: record->cannotBeAttacked = reader.GetInt() != 0; break;
				}
			}
		}
		return manager;
	}

	// Original function: FUN_0041277c; CStaticArmorTypeDataManager constructor.
	void* __fastcall StaticArmorTypeDataManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticArmorTypeDataManager = manager;
		std::memset(manager, 0, 1);
		gStaticArmorTypeData = nullptr;
		gStaticArmorTypeData = NXBasics::Runtime_Allocate(260);
		std::memset(gStaticArmorTypeData, 0, 260);
		std::strcpy(static_cast<char*>(gStaticArmorTypeData), "none");
		NXBasics::CIniFileReader reader("data\\logic\\armortypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return manager;
		for (char* group = reader.GetFirstGroup("armortype"); group != nullptr; group = reader.GetNextGroup("armortype"))
		{
			if (!reader.SearchCommand("type")) continue;
			const auto type = static_cast<std::uint32_t>(reader.GetInt());
			if (type == 0 || type >= 5) continue;
			auto* record = reinterpret_cast<SStaticArmorTypeData*>(gStaticArmorTypeData) + type;
			reader.SetCurrentLineToGroupStart();
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gArmorTypeTokenFlags & 1u) == 0)
				{
					gArmorTypeTokenFlags |= 1u;
					static constexpr NXBasics::SToken tokens[8] = { { "type", NC2DataIds::ArmorTypeToken::Type },{ "name", SwitchIds::ArmorTypeToken::Name },{ "maintype", SwitchIds::ArmorTypeToken::Maintype },{ "goodtype", SwitchIds::ArmorTypeToken::GoodType },{ "weight", SwitchIds::ArmorTypeToken::Weight },{ "blockingValue", SwitchIds::ArmorTypeToken::BlockingValue },{ "materialType", SwitchIds::ArmorTypeToken::MaterialType },{ nullptr, 0 } };
					std::memcpy(gArmorTypeTokens, tokens, sizeof(tokens));
				}
				if ((gArmorTypeTokenFlags & 2u) == 0)
				{
					gArmorTypeTokenFlags |= 2u;
					::new (gArmorTypeTokenList) NXBasics::CTokenList(gArmorTypeTokens);
					NXBasics::Runtime_AtExit(ArmorTokenList_Exit);
				}
				switch (reinterpret_cast<NXBasics::CTokenList*>(gArmorTypeTokenList)->FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::ArmorTypeToken::Name: std::strncpy(record->name, reader.GetString(), 29); break;
					case SwitchIds::ArmorTypeToken::Maintype: record->mainType = reader.GetInt(); break;
					case SwitchIds::ArmorTypeToken::GoodType: record->goodType = reader.GetInt(); break;
					case SwitchIds::ArmorTypeToken::MaterialType: record->materialType = reader.GetInt(); break;
					case SwitchIds::ArmorTypeToken::BlockingValue: record->blockingValue = reader.GetInt(); break;
					case SwitchIds::ArmorTypeToken::Weight: record->weight = reader.GetInt(); break;
				}
			}
		}
		return manager;
	}

	// Original function: FUN_00411e81; CStaticHumanJobExperienceTypeDataManager constructor.
	void* __fastcall StaticHumanJobExperienceTypeDataManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticHumanJobExperienceTypeDataManager = manager;
		gStaticHumanJobExperienceData = nullptr;
		gStaticHumanJobExperienceTable00510A3C = nullptr;
		gStaticHumanJobExperienceData = NXBasics::Runtime_Allocate(4056);
		std::memset(gStaticHumanJobExperienceData, 0, 4056);
		gStaticHumanJobExperienceTable00510A3C = NXBasics::Runtime_Allocate(2464);
		std::memset(gStaticHumanJobExperienceTable00510A3C, 0, 2464);
		std::strcpy(static_cast<char*>(gStaticHumanJobExperienceData), "none");
		NXBasics::CIniFileReader reader("data\\logic\\humanjobexperiencetypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return manager;
		for (char* group = reader.GetFirstGroup("humanjobexperiencetype"); group != nullptr; group = reader.GetNextGroup("humanjobexperiencetype"))
		{
			if (!reader.SearchCommand("type")) continue;
			const auto type = reader.GetInt();
			if (type <= 0 || type >= 78) continue;
			auto* record = reinterpret_cast<SStaticHumanJobExperienceTypeData*>(gStaticHumanJobExperienceData) + type;
			if (std::strlen(record->name) != 0) continue;
			record->job = 0;
			record->goods[0] = 0;
			record->goods[1] = 0;
			record->experienceFactor = 100;
			record->baseRepeatCounter = 10;
			reader.SetCurrentLineToGroupStart();
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gExperienceTypeTokenFlags & 1u) == 0)
				{
					gExperienceTypeTokenFlags |= 1u;
					static constexpr NXBasics::SToken tokens[7] = { { "type", NC2DataIds::HumanJobExperienceTypeToken::Type },{ "name", SwitchIds::ArmorTypeToken::Name },{ "job", SwitchIds::JobExperienceToken::Job },{ "good", SwitchIds::JobExperienceToken::Good },{ "experiencefactor", SwitchIds::JobExperienceToken::ExperienceFactor },{ "baserepeatcounter", SwitchIds::JobExperienceToken::Baserepeatcounter },{ nullptr, 0 } };
					std::memcpy(gExperienceTypeTokens, tokens, sizeof(tokens));
				}
				if ((gExperienceTypeTokenFlags & 2u) == 0)
				{
					gExperienceTypeTokenFlags |= 2u;
					::new (gExperienceTypeTokenList) NXBasics::CTokenList(gExperienceTypeTokens);
					NXBasics::Runtime_AtExit(ExperienceTokenList_Exit);
				}
				switch (reinterpret_cast<NXBasics::CTokenList*>(gExperienceTypeTokenList)->FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::JobExperienceToken::Name: std::strncpy(record->name, reader.GetString(), 29); break;
					case SwitchIds::JobExperienceToken::Good:
						while (reader.GetTypeOfNextParameter() == 3)
						{
							const auto good = reader.GetInt();
							if (record->goods[0] == 0) record->goods[0] = good;
							else if (record->goods[1] == 0) record->goods[1] = good;
							else break;
						}
						break;
					case SwitchIds::JobExperienceToken::ExperienceFactor: record->experienceFactor = reader.GetInt(); break;
					case SwitchIds::JobExperienceToken::Baserepeatcounter: record->baseRepeatCounter = reader.GetInt(); break;
					case SwitchIds::JobExperienceToken::Job:
						{
							record->job = reader.GetInt();
							auto* list = reinterpret_cast<SJobExperienceTypeList*>(static_cast<std::byte*>(gStaticHumanJobExperienceTable00510A3C) + static_cast<std::uint32_t>(record->job) * 0x2Cu);
							if (list->count != 10)
							{
								list->types[list->count] = type;
								++list->count;
							}
							break;
						}
				}
			}
		}
		return manager;
	}

	static NXBasics::SToken gWeaponTypeTokens[20]{ }; // 00510EF8..00510F97.
	static std::uint8_t gWeaponTypeTokenFlags = 0; // 00510E44.
	alignas(NXBasics::CTokenList) static std::byte gWeaponTypeTokenList[4]{ }; // 00510E40.
	static NXBasics::SToken gJobTypeTokens[14]{ }; // 00510FA8..00511017.
	static std::uint8_t gJobTypeTokenFlags = 0; // 00510F98.
	alignas(NXBasics::CTokenList) static std::byte gJobTypeTokenList[4]{ }; // 00510F9C.
	// Original functions: FUN_00412fdf and FUN_004134b5; empty token-list cleanup.
	static int __cdecl WeaponTokenList_Exit()
	{
		return 0;
	}
	static int __cdecl JobTokenList_Exit()
	{
		return 0;
	}

	// Original function: FUN_00412a5d; CStaticWeaponTypeDataManager constructor.
	void* __fastcall StaticWeaponTypeDataManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticWeaponTypeDataManager = manager;
		std::memset(manager, 0, 1);
		std::memset(gStaticWeaponTypeArrays, 0, sizeof(gStaticWeaponTypeArrays));
		for (void*& table : gStaticWeaponTypeArrays)
		{
			table = NXBasics::Runtime_Allocate(4312);
			std::memset(table, 0, 4312);
			std::strcpy(static_cast<char*>(table), "none");
		}
		NXBasics::CIniFileReader reader("data\\logic\\weapontypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return manager;
		for (char* group = reader.GetFirstGroup("weapontype"); group != nullptr; group = reader.GetNextGroup("weapontype"))
		{
			if (!reader.SearchCommand("tribetype")) continue;
			const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
			if (tribe == 0 || tribe >= 42) continue;
			reader.SetCurrentLineToGroupStart();
			if (!reader.SearchCommand("type")) continue;
			const auto type = static_cast<std::uint32_t>(reader.GetInt());
			if (type >= 22) continue;
			auto* record = static_cast<SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe]) + type;
			reader.SetCurrentLineToGroupStart();
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gWeaponTypeTokenFlags & 1u) == 0)
				{
					gWeaponTypeTokenFlags |= 1u;
					static constexpr NXBasics::SToken tokens[20] = {
						{ "type", NC2DataIds::WeaponTypeToken::Type },{ "tribetype", NC2DataIds::WeaponTypeToken::TribeType },{ "name", SwitchIds::ArmorTypeToken::Name },{ "maintype", SwitchIds::ArmorTypeToken::Maintype },
						{ "goodtype", SwitchIds::ArmorTypeToken::GoodType },{ "weight", SwitchIds::ArmorTypeToken::Weight },{ "minimumrange", SwitchIds::WeaponTypeToken::MinimumRange },{ "maximumrange", SwitchIds::WeaponTypeToken::MaximumRange },
						{ "munitiontype", SwitchIds::WeaponTypeToken::MunitionType },{ "damagetype", SwitchIds::WeaponTypeToken::DamageType },{ "damagevalue", SwitchIds::WeaponTypeToken::Damagevalue },{ "jobtype", SwitchIds::WeaponTypeToken::JobType },
						{ "atomicactiontype", SwitchIds::WeaponTypeToken::AtomicActionType },{ "speed", SwitchIds::WeaponTypeToken::Speed },{ "soundtype_hit", SwitchIds::WeaponTypeToken::SoundtypeHit },{ "soundtype_nohit", SwitchIds::WeaponTypeToken::SoundtypeNohit },
						{ "hitself", SwitchIds::WeaponTypeToken::Hitself },{ "createsmoke", SwitchIds::WeaponTypeToken::Createsmoke },{ "smokelifetime", SwitchIds::WeaponTypeToken::Smokelifetime },{ nullptr, 0 }
					};
					std::memcpy(gWeaponTypeTokens, tokens, sizeof(tokens));
				}
				if ((gWeaponTypeTokenFlags & 2u) == 0)
				{
					gWeaponTypeTokenFlags |= 2u;
					::new (gWeaponTypeTokenList) NXBasics::CTokenList(gWeaponTypeTokens);
					NXBasics::Runtime_AtExit(WeaponTokenList_Exit);
				}
				const auto token = reinterpret_cast<NXBasics::CTokenList*>(gWeaponTypeTokenList)->FindToken(reader.GetCurrentName());
				switch (token)
				{
					case SwitchIds::WeaponTypeToken::Name: std::strncpy(record->name, reader.GetString(), 29); break;
					case SwitchIds::WeaponTypeToken::Maintype: record->mainType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::GoodType: record->goodType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::Weight: record->weight = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::MinimumRange: record->minimumRange = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::MaximumRange: record->maximumRange = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::JobType: record->jobType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::MunitionType: record->hasMunition = 1; record->munitionType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::DamageType: record->damageType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::AtomicActionType: record->atomicActionType = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::Speed: record->speed = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::Hitself: record->hitSelf = reader.GetInt() != 0; break;
					case SwitchIds::WeaponTypeToken::Createsmoke: record->createsSmoke = reader.GetInt() != 0; break;
					case SwitchIds::WeaponTypeToken::Smokelifetime: record->smokeLifetime = reader.GetInt(); break;
					case SwitchIds::WeaponTypeToken::Damagevalue:
					case SwitchIds::WeaponTypeToken::SoundtypeHit:
					case SwitchIds::WeaponTypeToken::SoundtypeNohit:
						{
							const auto index = static_cast<std::uint32_t>(reader.GetInt());
							const auto value = reader.GetInt();
							const auto offset = (token == SwitchIds::WeaponTypeToken::Damagevalue ? 0x38u : token == SwitchIds::WeaponTypeToken::SoundtypeHit ? 0x64u : 0x84u) + index * 4u;
							std::memcpy(reinterpret_cast<std::byte*>(record) + offset, &value, sizeof(value));
							break;
						}
				}
			}
		}
		return manager;
	}

	// Original function: FUN_0041300a; CStaticJobTypeDataManager constructor.
	void* __fastcall StaticJobTypeDataManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticJobTypeDataManager = manager;
		std::memset(manager, 0, 1);
		gStaticJobTypeData = nullptr;
		gStaticJobTypeData = NXBasics::Runtime_Allocate(7616);
		std::memset(gStaticJobTypeData, 0, 7616);
		std::strcpy(static_cast<char*>(gStaticJobTypeData), "none");
		{
			NXBasics::CIniFileReader reader("data\\logic\\jobtypes.ini", nullptr, false, nullptr, nullptr);
			if (!reader.IsLoaded()) return manager;
			for (char* group = reader.GetFirstGroup("jobtype"); group != nullptr; group = reader.GetNextGroup("jobtype"))
			{
				if (!reader.SearchCommand("type")) continue;
				const auto type = reader.GetInt();
				if (type <= 0 || type >= 56) continue;
				auto* record = static_cast<SStaticJobTypeData*>(gStaticJobTypeData) + type;
				if (std::strlen(record->name) != 0) continue;
				record->baseAtomics = 0;
				reader.SetCurrentLineToGroupStart();
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gJobTypeTokenFlags & 1u) == 0)
					{
						gJobTypeTokenFlags |= 1u;
						static constexpr NXBasics::SToken tokens[14] = {
							{ "type", NC2DataIds::JobTypeToken::Type },{ "Name", SwitchIds::ArmorTypeToken::Name },{ "Allowatomic", SwitchIds::JobTypeToken::Allowatomic },{ "Forbidatomic", SwitchIds::JobTypeToken::Forbidatomic },
							{ "Baseatomics", SwitchIds::JobTypeToken::Baseatomics },{ "needsReligionFlag", SwitchIds::JobTypeToken::NeedsReligionFlag },{ "ignoresHomeHouseFlag", SwitchIds::JobTypeToken::IgnoresHomeHouseFlag },
							{ "CanHaveWorkHouseFlag", SwitchIds::JobTypeToken::CanHaveWorkHouseFlag },{ "MustHaveWorkHouseFlag", SwitchIds::JobTypeToken::MustHaveWorkHouseFlag },{ "MustHaveFinishedWorkHouseFlag", SwitchIds::JobTypeToken::MustHaveFinishedWorkHouseFlag },
							{ "UserCanChangeProductionFlag", SwitchIds::JobTypeToken::UserCanChangeProductionFlag },{ "UserShouldAttachWorkPlaceAfterJobChangeFlag", SwitchIds::JobTypeToken::UserShouldAttachWorkPlaceAfterJobChangeFlag },{ "CanBeTrainedFlag", SwitchIds::JobTypeToken::CanBeTrainedFlag },{ nullptr, 0 }
						};
						std::memcpy(gJobTypeTokens, tokens, sizeof(tokens));
					}
					if ((gJobTypeTokenFlags & 2u) == 0)
					{
						gJobTypeTokenFlags |= 2u;
						::new (gJobTypeTokenList) NXBasics::CTokenList(gJobTypeTokens);
						NXBasics::Runtime_AtExit(JobTokenList_Exit);
					}
					const auto token = reinterpret_cast<NXBasics::CTokenList*>(gJobTypeTokenList)->FindToken(reader.GetCurrentName());
					switch (token)
					{
						case SwitchIds::JobTypeToken::Name: std::strncpy(record->name, reader.GetString(), 29); break;
						case SwitchIds::JobTypeToken::Allowatomic:
						case SwitchIds::JobTypeToken::Forbidatomic:
							{
								const auto offset = 0x24u + static_cast<std::uint32_t>(reader.GetInt());
								reinterpret_cast<std::uint8_t*>(record)[offset] = token == SwitchIds::JobTypeToken::Allowatomic;
								break;
							}
						case SwitchIds::JobTypeToken::Baseatomics:
							{
								const auto base = reader.GetInt();
								if (base > 0 && base < 56 && record->baseAtomics == 0)
								{
									record->baseAtomics = base;
									const auto* source = static_cast<SStaticJobTypeData*>(gStaticJobTypeData) + base;
									std::memcpy(record->allowedAtomics, source->allowedAtomics, 92);
								}
								break;
							}
						case SwitchIds::JobTypeToken::NeedsReligionFlag: record->needsReligion = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::IgnoresHomeHouseFlag: record->ignoresHomeHouse = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::CanHaveWorkHouseFlag: record->canHaveWorkHouse = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::MustHaveWorkHouseFlag: record->mustHaveWorkHouse = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::MustHaveFinishedWorkHouseFlag: record->mustHaveFinishedWorkHouse = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::UserCanChangeProductionFlag: record->userCanChangeProduction = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::UserShouldAttachWorkPlaceAfterJobChangeFlag: record->userShouldAttachWorkPlaceAfterJobChange = reader.GetInt() != 0; break;
						case SwitchIds::JobTypeToken::CanBeTrainedFlag: record->canBeTrained = reader.GetInt() != 0; break;
					}
				}
			}
		}
		for (int type = 0; type < 56; ++type)
		{
			const auto* record = static_cast<SStaticJobTypeData*>(gStaticJobTypeData) + type;
			if (std::strlen(record->name) == 0) C2Debug_WarningBoxV("CStaticJobTypeDataManager::CStaticJobTypeDataManager()\nError while parsing jobtypes.ini\n\nMissing definition for Job #%d!\n(numeric value in TJobTypes enum)", type);
		}
		return manager;
	}
}
