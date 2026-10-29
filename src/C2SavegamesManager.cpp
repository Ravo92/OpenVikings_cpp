#include "C2LifetimeGlobals.h"
#include "C2LogicIOAccess.h"
#include "C2SavegameIO.h"
#include "C2SavegamesManager.h"
#include "XDirectoryScanner.h"

namespace NC2Logic
{
	using namespace NXBasics;

	// Original function: FUN_0041637e; CList<SSavegameInfoData> destructor.
	CSavegameInfoList::~CSavegameInfoList()
	{
		if (mDeleteObjects != 0)
			DeleteAllElements();
	}

	// Original function: FUN_0041639b; native scalar deleting-destructor adapter.
	void* CSavegameInfoList::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CSavegameInfoList();

		if ((flags & 1u) != 0)
			Runtime_Free(result);

		return result;
	}

	// Original function: FUN_00415df6.
	CSavegamesManager::CSavegamesManager() : mSavegames(false)
	{
		gSavegamesManager = this;
		std::memset(this, 0, 12);
		std::memset(gCurrentPlayerSavegameDirectory, 0, sizeof(gCurrentPlayerSavegameDirectory));
		mNextUniqueId = 1;
		mSavegameType = 1;
		mSortType = 3;
		l_SavegameList_Update();
	}

	// Original function: FUN_00415e4b; member-list destructor: FUN_0041637e.
	CSavegamesManager::~CSavegamesManager()
	{
		l_SavegameList_FreeAll();
		gSavegamesManager = nullptr;
	}

	// Original function: FUN_00415e64.
	void CSavegamesManager::SavegameList_Update(std::uint32_t savegameType, bool force)
	{
		if (savegameType == 0)
			savegameType = gGameStaticVars.networkGame != 0 ? 2u : 1u;

		if (mSavegameType != savegameType || force)
		{
			mSavegameType = savegameType;
			l_SavegameList_Update();
		}
	}

	// Original function: FUN_00415e8d.
	SSavegameInfoData* CSavegamesManager::GetSavegameInfoPtrByUniqueId(std::uint32_t uniqueId)
	{
		CListBase::Iterator iterator(mSavegames);

		for (auto* data = static_cast<SSavegameInfoData*>(iterator.First()); data != nullptr; data = static_cast<SSavegameInfoData*>(iterator.Next()))
		{
			if (data->uniqueId == uniqueId)
				return data;
		}

		return nullptr;
	}

	// Original function: FUN_00415ec5.
	SSavegameInfoData* CSavegamesManager::GetSavegameInfoPtrByFilename(const char* filename)
	{
		CListBase::Iterator iterator(mSavegames);

		for (auto* data = static_cast<SSavegameInfoData*>(iterator.First()); data != nullptr; data = static_cast<SSavegameInfoData*>(iterator.Next()))
		{
			if (Runtime_CompareStringsIgnoreCase(data->filename, filename) == 0)
				return data;
		}

		return nullptr;
	}

	// Original function: FUN_00415f0c. The unique-ID counter survives reloads.
	void CSavegamesManager::l_SavegameList_FreeAll()
	{
		NC2Lifetime::DrainRuntimeList(&mSavegames);
	}

	// Original function: FUN_00415f32.
	void CSavegamesManager::l_SavegameList_Update()
	{
		l_SavegameList_FreeAll();
		SDirectoryScannerState scanner;
		DirectoryScanner_Initialize(&scanner);

		for (bool found = DirectoryScanner_GetFirstMatchingFile(&scanner, Tool_GetCurrentPlayerSavegameDirectoryNamePtr(), ".sav", false); found; found = DirectoryScanner_GetNextMatchingFile(&scanner))
		{
			const char* filename = DirectoryScanner_GetCurrentNamePtr(&scanner);

			if (filename == nullptr)
				continue;

			auto* data = static_cast<SSavegameInfoData*>(Runtime_AllocateWithNewHandler(sizeof(SSavegameInfoData), 1));

			if (data == nullptr)
				continue;

			std::memset(data, 0, sizeof(*data));
			char basename[260];
			Runtime_SplitPath(filename, nullptr, nullptr, basename, nullptr);
			std::strncpy(data->filename, basename, 99);
			data->uniqueId = mNextUniqueId++;

			if (GameIO_Savegame_GetInfoHeader(data->filename, &data->header) == -1)
			{
				Runtime_Free(data);
			}
			else
			{
				l_SavegameList_InsertSorted(data);
			}
		}

		DirectoryScanner_Destroy(&scanner);
	}

	// Original function: FUN_00416001.
	void CSavegamesManager::l_SavegameList_InsertSorted(SSavegameInfoData* data)
	{
		if (mSavegames.GetCount() == 0)
		{
			mSavegames.InsertAtStart(data);
			return;
		}

		CListBase::Iterator iterator(mSavegames);

		if (mSortType >= 1 && mSortType <= 3)
		{
			for (auto* current = static_cast<SSavegameInfoData*>(iterator.First()); current != nullptr; current = static_cast<SSavegameInfoData*>(iterator.Next()))
			{
				bool insert = false;

				if (mSortType == 1)
				{
					insert = Runtime_CompareStringsIgnoreCase(current->header.mapName, data->header.mapName) > 0;
				}
				else if (mSortType == 3)
				{
					insert = current->header.slotId > data->header.slotId;
				}
				else
				{
					// Unsigned descending comparison; equal timestamps insert before.
					insert = true;

					for (std::uint32_t index = 0; index < 7; ++index)
					{
						const auto first = current->header.creationTimeFields[index];
						const auto second = data->header.creationTimeFields[index];

						if (first != second)
						{
							insert = first < second;
							break;
						}
					}
				}

				if (insert)
				{
					mSavegames.InsertBefore(current, data);

					return;
				}
			}
		}

		mSavegames.InsertAtEnd(data);
	}

	// Original function: FUN_00416193.
	void CSavegamesManager::SortType_Set(std::uint32_t sortType)
	{
		mSortType = sortType;
		l_SavegameList_Update();
	}

	// Original function: FUN_004161a2.
	void CSavegamesManager::Tool_CreateFullSavegameFilename(const char* filename, char* destination) const
	{
		Runtime_PrintString(destination, "%s%s%s", Tool_GetCurrentPlayerSavegameDirectoryNamePtr(), filename, ".sav");
	}

	// Original function: FUN_004161c5.
	const char* CSavegamesManager::Tool_GetCurrentPlayerSavegameDirectoryNamePtr() const
	{
		const char* player = gGameStaticVars.networkGame == 0 && mSavegameType != 2 ? Player_GetSystemPlayerFileSystemName() : "multiplayer";
		Runtime_PrintString(gCurrentPlayerSavegameDirectory, "%s\\%s\\%s\\", IO_Tool_GetPlayerDataDirectoryNamePtr(), player, "savegames");

		return gCurrentPlayerSavegameDirectory;
	}

	// Original function: FUN_00416204.
	std::uint32_t CSavegamesManager::Tool_ConvertSlotIdToType(std::int32_t slotId)
	{
		if (slotId < 1000)
			return 1;

		if (slotId < 2000)
			return 2;

		if (slotId < 3000)
			return 3;

		return slotId < 8000 ? 4u : 5u;
	}

	// Original function: FUN_00416239.
	bool SSavegameInfoData::Tool_IsSystemSavegame() const
	{

		return Runtime_CompareStringsIgnoreCase(filename, "sys_continue") == 0;
	}

	// Original function: FUN_0041624c.
	void SSavegameInfoData::Tool_CreateSavegameButtonName(char* destination) const
	{
		char buffer[100];
		destination[0] = '\0';

		if (header.slotId >= 1000 && header.slotId < 8000)
		{
			const std::int32_t base = header.slotId < 2000 ? 1000 : (header.slotId < 3000 ? 2000 : 3000);
			Runtime_PrintString(buffer, "#%d - ", header.slotId - base);
			std::strcat(destination, buffer);
		}

		std::strcat(destination, std::strlen(header.mapName) != 0 ? header.mapName : header.info);
		Runtime_PrintString(buffer, "%d.%d.%d - %d:%2.2d:%2.2d", header.creationTimeFields[2], header.creationTimeFields[1], header.creationTimeFields[0], header.creationTimeFields[3], header.creationTimeFields[4], header.creationTimeFields[5]);
		std::strcat(destination, " - ");
		std::strcat(destination, buffer);
	}
}