#pragma once

#include "C2CallbackManager.h"
#include "C2ObjectLayout.h"
#include "XList.h"
#include <cstddef>

namespace NC2E2
{
	// Windows cache payloads own their loaded storable; the two manager fields borrow it.
	struct SStaticBobCacheEntryStorage
	{
		char filename[80]; void* bobManager;
	};
	struct SStaticBobCacheManagerStorage
	{
		alignas(NXBasics::CListBase) std::byte inheritedList[sizeof(NXBasics::CListBase)]; void* fishBobManager; void* guiBubblesBobManager;
	};

	static_assert(sizeof(SStaticBobCacheEntryStorage) == (sizeof(void*) == 8 ? 88 : 84));
	static_assert(offsetof(SStaticBobCacheEntryStorage, bobManager) == NC2Layout::StaticBobCacheEntryOffset::BobManager);
	static_assert(sizeof(SStaticBobCacheManagerStorage) == (sizeof(void*) == 8 ? 56 : 28));
	static_assert(offsetof(SStaticBobCacheManagerStorage, fishBobManager) == NC2Layout::StaticBobCacheManagerOffset::FishBobManager);
	static_assert(offsetof(SStaticBobCacheManagerStorage, guiBubblesBobManager) == NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager);
	static_assert(alignof(SStaticBobCacheManagerStorage) >= alignof(NXBasics::CListBase));

	// Windows weather data is raw storage; these records document its exact layout.
	struct SWeatherSector
	{
		std::int32_t rainDensity;
		std::int32_t snowDensity;
		std::int32_t sandDensity;
	};

	struct SWeatherSectorManagerData
	{
		std::int32_t sectorWidth;
		std::int32_t sectorHeight;
		SWeatherSector* sectors;
	};

	static_assert(offsetof(SWeatherSectorManagerData, sectorWidth) == 0);
	static_assert(offsetof(SWeatherSectorManagerData, sectorHeight) == 4);
	static_assert(offsetof(SWeatherSectorManagerData, sectors) == 8);
	static_assert(sizeof(SWeatherSector) == 12);
	static_assert(sizeof(SWeatherSectorManagerData) == (sizeof(void*) == 8 ? 16 : 12));

	void* E2MapWeatherManagerSectorManager_GetExistingObjectPtr();
	extern void* gE2Manager;
	void E2MapWeatherManagerSectorManager_WeatherSetRectangle(void* manager, std::uint32_t first, std::uint32_t second, std::int32_t type, std::int32_t density);
	std::int32_t E2MapWeatherManagerSectorManager_Snow_GetDensity(void* manager, std::uint32_t point, bool ignoreDark, std::int32_t playerId);
	std::int32_t E2MapWeatherManagerSectorManager_Rain_GetDensity(void* manager, std::uint32_t point, bool ignoreDark, std::int32_t playerId);
	std::int32_t E2MapWeatherManagerSectorManager_Sand_GetDensity(void* manager, std::uint32_t point, bool ignoreDark, std::int32_t playerId);
	void E2Manager_Destruct(void* manager);
	void* __fastcall E2MapWeatherManagerSectorManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2StaticDataTexturesManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2StaticDataBobManagersManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2StaticDataBobManagersManager_GetBobManagerPtr(void* manager, void* unusedEdx, const char* fileName);
	const char* __fastcall E2StaticDataBobManagersManager_GetBobManagerFilenamePtr(void* manager, void* unusedEdx, const void* bobManager);
	void E2MapWeatherManagerSectorManager_FreeSectorData(void* manager);
	void E2ParticelManager_Initialize(void* manager);
	void* __fastcall E2ParticelManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2DynamicDataHumanManager_Construct(void* manager, void* unusedEdx);
	void WorldMusic_Update(std::uint32_t gameTime);
	void WorldMusic_UpdateCdTrack();
	void* __fastcall WorldMusic_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2HumanSoundManager_Construct(void* manager, void* unusedEdx);
	bool __cdecl E2Manager_CallbackSomethingInLogicXtructed(NC2Logic::TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	bool __cdecl E2MapWeatherManagerSectorManager_LogicCallback(NC2Logic::TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	bool __cdecl E2ParticelManager_Callback(NC2Logic::TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	bool __cdecl E2AmbientSoundManager_Callback(NC2Logic::TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	bool __cdecl WorldMusic_Callback(NC2Logic::TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
}

namespace NC2LogicExternals
{
	void External_FX_Weather_SetRectangle(std::uint32_t first, std::uint32_t second, bool snow, std::int32_t densityPercent);
}
