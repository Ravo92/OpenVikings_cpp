#pragma once
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
	void __fastcall GameMap_IO_Savegame_DoSave(void* map, void* unusedEdx, CIoHelper* io);
	void* __fastcall GameMap_ConstructFromSavegame(void* map, void* unusedEdx, CIoHelper* io);
	void CleanMapGuid_InitWhenNotSet();
	void GameIO_Cleanmap_Load(const char* mapRoot, bool enableTime, bool retainIni, bool networkGame);
	bool GameIO_Cleanmap_LoadX(const char* mapRoot, const char* libraryFilename, bool enableTime, bool retainIni, bool networkGame);
	bool GameMap_IO_Cleanmap_Load(CIoHelper* io);
	bool GameMap_l_GlobalIO_Load(CIoHelper* io);
}