#pragma once

#include "C2LogicObjectArrays.h"

namespace NC2Logic
{
	// Physical Windows timer entry used by the distinct regrow and auto-transition arrays.
	struct SMapLandscapeTimerStorage
	{
		std::uint8_t allocated;
		std::byte unclassified01[3];
		std::uint32_t elapsedTicks, durationTicks;
		std::int32_t engineLandscapeId;
		std::int16_t x, y;
	};

	class CMapBioRegrowManager : public CArrayHandler
	{
	public:
		CMapBioRegrowManager();
		explicit CMapBioRegrowManager(CIoHelper& io);
		void IO_Savegame_DoSave(CIoHelper& io);
		void BioRegrow_Add(const void* point, std::uint32_t engineId);
		void System_DoOneGameTick();
		void l_BioRegrow_Execute(const void* point, std::uint32_t engineId);
		std::byte* mRecords; // +0x30: owned 20-byte regrow entries.
	};

	class CMapLandscapeAutoTransitionManager : public CArrayHandler
	{
	public:
		CMapLandscapeAutoTransitionManager();
		explicit CMapLandscapeAutoTransitionManager(CIoHelper& io);
		void IO_Savegame_DoSave(CIoHelper& io);
		std::uint32_t AutoTransition_GetDataId(const void* point) const;
		void AutoTransition_Add(const void* point);
		void System_DoOneGameTick();
		std::uint32_t AutoTransition_GetTimeDonePercentage(const void* point) const;
		std::byte* mRecords; // +0x30: owned 20-byte transition entries.
	};

	struct SFishSwarm
	{
		std::int16_t x;
		std::int16_t y;
		std::uint32_t fishCount;
		std::uint32_t continentId;
	};

	class CFishSwarmIterator
	{
	public:
		CFishSwarmIterator();
		std::int32_t GetFirstFishSwarmId(bool populatedOnly);
		std::int32_t GetNextFishSwarmId();
		std::int32_t l_GetNext(std::int32_t previous);
		std::uint32_t mMode;
		std::uint32_t mPoint;
		std::uint32_t mRadius;
		std::int32_t mIndex;
	};
	static_assert(sizeof(CFishSwarmIterator) == 16);

	class CMapFishManager
	{
	public:
		CMapFishManager();
		explicit CMapFishManager(CIoHelper& io);
		void IO_Savegame_DoSave(CIoHelper& io);
		bool IO_Cleanmap_Load(CIoHelper& io);
		bool AddFishAtMapPosition(const void* point, std::uint32_t count, std::uint32_t maximumDistance);
		bool IsFishNearMapPosition(const void* point, std::uint32_t maximumDistance) const;
		bool GetOneFishFromNearestSwarm(const void* point, std::uint32_t maximumDistance, void* swarmPosition);
		void ClearSwarmAtPosition(const void* point);
		void l_FishReproduction();
		std::uint32_t RemoveInvalidSwarms();

		SFishSwarm mSwarms[500];
		std::int32_t mSwarmCount;
	private:
		void l_RegisterCallbacks();
	};

	// Physical Windows build records. Raw accesses preserve unaligned DWORD operations.
	struct SBuildPointStorage
	{
		std::int16_t x;
		std::int16_t y;
		std::uint8_t assigned;
		std::uint8_t unresolved05;
	};
	struct SWallBuildPlayerStorage
	{
		std::uint32_t pointCount;
		std::uint32_t assignedWorkerCount;
		SBuildPointStorage points[100];
	};
	struct SRoadBuildPlayerStorage
	{
		std::uint32_t pointCount;
		std::uint32_t assignedWorkerCount;
		SBuildPointStorage points[200];
	};

	class CWallBuildManager
	{
	public:
		CWallBuildManager();
		explicit CWallBuildManager(CIoHelper& io);
		CWallBuildManager(const CWallBuildManager&) = delete;
		CWallBuildManager& operator=(const CWallBuildManager&) = delete;
		void IO_Savegame_DoSave(CIoHelper& io);
		std::int32_t mPlayerCount;
		std::byte** mPlayers;
	};

	class CRoadBuildManager
	{
	public:
		CRoadBuildManager();
		explicit CRoadBuildManager(CIoHelper& io);
		CRoadBuildManager(const CRoadBuildManager&) = delete;
		CRoadBuildManager& operator=(const CRoadBuildManager&) = delete;
		void IO_Savegame_DoSave(CIoHelper& io);
		std::int32_t mPlayerCount;
		std::byte** mPlayers;
	};

	bool __cdecl MapFishManager_l_FishManager_Callback(std::uint32_t type, std::uintptr_t userData, std::uintptr_t parameter1, std::uintptr_t parameter2, std::uintptr_t parameter3);
	void Map_BioSystem_UpdateRectangle(const std::int16_t* rectangle);
	void Map_BioSystem_DoOneGameTick();

	static_assert(sizeof(CMapBioRegrowManager) == (sizeof(void*) == 8 ? 80 : 52));
	static_assert(sizeof(CMapLandscapeAutoTransitionManager) == (sizeof(void*) == 8 ? 80 : 52));
	static_assert(offsetof(CMapBioRegrowManager, mRecords) == sizeof(CArrayHandler));
	static_assert(offsetof(CMapLandscapeAutoTransitionManager, mRecords) == sizeof(CArrayHandler));
	static_assert(sizeof(SFishSwarm) == 12);
	static_assert(sizeof(CMapFishManager) == 6004);
	static_assert(offsetof(CMapFishManager, mSwarmCount) == 0x1770);
	static_assert(sizeof(SBuildPointStorage) == 6);
	static_assert(sizeof(SWallBuildPlayerStorage) == 608);
	static_assert(sizeof(SRoadBuildPlayerStorage) == 1208);
	static_assert(offsetof(SWallBuildPlayerStorage, points) == 8);
	static_assert(offsetof(SRoadBuildPlayerStorage, points) == 8);
	static_assert(sizeof(CWallBuildManager) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(CRoadBuildManager) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(CWallBuildManager, mPlayerCount) == 0);
	static_assert(offsetof(CRoadBuildManager, mPlayerCount) == 0);
	static_assert(sizeof(CWallBuildManager::mPlayerCount) == 4);
	static_assert(sizeof(CRoadBuildManager::mPlayerCount) == 4);
	static_assert(offsetof(CWallBuildManager, mPlayers) == sizeof(void*));
	static_assert(offsetof(CRoadBuildManager, mPlayers) == sizeof(void*));
}
