#include "ApplicationGlobals.h"
#include "C2E2AmbientSoundManagerAccess.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineLifetimeAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapGeometry.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "DSManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>


namespace NC2E2
{
	template<class T> static T Ambient_ReadField(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
		return value;
	}

	static std::byte* Ambient_AddressAt(void* object, std::size_t offset)
	{
		const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(object) + offset;
		return reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(address));
	}

	struct SAmbientLoopEntry
	{
		std::uint32_t ambientId;
		std::uint32_t volume;
	};

	struct SSoundLoopIds
	{
		std::int32_t count;
		SAmbientLoopEntry entries[10];
	};

	static_assert(sizeof(SSoundLoopIds) == 84);

	static bool Ambient_SoundDeviceIsReady()
	{
		return NDSManager::CManager::sTheObjectPtr != nullptr && Ambient_ReadField<std::uint8_t>(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Initialized) != 0;
	}

	static void* Ambient_GetStaticRecord(std::uint32_t ambientId)
	{
		return Ambient_AddressAt(CE2StaticDataSoundFXManager::sTheObjectPtr->ambientSounds, ambientId * NC2Layout::RecordSize::StaticAmbientSoundUnsigned);
	}

	static void* Ambient_GetSector(void* manager, std::int32_t x, std::int32_t y);

	// Complete contiguous scratch region: 00554DB4..005550D3, two 100-DWORD arrays.
	static std::uint32_t gLandscapeAmbientGroupCounts[100]{ }; // 00554DB4..00554F43.
	static std::uint32_t gPatternAmbientGroupCounts[100]{ }; // 00554F44..005550D3.

	// Original function: FUN_0047f3c7.
	void* __fastcall AmbientSoundManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2AmbientSoundManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::AmbientSoundManager);
		std::memset(&gAmbientSoundChangeState, 0, NC2Layout::RecordSize::AmbientChangeState);
		if (NDSManager::CManager::sTheObjectPtr != nullptr)
		{
			for (const auto type : { 2u, 0x16u, 0x31u, 0x32u, 0x33u }) NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(type, E2AmbientSoundManager_Callback, 0x32, 0xC8);
		}
		return manager;
	}

	// Original function: FUN_0047f4d6; CE2AmbientSoundManager::l_AllocateMemory.
	void AmbientSoundManager_AllocateMemory(void* manager)
	{
		using namespace NC2Lifetime;
		if (Read<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors) != nullptr)
		{
			NXBasics::Runtime_Free(Read<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors));
			Write<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors, nullptr);
		}
		Write<std::int32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorWidth, (static_cast<std::int32_t>(static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth())) + NC2Runtime::AmbientSoundGrid::MacroRoundUp) / NC2Runtime::AmbientSoundGrid::MacroPointsPerSector);
		Write<std::int32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorHeight, (static_cast<std::int32_t>(static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight())) + NC2Runtime::AmbientSoundGrid::MacroRoundUp) / NC2Runtime::AmbientSoundGrid::MacroPointsPerSector);
		const auto size = Read<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorHeight) * NC2Layout::RecordSize::AmbientSectorUnsigned;
		void* sectors = NXBasics::Runtime_Allocate(size);
		Write<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors, sectors);
		const auto clearSize = Read<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorWidth) * Read<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorHeight) * NC2Layout::RecordSize::AmbientSectorUnsigned;
		std::memset(sectors, 0, clearSize);
	}

	// Original function: FUN_0047f56e; l_RecalculatePatternAmbientSoundLoops.
	static void AmbientSoundManager_RecalculatePatternAmbientSoundLoops(void* sectorData, const NC2Logic::SSectorGridCursor* sector)
	{
		using namespace NC2Lifetime;
		Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternCount, 0);
		std::memset(gPatternAmbientGroupCounts, 0, sizeof(gPatternAmbientGroupCounts));
		const auto startX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(sector->x) * NC2Runtime::AmbientSoundGrid::MacroPointsPerSectorUnsigned);
		const auto startY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(sector->y) * NC2Runtime::AmbientSoundGrid::MacroPointsPerSectorUnsigned);
		const auto endX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(startX) + NC2Runtime::AmbientSoundGrid::MacroPointsPerSectorUnsigned);
		const auto endY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(startY) + NC2Runtime::AmbientSoundGrid::MacroPointsPerSectorUnsigned);
		for (auto y = startY; y < endY; ++y)
		{
			for (auto x = startX; x < endX; ++x)
			{
				const std::int16_t point[2] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
				if (NC2Logic::MapMagPoint_IsBorderPoint(point)) continue;
				const auto* data = Engine2d_EntryPoint_Tool_GetEngineMacroPointDataPtr(point);
				for (std::uint32_t triangle = 0; triangle < 2; ++triangle)
				{
					const auto pattern = Read<std::uint16_t>(data, triangle * NC2Layout::RecordSize::EnginePatternIdUnsigned);
					// Original lookup entry: FUN_00480fe3, 70-byte static pattern records.
					const auto* record = static_cast<const std::byte*>(gStaticPatternEntries) + static_cast<std::uint32_t>(pattern) * NC2Layout::RecordSize::StaticEnginePatternUnsigned;
					for (std::uint32_t index = 0; index < Read<std::uint32_t>(record, NC2Layout::TerrainAmbientGroupsOffset::Count); ++index)
					{
						const auto group = Read<std::uint32_t>(record, NC2Layout::TerrainAmbientGroupsOffset::IdsUnsigned + index * NC2Layout::RecordSize::AmbientGroupIdUnsigned);
						// Windows 83 /7 sign-extends the FF immediate to FFFFFFFF.
						if (group < NC2Runtime::AmbientSoundLimits::GroupCount && CE2StaticDataSoundFXManager::sTheObjectPtr != nullptr && CE2StaticDataSoundFXManager::sTheObjectPtr->patternGroupAmbientIds[group] < CE2StaticDataSoundFXManager::sTheObjectPtr->ambientSoundCount) ++gPatternAmbientGroupCounts[group];
					}
				}
			}
		}
		for (std::uint32_t group = 0; group < NC2Runtime::AmbientSoundLimits::GroupCount; ++group)
		{
			const auto weight = gPatternAmbientGroupCounts[group];
			if (weight == 0) continue;
			std::int32_t position = 0;
			auto count = Read<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternCount);
			while (position < count && weight <= gPatternAmbientGroupCounts[Read<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(position) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned)]) ++position;
			if (position < count)
			{
				for (auto destination = NC2Runtime::AmbientSoundLimits::LastSectorEntry; destination > position; --destination) Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(destination) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, Read<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(destination - 1) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned));
				Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(position) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, group);
				if (count < NC2Runtime::AmbientSoundLimits::SectorEntries) Write<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternCount, ++count);
			}
			if (position == count && count < NC2Runtime::AmbientSoundLimits::SectorEntries)
			{
				Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(count) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, group);
				Write<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternCount, count + 1);
			}
		}
		for (std::int32_t index = 0; index < Read<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::PatternCount); ++index)
		{
			const auto offset = NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned;
			const auto group = Read<std::uint32_t>(sectorData, offset);
			Write<std::uint32_t>(sectorData, offset + NC2Layout::AmbientLoopEntryOffset::VolumeUnsigned, gPatternAmbientGroupCounts[group] * NC2Runtime::AmbientSoundGrid::FullPercentageUnsigned / NC2Runtime::AmbientSoundGrid::PatternTrianglesPerSectorUnsigned);
			Write<std::uint32_t>(sectorData, offset, CE2StaticDataSoundFXManager::sTheObjectPtr->patternGroupAmbientIds[group]);
		}
	}

	// Original function: FUN_0047f760; l_RecalculateLandscapeAmbientSounds.
	static void AmbientSoundManager_RecalculateLandscapeAmbientSounds(void* sectorData, const NC2Logic::SSectorGridCursor* sector)
	{
		using namespace NC2Lifetime;
		Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeCount, 0);
		std::memset(gLandscapeAmbientGroupCounts, 0, sizeof(gLandscapeAmbientGroupCounts));
		const auto startX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(sector->x) * NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned);
		const auto startY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(sector->y) * NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned);
		const auto endX = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(startX) + NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned);
		const auto endY = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(startY) + NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned);
		for (auto y = startY; y < endY; ++y)
		{
			for (auto x = startX; x < endX; ++x)
			{
				const std::int16_t point[2] = { static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
				if (NC2Logic::MapMigPoint_IsBorderPoint(point)) continue;
				const auto* data = Engine2d_EntryPoint_Tool_GetEngineMicroPointDataPtr(point);
				const auto landscape = Read<std::uint16_t>(data, NC2Layout::EngineMicroPointOffset::LandscapeId);
				if (landscape == NXBasics::IntegerRange::UInt16Maximum) continue;
				// Original lookup entry: FUN_00480ff1; native runtime landscape stride.
				const auto* record = static_cast<const std::byte*>(gStaticLandscapeEntries) + static_cast<std::size_t>(landscape) * NC2Layout::RecordSize::StaticEngineLandscapeUnsigned;
				for (std::uint32_t index = 0; index < Read<std::uint32_t>(record, NC2Layout::TerrainAmbientGroupsOffset::Count); ++index)
				{
					const auto group = Read<std::uint32_t>(record, NC2Layout::TerrainAmbientGroupsOffset::IdsUnsigned + index * NC2Layout::RecordSize::AmbientGroupIdUnsigned);
					if (group < NC2Runtime::AmbientSoundLimits::GroupCount && CE2StaticDataSoundFXManager::sTheObjectPtr != nullptr && CE2StaticDataSoundFXManager::sTheObjectPtr->landscapeGroupAmbientIds[group] < CE2StaticDataSoundFXManager::sTheObjectPtr->ambientSoundCount) ++gLandscapeAmbientGroupCounts[group];
				}
			}
		}
		for (std::uint32_t group = 0; group < NC2Runtime::AmbientSoundLimits::GroupCount; ++group)
		{
			const auto weight = gLandscapeAmbientGroupCounts[group];
			if (weight == 0) continue;
			std::int32_t position = 0;
			auto count = Read<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeCount);
			while (position < count && weight <= gLandscapeAmbientGroupCounts[Read<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(position) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned)]) ++position;
			if (position < count)
			{
				for (auto destination = NC2Runtime::AmbientSoundLimits::LastSectorEntry; destination > position; --destination) Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(destination) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, Read<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(destination - 1) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned));
				Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(position) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, group);
				if (count < NC2Runtime::AmbientSoundLimits::SectorEntries) Write<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeCount, ++count);
			}
			if (position == count && count < NC2Runtime::AmbientSoundLimits::SectorEntries)
			{
				Write<std::uint32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(count) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned, group);
				Write<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeCount, count + 1);
			}
		}
		for (std::int32_t index = 0; index < Read<std::int32_t>(sectorData, NC2Layout::AmbientSectorOffset::LandscapeCount); ++index)
		{
			const auto offset = NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned;
			const auto group = Read<std::uint32_t>(sectorData, offset);
			Write<std::uint32_t>(sectorData, offset + NC2Layout::AmbientLoopEntryOffset::VolumeUnsigned, gLandscapeAmbientGroupCounts[group]);
			Write<std::uint32_t>(sectorData, offset, CE2StaticDataSoundFXManager::sTheObjectPtr->landscapeGroupAmbientIds[group]);
		}
	}

	// Original function: FUN_0047f533.
	void AmbientSoundManager_RecalculateAmbientSoundSector(void* manager, const NC2Logic::SSectorGridCursor* sector, bool patterns, bool landscapes)
	{
		void* data = Ambient_GetSector(manager, sector->x, sector->y);
		if (patterns) AmbientSoundManager_RecalculatePatternAmbientSoundLoops(data, sector);
		if (landscapes) AmbientSoundManager_RecalculateLandscapeAmbientSounds(data, sector);
	}

	// Original function: FUN_0047f90b.
	void AmbientSoundManager_RecalculateAllAmbientSoundSector(void* manager)
	{
		for (std::uint32_t y = 0; y < Ambient_ReadField<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorHeight); ++y)
		{
			for (std::uint32_t x = 0; x < Ambient_ReadField<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorWidth); ++x)
			{
				NC2Logic::SSectorGridCursor sector;
				sector.x = NXBasics::Runtime_TimeSigned32(x);
				sector.y = NXBasics::Runtime_TimeSigned32(y);
				AmbientSoundManager_RecalculateAmbientSoundSector(manager, &sector, true, true);
			}
		}
	}

	// Original function: FUN_0047f947; CE2AmbientSoundManager::l_UpdateLoopList.
	static void AmbientSoundManager_UpdateLoopList(void* manager, const SSoundLoopIds& requested)
	{
		if (!Ambient_SoundDeviceIsReady())
			return;

		auto* previous = reinterpret_cast<SSoundLoopIds*>(Ambient_AddressAt(manager, NC2Layout::AmbientSoundManagerOffset::ActiveLoops));
		for (std::int32_t index = 0; index < requested.count; ++index)
		{
			const auto& entry = requested.entries[index];
			std::int32_t previousIndex = 0;
			while (previousIndex < previous->count && previous->entries[previousIndex].ambientId != entry.ambientId)
				++previousIndex;

			if (previousIndex == previous->count || (previousIndex < previous->count && previous->entries[previousIndex].volume != entry.volume))
			{
				const std::uint32_t waveId = Ambient_ReadField<std::uint32_t>(Ambient_GetStaticRecord(entry.ambientId), NC2Layout::StaticAmbientSoundOffset::FirstWaveId);
				if (NDSManager::CManager::sTheObjectPtr->PreloadWave_IsRunning(waveId))
					NDSManager::CManager::sTheObjectPtr->PreloadWave_ChangeVolume(waveId, entry.volume, true, 1000);
				else
					NDSManager::CManager::sTheObjectPtr->PreloadWave_Play(waveId, entry.volume, NXBasics::IntegerRange::UInt32Maximum, 64, true, 1000);
			}

			if (previousIndex < previous->count)
				previous->entries[previousIndex].ambientId = NXBasics::IntegerRange::UInt32Maximum;
		}

		for (std::int32_t index = 0; index < previous->count; ++index)
		{
			if (previous->entries[index].ambientId == NXBasics::IntegerRange::UInt32Maximum)
				continue;
			const std::uint32_t waveId = Ambient_ReadField<std::uint32_t>(Ambient_GetStaticRecord(previous->entries[index].ambientId), NC2Layout::StaticAmbientSoundOffset::FirstWaveId);
			if (NDSManager::CManager::sTheObjectPtr->PreloadWave_IsRunning(waveId))
				NDSManager::CManager::sTheObjectPtr->PreloadWave_ChangeVolume(waveId, 0, true, 1000);
		}
		std::memcpy(previous, &requested, sizeof(requested));
	}

	// Original predicate: FUN_004585a4, SSectorGridCursor::IsInsideMap.
	static bool Ambient_SectorIsInsideMap(std::int32_t x, std::int32_t y)
	{
		return x < NC2Logic::SSectorGridCursor::sGridWidth && x >= 0 && y < NC2Logic::SSectorGridCursor::sGridHeight && y >= 0;
	}

	static std::uint32_t Ambient_GetSectorCenter(std::int32_t x, std::int32_t y)
	{
		std::uint16_t pointX = static_cast<std::uint16_t>(static_cast<std::uint32_t>(x) * NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned + NC2Runtime::AmbientSoundGrid::MicroSectorCenterUnsigned);
		std::uint16_t pointY = static_cast<std::uint16_t>(static_cast<std::uint32_t>(y) * NC2Runtime::AmbientSoundGrid::MicroPointsPerSectorUnsigned + NC2Runtime::AmbientSoundGrid::MicroSectorCenterUnsigned);
		std::int16_t signedX;
		std::int16_t signedY;
		std::memcpy(&signedX, &pointX, sizeof(signedX));
		std::memcpy(&signedY, &pointY, sizeof(signedY));
		// 005125f8/005125fa mirror the map dimensions during map initialization.
		// The current reconstruction keeps their storage in the game-map block.
		if (signedX >= NC2Logic::GameMap_GetWidth())
			pointX = static_cast<std::uint16_t>(static_cast<std::uint32_t>(NC2Logic::GameMap_GetWidth()) - 1u);
		if (signedY >= NC2Logic::GameMap_GetHeight())
			pointY = static_cast<std::uint16_t>(static_cast<std::uint32_t>(NC2Logic::GameMap_GetHeight()) - 1u);
		return pointX | (static_cast<std::uint32_t>(pointY) << 16);
	}

	static bool Ambient_SectorIsVisible(const void* display, std::uint32_t point)
	{
		const std::uint32_t player = Ambient_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		if ((Ambient_ReadField<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 4u) == 0 || player >= 16u)
			return true;
		const void* microPoint = NC2Logic::GameMap_GetConstMapMicroPointPtr(&point);
		return (Ambient_ReadField<std::uint16_t>(microPoint, NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) != 0;
	}

	static void* Ambient_GetSector(void* manager, std::int32_t x, std::int32_t y)
	{
		const std::uint32_t index = Ambient_ReadField<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::SectorWidth) * static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(x);
		return Ambient_AddressAt(Ambient_ReadField<void*>(manager, NC2Layout::AmbientSoundManagerOffset::Sectors), index * NC2Layout::RecordSize::AmbientSectorUnsigned);
	}

	// Original function: FUN_0047fdca; CE2AmbientSoundManager::l_UpdateAmbientLoops.
	static void AmbientSoundManager_UpdateAmbientLoops(void* manager, const NC2Logic::SSectorGridCursor& first, const NC2Logic::SSectorGridCursor& last, const void* display)
	{
		SSoundLoopIds requested;
		requested.count = 0;
		for (std::int32_t slot = 0; slot < NC2Runtime::AmbientSoundLimits::SectorEntries; ++slot)
		{
			for (std::int32_t y = first.y; y <= last.y; ++y)
			{
				for (std::int32_t x = first.x; x <= last.x; ++x)
				{
					if (!Ambient_SectorIsInsideMap(x, y) || !Ambient_SectorIsVisible(display, Ambient_GetSectorCenter(x, y)))
						continue;

					const void* sector = Ambient_GetSector(manager, x, y);
					if (slot >= Ambient_ReadField<std::int32_t>(sector, NC2Layout::AmbientSectorOffset::PatternCount))
						continue;
					const std::uint32_t ambientId = Ambient_ReadField<std::uint32_t>(sector, NC2Layout::AmbientSectorOffset::PatternEntriesUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned);
					if (ambientId == NXBasics::IntegerRange::UInt32Maximum)
						continue;
					const std::uint32_t volume = Ambient_ReadField<std::uint32_t>(sector, NC2Layout::AmbientSectorOffset::FirstPatternVolumeUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned);
					std::int32_t index = 0;
					while (index < requested.count && requested.entries[index].ambientId != ambientId)
						++index;
					if (index < requested.count)
						requested.entries[index].volume += volume;
					else if (requested.count < NC2Runtime::AmbientSoundLimits::ActiveLoops)
						requested.entries[requested.count++] = { ambientId, volume };
				}
			}
		}
		// The Windows DIV uses the number of distinct loop IDs, not sector count.
		for (std::int32_t index = 0; index < requested.count; ++index)
			requested.entries[index].volume /= static_cast<std::uint32_t>(requested.count);
		AmbientSoundManager_UpdateLoopList(manager, requested);
	}

	// Original function: FUN_004800c8; CE2AmbientSoundManager::StopAllAmbientSounds.
	void AmbientSoundManager_StopAllAmbientSounds(void* manager)
	{
		auto* soundManager = NDSManager::CManager::sTheObjectPtr;
		if (soundManager == nullptr || Ambient_ReadField<std::uint8_t>(soundManager, NC2Layout::DirectSoundManagerOffset::Initialized) == 0)
			return;

		auto* loops = reinterpret_cast<SSoundLoopIds*>(Ambient_AddressAt(manager, NC2Layout::AmbientSoundManagerOffset::ActiveLoops));
		for (std::int32_t index = 0; index < loops->count; ++index)
		{
			if (index != 0) soundManager = NDSManager::CManager::sTheObjectPtr;
			auto& entry = loops->entries[index];
			if (entry.ambientId == NXBasics::IntegerRange::UInt32Maximum) continue;
			const std::uint32_t waveId = Ambient_ReadField<std::uint32_t>(Ambient_GetStaticRecord(entry.ambientId), NC2Layout::StaticAmbientSoundOffset::FirstWaveId);
			if (soundManager->PreloadWave_IsRunning(waveId))
			{
				NDSManager::CManager::sTheObjectPtr->PreloadWave_Stop(waveId);
				entry.ambientId = NXBasics::IntegerRange::UInt32Maximum;
			}
		}
	}

	// Original function: FUN_00480137; weighted selection within a static ambient record.
	static const void* Ambient_SelectLandscapeSound(const void* record)
	{
		std::uint32_t remaining = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % Ambient_ReadField<std::uint32_t>(record, NC2Layout::StaticAmbientSoundOffset::TotalSelectionWeight);
		std::uint32_t index = 0;
		for (;; ++index)
		{
			remaining -= Ambient_ReadField<std::uint32_t>(record, NC2Layout::StaticAmbientSoundOffset::FirstSelectionWeightUnsigned + index * NC2Layout::RecordSize::StaticAmbientWaveUnsigned);
			if ((remaining & NXBasics::IntegerRange::Bit31) != 0)
				return static_cast<const std::byte*>(record) + NC2Layout::StaticAmbientSoundOffset::SoundsUnsigned + index * NC2Layout::RecordSize::StaticAmbientWaveUnsigned;
		}
	}

	// Original function: FUN_0047fb9d; CE2AmbientSoundManager::l_PlayLandscapeAmbientSounds.
	static void AmbientSoundManager_PlayLandscapeAmbientSounds(void* manager, const NC2Logic::SSectorGridCursor& first, const NC2Logic::SSectorGridCursor& last, const std::int16_t* rectangle, const void* display)
	{
		if (!Ambient_SoundDeviceIsReady() || Ambient_ReadField<std::uint32_t>(manager, NC2Layout::AmbientSoundManagerOffset::LastLandscapeSoundTick) == NC2Logic::gGameTime.tick)
			return;

		const std::int32_t centerX = rectangle[0] + (static_cast<std::int32_t>(rectangle[2]) >> 1);
		const std::int32_t centerY = rectangle[1] + (static_cast<std::int32_t>(rectangle[3]) >> 1);
		for (std::int32_t y = first.y; y <= last.y; ++y)
		{
			for (std::int32_t x = first.x; x <= last.x; ++x)
			{
				if (!Ambient_SectorIsInsideMap(x, y))
					continue;
				const std::uint32_t point = Ambient_GetSectorCenter(x, y);
				if (!Ambient_SectorIsVisible(display, point))
					continue;
				const void* sector = Ambient_GetSector(manager, x, y);
				for (std::int32_t index = 0; index < Ambient_ReadField<std::int32_t>(sector, NC2Layout::AmbientSectorOffset::LandscapeCount); ++index)
				{
					const std::uint32_t offset = static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::AmbientLoopEntryUnsigned;
					const void* record = Ambient_GetStaticRecord(Ambient_ReadField<std::uint32_t>(sector, NC2Layout::AmbientSectorOffset::LandscapeEntriesUnsigned + offset));
					if (Ambient_ReadField<std::uint32_t>(record, NC2Layout::StaticAmbientSoundOffset::SoundCount) == 0)
						continue;
					const void* sound = Ambient_SelectLandscapeSound(record);
					const std::uint32_t chance = static_cast<std::uint32_t>(NXBasics::Runtime_Rand() % 10000);
					const std::uint32_t threshold = Ambient_ReadField<std::uint32_t>(sector, NC2Layout::AmbientSectorOffset::FirstLandscapeVolumeUnsigned + offset) * Ambient_ReadField<std::uint32_t>(sound, NC2Layout::StaticAmbientWaveOffset::ChanceMultiplier);
					if (chance >= threshold)
						continue;

					const std::int32_t pointX = Ambient_ReadField<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X);
					const std::int32_t pointY = Ambient_ReadField<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y);
					const std::int32_t horizontal = NXBasics::Runtime_TimeSigned32((static_cast<std::uint32_t>(pointX) - static_cast<std::uint32_t>(centerX)) << 7) / rectangle[2];
					const std::int32_t vertical = NXBasics::Runtime_TimeSigned32((static_cast<std::uint32_t>(pointY) - static_cast<std::uint32_t>(centerY)) << 7) / rectangle[3];
					const std::uint32_t absoluteX = horizontal < 0 ? 0u - static_cast<std::uint32_t>(horizontal) : static_cast<std::uint32_t>(horizontal);
					const std::uint32_t absoluteY = vertical < 0 ? 0u - static_cast<std::uint32_t>(vertical) : static_cast<std::uint32_t>(vertical);
					const std::int32_t distance = NXBasics::Runtime_TimeSigned32(absoluteX + absoluteY) / 2;
					if (distance > 64)
						continue;
					const std::uint32_t volume = ((64u - static_cast<std::uint32_t>(distance)) * Ambient_ReadField<std::uint32_t>(sound, NC2Layout::StaticAmbientWaveOffset::Volume)) >> 6;
					NDSManager::CManager::sTheObjectPtr->PreloadWave_Play(Ambient_ReadField<std::uint32_t>(sound, NC2Layout::StaticAmbientWaveOffset::WaveId), volume, 1, static_cast<std::uint32_t>(horizontal) + 64u, false, 0);
				}
			}
		}
		std::memcpy(static_cast<std::byte*>(manager) + NC2Layout::AmbientSoundManagerOffset::LastLandscapeSoundTick, &NC2Logic::gGameTime.tick, sizeof(NC2Logic::gGameTime.tick));
	}

	// Original function: FUN_0047fae2; CE2AmbientSoundManager::UpdateAmbientSounds.
	void AmbientSoundManager_UpdateAmbientSounds(void* manager, const void* display)
	{
		if (!Ambient_SoundDeviceIsReady())
			return;

		std::int16_t rectangle[4]{ };
		DisplayDescription_GetMapRectangle(display, Ambient_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX), Ambient_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY), Ambient_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth), Ambient_ReadField<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight), rectangle);
		const std::uint32_t firstPoint = static_cast<std::uint16_t>(rectangle[0]) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rectangle[1])) << 16);
		const std::uint16_t lastX = static_cast<std::uint16_t>(static_cast<std::int32_t>(rectangle[0]) + rectangle[2] - 1);
		const std::uint16_t lastY = static_cast<std::uint16_t>(static_cast<std::int32_t>(rectangle[1]) + rectangle[3] - 1);
		NC2Logic::SSectorGridCursor first(firstPoint);
		NC2Logic::SSectorGridCursor last(lastX | (static_cast<std::uint32_t>(lastY) << 16));
		AmbientSoundManager_UpdateAmbientLoops(manager, first, last, display);
		AmbientSoundManager_PlayLandscapeAmbientSounds(manager, first, last, rectangle, display);
	}
}
