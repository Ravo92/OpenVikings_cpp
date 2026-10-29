#include "C2DataIds.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionScriptData.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00449c66.
	std::int32_t MissionCheck_GetFirstAnimalIdWithMissionCheckId(std::uint32_t missionCheck)
	{
		CAnimalIterator iterator;

		for (auto id = iterator.GetFirstAnimalId(); id != -1; id = iterator.GetNextAnimalId())
		{
			if (Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449caa; retain the live upper bound and allocated-byte filter.
	std::int32_t MissionCheck_GetNextAnimalIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous)
	{
		if (previous == -1)
			return -1;

		for (auto id = previous + 1; id <= CAnimalArray::sTheObjectPtr->mLastScanIndex; ++id)
		{
			const void* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize;

			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449cf1.
	std::int32_t MissionCheck_GetNumberOfAnimalsWithMissionCheckId(std::uint32_t missionCheck)
	{
		CAnimalIterator iterator;
		std::int32_t count = 0;

		for (auto id = iterator.GetFirstAnimalId(); id != -1; id = iterator.GetNextAnimalId())
		{
			if (Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::MissionCheckId) == missionCheck)
				++count;
		}

		return count;
	}

	// Original function: FUN_0040aeda; the player comparison is signed.
	bool Logic_EntryPoint_IsPlayerInGame(std::int32_t player)
	{

		return player < NC2Runtime::MissionStartLimits::Players && Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) != 0;
	}

	// Original function: FUN_00409d1b.
	static bool GameControl_IsPlayerNotInMultiPlayerGame(const void* control, std::int32_t player)
	{
		if (gGameStaticVars.networkGame == 0)
			return false;

		return player >= NC2Runtime::MissionStartLimits::Players || Read<std::uint8_t>(control, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0;
	}

	// Original function: FUN_0040aef6.
	bool Logic_EntryPoint_IsPlayerNotInMultiPlayerGame(std::int32_t player)
	{

		return GameControl_IsPlayerNotInMultiPlayerGame(gGameControl, player);
	}

	// Original function: FUN_0040ae7e.
	std::int32_t Logic_EntryPoint_House_GetOnPosition(const void* point)
	{
		const void* micro = GameMap_GetMapMicroPointPtr(point);

		if (micro == nullptr || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::House)
			return -1;

		return Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
	}

	// Original function: FUN_0040ae9a.
	void Logic_EntryPoint_Guide_Set(std::uint32_t player, const void* point, std::uint32_t flags)
	{
		GuideManager_InsertGuideAtPosition(gGuideManager, nullptr, player, point, flags);
	}

	// Original function: FUN_0042c54e.
	bool Human_IsSexFemale(const void* human)
	{
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		return job == SwitchIds::HumanJob::BabyFemale || job == SwitchIds::HumanJob::ChildFemale || job == SwitchIds::HumanJob::Woman || (job == SwitchIds::HumanJob::HeroineBowXENA && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3);
	}

	// Original function: FUN_0042ca07.
	bool Human_IsAbleToMarry(const void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		if (job >= SwitchIds::HumanJob::BabyFemale && job <= SwitchIds::HumanJob::ChildMale)
			return false;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId) != NXBasics::IntegerRange::UInt32Maximum || Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId) != NXBasics::IntegerRange::UInt32Maximum)
			return false;

		return job != SwitchIds::HumanJob::Scout && !(job >= SwitchIds::HumanJob::SoldierUnarmed && job <= SwitchIds::HumanJob::SoldierBowLong) && !(job >= SwitchIds::HumanJob::HeroUnarmed && job <= SwitchIds::HumanJob::HeroineBowXENA);
	}

	// Original function: FUN_0042ca95; shared with human message validation.
	bool Human_IsMarried(const void* human)
	{

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0042caa2.
	bool Human_IsHavingChild(const void* human)
	{

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0044b9c9; the good index is unchecked.
	void Human_Production_SetCounter(void* human, std::uint32_t good, std::uint32_t counter)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanProductionCounterUnsigned, counter);
	}

	// Original function: FUN_00449ab8.
	std::int32_t MissionCheck_GetFirstHumanIdWithMissionCheckId(std::uint32_t missionCheck)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(); id != -1; id = iterator.GetNextHumanId())
		{
			if (Read<std::uint32_t>(GetHumanPtr(id), NC2Layout::HumanOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449afc.
	std::int32_t MissionCheck_GetNextHumanIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous)
	{
		if (previous == -1)
			return -1;

		for (auto id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u); id <= CHumanArray::sTheObjectPtr->mLastScanIndex; id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(id) + 1u))
		{
			const auto* human = GetHumanPtr(id);

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449b43.
	std::int32_t MissionCheck_GetFirstVehicleIdWithMissionCheckId(std::uint32_t missionCheck)
	{
		// The unfiltered iterator at 00440CCC/00440E19 checks allocation only.

		for (auto id = CVehicleArray::sTheObjectPtr->mFirstScanIndex; id <= CVehicleArray::sTheObjectPtr->mLastScanIndex; id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(id) + 1u))
		{
			const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;

			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449b87.
	std::int32_t MissionCheck_GetNextVehicleIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous)
	{
		if (previous == -1)
			return -1;

		for (auto id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u); id <= CVehicleArray::sTheObjectPtr->mLastScanIndex; id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(id) + 1u))
		{
			const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;

			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449bce.
	std::int32_t MissionCheck_GetFirstHouseIdWithMissionCheckId(std::uint32_t missionCheck)
	{
		CHouseIterator iterator;

		for (auto id = iterator.GetFirstHouseId(); id != -1; id = iterator.GetNextHouseId())
		{
			if (Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize, NC2Layout::HouseOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	// Original function: FUN_00449c12.
	std::int32_t MissionCheck_GetNextHouseIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous)
	{
		if (previous == -1)
			return -1;

		for (auto id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u); id <= CHouseArray::sTheObjectPtr->mLastScanIndex; id = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(id) + 1u))
		{
			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::MissionCheckId) == missionCheck)
				return id;
		}

		return -1;
	}

	namespace
	{
		void* Human(std::uint32_t id)
		{

			return const_cast<void*>(GetHumanPtr(static_cast<std::int32_t>(id)));
		}

		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}

		void* Vehicle(std::uint32_t id)
		{

			return CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize;
		}

		void ReadPosition(NXBasics::CIniFileReader& reader, std::int16_t (&point)[2])
		{
			point[0] = static_cast<std::int16_t>(reader.GetInt());
			point[1] = static_cast<std::int16_t>(reader.GetInt());
		}

		std::uint32_t ReadGood(NXBasics::CIniFileReader& reader)
		{

			return reader.GetTypeOfNextParameter() == SwitchIds::IniParameterType::String ? Logic_EntryPoint_GetGoodType(reader.GetString()) : static_cast<std::uint32_t>(reader.GetInt());
		}

		void AssignHome(void* human, std::uint32_t home)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId, home);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(House(home), NC2Layout::HouseOffset::UniqueId));
			House_Inhabitants_AttachHuman(House(home), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), SwitchIds::HouseAttachmentMask::HomeUnsigned);
		}

		void JoinFamilyHome(void* woman, void* man)
		{
			const auto manHome = Read<std::uint32_t>(man, NC2Layout::HumanOffset::HomeHouseId);
			const auto womanHome = Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HomeHouseId);

			if (manHome == NXBasics::IntegerRange::UInt32Maximum)
			{
				AssignHome(man, womanHome);
			}
			else
			{
				const auto work = Read<std::uint32_t>(man, NC2Layout::HumanOffset::WorkAttachmentHouseId);
				bool useWomanHome = false;

				if (work != NXBasics::IntegerRange::UInt32Maximum)
				{
					const auto* workHouse = House(work);
					const auto* firstHome = House(womanHome);
					const auto* secondHome = House(manHome);
					const auto firstDistance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(workHouse, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(workHouse, NC2Layout::HouseOffset::MapPositionY), Read<std::int16_t>(firstHome, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(firstHome, NC2Layout::HouseOffset::MapPositionY)));
					const auto secondDistance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(workHouse, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(workHouse, NC2Layout::HouseOffset::MapPositionY), Read<std::int16_t>(secondHome, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(secondHome, NC2Layout::HouseOffset::MapPositionY)));
					useWomanHome = firstDistance < secondDistance;
				}

				void* moving = useWomanHome ? man : woman;
				House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(moving, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(moving, NC2Layout::HumanOffset::HumanId), SwitchIds::HouseAttachmentMask::HomeUnsigned);
				AssignHome(moving, useWomanHome ? womanHome : manHome);
			}
		}

		void* FindFamilyHuman(NXBasics::CIniFileReader& reader, bool woman, bool child)
		{
			std::int16_t point[2];
			ReadPosition(reader, point);

			if (!MapMigPoint_IsInsideMap(point))
				return nullptr;

			CMapMoveableIterator iterator;

			for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
			{
				const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

				if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != SwitchIds::MapMoveableType::Human)
					continue;

				void* human = Human(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));

				if (child)
				{
					if (!Human_IsAgeAdult(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::MotherId) == NXBasics::IntegerRange::UInt32Maximum)
						return human;
				}
				else if ((woman ? Human_IsSexFemale(human) : Human_IsSexMale(human)) && Human_IsAbleToMarry(human))
				{
					return human;
				}
			}

			return nullptr;
		}
	}

	// Original function: FUN_0041d09b; preserve mission-data loading before object creation.
	void __fastcall MissionManager_LoadMissionScript(void* manager, void* /*unusedEdx*/, NXBasics::CIniFileReader* reader)
	{
		MissionManager_l_LoadMissionData(manager, nullptr, reader);
		MissionManager_l_LoadObjectData(manager, nullptr, reader);
	}

	// Original function: FUN_0041d24b; Windows command dispatch, offsets and read order.
	void __fastcall MissionManager_l_LoadObjectData(void* /*manager*/, void* /*unusedEdx*/, NXBasics::CIniFileReader* reader)
	{
		// Complete Windows region 004F38F8..004F3977, including two unused tokens.
		static constexpr NXBasics::SToken sObjectTokenArray[] = { { "sethuman", SwitchIds::MissionStartToken::SetHuman }, { "setvehicle", SwitchIds::MissionStartToken::SetVehicle }, { "sethouse", SwitchIds::MissionStartToken::SetHouse }, { "setanimal", SwitchIds::MissionStartToken::SetAnimal }, { "setguide", SwitchIds::MissionStartToken::SetGuide }, { "addgoods", SwitchIds::MissionStartToken::AddGoods }, { "attachtohouse", SwitchIds::MissionStartToken::AttachToHouse }, { "setproducedgood", SwitchIds::MissionStartToken::SetProducedGood }, { "attachtovehicle", SwitchIds::MissionStartToken::AttachToVehicle }, { "moveintovehicle", SwitchIds::MissionStartToken::MoveInToVehicle }, { "setexpierence", SwitchIds::MissionStartToken::SetExperience }, { "allowjob", NC2DataIds::MissionStartObjectsToken::AllowJob }, { "forbidjob", NC2DataIds::MissionStartObjectsToken::ForbidJob }, { "marry", SwitchIds::MissionStartToken::Marry }, { "childOfWoman", SwitchIds::MissionStartToken::ChildOfWoman }, { nullptr, 0 } };

		for (auto* group = reader->GetFirstGroup("StaticObjects"); group != nullptr; group = reader->GetNextGroup("StaticObjects"))
		{
			std::uint32_t currentObjectKind = SwitchIds::MissionStartObjectKind::None, currentObjectId = 0, currentObjectRevision = 0;

			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				const auto token = NXBasics::XB_Token_FindTokenInArray(sObjectTokenArray, reader->GetCurrentName());

				switch (token)
				{
					case SwitchIds::MissionStartToken::SetHuman:
					case SwitchIds::MissionStartToken::SetVehicle:
					case SwitchIds::MissionStartToken::SetAnimal:
					{
						currentObjectKind = SwitchIds::MissionStartObjectKind::None;
						const auto player = reader->GetInt();

						if (token != SwitchIds::MissionStartToken::SetAnimal || player < NC2Runtime::MissionStartLimits::Players)
						{
							if (Logic_EntryPoint_IsPlayerNotInMultiPlayerGame(player) || !Logic_EntryPoint_IsPlayerInGame(player))
								break;
						}

						const auto tribe = Logic_EntryPoint_GetTribeType(reader->GetString());
						const auto type = token == SwitchIds::MissionStartToken::SetVehicle ? Logic_EntryPoint_GetVehicleType(reader->GetString()) : Logic_EntryPoint_GetJobType(reader->GetString());
						std::int16_t point[2];
						ReadPosition(*reader, point);
						const auto missionCheck = static_cast<std::uint32_t>(reader->GetInt());
						std::int32_t id;

						if (token == SwitchIds::MissionStartToken::SetVehicle)
						{
							id = Logic_EntryPoint_Add_Vehicle(player, tribe, type, point, missionCheck);
						}
						else
						{
							const auto flags = static_cast<std::uint32_t>(reader->GetInt());
							id = token == SwitchIds::MissionStartToken::SetHuman ? Logic_EntryPoint_Add_Human(player, tribe, type, point, missionCheck, flags) : Logic_EntryPoint_Add_Animal(player, tribe, type, point, missionCheck, flags);
						}

						if (id == -1)
							break;

						currentObjectId = static_cast<std::uint32_t>(id);
						currentObjectKind = token == SwitchIds::MissionStartToken::SetHuman ? SwitchIds::MissionStartObjectKind::Human : token == SwitchIds::MissionStartToken::SetVehicle ? SwitchIds::MissionStartObjectKind::Vehicle
						                                                                                                                                                                   : SwitchIds::MissionStartObjectKind::Animal;
						const void* object = currentObjectKind == SwitchIds::MissionStartObjectKind::Human ? Human(currentObjectId) : currentObjectKind == SwitchIds::MissionStartObjectKind::Vehicle ? Vehicle(currentObjectId)
						                                                                                                                                                                              : GetAnimalPtr(id);
						currentObjectRevision = Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId);

						if (currentObjectKind == SwitchIds::MissionStartObjectKind::Human && type >= SwitchIds::HumanJob::BabyFemale && type <= SwitchIds::HumanJob::ChildMale)
							Write<std::uint32_t>(Human(currentObjectId), NC2Layout::HumanOffset::AgeCounter, Game_Random_GetNumber() % NC2Runtime::MissionStartLimits::AgeRandomRangeUnsigned + (type <= SwitchIds::HumanJob::BabyMale ? 0u : NC2Runtime::MissionStartLimits::ChildAgeBaseUnsigned));

						break;
					}
					case SwitchIds::MissionStartToken::SetHouse:
					{
						currentObjectKind = SwitchIds::MissionStartObjectKind::None;
						const auto player = reader->GetInt();

						if (Logic_EntryPoint_IsPlayerNotInMultiPlayerGame(player) || !Logic_EntryPoint_IsPlayerInGame(player))
							break;

						const auto engineType = NC2LogicExternals::External_HouseType_GetEngineId(reader->GetString());
						const auto stage = reader->GetInt();
						const bool completed = reader->GetInt() != 0;
						std::int16_t point[2];
						ReadPosition(*reader, point);
						const auto missionCheck = reader->GetInt();
						const auto id = Logic_EntryPoint_House_Set(player, engineType, stage, point, completed, missionCheck);

						if (id != -1)
						{
							currentObjectId = id;
							currentObjectRevision = Read<std::uint32_t>(House(currentObjectId), NC2Layout::HouseOffset::UniqueId);
							currentObjectKind = SwitchIds::MissionStartObjectKind::House;
						}

						break;
					}
					case SwitchIds::MissionStartToken::SetGuide:
					{
						currentObjectKind = SwitchIds::MissionStartObjectKind::None;
						const auto player = reader->GetInt();

						if (Logic_EntryPoint_IsPlayerNotInMultiPlayerGame(player) || !Logic_EntryPoint_IsPlayerInGame(player))
							break;

						std::int16_t point[2];
						ReadPosition(*reader, point);
						Logic_EntryPoint_Guide_Set(player, point, 0);
						break;
					}
					case SwitchIds::MissionStartToken::AddGoods:
					{
						if ((currentObjectKind != SwitchIds::MissionStartObjectKind::House && currentObjectKind != SwitchIds::MissionStartObjectKind::Vehicle) || currentObjectId == NXBasics::IntegerRange::UInt32Maximum)
							break;

						const auto good = ReadGood(*reader);
						const auto amount = reader->GetInt();

						if (currentObjectKind == SwitchIds::MissionStartObjectKind::House)
						{
							House_Stock_ModifyAmount(House(currentObjectId), nullptr, good, amount, true);
						}
						else
						{
							Vehicle_Stock_ModifyFutureAmount(Vehicle(currentObjectId), good, amount, NXBasics::IntegerRange::UInt32Maximum);
							Vehicle_Stock_ModifyAmount(Vehicle(currentObjectId), good, amount, NXBasics::IntegerRange::UInt32Maximum);
						}

						break;
					}
					case SwitchIds::MissionStartToken::AttachToHouse:
					{
						if (currentObjectKind != SwitchIds::MissionStartObjectKind::Human || currentObjectId == NXBasics::IntegerRange::UInt32Maximum)
							break;

						std::int16_t point[2];
						ReadPosition(*reader, point);
						const auto house = Logic_EntryPoint_House_GetOnPosition(point);

						if (house == -1)
							break;

						switch (reader->GetInt())
						{
							case SwitchIds::HouseAttachmentMask::Home:
								Human_DoExecuteUserCommand_AttachHomeHouse(Human(currentObjectId), house, Read<std::uint32_t>(House(house), NC2Layout::HouseOffset::UniqueId));
								break;
							case SwitchIds::HouseAttachmentMask::Worker:
								Human_DoExecuteUserCommand_AttachWorkHouse(Human(currentObjectId), house, Read<std::uint32_t>(House(house), NC2Layout::HouseOffset::UniqueId));
								break;
							case SwitchIds::HouseAttachmentMask::Builder:
								Human_DoExecuteUserCommand_AttachBuildHouse(Human(currentObjectId), house, Read<std::uint32_t>(House(house), NC2Layout::HouseOffset::UniqueId));
								break;
						}

						break;
					}
					case SwitchIds::MissionStartToken::AttachToVehicle:
					{
						if (currentObjectKind != SwitchIds::MissionStartObjectKind::Human || currentObjectId == NXBasics::IntegerRange::UInt32Maximum)
							break;

						std::int16_t point[2];
						ReadPosition(*reader, point);

						if (!MapMigPoint_IsInsideMap(point))
							break;

						CMapMoveableIterator iterator;

						for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
						{
							const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

							if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != SwitchIds::MapMoveableType::Vehicle)
								continue;

							const auto vehicleId = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);

							if (!Vehicle_Passengers_AttachHuman(Vehicle(vehicleId), Read<std::uint32_t>(Human(currentObjectId), NC2Layout::HumanOffset::HumanId)))
								continue;

							Write<std::uint32_t>(Human(currentObjectId), NC2Layout::HumanOffset::AttachedVehicleId, vehicleId);
							Write<std::uint32_t>(Human(currentObjectId), NC2Layout::HumanOffset::AttachedVehicleRevision, Read<std::uint32_t>(Vehicle(vehicleId), NC2Layout::VehicleOffset::UniqueId));
							break;
						}

						break;
					}
					case SwitchIds::MissionStartToken::MoveInToVehicle:
						if (currentObjectKind == SwitchIds::MissionStartObjectKind::Human && currentObjectId != NXBasics::IntegerRange::UInt32Maximum)
							Human_VehicleMisc_Enter(Human(currentObjectId));

						break;
					case SwitchIds::MissionStartToken::SetProducedGood:
					{
						if (currentObjectKind != SwitchIds::MissionStartObjectKind::Human || currentObjectId == NXBasics::IntegerRange::UInt32Maximum)
							break;

						auto good = ReadGood(*reader);

						if (good >= NC2Runtime::MissionStartLimits::GoodsUnsigned)
							good = 0;

						void* human = Human(currentObjectId);

						if (good == 65 || Human_IsAbleToProduceGoodNow(human, good))
						{
							Human_Production_SetCounter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType), 0);
							Human_Production_SetCounter(human, good, NC2Runtime::ProductionCounter::Infinite);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex, good);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType, good);
						}
						else
						{
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionRotationIndex, 0);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType, 0);
						}

						Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
						break;
					}
					case SwitchIds::MissionStartToken::SetExperience:
					{
						if (currentObjectKind != SwitchIds::MissionStartObjectKind::Human || currentObjectId == NXBasics::IntegerRange::UInt32Maximum || Human_IsJobHero(Human(currentObjectId)))
							break;

						const auto job = static_cast<std::uint32_t>(reader->GetInt());
						const auto amount = static_cast<std::uint32_t>(reader->GetInt());
						const auto offset = NC2Layout::HumanOffset::JobExperience + job * NC2Layout::RecordSize::HumanJobExperienceUnsigned;
						const auto oldAmount = Read<std::uint32_t>(Human(currentObjectId), offset);
						Write<std::uint32_t>(Human(currentObjectId), offset, oldAmount > NC2Runtime::HumanExperienceLimits::MaximumUnsigned - amount ? NC2Runtime::HumanExperienceLimits::MaximumUnsigned : oldAmount + amount);
						Human_UpdateCurrentJobType(Human(currentObjectId));
						break;
					}
					case SwitchIds::MissionStartToken::Marry:
					{
						void* woman = FindFamilyHuman(*reader, true, false);

						if (woman == nullptr)
							break;

						void* man = FindFamilyHuman(*reader, false, false);

						if (man == nullptr)
							break;
						// Windows writes and clears each waiting reservation before publishing the spouse ID.
						Write<std::uint32_t>(woman, NC2Layout::HumanOffset::WaitingHumanId, Read<std::uint32_t>(man, NC2Layout::HumanOffset::HumanId));
						Write<std::uint32_t>(woman, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
						Write<std::uint32_t>(woman, NC2Layout::HumanOffset::SpouseId, Read<std::uint32_t>(man, NC2Layout::HumanOffset::HumanId));
						Write<std::uint32_t>(man, NC2Layout::HumanOffset::WaitingHumanId, Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HumanId));
						Write<std::uint32_t>(man, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
						Write<std::uint32_t>(man, NC2Layout::HumanOffset::SpouseId, Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HumanId));

						if (Human_IsHavingChild(woman))
						{
							const auto child = Read<std::uint32_t>(woman, NC2Layout::HumanOffset::ChildId);
							Write<std::uint32_t>(man, NC2Layout::HumanOffset::ChildId, child);
							Write<std::uint32_t>(Human(child), NC2Layout::HumanOffset::FatherId, Read<std::uint32_t>(man, NC2Layout::HumanOffset::HumanId));
						}

						if (!Human_IsAttachedToHomeHouse(woman))
							break;

						if (Human_IsHavingChild(woman))
						{
							const auto child = Read<std::uint32_t>(woman, NC2Layout::HumanOffset::ChildId);
							House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(Human(child), NC2Layout::HumanOffset::HomeHouseId)), child, SwitchIds::HouseAttachmentMask::HomeUnsigned);
						}

						JoinFamilyHome(woman, man);

						if (Human_IsHavingChild(woman))
							AssignHome(Human(Read<std::uint32_t>(woman, NC2Layout::HumanOffset::ChildId)), Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HomeHouseId));

						break;
					}
					case SwitchIds::MissionStartToken::ChildOfWoman:
					{
						void* child = FindFamilyHuman(*reader, false, true);

						if (child == nullptr)
							break;

						std::int16_t point[2];
						ReadPosition(*reader, point);

						if (!MapMigPoint_IsInsideMap(point))
							break;

						CMapMoveableIterator iterator;
						void* woman = nullptr;

						for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
						{
							const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

							if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != SwitchIds::MapMoveableType::Human)
								continue;

							void* candidate = Human(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));

							if (Human_IsSexFemale(candidate) && !Human_IsHavingChild(candidate))
							{
								woman = candidate;
								break;
							}
						}

						if (woman == nullptr)
							break;

						Write<std::uint32_t>(child, NC2Layout::HumanOffset::MotherId, Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HumanId));
						Write<std::uint32_t>(woman, NC2Layout::HumanOffset::ChildId, Read<std::uint32_t>(child, NC2Layout::HumanOffset::HumanId));

						if (Human_IsMarried(woman))
						{
							const auto father = Read<std::uint32_t>(woman, NC2Layout::HumanOffset::SpouseId);
							Write<std::uint32_t>(child, NC2Layout::HumanOffset::FatherId, father);
							Write<std::uint32_t>(Human(father), NC2Layout::HumanOffset::ChildId, Read<std::uint32_t>(child, NC2Layout::HumanOffset::HumanId));
						}

						if (!Human_IsAttachedToHomeHouse(woman))
							break;

						if (Human_IsMarried(woman))
							JoinFamilyHome(woman, Human(Read<std::uint32_t>(woman, NC2Layout::HumanOffset::SpouseId)));

						if (Human_IsHavingChild(woman))
							AssignHome(Human(Read<std::uint32_t>(woman, NC2Layout::HumanOffset::ChildId)), Read<std::uint32_t>(woman, NC2Layout::HumanOffset::HomeHouseId));

						break;
					}
						// Windows has no dispatch branches for allowjob or forbidjob.
				}
			}

			(void)currentObjectRevision;
		}

		static constexpr NXBasics::SToken sNameTokenArray[] = { { "setname", NC2DataIds::MissionStartObjectsToken::SetName }, { nullptr, 0 } };

		if (reader->GetFirstGroup("misc_humannames") != nullptr)
		{
			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				if (NXBasics::XB_Token_FindTokenInArray(sNameTokenArray, reader->GetCurrentName()) != NC2DataIds::MissionStartObjectsToken::SetName)
					continue;

				const auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(reader->GetInt());

				if (id != -1)
					NC2LogicExternals::External_Human_SetName(id, reader->GetInt());
			}
		}

		static constexpr NXBasics::SToken sGraphicsTokenArray[] = { { "setpalette", NC2DataIds::MissionStartObjectsToken::SetPalette }, { nullptr, 0 } };

		if (reader->GetFirstGroup("misc_humangraphics") != nullptr)
		{
			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				if (NXBasics::XB_Token_FindTokenInArray(sGraphicsTokenArray, reader->GetCurrentName()) != NC2DataIds::MissionStartObjectsToken::SetPalette)
					continue;

				const auto check = static_cast<std::uint32_t>(reader->GetInt());
				const auto* palette = reader->GetString();

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(check); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(check, id))
				{
					NC2LogicExternals::External_Human_SetRandomPalettePatch(id, palette);
				}
			}
		}
	}
}

namespace NC2E2
{
	// Original function: FUN_0048f6ab; CE2StaticDataHousesManager::GetHouseId.
	std::int32_t StaticDataHouses_GetHouseId(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticHouseCount; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(static_cast<const char*>(gStaticHouseEntries) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::EngineHouseTypeUnsigned, name) == 0)
				return index;
		}

		return -1;
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004808b3.
	std::int32_t External_HouseType_GetEngineId(const char* name)
	{

		return NC2E2::StaticDataHouses_GetHouseId(name);
	}
}