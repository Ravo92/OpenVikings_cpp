#include "C2E2StaticDataGroups.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "DSManager.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
	// Complete Windows token array 004F4A20..004F4A57, including its terminator.
	const NXBasics::SToken gSoundFxTokens[] = {
		{ "Name", SwitchIds::AmbientSoundToken::Name },
		{ "SFX", SwitchIds::AmbientSoundToken::SFX },
		{ "PatternGroup", SwitchIds::AmbientSoundToken::PatternGroup },
		{ "LandscapeGroup", SwitchIds::AmbientSoundToken::LandscapeGroup },
		{ "LogicSoundType", SwitchIds::SoundFxToken::LogicSoundType },
		{ "MusicType", SwitchIds::HumanMiscSoundToken::MusicType },
		{ nullptr, 0 }
	};
	// Complete lazy token state 00568A20..00568A27; both fields start in PE BSS.
	const NXBasics::SToken* gSoundFxTokenTable = nullptr;
	std::uint8_t gSoundFxTokenInitFlags = 0;
}

namespace NC2E2
{
	static int __cdecl DestroySoundTokenList();

	// Original functions: FUN_0042b679, FUN_0042b689
	std::uint32_t CE2StaticDataSoundFXManager::l_GetSoundCommand(const char* name)
	{
		NXBasics::CTokenList tokens(gSoundFxTokenTable);

		return tokens.FindToken(name);
	}

	// Original function: FUN_0048dc74
	CE2StaticDataSoundFXManager::CE2StaticDataSoundFXManager()
	{
		sTheObjectPtr = this;

		std::memset(this, 0, sizeof(*this));

		for (std::uint32_t& value : patternGroupAmbientIds)
		{
			value = NXBasics::IntegerRange::UInt32Maximum;
		}

		for (std::uint32_t& value : landscapeGroupAmbientIds)
		{
			value = NXBasics::IntegerRange::UInt32Maximum;
		}

		for (std::uint32_t& value : staticSoundIdsByType)
		{
			value = NXBasics::IntegerRange::UInt32Maximum;
		}

		if (NDSManager::CManager::sTheObjectPtr != nullptr)
		{
			std::uint8_t initialized;
			std::memcpy(&initialized, reinterpret_cast<const std::byte*>(NDSManager::CManager::sTheObjectPtr) + NC2Layout::DirectSoundManagerOffset::Initialized, sizeof(initialized));

			if (initialized != 0)
				l_LoadSoundFxFromIniFile("data\\engine2d\\inis\\soundfx\\soundfx.ini");
		}
	}

	// Original function: FUN_0048dccf
	CE2StaticDataSoundFXManager::~CE2StaticDataSoundFXManager()
	{
		if (staticSounds != nullptr)
		{
			NXBasics::Runtime_Free(staticSounds);
			staticSounds = nullptr;
		}

		if (jingles != nullptr)
		{
			NXBasics::Runtime_Free(jingles);
			jingles = nullptr;
		}

		if (ambientSounds != nullptr)
		{
			NXBasics::Runtime_Free(ambientSounds);
			ambientSounds = nullptr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_0048dd23
	void CE2StaticDataSoundFXManager::l_LoadSoundFxFromIniFile(const char* fileName)
	{
		NXBasics::CIniFileReader reader(fileName, nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return;

		if ((gSoundFxTokenInitFlags & 1u) == 0)
		{
			gSoundFxTokenInitFlags |= 1u;
			gSoundFxTokenTable = gSoundFxTokens;
			NXBasics::Runtime_AtExit(DestroySoundTokenList);
		}

		// Remaster: allocate replacement tables before releasing any existing owners.
		const std::uint32_t counts[] = { reader.GetNumberOfGroups("SoundFXStatic"), reader.GetNumberOfGroups("SoundFXJingle"), reader.GetNumberOfGroups("SoundFXAmbient") };
		const std::uint32_t strides[] = { sizeof(SStaticSoundFxStorage), sizeof(SJingleSoundData), sizeof(SStaticAmbientSoundStorage) };
		void* replacements[3]{ };

		for (std::uint32_t i = 0; i < 3; ++i)
		{
			if (counts[i] > UINT32_MAX / strides[i])
			{
				for (void* value : replacements)
				{
					NXBasics::Runtime_Free(value);
				}

				return;
			}

			if (counts[i] == 0)
				continue;

			replacements[i] = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::size_t>(counts[i]) * strides[i], 1);

			if (replacements[i] == nullptr)
			{
				for (void* value : replacements)
				{
					NXBasics::Runtime_Free(value);
				}

				return;
			}

			std::memset(replacements[i], 0, counts[i] * strides[i]);
		}

		NXBasics::Runtime_Free(staticSounds);
		NXBasics::Runtime_Free(jingles);
		NXBasics::Runtime_Free(ambientSounds);
		staticSounds = replacements[0];
		jingles = static_cast<SJingleSoundData*>(replacements[1]);
		ambientSounds = replacements[2];
		staticSoundCount = jingleCount = ambientSoundCount = 0;

		for (auto& id : staticSoundIdsByType)
		{
			id = UINT32_MAX;
		}

		for (auto& id : patternGroupAmbientIds)
		{
			id = UINT32_MAX;
		}

		for (auto& id : landscapeGroupAmbientIds)
		{
			id = UINT32_MAX;
		}

		const auto addressAt = [](void* base, std::size_t offset) -> std::byte*
		{
			const auto address = reinterpret_cast<std::uintptr_t>(base) + offset;

			return reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(address));
		};

		const auto readWord = [](const void* source) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, source, sizeof(value));

			return value;
		};

