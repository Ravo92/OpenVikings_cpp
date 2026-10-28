#pragma once

#include "C2LifetimeAccess.h"
#include "C2MapGuid.h"

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
	struct SCleanMapInfoData
	{
		char mapRoot[260];                       // +0x000
		std::uint32_t version;                   // +0x104
		std::uint32_t width;                     // +0x108
		std::uint32_t height;                    // +0x10C
		SCleanMapGuid guid;                      // +0x110
		std::uint32_t mapTypes;                  // +0x120
		std::uint32_t campaignId;                // +0x124
		std::uint32_t campaignMapId;             // +0x128
		char mapName[200];                       // +0x12C
		char description[400];                  // +0x1F4
		std::uint8_t hiddenFromSelection;                   // +0x384
		std::uint8_t userMap;                    // +0x385
		char archiveFilename[260];              // +0x386
		std::uint8_t multiplayerOnly;            // +0x48A
		std::byte padding48B;

		void Tool_GetMapName(char* destination) const;
		const char* Tool_GetMapDescriptionPtr() const;
	};

	static_assert(offsetof(SCleanMapInfoData, hiddenFromSelection) == 0x384);

	class CCleanMapInfoList final : public NXBasics::CListBase
	{
	public:
		explicit CCleanMapInfoList(bool deleteObjects) : CListBase(deleteObjects)
		{}
		~CCleanMapInfoList() override;
		void* Delete(std::uint32_t flags);
	protected:
		void l_DeleteData(void* data) override
		{
			NXBasics::Runtime_Free(data);
		}
	};

	class CCleanMapsManager
	{
	public:
		CCleanMapsManager();
		~CCleanMapsManager();
		void CleanMapList_Update();
		SCleanMapInfoData* CleanMapList_GetInfoPtrByGuid(const SCleanMapGuid& guid);
		SCleanMapInfoData* CleanMapList_GetCampaignMapInfoPtr(std::uint32_t campaignId, std::uint32_t mapId);
		void CleanMapList_Tool_SortByNames();
		static void Tool_GenerateNewGuid(SCleanMapGuid* guid);

		std::uint32_t mMapTypeCounts[7];          // +0x00; slot zero remains unused
		CCleanMapInfoList mMaps; // +0x1C

	private:
		void l_CleanMapList_FreeAll();
		void l_CleanMapList_Update();
		bool l_CleanMapList_Update_WorkOnMap(NXBasics::CIniFileReader& reader, const char* mapRoot, bool userMap, const char* archiveFilename);
		SCleanMapInfoData* l_CleanMapList_GetPtrByGuid(const SCleanMapGuid& guid);
	};

	static_assert(sizeof(SCleanMapInfoData) == 1164);
	static_assert(offsetof(SCleanMapInfoData, guid) == 0x110);
	static_assert(offsetof(SCleanMapInfoData, mapName) == 0x12C);
	static_assert(offsetof(SCleanMapInfoData, description) == 0x1F4);
	static_assert(offsetof(SCleanMapInfoData, archiveFilename) == 0x386);
	static_assert(offsetof(SCleanMapInfoData, multiplayerOnly) == 0x48A);
	static_assert(sizeof(CCleanMapsManager) == (sizeof(void*) == 8 ? 72 : 48));
	static_assert(offsetof(CCleanMapsManager, mMaps) == (sizeof(void*) == 8 ? 32 : 28));
}
