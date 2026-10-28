#include "ApplicationGlobals.h"
#include "C2DataIds.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineMacroMaps.h"
#include "C2EngineMapIO.h"
#include "C2EngineObjectData.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GuiToolsBase.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XList.h"
#include "XNumericConstants.h"
#include "XPaletteAccess.h"

#include <cstring>

namespace NC2E2
{
	using NC2Lifetime::Read;
	using NC2Lifetime::Write;
	using NC2Logic::CIoHelper;

	namespace
	{
		std::byte* MapScratch()
		{
			return Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
		}
		std::uint32_t MacroCount()
		{
			return Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount);
		}
		std::uint32_t MicroCount()
		{
			return Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount);
		}
		std::byte* EngineMacro()
		{
			return Read<std::byte*>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MacroPoints);
		}
		std::byte* EngineMicro()
		{
			return Read<std::byte*>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MicroPoints);
		}

		void WritePlane(CIoHelper* io, std::uint32_t marker, bool micro, std::uint32_t offset, bool words)
		{
			io->IO_Tool_Chunk_StartCreation(marker, 1);
			auto* scratch = MapScratch();
			const auto count = micro ? MicroCount() : MacroCount();
			const auto stride = micro ? 4u : 12u;
			const auto* entries = micro ? EngineMicro() : EngineMacro();
			for (std::uint32_t i = 0; i < count; ++i)
			{
				if (words) Write<std::uint16_t>(scratch, i * NC2Layout::RecordSize::SerializedWordUnsigned, Read<std::uint16_t>(entries, i * stride + offset));
				else scratch[i] = entries[i * stride + offset];
			}
			io->IO_Memory_PackAndWriteBinary(scratch, count * (words ? 2u : 1u), words ? NC2DataIds::PackingType::Rle16 : NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		void WriteNames(CIoHelper* io, std::uint32_t marker, std::uint32_t count, const void* entries, std::size_t stride)
		{
			io->IO_Tool_Chunk_StartCreation(marker, 1);
			io->IO_Memory_WriteLong(count);
			for (std::uint32_t i = 0; i < count; ++i) io->IO_Memory_WriteString(static_cast<const char*>(entries) + i * stride);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		struct SNameMap
		{
			std::uint32_t count = 0; std::int32_t* ids = nullptr;
		};
		void ReadNames(CIoHelper* io, SNameMap& mapping, std::int32_t(*lookup)(const char*))
		{
			// Repeated name chunks overwrite the owner without freeing it, as in Windows.
			mapping.count = io->IO_File_ReadLong();
			mapping.ids = static_cast<std::int32_t*>(NXBasics::Runtime_Allocate(mapping.count * 4u));
			for (std::uint32_t i = 0; i < mapping.count; ++i) mapping.ids[i] = lookup(io->IO_File_ReadString());
		}

		void ReadBytePlane(CIoHelper* io, bool micro, std::uint32_t offset)
		{
			auto* scratch = MapScratch();
			const auto count = micro ? MicroCount() : MacroCount();
			io->IO_File_ReadAndUnpackBinary(scratch, count);
			auto* entries = micro ? EngineMicro() : EngineMacro();
			const auto stride = micro ? 4u : 12u;
			for (std::uint32_t i = 0; i < count; ++i) entries[i * stride + offset] = scratch[i];
		}

		void ReadPatternPlane(CIoHelper* io, const SNameMap& mapping, std::uint32_t offset, std::int32_t borderId)
		{
			auto* scratch = MapScratch();
			io->IO_File_ReadAndUnpackBinary(scratch, MacroCount() * 2u);
			for (std::uint32_t i = 0; i < MacroCount(); ++i)
			{
				const auto stored = Read<std::uint16_t>(scratch, i * NC2Layout::RecordSize::SerializedWordUnsigned);
				std::int32_t id = borderId;
				if (stored < mapping.count && mapping.ids[stored] != -1) id = mapping.ids[stored];
				else Write<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag, 1);
				Write<std::uint16_t>(EngineMacro(), i * NC2Layout::RecordSize::EngineMapMacroPointUnsigned + offset, static_cast<std::uint16_t>(id));
			}
		}

		void ReadTransitionPlane(CIoHelper* io, const SNameMap& mapping, std::uint32_t offset)
		{
			auto* scratch = MapScratch();
			io->IO_File_ReadAndUnpackBinary(scratch, MacroCount());
			for (std::uint32_t i = 0; i < MacroCount(); ++i)
			{
				const auto stored = Read<std::uint8_t>(scratch, i);
				std::uint8_t encoded = NXBasics::IntegerRange::UInt8MaximumSigned;
				if (stored != NXBasics::IntegerRange::UInt8MaximumSigned && stored / 6u < mapping.count && mapping.ids[stored / 6u] != -1) encoded = static_cast<std::uint8_t>(mapping.ids[stored / 6u] * 6u + stored % 6u);
				Write(EngineMacro(), i * NC2Layout::RecordSize::EngineMapMacroPointUnsigned + offset, encoded);
			}
		}

		void ReadLandscapePlane(CIoHelper* io, const SNameMap& mapping)
		{
			auto* scratch = MapScratch();
			io->IO_File_ReadAndUnpackBinary(scratch, MicroCount() * 2u);
			for (std::uint32_t i = 0; i < MicroCount(); ++i)
			{
				const auto stored = Read<std::uint16_t>(scratch, i * NC2Layout::RecordSize::SerializedWordUnsigned);
				std::uint16_t id = NXBasics::IntegerRange::UInt16MaximumSigned;
				const bool missing = stored != NXBasics::IntegerRange::UInt16MaximumSigned && (stored >= mapping.count || mapping.ids[stored] == -1);
				if (stored != NXBasics::IntegerRange::UInt16MaximumSigned && !missing) id = static_cast<std::uint16_t>(mapping.ids[stored]);
				Write(EngineMicro(), i * NC2Layout::RecordSize::EngineMapMicroPointUnsigned, id);
				if (missing)
				{
					Write<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag, 1);
					auto* logic = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage) + i * 14u;
					if (Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType) <= 1) Write<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType, 0);
				}
			}
		}
	}

	// Original function: FUN_004832fa.
	void EngineMap_IO_Cleanmap_Save(CIoHelper* io)
	{
		io->IO_File_Chunk_WriteMarkerAndVersion(NC2DataIds::EngineMapChunk::MapHeader, 0, 0);
		WritePlane(io, SwitchIds::EngineMapChunk::MacroBrightness, false, 4, false);
		WritePlane(io, SwitchIds::EngineMapChunk::MacroVertexColor, false, 5, false);
		WritePlane(io, SwitchIds::EngineMapChunk::MacroFlags, false, 6, false);
		WritePlane(io, SwitchIds::EngineMapChunk::MicroFlags, true, 2, false);
		WriteNames(io, SwitchIds::EngineMapChunk::PatternNames, gStaticPatternStorage.count, gStaticPatternStorage.entries, 0x70);
		WritePlane(io, SwitchIds::EngineMapChunk::PatternA, false, 0, true);
		WritePlane(io, SwitchIds::EngineMapChunk::PatternB, false, 2, true);
		WriteNames(io, SwitchIds::EngineMapChunk::TransitionNames, static_cast<std::uint32_t>(gStaticTransitionStorage.count), gStaticTransitionStorage.entries, 0x154);
		for (std::uint32_t i = 0; i != 4; ++i) WritePlane(io, SwitchIds::EngineMapChunk::TransitionFirst + i, false, 7 + i, false);
		WriteNames(io, SwitchIds::EngineMapChunk::LandscapeNames, static_cast<std::uint32_t>(gStaticLandscapeStorage.count), gStaticLandscapeStorage.entries, sizeof(SE2StaticDataLandscape));
		WritePlane(io, SwitchIds::EngineMapChunk::Landscapes, true, 0, true);
		io->IO_Tool_Chunk_WriteEndChunk();
	}

	// Original function: FUN_00483800.
	void EngineMap_IO_Cleanmap_Load(CIoHelper* io)
	{
		if (!io->IO_File_Chunk_CheckMarkerAndVersion(NC2DataIds::EngineMapChunk::MapHeader, 0)) return;
		Write<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag, 0);
		const auto borderId = gStaticPatternStorage.borderIds[1];
		SNameMap patterns, transitions, landscapes;
		while (io->IO_File_Chunk_ReadHeader())
		{
			switch (io->mReadHeader.marker)
			{
				case SwitchIds::EngineMapChunk::MacroBrightness: ReadBytePlane(io, false, 4); break;
				case SwitchIds::EngineMapChunk::MacroVertexColor: ReadBytePlane(io, false, 5); break;
				case SwitchIds::EngineMapChunk::MacroFlags: ReadBytePlane(io, false, 6); break;
				case SwitchIds::EngineMapChunk::MicroFlags: ReadBytePlane(io, true, 2); break;
				case SwitchIds::EngineMapChunk::PatternNames: ReadNames(io, patterns, E2StaticDataPatternManager_GetPatternId); break;
				case SwitchIds::EngineMapChunk::TransitionNames: ReadNames(io, transitions, E2StaticDataTransitionsManager_GetTransitionId); break;
				case SwitchIds::EngineMapChunk::LandscapeNames: ReadNames(io, landscapes, E2StaticDataLandscapesManager_GetLandscapeId); break;
				case SwitchIds::EngineMapChunk::PatternA: ReadPatternPlane(io, patterns, 0, borderId); break;
				case SwitchIds::EngineMapChunk::PatternB: ReadPatternPlane(io, patterns, 2, borderId); break;
				case SwitchIds::EngineMapChunk::TransitionFirst: ReadTransitionPlane(io, transitions, 7); break;
				case SwitchIds::EngineMapChunk::TransitionSecond: ReadTransitionPlane(io, transitions, 8); break;
				case SwitchIds::EngineMapChunk::TransitionThird: ReadTransitionPlane(io, transitions, 9); break;
				case SwitchIds::EngineMapChunk::TransitionFourth: ReadTransitionPlane(io, transitions, 10); break;
				case SwitchIds::EngineMapChunk::Landscapes: ReadLandscapePlane(io, landscapes); break;
				default: io->IO_File_Chunk_Skip(); break;
			}
		}
		NXBasics::Runtime_Free(transitions.ids);
		NXBasics::Runtime_Free(landscapes.ids);
		NXBasics::Runtime_Free(patterns.ids);
		if (Read<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag) != 0)
		{
			const char* message = NC2GuiToolsBase::StringTool_GetOdin001StringPtr(10);
			auto* gui = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
			if (gui != nullptr) gui->MiscWindows_StringOutputWindow_PrintString(message);
			Write<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag, 0);
		}
	}

	// Original function: FUN_00483e30.
	void EngineMap_IO_Savegame_Save(CIoHelper* io)
	{
		io->IO_File_Chunk_WriteMarkerAndVersion(SwitchIds::EngineSavegameChunk::EngineMap, 1, 0);
		EngineMap_IO_Cleanmap_Save(io);
	}
	// Original function: FUN_00483e4d.
	void EngineMap_IO_Savegame_Load(CIoHelper* io)
	{
		EngineMap_IO_Cleanmap_Load(io);
	}

	// Original function: FUN_004906cd.
	const char* __fastcall E2StaticDataBobManagersManager_GetBobManagerFilenamePtr(void* manager, void* /*unusedEdx*/, const void* bobManager)
	{
		auto* list = static_cast<NXBasics::CListBase*>(manager);
		NXBasics::CListBase::Iterator iterator(*list);
		for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next())
			if (Read<void*>(entry, NC2Layout::StaticBobCacheEntryOffset::BobManager) == bobManager) return static_cast<const char*>(entry);
		return nullptr;
	}

	// Original function: FUN_00490c90; SDisplayAtomInformation::IO_Savegame_Save.
	static void DisplayAtomInformation_IO_Savegame_Save(void* atom, CIoHelper* io)
	{
		for (std::uint32_t offset = NC2Layout::DisplayAtomInformationOffset::BodyBobManager; offset <= NC2Layout::DisplayAtomInformationOffset::ShadowBobManager; offset += 4)
		{
			const void* bob = Read<void*>(atom, offset);
			const char* filename = bob == nullptr ? nullptr : E2StaticDataBobManagersManager_GetBobManagerFilenamePtr(gE2StaticDataBobManagersManager, nullptr, bob);
			io->IO_Memory_WriteString(filename != nullptr ? filename : " ");
		}
	}

	// Original function: FUN_00490d0b; SDisplayAtomInformation::IO_Savegame_Load.
	static void DisplayAtomInformation_IO_Savegame_Load(void* atom, CIoHelper* io)
	{
		std::memset(atom, 0, NC2Layout::RecordSize::DisplayAtomInformation);
		Write<std::uint32_t>(atom, NC2Layout::DisplayAtomInformationOffset::BodyBobId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(atom, NC2Layout::DisplayAtomInformationOffset::HeadBobId, NXBasics::IntegerRange::UInt32Maximum);
		for (std::uint32_t offset = NC2Layout::DisplayAtomInformationOffset::BodyBobManager; offset <= NC2Layout::DisplayAtomInformationOffset::ShadowBobManager; offset += 4)
		{
			const char* filename = io->IO_File_ReadString();
			if (std::strlen(filename) != 0) Write(atom, offset, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, filename));
		}
	}

	namespace
	{
		template<class Array> void SaveDynamicObjects(CIoHelper* io, std::uint32_t marker, SDynamicObjectArrayStorage& storage, std::uint32_t stride, std::uint32_t payload, bool atom, bool human)
		{
			io->IO_Tool_Chunk_StartCreation(marker, 1);
			io->IO_Memory_WriteLong(1);
			for (std::int32_t i = Array::sTheObjectPtr->mFirstScanIndex; i <= Array::sTheObjectPtr->mLastScanIndex; ++i)
			{
				if (Read<std::uint8_t>(Array::mStaticVars, static_cast<std::uint32_t>(i) * Array::ElementSize) == 0) continue;
				auto* entry = storage.entries + static_cast<std::uint32_t>(i) * stride;
				if (atom) DisplayAtomInformation_IO_Savegame_Save(entry, io);
				if (human)
				{
					NXBasics::Palette_PaletteChanged(entry + NC2Layout::DynamicHumanOffset::BodyPalette);
					NXBasics::Palette_PaletteChanged(entry + NC2Layout::DynamicHumanOffset::HeadPalette);
				}
				io->IO_Memory_WriteBinary(entry + (atom ? NC2Layout::DisplayAtomInformationOffset::ActorSpecificData : 0), payload);
			}
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		template<class Array> void LoadDynamicObjects(CIoHelper* io, SDynamicObjectArrayStorage& storage, std::uint32_t stride, std::uint32_t payload, bool atom)
		{
			if (io->mReadHeader.version != 1)
			{
				io->IO_File_Chunk_Skip(); return;
			}
			io->IO_File_ReadLong();
			for (std::int32_t i = Array::sTheObjectPtr->mFirstScanIndex; i <= Array::sTheObjectPtr->mLastScanIndex; ++i)
			{
				if (Read<std::uint8_t>(Array::mStaticVars, static_cast<std::uint32_t>(i) * Array::ElementSize) == 0) continue;
				auto* entry = storage.entries + static_cast<std::uint32_t>(i) * stride;
				if (atom) DisplayAtomInformation_IO_Savegame_Load(entry, io);
				io->IO_File_ReadBinary(entry + (atom ? NC2Layout::DisplayAtomInformationOffset::ActorSpecificData : 0), payload);
			}
		}
	}

	// Original function: FUN_0048029c.
	static void DynamicDataHumanManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CHumanArray>(io, SwitchIds::EngineSavegameChunk::DynamicHumans, gDynamicDataHumans, NC2Layout::RecordSize::DynamicHuman, NC2Layout::RecordSize::DynamicHumanSavegamePayload, true, true);
	}
	// Original function: FUN_00480348.
	static void DynamicDataHumanManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CHumanArray>(io, gDynamicDataHumans, NC2Layout::RecordSize::DynamicHuman, NC2Layout::RecordSize::DynamicHumanSavegamePayload, true);
	}
	// Original function: FUN_00490798.
	static void DynamicDataHouseManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CHouseArray>(io, SwitchIds::EngineSavegameChunk::DynamicHouses, gDynamicDataHouses, NC2Layout::RecordSize::DynamicHouse, NC2Layout::RecordSize::DynamicHouseSavedPrefix, false, false);
	}
	// Original function: FUN_00490817.
	static void DynamicDataHouseManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CHouseArray>(io, gDynamicDataHouses, NC2Layout::RecordSize::DynamicHouse, NC2Layout::RecordSize::DynamicHouseSavedPrefix, false);
	}
	// Original function: FUN_00490918.
	static void DynamicDataVehicleManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CVehicleArray>(io, SwitchIds::EngineSavegameChunk::DynamicVehicles, gDynamicDataVehicles, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true, false);
	}
	// Original function: FUN_004909a8.
	static void DynamicDataVehicleManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CVehicleArray>(io, gDynamicDataVehicles, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true);
	}
	// Original function: FUN_00490b09.
	static void DynamicDataAnimalManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CAnimalArray>(io, SwitchIds::EngineSavegameChunk::DynamicAnimals, gDynamicDataAnimals, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true, false);
	}
	// Original function: FUN_00490b99.
	static void DynamicDataAnimalManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CAnimalArray>(io, gDynamicDataAnimals, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true);
	}

	// Original function: FUN_00484cc3.
	static void E2MapWeatherManagerSectorManager_IO_Savegame_Load(void* manager, CIoHelper* io)
	{
		if (io->mReadHeader.version != 1)
		{
			io->IO_File_Chunk_Skip(); return;
		}
		E2MapWeatherManagerSectorManager_FreeSectorData(manager);
		io->IO_File_ReadBinary(manager, NC2Layout::RecordSize::WeatherSectorDimensions);
		const auto size = Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight) * NC2Layout::RecordSize::WeatherSector;
		void* entries = NXBasics::Runtime_Allocate(size);
		Write(manager, NC2Layout::WeatherSectorManagerOffset::Sectors, entries);
		std::memset(entries, 0, size);
		io->IO_File_ReadBinary(entries, size);
	}
	// Original function: FUN_00484d34.
	static void E2MapWeatherManagerSectorManager_IO_Savegame_Save(void* manager, CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::EngineSavegameChunk::Weather, 1);
		io->IO_Memory_WriteBinary(manager, NC2Layout::RecordSize::WeatherSectorDimensions);
		io->IO_Memory_WriteBinary(Read<void*>(manager, NC2Layout::WeatherSectorManagerOffset::Sectors), Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::WeatherSectorManagerOffset::SectorHeight) * NC2Layout::RecordSize::WeatherSector);
		io->IO_Tool_Chunk_FinishAndWrite();
	}
	// Original function: FUN_00485f97; the owning class name remains unresolved.
	static void WorldMusic_IO_Savegame_Load(CIoHelper* io)
	{
		if (io->mReadHeader.version != 1) io->IO_File_Chunk_Skip();
		else io->IO_File_ReadBinary(&gWorldMusicState, 0x68);
	}
	// Original function: FUN_00485fb3.
	static void WorldMusic_IO_Savegame_Save(CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::EngineSavegameChunk::WorldMusic, 1);
		io->IO_Memory_WriteBinary(&gWorldMusicState, 0x68);
		io->IO_Tool_Chunk_FinishAndWrite();
	}
	// Original function: FUN_00486ed9.
	static void E2ParticelManager_IO_Savegame_Save(void* manager, CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::EngineSavegameChunk::Particles, 1);
		io->IO_Memory_WriteBinary(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Generators, NC2Layout::RecordSize::ParticleGeneratorPoolBytes);
		io->IO_Tool_Chunk_FinishAndWrite();
	}
	// Original function: FUN_00486f0e.
	static void E2ParticelManager_IO_Savegame_Load(void* manager, CIoHelper* io)
	{
		E2ParticelManager_Initialize(manager);
		if (io->mReadHeader.version != 1) io->IO_File_Chunk_Skip();
		else io->IO_File_ReadBinary(static_cast<std::byte*>(manager) + NC2Layout::ParticleManagerOffset::Generators, NC2Layout::RecordSize::ParticleGeneratorPoolBytes);
	}

	// Original function: FUN_0047f272; the Windows-only music chunk precedes weather.
	static void E2Manager_l_IO_Savegame_Save(CIoHelper* io)
	{
		io->IO_File_Chunk_WriteMarkerAndVersion(NC2DataIds::EngineSavegameChunk::EngineHeader, 0, 0);
		EngineMap_IO_Savegame_Save(io);
		DynamicDataHumanManager_IO_Savegame_Save(io);
		DynamicDataVehicleManager_IO_Savegame_Save(io);
		DynamicDataAnimalManager_IO_Savegame_Save(io);
		DynamicDataHouseManager_IO_Savegame_Save(io);
		E2ParticelManager_IO_Savegame_Save(gE2ParticelManager, io);
		WorldMusic_IO_Savegame_Save(io);
		E2MapWeatherManagerSectorManager_IO_Savegame_Save(gE2MapWeatherManagerSectorManager, io);
		io->IO_Tool_Chunk_WriteEndChunk();
	}
	// Original function: FUN_0047f2d0; unknown engine chunks terminate dispatch.
	static void E2Manager_l_IO_Savegame_Load(CIoHelper* io)
	{
		std::uint32_t version = 0;
		if (!io->IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::EngineSavegameChunk::EngineHeader, &version, nullptr)) return;
		io->IO_File_Chunk_Skip();
		while (io->IO_File_Chunk_ReadHeader())
		{
			switch (io->mReadHeader.marker)
			{
				case SwitchIds::EngineSavegameChunk::DynamicAnimals: DynamicDataAnimalManager_IO_Savegame_Load(io); break;
				case SwitchIds::EngineSavegameChunk::DynamicHumans: DynamicDataHumanManager_IO_Savegame_Load(io); break;
				case SwitchIds::EngineSavegameChunk::DynamicHouses: DynamicDataHouseManager_IO_Savegame_Load(io); break;
				case SwitchIds::EngineSavegameChunk::DynamicVehicles: DynamicDataVehicleManager_IO_Savegame_Load(io); break;
				case SwitchIds::EngineSavegameChunk::WorldMusic: WorldMusic_IO_Savegame_Load(io); break;
				case SwitchIds::EngineSavegameChunk::Particles: E2ParticelManager_IO_Savegame_Load(gE2ParticelManager, io); break;
				case SwitchIds::EngineSavegameChunk::Weather: E2MapWeatherManagerSectorManager_IO_Savegame_Load(gE2MapWeatherManagerSectorManager, io); break;
				case SwitchIds::EngineSavegameChunk::EngineMap: EngineMap_IO_Savegame_Load(io); break;
				default: return;
			}
		}
	}

	// Original function: FUN_0047ea9f.
	bool __cdecl E2Manager_sl_Callback_MapIO(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t param1, std::uintptr_t /*param2*/, std::uintptr_t /*param3*/)
	{
		auto* io = reinterpret_cast<CIoHelper*>(static_cast<std::uintptr_t>(param1));
		switch (type)
		{
			case SwitchIds::CallbackType::SaveCleanmap: EngineMap_IO_Cleanmap_Save(io); break;
			case SwitchIds::CallbackType::LoadCleanmap: EngineMap_IO_Cleanmap_Load(io); break;
			case SwitchIds::CallbackType::SaveSavegame: E2Manager_l_IO_Savegame_Save(io); break;
			case SwitchIds::CallbackType::LoadSavegame: E2Manager_l_IO_Savegame_Load(io); break;
			case SwitchIds::CallbackType::LoadDone:
				EngineMap_PostLoad_RebuildIfRequired();
				C2DEngineDisplayManager::sTheObjectPtr->InformThatLoadDone();
				break;
		}
		return true;
	}
}
