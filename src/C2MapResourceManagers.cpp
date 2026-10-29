#include "C2GameMapStorage.h"
#include "C2AIConstruction.h"
#include "C2CallbackManager.h"
#include "C2DataIds.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapResourceManagers.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticLandscapeTypeData.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		constexpr std::uint8_t FallingTreeLandscape = 5; // Windows landscapetypes.cif: tree falling -> trunk, automatic transition 13.
		bool gFallingTreeTimersNeedRepair = false;
	}

	// Original function: FUN_0049eb44; mode and radius are deliberately untouched.
	CFishSwarmIterator::CFishSwarmIterator() : mPoint(0), mIndex(-1)
	{}

	// Original function: FUN_0049eb55; false retains the current mode.
	std::int32_t CFishSwarmIterator::GetFirstFishSwarmId(bool populatedOnly)
	{
		if (populatedOnly) mMode = NC2Runtime::FishIteratorMode::PopulatedOnly;
		mIndex = l_GetNext(-1);
		return mIndex;
	}
	// Original function: FUN_0049eb75.
	std::int32_t CFishSwarmIterator::GetNextFishSwarmId()
	{
		if (mIndex == -1) return -1;
		mIndex = l_GetNext(mIndex);
		return mIndex;
	}
	// Original function: FUN_0049eb91.
	std::int32_t CFishSwarmIterator::l_GetNext(std::int32_t previous)
	{
		auto index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u);
		while (index < static_cast<CMapFishManager*>(gMapFishManager)->mSwarmCount)
		{
			const auto& swarm = static_cast<CMapFishManager*>(gMapFishManager)->mSwarms[index];
			if ((mMode & NC2Runtime::FishIteratorMode::PopulatedOnlyUnsigned) != 0 && swarm.fishCount == 0)
			{
				index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u);
				continue;
			}
			if ((mMode & NC2Runtime::FishIteratorMode::WithinRadiusUnsigned) == 0 || static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(swarm.x, swarm.y, Read<std::int16_t>(&mPoint, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&mPoint, NC2Layout::MapCoordinatePairOffset::Y))) <= mRadius) return index;
			index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u);
		}
		return -1;
	}

	namespace
	{
		// Original byte: 00550E60; set by fish-relevant map modification callbacks.
		std::uint8_t gFishSwarmValidationNeeded = 0;

		struct SBioRegrowSearch
		{
			std::uint32_t count;
			std::uint32_t points[NC2Runtime::BioRegrowLimits::Candidates];
			SBioRegrowSearch();
		};
		// Original function: FUN_00426E15; the count is subsequently cleared by the caller.
		SBioRegrowSearch::SBioRegrowSearch()
		{
			for (auto& point : points) MapMigPoint_ConstructZero(&point);
		}
		// Original function: FUN_00426c11.
		bool __cdecl ls_BioRegrow_PathfinderCallback(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0) return false;
			const void* patterns[NC2Runtime::PathfinderGeometry::NeighbourDirections];
			if (!GameMap_Misc_GetTrianglePatternDescriptions_AroundPoint(point, patterns)) return false;
			for (std::uint32_t i = 0; i < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++i) if (Read<std::uint8_t>(patterns[i], NC2Layout::TrianglePatternTypeOffset::BioCanGrowOn) == 0 || Read<std::uint8_t>(patterns[i], NC2Layout::TrianglePatternTypeOffset::BioCanPlantOn) == 0) return false;
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & NC2Runtime::MapTerrainFlags::RoadUnsigned) != 0) return false;
			auto cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			for (std::uint32_t radius = 1; radius <= NC2Runtime::BioRegrowLimits::RoadClearanceRadius; ++radius)
			{
				MapMigPoint_MoveCursorUnwrapped(cursor, SwitchIds::HexagonDirection::NorthWest);
				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
					for (std::uint32_t step = 0; step < radius; ++step)
					{
						if (!MapMigPoint_IsInsideMap(&cursor) || (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&cursor), NC2Layout::MapMicroPointOffset::TerrainFlags) & NC2Runtime::MapTerrainFlags::RoadUnsigned) != 0) return false;
						MapMigPoint_MoveCursorUnwrapped(cursor, direction);
					}
			}
			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & NC2Runtime::MapTerrainFlags::BuildBlockedUnsigned) != 0) return false;
			cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(cursor, SwitchIds::HexagonDirection::NorthWest);
			for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
			{
				if (!MapMigPoint_IsInsideMap(&cursor)) return false;
				const auto* neighbor = GameMap_GetConstMapMicroPointPtr(&cursor);
				if ((Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || (Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::TerrainFlags) & NC2Runtime::MapTerrainFlags::BuildBlockedUnsigned) != 0) return false;
				MapMigPoint_MoveCursorUnwrapped(cursor, direction);
			}
			cursor = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			std::uint32_t trees = 0;
			for (std::uint32_t radius = 1; radius <= NC2Runtime::BioRegrowLimits::LandscapeSearchRadius; ++radius)
			{
				MapMigPoint_MoveCursorUnwrapped(cursor, SwitchIds::HexagonDirection::NorthWest);
				for (std::uint32_t direction = 0; direction < NC2Runtime::PathfinderGeometry::NeighbourDirections; ++direction)
					for (std::uint32_t step = 0; step < radius; ++step)
					{
						if (!MapMigPoint_IsInsideMap(&cursor)) return false;
						const auto* neighbor = GameMap_GetConstMapMicroPointPtr(&cursor);
						if (Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(neighbor, NC2Layout::MapMicroPointOffset::ObjectData) == 4) ++trees;
						MapMigPoint_MoveCursorUnwrapped(cursor, direction);
					}
			}
			if (trees < NC2Runtime::BioRegrowLimits::MinimumMatchingLandscapes) return false;
			auto& data = *static_cast<SBioRegrowSearch*>(callback->userData);
			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			return data.count == NC2Runtime::BioRegrowLimits::Candidates;
		}

		template<class T> void InitializeResourceArray(T* manager, void*& owner, std::uint32_t capacity)
		{
			owner = manager;
			std::memset(manager, 0, sizeof(T));
			manager->mRecords = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(capacity, NC2Layout::RecordSize::LandscapeTimerUnsigned));
			// Original point constructors clear words subsequently covered by this clear.
			std::memset(manager->mRecords, 0, static_cast<std::size_t>(capacity) * NC2Layout::RecordSize::LandscapeTimerUnsigned);
			manager->System_InitArray(static_cast<std::int32_t>(capacity), manager->mRecords, NC2Layout::RecordSize::LandscapeTimer, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		}

		template<class T> void LoadResourceArray(T* manager, void*& owner, CIoHelper& io)
		{
			owner = manager;
			std::memset(manager, 0, sizeof(T));
			if (io.mReadHeader.version != 1u)
			{
				io.IO_File_Chunk_Skip();
				return;
			}
			const std::uint32_t capacity = io.IO_File_ReadLong();
			manager->mRecords = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(capacity, NC2Layout::RecordSize::LandscapeTimerUnsigned));
			std::memset(manager->mRecords, 0, static_cast<std::size_t>(capacity) * NC2Layout::RecordSize::LandscapeTimerUnsigned);
			manager->IO_InitAndLoadArray(manager->mRecords, &io, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		}

		// Original function: FUN_0042732b; remaining player-record bytes are untouched.
		void InitializeWallPlayerData(std::byte* record)
		{
			for (std::uint32_t index = 0; index < NC2Runtime::BuildLimits::WallPoints; ++index)
				Write<std::uint32_t>(record, NC2Layout::BuildPlayerOffset::Points + index * NC2Layout::RecordSize::BuildPoint, 0);
		}

		// Original function: FUN_00427f9e.
		void InitializeRoadPlayerData(std::byte* record)
		{
			for (std::uint32_t index = 0; index < NC2Runtime::BuildLimits::RoadPoints; ++index)
				Write<std::uint32_t>(record, NC2Layout::BuildPlayerOffset::Points + index * NC2Layout::RecordSize::BuildPoint, 0);
		}

		template<class T, class F> void InitializeBuildManager(T* manager, void*& owner, std::uint32_t recordBytes, F initialize)
		{
			static_assert(offsetof(T, mPlayerCount) == NC2Layout::BuildManagerOffset::PlayerCount);
			static_assert(offsetof(T, mPlayers) == NC2Layout::BuildManagerOffset::PlayerRecords);
			static_assert(NC2Layout::RecordSize::BuildPlayerPointerArray == NC2Runtime::BuildLimits::Players * sizeof(*manager->mPlayers));
			owner = manager;
			manager->mPlayerCount = 0;
			manager->mPlayerCount = NC2Runtime::BuildLimits::Players;
			manager->mPlayers = static_cast<std::byte**>(NXBasics::Runtime_AllocateArrayWithNewHandler(NC2Runtime::BuildLimits::Players, sizeof(*manager->mPlayers)));
			std::memset(manager->mPlayers, 0, NC2Layout::RecordSize::BuildPlayerPointerArray);
			for (std::int32_t player = 0; player < manager->mPlayerCount; ++player)
			{
				if (player >= NC2Runtime::BuildLimits::Players || Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0)
					continue;
				auto* record = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(recordBytes, 1));
				if (record != nullptr)
					initialize(record);
				manager->mPlayers[player] = record;
				std::memset(record, 0, recordBytes);
			}
		}

		template<class T> void LoadBuildPlayers(T* manager, CIoHelper& io)
		{
			io.IO_File_ReadBinary(&manager->mPlayerCount, NC2Layout::RecordSize::BuildPlayerCount);
			const auto count = static_cast<std::uint32_t>(manager->mPlayerCount);
			manager->mPlayers = static_cast<std::byte**>(NXBasics::Runtime_AllocateArrayWithNewHandler(count, sizeof(*manager->mPlayers)));
			const std::size_t bytes = static_cast<std::size_t>(count) * sizeof(*manager->mPlayers);
			std::memset(manager->mPlayers, 0, bytes);
		}

		template<class T, class F> void LoadBuildRecords(T* manager, CIoHelper& io, std::uint32_t recordBytes, std::uint8_t mapFlag, F initialize)
		{
			for (std::int32_t player = 0; player < manager->mPlayerCount; ++player)
			{
				if (io.IO_File_ReadByte() == 0)
					continue;
				auto* record = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(recordBytes, 1));
				if (record != nullptr)
					initialize(record);
				manager->mPlayers[player] = record;
				io.IO_File_ReadBinary(record, recordBytes);
				for (std::uint32_t index = 0; index < Read<std::uint32_t>(record, NC2Layout::BuildPlayerOffset::PointCount); ++index)
				{
					void* microPoint = GameMap_GetMapMicroPointPtr(record + NC2Layout::BuildPlayerOffset::Points + index * NC2Layout::RecordSize::BuildPointUnsigned);
					Write<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::TerrainFlags, Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::TerrainFlags) | mapFlag);
				}
			}
		}
	}

	// Original function: FUN_0042692d.
	CMapBioRegrowManager::CMapBioRegrowManager()
	{
		InitializeResourceArray(this, gMapBioRegrowManager, NC2Runtime::MapResourceLimits::RegrowEntries);
	}
	// Original function: FUN_004269f9.
	CMapBioRegrowManager::CMapBioRegrowManager(CIoHelper& io)
	{
		LoadResourceArray(this, gMapBioRegrowManager, io);
	}
	// Original function: FUN_00426e2d.
	CMapLandscapeAutoTransitionManager::CMapLandscapeAutoTransitionManager()
	{
		InitializeResourceArray(this, gMapLandscapeAutoTransitionManager, NC2Runtime::MapResourceLimits::AutoTransitionEntries);
		gFallingTreeTimersNeedRepair = true;
	}
	// Original function: FUN_00426ef4.
	CMapLandscapeAutoTransitionManager::CMapLandscapeAutoTransitionManager(CIoHelper& io)
	{
		LoadResourceArray(this, gMapLandscapeAutoTransitionManager, io);
		gFallingTreeTimersNeedRepair = true;
	}

	// Original function: FUN_00426ac7; re-read the live upper bound after freeing each entry.
	void CMapBioRegrowManager::System_DoOneGameTick()
	{
		for (auto index = static_cast<std::uint32_t>(mFirstScanIndex); index <= static_cast<std::uint32_t>(mLastScanIndex); ++index)
		{
			auto* record = mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned;
			if (Read<std::uint8_t>(record, NC2Layout::LandscapeTimerOffset::Allocated) == 0) continue;
			Write(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks, Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks) + 1u);
			if (Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks) < Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::DurationTicks)) continue;
			l_BioRegrow_Execute(record + NC2Layout::LandscapeTimerOffset::MapPosition, Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::EngineLandscapeId));
			if (Element_Free(static_cast<std::int32_t>(index), true)) Write<std::uint8_t>(mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned, NC2Layout::LandscapeTimerOffset::Allocated, 0);
		}
	}

	// Original function: FUN_00426b70; even a failed regrow attempt expires its record.
	void CMapBioRegrowManager::l_BioRegrow_Execute(const void* point, std::uint32_t engineId)
	{
		if (Game_Random_GetNumber() % NC2Runtime::BioRegrowTiming::ChanceDivisorUnsigned < NC2Runtime::BioRegrowTiming::SkippedChanceUnsigned) return;
		if (GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId)) != SwitchIds::ContinentType::Land) return;
		SBioRegrowSearch data;
		std::memset(&data, 0, sizeof(data));
		CMapPathfinderElement::SearchOverMap(point, NC2Runtime::BioRegrowLimits::SearchRadius, false, ls_BioRegrow_PathfinderCallback, &data, false, -1);
		if (data.count == 0) return;
		const auto selected = Game_Random_GetNumber() % data.count;
		GameMapModifyManager_StartGameMapModify();
		const auto landscape = Read<std::uint32_t>(NC2LogicExternals::External_Landscape_GetLogicDataPtr(static_cast<std::int32_t>(engineId)), NC2Layout::EngineLandscapeLogicViewOffset::LogicType);
		GameMap_MapMicroPoint_Landscape_SetType(&data.points[selected], landscape, 1, static_cast<std::int32_t>(engineId), false, -1);
		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_00426f84.
	void CMapLandscapeAutoTransitionManager::System_DoOneGameTick()
	{
		// Remaster: old saves may contain falling trees whose failed transition timer was discarded.
		if (gFallingTreeTimersNeedRepair && gGameMapStaticStorage != nullptr && mRecords != nullptr)
		{
			const auto width = Read<std::uint16_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroWidth);
			const auto count = Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount);
			const auto* points = Read<const std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);
			if (width != 0 && points != nullptr)
			{
				gFallingTreeTimersNeedRepair = false;
				for (std::uint32_t index = 0; index < count; ++index)
				{
					const auto* micro = points + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
					if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != FallingTreeLandscape) continue;
					const std::int16_t position[2] = { static_cast<std::int16_t>(index % width), static_cast<std::int16_t>(index / width) };
					if (AutoTransition_GetDataId(position) == NXBasics::IntegerRange::UInt32Maximum) AutoTransition_Add(position);
				}
			}
		}
		for (auto index = static_cast<std::uint32_t>(mFirstScanIndex); index <= static_cast<std::uint32_t>(mLastScanIndex); ++index)
		{
			auto* record = mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned;
			if (Read<std::uint8_t>(record, NC2Layout::LandscapeTimerOffset::Allocated) == 0) continue;
			Write(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks, Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks) + 1u);
			if (Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks) < Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::DurationTicks)) continue;
			// Copy the point: a failed transition can rebuild this same timer record.
			const auto position = Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::MapPosition);
			const auto engine = Read<std::int32_t>(record, NC2Layout::LandscapeTimerOffset::EngineLandscapeId);
			if (NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(&position) == engine)
			{
				const auto result = GameMap_MapMicroPoint_Landscape_ApplyTransition(&position, SwitchIds::LandscapeTransition::Automatic);
				const auto* micro = GameMap_GetConstMapMicroPointPtr(&position);
				if (result == -1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == FallingTreeLandscape && NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(&position) == engine)
				{
					// No safe nearby pile position exists yet. Retain the wood at the finished
					// animation frame instead of replaying the falling tree on every retry.
					AutoTransition_Add(&position);
					const auto retry = AutoTransition_GetDataId(&position);
					if (retry != NXBasics::IntegerRange::UInt32Maximum)
					{
						auto* timer = mRecords + retry * NC2Layout::RecordSize::LandscapeTimerUnsigned;
						Write(timer, NC2Layout::LandscapeTimerOffset::ElapsedTicks, Read<std::uint32_t>(timer, NC2Layout::LandscapeTimerOffset::DurationTicks));
					}
					continue;
				}
			}
			if (Element_Free(static_cast<std::int32_t>(index), true)) Write<std::uint8_t>(mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned, NC2Layout::LandscapeTimerOffset::Allocated, 0);
		}
	}

	// Original function: FUN_004271f0; row-major scan and one surrounding map-modification batch.
	void Map_BioSystem_UpdateRectangle(const std::int16_t* rectangle)
	{
		GameMapModifyManager_StartGameMapModify();
		const auto lastX = static_cast<std::int16_t>(static_cast<std::uint16_t>(rectangle[0]) + static_cast<std::uint16_t>(rectangle[2]) - 1u);
		const auto lastY = static_cast<std::int16_t>(static_cast<std::uint16_t>(rectangle[1]) + static_cast<std::uint16_t>(rectangle[3]) - 1u);
		for (std::int32_t y = rectangle[1]; y <= lastY; ++y)
			for (std::int32_t x = rectangle[0]; x <= lastX; ++x)
			{
				const std::int16_t point[2] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
				const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1) continue;
				const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
				if (landscape != 0 && (gStaticLandscapeTypeStorage.records[landscape].transitionMask & NXBasics::BitMask::Bit7) != 0) GameMap_MapMicroPoint_Landscape_ApplyTransition(point, SwitchIds::LandscapeTransition::BioUpdate);
			}
		GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_0042712c; the 20-by-20 partition advances every 24 ticks.
	void Map_BioSystem_DoOneGameTick()
	{
		if (gGameTime.tick % NC2Runtime::BioRegrowTiming::PartitionIntervalTicksUnsigned != 0) return;
		const auto cell = gGameTime.tick / NC2Runtime::BioRegrowTiming::PartitionIntervalTicksUnsigned % NC2Runtime::BioRegrowTiming::PartitionCountUnsigned;
		const auto xPercent = static_cast<std::int32_t>(cell % NC2Runtime::BioRegrowTiming::PartitionsPerAxisUnsigned * NC2Runtime::BioRegrowTiming::PartitionPercentUnsigned);
		const auto yPercent = static_cast<std::int32_t>(cell / NC2Runtime::BioRegrowTiming::PartitionsPerAxisUnsigned * NC2Runtime::BioRegrowTiming::PartitionPercentUnsigned);
		const auto x = GameMap_GetWidth() * xPercent / 100;
		const auto y = GameMap_GetHeight() * yPercent / 100;
		const auto endX = GameMap_GetWidth() * (xPercent + NC2Runtime::BioRegrowTiming::PartitionPercent) / 100;
		const auto endY = GameMap_GetHeight() * (yPercent + NC2Runtime::BioRegrowTiming::PartitionPercent) / 100;
		if (x == endX || y == endY) return;
		const std::int16_t rectangle[4] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), static_cast<std::int16_t>(endX - x), static_cast<std::int16_t>(endY - y) };
		Map_BioSystem_UpdateRectangle(rectangle);
	}

	// Original function: FUN_00426b1f.
	void CMapBioRegrowManager::BioRegrow_Add(const void* point, std::uint32_t engineId)
	{
		const auto id = Element_Allocate();
		if (id == -1) return;
		auto* record = mRecords + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::LandscapeTimerUnsigned;
		std::memset(record, 0, NC2Layout::RecordSize::LandscapeTimer);
		Write<std::uint8_t>(record, NC2Layout::LandscapeTimerOffset::Allocated, 1);
		Write(record, NC2Layout::LandscapeTimerOffset::MapPosition, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(record, NC2Layout::LandscapeTimerOffset::EngineLandscapeId, engineId);
		Write(record, NC2Layout::LandscapeTimerOffset::DurationTicks, Game_Random_GetNumber() % NC2Runtime::BioRegrowTiming::RandomDelayTicksUnsigned + NC2Runtime::BioRegrowTiming::MinimumDelayTicksUnsigned);
	}

	// Original function: FUN_00426ff8; a full array applies transition 13 immediately.
	void CMapLandscapeAutoTransitionManager::AutoTransition_Add(const void* point)
	{
		const auto engine = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point);
		const auto* micro = GameMap_GetMapMicroPointPtr(point);
		const auto type = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) : 0u;
		const auto* data = gStaticLandscapeTypeStorage.records + type;
		auto id = AutoTransition_GetDataId(point);
		if (id == NXBasics::IntegerRange::UInt32Maximum)
		{
			id = static_cast<std::uint32_t>(Element_Allocate());
			if (id == NXBasics::IntegerRange::UInt32Maximum)
			{
				if (NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(point) == engine) GameMap_MapMicroPoint_Landscape_ApplyTransition(point, SwitchIds::LandscapeTransition::Automatic);
				return;
			}
		}
		std::uint32_t duration = 1;
		for (std::uint32_t i = 0; i < data->transitionCount; ++i) if (data->transitions[i].type == SwitchIds::LandscapeTransition::Automatic)
		{
			duration = data->transitions[i].resultValue;
			break;
		}
		auto* record = mRecords + id * NC2Layout::RecordSize::LandscapeTimerUnsigned;
		std::memset(record, 0, NC2Layout::RecordSize::LandscapeTimer);
		Write<std::uint8_t>(record, NC2Layout::LandscapeTimerOffset::Allocated, 1);
		Write(record, NC2Layout::LandscapeTimerOffset::MapPosition, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write(record, NC2Layout::LandscapeTimerOffset::EngineLandscapeId, engine);
		Write(record, NC2Layout::LandscapeTimerOffset::DurationTicks, duration);
	}

	// Original function: FUN_004270be.
	std::uint32_t CMapLandscapeAutoTransitionManager::AutoTransition_GetDataId(const void* point) const
	{
		const std::uint32_t last = static_cast<std::uint32_t>(mLastScanIndex);
		for (std::uint32_t index = static_cast<std::uint32_t>(mFirstScanIndex); index <= last; ++index)
		{
			const auto* record = mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned;
			if (Read<std::uint8_t>(record, NC2Layout::LandscapeTimerOffset::Allocated) != 0 && Read<std::uint16_t>(record, NC2Layout::LandscapeTimerOffset::MapPosition) == Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates) && Read<std::uint16_t>(record, NC2Layout::LandscapeTimerOffset::MapPositionY) == Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)) return index;
		}
		return NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_004270ff; multiplication wraps before unsigned division.
	std::uint32_t CMapLandscapeAutoTransitionManager::AutoTransition_GetTimeDonePercentage(const void* point) const
	{
		const std::uint32_t index = AutoTransition_GetDataId(point);
		if (index == NXBasics::IntegerRange::UInt32Maximum) return 0;
		const auto* record = mRecords + index * NC2Layout::RecordSize::LandscapeTimerUnsigned;
		return (Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::ElapsedTicks) * 100u) / Read<std::uint32_t>(record, NC2Layout::LandscapeTimerOffset::DurationTicks);
	}

	// Original function: FUN_00427297.
	CWallBuildManager::CWallBuildManager()
	{
		InitializeBuildManager(this, gWallBuildManager, NC2Layout::RecordSize::WallBuildPlayer, InitializeWallPlayerData);
	}
	// Original function: FUN_004273de.
	CWallBuildManager::CWallBuildManager(CIoHelper& io)
	{
		gWallBuildManager = this;
		mPlayerCount = 0;
		if (io.mReadHeader.version != 1u)
			io.IO_File_Chunk_Skip();
		else
		{
			LoadBuildPlayers(this, io);
			LoadBuildRecords(this, io, NC2Layout::RecordSize::WallBuildPlayer, NC2Runtime::MapTerrainFlags::WallBuildPending, InitializeWallPlayerData);
		}
	}

	// Original function: FUN_00427f0a.
	CRoadBuildManager::CRoadBuildManager()
	{
		InitializeBuildManager(this, gRoadBuildManager, NC2Layout::RecordSize::RoadBuildPlayer, InitializeRoadPlayerData);
	}
	// Original function: FUN_00428053.
	CRoadBuildManager::CRoadBuildManager(CIoHelper& io)
	{
		gRoadBuildManager = this;
		mPlayerCount = 0;
		if (io.mReadHeader.version == 1u)
		{
			LoadBuildPlayers(this, io);
			io.IO_File_Chunk_Skip();
		}
		else if (io.mReadHeader.version == 2u)
		{
			LoadBuildPlayers(this, io);
			LoadBuildRecords(this, io, NC2Layout::RecordSize::RoadBuildPlayer, NC2Runtime::MapTerrainFlags::RoadBuildPending, InitializeRoadPlayerData);
		}
		else
			io.IO_File_Chunk_Skip();
	}

	// Original function: FUN_0044e610.
	CMapFishManager::CMapFishManager()
	{
		MapFishManager_ConstructMembers(this);
		gMapFishManager = this;
		std::memset(this, 0, sizeof(*this));
		l_RegisterCallbacks();
		gFishSwarmValidationNeeded = 0;
	}

	// Original function: FUN_0044e6aa.
	void CMapFishManager::l_RegisterCallbacks()
	{
		for (std::uint32_t type : {SwitchIds::CallbackType::GameTickUnsigned, SwitchIds::CallbackType::BeginMapChangesUnsigned, SwitchIds::CallbackType::EndMapChangesUnsigned, SwitchIds::CallbackType::MapPointChangedUnsigned})
			CCallbackManager::sTheObjectPtr->RegisterCallback(type, MapFishManager_l_FishManager_Callback, NC2Runtime::FishSwarmCallback::RegistrationTag, 0);
	}

	// Original function: FUN_0044e6fa.
	bool CMapFishManager::IO_Cleanmap_Load(CIoHelper& io)
	{
		if (io.mReadHeader.version == 2u)
			io.IO_File_ReadBinary(this, NC2Layout::ObjectSize::MapFishManager);
		else
		{
			io.IO_File_ReadBinary(this, NC2Layout::RecordSize::LegacyFishSwarms);
			io.IO_File_ReadBinary(&mSwarmCount, NC2Layout::RecordSize::FishSwarmCount);
		}
		return true;
	}

	// Original function: FUN_0044e762.
	CMapFishManager::CMapFishManager(CIoHelper& io)
	{
		MapFishManager_ConstructMembers(this);
		gFishSwarmValidationNeeded = 0;
		gMapFishManager = this;
		std::memset(this, 0, sizeof(*this));
		l_RegisterCallbacks();
		if (io.mReadHeader.version != 1u)
			io.IO_File_Chunk_Skip();
		else
			io.IO_File_ReadBinary(this, NC2Layout::ObjectSize::MapFishManager);
	}

	// Original function: FUN_0044e7bd.
	bool CMapFishManager::AddFishAtMapPosition(const void* point, std::uint32_t count, std::uint32_t maximumDistance)
	{
		if (!MapMigPoint_IsInsideMap(point) || MapMigPoint_IsBorderPoint(point))
			return false;
		const void* microPoint = GameMap_GetConstMapMicroPointPtr(point);
		if (GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(microPoint, NC2Layout::MapMicroPointOffset::ContinentId)) != SwitchIds::ContinentType::WaterUnsigned)
			return false;
		const std::uint32_t continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);
		std::int32_t nearest = mSwarmCount;
		std::uint32_t nearestDistance = NC2Runtime::FishSwarmLimits::DistanceNotFound;
		std::int32_t empty = NC2Runtime::FishSwarmLimits::Capacity;
		bool nearbyFullSwarm = false;
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			auto& swarm = mSwarms[index];
			if (swarm.continentId == continent)
			{
				const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(swarm.x, swarm.y, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)));
				if (distance < maximumDistance)
				{
					if (distance < nearestDistance)
					{
						if (swarm.fishCount < NC2Runtime::FishSwarmLimits::FishPerSwarmUnsigned)
						{
							nearest = index;
							nearestDistance = distance;
						}
						else
							nearbyFullSwarm = true;
					}
					continue;
				}
			}
			if (swarm.fishCount == 0)
				empty = index;
		}
		if (nearest < mSwarmCount)
		{
			auto& fishCount = mSwarms[nearest].fishCount;
			fishCount += count;
			if (fishCount > NC2Runtime::FishSwarmLimits::FishPerSwarmUnsigned) fishCount = NC2Runtime::FishSwarmLimits::FishPerSwarm;
			return true;
		}
		if (nearbyFullSwarm)
			return false;
		if (static_cast<std::uint32_t>(nearest) < NC2Runtime::FishSwarmLimits::CapacityUnsigned)
		{
			auto& swarm = mSwarms[nearest];
			std::memcpy(&swarm.x, point, NC2Layout::RecordSize::PackedMapPoint);
			swarm.fishCount = count > NC2Runtime::FishSwarmLimits::FishPerSwarmUnsigned ? NC2Runtime::FishSwarmLimits::FishPerSwarmUnsigned : count;
			swarm.continentId = continent;
			++mSwarmCount;
		}
		else
		{
			if (empty >= mSwarmCount)
				return false;
			auto& swarm = mSwarms[empty];
			std::memcpy(&swarm.x, point, NC2Layout::RecordSize::PackedMapPoint);
			swarm.fishCount = 1;
			swarm.continentId = continent;
		}
		return true;
	}

	// Original function: FUN_0044e928.
	bool CMapFishManager::IsFishNearMapPosition(const void* point, std::uint32_t maximumDistance) const
	{
		if (MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(point)) != SwitchIds::ContinentType::WaterUnsigned) return false;
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			const auto& swarm = mSwarms[index];
			if (swarm.continentId != continent) continue;
			if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(swarm.x, swarm.y, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y))) < maximumDistance && static_cast<std::int32_t>(swarm.fishCount) > 0) return true;
		}
		return false;
	}

	// Original function: FUN_0044e9b8.
	bool CMapFishManager::GetOneFishFromNearestSwarm(const void* point, std::uint32_t maximumDistance, void* swarmPosition)
	{
		if (MapMicroPointData_Continent_GetContinentTyp(GameMap_GetConstMapMicroPointPtr(point)) != SwitchIds::ContinentType::WaterUnsigned) return false;
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ContinentId);
		auto nearest = mSwarmCount;
		std::uint32_t nearestDistance = NC2Runtime::FishSwarmLimits::DistanceNotFound;
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			const auto& swarm = mSwarms[index];
			if (swarm.continentId != continent) continue;
			const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(swarm.x, swarm.y, Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)));
			if (distance < maximumDistance && distance < nearestDistance && static_cast<std::int32_t>(swarm.fishCount) > 0)
			{
				nearest = index;
				nearestDistance = distance;
			}
		}
		if (nearest >= mSwarmCount) return false;
		--mSwarms[nearest].fishCount;
		Write(swarmPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(&mSwarms[nearest], NC2Layout::MapCoordinatePairOffset::Coordinates));
		return true;
	}

	// Original function: FUN_0044ea7c.
	void CMapFishManager::ClearSwarmAtPosition(const void* point)
	{
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			if (mSwarms[index].x != Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates) || mSwarms[index].y != Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y))
				continue;
			std::memmove(mSwarms + index, mSwarms + index + 1, (static_cast<std::uint32_t>(mSwarmCount) - static_cast<std::uint32_t>(index) - 1u) * NC2Layout::RecordSize::FishSwarmUnsigned);
			--mSwarmCount;
			break;
		}
	}

	// Original function: FUN_0044eadd.
	void CMapFishManager::l_FishReproduction()
	{
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			auto& swarm = mSwarms[index];
			if (NXBasics::Runtime_TimeSigned32(swarm.fishCount) <= 0)
				continue;
			if (NXBasics::Runtime_TimeSigned32(swarm.fishCount) < NC2Runtime::FishSwarmLimits::FishPerSwarm)
				++swarm.fishCount;
			else
				AddFishAtMapPosition(&swarm.x, 1, NC2Runtime::FishSwarmLimits::ReproductionRadius);
		}
	}

	// Original function: FUN_0044eb1e.
	bool __cdecl MapFishManager_l_FishManager_Callback(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParameter1, std::uintptr_t /*parameter2*/, std::uintptr_t /*parameter3*/)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		switch (type)
		{
			case SwitchIds::CallbackType::GameTick:
				if (parameter1 % NC2Runtime::FishSwarmTiming::ReproductionIntervalTicksUnsigned == 0)
					static_cast<CMapFishManager*>(gMapFishManager)->l_FishReproduction();
				break;
			case SwitchIds::CallbackType::BeginMapChanges: gFishSwarmValidationNeeded = 0;
				break;
			case SwitchIds::CallbackType::EndMapChanges:
				if (gFishSwarmValidationNeeded != 0)
					static_cast<CMapFishManager*>(gMapFishManager)->RemoveInvalidSwarms();
				break;
			case SwitchIds::CallbackType::MapPointChanged:
				if (parameter1 == 0x0B || parameter1 == 0x0E)
					gFishSwarmValidationNeeded = 1;
				break;
		}
		return true;
	}

	// Original function: FUN_0044eb85.
	std::uint32_t CMapFishManager::RemoveInvalidSwarms()
	{
		std::uint32_t removed = 0;
		for (std::int32_t index = 0; index < mSwarmCount; ++index)
		{
			const void* point = GameMap_GetConstMapMicroPointPtr(&mSwarms[index].x);
			if ((Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::WalkingFlags) & NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned) >= NC2Runtime::MapWalkingFlags::VehicleSizeMaskUnsigned && GameMap_Continent_GetContinentTyp(Read<std::uint8_t>(point, NC2Layout::MapMicroPointOffset::ContinentId)) == SwitchIds::ContinentType::WaterUnsigned)
				continue;
			ClearSwarmAtPosition(&mSwarms[index].x);
			--index;
			++removed;
		}
		return removed;
	}
	// Original function: FUN_004269c5.
	void CMapBioRegrowManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::BioRegrowth, 1);
		io.IO_Memory_WriteLong(static_cast<std::uint32_t>(mCapacity));
		IO_SaveArray(&io);
		io.IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00426ec0.
	void CMapLandscapeAutoTransitionManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::AutoTransitions, 1);
		io.IO_Memory_WriteLong(static_cast<std::uint32_t>(mCapacity));
		IO_SaveArray(&io);
		io.IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_0044e733.
	void CMapFishManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::FishManager, 1);
		io.IO_Memory_WriteBinary(this, NC2Layout::ObjectSize::MapFishManager);
		io.IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00427feb.
	void CRoadBuildManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::RoadBuildManager, 2);
		io.IO_Memory_WriteBinary(this, NC2Layout::RecordSize::BuildPlayerCount);
		for (std::int32_t index = 0; index < mPlayerCount; ++index)
		{
			io.IO_Memory_WriteByte(mPlayers[index] != nullptr);
			if (mPlayers[index] != nullptr) io.IO_Memory_WriteBinary(mPlayers[index], NC2Layout::RecordSize::RoadBuildPlayer);
		}
		io.IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00427376.
	void CWallBuildManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::WallBuildManager, 1);
		io.IO_Memory_WriteBinary(this, NC2Layout::RecordSize::BuildPlayerCount);
		for (std::int32_t index = 0; index < mPlayerCount; ++index)
		{
			io.IO_Memory_WriteByte(mPlayers[index] != nullptr);
			if (mPlayers[index] != nullptr) io.IO_Memory_WriteBinary(mPlayers[index], NC2Layout::RecordSize::WallBuildPlayer);
		}
		io.IO_Tool_Chunk_FinishAndWrite();
	}

}
