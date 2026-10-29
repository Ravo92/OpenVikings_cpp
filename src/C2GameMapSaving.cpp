#include "C2GameMapStorage.h"
#include "C2DataIds.h"
#include "C2GameMapIO.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2MapResourceManagers.h"
#include "C2MapSectorAttachment.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Original function: FUN_00430420.
		void WriteMapSize(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MapSize, 1);
			io->IO_Memory_WriteLong(static_cast<std::uint32_t>(static_cast<std::int32_t>(Read<std::int16_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroWidth))));
			io->IO_Memory_WriteLong(static_cast<std::uint32_t>(static_cast<std::int32_t>(Read<std::int16_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroHeight))));
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_0043045a.
		void WriteMacroHeights(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MacroHeight, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMacroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMacroPointOffset::Height)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004304b8.
		void WriteMacroPatternA(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MacroPatternA, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMacroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMacroPointOffset::PatternA)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430513.
		void WriteMacroPatternB(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MacroPatternB, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMacroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMacroPointOffset::PatternB)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_0043056f.
		void WriteLandscapeTypes(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::LandscapeType, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) != 0 ? Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) : 0));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004305e2.
		void WriteLandscapeValues(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::LandscapeValency, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) != 0 ? Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430653.
		void WriteLandscapePlayers(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::LandscapePlayer, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) != 0 ? Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::LandscapePlayerId) : NXBasics::IntegerRange::UInt8MaximumSigned));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004306c5.
		void WriteContinents(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroContinent, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ContinentId)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430723.
		void WriteWalkTypes(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroWalkConnections, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::WalkingDirections)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430781.
		void WriteMaximumMoveableSizes(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroMaximumMoveableSize, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::WalkingFlags) & 7));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004307e2.
		void WriteResistance(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroPathResistance, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::WalkingFlags) >> 3) & 15));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430846.
		void WriteWalkBlocked(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroWalkingBlocked, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::WalkingFlags) >> 7));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004308a7.
		void WriteBuildBlocked(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroBuildingBlocked, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::TerrainFlags) & 1));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430908.
		void WriteRoads(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroRoad, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::TerrainFlags) >> 1) & 1));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_0043096b.
		void WriteSpecialBlocked(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroSectorBasePoint, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::TerrainFlags) >> 4) & 1));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_004309cf.
		void WriteAtomicObjects(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::AttachedObjects, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint16_t>(scratch, index * NC2Layout::RecordSize::MapPlaneWordUnsigned, static_cast<std::uint16_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 4 ? Read<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) : 0));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 2u, NC2DataIds::PackingType::Rle16);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430a3b.
		void WriteHeightFlags(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::MicroHouseBuildForbidden, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint8_t>(scratch, index * NC2Layout::RecordSize::MapPlaneByteUnsigned, static_cast<std::uint8_t>((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::TerrainFlags) >> 5) & 1));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 1u, NC2DataIds::PackingType::Rle8);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430acc.
		void WriteHouses(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::Houses, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint16_t>(scratch, index * NC2Layout::RecordSize::MapPlaneWordUnsigned, static_cast<std::uint16_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 2 ? (Read<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::HouseId) | ((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) & 1) << 15) | ((Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) & 2) << 13)) : NXBasics::IntegerRange::UInt16MaximumSigned));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 2u, NC2DataIds::PackingType::Rle16);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430b63.
		void WriteGuides(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::Guides, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint16_t>(scratch, index * NC2Layout::RecordSize::MapPlaneWordUnsigned, static_cast<std::uint16_t>(Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 3 ? Read<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) : NXBasics::IntegerRange::UInt16MaximumSigned));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 2u, NC2DataIds::PackingType::Rle16);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430bce.
		void WriteExtraValues(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::Exploration, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint16_t>(scratch, index * NC2Layout::RecordSize::MapPlaneWordUnsigned, static_cast<std::uint16_t>(Read<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::ExplorationMask)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 2u, NC2DataIds::PackingType::Rle16);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430c2f.
		void WriteFirstMapMoveableIdMapChunk(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::FirstMapMoveableId, 1);
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* records = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);

			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount); ++index)
			{
				auto* record = records + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
				Write<std::uint16_t>(scratch, index * NC2Layout::RecordSize::MapPlaneWordUnsigned, static_cast<std::uint16_t>(Read<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::FirstMapMoveableId)));
			}

			io->IO_Memory_PackAndWriteBinary(scratch, Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount) * 2u, NC2DataIds::PackingType::Rle16);
			io->IO_Tool_Chunk_FinishAndWrite();
		}

		// Original function: FUN_00430a9f.
		void WriteContinentData(CIoHelper* io)
		{
			io->IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::Continents, 1);
			io->IO_Memory_WriteBinary(gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, NC2Layout::RecordSize::MapContinentRegion);
			io->IO_Tool_Chunk_FinishAndWrite();
		}
	}

	// Original function: FUN_0042fa61.
	void __fastcall GameMap_IO_Savegame_DoSave(void* /*map*/, void* /*unusedEdx*/, CIoHelper* io)
	{
		io->IO_File_Chunk_WriteMarkerAndVersion(SwitchIds::SavegameChunk::GameMap, 1, 0);
		WriteMapSize(io);
		WriteMacroHeights(io);
		WriteMacroPatternA(io);
		WriteMacroPatternB(io);
		WriteLandscapeTypes(io);
		WriteLandscapeValues(io);
		WriteLandscapePlayers(io);
		WriteContinents(io);
		WriteWalkTypes(io);
		WriteMaximumMoveableSizes(io);
		WriteResistance(io);
		WriteWalkBlocked(io);
		WriteBuildBlocked(io);
		WriteRoads(io);
		WriteSpecialBlocked(io);
		WriteAtomicObjects(io);
		WriteHeightFlags(io);
		WriteContinentData(io);
		WriteHouses(io);
		WriteGuides(io);
		WriteExtraValues(io);
		WriteFirstMapMoveableIdMapChunk(io);
		CMapWalkSectorManager::IO_Savegame_DoSave(*io);
		CGameMapSectorAttachmentManager::sTheObjectPtr->IO_Savegame_DoSave(*io);
		static_cast<CMapLandscapeAutoTransitionManager*>(gMapLandscapeAutoTransitionManager)->IO_Savegame_DoSave(*io);
		static_cast<CMapBioRegrowManager*>(gMapBioRegrowManager)->IO_Savegame_DoSave(*io);
		static_cast<CMapFishManager*>(gMapFishManager)->IO_Savegame_DoSave(*io);
		static_cast<CRoadBuildManager*>(gRoadBuildManager)->IO_Savegame_DoSave(*io);
		static_cast<CWallBuildManager*>(gWallBuildManager)->IO_Savegame_DoSave(*io);
		io->IO_Tool_Chunk_WriteEndChunk();
	}
}