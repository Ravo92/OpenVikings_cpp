#pragma once

#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
}

namespace NC2E2
{
	void DynamicDataHumanManager_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void DynamicDataHumanManager_IO_Savegame_Load(NC2Logic::CIoHelper* io);
	void DynamicDataHouseManager_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void DynamicDataHouseManager_IO_Savegame_Load(NC2Logic::CIoHelper* io);
	void DynamicDataVehicleManager_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void DynamicDataVehicleManager_IO_Savegame_Load(NC2Logic::CIoHelper* io);
	void DynamicDataAnimalManager_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void DynamicDataAnimalManager_IO_Savegame_Load(NC2Logic::CIoHelper* io);
	void EngineMap_IO_Cleanmap_Save(NC2Logic::CIoHelper* io);
	void EngineMap_IO_Cleanmap_Load(NC2Logic::CIoHelper* io);
	void EngineMap_IO_Savegame_Save(NC2Logic::CIoHelper* io);
	void EngineMap_IO_Savegame_Load(NC2Logic::CIoHelper* io);
	bool __cdecl E2Manager_sl_Callback_MapIO(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
}