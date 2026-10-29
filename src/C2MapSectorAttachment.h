#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
	struct SSectorGridCursor;

	class CGameMapSectorAttachmentManager
	{
	  public:
		// Original data: DAT_00550e68.
		inline static CGameMapSectorAttachmentManager* sTheObjectPtr = nullptr;

		// Original function: FUN_0044ebd7.
		CGameMapSectorAttachmentManager();
		explicit CGameMapSectorAttachmentManager(CIoHelper& io);
		void IO_Savegame_DoSave(CIoHelper& io);
		bool CheckForEnemyTarget_Human(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target, bool ignoreAnimals, bool allowLandscape);

		// Original function: FUN_0044ebf8.
		~CGameMapSectorAttachmentManager();
		CGameMapSectorAttachmentManager(const CGameMapSectorAttachmentManager&) = delete;
		CGameMapSectorAttachmentManager& operator=(const CGameMapSectorAttachmentManager&) = delete;

		std::int32_t GetGridWidth() const noexcept
		{

			return mGridWidth;
		}

		const std::byte* GetSectorData() const noexcept
		{

			return mSectorData;
		}

		std::byte* GetSectorData() noexcept
		{

			return mSectorData;
		}

	  private:
		// Original function: FUN_0044ec28.
		void l_InitSizeAndAllocateSectorArray();
		bool l_CheckSectorForEnemyTarget_Human(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target, bool ignoreAnimals, bool allowLandscape);

		std::int32_t mGridHeight;        // +0x00: number of sector rows.
		std::int32_t mGridWidth;         // +0x04: number of sector columns.
		std::byte* mSectorData;          // +0x08: 0x540 bytes per sector.
		std::byte* mSectorAuxiliaryData; // +0x0C: 21 player records of 12 bytes per sector; enemy-search cache.
	};

	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForSector(const SSectorGridCursor& cursor, std::int32_t player);
	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForPosition(const void* point, std::int32_t player);
	std::int32_t AIMilitaryManager_Military_GetDangerValueForPosition(const void* point, std::int32_t player);
	std::int32_t AIMilitaryManager_Military_GetDangerValueForSector(const SSectorGridCursor& cursor, std::int32_t player);
	std::int32_t AIMilitaryManager_Military_GetCivilianDangerValueForPosition(const void* point, std::int32_t player, std::uint32_t radius);

	static_assert(sizeof(CGameMapSectorAttachmentManager) == (sizeof(void*) == 8 ? 24 : 16));
}