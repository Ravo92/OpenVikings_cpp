#include "C2CallbackManager.h"
#include "C2GameControlStartup.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanNeeds.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapMoveableAccess.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticTribeTypeData.h"
#include "C2UserMessageAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	// Original function: FUN_0041cd08; distinct Windows outlined access, Mac build inlines it.
	void* Human_l_GetMerchantDataPtr(const void* human)
	{
		if (!Human_IsJobTrader(human))
			return nullptr;

		return NC2Lifetime::Read<std::byte*>(gHumanMerchantArray, NC2Layout::MerchantManagerOffset::Entries) + static_cast<std::uint32_t>(NC2Lifetime::Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)) * 0xC4u;
	}

	// Original function: FUN_00436B16; Windows outlined trader-only merchant reference.
	void* Human_GetMerchantDataPtr(const void* human)
	{
		return Human_IsJobTrader(human) ? HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, NC2Lifetime::Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)) : nullptr;
	}

	// Original function: FUN_0044B958.
	bool Human_Production_IsInfinite(const void* human, std::uint32_t good)
	{
		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == NC2Runtime::ProductionCounter::Infinite;
	}

	// Original function: FUN_0044B96C.
	bool Human_Production_IsStopped(const void* human, std::uint32_t good)
	{
		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == 0;
	}

	// Original function: FUN_0044B97F.
	bool Human_Production_IsForbidden(const void* human, std::uint32_t good)
	{
		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == NC2Runtime::ProductionCounter::Forbidden;
	}

	// Original function: FUN_0044B993.
	bool Human_Production_IsIncapable(const void* human, std::uint32_t good)
	{
		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == NC2Runtime::ProductionCounter::Incapable;
	}

	// Original function: FUN_0044B9A7.
	bool Human_Production_IsImpossible(const void* human, std::uint32_t good)
	{
		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == NC2Runtime::ProductionCounter::Impossible;
	}

	// Original function: FUN_0042c8e6.
	bool Human_IsForcedToHaveFinishedWorkHouse(const void* human)
	{
		return NC2Lifetime::Read<std::uint8_t>(gStaticJobTypeData, NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::MustHaveFinishedWorkHouseUnsigned) != 0;
	}

	using namespace NC2Lifetime;

	// Original function: FUN_0044b3c5.
	void Human_UpdateCurrentBuildHouse(void* human)
	{
		const void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;
		Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Human_WorkCenterMarker_Update(human);
	}

	// Original function: FUN_0044b3ed.
	void Human_UpdateCurrentWorkHouse(void* human)
	{
		if (Human_WorkCenterMarker_IsAllowed(human))
			return;

		const void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;
		Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Human_WorkCenterMarker_Update(human);
	}

	// Original function: FUN_0044b425; outlined IsJobWoman 0042C5AA checks job 5 alone.
	void Human_UpdateCurrentHomeHouse(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 5)
			return;

		const void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) * CHouseArray::ElementSize;
		Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Human_WorkCenterMarker_Update(human);
	}

	// Original function: FUN_004422be.
	std::uint32_t Human_l_JobEfficiency_CalculateToolFactor(const void* human)
	{
		if (Human_HasEquippedTools(human))
		{
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools);

			if (good == 31)
				return 125;

			if (good == 32)
				return 175;
		}

		return 100;
	}

	namespace
	{
		const STribeJobEnables& JobEnables(const void* human)
		{
			return gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].jobEnables[Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType)];
		}
		void AppendMessageItem(void* message, std::uint32_t countOffset, std::uint32_t item)
		{
			const auto count = Read<std::uint32_t>(message, countOffset);
			Write<std::uint32_t>(message, countOffset + NC2Layout::CountedArrayOffset::EntriesUnsigned + count * NC2Layout::RecordSize::UserMessageItemUnsigned, item);
			Write<std::uint32_t>(message, countOffset, count + 1u);
		}
	}

	// Original function: FUN_00441aad.
	bool Human_Misc_IsHidden(const void* human)
	{
		return Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag) != 0 || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0;
	}

	// Original function: FUN_004355b5; the first queued job-change command overrides the current job.
	std::uint32_t Human_UserControl_GetNextJobType(const void* human)
	{
		for (std::uint32_t slot = 0; slot < 87; ++slot)
		{
			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + slot * NC2Layout::RecordSize::UserNetworkCommand) == 21)
				return Read<std::uint32_t>(human, NC2Layout::HumanOffset::FirstUserCommandParameter1Unsigned + slot * NC2Layout::RecordSize::UserNetworkCommand);
		}

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
	}

	// Original function: FUN_004622b9.
	std::uint32_t Tool_Human_GetReallyProducedGoodType(std::uint32_t job, std::uint32_t good)
	{
		if (job == 15 && good == 56)
			return 21;

		return good == 22 ? 16u : good;
	}

	// Original function: FUN_004369e1; separate scans retain command precedence.
	bool Human_UserControl_DoesWantToMarry(const void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 5)
			return true;

		for (const auto opcode : { 13u, 14u })
		{
			for (std::uint32_t index = 0; index < 87; ++index)
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + index * NC2Layout::RecordSize::UserNetworkCommand) == opcode)
					return true;
			}
		}

		return false;
	}

	// Original function: FUN_00436a19; only job 5 enters the child-command scans.
	bool Human_UserControl_DoesWantToGetChild(const void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 5)
			return false;

		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (task == 11 || task == 12)
			return true;

		for (const auto opcode : { 15u, 16u })
		{
			for (std::uint32_t index = 0; index < 87; ++index)
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + index * NC2Layout::RecordSize::UserNetworkCommand) == opcode)
					return true;
			}
		}

		return false;
	}

	// Original function: FUN_004421e2, including unsigned product wrap before division.
	std::uint32_t Human_Tool_Gui_GetExperienceForEquippedWeapon(const void* human)
	{
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const auto weapon = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);
		const auto category = Read<std::uint32_t>(gStaticWeaponTypeArrays[tribe], weapon * NC2Layout::RecordSize::StaticWeaponTypeUnsigned + NC2Layout::StaticWeaponTypeOffset::MainType);

		std::uint32_t experience = 0;

		if (category == 1)
		{
			experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory1Experience);
		}
		else if (category == 2)
		{
			experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory2Experience);
		}
		else if (category == 3)
		{
			experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory3Experience);
		}
		else if (category == 6)
		{
			experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory6Experience);
		}
		else if (category == 7)
		{
			experience = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WeaponCategory7Experience);
		}

		const auto percentage = experience * 100u / 100u;

		return percentage > 100u ? 100u : percentage;
	}

	// Original function: FUN_0046224c; the final matching experience IDs win.
	void Tool_Human_GetExperienceType_GainedViaWorking(std::int32_t job, std::uint32_t good, std::uint32_t& general, std::uint32_t& specific)
	{
		general = 0; specific = 0;

		if (job < 7 || job > 30)
			return;

		const auto* table = static_cast<const std::byte*>(gStaticHumanJobExperienceTable00510A3C) + static_cast<std::uint32_t>(job) * NC2Layout::RecordSize::JobExperienceTypeListUnsigned;
		const auto count = Read<std::uint32_t>(table, NC2Layout::JobExperienceTypeListOffset::Count);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto id = Read<std::uint32_t>(table, NC2Layout::JobExperienceTypeListOffset::TypesUnsigned + index * NC2Layout::RecordSize::JobExperienceTypeIdUnsigned);
			const auto* data = static_cast<const std::byte*>(gStaticHumanJobExperienceData) + id * NC2Layout::RecordSize::JobExperienceTypeUnsigned;
			const auto firstGood = Read<std::uint32_t>(data, NC2Layout::JobExperienceTypeOffset::FirstGoodType);

			if (firstGood == 0)
			{
				general = id;
			}
			else if (firstGood == good || (Read<std::uint32_t>(data, NC2Layout::JobExperienceTypeOffset::SecondGoodType) != 0 && Read<std::uint32_t>(data, NC2Layout::JobExperienceTypeOffset::SecondGoodType) == good))
			{
				specific = id;
			}
		}
	}

	// Original function: FUN_00442047.
	void Human_GainExperienceForEquippedWeapon(void* human, std::uint32_t amount)
	{
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit11) == 0) Human_GainExperienceForWeapon(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon), amount);
	}

	// Original function: FUN_0044206c; weapon-category experience uses unsigned saturation.
	void Human_GainExperienceForWeapon(void* human, std::uint32_t weapon, std::uint32_t amount)
	{
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit11) != 0)
			return;

		const auto category = Read<std::uint32_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)], weapon * NC2Layout::RecordSize::StaticWeaponTypeUnsigned + NC2Layout::StaticWeaponTypeOffset::MainTypeUnsigned);
		const auto offset = category == 1 ? NC2Layout::HumanOffset::WeaponCategory1ExperienceUnsigned : category == 2 ? NC2Layout::HumanOffset::WeaponCategory2ExperienceUnsigned : category == 3 ? NC2Layout::HumanOffset::WeaponCategory3ExperienceUnsigned : category == 6 ? NC2Layout::HumanOffset::WeaponCategory6ExperienceUnsigned : category == 7 ? NC2Layout::HumanOffset::WeaponCategory7ExperienceUnsigned : 0u;

		if (offset == 0)
			return;

		const auto previous = Read<std::uint32_t>(human, offset);
		Write(human, offset, previous > 10000u - amount ? 10000u : previous + amount);
	}

	// Original function: FUN_00442420.
	void Human_l_JobExperience_Gain(void* human)
	{
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit11) != 0)
			return;

		auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (good == 65)
			good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CollectionGoodType);

		std::uint32_t general, specific;
		Tool_Human_GetExperienceType_GainedViaWorking(Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType), good, general, specific);

		auto increment = [human] (std::uint32_t offset)
		{
			const auto previous = Read<std::uint32_t>(human, offset); Write(human, offset, previous > 9999u ? 10000u : previous + 1u);
		};

		if (general != 0)
			increment(NC2Layout::HumanOffset::JobExperience + general * NC2Layout::RecordSize::HumanJobValueUnsigned);

		if (specific != 0 && specific != general)
			increment(NC2Layout::HumanOffset::JobExperience + specific * NC2Layout::RecordSize::HumanJobValueUnsigned);

		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		if ((job >= 31 && job <= 35) || (job >= 40 && job <= 41))
		{
			increment(NC2Layout::HumanOffset::SoldierWorkExperience);
			Human_GainExperienceForWeapon(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon), 1);
		}
	}

	// Original function: FUN_0044b9db; increasing can select infinite production; decreasing stops at zero.
	void Human_Production_ChangeCounter(void* human, std::uint32_t good, std::int32_t amount)
	{
		const auto offset = NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned;
		if (amount > 0)
		{
			while (amount-- > 0 && Read<std::int32_t>(human, offset) < NC2Runtime::ProductionCounter::Infinite) Write(human, offset, Read<std::uint32_t>(human, offset) + 1u);
		}
		else while (amount++ < 0 && Read<std::int32_t>(human, offset) > 0)
		{
			Write(human, offset, Read<std::uint32_t>(human, offset) - 1u);
		}
	}

	// Original function: FUN_0044b8fc; rotation also resets the production failure counter.
	bool Human_Production_Rotate(void* human)
	{
		const auto initial = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex);
		for (;;)
		{
			auto index = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex) + 1u;

			if (NXBasics::Runtime_TimeSigned32(index) >= 66)
				index = 0;

			Write(human, NC2Layout::HumanOffset::ProductionRotationIndex, index);
			const auto counter = Read<std::int32_t>(human, NC2Layout::HumanOffset::ProductionCounters + index * NC2Layout::RecordSize::HumanJobValueUnsigned);

			if (counter > 0 && counter <= NC2Runtime::ProductionCounter::Infinite)
			{
				Write(human, NC2Layout::HumanOffset::ProducedGoodType, index); Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0); return true;
			}

			if (index == initial)
				break;
		}
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);

		return false;
	}

	// Original function: FUN_0044B871; round at five decimal tenths.
	void Human_Production_DidProduceGoods(void* human, std::int32_t amount, std::int32_t fraction)
	{
		if (fraction >= 5) amount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(amount) + 1u);
		Human_Production_DidProduceGoods(human, static_cast<std::uint32_t>(amount));
	}

	// Original function: FUN_0044b888.
	void Human_Production_DidProduceGoods(void* human, std::uint32_t amount)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) != NC2Runtime::ProductionCounter::Infinite)
			Human_Production_ChangeCounter(human, good, NXBasics::Runtime_TimeSigned32(0u - amount));

		if (Human_IsJobBuilder(human))
			return;

		// Original message functions: FUN_0045c608 and FUN_0045c667.
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned) == 0 && good != 0)
		{
			Human_UserMessage_SendToUserMessage_DidProduceOneGood(human, good);
		}

		if (!Human_Production_Rotate(human))
		{
			Human_UserMessage_SendToUserMessage_DidProduceAllGoods(human);
		}
	}

	// Original function: FUN_0044b8dd.
	void Human_Production_CouldNotProduceCurrentGood(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (good != 0)
		{
			Human_UserMessage_SendToUserMessage_CouldNotProduceOneGood(human, good);
		}

		if (!Human_Production_Rotate(human))
		{
			Human_UserMessage_SendToUserMessage_CouldNotProduceAnyGoods(human);
		}
	}

	// Original function: FUN_0044224e.
	std::int32_t Human_l_JobEfficiency_GetNeededRetries(const void* human)
	{
		return Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkRetriesRequired);
	}

	// Original function: FUN_00442255.
	void Human_l_JobEfficiency_SetupNeededRetries(void* human)
	{
		Write(human, NC2Layout::HumanOffset::WorkRetriesRequired, Human_l_JobEfficiency_CalculateNeededRetries(human));
	}

	// Original function: FUN_00442265; preserve signed division and x86 wrapping arithmetic.
	std::int32_t Human_l_JobEfficiency_CalculateNeededRetries(const void* human)
	{
		const auto base = Human_JobEfficiency_GetBaseRetryCounter(human);
		const auto factor = static_cast<std::int32_t>(Human_l_JobEfficiency_CalculateToolFactor(human));
		const auto reduction = Human_JobEfficiency_CalculateExperienceFactor(human) / 20;
		const auto retries = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(base) * 100u) / factor) - static_cast<std::uint32_t>(reduction));

		return retries < 1 ? 1 : retries;
	}

	// Original function: FUN_004422e9; tool quality is checked by HasEquippedTools.
	std::int32_t Human_l_JobEfficiency_CalculateOutputAmountDecimal(const void* human)
	{
		std::uint32_t factor = 100;
		if (Human_HasEquippedTools(human))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 31)
			{
				factor = 125;
			}
			else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 32)
			{
				factor = 175;
			}
		}
		const auto experience = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(Human_JobEfficiency_CalculateExperienceFactor(human)) * 15u) / 100;

		return NXBasics::Runtime_TimeSigned32(factor * 10u) / 100 + experience;
	}

	// Original function: FUN_00442335; signed and unsigned divisions match their native stages.
	std::int32_t Human_JobEfficiency_CalculateExperienceFactor(const void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		std::uint32_t general, specific;
		Tool_Human_GetExperienceType_GainedViaWorking(Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType), good == 65 ? Read<std::uint32_t>(human, NC2Layout::HumanOffset::CollectionGoodType) : good, general, specific);
		const auto id = specific != 0 ? specific : general;

		if (id == 0)
			return 0;

		const auto weighted = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + id * NC2Layout::RecordSize::HumanJobValueUnsigned) * Read<std::uint32_t>(gStaticHumanJobExperienceData, id * NC2Layout::RecordSize::JobExperienceTypeUnsigned + NC2Layout::JobExperienceTypeOffset::ExperienceFactor) / 100u;
		const auto divisor = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(weighted * 100u) / 5) + 100u);
		const auto value = NXBasics::Runtime_TimeSigned32(100u - static_cast<std::uint32_t>(10000 / divisor));
		const auto percentage = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) * 100u) / 98;

		return percentage < 0 ? 0 : percentage > 100 ? 100 : percentage;
	}

	// Original function: FUN_004423ce.
	std::int32_t Human_JobEfficiency_GetBaseRetryCounter(const void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		std::uint32_t general, specific;
		Tool_Human_GetExperienceType_GainedViaWorking(Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType), good == 65 ? Read<std::uint32_t>(human, NC2Layout::HumanOffset::CollectionGoodType) : good, general, specific);
		const auto id = specific != 0 ? specific : general;

		return id == 0 ? 10 : Read<std::int32_t>(gStaticHumanJobExperienceData, id * NC2Layout::RecordSize::JobExperienceTypeUnsigned + NC2Layout::JobExperienceTypeOffset::BaseRepeatCounterUnsigned);
	}

	// Original function: FUN_0046a3b3; Windows-only leaf, original Mac name unresolved.
	void Human_ChangeToolCondition(void* human, std::int32_t delta)
	{
		auto condition = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) + static_cast<std::uint32_t>(delta));

		if (condition < 0)
		{
			condition = 0;
		}
		else if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 31 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 32) && condition > 10000)
		{
			condition = 10000;
		}

		Write(human, NC2Layout::HumanOffset::ToolCondition, condition);
	}

	// Original function: FUN_0042c506. Signed player comparison; negative IDs remain unchecked.
	std::uint32_t Human_GetPlayerType(const void* human)
	{
		const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
		if (player >= 20)
			return 0;

		return Read<std::uint32_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TypeUnsigned);
	}

	// Original function: FUN_0042c571.
	bool Human_IsAgeBaby(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType); return job == 1 || job == 2;
	}

	// Original function: FUN_0042c5be.
	bool Human_IsJobBuilder(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 7;
	}

	// Original function: FUN_0042c5db.
	bool Human_IsJobScout(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 27;
	}

	// Original function: FUN_0042c5e5.
	bool Human_IsJobHunter(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 15;
	}

	// Original function: FUN_0042c5ef.
	bool Human_IsJobBreeder(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 16;
	}

	// Original function: FUN_0042c5f9.
	bool Human_IsJobTrader(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType); return job == 25 || job == 26;
	}

	// Original function: FUN_0042c60d.
	bool Human_IsJobCarrier(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 24;
	}

	// Original function: FUN_0042c617.
	bool Human_IsJobFisher(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType); return job == 22 || job == 23;
	}

	// Original function: FUN_0042c878.
	bool Human_IsProducingVehicle(const void* human)
	{
		const auto good = Read<std::int32_t>(human, NC2Layout::HumanOffset::ProducedGoodType); return good >= 59 && good <= 63;
	}

	// Original function: FUN_0042d471.
	void Human_Equipment_SetShoeType(void* human, std::uint32_t good)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes, good);
	}

	void* HumanMerchantArray_GetMerchantDataPtr(const void* manager, std::int32_t id)
	{
		return Read<std::byte*>(manager, NC2Layout::MerchantManagerOffset::Entries) + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::Merchant;
	}

	// Original function: FUN_0041c799; the two house records have a 0x54-byte stride.
	void* MerchantData_GetHouseDataPtr(void* merchant, std::uint32_t index)
	{
		return static_cast<std::byte*>(merchant) + NC2Layout::MerchantOffset::HousesUnsigned + index * NC2Layout::RecordSize::MerchantHouseUnsigned;
	}

	// Original function: FUN_0041c98d; allocation and revision both participate.
	bool MerchantHouseData_IsValid(const void* houseData)
	{
		const auto id = Read<std::int32_t>(houseData, NC2Layout::MerchantHouseOffset::HouseId);

		if (id == -1)
			return false;

		const auto* house = CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * CHouseArray::ElementSize;

		return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(houseData, NC2Layout::MerchantHouseOffset::HouseRevision);
	}

	// Original function: FUN_0042cc0b.
	std::uint32_t Human_GetMilitaryMode(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode);
	}

	// Original function: FUN_0042d6ad.
	bool Human_CanEquipShoes(const void* human)
	{
		if (!Human_IsSexMale(human))
			return false;

		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);
		return job < 1 || job > 4;
	}

	// Original function: FUN_0042d8ac; the first experience type of the requirement.
	std::uint32_t Human_GetExperienceNeededYetForGoodProduction_Type(const void* human, std::uint32_t good)
	{
		return static_cast<std::uint32_t>(gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].goodNeeds[good].jobs[0]);
	}

	// Original function: FUN_0042d8ce; subtraction retains Windows DWORD wrapping.
	std::int32_t Human_GetExperienceNeededYetForGoodProduction_Value(const void* human, std::uint32_t good)
	{
		const auto& requirement = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].goodNeeds[good];
		std::uint32_t gained = 0;

		for (std::uint32_t index = 0; index < requirement.jobCount; ++index)
		{
			gained += Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + static_cast<std::uint32_t>(requirement.jobs[index]) * NC2Layout::RecordSize::HumanJobValueUnsigned);
		}

		return NXBasics::Runtime_TimeSigned32(requirement.requiredExperience - gained);
	}

	// Original function: FUN_0042d6cf. Both initial tool checks call this same Windows function.
	bool Human_CanEquipToolIron(const void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);
		return (job >= 8 && job <= 30) || job == 7;
	}

	// Original function: FUN_0042d798.
	bool Human_DoesExperienceAllowJobChange(const void* human, std::uint32_t job)
	{
		const auto& requirement = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].jobNeeds[job];
		std::uint32_t experience = 0;

		for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
		{
			experience += Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + static_cast<std::uint32_t>(requirement.jobs[i]) * NC2Layout::RecordSize::HumanJobValueUnsigned);
		}

		return experience >= requirement.requiredExperience;
	}

	// Original function: FUN_0042d7dd.
	bool Human_DoesExperienceAllowGoodProduction(const void* human, std::uint32_t good)
	{
		const auto& requirement = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].goodNeeds[good];
		std::uint32_t experience = 0;

		for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
		{
			experience += Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + static_cast<std::uint32_t>(requirement.jobs[i]) * NC2Layout::RecordSize::HumanJobValueUnsigned);
		}

		return experience >= requirement.requiredExperience;
	}

	// Original function: FUN_0042d822.
	bool Human_DoesExperienceAllowHouseBuilding(const void* human, std::uint32_t house)
	{
		const auto& requirement = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].houseNeeds[house];
		std::uint32_t experience = 0;

		for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
		{
			experience += Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + static_cast<std::uint32_t>(requirement.jobs[i]) * NC2Layout::RecordSize::HumanJobValueUnsigned);
		}

		return experience >= requirement.requiredExperience;
	}

	// Original function: FUN_0042d867.
	bool Human_DoesExperienceAllowEquipment(const void* human, std::uint32_t good)
	{
		const auto& requirement = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)].equipmentNeeds[good];
		std::uint32_t experience = 0;

		for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
		{
			experience += Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobExperience + static_cast<std::uint32_t>(requirement.jobs[i]) * NC2Layout::RecordSize::HumanJobValueUnsigned);
		}

		return experience >= requirement.requiredExperience;
	}

	// Original function: FUN_0042d912; remaster disables the unrelated-job training-counter shortcut.
	bool Human_DoesEducationAllowJobChange(const void* /*human*/, std::uint32_t /*job*/)
	{
		// Human_l_JobTraining_Train explicitly unlocks the selected job on completion.
		return false;
	}

	// Original function: FUN_0042d938; remaster keeps partial training out of general good unlocks.
	bool Human_DoesEducationAllowGoodProduction(const void* /*human*/, std::uint32_t /*good*/)
	{
		// The selected training good is granted by the same completed-training path.
		return false;
	}

	// Original function: FUN_0042d9fa.
	bool Human_IsAbleToProduceGoodNow(const void* human, std::uint32_t good)
	{
		if (good == 0)
			return true;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) == 0 || !Human_IsCapableToProduceGood(human, good))
			return false;

		const auto houseId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		if (houseId == NXBasics::IntegerRange::UInt32Maximum)
			return Read<std::uint8_t>(gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::MustHaveWorkHouseUnsigned) == 0;

		const void* house = CHouseArray::mStaticVars + houseId * CHouseArray::ElementSize;

		if (Read<std::uint8_t>(gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::MustHaveFinishedWorkHouseUnsigned) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
			return false;

		return House_Production_IsAbleToProduceGood(house, good, true) || House_Production_IsAbleToProduceGood(house, good, false);
	}

	// Original function: FUN_0042d95e.
	bool Human_IsAllowedToDoJob(const void* human, std::uint32_t job)
	{
		return Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job) != 0;
	}

	// Original function: FUN_0042d96c; military jobs also require the matching equipped weapon.
	bool Human_IsAbleToDoJob(const void* human, std::uint32_t job)
	{
		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job) == 0)
			return false;

		const auto weapon = Read<std::int32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);

		switch (job)
		{
			case SwitchIds::HumanJob::SoldierUnarmed: return weapon < 4 || weapon > 17;
			case SwitchIds::HumanJob::SoldierSpearWooden: return weapon == 4;
			case SwitchIds::HumanJob::SoldierSpearIron: return weapon == 5;
			case SwitchIds::HumanJob::SoldierSwordShort: return weapon == 7;
			case SwitchIds::HumanJob::SoldierSwordLong: return weapon == 8;
			case SwitchIds::HumanJob::SoldierBowShort: return weapon == 16;
			case SwitchIds::HumanJob::SoldierBowLong: return weapon == 17;
			default: return true;
		}
	}

	// Original function: FUN_0042dac1.
	bool Human_IsCapableToProduceGood(const void* human, std::uint32_t good)
	{
		if (good == 0 || Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) == 0)
			return false;

		const auto& enables = JobEnables(human);

		for (std::uint32_t i = 0; i < enables.goodCount; ++i)
		{
			if (static_cast<std::uint32_t>(enables.goods[i]) == good)
				return true;
		}

		return false;
	}

	// Original function: FUN_0042db3e.
	std::uint32_t Human_GetHumanAttributeMaximum_Hitpoints(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32 ? 20000u : 5000u;
	}

	// Original function: FUN_0042db55.
	std::int32_t Human_GetHumanAttributeMargin_Hitpoints(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 32 ? 2880 : 720;
	}

	// Original function: FUN_004316ef.
	bool House_Production_IsAbleToProduceGood(const void* house, std::uint32_t good, bool primary)
	{
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
		const auto* data = reinterpret_cast<const std::byte*>(gStaticHouseTypeData) + type * 0x624u;
		const auto category = Read<std::uint32_t>(data, NC2Layout::StaticHouseTypeOffset::MainType);

		if (category == 2)
		{
			if (good == 29)
				return !primary && Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancers) != 0;

			if (good == 28)
				return !primary && Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer1Allowed) != 0;

			if (good == 15)
				return !primary && Read<std::uint8_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Allowed) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) >= 2;
		}
		else if (!primary && good == 15 && type == 37) return true;

		const auto slot = House_l_Stock_GetStockSlotIdForGoodType(house, good);

		if (slot == -1)
			return false;

		if (category != 3)
			return true;

		const bool slotPrimary = Read<std::uint8_t>(data, NC2Layout::StaticHouseTypeOffset::StockUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::StaticHouseStockUnsigned + NC2Layout::StaticHouseStockOffset::IsOutputStockUnsigned) != 0;
		return primary ? !slotPrimary : slotPrimary;
	}

	// Original function: FUN_0044a07c; update existing capabilities after script allowances change.
	void Human_UpdatePlayerSpecificStuffAndCaps(void* human)
	{
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, SwitchIds::UserMessage::ExperienceUnlocks);
		const auto* player = static_cast<const std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;

		if (Human_IsSexMale(human) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit6) == 0)
		{
			for (std::uint32_t job = 6; job <= 41; ++job)
			{
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) == 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 0);

				else if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job) == 0 && Human_DoesExperienceAllowJobChange(human, job))
				{
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
					AppendMessageItem(message, NC2Layout::ToUserMessageOffset::JobCount, job);
				}
			}
		}

		else Write<std::uint8_t>(human, NC2Layout::HumanOffset::WomanJobAllowedFlag, Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::WomanJobAllowedFlagUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned) != 0);
		if (Human_IsJobSoldier(human))
		{
			for (std::uint32_t job = 31; job <= 41; ++job)
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job) == 0 && Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0) Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
			for (std::uint32_t item = 37; item <= 42; ++item)
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item) != 0 || Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) == 0) continue;
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
				Human_UserMessage_SendToUserMessage_CanEquipNewItem(human, item);
			}
			for (std::uint32_t item = 33; item <= 36; ++item)
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item) != 0 || Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) == 0) continue;
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
				Human_UserMessage_SendToUserMessage_CanEquipNewItem(human, item);
			}
		}

		else if (Human_IsJobHero(human))
		{
			for (std::uint32_t job = 42; job <= 47; ++job)
			{
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
			}

			for (std::uint32_t item = 33; item <= 36; ++item)
			{
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
			}
		}

		if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::JobCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::GoodCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::EquipmentCount) != 0)
		{
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 8, 0);
		}

		Human_SetupCurrentJobType(human);
		Human_UpdateCurrentJobType(human);
	}

	// Original function: FUN_0044a45d.
	void Human_SetupCurrentJobType(void* human)
	{
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, SwitchIds::UserMessage::ExperienceUnlocks);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId, player);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::ProductionSelectionMode, Read<std::uint8_t>(gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::UserCanChangeProductionUnsigned) != 0 ? 1 : 0);
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		auto* playerData = static_cast<std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
		const auto& enables = JobEnables(human);

		const auto enableGood = [&] (std::uint32_t unlockedJob, std::uint32_t good)
		{
			if (unlockedJob == 15)
			{
				if (good == 56) Write<std::uint8_t>(human, NC2Layout::HumanOffset::HuntingProductionAllowedFlag, 1);
			}
			else if (unlockedJob != 16 || good == 57 || good == 58)
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good, 1);

			const auto availableOffset = NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good;

			if (Read<std::uint8_t>(playerData, availableOffset) == 0)
			{
				Write<std::uint8_t>(playerData, availableOffset, 1);
				AppendMessageItem(message, NC2Layout::ToUserMessageOffset::GoodCount, good);
			}
		};

		const auto requirementsMet = [&] (std::uint32_t house)
		{
			STribeBuildRequirement requirement;
			std::memcpy(&requirement, &gStaticTribeTypeData[tribe].houseBuildNeeds[house], NC2Layout::RecordSize::TribeBuildRequirement);

			for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
			{
				if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + static_cast<std::uint32_t>(requirement.jobs[i])) == 0)
					return false;
			}

			for (std::uint32_t i = 0; i < requirement.goodCount; ++i)
			{
				if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + static_cast<std::uint32_t>(requirement.goods[i])) == 0)
					return false;
			}

			return true;
		};

		const auto enableHouse = [&] (std::uint32_t house)
		{
			PlayerAllowedThings_MarkHouseAvailable(playerData + NC2Layout::GameControlPlayerOffset::AllowedThings, tribe, house);
			AppendMessageItem(message, NC2Layout::ToUserMessageOffset::HouseCount, house);
		};

		for (std::uint32_t i = 0; i < enables.jobCount; ++i)
		{
			const auto unlockedJob = static_cast<std::uint32_t>(enables.jobs[i]);

			if (!Human_DoesExperienceAllowJobChange(human, unlockedJob) && !Human_DoesEducationAllowJobChange(human, unlockedJob) && Human_GetPlayerType(human) != 2)
				continue;

			auto* currentPlayer = static_cast<std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
			const auto currentTribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);

			if (Read<std::uint8_t>(currentPlayer, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + currentTribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + unlockedJob) == 0)
				continue;

			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + unlockedJob, 1);
			const auto availableOffset = NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + currentTribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + unlockedJob;

			if (Read<std::uint8_t>(currentPlayer, availableOffset) != 0)
				continue;

			Write<std::uint8_t>(currentPlayer, availableOffset, 1);
			AppendMessageItem(message, NC2Layout::ToUserMessageOffset::JobCount, unlockedJob);
			const auto& unlocked = gStaticTribeTypeData[currentTribe].jobEnables[unlockedJob];

			for (std::uint32_t j = 0; j < unlocked.goodCount; ++j)
			{
				const auto good = static_cast<std::uint32_t>(unlocked.goods[j]);
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0 ||
					Read<std::uint8_t>(currentPlayer, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + currentTribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
					continue;

				if (Human_DoesExperienceAllowGoodProduction(human, good) || Human_DoesEducationAllowGoodProduction(human, good) || Human_GetPlayerType(human) == 2)
					enableGood(unlockedJob, good);
			}

			for (std::uint32_t house = 2; house < 55; ++house)
			{
				if (Read<std::uint8_t>(currentPlayer, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + currentTribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) == 0 ||
					Read<std::uint8_t>(currentPlayer, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + currentTribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) != 0)
					continue;

				if (Read<std::uint32_t>(gStaticHouseTypeData, house * NC2Layout::RecordSize::StaticHouseTypeUnsigned) == 7 || !requirementsMet(house))
					continue;

				enableHouse(house);
			}
		}

		for (std::uint32_t i = 0; i < enables.goodCount; ++i)
		{
			const auto good = static_cast<std::uint32_t>(enables.goods[i]);
			if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
				continue;

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0 &&
				Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) != 0)
				continue;

			if (!Human_DoesExperienceAllowGoodProduction(human, good) && !Human_DoesEducationAllowGoodProduction(human, good) && Human_GetPlayerType(human) != 2)
				continue;

			enableGood(Human_IsJobHunter(human) ? 15u : (Human_IsJobBreeder(human) ? 16u : 0u), good);
		}

		for (std::uint32_t i = 0; i < enables.houseCount; ++i)
		{
			const auto house = static_cast<std::uint32_t>(enables.houses[i]);
			if (!Human_DoesExperienceAllowHouseBuilding(human, house) && Human_GetPlayerType(human) != 2) continue;
			if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) == 0 || Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) != 0) continue;
			if (Read<std::uint32_t>(gStaticHouseTypeData, house * NC2Layout::RecordSize::StaticHouseTypeUnsigned) == 7) continue;
			const bool allowed = requirementsMet(house);
			if (Human_GetPlayerType(human) == 2 || allowed) enableHouse(house);
		}

		auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (good != 0 && !Human_IsCapableToProduceGood(human, good))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		}

		good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if ((good == 0 || Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) == 0) && enables.defaultGood != 0)
		{
			const auto preferred = static_cast<std::uint32_t>(enables.defaultGood);

			if (Human_IsAbleToProduceGoodNow(human, preferred))
				Human_DoExecuteUserCommand_ChangeProducedGood(human, preferred);

			else for (std::uint32_t i = 0; i < enables.goodCount; ++i)
				if (Human_IsAbleToProduceGoodNow(human, static_cast<std::uint32_t>(enables.goods[i])))
				{
					Human_DoExecuteUserCommand_ChangeProducedGood(human, static_cast<std::uint32_t>(enables.goods[i]));
					break;
				}
		}

		std::memset(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AllowedEquipmentFlagsSigned, 0, 56);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::JobSelectionEnabledFlag, Human_IsAgeAdult(human) && Human_IsSexMale(human) && !Human_IsJobHero(human) ? 1 : 0);

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::JobSelectionEnabledFlag) != 0)
		{
			// Remaster: equipment entries have their own count, independent of production goods.
			for (std::uint32_t i = 0; i < enables.equipmentCount && i < 50u; ++i)
			{
				const auto item = static_cast<std::uint32_t>(enables.equipment[i]);

				if (item >= 56u)
					continue;

				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item) != 0)
					continue;

				if ((Human_DoesExperienceAllowEquipment(human, item) || Human_GetPlayerType(human) == 2) && Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
			}

			if (Human_CanEquipShoes(human))
			{
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipment30Flag, 1);
				if (Human_CanEquipToolIron(human))
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipment31Flag, 1);

				if (Human_CanEquipToolIron(human))
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipment32Flag, 1);
			}

			for (std::uint32_t item = 43; item <= 55; ++item)
			{
				if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
			}

			if (Human_IsJobSoldier(human))
			{
				for (std::uint32_t nextJob = 31; nextJob <= 41; ++nextJob)
					if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + nextJob) != 0)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + nextJob, 1);

				for (std::uint32_t item = 37; item <= 42; ++item)
					if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);

				for (std::uint32_t item = 33; item <= 36; ++item)
					if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
			}
			else if (Human_IsJobHero(human))
			{
				for (std::uint32_t nextJob = 42; nextJob <= 47; ++nextJob)
					if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + nextJob) != 0)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + nextJob, 1);

				for (std::uint32_t item = 33; item <= 36; ++item)
					if (Read<std::uint8_t>(playerData, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
			}
		}
		// Original PE constants: 004F3B8C/90/94/98 = 10/5/20/20.
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius, Human_IsJobScout(human) ? 20u : (Human_IsJobHero(human) ? 20u : (Human_IsAgeAdult(human) ? 10u : 5u)));
		Human_DoCheckAllUserCommands(human);

		if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::JobCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::GoodCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::EquipmentCount) != 0)
		{
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 8, 0);
		}
	}

	// Original function: FUN_0044ad2e; update only newly unlocked entries and retain native message order.
	void Human_UpdateCurrentJobType(void* human)
	{
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, SwitchIds::UserMessage::ExperienceUnlocks);
		const auto& enables = JobEnables(human);
		const auto requirementsMet = [human] (std::uint32_t house)
		{
			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			const auto* player = static_cast<const std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
			STribeBuildRequirement requirement;
			std::memcpy(&requirement, &gStaticTribeTypeData[tribe].houseBuildNeeds[house], NC2Layout::RecordSize::TribeBuildRequirement);

			for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + static_cast<std::uint32_t>(requirement.jobs[i])) == 0)
					return false;

			for (std::uint32_t i = 0; i < requirement.goodCount; ++i)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + static_cast<std::uint32_t>(requirement.goods[i])) == 0)
					return false;
			return true;
		};

		for (std::uint32_t i = 0; i < enables.jobCount; ++i)
		{
			const auto job = static_cast<std::uint32_t>(enables.jobs[i]);
			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job) != 0)
				continue;

			if (!Human_DoesExperienceAllowJobChange(human, job) && !Human_DoesEducationAllowJobChange(human, job) && Human_GetPlayerType(human) != 2)
				continue;

			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			auto* player = static_cast<std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;

			if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) == 0)
				continue;

			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
			if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0)
				continue;

			Write<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job, 1);
			AppendMessageItem(message, NC2Layout::ToUserMessageOffset::JobCount, job);
			const auto& unlocked = gStaticTribeTypeData[tribe].jobEnables[job];

			for (std::uint32_t j = 0; j < unlocked.goodCount; ++j)
			{
				const auto good = static_cast<std::uint32_t>(unlocked.goods[j]);
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0 || Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
					continue;

				if (!Human_DoesExperienceAllowGoodProduction(human, good) && !Human_DoesEducationAllowGoodProduction(human, good) && Human_GetPlayerType(human) != 2)
					continue;

				if (job == 15)
				{
					if (good == 56)
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::HuntingProductionAllowedFlag, 1);
				}
				else if (job != 16 || good == 57 || good == 58) Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good, 1);

				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
				{
					Write<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good, 1);
					AppendMessageItem(message, NC2Layout::ToUserMessageOffset::GoodCount, good);
				}
			}
			for (std::uint32_t house = 2; house < 55; ++house)
			{
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) == 0 ||
					Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) != 0 ||
					Read<std::uint32_t>(gStaticHouseTypeData, house * NC2Layout::RecordSize::StaticHouseTypeUnsigned) == 7
					|| !requirementsMet(house))

					continue;

				PlayerAllowedThings_MarkHouseAvailable(player + NC2Layout::GameControlPlayerOffset::AllowedThings, tribe, house);
				AppendMessageItem(message, NC2Layout::ToUserMessageOffset::HouseCount, house);
			}
		}
		for (std::uint32_t i = 0; i < enables.goodCount; ++i)
		{
			const auto good = static_cast<std::uint32_t>(enables.goods[i]);
			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0)
				continue;

			if (!Human_DoesExperienceAllowGoodProduction(human, good) && !Human_DoesEducationAllowGoodProduction(human, good) && Human_GetPlayerType(human) != 2)
				continue;

			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			auto* player = static_cast<std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;

			if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
				continue;

			if ((!Human_IsJobHunter(human) || good == 56) && (!Human_IsJobBreeder(human) || good == 57 || good == 58))
			{
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good, 1);
				Human_Production_SetCounter(human, good, Human_IsAbleToProduceGoodNow(human, good) ? 0u : NC2Runtime::ProductionCounter::ImpossibleUnsigned);
			}

			if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good) == 0)
			{
				Write<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + good, 1);
				AppendMessageItem(message, NC2Layout::ToUserMessageOffset::GoodCount, good);
			}
		}
		for (std::uint32_t i = 0; i < enables.houseCount; ++i)
		{
			const auto house = static_cast<std::uint32_t>(enables.houses[i]);

			if (!Human_DoesExperienceAllowHouseBuilding(human, house) && Human_GetPlayerType(human) != 2)
				continue;

			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			auto* player = static_cast<std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;

			if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) == 0 ||
				Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + house) != 0 ||
				Read<std::uint32_t>(gStaticHouseTypeData, house * NC2Layout::RecordSize::StaticHouseTypeUnsigned) == 7)
				continue;

			const auto allowed = requirementsMet(house);

			if (Human_GetPlayerType(human) != 2 && !allowed)
				continue;

			PlayerAllowedThings_MarkHouseAvailable(player + NC2Layout::GameControlPlayerOffset::AllowedThings, tribe, house);
			AppendMessageItem(message, NC2Layout::ToUserMessageOffset::HouseCount, house);
		}
		// Windows retains the goods-loop index when selecting the equipment pointer
		// and never advances that pointer (0044B309..0044B387).
		if (enables.equipmentCount != 0)
		{
			const auto item = Read<std::uint32_t>(&enables, NC2Layout::TribeJobEnablesOffset::EquipmentUnsigned + enables.goodCount * NC2Layout::RecordSize::TribeJobEnableItemUnsigned);
			for (std::uint32_t i = 0; i < enables.equipmentCount; ++i)
			{
				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item) != 0)
					continue;

				if (!Human_DoesExperienceAllowEquipment(human, item) && Human_GetPlayerType(human) != 2)
					continue;

				const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
				const auto* player = static_cast<const std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;

				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) == 0)
					continue;

				Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
				Human_UserMessage_SendToUserMessage_CanEquipNewItem(human, item);
			}
		}
		if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::JobCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::GoodCount) != 0 ||
			Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::EquipmentCount) != 0)
		{
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
		}
	}

	// Original function: FUN_0044a2ab.
	void Human_SetJobType(void* human, std::uint32_t job)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		const auto oldJob = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		if (oldJob != job)
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1F, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), oldJob, job);

		if (Human_IsAttachedToWorkHouse(human))
		{
			void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;

			if (Human_IsAttachedToLearnHouse(human))
				House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8);

			else
			{
				House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 7 ? 4 : 2);
				if (House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), job == 7 ? 4 : 2))
				{
					Human_l_AssignWorkHouse(human, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
					Human_WorkCenterMarker_Update(human);
				}
			}
		}
		if (Human_IsJobTrader(human) && job != 25)
		{
			const auto merchant = Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId);
			if (merchant != -1)
			{
				HumanMerchantArray_FreeMerchant(gHumanMerchantArray, merchant); Write<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId, -1);
			}
		}
		else if (job == 25 && Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId) == -1)
			Write<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId, HumanMerchantArray_AllocateMerchant(gHumanMerchantArray, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)));

		if (job == 15)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 19);

		else if
			(Human_IsJobHunter(human)) Write<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon, 1);

		const auto currentJob = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);
		const auto signedJob = static_cast<std::int32_t>(job);

		if ((currentJob >= 31 && currentJob <= 47) || (signedJob >= 31 && signedJob <= 47))
		{
			void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
			{
				const auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
				MapMoveable_Attachment_DetachFromMap(moveable);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::JobType, job);
				MapMoveable_Attachment_AttachToMap(moveable, &point);
			}
			else
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::JobType, job);
		}
		else
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::JobType, job);

		Human_SetupCurrentJobType(human);
		Human_Production_Setup(human);
	}

	// Original function: FUN_00449f48.
	void Human_SetupPlayerSpecificStuff(void* human)
	{
		const auto* player = static_cast<const std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		if (Human_IsSexMale(human))
		{
			for (std::uint32_t job = 6; job <= 41; ++job)
			{
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0)
				{
					if (Human_DoesExperienceAllowJobChange(human, job)) Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
				}
				else Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 0);
			}
		}
		else
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::WomanJobAllowedFlag, Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::WomanJobAllowedFlagUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned) != 0 ? 1 : 0);

		if (Human_IsJobSoldier(human))
		{
			for (std::uint32_t job = 31; job <= 41; ++job)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);

			for (std::uint32_t item = 37; item <= 42; ++item)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);

			for (std::uint32_t item = 33; item <= 36; ++item)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
		}
		else if (Human_IsJobHero(human))
		{
			for (std::uint32_t job = 42; job <= 47; ++job)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedJobsUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + job) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);

			for (std::uint32_t item = 33; item <= 36; ++item)
				if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::AllowedEquipmentUnsigned + tribe * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + item) != 0)
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedEquipmentFlags + item, 1);
		}
	}

	// Original function: FUN_0044b5fe.
	void Human_Production_Setup(void* human)
	{
		for (std::uint32_t good = 1; good < 66; ++good)
		{
			std::uint32_t state;

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) == 0)
				state = NC2Runtime::ProductionCounter::Forbidden;

			else if (!Human_IsCapableToProduceGood(human, good))
				state = NC2Runtime::ProductionCounter::Incapable;

			else
				state = Human_IsAbleToProduceGoodNow(human, good) ? 0u : NC2Runtime::ProductionCounter::ImpossibleUnsigned;

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned, state);
		}
		const auto& enables = JobEnables(human);

		if (enables.defaultGood == 0)
			return;

		const auto preferred = static_cast<std::uint32_t>(enables.defaultGood);

		if (Human_IsAbleToProduceGoodNow(human, preferred))
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + preferred * NC2Layout::RecordSize::HumanJobValueUnsigned, NC2Runtime::ProductionCounter::Infinite);

		else for (std::uint32_t i = 0; i < enables.goodCount; ++i)
			if (Human_IsAbleToProduceGoodNow(human, static_cast<std::uint32_t>(enables.goods[i])))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + static_cast<std::uint32_t>(enables.goods[i]) * NC2Layout::RecordSize::HumanJobValueUnsigned, NC2Runtime::ProductionCounter::Infinite);
				break;
			}
	}

	// Original function: FUN_0044b9bb.
	std::uint32_t Human_Production_GetCounter(const void* human, std::uint32_t good)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned);
	}

	// Original function: FUN_0043e4b0; CHuman::ChangeProducedGoodType.
	void Human_ChangeProducedGoodType(void* human, std::uint32_t good)
	{
		if (!Human_IsCapableToProduceGood(human, good))
			return;

		const auto& enables = JobEnables(human);

		for (std::uint32_t i = 0; i < enables.goodCount; ++i)
		{
			const auto item = static_cast<std::uint32_t>(enables.goods[i]);
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::ProductionCounters + item * NC2Layout::RecordSize::HumanJobValueUnsigned) <= NC2Runtime::ProductionCounter::Infinite &&
				Read<std::int32_t>(human, NC2Layout::HumanOffset::ProductionCounters + item * NC2Layout::RecordSize::HumanJobValueUnsigned) >= 1)
				Human_Production_SetCounter(human, item, 0);
		}

		Human_Production_SetCounter(human, good, NC2Runtime::ProductionCounter::Infinite);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Write(human, NC2Layout::HumanOffset::ProductionRotationIndex, good);
		Write(human, NC2Layout::HumanOffset::ProducedGoodType, good);
	}

	// Original function: FUN_004622b9; Windows outlined conversion, inlined in the Mac caller.
	static std::uint32_t Map_GoodTool_GetRealGoodType(std::uint32_t job, std::uint32_t good)
	{
		if (job == 15 && good == 56)
			return 21;

		return good == 22 ? 16u : good;
	}

	// Original function: FUN_0043dcb0; returns 0/1/2, not a Boolean.
	std::uint32_t Human_DoExecuteUserCommand_ChangeProducedGood(void* human, std::uint32_t good)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsCapableToProduceGood(human, good))
			return 1;

		if (Human_IsProducingVehicle(human))
		{
			Write<std::int32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, 0);
		}

		Human_ChangeProducedGoodType(human, good);

		if (Human_IsJobBuilder(human) && !Human_IsAttachedToWorkHouse(human))
		{
			if (Human_IsAttachedToRoadPoint(human))
				BuildManager_DetachBuilder(gRoadBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);

			if (Human_IsAttachedToWallPoint(human))
				BuildManager_DetachBuilder(gWallBuildManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter);
		}
		if (Human_IsAttachedToWorkHouse(human) && !Human_IsAttachedToLearnHouse(human) && good != 0 && good != 65)
		{
			const auto realGood = Map_GoodTool_GetRealGoodType(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), good);
			const void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize;

			if (!House_Production_IsAbleToProduceGood(house, realGood, false) && !House_Production_IsAbleToProduceGood(house, realGood, true))
				Human_DoExecuteUserCommand_DetachWorkHouse(human);
		}
		return 2;
	}

	// Original function: FUN_0044b6dc.
	void Human_Production_Update(void* human)
	{
		const auto& enables = JobEnables(human);

		for (std::uint32_t i = 0; i < enables.goodCount; ++i)
		{
			const auto good = static_cast<std::uint32_t>(enables.goods[i]);
			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) != 0)
				Human_Production_SetCounter(human, good, Human_IsAbleToProduceGoodNow(human, good) ? 0u : NC2Runtime::ProductionCounter::ImpossibleUnsigned);
		}

		const auto preferred = static_cast<std::uint32_t>(enables.defaultGood);

		if (preferred != 0)
		{
			if (Human_IsAbleToProduceGoodNow(human, preferred)) Human_Production_SetCounter(human, preferred, 0);
			else for (std::uint32_t i = 0; i < enables.goodCount; ++i) if (Human_IsAbleToProduceGoodNow(human, static_cast<std::uint32_t>(enables.goods[i])))
			{
				Human_Production_SetCounter(human, static_cast<std::uint32_t>(enables.goods[i]), 0); break;
			}
		}

		const auto current = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (current != 0 && Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + current) != 0 && !Human_Production_IsStopped(human, current))
			return;

		if (preferred == 0)
			return;

		std::uint32_t selected = preferred;

		if (!Human_IsAbleToProduceGoodNow(human, preferred))
		{
			std::uint32_t i;
			for (i = 0; i < enables.goodCount; ++i)
			{
				if (Human_IsAbleToProduceGoodNow(human, static_cast<std::uint32_t>(enables.goods[i])))
				{
					selected = static_cast<std::uint32_t>(enables.goods[i]); break;
				}
			}

			if (i == enables.goodCount)
				return;
		}
		Human_Production_SetCounter(human, selected, NC2Runtime::ProductionCounter::Infinite);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Write(human, NC2Layout::HumanOffset::ProductionRotationIndex, selected);
		Write(human, NC2Layout::HumanOffset::ProducedGoodType, selected);
	}

	// Original function: FUN_0042c584.
	bool Human_IsAgeChild(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		return job == 3 || job == 4;
	}

	// Original function: FUN_0042c5b4.
	bool Human_IsJobCivilist(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 6;
	}

	// Original function: FUN_0042c5c8.
	bool Human_IsJobWorker(const void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);
		return job >= 8 && job <= 30;
	}

	// Original function: FUN_0042c66c.
	bool Human_IsAttachedToWorkHouse2(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0042c865.
	bool Human_IsProducingNothing(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 0;
	}

	// Original function: FUN_0042c86e.
	bool Human_IsProducingFood(const void* human)
	{
		return TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType));
	}

	// Original function: FUN_0042c88b.
	bool Human_IsProducingMultipleGoods(const void* human)
	{
		std::uint32_t count = 0;
		for (std::uint32_t good = 0; good < 66; ++good)
		{
			const auto amount = Read<std::int32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned);

			if (amount > 0 && amount <= 11 && ++count >= 2)
				return true;
		}
		return false;
	}

	// Original function: FUN_0042c8cf.
	bool Human_IsForcedToHaveWorkHouse(const void* human)
	{
		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::MustHaveWorkHouseUnsigned) != 0;
	}

	// Original function: FUN_0042c8fd; Windows accepts every job.
	bool Human_IsAbleToUseVehicle(const void* human)
	{
		(void) human; return true;
	}

	// Original function: FUN_0042c96c.
	bool Human_IsDoingNothing(const void* human)
	{
		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 1 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1;
	}

	// Original function: FUN_00460320.
	const void* Human_GetWorkCenterPointRef(const void* human)
	{
		return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
	}

}