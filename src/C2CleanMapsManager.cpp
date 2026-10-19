#include "C2CleanMapsManager.h"
#include "XTextEncoding.h"
#include "C2DataIds.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicIOAccess.h"
#include "C2StringIO.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XDirectoryScanner.h"
#include "XFileAccess.h"
#include "XIniFileReader.h"
#include "XStringArray.h"

#include <objbase.h>
#include <Windows.h>

namespace NC2Logic
{
	using namespace NXBasics;

	// Original function: FUN_00416be5; CList<SCleanMapInfoData> destructor.
	CCleanMapInfoList::~CCleanMapInfoList()
	{
		if (mDeleteObjects != 0) DeleteAllElements();
	}
	// Original function: FUN_00416c02; native scalar deleting-destructor adapter.
	void* CCleanMapInfoList::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CCleanMapInfoList();
		if ((flags & 1u) != 0) Runtime_Free(result);
		return result;
	}

	// Original function: FUN_004163b7.
	CCleanMapsManager::CCleanMapsManager() : mMaps(false)
	{
		gCleanMapsManager = this;
		std::memset(mMapTypeCounts, 0, sizeof(mMapTypeCounts));
		l_CleanMapList_Update();
	}

	// Original function: FUN_004163fa; member-list destructor: FUN_00416be5.
	CCleanMapsManager::~CCleanMapsManager()
	{
		l_CleanMapList_FreeAll();
		gCleanMapsManager = nullptr;
	}

	// Original function: FUN_00416413.
	void CCleanMapsManager::CleanMapList_Update()
	{
		l_CleanMapList_Update();
	}

	// Original function: FUN_00416418.
	SCleanMapInfoData* CCleanMapsManager::CleanMapList_GetInfoPtrByGuid(const SCleanMapGuid& guid)
	{
		return l_CleanMapList_GetPtrByGuid(guid);
	}

	// Original function: FUN_00416424.
	SCleanMapInfoData* CCleanMapsManager::CleanMapList_GetCampaignMapInfoPtr(std::uint32_t campaignId, std::uint32_t mapId)
	{
		CListBase::Iterator iterator(mMaps);
		for (auto* data = static_cast<SCleanMapInfoData*>(iterator.First()); data != nullptr; data = static_cast<SCleanMapInfoData*>(iterator.Next()))
			if ((data->mapTypes & 2u) != 0 && data->campaignId == campaignId && data->campaignMapId == mapId)
				return data;
		return nullptr;
	}

	// Original function: FUN_00416543.
	static std::int32_t __cdecl CleanMap_CompareNames(const void* first, const void* second)
	{
		char firstName[200];
		char secondName[200];
		static_cast<const SCleanMapInfoData*>(first)->Tool_GetMapName(firstName);
		static_cast<const SCleanMapInfoData*>(second)->Tool_GetMapName(secondName);
		return Runtime_CompareStringsIgnoreCase(firstName, secondName);
	}

	// Original function: FUN_00416473. Sort record contents without relinking nodes.
	void CCleanMapsManager::CleanMapList_Tool_SortByNames()
	{
		const auto count = static_cast<std::int32_t>(mMaps.GetCount());
		if (count < 2)
			return;
		const std::uint32_t bytes = static_cast<std::uint32_t>(count) * sizeof(SCleanMapInfoData);
		auto* records = static_cast<SCleanMapInfoData*>(Runtime_AllocateWithNewHandler(bytes, 1));
		std::memset(records, 0, bytes);
		CListBase::Iterator iterator(mMaps);
		auto* destination = records;
		for (void* data = iterator.First(); data != nullptr; data = iterator.Next())
			std::memcpy(destination++, data, sizeof(SCleanMapInfoData));
		Runtime_QuickSort(records, static_cast<std::uint32_t>(count), sizeof(SCleanMapInfoData), CleanMap_CompareNames);
		CListBase::Iterator writeIterator(mMaps);
		auto* source = records;
		for (void* data = writeIterator.First(); data != nullptr; data = writeIterator.Next())
			std::memcpy(data, source++, sizeof(SCleanMapInfoData));
		Runtime_Free(records);
	}

	// Original function: FUN_00416581.
	SCleanMapInfoData* CCleanMapsManager::l_CleanMapList_GetPtrByGuid(const SCleanMapGuid& guid)
	{
		CListBase::Iterator iterator(mMaps);
		for (auto* data = static_cast<SCleanMapInfoData*>(iterator.First()); data != nullptr; data = static_cast<SCleanMapInfoData*>(iterator.Next()))
			if (std::memcmp(&data->guid, &guid, sizeof(guid)) == 0)
				return data;
		return nullptr;
	}

	// Original function: FUN_004165d1.
	void CCleanMapsManager::l_CleanMapList_FreeAll()
	{
		NC2Lifetime::DrainRuntimeList(&mMaps);
		std::memset(mMapTypeCounts, 0, sizeof(mMapTypeCounts));
	}

	// Original function: FUN_00416608.
	void CCleanMapsManager::l_CleanMapList_Update()
	{
		l_CleanMapList_FreeAll();
		std::memset(mMapTypeCounts, 0, sizeof(mMapTypeCounts));
		SDirectoryScannerState scanner;
		DirectoryScanner_Initialize(&scanner);
		for (bool found = DirectoryScanner_GetFirstMatchingFile(&scanner, "data\\maps\\", nullptr, true); found; found = DirectoryScanner_GetNextMatchingFile(&scanner))
		{
			const char* root = DirectoryScanner_GetCurrentNamePtr(&scanner);
			char filename[260];
			if (root == nullptr || std::strlen(root) + sizeof("map.ini") > sizeof(filename)) continue;
			std::strcpy(filename, root);
			std::strcat(filename, "map.ini");
			CFile::FileSystem_SystemPathName_Set("$maproot$", root);
			CIniFileReader reader(filename, nullptr, false, nullptr, nullptr);
			l_CleanMapList_Update_WorkOnMap(reader, root, false, nullptr);
			CFile::FileSystem_SystemPathName_Clear("$maproot$");
		}
		DirectoryScanner_Destroy(&scanner);
		DirectoryScanner_Initialize(&scanner);
		for (bool found = DirectoryScanner_GetFirstMatchingFile(&scanner, IO_Tool_GetUserMapDirectoryNamePtr(), "c2m", false); found; found = DirectoryScanner_GetNextMatchingFile(&scanner))
		{
			const char* filename = DirectoryScanner_GetCurrentNamePtr(&scanner);
			if (filename == nullptr) continue;
			void* archive = Runtime_AllocateWithNewHandler(20, 1);
			if (archive != nullptr)
				archive = Archive_Initialize(archive, filename);
			CFile::FileSystem_FileLibrary_Add(archive);
			CIniFileReader reader("currentusermap\\map.ini", nullptr, false, nullptr, nullptr);
			l_CleanMapList_Update_WorkOnMap(reader, nullptr, true, filename);
			CFile::FileSystem_FileLibrary_Remove(archive);
			if (archive != nullptr)
			{
				Archive_Destroy(archive);
				Runtime_Free(archive);
			}
			// The INI reader is destroyed after removal and deletion of the archive.
		}
		DirectoryScanner_Destroy(&scanner);
	}

	// Original function: FUN_004167a1; complete token regions: 004F37B0..004F37E7.
	bool CCleanMapsManager::l_CleanMapList_Update_WorkOnMap(CIniFileReader& reader, const char* mapRoot, bool userMap, const char* archiveFilename)
	{
		static constexpr SToken typeTokens[] = { { "maptype", SwitchIds::CleanMapToken::MapType },{ "mapcampaignid", SwitchIds::CleanMapToken::Mapcampaignid },{ "mapmultiplayeronly", SwitchIds::CleanMapToken::Mapmultiplayeronly },{ nullptr, 0 } };
		static constexpr SToken nameTokens[] = { { "mapnamestringid", NC2DataIds::CleanMapsManagerToken::MapNameStringId },{ "mapdescriptionstringid", NC2DataIds::CleanMapsManagerToken::MapDescriptionStringId },{ nullptr, 0 } };
		if (!reader.IsLoaded() || !reader.SearchGroup("logiccontrol"))
			return false;
		SCleanMapInfoData info;
		std::memset(&info, 0, sizeof(info));
		if (mapRoot != nullptr)
		{
			std::strncpy(info.mapRoot, mapRoot, 259);
			Runtime_LowercaseString(info.mapRoot);
		}
		else if (userMap)
			std::strcpy(info.mapRoot, "currentusermap");
		info.userMap = static_cast<std::uint8_t>(userMap);
		if (userMap)
			std::strncpy(info.archiveFilename, archiveFilename, 259);
		if (std::strstr(info.mapRoot, "system_rebuild") != nullptr)
			return false;
		reader.SetCurrentLineToGroupStart();
		if (reader.SearchCommand("version"))
			info.version = static_cast<std::uint32_t>(reader.GetInt());
		reader.SetCurrentLineToGroupStart();
		if (reader.SearchCommand("mapsize"))
		{
			info.width = static_cast<std::uint32_t>(reader.GetInt());
			info.height = static_cast<std::uint32_t>(reader.GetInt());
		}
		reader.SetCurrentLineToGroupStart();
		if (reader.SearchCommand("mapguid"))
			for (auto& byte : info.guid.bytes)
				byte = static_cast<std::uint8_t>(reader.GetInt());
		if (!info.guid.IsValid())
			return false;
		if (reader.SearchGroup("misc_maptype"))
		{
			CTokenList tokens(typeTokens);
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (tokens.FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::CleanMapToken::MapType:
						{
							const auto type = static_cast<std::uint32_t>(reader.GetInt());
							if (type > 0 && type < 7)
							{
								info.mapTypes |= 1u << type;
								++mMapTypeCounts[type];
							}
							break;
						}
					case SwitchIds::CleanMapToken::Mapcampaignid:
						if ((info.mapTypes & 2u) != 0)
						{
							info.campaignId = static_cast<std::uint32_t>(reader.GetInt());
							info.campaignMapId = static_cast<std::uint32_t>(reader.GetInt());
						}
						break;
					case SwitchIds::CleanMapToken::Mapmultiplayeronly:
						info.multiplayerOnly = 1;
						break;
				}
			}
		}
		if (reader.SearchGroup("misc_mapname"))
		{
			CTokenList tokens(nameTokens);
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				const std::uint32_t token = tokens.FindToken(reader.GetCurrentName());
				if (token != NC2DataIds::CleanMapsManagerToken::MapNameStringId && token != NC2DataIds::CleanMapsManagerToken::MapDescriptionStringId)
					continue;
				const auto id = static_cast<std::uint32_t>(reader.GetInt());
				CStringArray* strings = StringIO_GetMissionStringArrayPtr(info.mapRoot);
				if (strings == nullptr)
					continue;
				if (const char* text = strings->GetStringPtr(id))
				{
					char* destination = token == NC2DataIds::CleanMapsManagerToken::MapNameStringId ? info.mapName : info.description;
					const std::uint32_t capacity = token == NC2DataIds::CleanMapsManagerToken::MapNameStringId ? 200u : 400u;
					NXBasics::Text_Copy(destination, capacity, text);
					destination[capacity - 1] = '\0';
				}
				NC2Lifetime::DeleteStorable(strings);
			}
		}
		if (SCleanMapInfoData* existing = l_CleanMapList_GetPtrByGuid(info.guid))
		{
			std::memcpy(existing, &info, sizeof(info));
			return true;
		}
		auto* data = static_cast<SCleanMapInfoData*>(Runtime_AllocateWithNewHandler(sizeof(info), 1));
		std::memset(data, 0, sizeof(info));
		std::memcpy(data, &info, sizeof(info));
		CListBase::Iterator iterator(mMaps);
		for (auto* current = static_cast<SCleanMapInfoData*>(iterator.First()); current != nullptr; current = static_cast<SCleanMapInfoData*>(iterator.Next()))
		{
			if (Runtime_CompareStringsIgnoreCase(current->mapRoot, data->mapRoot) > 0)
			{
				mMaps.InsertBefore(current, data);
				return true;
			}
		}
		mMaps.InsertAtEnd(data);
		return true;
	}

	// Original function: FUN_00416b2d.
	void CCleanMapsManager::Tool_GenerateNewGuid(SCleanMapGuid* guid)
	{
		std::memset(guid, 0, sizeof(*guid));
		GUID generated;
		if (CoCreateGuid(&generated) == S_OK)
			std::memcpy(guid, &generated, sizeof(*guid));
	}

	// Original function: FUN_00416b63.
	bool SCleanMapGuid::IsValid() const
	{
		for (std::uint8_t byte : bytes)
			if (byte != 0)
				return true;
		return false;
	}

	// Original function: FUN_00416b77.
	bool SCleanMapGuid::IsEqualTo(const SCleanMapGuid& other) const
	{
		for (std::uint32_t index = 0; index < sizeof(bytes); ++index)
			if (bytes[index] != other.bytes[index])
				return false;
		return true;
	}

	// Original function: FUN_00416b98.
	void SCleanMapInfoData::Tool_GetMapName(char* destination) const
	{
		if (mapName[0] != '\0')
			std::strcpy(destination, mapName);
		else
			Runtime_PrintString(destination, "%s - %d * %d", mapRoot, width, height);
	}

	// Original function: FUN_00416bd6.
	const char* SCleanMapInfoData::Tool_GetMapDescriptionPtr() const
	{
		return description[0] != '\0' ? description : nullptr;
	}
}
