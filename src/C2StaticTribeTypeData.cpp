#include "C2DataIds.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticTribeTypeData.h"
#include "GameProfile.h"
#include "SwitchIdentifiers.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticTribeTypeStorage gStaticTribeTypeStorage{ };
	SStaticTribeTypeData*& gStaticTribeTypeData = gStaticTribeTypeStorage.records;
	CStaticTribeTypeDataManager*& gStaticTribeTypeDataManager = gStaticTribeTypeStorage.owner;

	// Original function: FUN_0044CF00; the upper-bound check is signed, negative ids remain unchecked.
	const char* StaticTribeTypeData_GetNamePtr(std::int32_t tribe)
	{
		return tribe > 42 ? "<error>" : gStaticTribeTypeData[tribe].name;
	}

	namespace
	{
		// Original function: FUN_00414172, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl TribeTokens_OnExit()
		{
			return 0;
		}

		NXBasics::CTokenList& TribeTokens()
		{
			auto& storage = gStaticTribeTypeStorage;
			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				storage.tokens[0] = { "type", NC2DataIds::TribeToken::Type };
				storage.tokens[1] = { "name", SwitchIds::TribeTypeToken::Name };
				storage.tokens[2] = { "setatomic", SwitchIds::TribeTypeToken::SetAtomic };
				storage.tokens[3] = { "allowjob", SwitchIds::TribeTypeToken::AllowJob };
				storage.tokens[4] = { "allowgood", SwitchIds::TribeTypeToken::AllowGood };
				storage.tokens[5] = { "allowequip", SwitchIds::TribeTypeToken::AllowEquip };
				storage.tokens[6] = { "allowhouse", SwitchIds::TribeTypeToken::AllowHouse };
				storage.tokens[7] = { "allowvehicle", SwitchIds::TribeTypeToken::AllowVehicle };
				storage.tokens[8] = { "needforjob", SwitchIds::TribeTypeToken::NeedForJob };
				storage.tokens[9] = { "needforgood", SwitchIds::TribeTypeToken::NeedForGood };
				storage.tokens[10] = { "needforhouse", SwitchIds::TribeTypeToken::NeedForHouse };
				storage.tokens[11] = { "needforvehicle", SwitchIds::TribeTypeToken::NeedForVehicle };
				storage.tokens[12] = { "needforequipment", SwitchIds::TribeTypeToken::NeedForEquipment };
				storage.tokens[13] = { "needforarmor", SwitchIds::TribeTypeToken::NeedForArmor };
				storage.tokens[14] = { "needforweapon", SwitchIds::TribeTypeToken::NeedForWeapon };
				storage.tokens[15] = { "trainforjob", SwitchIds::TribeTypeToken::TrainForJob };
				storage.tokens[16] = { "trainforgood", SwitchIds::TribeTypeToken::TrainForGood };
				storage.tokens[17] = { "jobEnablesJob", SwitchIds::TribeTypeToken::JobEnablesJob };
				storage.tokens[18] = { "jobEnablesGood", SwitchIds::TribeTypeToken::JobEnablesGood };
				storage.tokens[19] = { "jobEnablesHouse", SwitchIds::TribeTypeToken::JobEnablesHouse };
				storage.tokens[20] = { "jobEnablesVehicle", SwitchIds::TribeTypeToken::JobEnablesVehicle };
				storage.tokens[21] = { "jobEnablesEquipment", SwitchIds::TribeTypeToken::JobEnablesEquipment };
				storage.tokens[22] = { "jobDefaultGood", SwitchIds::TribeTypeToken::JobDefaultGood };
				storage.tokens[23] = { "toBuildHouseNeedJob", SwitchIds::TribeTypeToken::ToBuildHouseNeedJob };
				storage.tokens[24] = { "toBuildHouseNeedGood", SwitchIds::TribeTypeToken::ToBuildHouseNeedGood };
				storage.tokens[25] = { "toBuildVehicleNeedJob", SwitchIds::TribeTypeToken::ToBuildVehicleNeedJob };
				storage.tokens[26] = { "toBuildVehicleNeedGood", SwitchIds::TribeTypeToken::ToBuildVehicleNeedGood };
				storage.tokens[27] = { "toProduceGoodNeedHouse", SwitchIds::TribeTypeToken::ToProduceGoodNeedHouse };
				storage.tokens[28] = { nullptr, 0 };
			}
			if ((storage.tokenInitFlags & 2) == 0)
			{
				storage.tokenInitFlags |= 2;
				::new (storage.tokenList) NXBasics::CTokenList(storage.tokens);
				NXBasics::Runtime_AtExit(TribeTokens_OnExit);
			}
			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}
	}

	// Original function: FUN_004134D1.
	CStaticTribeTypeDataManager::CStaticTribeTypeDataManager()
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;
		gStaticTribeTypeDataManager = this;
		std::memset(this, 0, 1);
		gStaticTribeTypeData = nullptr;
		gStaticTribeTypeData = static_cast<SStaticTribeTypeData*>(NXBasics::Runtime_AllocateWithNewHandler(4623696, 1));
		std::memset(gStaticTribeTypeData, 0, 4623696);
		std::strcpy(gStaticTribeTypeData[0].name, "none");
		NXBasics::CIniFileReader reader("data\\logic\\tribetypes\\tribetypes.ini", nullptr, false, nullptr, nullptr);
		if (!reader.IsLoaded()) return;
		for (char* group = reader.GetFirstGroup("tribetype"); group != nullptr; group = reader.GetNextGroup("tribetype"))
		{
			if (!reader.SearchCommand("type")) continue;
			const auto tribe = static_cast<std::uint32_t>(reader.GetInt());
			if (tribe == 0 || tribe >= 42 || !Tool_AllowedItems_IsTribeTypeAllowed(tribe)) continue;
			auto& record = gStaticTribeTypeData[tribe];
			std::memset(record.atomicIds, NXBasics::IntegerRange::UInt8MaximumSigned, sizeof(record.atomicIds));
			reader.SetCurrentLineToGroupStart();

			// Remaster validates selectors before accessing the native record layout.
			const auto readExperienceRequirement = [&] (std::uint32_t base, std::uint32_t limit)
			{
				const auto id = static_cast<std::uint32_t>(reader.GetInt());
				if (id >= limit) return;
				const auto entry = base + id * 0x14u;
				Write<std::int32_t>(&record, entry, reader.GetInt());
				while (reader.GetTypeOfNextParameter() == 3)
				{
					const auto job = reader.GetInt();
					const auto count = Read<std::uint32_t>(&record, entry + NC2Layout::TribeExperienceRequirementOffset::JobCount);
					// These are experience-type IDs (0..77), not human job IDs (0..55).
					if (job < 0 || job >= 78 || count >= 3) continue;
					Write<std::int32_t>(&record, entry + NC2Layout::TribeExperienceRequirementOffset::Jobs + count * NC2Layout::RecordSize::TribeEnableItemUnsigned, job);
					Write<std::uint32_t>(&record, entry + NC2Layout::TribeExperienceRequirementOffset::JobCount, Read<std::uint32_t>(&record, entry + NC2Layout::TribeExperienceRequirementOffset::JobCount) + 1);
				}
			};

			const auto readBuildRequirement = [&] (std::uint32_t base, std::int32_t limit, std::uint32_t countOffset, std::uint32_t itemsOffset, std::int32_t itemLimit)
			{
				const auto id = reader.GetInt();
				if (id < 0 || id >= limit) return;
				const auto entry = base + static_cast<std::uint32_t>(id) * 0x198u;
				while (reader.GetTypeOfNextParameter() == 3)
				{
					const auto value = reader.GetInt();
					if (value < 0 || value >= itemLimit) return;
					const auto count = Read<std::uint32_t>(&record, entry + countOffset);
					if (count >= 50) return;
					Write<std::int32_t>(&record, entry + itemsOffset + count * NC2Layout::RecordSize::TribeEnableItemUnsigned, value);
					Write<std::uint32_t>(&record, entry + countOffset, count + 1);
				}
			};

			const auto readJobEnables = [&] (std::uint32_t countOffset, std::uint32_t itemsOffset, std::int32_t itemLimit, bool inclusiveLimit, bool setProducingJob)
			{
				const auto job = reader.GetInt();
				const auto value = reader.GetInt();
				if (job < 0 || job >= 56 || value < 0 || (inclusiveLimit ? value > itemLimit : value >= itemLimit)) return;
				const auto entry = 0x6928u + static_cast<std::uint32_t>(job) * 0x400u;
				const auto count = Read<std::uint32_t>(&record, entry + countOffset);
				if (count >= 50) return;
				Write<std::int32_t>(&record, entry + itemsOffset + count * NC2Layout::RecordSize::TribeEnableItemUnsigned, value);
				Write<std::uint32_t>(&record, entry + countOffset, count + 1);
				if (setProducingJob) Write<std::int32_t>(&record, NC2Layout::StaticTribeOffset::GoodProductionUnsigned + static_cast<std::uint32_t>(value) * NC2Layout::RecordSize::TribeGoodProductionUnsigned, job);
			};

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (TribeTokens().FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::TribeTypeToken::Name: std::strncpy(record.name, reader.GetString(), 29); break;
					case SwitchIds::TribeTypeToken::AllowJob:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt()); if (id < 56) record.allowedJobs[id] = 1; break;
						}
					case SwitchIds::TribeTypeToken::AllowGood:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt()); if (id < 66) record.allowedGoods[id] = 1; break;
						}
					case SwitchIds::TribeTypeToken::AllowEquip:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt()); if (id < 66) record.allowedEquipment[id] = 1; break;
						}
					case SwitchIds::TribeTypeToken::AllowHouse:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt()); if (id < 55) record.allowedHouses[id] = 1; break;
						}
					case SwitchIds::TribeTypeToken::AllowVehicle:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt()); if (id < 7) record.allowedVehicles[id] = 1; break;
						}
					case SwitchIds::TribeTypeToken::NeedForJob: readExperienceRequirement(0x519C, 56); break;
					case SwitchIds::TribeTypeToken::NeedForGood: readExperienceRequirement(0x55FC, 66); break;
					case SwitchIds::TribeTypeToken::NeedForHouse: readExperienceRequirement(0x5B24, 55); break;
					case SwitchIds::TribeTypeToken::NeedForVehicle: readExperienceRequirement(0x5F70, 7); break;
					case SwitchIds::TribeTypeToken::NeedForEquipment: readExperienceRequirement(0x5FFC, 66); break;
					case SwitchIds::TribeTypeToken::NeedForArmor: readExperienceRequirement(0x6524, 5); break;
					case SwitchIds::TribeTypeToken::NeedForWeapon: readExperienceRequirement(0x6588, 22); break;
					case SwitchIds::TribeTypeToken::ToBuildHouseNeedJob: readBuildRequirement(0x14B38, 55, 0, 8, 56); break;
					case SwitchIds::TribeTypeToken::ToBuildHouseNeedGood: readBuildRequirement(0x14B38, 55, 4, 0xD0, 66); break;
					case SwitchIds::TribeTypeToken::ToBuildVehicleNeedJob: readBuildRequirement(0x1A2E0, 7, 0, 8, 56); break;
					case SwitchIds::TribeTypeToken::ToBuildVehicleNeedGood: readBuildRequirement(0x1A2E0, 7, 4, 0xD0, 66); break;
					case SwitchIds::TribeTypeToken::JobEnablesJob: readJobEnables(0, 0x10, 56, false, false); break;
					case SwitchIds::TribeTypeToken::JobEnablesGood: readJobEnables(4, 0xD8, 66, false, true); break;
					case SwitchIds::TribeTypeToken::JobEnablesHouse: readJobEnables(8, 0x1A0, 55, false, false); break;
					case SwitchIds::TribeTypeToken::JobEnablesVehicle: readJobEnables(0x0C, 0x268, 7, false, false); break;
					case SwitchIds::TribeTypeToken::JobEnablesEquipment: readJobEnables(0x334, 0x338, 55, true, false); break;
					case SwitchIds::TribeTypeToken::TrainForJob:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt());
							if (id < 56) record.jobTraining[id] = static_cast<std::uint32_t>(reader.GetInt());
							break;
						}
					case SwitchIds::TribeTypeToken::TrainForGood:
						{
							const auto id = static_cast<std::uint32_t>(reader.GetInt());
							if (id < 66) record.goodTraining[id] = static_cast<std::uint32_t>(reader.GetInt());
							break;
						}
					case SwitchIds::TribeTypeToken::JobDefaultGood:
						{
							const auto job = reader.GetInt();
							const auto good = reader.GetInt();
							if (job < 0 || job >= 56 || good < 0 || good >= 66) break;
							Write<std::int32_t>(&record, NC2Layout::StaticTribeOffset::FirstJobDefaultGoodUnsigned + static_cast<std::uint32_t>(job) * NC2Layout::RecordSize::TribeJobEnablesUnsigned, good);
							Write<std::int32_t>(&record, NC2Layout::StaticTribeOffset::GoodProductionUnsigned + static_cast<std::uint32_t>(good) * NC2Layout::RecordSize::TribeGoodProductionUnsigned, job);
							break;
						}
					case SwitchIds::TribeTypeToken::ToProduceGoodNeedHouse:
						{
							const auto good = reader.GetInt();
							const auto house = reader.GetInt();
							if (good >= 0 && good < 66 && house >= 0 && house < 55) Write<std::int32_t>(&record, NC2Layout::StaticTribeOffset::FirstGoodProductionHouseUnsigned + static_cast<std::uint32_t>(good) * NC2Layout::RecordSize::TribeGoodProductionUnsigned, house);
							break;
						}
					case SwitchIds::TribeTypeToken::SetAtomic:
						{
							const auto job = static_cast<std::uint32_t>(reader.GetInt());
							const auto state = static_cast<std::uint32_t>(reader.GetInt());
							if (job >= 56 || state >= 92) break;
							const auto atomic = CStaticAtomicAnimationDataManager::GetAtomicId(reader.GetString());
							if (atomic == NXBasics::IntegerRange::UInt32Maximum) break;
							const auto offset = 0x20u + (job * 92u + state) * 4u;
							if (Read<std::uint32_t>(&record, offset) == NXBasics::IntegerRange::UInt32Maximum) Write<std::uint32_t>(&record, offset, atomic);
							break;
						}
				}
			}
		}
	}

	// Original function: FUN_00414173; moved from the lifetime module.
	CStaticTribeTypeDataManager::~CStaticTribeTypeDataManager()
	{
		NXBasics::Runtime_Free(gStaticTribeTypeData);
		gStaticTribeTypeData = nullptr;
		gStaticTribeTypeDataManager = nullptr;
	}

	// Original function: FUN_0042DB8E. Decoded from both Windows jump-table targets.
	bool Tool_AllowedItems_IsTribeTypeAllowed(std::uint32_t tribeType)
	{
		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2)
		{
			// Cultures2.exe 004279E7, complete dispatch bytes 00427A0E..00427A36.
			static constexpr bool allowedCultures2[41] = {
				true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, true,
				true, true, true, true, true, false, false, false, true, true, false, false, false, true,
				false, false, false, true, true, false, false, false, false, false, false
			};
			return tribeType < 41 && allowedCultures2[tribeType];
		}
		// Original dispatch regions: 0042DBAD..0042DBB4 and 0042DBB5..0042DBDE.
		static constexpr bool allowed[42] = {
			true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false,
			true, true, true, true, true, true, false, false, false, true, true, false, false, false,
			true, false, false, false, true, true, false, false, false, false, false, false
		};
		return tribeType < 42 && allowed[tribeType];
	}
}
