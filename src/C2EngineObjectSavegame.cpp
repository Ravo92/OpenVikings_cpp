#include "C2EngineMapIO.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineObjectData.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2SavegameFormats.h"
#include "SwitchIdentifiers.h"
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
		void WritePaletteRecord(void* palette, NC2SaveFormat::SPaletteRecord& record)
		{
			NXBasics::Palette_PaletteChanged(palette);
			std::memcpy(record.colors, static_cast<const std::byte*>(palette) + NXBasics::PaletteOffset::Colors, sizeof(record.colors));
		}

		void ReadPaletteRecord(void* palette, const NC2SaveFormat::SPaletteRecord& record)
		{
			// Saved x86 vtable/cache words never become native addresses.
			NXBasics::Palette_PaletteChanged(palette);
			std::memcpy(static_cast<std::byte*>(palette) + NXBasics::PaletteOffset::Colors, record.colors, sizeof(record.colors));
		}

		void WriteHumanGraphics(void* human, CIoHelper* io)
		{
			NC2SaveFormat::SHumanGraphicsRecord record{ };
			record.bodyVariant = Read<std::uint32_t>(human, NC2Layout::DynamicHumanOffset::BodyVariant);
			record.headVariant = Read<std::uint32_t>(human, NC2Layout::DynamicHumanOffset::HeadVariant);
			WritePaletteRecord(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::BodyPalette, record.bodyPalette);
			WritePaletteRecord(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::HeadPalette, record.headPalette);
			std::memcpy(record.name, static_cast<const std::byte*>(human) + NC2Layout::DynamicHumanOffset::Name, sizeof(record.name));
			record.hasUserMessage = Read<std::uint32_t>(human, NC2Layout::DynamicHumanOffset::HasUserMessageFlag);
			io->IO_Memory_WriteBinary(&record, sizeof(record));
		}

		void ReadHumanGraphics(void* human, CIoHelper* io)
		{
			NC2SaveFormat::SHumanGraphicsRecord record{ };
			io->IO_File_ReadBinary(&record, sizeof(record));
			Write(human, NC2Layout::DynamicHumanOffset::BodyVariant, record.bodyVariant);
			Write(human, NC2Layout::DynamicHumanOffset::HeadVariant, record.headVariant);
			ReadPaletteRecord(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::BodyPalette, record.bodyPalette);
			ReadPaletteRecord(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::HeadPalette, record.headPalette);
			std::memcpy(static_cast<std::byte*>(human) + NC2Layout::DynamicHumanOffset::Name, record.name, sizeof(record.name));
			Write(human, NC2Layout::DynamicHumanOffset::HasUserMessageFlag, record.hasUserMessage);
		}
	}

	// Original function: FUN_00490c90; SDisplayAtomInformation::IO_Savegame_Save.
	static void DisplayAtomInformation_IO_Savegame_Save(void* atom, CIoHelper* io)
	{
		for (std::size_t offset = NC2Layout::DisplayAtomInformationOffset::BodyBobManager; offset <= NC2Layout::DisplayAtomInformationOffset::ShadowBobManager; offset += sizeof(void*))
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

		for (std::size_t offset = NC2Layout::DisplayAtomInformationOffset::BodyBobManager; offset <= NC2Layout::DisplayAtomInformationOffset::ShadowBobManager; offset += sizeof(void*))
		{
			const char* filename = io->IO_File_ReadString();

			if (std::strlen(filename) != 0)
				Write(atom, offset, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, filename));
		}
	}

	namespace
	{
		template <class Array>
		void SaveDynamicObjects(CIoHelper* io, std::uint32_t marker, SDynamicObjectArrayStorage& storage, std::size_t stride, std::uint32_t payload, bool atom, bool human)
		{
			io->IO_Tool_Chunk_StartCreation(marker, 1);
			io->IO_Memory_WriteLong(1);

			for (std::int32_t i = Array::sTheObjectPtr->mFirstScanIndex; i <= Array::sTheObjectPtr->mLastScanIndex; ++i)
			{
				if (Read<std::uint8_t>(Array::mStaticVars, static_cast<std::uint32_t>(i) * Array::ElementSize) == 0)
					continue;

				auto* entry = storage.entries + static_cast<std::size_t>(i) * stride;

				if (atom)
					DisplayAtomInformation_IO_Savegame_Save(entry, io);

				if (human)
				{
					WriteHumanGraphics(entry, io);
				}
				else
				{
					io->IO_Memory_WriteBinary(entry + (atom ? NC2Layout::DisplayAtomInformationOffset::ActorSpecificData : 0), payload);
				}
			}

			io->IO_Tool_Chunk_FinishAndWrite();
		}

		template <class Array>
		void LoadDynamicObjects(CIoHelper* io, SDynamicObjectArrayStorage& storage, std::size_t stride, std::uint32_t payload, bool atom, bool human = false)
		{
			if (io->mReadHeader.version != 1)
			{
				io->IO_File_Chunk_Skip();

				return;
			}

			io->IO_File_ReadLong();

			for (std::int32_t i = Array::sTheObjectPtr->mFirstScanIndex; i <= Array::sTheObjectPtr->mLastScanIndex; ++i)
			{
				if (Read<std::uint8_t>(Array::mStaticVars, static_cast<std::uint32_t>(i) * Array::ElementSize) == 0)
					continue;

				auto* entry = storage.entries + static_cast<std::size_t>(i) * stride;

				if (atom)
					DisplayAtomInformation_IO_Savegame_Load(entry, io);

				if (human)
				{
					ReadHumanGraphics(entry, io);
				}
				else
				{
					io->IO_File_ReadBinary(entry + (atom ? NC2Layout::DisplayAtomInformationOffset::ActorSpecificData : 0), payload);
				}
			}
		}
	}

	// Original function: FUN_0048029c.
	void DynamicDataHumanManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CHumanArray>(io, SwitchIds::EngineSavegameChunk::DynamicHumans, gDynamicDataHumans, NC2Layout::RecordSize::DynamicHuman, NC2Layout::RecordSize::DynamicHumanSavegamePayload, true, true);
	}

	// Original function: FUN_00480348.
	void DynamicDataHumanManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CHumanArray>(io, gDynamicDataHumans, NC2Layout::RecordSize::DynamicHuman, NC2Layout::RecordSize::DynamicHumanSavegamePayload, true, true);
	}

	// Original function: FUN_00490798.
	void DynamicDataHouseManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CHouseArray>(io, SwitchIds::EngineSavegameChunk::DynamicHouses, gDynamicDataHouses, NC2Layout::RecordSize::DynamicHouse, NC2Layout::RecordSize::DynamicHouseSavedPrefix, false, false);
	}

	// Original function: FUN_00490817.
	void DynamicDataHouseManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CHouseArray>(io, gDynamicDataHouses, NC2Layout::RecordSize::DynamicHouse, NC2Layout::RecordSize::DynamicHouseSavedPrefix, false);
	}

	// Original function: FUN_00490918.
	void DynamicDataVehicleManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CVehicleArray>(io, SwitchIds::EngineSavegameChunk::DynamicVehicles, gDynamicDataVehicles, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true, false);
	}

	// Original function: FUN_004909a8.
	void DynamicDataVehicleManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CVehicleArray>(io, gDynamicDataVehicles, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true);
	}

	// Original function: FUN_00490b09.
	void DynamicDataAnimalManager_IO_Savegame_Save(CIoHelper* io)
	{
		SaveDynamicObjects<NC2Logic::CAnimalArray>(io, SwitchIds::EngineSavegameChunk::DynamicAnimals, gDynamicDataAnimals, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true, false);
	}

	// Original function: FUN_00490b99.
	void DynamicDataAnimalManager_IO_Savegame_Load(CIoHelper* io)
	{
		LoadDynamicObjects<NC2Logic::CAnimalArray>(io, gDynamicDataAnimals, NC2Layout::RecordSize::DynamicAnimalVehicle, 4, true);
	}

}