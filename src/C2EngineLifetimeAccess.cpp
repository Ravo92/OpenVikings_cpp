#include "ApplicationGlobals.h"
#include "CivilizationBuildings.h"
#include "C2DataIds.h"
#include "C2E2AmbientSoundManagerAccess.h"
#include "C2E2AnimalSoundManager.h"
#include "C2E2LevelOfDetailManager.h"
#include "C2E2StaticDataAnimalSounds.h"
#include "C2E2StaticDataMapMoveables.h"
#include "C2E2StaticDataParticles.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineMacroMaps.h"
#include "C2EngineMapIO.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2EngineObjectData.h"
#include "C2EngineParticleSimulation.h"
#include "C2EngineTerrainData.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "CDAudioManager.h"
#include "DMManager.h"
#include "DSManager.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XStorableIO.h"
#include <new>
#include <utility>
#include <cmath>

namespace NC2E2
{
	using namespace NC2Lifetime;
	// Original function: FUN_0047dc9e.
	void* E2MapWeatherManagerSectorManager_GetExistingObjectPtr()
	{
		return gE2MapWeatherManagerSectorManager;
	}
	SStaticDataTextures gStaticDataTextures{ };
	NXBasics::CPalette*& gVertexColorsPalette = gStaticPalettes.vertexColorsPalette;
	extern bool __cdecl E2Manager_sl_Callback_Misc(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	extern bool __cdecl E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	struct SStaticTextureListTag
	{};
	struct SStaticBobManagerListTag
	{};

	static NXBasics::SToken gHumanSoundTokens[5]{ }; // Complete 00568388..005683AF.
	static std::uint8_t gHumanSoundTokenFlags = 0; // 00568380.
	// Original function: FUN_004854d9.
	static void E2HumanSoundManager_Load(void* manager, NXBasics::CIniFileReader& reader)
	{
		if ((gHumanSoundTokenFlags & 1u) == 0)
		{
			gHumanSoundTokenFlags |= 1u;
			static constexpr NXBasics::SToken tokens[5] = { { "logictribe", NC2DataIds::EngineLifetimeToken::LogicTribe },{ "scream", NC2DataIds::EngineLifetimeToken::Scream },{ "generic", NC2DataIds::EngineLifetimeToken::Generic },{ "respond", NC2DataIds::EngineLifetimeToken::Respond },{ nullptr, 0 } };
			std::memcpy(gHumanSoundTokens, tokens, sizeof(tokens));
		}
		for (char* group = reader.GetFirstGroup("sounds"); group != nullptr; group = reader.GetNextGroup("sounds"))
		{
			reader.GetNextCommandLineInGroup();
			std::uint32_t tribe = 0;
			if (NXBasics::Runtime_CompareStringsIgnoreCase("logictribe", reader.GetCurrentName()) == 0) tribe = static_cast<std::uint32_t>(reader.GetInt());
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				const auto token = NXBasics::XB_Token_FindTokenInArray(gHumanSoundTokens, reader.GetCurrentName());
				if (token == NC2DataIds::EngineLifetimeToken::Scream || token == NC2DataIds::EngineLifetimeToken::Generic)
				{
					const auto sexAge = static_cast<std::uint32_t>(reader.GetInt());
					auto* soundManager = CE2StaticDataSoundFXManager::sTheObjectPtr;
					const char* name = reader.GetString();
					const auto id = soundManager->GetStaticSoundFxIdByNamePtr(name);
					Write<std::uint32_t>(manager, (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + sexAge) * NC2Layout::RecordSize::HumanSoundRecordUnsigned + (token == NC2DataIds::EngineLifetimeToken::Scream ? NC2Layout::HumanSoundManagerViewOffset::FirstScreamSoundIdUnsigned : NC2Layout::HumanSoundManagerViewOffset::FirstGenericSoundIdUnsigned), id);
				}
				else if (token == NC2DataIds::EngineLifetimeToken::Respond)
				{
					const auto sexAge = static_cast<std::uint32_t>(reader.GetInt());
					const auto response = static_cast<std::uint32_t>(reader.GetInt());
					auto* soundManager = CE2StaticDataSoundFXManager::sTheObjectPtr;
					const char* name = reader.GetString();
					const auto id = soundManager->GetStaticSoundFxIdByNamePtr(name);
					const auto offset = (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + sexAge) * NC2Layout::RecordSize::HumanSoundRecordUnsigned + response * NC2Layout::RecordSize::HumanSoundResponseGroupUnsigned + NC2Layout::HumanSoundManagerViewOffset::FirstResponsesUnsigned;
					const auto count = Read<std::uint32_t>(manager, offset);
					if (count < NC2Runtime::HumanSoundLimits::ResponseSoundsUnsigned)
					{
						Write<std::uint32_t>(manager, offset + count * NC2Layout::RecordSize::StaticSoundIdUnsigned + NC2Layout::HumanSoundResponseGroupOffset::SoundIdsUnsigned, id);
						Write<std::uint32_t>(manager, offset, count + 1u);
					}
				}
			}
		}
	}
	// Original function: FUN_00485453; CE2HumanSoundManager constructor.
	void* __fastcall E2HumanSoundManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2HumanSoundManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::E2HumanSoundManager);
		for (std::uint32_t record = 0; record < NC2Runtime::HumanSoundLimits::Records; ++record)
		{
			Write<std::uint32_t>(manager, NC2Layout::HumanSoundManagerViewOffset::FirstScreamSoundIdUnsigned + record * NC2Layout::RecordSize::HumanSoundRecordUnsigned, NXBasics::IntegerRange::UInt32Maximum);
			Write<std::uint32_t>(manager, NC2Layout::HumanSoundManagerViewOffset::FirstGenericSoundIdUnsigned + record * NC2Layout::RecordSize::HumanSoundRecordUnsigned, NXBasics::IntegerRange::UInt32Maximum);
		}
		if (NDSManager::CManager::sTheObjectPtr != nullptr && Read<std::uint8_t>(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Initialized) != 0)
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\humans\\sounds.ini", nullptr, false, nullptr, nullptr);
			E2HumanSoundManager_Load(manager, reader);
		}
		return manager;
	}

	// Original function: FUN_0049063f; CE2StaticDataBobManagersManager::GetBobManagerPtr.
	void* __fastcall E2StaticDataBobManagersManager_GetBobManagerPtr(void* manager, void* /*unusedEdx*/, const char* fileName)
	{
		auto* list = static_cast<NXBasics::CListBase*>(manager);
		NXBasics::CListBase::Iterator iterator(*list);
		for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next())
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(static_cast<const char*>(entry), fileName) == 0) return Read<void*>(entry, NC2Layout::StaticBobCacheEntryOffset::BobManager);
		}
		if (!NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true)) return nullptr;
		void* entry = NXBasics::Runtime_Allocate(NC2Layout::RecordSize::StaticBobCacheEntry);
		std::memset(entry, 0, NC2Layout::RecordSize::StaticBobCacheEntry);
		std::strncpy(static_cast<char*>(entry), fileName, NC2Runtime::StaticBobCacheLimits::FilenameCopyLength);
		Write<void*>(entry, NC2Layout::StaticBobCacheEntryOffset::BobManager, NXBasics::XB_Storable_LoadObject(fileName));
		list->InsertAtEnd(entry);
		return Read<void*>(entry, NC2Layout::StaticBobCacheEntryOffset::BobManager);
	}

	// Original function: FUN_004905b9; Windows clears the entire inherited list header.
	void* __fastcall E2StaticDataBobManagersManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		::new (manager) CRuntimeOwnedList<SStaticBobManagerListTag>(false);
		gE2StaticDataBobManagersManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::E2StaticDataBobManagersManager);
		Write<void*>(manager, NC2Layout::StaticBobCacheManagerOffset::FishBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(manager, nullptr, "data\\engine2d\\bin\\bobs\\LS_fishes.bmd"));
		Write<void*>(manager, NC2Layout::StaticBobCacheManagerOffset::GuiBubblesBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(manager, nullptr, "data\\engine2d\\bin\\bobs\\ls_gui_bubbles.bmd"));
		return manager;
	}

	// Original function: FUN_00484bd1.
	void* __fastcall E2MapWeatherManagerSectorManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2MapWeatherManagerSectorManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::E2MapWeatherManagerSectorManager);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(SwitchIds::CallbackType::MapLifetime, E2MapWeatherManagerSectorManager_LogicCallback, 0x32, 0xC8);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(SwitchIds::CallbackType::LoadDone, E2MapWeatherManagerSectorManager_LogicCallback, 0x32, 0xC8);
		return manager;
	}

	// Original function: FUN_0049039e; the Windows constructor clears its list header.
	void* __fastcall E2StaticDataTexturesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		::new (manager) CRuntimeOwnedList<SStaticTextureListTag>(false);
		gE2StaticDataTexturesManager = manager;
		std::memset(manager, 0, 20);
		std::memset(&gStaticDataTextures, 0, 8);
		return manager;
	}

	// Original function: FUN_0048d109; direct destruction bypasses any cleared list vptr.
	static void StaticMoveableAnimationList_Destruct(void* list)
	{
		static_cast<CRuntimeOwnedList<SStaticMoveableAnimationListTag>*>(list)->CRuntimeOwnedList<SStaticMoveableAnimationListTag>::~CRuntimeOwnedList();
	}
	// Original function: FUN_0048d126.
	static void StaticMoveableWalkList_Destruct(void* list)
	{
		static_cast<CRuntimeOwnedList<SStaticMoveableWalkListTag>*>(list)->CRuntimeOwnedList<SStaticMoveableWalkListTag>::~CRuntimeOwnedList();
	}
	// Original function: FUN_00490580.
	static void StaticTextureList_Destruct(void* list)
	{
		static_cast<CRuntimeOwnedList<SStaticTextureListTag>*>(list)->CRuntimeOwnedList<SStaticTextureListTag>::~CRuntimeOwnedList();
	}
	// Original function: FUN_00490702.
	static void StaticBobManagerList_Destruct(void* list)
	{
		static_cast<CRuntimeOwnedList<SStaticBobManagerListTag>*>(list)->CRuntimeOwnedList<SStaticBobManagerListTag>::~CRuntimeOwnedList();
	}

	// Original function: FUN_0048d15c.
	void* __fastcall StaticMoveableAnimationList_Delete(void* list, void* /*unusedEdx*/, std::uint32_t flags)
	{
		StaticMoveableAnimationList_Destruct(list);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(list);
		return list;
	}
	// Original function: FUN_0048d178.
	void* __fastcall StaticMoveableWalkList_Delete(void* list, void* /*unusedEdx*/, std::uint32_t flags)
	{
		StaticMoveableWalkList_Destruct(list);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(list);
		return list;
	}
	// Original function: FUN_0049059d.
	void* __fastcall StaticTextureList_Delete(void* list, void* /*unusedEdx*/, std::uint32_t flags)
	{
		StaticTextureList_Destruct(list);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(list);
		return list;
	}
	// Original function: FUN_0049071f.
	void* __fastcall StaticBobManagerList_Delete(void* list, void* /*unusedEdx*/, std::uint32_t flags)
	{
		StaticBobManagerList_Destruct(list);
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(list);
		return list;
	}

	// Original function: FUN_00484ca5; Mac l_SectorData_Free.
	void E2MapWeatherManagerSectorManager_FreeSectorData(void* manager)
	{
		if (Read<void*>(manager, NC2Layout::WeatherSectorManagerOffset::Sectors) != nullptr)
			FreeAndClearField(manager, NC2Layout::WeatherSectorManagerOffset::Sectors);
		Write<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight, 0);
		Write<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth, 0);
	}
	// Original function: FUN_00484c4f; l_SectorData_Create.
	static void E2MapWeatherManagerSectorManager_CreateSectorData(void* manager)
	{
		E2MapWeatherManagerSectorManager_FreeSectorData(manager);
		Write<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth, (static_cast<std::int32_t>(NC2Logic::GameMap_GetWidth()) + NC2Runtime::WeatherGrid::SectorRoundUp) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints);
		Write<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight, (static_cast<std::int32_t>(NC2Logic::GameMap_GetHeight()) + NC2Runtime::WeatherGrid::SectorRoundUp) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints);
		const std::uint32_t size = Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight) * NC2Layout::RecordSize::WeatherSector;
		void* sectors = NXBasics::Runtime_Allocate(size);
		Write<void*>(manager, NC2Layout::WeatherSectorManagerOffset::Sectors, sectors);
		const std::uint32_t clearSize = Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight) * NC2Layout::RecordSize::WeatherSector;
		std::memset(sectors, 0, clearSize);
	}
	// Original function: FUN_004853d6; l_Sector_GetDataPtr.
	static void* E2MapWeatherManagerSectorManager_GetDataPtr(void* manager, std::int32_t x, std::int32_t y)
	{
		if (x < 0) x = 0;
		const auto width = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth);
		if (x >= width) x = width - 1;
		if (y < 0) y = 0;
		const auto height = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight);
		if (y >= height) y = height - 1;
		const std::uint32_t offset = (static_cast<std::uint32_t>(width) * static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(x)) * NC2Layout::RecordSize::WeatherSector;
		return static_cast<std::byte*>(Read<void*>(manager, NC2Layout::WeatherSectorManagerOffset::Sectors)) + offset;
	}
	// Original function: FUN_0048534a.
	static std::int32_t E2MapWeatherManagerSectorManager_Coordinates_GetInSectorXPercentage(const void* manager, const void* point)
	{
		const auto x = static_cast<std::int32_t>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X));
		const auto sector = (x - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		if (sector < 0 || sector >= Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth)) return 0;
		const auto percentage = (x - sector * NC2Runtime::WeatherGrid::SectorWidthInMapPoints - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) * NC2Runtime::WeatherGrid::FullPercentage / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		return percentage < 0 ? 0 : percentage > NC2Runtime::WeatherGrid::FullPercentage ? NC2Runtime::WeatherGrid::FullPercentage : percentage;
	}

	// Original function: FUN_0048538f.
	static std::int32_t E2MapWeatherManagerSectorManager_Coordinates_GetInSectorYPercentage(const void* manager, const void* point)
	{
		const auto y = static_cast<std::int32_t>(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y));
		const auto sector = (y - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		if (sector < 0 || sector >= Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight)) return 0;
		const auto percentage = (y - sector * NC2Runtime::WeatherGrid::SectorWidthInMapPoints - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) * NC2Runtime::WeatherGrid::FullPercentage / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		return percentage < 0 ? 0 : percentage > NC2Runtime::WeatherGrid::FullPercentage ? NC2Runtime::WeatherGrid::FullPercentage : percentage;
	}

	// Original function: FUN_00484d77; the Windows body does not read ignoreDark or playerId.
	std::int32_t E2MapWeatherManagerSectorManager_Snow_GetDensity(void* manager, std::uint32_t point, bool /*ignoreDark*/, std::int32_t /*playerId*/)
	{
		NC2Logic::MapMigPoint_ValidatePosition(&point);
		const auto x = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto y = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto width = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth);
		const auto height = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight);
		const auto sectorX = x < 0 ? 0 : x >= width ? width - 1 : x;
		const auto sectorY = y < 0 ? 0 : y >= height ? height - 1 : y;
		const auto percentageX = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorXPercentage(manager, &point);
		const auto percentageY = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorYPercentage(manager, &point);
		const auto* first = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY);
		const auto* second = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY);
		const auto* third = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY + 1);
		const auto* fourth = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY + 1);
		// x86 IMUL/ADD/SUB wrap at 32 bits before each signed division by 100.
		const auto topLeft = Read<std::uint32_t>(first, NC2Layout::WeatherSectorOffset::SnowDensity);
		const auto bottomLeft = Read<std::uint32_t>(third, NC2Layout::WeatherSectorOffset::SnowDensity);
		const auto top = topLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(second, NC2Layout::WeatherSectorOffset::SnowDensity) - topLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto bottom = bottomLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(fourth, NC2Layout::WeatherSectorOffset::SnowDensity) - bottomLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto density = NXBasics::Runtime_TimeSigned32(top + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((bottom - top) * static_cast<std::uint32_t>(percentageY)) / NC2Runtime::WeatherGrid::FullPercentage));
		return density < 0 ? 0 : density >= NC2Runtime::WeatherGrid::DensityMaximum ? NC2Runtime::WeatherGrid::DensityMaximum : density;
	}

	// Original function: FUN_00484e80; the Windows body does not read ignoreDark or playerId.
	std::int32_t E2MapWeatherManagerSectorManager_Rain_GetDensity(void* manager, std::uint32_t point, bool /*ignoreDark*/, std::int32_t /*playerId*/)
	{
		NC2Logic::MapMigPoint_ValidatePosition(&point);
		const auto x = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto y = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto width = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth);
		const auto height = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight);
		const auto sectorX = x < 0 ? 0 : x >= width ? width - 1 : x;
		const auto sectorY = y < 0 ? 0 : y >= height ? height - 1 : y;
		const auto percentageX = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorXPercentage(manager, &point);
		const auto percentageY = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorYPercentage(manager, &point);
		const auto* first = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY);
		const auto* second = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY);
		const auto* third = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY + 1);
		const auto* fourth = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY + 1);
		// x86 IMUL/ADD/SUB wrap at 32 bits before each signed division by 100.
		const auto topLeft = Read<std::uint32_t>(first, NC2Layout::WeatherSectorOffset::RainDensity);
		const auto bottomLeft = Read<std::uint32_t>(third, NC2Layout::WeatherSectorOffset::RainDensity);
		const auto top = topLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(second, NC2Layout::WeatherSectorOffset::RainDensity) - topLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto bottom = bottomLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(fourth, NC2Layout::WeatherSectorOffset::RainDensity) - bottomLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto density = NXBasics::Runtime_TimeSigned32(top + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((bottom - top) * static_cast<std::uint32_t>(percentageY)) / NC2Runtime::WeatherGrid::FullPercentage));
		return density < 0 ? 0 : density >= NC2Runtime::WeatherGrid::DensityMaximum ? NC2Runtime::WeatherGrid::DensityMaximum : density;
	}

	// Original function: FUN_00484f85; the Windows body does not read ignoreDark or playerId.
	std::int32_t E2MapWeatherManagerSectorManager_Sand_GetDensity(void* manager, std::uint32_t point, bool /*ignoreDark*/, std::int32_t /*playerId*/)
	{
		NC2Logic::MapMigPoint_ValidatePosition(&point);
		const auto x = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto y = (static_cast<std::int32_t>(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
		const auto width = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth);
		const auto height = Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight);
		const auto sectorX = x < 0 ? 0 : x >= width ? width - 1 : x;
		const auto sectorY = y < 0 ? 0 : y >= height ? height - 1 : y;
		const auto percentageX = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorXPercentage(manager, &point);
		const auto percentageY = E2MapWeatherManagerSectorManager_Coordinates_GetInSectorYPercentage(manager, &point);
		const auto* first = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY);
		const auto* second = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY);
		const auto* third = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX, sectorY + 1);
		const auto* fourth = E2MapWeatherManagerSectorManager_GetDataPtr(manager, sectorX + 1, sectorY + 1);
		// x86 IMUL/ADD/SUB wrap at 32 bits before each signed division by 100.
		const auto topLeft = Read<std::uint32_t>(first, NC2Layout::WeatherSectorOffset::SandDensity);
		const auto bottomLeft = Read<std::uint32_t>(third, NC2Layout::WeatherSectorOffset::SandDensity);
		const auto top = topLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(second, NC2Layout::WeatherSectorOffset::SandDensity) - topLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto bottom = bottomLeft + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((Read<std::uint32_t>(fourth, NC2Layout::WeatherSectorOffset::SandDensity) - bottomLeft) * static_cast<std::uint32_t>(percentageX)) / NC2Runtime::WeatherGrid::FullPercentage);
		const auto density = NXBasics::Runtime_TimeSigned32(top + static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32((bottom - top) * static_cast<std::uint32_t>(percentageY)) / NC2Runtime::WeatherGrid::FullPercentage));
		return density < 0 ? 0 : density >= NC2Runtime::WeatherGrid::DensityMaximum ? NC2Runtime::WeatherGrid::DensityMaximum : density;
	}

	// Original function: FUN_0048520f; Weather_SetRectangle receives two points by value.
	void E2MapWeatherManagerSectorManager_WeatherSetRectangle(void* manager, std::uint32_t first, std::uint32_t second, std::int32_t type, std::int32_t density)
	{
		NC2Logic::MapMigPoint_ValidatePosition(&first);
		NC2Logic::MapMigPoint_ValidatePosition(&second);
		std::int16_t start[2], end[2];
		std::memcpy(start, &first, 4);
		std::memcpy(end, &second, 4);
		if (start[0] > end[0]) std::swap(start[0], end[0]);
		if (start[1] > end[1]) std::swap(start[1], end[1]);
		if (density < 0) density = 0;
		if (density > NC2Runtime::WeatherGrid::DensityMaximum) density = NC2Runtime::WeatherGrid::DensityMaximum;
		const auto sectorCoordinate = [] (std::int16_t coordinate, std::int32_t dimension)
		{
			const std::int32_t sector = (static_cast<std::int32_t>(coordinate) - NC2Runtime::WeatherGrid::SectorCenterInMapPoints) / NC2Runtime::WeatherGrid::SectorWidthInMapPoints;
			return sector < 0 ? 0 : sector >= dimension ? dimension - 1 : sector;
		};
		const auto startX = sectorCoordinate(start[0], Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth));
		const auto startY = sectorCoordinate(start[1], Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight));
		const auto endX = sectorCoordinate(end[0], Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth));
		const auto endY = sectorCoordinate(end[1], Read<std::int32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight));
		for (std::int32_t y = startY; y <= endY; ++y)
		{
			for (std::int32_t x = startX; x <= endX; ++x)
			{
				void* sector = E2MapWeatherManagerSectorManager_GetDataPtr(manager, x, y);
				if (type >= NC2Runtime::WeatherType::Rain && type <= NC2Runtime::WeatherType::Sand)
					Write<std::int32_t>(sector, static_cast<std::uint32_t>(type - NC2Runtime::WeatherType::Rain) * NC2Layout::RecordSize::WeatherDensity, density);
			}
		}
	}
	// Original function: FUN_0048508e; l_Init_DataFromMapIni.
	static void E2MapWeatherManagerSectorManager_InitDataFromMapIni(void* manager, NXBasics::CIniFileReader* reader)
	{
		void* sectors = Read<void*>(manager, NC2Layout::WeatherSectorManagerOffset::Sectors);
		if (sectors == nullptr) return;
		std::memset(sectors, 0, Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight) * NC2Layout::RecordSize::WeatherSector);
		if (!reader->SearchGroup("misc_weather")) return;
		// Complete original command table 004F4660..004F467F, including its terminator.
		static constexpr NXBasics::SToken tokens[] = { { "setsnowrectangle", NC2DataIds::EngineLifetimeToken::SetSnowRectangle },{ "setrainrectangle", NC2DataIds::EngineLifetimeToken::SetRainRectangle },{ "setsandrectangle", NC2DataIds::EngineLifetimeToken::SetSandRectangle },{ nullptr, 0 } };
		while (reader->GetNextCommandLineInGroup() != nullptr)
		{
			const auto command = NXBasics::XB_Token_FindTokenInArray(tokens, reader->GetCurrentName());
			const std::int32_t type = command == NC2DataIds::EngineLifetimeToken::SetSnowRectangle ? NC2Runtime::WeatherType::Snow : command == NC2DataIds::EngineLifetimeToken::SetRainRectangle ? NC2Runtime::WeatherType::Rain : command == NC2DataIds::EngineLifetimeToken::SetSandRectangle ? NC2Runtime::WeatherType::Sand : NC2Runtime::WeatherType::None;
			if (type == NC2Runtime::WeatherType::None) continue;
			const auto x1 = reader->GetInt();
			const auto y1 = reader->GetInt();
			const auto x2 = reader->GetInt();
			const auto y2 = reader->GetInt();
			const auto density = reader->GetInt();
			const std::uint32_t first = static_cast<std::uint16_t>(x1) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y1)) << 16);
			const std::uint32_t second = static_cast<std::uint16_t>(x2) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y2)) << 16);
			E2MapWeatherManagerSectorManager_WeatherSetRectangle(manager, first, second, type, density);
		}
	}
	// Original function: FUN_00484c1b.
	static void E2MapWeatherManagerSectorManager_Destruct(void* manager)
	{
		E2MapWeatherManagerSectorManager_FreeSectorData(manager);
		Unregister(E2MapWeatherManagerSectorManager_LogicCallback, 0x32, { SwitchIds::CallbackType::MapLifetime, SwitchIds::CallbackType::LoadDone });
		gE2MapWeatherManagerSectorManager = nullptr;
	}
	// Original function: FUN_004854d1. The 0xD24 owner matches CE2HumanSoundManager.
	static void E2HumanSoundManager_Destruct(void*)
	{
		gE2HumanSoundManager = nullptr;
	}
	// Original function: FUN_00485b3c; Windows-only owner, class identity unresolved.
	static void Singleton00568418_Destruct(void*)
	{
		Unregister(WorldMusic_Callback, 0xDE, { 0x16, 0x13, 2 });
		if (gDirectMusicManager != nullptr) Unregister(WorldMusic_Callback, 0xDE, { 1, 0x28 });
		gSingleton00568418 = nullptr;
	}
	// Original function: FUN_0047f459.
	static void E2AmbientSoundManager_Destruct(void* manager)
	{
		if (NDSManager::CManager::sTheObjectPtr != nullptr) Unregister(E2AmbientSoundManager_Callback, 0x32, { 2, 0x16, 0x31, 0x32, 0x33 });
		if (Read<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors) != nullptr) FreeAndClearField(manager, NC2Layout::AmbientSoundManagerOffset::Sectors);
		gE2AmbientSoundManager = nullptr;
	}
	// Original function: FUN_004973c8.
	static void E2ParticelGeneratorManager_Destruct(void* generator)
	{
		if (Read<void*>(generator, NC2Layout::ParticleGeneratorManagerOffset::Entries) != nullptr) FreeAndClearField(generator, 4);
		gE2ParticelGeneratorManager = nullptr;
	}
	// Original function: FUN_00486404.
	static void E2ParticelManager_Destruct(void* manager)
	{
		DestroyOwned(Read<void*>(manager, NC2Layout::ParticleManagerOffset::GeneratorManager), E2ParticelGeneratorManager_Destruct);
		if (void* entries = Read<void*>(manager, NC2Layout::ParticleManagerOffset::Wind))
		{
			gE2ParticelWind = nullptr;
			NXBasics::Runtime_Free(entries);
			Write<void*>(manager, NC2Layout::ParticleManagerOffset::Wind, nullptr);
		}
		Unregister(E2ParticelManager_Callback, 0x14, { 1, 2 });
		gE2ParticelManager = nullptr;
	}
	// Original function: FUN_00487556.
	void MacroMap_l_FreeAllData(void* map)
	{
		for (std::uint32_t offset = NC2Layout::MacroMapOffset::PatternNames; offset <= NC2Layout::MacroMapOffset::FishSwarms; offset += NC2Layout::RecordSize::MacroMapSection) FreeField(map, offset);
		std::memset(map, 0, NC2Layout::RecordSize::MacroMap);
	}
	// Original function: FUN_00489f2f.
	void MacroMapManager_l_FreeAllMacroMaps(void* manager)
	{
		if (Read<void*>(manager, NC2Layout::MacroMapManagerOffset::Entries) == nullptr) return;
		for (std::int32_t index = 0; index < Read<std::int32_t>(manager, NC2Layout::MacroMapManagerOffset::Count); ++index)
		{
			void* map = Read<void*>(Read<void*>(manager, NC2Layout::MacroMapManagerOffset::Entries), static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::MacroMapPointer);
			if (map != nullptr)
			{
				MacroMap_Destruct(map);
				NXBasics::Runtime_Free(map);
				Write<void*>(Read<void*>(manager, NC2Layout::MacroMapManagerOffset::Entries), static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::MacroMapPointer, nullptr);
			}
		}
		FreeAndClearField(manager, NC2Layout::MacroMapManagerOffset::Entries);
	}
	// Original function: FUN_00489e01.
	static void MacroMapManager_Destruct(void* manager)
	{
		MacroMapManager_l_FreeAllMacroMaps(manager);
		gMacroMapManager = nullptr;
	}
	// Original function: FUN_0048a47e.
	static void E2StaticDataParticelEffectManager_Destruct(void*)
	{
		if (gStaticParticelEffects != nullptr)
		{
			NXBasics::Runtime_Free(gStaticParticelEffects);
			gStaticParticelEffects = nullptr;
		}
		gE2StaticDataParticelEffectManager = nullptr;
	}
	// Original function: FUN_0048a7fc.
	static void E2StaticDataMiscLandscapeManager_Destruct(void*)
	{
		if (gStaticMiscLandscapeEntries != nullptr)
		{
			NXBasics::Runtime_Free(gStaticMiscLandscapeEntries);
			gStaticMiscLandscapeEntries = nullptr;
		}
		for (std::uint32_t index = 0; index < gStaticMiscLandscapeGroupCount; ++index)
		{
			const std::uint32_t offset = index * 0x14 + 0x10;
			if (Read<void*>(gStaticMiscLandscapeGroups, offset) != nullptr) FreeAndClearField(gStaticMiscLandscapeGroups, offset);
		}
		if (gStaticMiscLandscapeGroups != nullptr)
		{
			NXBasics::Runtime_Free(gStaticMiscLandscapeGroups);
			gStaticMiscLandscapeGroups = nullptr;
		}
		gE2StaticDataMiscLandscapeManager = nullptr;
	}
	// Original function: FUN_0048acb0.
	static void E2StaticDataGoodManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticGoodData);
		gStaticGoodData = nullptr;
		gE2StaticDataGoodManager = nullptr;
	}
	// Original function: FUN_0048af15.
	static void E2StaticDataHumanMiscManager_Destruct(void*)
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSetCount); ++index)
		{
			FreeField(Read<void*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets), index * NC2Layout::RecordSize::RandomPaletteSet + NC2Layout::RandomPaletteSetOffset::Patches);
		}
		FreeField(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets);
		FreeField(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphics);
		FreeField(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobChangeGraphics);
		for (std::uint32_t offset = NC2Layout::StaticHumanMiscOffset::TribeNameArrays; offset < NC2Layout::StaticHumanMiscOffset::PlayerColors; offset += NC2Layout::RecordSize::TribeHumanNameArrays)
		{
			DeleteStorable(Read<void*>(gStaticHumanMiscData, offset));
			DeleteStorable(Read<void*>(gStaticHumanMiscData, offset + NC2Layout::TribeHumanNameArraysOffset::Female));
		}
		std::memset(gStaticHumanMiscData, 0, sizeof(gStaticHumanMiscData));
		gE2StaticDataHumanMiscManager = nullptr;
	}

	// Original function: FUN_004806e8; MSVC vector destructor iterator (stdcall).
	static void __stdcall Vector_Destruct(void* objects, std::uint32_t stride, std::uint32_t count, void(*destruct)(void*))
	{
		// The original tests count - 1 as a signed value, including count == 0x80000000.
		if (NXBasics::Runtime_TimeSigned32(count - 1u) < 0) return;
		std::uintptr_t cursor = reinterpret_cast<std::uintptr_t>(objects) + count * stride;
		do
		{
			cursor -= stride;
			destruct(reinterpret_cast<void*>(cursor));
		} while (--count != 0);
	}
	// Original function: FUN_0048ce91.
	static void StaticMoveableAnimation_Destruct(void* object)
	{
		StaticMoveableAnimationList_Destruct(static_cast<std::byte*>(object) + NC2Layout::FastMoveableJobOffset::Entries);
	}
	// Original function: FUN_0048ce99.
	static void StaticMoveableWalk_Destruct(void* object)
	{
		StaticMoveableWalkList_Destruct(static_cast<std::byte*>(object) + NC2Layout::FastMoveableJobOffset::Entries);
	}
	// Original function: FUN_0048ce45.
	static void StaticMoveableAnimationGroups_Destruct(void* object)
	{
		std::uint32_t offset = NC2Layout::FastMoveableTribeOffset::ReverseDestructionEnd;
		for (std::uint32_t group = 0; group < NC2Runtime::FastMoveableAccessLimits::TribeSlots; ++group)
		{
			offset -= NC2Layout::RecordSize::FastMoveableTribe;
			Vector_Destruct(static_cast<std::byte*>(object) + offset, NC2Layout::RecordSize::FastMoveableJob, NC2Runtime::FastMoveableAccessLimits::JobSlotsHex, StaticMoveableAnimation_Destruct);
		}
	}
	// Original function: FUN_0048ce6b.
	static void StaticMoveableWalkGroups_Destruct(void* object)
	{
		std::uint32_t offset = NC2Layout::FastMoveableTribeOffset::ReverseDestructionEnd;
		for (std::uint32_t group = 0; group < NC2Runtime::FastMoveableAccessLimits::TribeSlots; ++group)
		{
			offset -= NC2Layout::RecordSize::FastMoveableTribe;
			Vector_Destruct(static_cast<std::byte*>(object) + offset, NC2Layout::RecordSize::FastMoveableJob, NC2Runtime::FastMoveableAccessLimits::JobSlotsHex, StaticMoveableWalk_Destruct);
		}
	}
	// Original function: FUN_0048cd18.
	static void E2StaticDataMapMoveablesManager_Destruct(void* manager)
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimationCount); ++index)
		{
			const std::uint32_t offset = index * NC2Layout::RecordSize::StaticMoveableAnimation;
			void* entries = Read<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations);
			const std::uint32_t animationMode = Read<std::uint32_t>(entries, offset + NC2Layout::StaticMoveableAnimationOffset::Mode);
			if (animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames)
			{
				FreeField(entries, offset + NC2Layout::StaticMoveableAnimationOffset::FrameArray);
				for (std::uint32_t direction = 0; direction < 8; ++direction)
				{
					FreeField(Read<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations), offset + NC2Layout::StaticMoveableAnimationOffset::DirectionFramePointers + direction * NC2Layout::RecordSize::FrameList);
				}
			}
			entries = Read<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations);
			if (Read<std::uint32_t>(entries, offset + NC2Layout::StaticMoveableAnimationOffset::Mode) == NC2Runtime::MoveableAnimationMode::Compound) FreeField(entries, offset + NC2Layout::StaticMoveableAnimationOffset::PartArray);
		}
		FreeField(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations);
		Write<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimationCount, 0);
		Write<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations, nullptr);
		FreeField(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::VehicleGraphics);
		FreeField(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimalGraphics);
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::WalkCount); ++index)
		{
			for (std::uint32_t direction = 0; direction < 8; ++direction)
			{
				FreeField(Read<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Walks), index * NC2Layout::RecordSize::StaticMoveableWalk + NC2Layout::StaticMoveableWalkOffset::WalkFramePointers + direction * NC2Layout::RecordSize::FrameList);
				FreeField(Read<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Walks), index * NC2Layout::RecordSize::StaticMoveableWalk + NC2Layout::StaticMoveableWalkOffset::TurnFramePointers + direction * NC2Layout::RecordSize::FrameList);
			}
		}
		FreeField(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Walks);
		Write<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::WalkCount, 0);
		Write<void*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Walks, nullptr);
		gE2StaticDataMapMoveablesManager = nullptr;
		StaticMoveableWalkGroups_Destruct(static_cast<std::byte*>(manager) + NC2Layout::StaticMoveableManagerOffset::WalkAccess);
		StaticMoveableAnimationGroups_Destruct(static_cast<std::byte*>(manager) + NC2Layout::StaticMoveableManagerOffset::AnimationAccess);
	}

	// Original function: FUN_0048d6eb; unnamed nested particle record deleting destructor.
	static void* StaticParticelEntry_Delete(void* entry, std::uint32_t flags)
	{
		if (Read<void*>(entry, NC2Layout::ParticleFramesOffset::FrameIds) != nullptr) FreeAndClearField(entry, NC2Layout::ParticleFramesOffset::FrameIds);
		if ((flags & 1) != 0) NXBasics::Runtime_Free(entry);
		return entry;
	}
	// Original function: FUN_0048d5b6.
	static void StaticParticelEntryList_Destruct(void* list)
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(list, NC2Layout::ParticleGraphicsOffset::EntryCount); ++index)
		{
			void* entry = Read<void*>(Read<void*>(list, NC2Layout::ParticleGraphicsOffset::Entries), index * NC2Layout::RecordSize::ParticleFrameId);
			if (entry != nullptr)
			{
				StaticParticelEntry_Delete(entry, 1);
				Write<void*>(Read<void*>(list, NC2Layout::ParticleGraphicsOffset::Entries), index * NC2Layout::RecordSize::ParticleFrameId, nullptr);
			}
		}
		if (Read<void*>(list, NC2Layout::ParticleGraphicsOffset::Entries) != nullptr) FreeAndClearField(list, NC2Layout::ParticleGraphicsOffset::Entries);
	}
	// Original function: FUN_0048d20d; scalar/vector deleting destructor, stride 0x48.
	static void* StaticParticelData_Delete(void* data, std::uint32_t flags)
	{
		void* allocation = data;
		if ((flags & 2) != 0)
		{
			allocation = static_cast<std::byte*>(data) - 4;
			const std::uint32_t count = Read<std::uint32_t>(allocation, NC2Layout::ObjectArrayCookieOffset::ElementCount);
			if (NXBasics::Runtime_TimeSigned32(count - 1u) >= 0)
			{
				std::uintptr_t cursor = reinterpret_cast<std::uintptr_t>(data) + count * 0x48u + 0x34u;
				std::uint32_t remaining = count;
				do
				{
					cursor -= 0x48;
					StaticParticelEntryList_Destruct(reinterpret_cast<void*>(cursor));
				} while (--remaining != 0);
			}
		}
		else
		{
			StaticParticelEntryList_Destruct(static_cast<std::byte*>(data) + 0x34);
		}
		if ((flags & 1) != 0) NXBasics::Runtime_Free(allocation);
		return allocation;
	}
	// Original function: FUN_0048d26b.
	static void E2StaticDataParticelManager_Destruct(void*)
	{
		if (gStaticParticelData != nullptr)
		{
			StaticParticelData_Delete(gStaticParticelData, 3);
			gStaticParticelData = nullptr;
		}
		gE2StaticDataParticelManager = nullptr;
	}
	// Original function: FUN_0048dbcc.
	static void E2StaticDataTransitionsManager_Destruct(void*)
	{
		FreeField(gStaticTransitionData, 0x0C);
		FreeField(gStaticTransitionData, 4);
		std::memset(gStaticTransitionData, 0, sizeof(gStaticTransitionData));
		gE2StaticDataTransitionsManager = nullptr;
	}
	// Original function: FUN_0048e56c.
	static void E2StaticDataPatternManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticPatternEntries);
		gStaticPatternEntries = nullptr;
		gStaticPatternCount = 0;
		gE2StaticDataPatternManager = nullptr;
	}

	static void MapArea_Destruct(void* list)
	{
		static_cast<NC2Logic::CMapArea*>(list)->~CMapArea();
	}
	static void DestroyMapArea(void* list)
	{
		DestroyOwned(list, MapArea_Destruct);
	}
	// Original function: FUN_0048f5ba.
	static void E2StaticDataHousesManager_Destruct(void*)
	{
		// Session records borrow native children: restore the sole owning table before destroying it.
		NRemaster::CivilizationBuildings::End();
		for (std::int32_t index = 0; index < gStaticHouseCount; ++index)
		{
			void* entry = static_cast<std::byte*>(gStaticHouseEntries) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::EngineHouseType;
			DestroyMapArea(Read<void*>(entry, NC2Layout::EngineHouseTypeOffset::BuildArea));
			for (std::uint32_t state = 0; state < 5; ++state)
			{
				DestroyMapArea(Read<void*>(entry, NC2Layout::EngineHouseTypeOffset::WalkAreas + state * NC2Layout::RecordSize::HouseStageValue));
				DestroyMapArea(Read<void*>(entry, NC2Layout::EngineHouseTypeOffset::WorkAreas + state * NC2Layout::RecordSize::HouseStageValue));
				const std::uint32_t offset = NC2Layout::EngineHouseStageViewOffset::ForwardSmokePoints + state * NC2Layout::RecordSize::EngineHouseStagePoints;
				FreeField(entry, offset - 8);
				FreeField(entry, offset);
				FreeField(entry, offset + 8);
				FreeField(entry, offset + 0x10);
			}
			FreeField(entry, 0x494);
		}
		NXBasics::Runtime_Free(gStaticHouseEntries);
		gStaticHouseEntries = nullptr;
		gStaticHouseCount = 0;
		gE2StaticDataHousesManager = nullptr;
	}
	// Original function: FUN_00490277.
	static void E2StaticDataLandscapesManager_Destruct(void*)
	{
		for (std::int32_t index = 0; index < gStaticLandscapeCount; ++index)
		{
			void* entry = static_cast<std::byte*>(gStaticLandscapeEntries) + static_cast<std::uint32_t>(index) * 0x8C;
			FreeField(entry, 0x68);
			for (std::uint32_t group = 0; group < 4; ++group)
			{
				const std::uint32_t offset = 0x44 + group * 8;
				if (Read<std::uint32_t>(entry, offset) != 0)
				{
					for (std::uint32_t item = 0; item < Read<std::uint32_t>(entry, offset); ++item)
					{
						DestroyMapArea(Read<void*>(Read<void*>(entry, offset + NC2Layout::StaticLandscapeAreaListOffset::Entries), item * NC2Layout::RecordSize::StaticLandscapeArea + NC2Layout::StaticLandscapeAreaOffset::Area));
					}
					FreeField(entry, offset + NC2Layout::StaticLandscapeAreaListOffset::Entries);
				}
			}
			for (std::uint32_t item = 0; item < Read<std::uint32_t>(entry, NC2Layout::StaticLandscapeOffset::FrameGroupCount); ++item)
			{
				FreeField(Read<void*>(entry, NC2Layout::StaticLandscapeOffset::FrameGroups), item * 0x0C + 8);
			}
			FreeField(entry, 0x88);
		}
		NXBasics::Runtime_Free(gStaticLandscapeEntries);
		gStaticLandscapeEntries = nullptr;
		gStaticLandscapeCount = 0;
		gE2StaticDataLandscapesManager = nullptr;
	}
	// Original function: FUN_00480ebd.
	static void E2StaticDataPalettesManager_Destruct(void*)
	{
		DeleteStorable(gStaticPalettes.vertexColorsPalette);
		DeleteStorable(gStaticPalettes.blackPalette);
		NXBasics::Runtime_Free(gStaticPalettes.palettes16);
		gStaticPalettes.palettes16 = nullptr;
		gStaticPalettes.palette16Count = 0;
		for (std::int32_t index = 0; index < gStaticPalettes.palette256Count; ++index)
		{
			auto& entry = gStaticPalettes.palettes256[index];
			DeleteStorable(entry.palette);
			if (entry.preshadedPalettes != nullptr) NXBasics::CPalette::DeleteOriginal(entry.preshadedPalettes, 3);
			NXBasics::Runtime_Free(entry.remapTable);
		}
		NXBasics::Runtime_Free(gStaticPalettes.palettes256);
		gStaticPalettes.palettes256 = nullptr;
		gStaticPalettes.palette256Count = 0;
		gE2StaticDataPalettesManager = nullptr;
	}
	// Original function: FUN_004903d3.
	static void E2StaticDataTexturesManager_Destruct(void* manager)
	{
		auto* list = static_cast<NXBasics::CListBase*>(manager);
		while (void* entry = list->l_Base_GetStartElement())
		{
			list->l_Base_RemoveElement(entry);
			DeleteStorable(Read<void*>(entry, NC2Layout::TextureCacheEntryOffset::Bitmap));
			DeleteStorable(Read<void*>(entry, NC2Layout::TextureCacheEntryOffset::Palette));
			NXBasics::Runtime_Free(entry);
		}
		NXBasics::Runtime_Free(gStaticDataTextures.bitmaps);
		gStaticDataTextures.bitmaps = nullptr;
		NXBasics::Runtime_Free(gStaticDataTextures.palettes);
		gStaticDataTextures.palettes = nullptr;
		gE2StaticDataTexturesManager = nullptr;
		StaticTextureList_Destruct(manager);
	}
	// Original function: FUN_004905fe.
	static void E2StaticDataBobManagersManager_Destruct(void* manager)
	{
		auto* list = static_cast<NXBasics::CListBase*>(manager);
		while (void* entry = list->l_Base_GetStartElement())
		{
			list->l_Base_RemoveElement(entry);
			DeleteStorable(Read<void*>(entry, NC2Layout::StaticBobCacheEntryOffset::BobManager));
			NXBasics::Runtime_Free(entry);
		}
		gE2StaticDataBobManagersManager = nullptr;
		StaticBobManagerList_Destruct(manager);
	}

	SDynamicObjectArrayStorage gDynamicDataHumans{ };
	SDynamicObjectArrayStorage gDynamicDataHouses{ };
	SDynamicObjectArrayStorage gDynamicDataVehicles{ };
	SDynamicObjectArrayStorage gDynamicDataAnimals{ };

	// Original function: FUN_0048024a; SDynamicDataHuman palette subobjects.
	static void* DynamicHuman_Construct(void* human)
	{
		::new (static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::BodyPalette) NXBasics::CPalette;
		::new (static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::HeadPalette) NXBasics::CPalette;
		return human;
	}
	// Original function: FUN_00480162; Windows clears the records after construction.
	void* __fastcall E2DynamicDataHumanManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gDynamicDataHumans.manager = manager;
		std::memset(manager, 0, 1);
		gDynamicDataHumans.entries = nullptr;
		const auto count = static_cast<std::uint32_t>(NC2Logic::CHumanArray::sTheObjectPtr->mCapacity);
		const auto bytes = count * NC2Layout::RecordSize::DynamicHumanUnsigned;
		auto* allocation = static_cast<std::byte*>(NXBasics::Runtime_Allocate(bytes + 4u));
		std::byte* entries = nullptr;
		if (allocation != nullptr)
		{
			entries = allocation + 4;
			Write(allocation, NC2Layout::ObjectArrayCookieOffset::ElementCount, count);
			if (NXBasics::Runtime_TimeSigned32(count - 1u) >= 0)
				for (std::uint32_t i = 0; i < count; ++i) DynamicHuman_Construct(entries + i * NC2Layout::RecordSize::DynamicHumanUnsigned);
		}
		gDynamicDataHumans.entries = entries;
		std::memset(entries, 0, bytes);
		return manager;
	}

	// Original function: FUN_0048d093; JobGraphics_Animals_GetDataPtr.
	static void* StaticMapMoveables_JobGraphics_Animals_GetDataPtr(std::uint32_t tribe, std::uint32_t type)
	{
		auto* records = Read<std::byte*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimalGraphics);
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimalGraphicsCount); ++index)
		{
			void* record = records + index * NC2Layout::RecordSize::StaticAnimalVehicleGraphicsUnsigned;
			if (Read<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Tribe) == tribe && Read<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Type) == type) return record;
		}
		return nullptr;
	}
	// Original function: FUN_0048d0ce; JobGraphics_Vehicles_GetDataPtr.
	static void* StaticMapMoveables_JobGraphics_Vehicles_GetDataPtr(std::uint32_t tribe, std::uint32_t type)
	{
		auto* records = Read<std::byte*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::VehicleGraphics);
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::VehicleGraphicsCount); ++index)
		{
			void* record = records + index * NC2Layout::RecordSize::StaticAnimalVehicleGraphicsUnsigned;
			if (Read<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Tribe) == tribe && Read<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Type) == type) return record;
		}
		return nullptr;
	}
	// Original function: FUN_0049073b.
	static void* E2DynamicDataHouseManager_Construct(void* manager)
	{
		gDynamicDataHouses.manager = manager;
		gDynamicDataHouses.entries = nullptr;
		const std::uint32_t size = NC2Logic::CHouseArray::sTheObjectPtr->mCapacity * 0xD0u;
		gDynamicDataHouses.entries = static_cast<std::byte*>(NXBasics::Runtime_Allocate(size));
		std::memset(gDynamicDataHouses.entries, 0, size);
		return manager;
	}
	// Original function: FUN_0049088c; HouseXtructed.
	static void E2DynamicDataHouseManager_HouseXtructed(std::uint32_t index, bool created)
	{
		if (created) std::memset(gDynamicDataHouses.entries + index * NC2Layout::RecordSize::DynamicHouseUnsigned, 0, NC2Layout::RecordSize::DynamicHouse);
	}
	// Original function: FUN_004908b4.
	static void* E2DynamicDataVehicleManager_Construct(void* manager)
	{
		gDynamicDataVehicles.manager = manager;
		std::memset(manager, 0, 1);
		gDynamicDataVehicles.entries = nullptr;
		const std::uint32_t size = NC2Logic::CVehicleArray::sTheObjectPtr->mCapacity * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
		gDynamicDataVehicles.entries = static_cast<std::byte*>(NXBasics::Runtime_Allocate(size));
		std::memset(gDynamicDataVehicles.entries, 0, size);
		return manager;
	}
	// Original function: FUN_00490a2c; VehicleXtructed.
	static void E2DynamicDataVehicleManager_VehicleXtructed(std::uint32_t index, bool created)
	{
		if (!created) return;
		void* entry = gDynamicDataVehicles.entries + index * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
		void* vehicle = NC2Logic::CVehicleArray::mStaticVars + index * 0x54Cu;
		std::memset(entry, 0, NC2Layout::RecordSize::DynamicAnimalVehicle);
		if (void* graphics = StaticMapMoveables_JobGraphics_Vehicles_GetDataPtr(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::TribeType), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType)))
		{
			Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::BodyBobManager, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::BodyBobManager));
			Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::ShadowBobManager, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::ShadowBobManager));
			if (Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount) != 0)
			{
				const std::uint32_t variant = NC2Logic::Logic_EntryPoint_Random_GetNumber() % Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount);
				Write<std::uint32_t>(entry, NC2Layout::DynamicAnimalVehicleOffset::PaletteId, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteIds + variant * NC2Layout::RecordSize::MoveablePaletteIdUnsigned));
			}
		}
		Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::BodyBobId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::HeadBobId, NXBasics::IntegerRange::UInt32Maximum);
	}
	// Original function: FUN_00490aa5.
	static void* E2DynamicDataAnimalManager_Construct(void* manager)
	{
		gDynamicDataAnimals.manager = manager;
		std::memset(manager, 0, 1);
		gDynamicDataAnimals.entries = nullptr;
		const std::uint32_t size = NC2Logic::CAnimalArray::sTheObjectPtr->mCapacity * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
		gDynamicDataAnimals.entries = static_cast<std::byte*>(NXBasics::Runtime_Allocate(size));
		std::memset(gDynamicDataAnimals.entries, 0, size);
		return manager;
	}
	// Original function: FUN_00490c1d; AnimalXtructed.
	static void E2DynamicDataAnimalManager_AnimalXtructed(std::uint32_t index, bool created)
	{
		if (!created) return;
		void* entry = gDynamicDataAnimals.entries + index * NC2Layout::RecordSize::DynamicAnimalVehicleUnsigned;
		void* animal = NC2Logic::CAnimalArray::mStaticVars + index * 0x98u;
		std::memset(entry, 0, NC2Layout::RecordSize::DynamicAnimalVehicle);
		if (void* graphics = StaticMapMoveables_JobGraphics_Animals_GetDataPtr(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType)))
		{
			Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::BodyBobManager, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::BodyBobManager));
			Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::ShadowBobManager, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::ShadowBobManager));
			if (Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount) != 0)
			{
				const std::uint32_t variant = NC2Logic::Logic_EntryPoint_Random_GetNumber() % Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount);
				Write<std::uint32_t>(entry, NC2Layout::DynamicAnimalVehicleOffset::PaletteId, Read<std::uint32_t>(graphics, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteIds + variant * NC2Layout::RecordSize::MoveablePaletteIdUnsigned));
			}
		}
		Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::BodyBobId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(entry, NC2Layout::DisplayAtomInformationOffset::HeadBobId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Remaining original dependencies are creation/update paths, not shutdown bodies.
	extern void* __fastcall EngineMap_Construct(void*, void* unusedEdx);
	extern void EngineMap_OverlayPattern_HouseAreaChanged(const void*, bool);
	extern void __fastcall DisplayManager_InformThatStaticMapChanged(void*, void* unusedEdx, const void*, std::int32_t);
	extern bool __cdecl EngineMap_l_Transitions_Callback(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);
	extern bool __cdecl EngineMap_l_HeightChanged_Callback(NC2Logic::TCallbackTypes, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	// Original function: FUN_00480264; the two embedded CPalette members.
	static void DynamicHuman_Destruct(void* human)
	{
		reinterpret_cast<NXBasics::CPalette*>(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::HeadPalette)->NXBasics::CPalette::~CPalette();
		reinterpret_cast<NXBasics::CPalette*>(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::BodyPalette)->NXBasics::CPalette::~CPalette();
	}
	// Original function: FUN_004801e9.
	static void* DynamicHuman_Delete(void* human, std::uint32_t flags)
	{
		void* allocation = human;
		if ((flags & 2) != 0)
		{
			allocation = static_cast<std::byte*>(human) - 4;
			Vector_Destruct(human, NC2Layout::RecordSize::DynamicHuman, Read<std::uint32_t>(allocation, NC2Layout::ObjectArrayCookieOffset::ElementCount), DynamicHuman_Destruct);
		}
		else
		{
			DynamicHuman_Destruct(human);
		}
		if ((flags & 1) != 0) NXBasics::Runtime_Free(allocation);
		return allocation;
	}
	// Original function: FUN_0048027c.
	static void E2DynamicDataHumanManager_Destruct(void*)
	{
		if (gDynamicDataHumans.entries != nullptr) DynamicHuman_Delete(gDynamicDataHumans.entries, 3);
		gDynamicDataHumans.entries = nullptr;
		gDynamicDataHumans.manager = nullptr;
	}
	// Original function: FUN_0049077d.
	static void E2DynamicDataHouseManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gDynamicDataHouses.entries);
		gDynamicDataHouses.entries = nullptr;
		gDynamicDataHouses.manager = nullptr;
	}
	// Original function: FUN_004908fd.
	static void E2DynamicDataVehicleManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gDynamicDataVehicles.entries);
		gDynamicDataVehicles.entries = nullptr;
		gDynamicDataVehicles.manager = nullptr;
	}
	// Original function: FUN_00490aee.
	static void E2DynamicDataAnimalManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gDynamicDataAnimals.entries);
		gDynamicDataAnimals.entries = nullptr;
		gDynamicDataAnimals.manager = nullptr;
	}
	// Original function: FUN_004823c8.
	static void EngineMap_Destruct(void*)
	{
		Unregister(EngineMap_l_Transitions_Callback, 0, { 0x33, 0x32, 0x31 });
		Unregister(EngineMap_l_HeightChanged_Callback, 0, { 0x33, 0x32, 0x31 });
		FreeAndClearField(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MacroPoints);
		FreeAndClearField(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MicroPoints);
		gEngineMap = nullptr;
	}
	// Original function: FUN_0047e679, CE2Manager::sl_Callback_SomethingInLogicXtructed.
	bool __cdecl E2Manager_CallbackSomethingInLogicXtructed(NC2Logic::TCallbackTypes type, std::uint32_t /*userData*/, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3)
	{
		switch (type)
		{
			case SwitchIds::CallbackType::MapLifetime:
				if (static_cast<std::uint16_t>(param1) != 0)
				{
					void* map = NXBasics::Runtime_Allocate(1);
					if (map != nullptr) EngineMap_Construct(map, nullptr);
					C2DEngineDisplayManager::sTheObjectPtr->InformThatMapXtructed();
					Write<std::uint32_t>(gE2HumanSoundManager, NC2Layout::HumanSoundManagerOffset::LastGenericSoundTick, 0);
					Write<std::uint32_t>(CE2AnimalSoundManager::sTheObjectPtr, NC2Layout::AnimalSoundManagerOffset::LastUpdate, 0);
				}
				else
				{
					DestroyOwned(gEngineMap, EngineMap_Destruct);
					C2DEngineDisplayManager::sTheObjectPtr->InformThatMapXtructed();
				}
				break;
			case SwitchIds::CallbackType::HumanManagerLifetime:
				if (param1 != 0)
				{
					void* manager = NXBasics::Runtime_Allocate(1);
					if (manager != nullptr) E2DynamicDataHumanManager_Construct(manager, nullptr);
				}
				else DestroyOwned(gDynamicDataHumans.manager, E2DynamicDataHumanManager_Destruct);
				break;
			case SwitchIds::CallbackType::HumanLifetime: E2DynamicDataHumanManager_HumanXtructed(param1, param2 != 0, param3 != 0); break;
			case SwitchIds::CallbackType::AnimalManagerLifetime:
				if (param1 != 0)
				{
					void* manager = NXBasics::Runtime_Allocate(1);
					if (manager != nullptr) E2DynamicDataAnimalManager_Construct(manager);
				}
				else DestroyOwned(gDynamicDataAnimals.manager, E2DynamicDataAnimalManager_Destruct);
				break;
			case SwitchIds::CallbackType::AnimalLifetime: E2DynamicDataAnimalManager_AnimalXtructed(param1, param2 != 0); break;
			case SwitchIds::CallbackType::VehicleManagerLifetime:
				if (param1 != 0)
				{
					void* manager = NXBasics::Runtime_Allocate(1);
					if (manager != nullptr) E2DynamicDataVehicleManager_Construct(manager);
				}
				else DestroyOwned(gDynamicDataVehicles.manager, E2DynamicDataVehicleManager_Destruct);
				break;
			case SwitchIds::CallbackType::VehicleLifetime: E2DynamicDataVehicleManager_VehicleXtructed(param1, param2 != 0); break;
			case SwitchIds::CallbackType::HouseManagerLifetime:
				if (param1 != 0)
				{
					void* manager = NXBasics::Runtime_Allocate(1);
					if (manager != nullptr) E2DynamicDataHouseManager_Construct(manager);
				}
				else DestroyOwned(gDynamicDataHouses.manager, E2DynamicDataHouseManager_Destruct);
				break;
			case SwitchIds::CallbackType::HouseLifetime:
				{
					const bool created = param2 != 0;
					E2DynamicDataHouseManager_HouseXtructed(param1, created);
					void* house = NC2Logic::CHouseArray::mStaticVars + param1 * 0x7E0u;
					if (NC2Logic::House_GetHouseMainType(house) == 6) break;
					void* area = NC2Logic::House_Misc_WalkBlockArea_GetPtr(house);
					const std::uint32_t position = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
					for (std::uint32_t row = 0; row < Read<std::uint32_t>(area, NC2Layout::MapAreaOffset::RowCount); ++row)
					{
						void* line = static_cast<std::byte*>(Read<void*>(area, NC2Layout::MapAreaOffset::Rows)) + row * 8;
						std::uint32_t point;
						NC2Logic::MapMigPoint_ConstructOffset(&point, &position, Read<std::uint16_t>(line, NC2Layout::MapAreaRowOffset::X), Read<std::uint16_t>(line, NC2Layout::MapAreaRowOffset::Y), false);
						for (std::uint32_t column = 0; column < Read<std::uint32_t>(line, NC2Layout::MapAreaRowOffset::Width); ++column)
						{
							if (NC2Logic::MapMigPoint_IsInsideMap(&point)) EngineMap_OverlayPattern_HouseAreaChanged(&point, created);
							point = (point & NXBasics::BitMask::DwordExceptBits0Through15) | static_cast<std::uint16_t>(point + 1u);
						}
					}
					const std::uint16_t x = static_cast<std::uint16_t>(position - 20u);
					const std::uint16_t y = static_cast<std::uint16_t>((position >> 16) - 20u);
					std::int16_t rectangle[4];
					const std::uint16_t rectangleBits[4] = { x, y, 40, 40 };
					std::memcpy(rectangle, rectangleBits, sizeof(rectangle));
					NC2Logic::MapMigRectangle_CutInsideMap(rectangle);
					DisplayManager_InformThatStaticMapChanged(C2DEngineDisplayManager::sTheObjectPtr, nullptr, rectangle, -1);
					break;
				}
		}
		return true;
	}

	// Original function: FUN_00485412, l_Logic_Callback.
	bool __cdecl E2MapWeatherManagerSectorManager_LogicCallback(NC2Logic::TCallbackTypes type, std::uint32_t /*userData*/, std::uint32_t param1, std::uint32_t, std::uint32_t)
	{
		if (type == SwitchIds::CallbackType::MapLifetime)
		{
			if (static_cast<std::uint16_t>(param1) != 0) E2MapWeatherManagerSectorManager_CreateSectorData(gE2MapWeatherManagerSectorManager);
			else E2MapWeatherManagerSectorManager_FreeSectorData(gE2MapWeatherManagerSectorManager);
		}
		else if (type == SwitchIds::CallbackType::LoadDone && NC2Logic::gGameStaticVars.retainedMapIni != nullptr)
		{
			E2MapWeatherManagerSectorManager_InitDataFromMapIni(gE2MapWeatherManagerSectorManager, NC2Logic::gGameStaticVars.retainedMapIni);
		}
		return true;
	}
	// Original function: FUN_0048707e; C2E2ParticelWind constructor, 0x58 bytes.
	static void* E2ParticelWind_Construct(void* state)
	{
		gE2ParticelWind = state;
		std::memset(state, 0, 88);
		const std::uint32_t horizontal = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % 5;
		Write<std::uint32_t>(state, NC2Layout::ParticleWindOffset::HorizontalSpeed, horizontal);
		const std::uint32_t vertical = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % 5;
		Write<std::uint32_t>(state, NC2Layout::ParticleWindOffset::VerticalSpeed, vertical);
		const double horizontalScale = (static_cast<double>(horizontal) * 0.01) * (1.0 / Read<std::int16_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroWidth));
		const double verticalScale = (static_cast<double>(vertical) * 0.01) * (1.0 / Read<std::int16_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroHeight));
		Write<double>(state, NC2Layout::ParticleWindOffset::HorizontalScale, horizontalScale);
		Write<double>(state, NC2Layout::ParticleWindOffset::VerticalScale, verticalScale);
		const std::int32_t angle = NXBasics::Runtime_Rand() % 361;
		Write<std::int32_t>(state, NC2Layout::ParticleWindOffset::AngleDegrees, angle);
		const double radians = (static_cast<double>(static_cast<std::uint32_t>(angle)) * 0.005555555555555556) * 3.1415927410125732;
		const double cosine = std::cos(radians);
		Write<double>(state, NC2Layout::ParticleWindOffset::Cosine, cosine);
		const double sine = std::sin(radians);
		Write<double>(state, NC2Layout::ParticleWindOffset::Sine, sine);
		Write<double>(state, NC2Layout::ParticleWindOffset::DeltaMapX, Read<double>(state, NC2Layout::ParticleWindOffset::HorizontalScale) * Read<double>(state, NC2Layout::ParticleWindOffset::Cosine));
		Write<double>(state, NC2Layout::ParticleWindOffset::DeltaMapY, sine * Read<double>(state, NC2Layout::ParticleWindOffset::VerticalScale));
		return state;
	}
	// Original function: FUN_00486464, CE2ParticelManager::l_Initialize.
	void E2ParticelManager_Initialize(void* manager)
	{
		if (void* entries = Read<void*>(manager, NC2Layout::ParticleManagerOffset::Wind))
		{
			gE2ParticelWind = nullptr;
			NXBasics::Runtime_Free(entries);
			Write<void*>(manager, NC2Layout::ParticleManagerOffset::Wind, nullptr);
		}
		void* drift = NXBasics::Runtime_Allocate(88);
		if (drift != nullptr) drift = E2ParticelWind_Construct(drift);
		Write<void*>(manager, NC2Layout::ParticleManagerOffset::Wind, drift);
		std::memset(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Particles, 0, NC2Layout::RecordSize::ParticlePoolClearBytes);
		std::memset(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Generators, 0, NC2Layout::RecordSize::ParticleGeneratorPoolBytes);
		Write<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastParticleIndex, 0);
		Write<std::uint32_t>(manager, NC2Layout::ParticleManagerOffset::LastGeneratorIndex, 0);
	}

	// Original function: FUN_004973e6; complete token table 004F4C00..004F4C5F.
	static void E2ParticelGeneratorManager_Initialize(void* generator, const char* filename)
	{
		const NXBasics::SToken tokens[] = { { "name", SwitchIds::ParticleGeneratorToken::Name },{ "generate", SwitchIds::ParticleGeneratorToken::Generate },{ "maxGoneLifeTime", SwitchIds::ParticleGeneratorToken::MaxGoneLifeTime },{ "maxLifeTime", SwitchIds::ParticleGeneratorToken::MaxLifeTime },{ "particelmove", SwitchIds::ParticleGeneratorToken::Particelmove },{ "particelspeed", SwitchIds::ParticleGeneratorToken::Particelspeed },{ "usemapheights", SwitchIds::ParticleGeneratorToken::Usemapheights },{ "usewind", SwitchIds::ParticleGeneratorToken::Usewind },{ "setarea", SwitchIds::ParticleGeneratorToken::SetArea },{ "groundcollision", SwitchIds::ParticleGeneratorToken::Groundcollision },{ "heightoffset", SwitchIds::ParticleGeneratorToken::HeightOffset },{ nullptr, 0 } };
		NXBasics::CIniFileReader reader(filename, nullptr, false, nullptr, nullptr);
		NXBasics::CTokenList tokenList(tokens);
		for (char* group = reader.GetFirstGroup("ParticelGenerator"); group != nullptr; group = reader.GetNextGroup("ParticelGenerator"))
		{
			const auto count = Read<std::uint32_t>(generator, NC2Layout::ParticleGeneratorManagerOffset::Count);
			auto* record = Read<std::byte*>(generator, NC2Layout::ParticleGeneratorManagerOffset::Entries) + count * NC2Layout::RecordSize::ParticleGeneratorUnsigned;
			Write(generator, NC2Layout::ParticleGeneratorManagerOffset::Count, count + 1u);
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				switch (tokenList.FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::ParticleGeneratorToken::Name: std::strcpy(reinterpret_cast<char*>(record), reader.GetString()); break;
					case SwitchIds::ParticleGeneratorToken::Generate: Write(record, NC2Layout::ParticleGeneratorOffset::ParticleId, E2StaticDataParticelManager_GetParticelId(reader.GetString())); break;
					case SwitchIds::ParticleGeneratorToken::MaxGoneLifeTime: Write(record, NC2Layout::ParticleGeneratorOffset::MaximumGoneLifetime, reader.GetInt()); break;
					case SwitchIds::ParticleGeneratorToken::MaxLifeTime: Write(record, NC2Layout::ParticleGeneratorOffset::LifetimeTicks, reader.GetInt()); break;
					case SwitchIds::ParticleGeneratorToken::Particelmove: Write(record, NC2Layout::ParticleGeneratorOffset::MovementMode, reader.GetInt()); break;
					case SwitchIds::ParticleGeneratorToken::Particelspeed:
						{
							const auto first = reader.GetInt(), second = reader.GetInt(), third = reader.GetInt();
							Write(record, NC2Layout::ParticleGeneratorOffset::SpeedMapY, first);
							Write(record, NC2Layout::ParticleGeneratorOffset::SpeedHeight, second);
							Write(record, NC2Layout::ParticleGeneratorOffset::SpeedMapX, third);
							break;
						}
					case SwitchIds::ParticleGeneratorToken::Usemapheights: Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorOffset::UseMapHeights, reader.GetInt() != 0); break;
					case SwitchIds::ParticleGeneratorToken::Usewind: if (reader.GetInt() != 0) Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorOffset::UseWind, 1); break;
					case SwitchIds::ParticleGeneratorToken::SetArea:
						{
							const auto first = reader.GetInt(), second = reader.GetInt();
							Write(record, NC2Layout::ParticleGeneratorOffset::AreaHeight, first);
							Write(record, NC2Layout::ParticleGeneratorOffset::AreaWidth, second);
							Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorOffset::UseRandomArea, 1);
							break;
						}
					case SwitchIds::ParticleGeneratorToken::Groundcollision: Write<std::uint8_t>(record, NC2Layout::ParticleGeneratorOffset::KillOnGroundCollision, reader.GetInt() != 0); break;
					case SwitchIds::ParticleGeneratorToken::HeightOffset: Write(record, NC2Layout::ParticleGeneratorOffset::HeightOffset, reader.GetInt()); break;
				}
			}
		}
	}
	// Original function: FUN_0049736d; owns thirty 0x68-byte generator records.
	static void* E2ParticelGeneratorManager_Construct(void* generator)
	{
		gE2ParticelGeneratorManager = generator;
		std::memset(generator, 0, 8);
		auto* records = static_cast<std::byte*>(NXBasics::Runtime_Allocate(3120));
		if (records != nullptr) for (unsigned i = 0; i != 30; ++i) std::memset(records + i * NC2Layout::RecordSize::ParticleGeneratorUnsigned, 0, 104);
		Write(generator, NC2Layout::ParticleGeneratorManagerOffset::Entries, records);
		E2ParticelGeneratorManager_Initialize(generator, "Data\\Engine2D\\Inis\\Particel\\Generator.ini");
		return generator;
	}
	// Original function: FUN_004873db; the first eight bytes remain untouched.
	static void* LogicParticelObject_Construct(void* object)
	{
		for (std::uint32_t offset = 8; offset <= 0x50; offset += 8) Write<double>(object, offset, 0.0);
		Write<std::uint16_t>(object, NC2Layout::LogicParticleOffset::MapPositionY, 0);
		Write<std::uint16_t>(object, NC2Layout::LogicParticleOffset::MapPositionX, 0);
		return object;
	}
	// Original function: FUN_004863b7.
	static void* ParticelArray_Construct(void* array)
	{
		for (std::uint32_t index = 0; index < 10000; ++index)
		{
			auto* record = static_cast<std::byte*>(array) + index * NC2Layout::RecordSize::ParticleUnsigned;
			LogicParticelObject_Construct(record + NC2Layout::ParticleOffset::UseMapHeights);
			std::memset(record, 0, NC2Layout::RecordSize::Particle);
		}
		return array;
	}
	// Original function: FUN_004863e9.
	static void* ParticelGeneratorArray_Construct(void* array)
	{
		for (std::uint32_t index = 0; index < 1000; ++index)
		{
			Write<std::uint16_t>(array, index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned + NC2Layout::MapCoordinatePairOffset::YUnsigned, 0);
			Write<std::uint16_t>(array, index * NC2Layout::RecordSize::ParticleGeneratorSlotUnsigned, 0);
		}
		return array;
	}
	// Original function: FUN_00486336.
	void* __fastcall E2ParticelManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		ParticelArray_Construct(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Particles);
		ParticelGeneratorArray_Construct(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Generators);
		gE2ParticelManager = manager;
		std::memset(manager, 0, NC2Layout::RecordSize::ParticleDrawPrefixBytes);
		void* allocation = NXBasics::Runtime_Allocate(8);
		void* generator = allocation == nullptr ? nullptr : E2ParticelGeneratorManager_Construct(allocation);
		Write<void*>(manager, NC2Layout::ParticleManagerOffset::Wind, nullptr);
		Write(manager, NC2Layout::ParticleManagerOffset::GeneratorManager, generator);
		E2ParticelManager_Initialize(manager);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(1, E2ParticelManager_Callback, 0x14, 1);
		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(2, E2ParticelManager_Callback, 0x14, 1);
		return manager;
	}
	// Original function: FUN_00486eb4, l_ParticelManager_CallBack.
	bool __cdecl E2ParticelManager_Callback(NC2Logic::TCallbackTypes type, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		if (type == 1) E2ParticelManager_DoOneGameTick(gE2ParticelManager, nullptr);
		else if (type == 2) E2ParticelManager_Initialize(gE2ParticelManager);
		return true;
	}

	// Original function: FUN_00485aa1; the owning Windows class name is unresolved.
	void* __fastcall WorldMusic_Construct(void* manager, void* /*unusedEdx*/)
	{
		gSingleton00568418 = manager;
		std::memset(&gWorldMusicState, 0, sizeof(gWorldMusicState));
		gWorldMusicState.selectedPlayer = NXBasics::IntegerRange::UInt32Maximum;
		gWorldMusicState.eventVariant = 4;
		for (std::uint32_t type : {0x13u, 2u, 0x16u}) NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, WorldMusic_Callback, 0xDE, 10);
		if (gDirectMusicManager != nullptr)
		{
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(1, WorldMusic_Callback, 0xDE, 10);
			NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(0x28, WorldMusic_Callback, 0xDE, 10);
		}
		return manager;
	}
	// Original function: FUN_00485ba6; thresholds include state-dependent hysteresis.
	static std::uint32_t WorldMusic_GetEventVariant()
	{
		const auto value = Read<std::int32_t>(NC2Logic::gGameControl, gWorldMusicState.controllingPlayer * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::HappinessValueUnsigned);
		switch (gWorldMusicState.eventVariant)
		{
			case SwitchIds::MusicEventVariant::Neutral: if (value <= -50) return 6; if (value >= 50) return 5; break;
			case SwitchIds::MusicEventVariant::Positive: if (value > 0) return 5; break;
			case SwitchIds::MusicEventVariant::Negative: if (value < 0) return 6; break;
		}
		return 4;
	}
	// Original function: FUN_00485c3a.
	void WorldMusic_Update(std::uint32_t gameTime)
	{
		const auto mode = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode");
		if (mode != 1 && mode != 2) return;
		auto* cd = NCDAudioManager::CCDAudioManager::sTheObjectPtr;
		if (cd != nullptr && cd->IsStillPlaying()) cd->StopTrack();
		if (mode == 1)
		{
			if (NDMManager::HasPendingOrActiveEvent(gDirectMusicManager)) NDMManager::ProcessEvent(gDirectMusicManager, 0, 0, 1);
			return;
		}
		if (gameTime % 120u != 0) gWorldMusicState.eventVariant = WorldMusic_GetEventVariant();
		if (gWorldMusicState.changePending != 0)
		{
			if (gameTime < gWorldMusicState.changeAtGameTime) return;
			gWorldMusicState.changePending = 0;
		}
		if (gWorldMusicState.selectedPlayer == NXBasics::IntegerRange::UInt32Maximum)
		{
			NDMManager::ProcessEvent(gDirectMusicManager, NXBasics::Runtime_TimeSigned32(gWorldMusicState.musicType), gWorldMusicState.eventVariant, 2);
			return;
		}
		const auto selected = NXBasics::Runtime_TimeSigned32(gWorldMusicState.selectedPlayer);
		const auto relationship = NC2Logic::GameControl_Diplomacy_GetState(NC2Logic::gGameControl, NXBasics::Runtime_TimeSigned32(gWorldMusicState.controllingPlayer), selected);
		const auto tribe = selected < 20 ? Read<std::int32_t>(NC2Logic::gGameControl, gWorldMusicState.selectedPlayer * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeTypeUnsigned) : 1;
		const auto event = tribe == 1 ? 2 : tribe == 2 ? 3 : tribe == 3 ? 4 : tribe == 4 ? 5 : tribe > 4 && tribe <= 6 ? 6 : 1;
		NDMManager::ProcessEvent(gDirectMusicManager, event, event == 6 ? 0 : static_cast<std::uint32_t>(relationship), 2);
	}
	// Original function: FUN_00485ed7.
	void WorldMusic_UpdateCdTrack()
	{
		if (NDMManager::HasPendingOrActiveEvent(gDirectMusicManager)) NDMManager::ProcessEvent(gDirectMusicManager, 0, 0, 1);
		auto* cd = NCDAudioManager::CCDAudioManager::sTheObjectPtr;
		if (cd == nullptr || NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode") != 3) return;
		const auto current = cd->GetCurrentTrack();
		// Complete contiguous DWORD region 00505FF4..0050608F. Only the base-indexed
		// Windows xref is known; subdivision of the trailing values is unresolved.
		static constexpr std::uint32_t tracks[39] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 8, 9, 10 };
		const auto track = tracks[gWorldMusicState.musicType];
		if (track != 0)
		{
			if (track != current) cd->StartTrack(track);
		}
		else if (cd->IsStillPlaying()) cd->StopTrack();
	}
	// Original function: FUN_00485f57.
	static void WorldMusic_LoadMusicType()
	{
		gWorldMusicState.musicType = 0;
		auto* reader = NC2Logic::gGameStaticVars.retainedMapIni;
		if (reader != nullptr && reader->SearchGroup("misc_music") && reader->SearchCommand("musictype")) gWorldMusicState.musicType = static_cast<std::uint32_t>(reader->GetInt());
	}
	// Original function: FUN_00485d7a; Windows music callback.
	bool __cdecl WorldMusic_Callback(NC2Logic::TCallbackTypes type, std::uint32_t, std::uint32_t param1, std::uint32_t param2, std::uint32_t)
	{
		const auto inTransition = [] ()
		{
			const std::int32_t programState = Read<std::int32_t>(NMasterControlProgram::CMasterControlProgram::sTheObjectPtr, NC2Layout::MasterControlOffset::ProgramState);
			return programState == 7 || programState == 8 || programState == 9;
		};
		switch (type)
		{
			case SwitchIds::CallbackType::GameTick:
				if (!inTransition()) WorldMusic_Update(param1);
				break;
			case SwitchIds::CallbackType::MapLifetime:
				gWorldMusicState.selectedPlayer = NXBasics::IntegerRange::UInt32Maximum;
				gWorldMusicState.eventVariant = 4;
				gWorldMusicState.changePending = 0;
				gWorldMusicState.changeAtGameTime = 0;
				break;
			case SwitchIds::CallbackType::LoadCleanmap:
				WorldMusic_LoadMusicType();
				WorldMusic_UpdateCdTrack();
				break;
			case SwitchIds::CallbackType::LoadDone: WorldMusic_UpdateCdTrack(); break;
			case SwitchIds::CallbackType::PlayerAttackedByPlayer:
				if (inTransition() || NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode") != 2) break;
				if (param1 != gWorldMusicState.controllingPlayer && param2 != gWorldMusicState.controllingPlayer) break;
				if (gWorldMusicState.changePending != 0)
				{
					gWorldMusicState.changeAtGameTime = NC2Logic::gGameTime.tick + 120u;
					break;
				}
				{
					const std::uint32_t player = param1 == gWorldMusicState.controllingPlayer ? param2 : param1;
					const std::int32_t tribe = NXBasics::Runtime_TimeSigned32(player) < 20 ? Read<std::int32_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType) : 1;
					gWorldMusicState.changeAtGameTime = NC2Logic::gGameTime.tick + 120u;
					gWorldMusicState.changePending = 1;
					const std::int32_t event = tribe == 2 ? 7 : tribe == 3 ? 8 : tribe == 4 ? 9 : 6;
					NDMManager::ProcessEvent(gDirectMusicManager, event, 0, 2);
				}
				break;
		}
		return true;
	}

	// Original function: FUN_0047ff55. Flags at 00554DB0/1 retain numeric identity.
	bool __cdecl E2AmbientSoundManager_Callback(NC2Logic::TCallbackTypes type, std::uint32_t, std::uint32_t param1, std::uint32_t param2, std::uint32_t)
	{
		switch (type)
		{
			case SwitchIds::CallbackType::MapLifetime:
				if (static_cast<std::uint16_t>(param1) != 0)
				{
					AmbientSoundManager_AllocateMemory(gE2AmbientSoundManager);
					Write<std::uint32_t>(gE2AmbientSoundManager, NC2Layout::AmbientSoundManagerOffset::LastLandscapeSoundTick, 0);
				}
				break;
			case SwitchIds::CallbackType::LoadDone: AmbientSoundManager_RecalculateAllAmbientSoundSector(gE2AmbientSoundManager); break;
			case SwitchIds::CallbackType::BeginMapChanges:
				Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastY, 0);
				Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastX, 0);
				gAmbientSoundChangeState.landscapesChanged = 0;
				gAmbientSoundChangeState.patternsChanged = 0;
				Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstY, 32000);
				Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstX, 32000);
				break;
			case SwitchIds::CallbackType::EndMapChanges:
				if (Read<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstX) != 32000)
				{
					const NC2Logic::SSectorGridCursor first(Read<std::uint32_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstX));
					const NC2Logic::SSectorGridCursor last(Read<std::uint32_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastX));
					for (std::int32_t y = first.y; y <= last.y; ++y)
					{
						for (std::int32_t x = first.x; x <= last.x; ++x)
						{
							NC2Logic::SSectorGridCursor sector;
							sector.x = x;
							sector.y = y;
							AmbientSoundManager_RecalculateAmbientSoundSector(gE2AmbientSoundManager, &sector, gAmbientSoundChangeState.patternsChanged != 0, gAmbientSoundChangeState.landscapesChanged != 0);
						}
					}
				}
				break;
			case SwitchIds::CallbackType::MapPointChanged:
				if (param1 == 0x0A) gAmbientSoundChangeState.landscapesChanged = 1;
				else if (param1 == 0x0B) break;
				else gAmbientSoundChangeState.patternsChanged = 1;
				{
					std::uint32_t point = param1;
					NC2Logic::MapMigPoint_AssignLong(&point, param2);
					const std::int16_t x = Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X);
					const std::int16_t y = Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y);
					if (x < Read<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstX)) Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstX, x);
					if (x > Read<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastX)) Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastX, x);
					if (y < Read<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstY)) Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::FirstY, y);
					if (y > Read<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastY)) Write<std::int16_t>(&gAmbientSoundChangeState, NC2Layout::AmbientChangeBoundsOffset::LastY, y);
				}
				break;
		}
		return true;
	}

	// Original function: FUN_0047e181.
	void E2Manager_Destruct(void*)
	{
		DestroyOwned(gE2MapWeatherManagerSectorManager, E2MapWeatherManagerSectorManager_Destruct);
		DestroyOwned(gE2HumanSoundManager, E2HumanSoundManager_Destruct);
		DestroyOwned(gSingleton00568418, Singleton00568418_Destruct);
		DestroyOwned(CE2AnimalSoundManager::sTheObjectPtr, [] (CE2AnimalSoundManager* manager)
		{
			manager->~CE2AnimalSoundManager();
		});
		DestroyOwned(gE2AmbientSoundManager, E2AmbientSoundManager_Destruct);
		DestroyOwned(gE2ParticelManager, E2ParticelManager_Destruct);
		DestroyOwned(C2DEngineDisplayManager::sTheObjectPtr, [] (C2DEngineDisplayManager* manager)
		{
			manager->~C2DEngineDisplayManager();
		});
		Unregister(E2Manager_sl_Callback_Misc, 0, { 0x25, 0x26, 0x18, 0x1D, 0x24, 0x27, 0x23, 0x20, 0x1F, 0x22, 0x21, 0x1C, 0x17 });
		Unregister(E2Manager_sl_Callback_MapEdited, 0, { 0x0D, 0x33, 0x32, 0x31 });
		Unregister(E2Manager_sl_Callback_MapIO, 0, { 0x16, 0x15, 0x14, 0x13, 0x12 });
		Unregister(E2Manager_sl_Callback_ObjectDisplayStateInLogicChanged, 0, { 0x11, 0x10, 0x0F, 0x0E });
		Unregister(E2Manager_CallbackSomethingInLogicXtructed, 0, { 0x0A, 9, 8, 7, 6, 5, 4, 3, 2 });
		DestroyOwned(gMacroMapManager, MacroMapManager_Destruct);
		DestroyOwned(CLevelOfDetailManager::sTheObjectPtr, [] (CLevelOfDetailManager* manager)
		{
			manager->~CLevelOfDetailManager();
		});
		DestroyOwned(gE2StaticDataParticelEffectManager, E2StaticDataParticelEffectManager_Destruct);
		DestroyOwned(gE2StaticDataMiscLandscapeManager, E2StaticDataMiscLandscapeManager_Destruct);
		DestroyOwned(CE2StaticDataAnimalSoundManager::sTheObjectPtr, [] (CE2StaticDataAnimalSoundManager* manager)
		{
			manager->~CE2StaticDataAnimalSoundManager();
		});
		DestroyOwned(gE2StaticDataGoodManager, E2StaticDataGoodManager_Destruct);
		DestroyOwned(gE2StaticDataHumanMiscManager, E2StaticDataHumanMiscManager_Destruct);
		DestroyOwned(gE2StaticDataMapMoveablesManager, E2StaticDataMapMoveablesManager_Destruct);
		DestroyOwned(gE2StaticDataParticelManager, E2StaticDataParticelManager_Destruct);
		DestroyOwned(gE2StaticDataTransitionsManager, E2StaticDataTransitionsManager_Destruct);
		DestroyOwned(CE2StaticDataSoundFXManager::sTheObjectPtr, [] (CE2StaticDataSoundFXManager* manager)
		{
			manager->~CE2StaticDataSoundFXManager();
		});
		DestroyOwned(gE2StaticDataPatternManager, E2StaticDataPatternManager_Destruct);
		DestroyOwned(gE2StaticDataHousesManager, E2StaticDataHousesManager_Destruct);
		DestroyOwned(gE2StaticDataLandscapesManager, E2StaticDataLandscapesManager_Destruct);
		DestroyOwned(gE2StaticDataPalettesManager, E2StaticDataPalettesManager_Destruct);
		DestroyOwned(gE2StaticDataTexturesManager, E2StaticDataTexturesManager_Destruct);
		DestroyOwned(gE2StaticDataBobManagersManager, E2StaticDataBobManagersManager_Destruct);
		gE2Manager = nullptr;
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004809ab; the density-percent multiplication wraps at 32 bits.
	void External_FX_Weather_SetRectangle(std::uint32_t first, std::uint32_t second, bool snow, std::int32_t densityPercent)
	{
		if (NC2E2::gE2MapWeatherManagerSectorManager != nullptr) NC2E2::E2MapWeatherManagerSectorManager_WeatherSetRectangle(NC2E2::E2MapWeatherManagerSectorManager_GetExistingObjectPtr(), first, second, snow ? NC2Runtime::WeatherType::Snow : NC2Runtime::WeatherType::Rain, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(densityPercent) * NC2Runtime::WeatherGrid::DensityUnitsPerPercent));
	}
}
