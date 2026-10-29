#include "RemasterAILog.h"
#include "RemasterAIEconomy.h"
#include "C2AIHumanRequests.h"
#include "C2AIGlobalGoods.h"
#include "C2AIObjectAccess.h"
#include "C2AIProduction.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanUserControl.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticHouseTypeData.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticTribeTypeData.h"
#include "C2VehicleAccess.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic::RemasterAIEconomy
{
	using namespace NC2Lifetime;

	namespace
	{
		// Windows Human_DoAddNewUserCommand stores opcode n in fixed native slot n - 2.
		bool Queued(const void* human, std::uint32_t opcode)
		{

			return Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandQueue + (opcode - 2u) * NC2Layout::RecordSize::UserNetworkCommand) == opcode;
		}

		std::uint32_t Distance(const void* a, const void* b)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, 0), Read<std::int16_t>(a, 2), Read<std::int16_t>(b, 0), Read<std::int16_t>(b, 2)));
		}

		bool IsSupply(std::uint32_t family)
		{
			const auto type = static_cast<EHouseType>(family);

			return type == EHouseType::Well || type == EHouseType::Hive || type == EHouseType::Farm || type == EHouseType::Mill || type == EHouseType::Bakery00;
		}

		bool HasVacancy(const void* house)
		{
			const auto* type = House_GetHouseTypeDataPtr(house);

			if (type->mainType != 3)
				return false;

			for (std::int32_t i = 0; i < type->workerCount; ++i)
			{
				const auto job = type->workers[i].jobType;
				// Collectors use flags; auxiliary carrier slots do not make a staffed craft idle.

				if (job == SwitchIds::HumanJob::Collector || (job == SwitchIds::HumanJob::Carrier && type->workerCount > 1))
					continue;

				if (job != 0 && House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job) == 0)
					return true;
			}

			return false;
		}

		bool CanPromoteWarehouseCarrier(const void* human, std::uint32_t job)
		{
			// Remaster: recover essential workers without taking the last warehouse carrier.

			if (job != SwitchIds::HumanJob::Baker && job != SwitchIds::HumanJob::Miller && job != SwitchIds::HumanJob::Builder)
				return false;

			if (!Human_IsJobCarrier(human) || !Human_IsAttachedToWorkHouse(human) || !Human_UserControl_CanChangeJob(human) || Human_IsAttachedToLearnHouse(human) || Human_IsAttachedToVehicle(human) || Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Human_UserControl_IsPriorityCommandInQueue(human))
				return false;

			for (const auto opcode : { SwitchIds::UserCommand::ChangeJob, SwitchIds::UserCommand::ChangeProducedGood, SwitchIds::UserCommand::ChangeProduction, SwitchIds::UserCommand::AttachWorkHouse, SwitchIds::UserCommand::DetachWorkHouse, SwitchIds::UserCommand::AttachBuildHouse, SwitchIds::UserCommand::AttachLearnHouse, SwitchIds::UserCommand::SetWorkCenter })
			{
				if (Queued(human, opcode))
					return false;
			}

			const auto houseId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

			if (houseId >= static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity))
				return false;

			const auto* house = GetHousePtr(static_cast<std::int32_t>(houseId));

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision) || Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) || House_GetHouseMainType(house) != 1 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				return false;

			CHumanIterator humans;

			for (auto id = humans.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)); id != -1; id = humans.GetNextHumanId())
			{
				const auto* other = GetHumanPtr(id);

				if (other == human || !Human_IsJobCarrier(other) || !Human_IsAttachedToWorkHouse(other) || Human_UserControl_GetNextJobType(other) != SwitchIds::HumanJob::Carrier || Queued(other, SwitchIds::UserCommand::DetachWorkHouse) || Queued(other, SwitchIds::UserCommand::AttachWorkHouse) || Human_IsAttachedToLearnHouse(other) || Human_IsAttachedToVehicle(other) || Read<std::uint32_t>(other, NC2Layout::HumanOffset::WorkAttachmentHouseId) != houseId || Read<std::uint32_t>(other, NC2Layout::HumanOffset::WorkAttachmentHouseRevision) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId))
					continue;

				if (Human_UserControl_IsPriorityCommandInQueue(other))
					continue;

				return true;
			}

			return false;
		}

		bool CanRecoverCollectorAsBuilder(const void* human)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Collector || !Human_UserControl_CanChangeJob(human) || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToLearnHouse(human) || Human_IsAttachedToVehicle(human) || Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Human_UserControl_IsPriorityCommandInQueue(human))
				return false;

			for (const auto opcode : { SwitchIds::UserCommand::ChangeJob, SwitchIds::UserCommand::ChangeProducedGood, SwitchIds::UserCommand::ChangeProduction, SwitchIds::UserCommand::AttachWorkHouse, SwitchIds::UserCommand::DetachWorkHouse, SwitchIds::UserCommand::AttachBuildHouse, SwitchIds::UserCommand::AttachLearnHouse, SwitchIds::UserCommand::SetWorkCenter })
			{
				if (Queued(human, opcode))
					return false;
			}

			return true;
		}

		bool IsCareerStep(std::uint32_t tribe, std::uint32_t source, std::uint32_t good, std::uint32_t target)
		{
			if (source == target || target >= SwitchIds::HumanJob::SoldierUnarmed)
				return false;

			std::uint32_t general = 0, specific = 0;
			Tool_Human_GetExperienceType_GainedViaWorking(static_cast<std::int32_t>(source), good, general, specific);
			// Requirement entries are experience IDs, not job IDs (Windows 0042D798/0046224C).
			const auto& requirement = gStaticTribeTypeData[tribe].jobNeeds[target];

			for (std::uint32_t i = 0; i < requirement.jobCount; ++i)
			{
				if ((general != 0 && requirement.jobs[i] == static_cast<std::int32_t>(general)) || (specific != 0 && requirement.jobs[i] == static_cast<std::int32_t>(specific)))
					return true;
			}

			return false;
		}

		bool CanPromoteCraftWorker(const void* human, std::uint32_t job)
		{
			const auto current = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

			if (!IsCareerStep(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), current, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType), job))
				return false;

			if (!Human_IsAttachedToWorkHouse(human) || !Human_UserControl_CanChangeJob(human) || Human_IsAttachedToLearnHouse(human) || Human_IsAttachedToVehicle(human) || Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Human_UserControl_IsPriorityCommandInQueue(human))
				return false;

			for (const auto opcode : { 21u, 22u, 23u, 28u, 29u, 30u, 32u, 17u })
			{
				if (Queued(human, opcode))
					return false;
			}

			const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

			if (id >= static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity))
				return false;

			const auto* house = GetHousePtr(static_cast<std::int32_t>(id));

			return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0;
		}

		void PrepareCareerWorkers(std::uint32_t player)
		{
			const auto tribe = Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType);
			auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
			bool needed[31]{ };
			CHouseIterator houses;

			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				const auto* house = GetHousePtr(id);

				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || !HasVacancy(house))
					continue;

				const auto* type = House_GetHouseTypeDataPtr(house);

				for (std::int32_t slot = 0; slot < type->workerCount; ++slot)
				{
					const auto target = static_cast<std::uint32_t>(type->workers[slot].jobType);

					if (target >= 31 || House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, target) != 0)
						continue;

					for (std::uint32_t source = 8; source < 24; ++source)
					{
						if (IsCareerStep(tribe, source, static_cast<std::uint32_t>(gStaticTribeTypeData[tribe].jobEnables[source].defaultGood), target))
							needed[source] = true;
					}
				}
			}
			// Maintain a small working feeder pool, including pending assignments. Hunters feed
			// both breeding and tailoring; new civilists gain their experience through real work.

			for (std::uint32_t job = 9; job < 24; ++job)
			{
				if (!needed[job])
					continue;

				const auto good = static_cast<std::uint32_t>(gStaticTribeTypeData[tribe].jobEnables[job].defaultGood);

				if (good != 0 && good < 66)
					AIVillagesData_Good_IsProducingGood(static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages, good, true);

				const auto reserve = job == SwitchIds::HumanJob::Hunter ? 3u : 1u;
				std::uint32_t count = 0;
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
				{
					const auto* human = GetHumanPtr(id);

					if (Human_UserControl_GetNextJobType(human) == job && (IsFamilyWorker(human) || Queued(human, SwitchIds::UserCommand::AttachWorkHouse)))
						++count;
				}

				for (auto id = houses.GetFirstHouseId(player); count < reserve && id != -1; id = houses.GetNextHouseId())
				{
					const auto* house = GetHousePtr(id);

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || !House_Inhabitants_IsFreeWorkSlotAvailable(house, job))
						continue;

					const auto* type = House_GetHouseTypeDataPtr(house);
					std::uint32_t capacity = 0;

					for (std::int32_t slot = 0; slot < type->workerCount; ++slot)
					{
						if (static_cast<std::uint32_t>(type->workers[slot].jobType) == job)
							capacity = type->workers[slot].maximumWorkers;
					}

					auto occupied = static_cast<std::uint32_t>(House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job));

					for (auto candidate = humans.GetFirstHumanId(player); candidate != -1; candidate = humans.GetNextHumanId())
					{
						const auto* human = GetHumanPtr(candidate);

						if (!Queued(human, SwitchIds::UserCommand::AttachWorkHouse) || Human_UserControl_GetNextJobType(human) != job)
							continue;

						const auto command = NC2Layout::HumanOffset::UserCommandQueue + (SwitchIds::UserCommand::AttachWorkHouse - 2u) * NC2Layout::RecordSize::UserNetworkCommand;

						if (Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter1) == static_cast<std::uint32_t>(id) && Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter2) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId))
							++occupied;
					}

					while (count < reserve && occupied < capacity)
					{
						// Feeders must not steal another employed specialist to fill their own pool.
						const auto worker = FindWorker(player, House_Door_GetEntryPointRef(house), job, false, true);

						if (worker == -1)
							break;

						auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(worker) * CHumanArray::ElementSize;
						const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);
						Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(player), worker, revision, job, 0);
						Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::AttachWorkHouse, static_cast<std::uint8_t>(player), worker, revision, id, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
						Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeProducedGood, static_cast<std::uint8_t>(player), worker, revision, static_cast<std::uint32_t>(gStaticTribeTypeData[tribe].jobEnables[job].defaultGood), 0);
						++count;
						++occupied;
					}
				}
			}
		}
	}

	bool IsActive(std::uint32_t player)
	{
		if (gAIManager == nullptr || player >= 20)
			return false;

		const auto* program = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;

		if (program != nullptr && program->Remaster_IsMainMenuActive())
			return false;

		const auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);

		return high != nullptr && Read<std::uint8_t>(high, NC2Layout::AIHighLevelOffset::Enabled) != 0;
	}

	std::uint32_t WaveSize(std::uint32_t population)
	{
		// The existing menu and saved game use 0=Normal, 1=Easy, 2=Very easy.
		const auto difficulty = Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel);
		const auto maximum = difficulty == 0 ? 96u : (difficulty == 1 ? 24u : 12u);
		auto size = population / (difficulty == 0 ? 4u : (difficulty == 1 ? 12u : 16u));

		if (size < 4)
			size = 4;

		return size > maximum ? maximum : size;
	}

	std::uint32_t MilitaryLimit()
	{
		const auto difficulty = Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel);

		return difficulty == 0 ? 190u : (difficulty == 1 ? 48u : 24u);
	}

	bool IsFamilyWorker(const void* human)
	{
		if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human))
			return false;

		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		if (job <= SwitchIds::HumanJob::Civilist || job >= SwitchIds::HumanJob::SoldierUnarmed || Human_IsJobIgnoringHomeHouse(human) || Human_UserControl_GetNextJobType(human) != job)
			return false;

		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

			if (id >= static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity))
				return false;

			const auto* house = GetHousePtr(static_cast<std::int32_t>(id));
			return Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0;
		}
		// Field professions work at a flag instead of occupying a workshop.

		return (job == SwitchIds::HumanJob::Collector || job == SwitchIds::HumanJob::Hunter || job == SwitchIds::HumanJob::Fisher) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != 0;
	}

	SWorkforce Inspect(std::uint32_t player)
	{
		SWorkforce result;
		CHumanIterator humans;

		for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
		{
			const auto* human = GetHumanPtr(id);
			++result.population;
			bool recruit = false;

			if (Queued(human, SwitchIds::UserCommand::AttachLearnHouse))
			{
				const auto command = NC2Layout::HumanOffset::UserCommandQueue + (SwitchIds::UserCommand::AttachLearnHouse - 2u) * NC2Layout::RecordSize::UserNetworkCommand;
				const auto target = Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter1);

				if (target < static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mCapacity))
				{
					const auto* house = GetHousePtr(static_cast<std::int32_t>(target));
					recruit = Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == static_cast<std::uint32_t>(EHouseType::Barracks) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter2);
				}
			}

			if (!Human_IsAgeAdult(human))
			{
				++result.children;
			}
			else if (Human_IsJobSoldier(human) || recruit)
			{
				++result.soldiers;
			}
			else if (Human_IsJobWoman(human))
			{
				++result.women;
			}
			else if (Human_IsSexMale(human) && !Human_IsJobHero(human))
			{
				++result.workers;
			}

			if (!recruit && IsFamilyWorker(human))
				++result.familyWorkers;

			if (Human_IsJobBuilder(human) || Queued(human, SwitchIds::UserCommand::AttachBuildHouse))
				++result.builders;
		}

		std::uint32_t supplyCounts[55]{ };
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;

			const auto family = BuildingFamily(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType));
			const bool surplus = IsSupply(family) && ++supplyCounts[family] > 2;
			const auto main = House_GetHouseMainType(house);

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
			{
				// Recruitment waits for every completed workshop, including otherwise optional copies.

				if (HasVacancy(house))
					++result.recruitmentVacancies;

				if (main == 1 && House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, SwitchIds::HumanJob::Carrier) == 0)
					++result.recruitmentVacancies;
			}

			if (main == 1)
				++result.warehouses;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
			{
				// Count physical food once: homes/warehouses share food aliases, bakeries hold bread.

				if (main == 1 || main == 2)
				{
					result.foodStock += House_Stock_GetAmountOfGoods(house, 16, true) + House_Stock_GetAmountOfGoods(house, 17, true);
				}
				else if (family == static_cast<std::uint32_t>(EHouseType::Bakery00))
				{
					result.foodStock += House_Stock_GetAmountOfGoods(house, 19, false);
				}
			}

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
			{
				++result.constructionSites;
			}
			else if (!surplus && HasVacancy(house))
			{
				++result.vacantWorkplaces;

				if (family == static_cast<std::uint32_t>(EHouseType::Well) || family == static_cast<std::uint32_t>(EHouseType::Farm) || family == static_cast<std::uint32_t>(EHouseType::Mill) || family == static_cast<std::uint32_t>(EHouseType::Bakery00))
					++result.foodVacancies;
			}
		}

		return result;
	}

	void StaffVacantWorkplaces(std::uint32_t player)
	{
		PrepareCareerWorkers(player);
		auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;

			const auto* type = House_GetHouseTypeDataPtr(house);

			if (type->mainType != 1 && type->mainType != 3)
				continue;

			for (std::int32_t slot = 0; slot < type->workerCount; ++slot)
			{
				const auto job = static_cast<std::uint32_t>(type->workers[slot].jobType);

				if (job == 0 || job == SwitchIds::HumanJob::Collector || (job == SwitchIds::HumanJob::Carrier && type->workerCount > 1) || House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job) != 0)
					continue;

				bool incoming = false;
				CHumanIterator humans;

				for (auto candidate = humans.GetFirstHumanId(player); candidate != -1; candidate = humans.GetNextHumanId())
				{
					const auto* human = GetHumanPtr(candidate);

					if (!Queued(human, SwitchIds::UserCommand::AttachWorkHouse) || Human_UserControl_GetNextJobType(human) != job)
						continue;

					const auto command = NC2Layout::HumanOffset::UserCommandQueue + (SwitchIds::UserCommand::AttachWorkHouse - 2u) * NC2Layout::RecordSize::UserNetworkCommand;

					if (Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter1) == static_cast<std::uint32_t>(id) && Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter2) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId))
					{
						incoming = true;
						break;
					}
				}

				if (incoming)
					continue;

				std::uint32_t output = 0;

				for (std::uint32_t i = 0; i < type->productionCount; ++i)
				{
					const auto good = static_cast<std::uint32_t>(type->productionGoods[i]);

					if (gStaticGoodTypeStorage.records[good].armorType != 0 || job != Read<std::uint32_t>(high, NC2Layout::AIHighLevelOffset::ResourceGoodRequirements + good * NC2Layout::RecordSize::AIResourceGood + NC2Layout::AIResourceGoodOffset::WorkerJob))
						continue;

					output = good;
					break;
				}
				// Native goods planning still establishes the real input chain, even before a worker qualifies.

				if (output != 0)
					AIVillagesData_Good_IsProducingGood(static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages, output, true);

				const auto worker = FindWorker(player, House_Door_GetEntryPointRef(house), job);

				if (worker == -1)
					continue;

				auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(worker) * CHumanArray::ElementSize;
				const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != job)
					Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(player), worker, revision, job, 0);

				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::AttachWorkHouse, static_cast<std::uint8_t>(player), worker, revision, id, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));

				if (output != 0)
					Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeProducedGood, static_cast<std::uint8_t>(player), worker, revision, output, 0);
			}
		}

		PrepareCareerWorkers(player);
	}

	bool NeedsStarterHome(const void* village)
	{
		const auto player = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId);

		if (!IsActive(player))
			return false;

		if (Read<std::uint8_t>(village, NC2Layout::AIVillageOffset::HouseBuildAllowedFlags + static_cast<std::uint32_t>(EHouseType::Home00)) == 0)
			return false;

		bool farm = false;
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player, static_cast<const std::byte*>(village) + NC2Layout::AIVillageOffset::CenterPoint, Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius)); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;
			// A home under construction already reserves this village's next building.

			if (House_GetHouseMainType(house) == 2)
				return false;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == static_cast<std::uint32_t>(EHouseType::Farm) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
				farm = true;
		}

		return farm;
	}

	bool HasConstructionGoods(const void* village, std::uint32_t engineType, std::uint32_t stage)
	{
		const auto player = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId);

		if (!IsActive(player))
			return false;

		SHouseConstructionGoods goods{ };
		House_Construction_CalculateGoodsNeeded(engineType, stage, false, &goods);

		if (goods.count < 0 || goods.count > 10)
			return false;

		std::uint64_t available[10]{ };
		const auto* center = static_cast<const std::byte*>(village) + NC2Layout::AIVillageOffset::CenterPoint;
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(center), NC2Layout::MapMicroPointOffset::ContinentId);
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player, center, Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius)); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (House_GetHouseMainType(house) != 1 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
				continue;

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(House_Door_GetEntryPointRef(house)), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
				continue;

			for (std::int32_t i = 0; i < goods.count; ++i)
			{
				available[i] += House_Stock_GetAmountOfGoods(house, goods.entries[i].good, true);
			}
		}

		for (std::int32_t i = 0; i < goods.count; ++i)
		{
			if (available[i] < goods.entries[i].requiredAmount)
			{
				RemasterAILog::Event(player, "material_wait", static_cast<std::int32_t>(engineType), "village=%u engine=%u stage=%u good=%u required=%u locally_stored=%llu", Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::VillageId), engineType, stage, goods.entries[i].good, goods.entries[i].requiredAmount, static_cast<unsigned long long>(available[i]));
				return false;
			}
		}

		return true;
	}

	std::uint32_t BuildingFamily(std::uint32_t logic)
	{
		if (logic >= static_cast<std::uint32_t>(EHouseType::Bakery00) && logic <= static_cast<std::uint32_t>(EHouseType::Bakery01))
			return static_cast<std::uint32_t>(EHouseType::Bakery00);

		if (logic >= static_cast<std::uint32_t>(EHouseType::Sewery00) && logic <= static_cast<std::uint32_t>(EHouseType::Sewery01))
			return static_cast<std::uint32_t>(EHouseType::Sewery00);

		if (logic >= static_cast<std::uint32_t>(EHouseType::Pottery00) && logic <= static_cast<std::uint32_t>(EHouseType::Pottery02))
			return static_cast<std::uint32_t>(EHouseType::Pottery00);

		if (logic >= static_cast<std::uint32_t>(EHouseType::Joinery00) && logic <= static_cast<std::uint32_t>(EHouseType::Joinery03))
			return static_cast<std::uint32_t>(EHouseType::Joinery00);

		if (logic == static_cast<std::uint32_t>(EHouseType::Armory01))
			return static_cast<std::uint32_t>(EHouseType::Armory00);

		if (logic == static_cast<std::uint32_t>(EHouseType::Smithy01))
			return static_cast<std::uint32_t>(EHouseType::Smithy00);

		return logic;
	}

	bool CanBuild(std::uint32_t player, std::uint32_t logic)
	{
		if (!IsActive(player))
			return true;

		const auto family = BuildingFamily(logic);
		std::uint32_t count = 0;
		bool vacant = false;
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || BuildingFamily(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType)) != family)
				continue;

			++count;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || HasVacancy(house))
				vacant = true;
		}
		// Count all villages and construction sites together, including upgraded bakeries.

		if (IsSupply(family) && count >= 2)
		{
			RemasterAILog::Event(player, "build_budget", static_cast<std::int32_t>(logic), "reason=supply_building_limit family=%u count=%u limit=2", family, count);
			return false;
		}

		const auto needs = Inspect(player);
		const auto main = gStaticHouseTypeStorage.records[logic].mainType;

		if (main == 1)
		{
			const bool allowed = needs.warehouses < 1 + needs.workers / 24 && needs.constructionSites < 2;

			if (!allowed)
				RemasterAILog::Event(player, "build_budget", static_cast<std::int32_t>(logic), "reason=warehouse_budget warehouses=%u limit=%u sites=%u limit_sites=2 workers=%u", needs.warehouses, 1 + needs.workers / 24, needs.constructionSites, needs.workers);

			return allowed;
		}

		if (main == 3)
		{
			if (vacant || needs.constructionSites >= 2)
			{
				RemasterAILog::Event(player, "build_budget", static_cast<std::int32_t>(logic), "reason=existing_copy_unfinished_or_unstaffed_or_site_limit family=%u count=%u vacant_copy=%u sites=%u", family, count, vacant, needs.constructionSites);
				return false;
			}
			// Establish missing production chains, but staff existing copies before expansion.

			if (count != 0 && (needs.vacantWorkplaces != 0 || needs.workers < 12 + count * 8))
			{
				RemasterAILog::Event(player, "build_budget", static_cast<std::int32_t>(logic), "reason=staff_existing_production family=%u count=%u vacancies=%u workers=%u required_workers=%u", family, count, needs.vacantWorkplaces, needs.workers, 12 + count * 8);
				return false;
			}

			const bool missingWeaponWorkshop = count == 0 && (family == static_cast<std::uint32_t>(EHouseType::Armory00) || family == static_cast<std::uint32_t>(EHouseType::Smithy00));

			if (needs.vacantWorkplaces >= 3 && !IsSupply(family) && !missingWeaponWorkshop)
			{
				RemasterAILog::Event(player, "build_budget", static_cast<std::int32_t>(logic), "reason=too_many_unstaffed_workplaces vacancies=%u", needs.vacantWorkplaces);
				return false;
			}
		}

		return true;
	}

	bool CanExpand(std::uint32_t player)
	{
		const auto needs = Inspect(player);
		const bool allowed = needs.vacantWorkplaces == 0 && needs.constructionSites == 0 && needs.warehouses < 1 + needs.workers / 24 && needs.soldiers >= 2;
		RemasterAILog::Event(player, "expansion_budget", -1, "allowed=%u vacancies=%u sites=%u warehouses=%u limit=%u soldiers=%u required_soldiers=2", allowed, needs.vacantWorkplaces, needs.constructionSites, needs.warehouses, 1 + needs.workers / 24, needs.soldiers);

		return allowed;
	}

	bool IsSurplusSupplyHouse(const void* house)
	{
		const auto family = BuildingFamily(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType));

		if (!IsSupply(family))
			return false;

		std::uint32_t count = 0;
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)); id != -1; id = houses.GetNextHouseId())
		{
			const auto* candidate = GetHousePtr(id);

			if (Read<std::uint8_t>(candidate, NC2Layout::HouseOffset::DemolitionFlag) != 0 || BuildingFamily(Read<std::uint32_t>(candidate, NC2Layout::HouseOffset::LogicType)) != family)
				continue;

			++count;

			if (candidate == house)
				return count > 2;
		}

		return false;
	}

	void EnsureBuilder(std::uint32_t player)
	{
		CHumanIterator humans;

		for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
		{
			const auto* human = GetHumanPtr(id);

			if (Human_IsJobBuilder(human))
				return;

			const auto command = NC2Layout::HumanOffset::UserCommandQueue + (SwitchIds::UserCommand::ChangeJob - 2u) * NC2Layout::RecordSize::UserNetworkCommand;

			if (Queued(human, SwitchIds::UserCommand::ChangeJob) && Read<std::uint32_t>(human, command + NC2Layout::UserNetworkCommandOffset::Parameter1) == SwitchIds::HumanJob::Builder)
				return;
		}

		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			const auto* house = GetHousePtr(id);

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;

			const auto worker = FindWorker(player, House_Door_GetEntryPointRef(house), SwitchIds::HumanJob::Builder);

			if (worker == -1)
				continue;

			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(worker) * CHumanArray::ElementSize;
			Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(player), worker, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), SwitchIds::HumanJob::Builder, 0);

			return;
		}
	}

	bool IsWorkerCommitted(const void* human, bool allowBuilder)
	{

		return (!allowBuilder && Human_IsJobBuilder(human)) || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToLearnHouse(human) || Human_IsAttachedToVehicle(human) || Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human) || Human_UserControl_IsPriorityCommandInQueue(human) || Queued(human, SwitchIds::UserCommand::ChangeJob) || Queued(human, SwitchIds::UserCommand::AttachWorkHouse) || Queued(human, SwitchIds::UserCommand::AttachBuildHouse) || Queued(human, SwitchIds::UserCommand::AttachLearnHouse) || Remaster_AIGoods_IsFieldLogisticsHuman(human);
	}

	std::int32_t FindWorker(std::uint32_t player, const void* point, std::uint32_t job, bool civilistsOnly, bool uncommittedOnly)
	{
		auto* high = Read<void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		const auto* single = Read<void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
		std::int32_t best = -1;
		std::uint32_t distance = UINT32_MAX;
		std::uint32_t bestRank = UINT32_MAX;
		std::uint32_t examined = 0, rejectedBuilder = 0, rejectedCivilist = 0, rejectedDemographic = 0, rejectedQualification = 0, rejectedCommitted = 0, rejectedReserved = 0, rejectedContinent = 0;
		CHumanIterator humans;

		for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
		{
			++examined;
			const auto* human = GetHumanPtr(id);
			// Native soldier jobs ignore the home and prevent their spouse's next pregnancy.

			if (job != SwitchIds::HumanJob::Builder && Human_IsJobBuilder(human))
			{
				++rejectedBuilder;
				continue;
			}

			if (civilistsOnly && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != SwitchIds::HumanJob::Civilist || Human_IsMarried(human) || Human_IsAboutToMarry(human)))
			{
				++rejectedCivilist;
				continue;
			}

			if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human) || Human_IsJobHero(human) || Human_IsJobSoldier(human) || Human_IsJobScout(human) || Human_IsJobTrader(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 32u) != 0)
			{
				++rejectedDemographic;
				continue;
			}

			const bool promotion = job > SwitchIds::HumanJob::Collector && job < SwitchIds::HumanJob::Carrier && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == SwitchIds::HumanJob::Collector && !Human_IsAttachedToWorkHouse(human) && !Human_UserControl_IsPriorityCommandInQueue(human) && !Queued(human, SwitchIds::UserCommand::ChangeJob) && !Queued(human, SwitchIds::UserCommand::ChangeProducedGood);

			if ((job != 0 && !Human_IsAbleToDoJob(human, job)) || AISinglePlayerHandler_Misc_Human_CheckRequestsForCreature(single, id))
			{
				++rejectedQualification;
				continue;
			}

			const bool carrierPromotion = !uncommittedOnly && CanPromoteWarehouseCarrier(human, job);
			const bool craftPromotion = !uncommittedOnly && job != 0 && CanPromoteCraftWorker(human, job);
			const bool builderRecovery = !uncommittedOnly && job == SwitchIds::HumanJob::Builder && CanRecoverCollectorAsBuilder(human);
			// Native IsAttachedToBuildSite also returns true for ordinary work-house attachments.

			if ((!carrierPromotion && !craftPromotion && Human_IsAttachedToBuildSite(human)) || (!promotion && !carrierPromotion && !craftPromotion && !builderRecovery && IsWorkerCommitted(human, job == SwitchIds::HumanJob::Builder)))
			{
				++rejectedCommitted;
				continue;
			}

			std::uint32_t target[3]{ 1, static_cast<std::uint32_t>(id), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) };

			if (!promotion && !carrierPromotion && !craftPromotion && !builderRecovery && AIHighLevelHandler_IsObjectUsed(high, target))
			{
				++rejectedReserved;
				continue;
			}

			const auto* position = Human_GetMapPositionRef(human);

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId) != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId))
			{
				++rejectedContinent;
				continue;
			}
			// Keep the wood supply when another raw-material collector can restore construction.
			const auto rank = carrierPromotion ? 3u : craftPromotion ? 2u
			                                      : promotion        ? 1u
			                                      : builderRecovery  ? (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 5 ? 5u : 4u)
			                                                         : 0u;
			const auto candidate = Distance(position, point);

			if (rank < bestRank || (rank == bestRank && candidate < distance))
			{
				best = id;
				bestRank = rank;
				distance = candidate;
			}
		}

		if (best != -1 && bestRank != 0)
		{
			// Release the promoted worker's AI reservation; native job commands detach the
			// old workplace and the normal goods/feeder planners replace its worker.

			for (std::uint32_t i = 0; i < 8; ++i)
			{
				auto* slot = static_cast<std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages + i * NC2Layout::RecordSize::AIVillageSlot;

				if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) == 0)
					continue;

				for (std::uint32_t good = 0; good < 66; ++good)
				{
					auto* entry = slot + NC2Layout::AIVillageSlotViewOffset::Goods + good * NC2Layout::RecordSize::AIGood;

					if (Read<std::int32_t>(entry, NC2Layout::AIGoodOffset::Producer) != best)
						continue;

					Write<std::int32_t>(entry, NC2Layout::AIGoodOffset::Producer, -1);
					Write<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag, 0);
					Write<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProductionConfirmationCount, 0);
				}
			}
		}

		RemasterAILog::Event(player, "worker_search", static_cast<std::int32_t>(job), "job=%u target=(%d,%d) civilists_only=%u uncommitted_only=%u chosen=%d rank=%u reason=%s", job, Read<std::int16_t>(point, 0), Read<std::int16_t>(point, 2), civilistsOnly, uncommittedOnly, best, bestRank, best == -1 ? "no_eligible_unreserved_worker_on_same_continent" : "qualified_worker_selected");
		RemasterAILog::Event(player, "worker_candidates", static_cast<std::int32_t>(job), "job=%u examined=%u rejected_builder=%u rejected_not_free_civilist=%u rejected_age_sex_or_excluded_role=%u rejected_qualification_or_pending_request=%u rejected_committed=%u rejected_reserved_by_ai=%u rejected_other_continent=%u", job, examined, rejectedBuilder, rejectedCivilist, rejectedDemographic, rejectedQualification, rejectedCommitted, rejectedReserved, rejectedContinent);

		return best;
	}

	std::uint32_t BuilderLimit(std::uint32_t player)
	{
		// Remaster: small settlements need their remaining workers for food and materials.

		return Inspect(player).workers < 12 ? 1u : 4u;
	}

	void ReleaseExcessBuilders(std::uint32_t player)
	{
		const auto limit = BuilderLimit(player);
		// Retain active construction first, then one idle reserve if no builder is assigned.
		std::uint32_t retained = 0, siteCount = 0, sites[4]{ }, counts[4]{ };
		CHumanIterator humans;

		for (std::uint32_t pass = 0; pass < 2; ++pass)
		{
			for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
			{
				auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;

				if (!Human_IsJobBuilder(human) || Human_UserControl_GetNextJobType(human) != SwitchIds::HumanJob::Builder)
					continue;

				const bool attached = Human_IsAttachedToBuildSite(human);

				if (attached != (pass == 0))
					continue;
				// Preserve road/wall assignments and commands already accepted by native logic.

				if (Human_IsAttachedToRoadPoint(human) || Human_IsAttachedToWallPoint(human) || Human_IsAttachedToLearnHouse(human) || Human_IsAttachedToVehicle(human) || Queued(human, SwitchIds::UserCommand::AttachBuildHouse) || Queued(human, SwitchIds::UserCommand::AttachWorkHouse) || Queued(human, SwitchIds::UserCommand::SetWorkCenter))
				{
					++retained;
					continue;
				}

				const auto houseId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);
				std::uint32_t site = 0;

				while (site < siteCount && sites[site] != houseId)
					++site;

				if (retained < limit && (!attached || site == siteCount || counts[site] < 2))
				{
					if (attached)
					{
						if (site == siteCount)
							sites[siteCount++] = houseId;

						++counts[site];
					}

					++retained;
					continue;
				}

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 || Human_UserControl_IsPriorityCommandInQueue(human))
					continue;

				RemasterAILog::Event(player, "builder_released", id, "reason=construction_workforce_budget builders_retained=%u limit=%u attached=%u next_job=civilist", retained, limit, attached);
				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::ChangeJob, static_cast<std::uint8_t>(player), id, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), SwitchIds::HumanJob::Civilist, 0);
			}
		}
	}

	std::int32_t UpdateHomes(void* village)
	{
		const auto player = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::PlayerId);
		const auto* center = static_cast<const std::byte*>(village) + NC2Layout::AIVillageOffset::CenterPoint;
		const auto radius = Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius);
		bool freeHome = false;
		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player, center, radius); id != -1; id = houses.GetNextHouseId())
		{
			auto* house = GetHousePtr(id);

			if (House_GetHouseMainType(house) != 2 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
			{
				freeHome = true;
				continue;
			}

			if (House_Inhabitants_GetNumberOfFamiliesLivingInHome(house, nullptr) >= static_cast<std::uint32_t>(House_GetHouseTypeDataPtr(house)->homeSize))
				continue;

			freeHome = true;
			CHumanIterator humans;

			for (auto humanId = humans.GetFirstSectorHumanId(player, Read<std::uint32_t>(center, 0), radius, false); humanId != -1; humanId = humans.GetNextSectorHumanId())
			{
				auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanId) * CHumanArray::ElementSize;

				if (!Human_IsAgeAdult(human) || !Human_IsSexMale(human) || Human_IsJobHero(human) || Human_IsAttachedToHomeHouse(human) || Human_UserControl_IsPriorityCommandInQueue(human) || Queued(human, SwitchIds::UserCommand::AttachHomeHouse))
					continue;

				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::AttachHomeHouse, static_cast<std::uint8_t>(player), humanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), id, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				return -2;
			}
		}

		return freeHome ? 1 : 0;
	}
}