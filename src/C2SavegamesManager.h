#pragma once

#include "C2CleanMapsManager.h"

namespace NC2Logic
{
	struct SSavegamePlayerInfoData
	{
		std::uint32_t playerType;
		char name[40];
		std::uint32_t unknown2C;
		std::uint32_t colorId;                  // +0x30; restored lobby/player color.
	};

	struct SSavegameInfoHeaderData
	{
		std::uint32_t version;                   // +0x000
		char info[200];                          // +0x004
		std::byte padding0CC[2];
		char mapName[200];                       // +0x0CE
		std::byte padding196[2];
		std::int32_t slotId;                     // +0x198
		std::uint32_t creationTimeFields[7];     // +0x19C; original comparison order
		SCleanMapGuid mapGuid;                   // +0x1B8
		std::uint8_t multiplayer;                // +0x1C8
		std::byte padding1C9[3];
		SSavegamePlayerInfoData players[20];     // +0x1CC
	};

	struct SSavegameInfoData
	{
		char filename[100];                     // +0x000
		std::uint32_t uniqueId;                  // +0x064
		SSavegameInfoHeaderData header;          // +0x068
		bool Tool_IsSystemSavegame() const;
		void Tool_CreateSavegameButtonName(char* destination) const;
	};

	class CSavegameInfoList final : public NXBasics::CListBase
	{
	public:
		explicit CSavegameInfoList(bool deleteObjects) : CListBase(deleteObjects)
		{}
		~CSavegameInfoList() override;
		void* Delete(std::uint32_t flags);
	protected:
		void l_DeleteData(void* data) override
		{
			NXBasics::Runtime_Free(data);
		}
	};

	class CSavegamesManager
	{
	public:
		CSavegamesManager();
		~CSavegamesManager();
		void SavegameList_Update(std::uint32_t savegameType, bool force);
		SSavegameInfoData* GetSavegameInfoPtrByUniqueId(std::uint32_t uniqueId);
		SSavegameInfoData* GetSavegameInfoPtrByFilename(const char* filename);
		void SortType_Set(std::uint32_t sortType);
		void Tool_CreateFullSavegameFilename(const char* filename, char* destination) const;
		const char* Tool_GetCurrentPlayerSavegameDirectoryNamePtr() const;
		static std::uint32_t Tool_ConvertSlotIdToType(std::int32_t slotId);

		std::uint32_t mNextUniqueId;             // +0x00
		std::uint32_t mSavegameType;             // +0x04
		std::uint32_t mSortType;                 // +0x08
		CSavegameInfoList mSavegames; // +0x0C

	private:
		void l_SavegameList_FreeAll();
		void l_SavegameList_Update();
		void l_SavegameList_InsertSorted(SSavegameInfoData* data);
	};

	// Complete CSavegamesManager::mStaticVars region at 005113A8..005114AB.
	inline char gCurrentPlayerSavegameDirectory[260]{ };
	static_assert(sizeof(SSavegamePlayerInfoData) == 52);
	static_assert(offsetof(SSavegamePlayerInfoData, playerType) == 0);
	static_assert(offsetof(SSavegamePlayerInfoData, colorId) == 0x30);
	static_assert(sizeof(SSavegameInfoHeaderData) == 1500);
	static_assert(offsetof(SSavegameInfoHeaderData, slotId) == 0x198);
	static_assert(offsetof(SSavegameInfoHeaderData, creationTimeFields) == 0x19C);
	static_assert(offsetof(SSavegameInfoHeaderData, players) == 0x1CC);
	static_assert(sizeof(SSavegameInfoData) == 1604);
	static_assert(sizeof(CSavegamesManager) == (sizeof(void*) == 8 ? 56 : 32));
	static_assert(offsetof(CSavegamesManager, mSavegames) == (sizeof(void*) == 8 ? 16 : 12));
}