		const auto writeWord = [](void* destination, std::uint32_t value)
		{
			std::memcpy(destination, &value, sizeof(value));
		};

		std::uint32_t groupCount = reader.GetNumberOfGroups("SoundFXStatic");

		if (groupCount != 0)
		{

			for (char* group = reader.GetFirstGroup("SoundFXStatic"); group != nullptr; group = reader.GetNextGroup("SoundFXStatic"))
			{
				if (staticSoundCount >= groupCount)
					break;

				std::byte* record = addressAt(staticSounds, staticSoundCount * NC2Layout::RecordSize::StaticSoundFxUnsigned);
				++staticSoundCount;

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					const std::uint32_t command = l_GetSoundCommand(reader.GetCurrentName());

					switch (command)
					{
						case SwitchIds::SoundFxToken::LogicSoundType:
						{
							const std::uint32_t recordIndex = staticSoundCount - 1u;
							const std::uint32_t type = static_cast<std::uint32_t>(reader.GetInt());

							// Remaster: validate the physical 120-entry logic-sound map.

							if (type >= 120u)
								break;

							writeWord(addressAt(this, offsetof(CE2StaticDataSoundFXManager, staticSoundIdsByType) + type * sizeof(std::uint32_t)), recordIndex);
							break;
						}

						case SwitchIds::SoundFxToken::Name:
						{
							const char* name = reader.GetString();

							if (name != nullptr && std::strlen(name) < 40u)
								std::strcpy(reinterpret_cast<char*>(record), name);
						}
						break;

						case SwitchIds::SoundFxToken::SFX:
						{
							const char* soundFileName = reader.GetString();

							if (readWord(record + NC2Layout::StaticSoundFxOffset::WaveCount) >= 130u || soundFileName == nullptr || NDSManager::CManager::sTheObjectPtr == nullptr)
								break;

							if (!NXBasics::CFile::FileSystem_Tool_FileExists(soundFileName, true))
								break;

							std::uint32_t identifier;

							if (!NDSManager::CManager::sTheObjectPtr->PreloadWave_Load(soundFileName, &identifier))
								break;

							const std::uint32_t value = static_cast<std::uint32_t>(reader.GetInt());
							const std::uint32_t soundCount = readWord(record + NC2Layout::StaticSoundFxOffset::WaveCount);

							writeWord(addressAt(record, NC2Layout::StaticSoundFxOffset::WavesUnsigned + soundCount * NC2Layout::RecordSize::StaticSoundWaveUnsigned), identifier);
							writeWord(addressAt(record, NC2Layout::StaticSoundFxOffset::FirstWaveVolumeUnsigned + soundCount * NC2Layout::RecordSize::StaticSoundWaveUnsigned), value);
							writeWord(record + NC2Layout::StaticSoundFxOffset::WaveCount, soundCount + 1u);
							break;
						}
					}
				}
			}
		}

		groupCount = reader.GetNumberOfGroups("SoundFXJingle");

		if (groupCount != 0)
		{

			for (char* group = reader.GetFirstGroup("SoundFXJingle"); group != nullptr; group = reader.GetNextGroup("SoundFXJingle"))
			{
				if (jingleCount >= groupCount)
					break;

				std::byte* record = addressAt(jingles, jingleCount << 7);
				++jingleCount;

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					const std::uint32_t command = l_GetSoundCommand(reader.GetCurrentName());

					switch (command)
					{
						case SwitchIds::HumanMiscSoundToken::MusicType:
							writeWord(record, static_cast<std::uint32_t>(reader.GetInt()));
							break;

						case SwitchIds::HumanMiscSoundToken::SFX:
						{
							char* soundFileName = reinterpret_cast<char*>(record + 4);
							const char* name = reader.GetString();

							if (name == nullptr || std::strlen(name) >= 120u)
								break;

							std::strcpy(soundFileName, name);

							if (NXBasics::CFile::FileSystem_Tool_FileExists(soundFileName, true))
								writeWord(record + 0x7C, static_cast<std::uint32_t>(reader.GetInt()));

							break;
						}
					}
				}
			}
		}

		groupCount = reader.GetNumberOfGroups("SoundFXAmbient");

		if (groupCount != 0)
		{

			alignas(4) std::byte soundData[136];

			for (char* group = reader.GetFirstGroup("SoundFXAmbient"); group != nullptr; group = reader.GetNextGroup("SoundFXAmbient"))
			{
				if (ambientSoundCount >= groupCount)
					break;

				std::byte* record = addressAt(ambientSounds, ambientSoundCount * NC2Layout::RecordSize::StaticAmbientSoundUnsigned);
				++ambientSoundCount;

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					const std::uint32_t command = l_GetSoundCommand(reader.GetCurrentName());

					switch (command)
					{
						case SwitchIds::AmbientSoundToken::LandscapeGroup:
						{
							const char* groupName = reader.GetString();
							const std::uint32_t recordIndex = ambientSoundCount - 1u;
							const std::uint32_t groupIndex = static_cast<std::uint32_t>(StaticDataGroups_FindIndex(gE2StaticDataLandscapesManager, groupName));

							if (groupIndex < 100u)
								writeWord(addressAt(this, offsetof(CE2StaticDataSoundFXManager, landscapeGroupAmbientIds) + groupIndex * 4u), recordIndex);

							break;
						}

						case SwitchIds::AmbientSoundToken::Name:
						{
							const char* name = reader.GetString();

							if (name != nullptr && std::strlen(name) < 40u)
								std::strcpy(reinterpret_cast<char*>(record), name);
						}
						break;

						case SwitchIds::AmbientSoundToken::PatternGroup:
						{
							const char* groupName = reader.GetString();
							const std::uint32_t recordIndex = ambientSoundCount - 1u;
							const std::uint32_t groupIndex = static_cast<std::uint32_t>(StaticDataGroups_FindIndex(gE2StaticDataPatternManager, groupName));

							if (groupIndex < 100u)
								writeWord(addressAt(this, offsetof(CE2StaticDataSoundFXManager, patternGroupAmbientIds) + groupIndex * 4u), recordIndex);

							break;
						}

						case SwitchIds::AmbientSoundToken::SFX:
						{
							char* soundFileName = reinterpret_cast<char*>(soundData);

							if (readWord(record + NC2Layout::StaticAmbientSoundOffset::SoundCount) >= 40u || NDSManager::CManager::sTheObjectPtr == nullptr)
								break;

							const char* name = reader.GetString();

							if (name == nullptr || std::strlen(name) >= 120u)
								break;

							std::strcpy(soundFileName, name);

							if (!NXBasics::CFile::FileSystem_Tool_FileExists(soundFileName, true))
								break;

							std::uint32_t identifier;

							if (!NDSManager::CManager::sTheObjectPtr->PreloadWave_Load(soundFileName, &identifier))
								break;

							writeWord(soundData + NC2Layout::StaticAmbientWaveOffset::WaveId, identifier);
							writeWord(soundData + NC2Layout::StaticAmbientWaveOffset::SelectionWeight, static_cast<std::uint32_t>(reader.GetInt()));
							writeWord(soundData + NC2Layout::StaticAmbientWaveOffset::Volume, static_cast<std::uint32_t>(reader.GetInt()));
							writeWord(soundData + NC2Layout::StaticAmbientWaveOffset::ChanceMultiplier, static_cast<std::uint32_t>(reader.GetInt()));

							l_Ambient_AddSound(record, soundData);
							break;
						}
					}
				}
			}
		}
	}

	// Original function: FUN_0048e21c
	void CE2StaticDataSoundFXManager::l_Ambient_AddSound(void* ambientRecord, const void* soundData)
	{
		auto* record = static_cast<std::byte*>(ambientRecord);
		const auto* source = static_cast<const std::byte*>(soundData);

		std::uint32_t count;
		std::memcpy(&count, record + NC2Layout::StaticAmbientSoundOffset::SoundCount, sizeof(count));

		if (count >= NC2Runtime::AmbientSoundLimits::StaticSoundsUnsigned)
		{

			return;
		}

		std::byte* destination = record + NC2Layout::StaticAmbientSoundOffset::Sounds + count * NC2Layout::RecordSize::StaticAmbientWaveUnsigned;

		// Preserve the forward DWORD copy performed by REP MOVSD.

		for (std::uint32_t offset = 0; offset < NC2Layout::RecordSize::StaticAmbientWaveUnsigned; offset += 4u)
		{
			std::uint32_t value;
			std::memcpy(&value, source + offset, sizeof(value));
			std::memcpy(destination + offset, &value, sizeof(value));
		}

		std::memcpy(&count, record + NC2Layout::StaticAmbientSoundOffset::SoundCount, sizeof(count));
		++count;
		std::memcpy(record + NC2Layout::StaticAmbientSoundOffset::SoundCount, &count, sizeof(count));

		std::uint32_t contribution;
		std::memcpy(&contribution, source + NC2Layout::StaticAmbientWaveOffset::SelectionWeight, sizeof(contribution));

		std::uint32_t total;
		std::memcpy(&total, record + NC2Layout::StaticAmbientSoundOffset::TotalSelectionWeight, sizeof(total));
		total += contribution;
		std::memcpy(record + NC2Layout::StaticAmbientSoundOffset::TotalSelectionWeight, &total, sizeof(total));
	}

	// Original function: FUN_0048e19a
	std::uint32_t CE2StaticDataSoundFXManager::GetStaticSoundFxIdByNamePtr(const char* name)
	{
		if (name == nullptr || staticSounds == nullptr || NDSManager::CManager::sTheObjectPtr == nullptr)
			return UINT32_MAX;

		std::uint8_t initialized;
		std::memcpy(&initialized, reinterpret_cast<const std::byte*>(NDSManager::CManager::sTheObjectPtr) + NC2Layout::DirectSoundManagerOffset::Initialized, sizeof(initialized));

		if (initialized == 0 || staticSoundCount == 0)
			return UINT32_MAX;

		std::uint32_t offset = 0;

		for (std::uint32_t index = 0; index < staticSoundCount; ++index)
		{
			const auto recordAddress = reinterpret_cast<std::uintptr_t>(staticSounds) + offset;
			const auto* recordName = reinterpret_cast<const char*>(static_cast<std::uintptr_t>(recordAddress));

			if (NXBasics::Runtime_CompareStringsIgnoreCase(recordName, name) == 0)
				return index;

			offset += NC2Layout::RecordSize::StaticSoundFxUnsigned;
		}

		return UINT32_MAX;
	}

	// Original function: FUN_0048e199
	static int __cdecl DestroySoundTokenList()
	{
		// The original callback performs no cleanup.

		return 0;
	}
}