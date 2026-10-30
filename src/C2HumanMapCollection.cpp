#include "C2EngineMapTerrainAccess.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanLandscapeSearch.h"
#include "C2HumanMapCollection.h"
#include "C2HumanProduction.h"
#include "C2HumanTaskAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* WorkHouse(const void* human)
		{

			return CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;
		}

		const void* Real(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
		}

		const void* Work(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
		}

		const void* Center(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		}

		void Status(void* human, std::uint32_t status)
		{
			Write(human, NC2Layout::HumanOffset::CurrentTaskStatus, status);
		}

		void Fail(void* human, std::uint32_t reason)
		{
			Status(human, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		void Start(void* human, std::uint32_t animation, std::int32_t direction)
		{
			Human_l_StartAtomicAnimation(human, animation, direction);
			Status(human, 1);
		}

		void Reserve(void* human)
		{
			House_Stock_ModifyFutureAmount(WorkHouse(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), 1, true);
		}

		void TargetCurrentPosition(void* human, std::uint32_t landscape)
		{
			Human_Target_SetTargetPositions(human, Human_GetMapPositionRef(human), Human_GetMapPositionRef(human));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2);
			Write(human, NC2Layout::HumanOffset::TargetLandscapeType, landscape);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 2);
		}

		bool CanPickupTarget(const void* human, std::uint32_t type, std::uint32_t pickup, std::uint32_t store)
		{
			if (type == pickup)
				return true;

			if (type != store)
				return false;

			if (type == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType) && !Human_IsAttachedToWorkHouse(human))
				return true;

			return Human_IsAttachedToWorkHouse(human) && House_Stock_HasDemandForGood(WorkHouse(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
		}
	}

	// Original function: FUN_0042d159.
	std::uint32_t Human_GetLandscapeTypeAtActiveMapPosition(const void* human)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));

		return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u;
	}

	// Original function: FUN_0042d172.
	std::uint32_t Human_GetLandscapeValencyAtActiveMapPosition(const void* human)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));

		return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
	}

	// Original function: FUN_0042d1a5.
	std::uint32_t Human_GetLandscapeValencyAtTargetPosition(const void* human)
	{
		const auto* micro = GameMap_GetConstMapMicroPointPtr(Real(human));

		return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0u;
	}

	// Original function: FUN_0042d1bf.
	bool Human_CanPlantBioAtTargetMapPosition(const void* human)
	{

		return Human_CanPlantBioAtMapPosition(Real(human));
	}

	// Original function: FUN_0042d1cd.
	bool Human_CanPlantBioAtMapPosition(const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point))
			return false;

		const auto* micro = GameMap_GetMapMicroPointPtr(point);
		const auto flags = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags), kind = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType);

		if ((flags & 3u) != 0 || kind == 3 || (kind == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0))
			return false;

		const void* descriptions[6];
		NC2E2::Engine2d_EntryPoint_Tool_GetPatternDescriptionsAroundPoint(point, descriptions);

		for (const auto* description : descriptions)
		{
			if (description == nullptr || gStaticTrianglePatternTypeStorage.records[Read<std::uint32_t>(description, NC2Layout::EngineTrianglePatternOffset::LogicType)].bioCanPlantOn != 1)
				return false;
		}

		return true;
	}

	// Original function: FUN_0046d127.
	void Human_l_StartTask_ExecuteJob_Farmer(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (!Human_IsAttachedToWorkHouse(human) || good == 0)
		{
			Fail(human, 2);
			return;
		}

		if (!Human_IsAbleToProduceGoodNow(human, good))
		{
			Fail(human, 0);
			return;
		}

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::None:
				if (Read<std::uint32_t>(WorkHouse(human), NC2Layout::HouseOffset::ConstructionState) != 3)
				{
					Fail(human, 2);
					return;
				}

				if (House_Stock_IsFullSoon(WorkHouse(human), good))
				{
					Write(human, NC2Layout::HumanOffset::RequestedGood, good);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);
					Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);
					return;
				}

				Human_l_StartTask_ExecuteJob_Tool_MapFarmer_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Tool_MapFarmer_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);

				return;
		}
	}

	// Original function: FUN_0046d1ea.
	void Human_l_StartTask_ExecuteJob_Fisher(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 0)
		{
			Fail(human, 2);
			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 22);

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::None:
				Human_l_StartTask_ExecuteJob_Fisher_TryToCollect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Fisher_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Fail(human, 0);

				return;
		}
	}

	// Original function: FUN_0046d245.
	void Human_l_StartTask_ExecuteJob_Fisher_Collect(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetGoodType) != 0)
		{
			if (Human_IsAtTargetPoint(human))
			{
				Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human);
				return;
			}

			if (Human_l_StartAtomicWalkAndFetch(human, Real(human), Work(human), false))
				return;

			Human_Target_Reset(human);
		}

		if (Human_IsAtTargetPoint(human))
		{
			const auto moveable = Read<std::int16_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::FirstMapMoveableId);

			if (moveable == Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId) && Read<std::int32_t>(gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(moveable) * NC2Layout::RecordSize::MapMoveable, NC2Layout::MapMoveableOffset::NextCellMoveableId) == -1)
			{
				Start(human, 36, Human_Target_GetDirectionTo(human));
			}
			else
			{
				Human_l_StartTask_GoAway(human, 0, 0);
			}

			return;
		}

		Human_StopCurrentGoodCirculationMode(human);
		Human_l_StartTask_ExecuteJob_Fisher_TryToCollect(human);
	}

	// Original function: FUN_0046d305.
	void Human_l_StartTask_ExecuteJob_Fisher_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		if (Human_IsAttachedToWorkHouse(human) && House_Stock_IsFull(WorkHouse(human), Tool_Human_GetReallyProducedGoodType(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType))))
		{
			Fail(human, 11);

			return;
		}

		std::uint32_t real = 0, work = 0;

		if (!Human_FindShorePointForFishing(human, &real, &work) || !Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			Fail(human, 3);
	}

	// Original function: FUN_0046f19d; unlike Farmer, no production-capability check.
	void Human_l_StartTask_ExecuteJob_HerbGuy(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (!Human_IsAttachedToWorkHouse(human) || good == 0)
		{
			Fail(human, 2);
			return;
		}

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::None:
				if (Human_IsForcedToHaveFinishedWorkHouse(human) && Read<std::uint32_t>(WorkHouse(human), NC2Layout::HouseOffset::ConstructionState) != 3)
				{
					Fail(human, 2);
					return;
				}

				if (!House_Stock_CanStoreGoodType(WorkHouse(human), good, true))
				{
					Fail(human, 2);
					return;
				}

				if (House_Stock_IsFullSoon(WorkHouse(human), good))
				{
					Write(human, NC2Layout::HumanOffset::RequestedGood, good);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);
					Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);
					return;
				}

				Human_l_StartTask_ExecuteJob_Tool_MapFarmer_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Tool_MapFarmer_Collect(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);

				return;
		}
	}

	// Original function: FUN_0046c876.
	void Human_l_StartTask_ExecuteJob_Collector(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
			return;
		}

		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (good == 0)
		{
			Fail(human, 2);
			return;
		}

		if (Human_Production_GetCounter(human, good) == 0)
		{
			Human_Production_Rotate(human);

			return;
		}

		Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect(human);
	}

	// Original function: FUN_0047603f.
	void Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect(void* human)
	{
		if (Human_Target_HasTarget(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 2)
		{
			Human_StopCurrentGoodCirculationMode(human);
			Human_Target_Reset(human);
		}

		auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (good != 65)
		{
			if (Human_IsAttachedToWorkHouse(human) && House_Stock_IsFull(WorkHouse(human), good))
			{
				Fail(human, 11);
				return;
			}
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) != 0)
		{
			good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood);
		}
		else
		{
			SHumanSearchLandscapesData data;
			std::uint32_t real = 0, work = 0;

			for (std::uint32_t candidate = 1; candidate < 55; ++candidate)
			{
				if (!Human_IsAbleToProduceGoodNow(human, candidate))
					continue;

				const auto& type = gStaticGoodTypeStorage.records[candidate];

				if (type.isBioLandscape != 0)
				{
					data.AddBio(type.landscapeToHarvest);
				}
				else
				{
					data.Add(type.landscapeToHarvest);
				}

				data.Add(type.landscapeToPickup);

				if (Human_IsAttachedToWorkHouse(human))
					data.Add(type.landscapeToStore);
			}

			for (std::uint32_t index = 0; index < data.typeCount; ++index)
			{
				data.entries[index].enabled = 1;
			}

			bool found = false;

			for (std::uint32_t retry = 0; retry < data.typeCount; ++retry)
			{
				if (!Human_FindLandscapesNearPos_Random(human, data, &real, &work, Center(human)))
					continue;

				const auto* micro = GameMap_GetConstMapMicroPointPtr(&real);
				good = gStaticDependencyGoodByLandscape[Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u];
				found = true;
				break;
			}

			if (!found)
			{
				Fail(human, 3);

				return;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Write(human, NC2Layout::HumanOffset::CollectionGoodType, good);
		}

		Write(human, NC2Layout::HumanOffset::RequestedGood, good);
		const auto& type = gStaticGoodTypeStorage.records[good];

		if (type.isBioLandscape != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect_Bio(human, type.landscapeToHarvest, type.landscapeToPickup, type.landscapeToStore, type.atomicForHarvesting);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect_NonBio(human, type.landscapeToHarvest, type.landscapeToPickup, type.landscapeToStore, type.atomicForHarvesting);
		}
	}

	// Original function: FUN_0047624a.
	void Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect_NonBio(void* human, std::uint32_t harvest, std::uint32_t pickup, std::uint32_t store, std::uint32_t animation)
	{
		if (Human_Target_HasTarget(human) && !Human_Target_IsValid(human))
		{
			Human_StopCurrentGoodCirculationMode(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			const auto type = Human_GetLandscapeTypeAtTargetPosition(human);

			if (harvest != 0 && type == harvest)
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 2);

					if (Human_IsAttachedToWorkHouse(human))
						Reserve(human);
				}

				Start(human, animation, Human_Target_GetDirectionTo(human));
				return;
			}

			if (CanPickupTarget(human, type, pickup, store))
			{
				Start(human, 22, 6);
				return;
			}

			Human_Target_Reset(human);
			Status(human, 3);
			return;
		}

		if (Human_Target_HasTarget(human) && Human_l_StartAtomicWalkAndFetch(human, Real(human), Work(human), false))
			return;

		if (harvest != 0 && Human_GetLandscapeTypeAtActiveMapPosition(human) == harvest)
		{
			TargetCurrentPosition(human, harvest);

			if (Human_IsAttachedToWorkHouse(human))
				Reserve(human);

			Start(human, animation, -1);
			return;
		}

		SHumanSearchLandscapesData data;
		std::uint32_t real = 0, work = 0;

		if (Human_IsAttachedToWorkHouse(human))
		{
			if (House_Stock_HasDemandForGood(WorkHouse(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
			{
				data.Add(store);
				data.Add(pickup);

				if (harvest != 0)
					data.Add(harvest);

				data.SetCurrent(store);

				if (Human_FindLandscapesNearPos_Sorted(human, data, &real, &work, Center(human)))
				{
					if (!Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
						Status(human, 3);

					return;
				}
			}
		}
		else
		{
			data.Add(pickup);

			if (harvest != 0)
				data.Add(harvest);
		}

		data.SetCurrent(pickup);

		if (Human_GetLandscapeTypeAtActiveMapPosition(human) == pickup)
		{
			TargetCurrentPosition(human, pickup);

			if (Human_IsAttachedToWorkHouse(human))
				Reserve(human);

			Start(human, 22, 6);
			return;
		}

		if (!Human_FindLandscapesNearPos_Sorted(human, data, &real, &work, Center(human)))
		{
			if (harvest == 0)
			{
				Status(human, 3);
				return;
			}

			data.SetCurrent(harvest);

			if (!Human_FindLandscapesNearPos_Sorted(human, data, &real, &work, Center(human)))
			{
				Fail(human, 3);

				return;
			}
		}

		if (!Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			Status(human, 3);
	}

	// Original function: FUN_004765cc.
	void Human_l_StartTask_ExecuteJob_Tool_MapCollector_Collect_Bio(void* human, std::uint32_t harvest, std::uint32_t pickup, std::uint32_t store, std::uint32_t animation)
	{
		if (Human_Target_HasTarget(human) && !Human_Target_IsValid(human))
		{
			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			const auto type = Human_GetLandscapeTypeAtTargetPosition(human);

			if (harvest != 0 && type == harvest)
			{
				if (harvest == 4 && Human_GetLandscapeValencyAtTargetPosition(human) < 3)
				{
					Human_Target_Reset(human);
					Human_StopCurrentGoodCirculationMode(human);
					return;
				}

				Start(human, animation, Human_Target_GetDirectionTo(human));
				return;
			}

			if (CanPickupTarget(human, type, pickup, store))
			{
				Start(human, 22, 6);
				return;
			}

			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);
			Status(human, 3);
			return;
		}

		if (Human_Target_HasTarget(human) && Human_l_StartAtomicWalkAndFetch(human, Real(human), Work(human), false))
			return;

		if (harvest != 0 && Human_GetLandscapeTypeAtActiveMapPosition(human) == harvest && Human_GetLandscapeValencyAtActiveMapPosition(human) >= 3)
		{
			TargetCurrentPosition(human, harvest);
			Reserve(human); // Native current-position bio branch requires a work house.
			Start(human, animation, -1);
			return;
		}

		SHumanSearchLandscapesData data;
		std::uint32_t real = 0, work = 0;

		if (Human_IsAttachedToWorkHouse(human))
		{
			if (House_Stock_HasDemandForGood(WorkHouse(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
			{
				if (Human_GetLandscapeTypeAtActiveMapPosition(human) == store)
				{
					TargetCurrentPosition(human, store);
					Reserve(human);
					Start(human, 22, 6);
					return;
				}

				data.Add(store);
				data.Add(pickup);
				data.SetCurrent(store);

				if (Human_FindLandscapesNearPos_Sorted(human, data, &real, &work, Center(human)))
				{
					if (!Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
						Status(human, 3);

					return;
				}
			}
		}
		else
		{
			data.Add(pickup);
		}

		data.SetCurrent(pickup);

		if (Human_GetLandscapeTypeAtActiveMapPosition(human) == pickup)
		{
			TargetCurrentPosition(human, pickup);

			if (Human_IsAttachedToWorkHouse(human))
				Reserve(human);

			Start(human, 22, 6);
			return;
		}

		if (!Human_FindLandscapesNearPos_Sorted(human, data, &real, &work, Center(human)))
		{
			if (harvest == 0 || !Human_FindBioLandscapeNearWorkCenter(human, harvest, &real, &work))
			{
				Fail(human, 3);

				return;
			}
		}

		if (!Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			Status(human, 3);
	}

	// Original function: FUN_0047696d.
	void Human_l_StartTask_ExecuteJob_Tool_MapFarmer_Collect(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		Write(human, NC2Layout::HumanOffset::RequestedGood, good);
		const auto& type = gStaticGoodTypeStorage.records[good];

		if (Human_IsAtTargetPoint(human))
		{
			const auto landscape = Human_GetLandscapeTypeAtTargetPosition(human);

			if (landscape == static_cast<std::uint32_t>(type.landscapeToHarvest) && Read<std::uint8_t>(WorkHouse(human), NC2Layout::HouseOffset::ProductionStoppedFlag) == 0)
			{
				const auto valency = Human_GetLandscapeValencyAtTargetPosition(human);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2);
				Write(human, NC2Layout::HumanOffset::TargetLandscapeType, type.landscapeToHarvest);
				Start(human, valency >= 5 ? type.atomicForHarvesting : type.atomicForCultivating, Human_Target_GetDirectionTo(human));
				return;
			}

			if (landscape == static_cast<std::uint32_t>(type.landscapeToPickup) && Human_IsAttachedToWorkHouse(human) && House_Stock_HasDemandForGood(WorkHouse(human), good))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 2);
				Write(human, NC2Layout::HumanOffset::TargetLandscapeType, type.landscapeToPickup);
				Start(human, 22, 6);
				return;
			}

			if (Human_CanPlantBioAtTargetMapPosition(human))
			{
				Human_Target_SetType(human, 1);
				Start(human, type.atomicForPlanting, Human_Target_GetDirectionTo(human));
				return;
			}

			Human_Target_Reset(human);
			Status(human, 3);
			return;
		}

		std::uint32_t real = 0, work = 0;

		if (Human_IsAttachedToWorkHouse(human))
		{
			if (House_Stock_HasDemandForGood(WorkHouse(human), good) && Human_FindLandscapeNearWorkCenter(human, type.landscapeToPickup, &real, &work))
			{
				if (!Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
					Status(human, 3);

				return;
			}

			if (Read<std::uint8_t>(WorkHouse(human), NC2Layout::HouseOffset::ProductionStoppedFlag) != 0)
			{
				Status(human, 3);

				return;
			}
		}

		if (!Human_FindGrowLandscapeNearWorkCenter(human, type.landscapeToHarvest, &real, &work) || !Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			Status(human, 3);
	}
}