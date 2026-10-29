#include "C2E2StaticDataAnimalSounds.h"
#include "C2E2StaticDataSoundFX.h"
#include "DSManager.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>

namespace
{
	// Complete Windows token array 004F4780..004F47A7, including its terminator.
	const NXBasics::SToken gAnimalSoundTokens[] = {
		{ "logictribetype", SwitchIds::AnimalSoundToken::LogicTribeType },
		{ "mincount", SwitchIds::AnimalSoundToken::MinCount },
		{ "probability", SwitchIds::AnimalSoundToken::Probability },
		{ "enginesoundgroup", SwitchIds::AnimalSoundToken::EngineSoundGroup },
		{ nullptr, 0 }
	};
	// Complete lazy token state 00568568..0056856F; both fields start in PE BSS.
	std::uint8_t gAnimalSoundTokenInitFlags = 0;
	const NXBasics::SToken* gAnimalSoundTokenTable = nullptr;
}

namespace NC2E2
{
	static int __cdecl DestroyAnimalSoundTokenList();

	// Original function: FUN_0048a969
	CE2StaticDataAnimalSoundManager::CE2StaticDataAnimalSoundManager()
	{
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));

		if (NDSManager::CManager::sTheObjectPtr == nullptr)
			return;

		std::uint8_t initialized;
		std::memcpy(&initialized, reinterpret_cast<const std::byte*>(NDSManager::CManager::sTheObjectPtr) + NC2Layout::DirectSoundManagerOffset::Initialized, sizeof(initialized));

		if (initialized == 0)
			return;

		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\animals\\sounds.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return;

		count = reader.GetNumberOfGroups("sounds");

		if (count == 0)
			return;

		if (count > UINT32_MAX / sizeof(SStaticAnimalSoundData)) { count = 0; return; }
		entries = static_cast<SStaticAnimalSoundData*>(NXBasics::Runtime_AllocateWithNewHandler(count << 4, 1));
		if (entries == nullptr) { count = 0; return; }
		std::memset(entries, 0, count << 4);

		SStaticAnimalSoundData* entry = entries;
		for (std::uint32_t i = 0; i < count; ++i) entries[i].staticSoundId = UINT32_MAX;

		for (char* group = reader.GetFirstGroup("sounds"); group != nullptr; group = reader.GetNextGroup("sounds"))
		{
			if (entry == entries + count) break;
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gAnimalSoundTokenInitFlags & 1u) == 0)
				{
					gAnimalSoundTokenInitFlags |= 1u;
					gAnimalSoundTokenTable = gAnimalSoundTokens;
					NXBasics::Runtime_AtExit(DestroyAnimalSoundTokenList);
				}

				NXBasics::CTokenList tokens(gAnimalSoundTokenTable);
				const std::uint32_t command = tokens.FindToken(reader.GetCurrentName());

				switch (command)
				{
					case SwitchIds::AnimalSoundToken::EngineSoundGroup:
						{
							auto* soundManager = CE2StaticDataSoundFXManager::sTheObjectPtr;
							const char* soundName = reader.GetString();
							entry->staticSoundId = soundManager == nullptr ? UINT32_MAX : soundManager->GetStaticSoundFxIdByNamePtr(soundName);
							break;
						}

					case SwitchIds::AnimalSoundToken::LogicTribeType:
						entry->logicTribeType = reader.GetInt();
						break;

					case SwitchIds::AnimalSoundToken::MinCount:
						entry->minimumCount = static_cast<std::uint32_t>(reader.GetInt());
						break;

					case SwitchIds::AnimalSoundToken::Probability:
						entry->probabilityThreshold = static_cast<std::uint32_t>(reader.GetInt());
						break;
				}
			}

			++entry;
		}
	}

	// Original function: FUN_0048ab10
	static int __cdecl DestroyAnimalSoundTokenList()
	{
		// The original callback performs no cleanup.
		return 0;
	}

	// Original function: FUN_0048ab11
	CE2StaticDataAnimalSoundManager::~CE2StaticDataAnimalSoundManager()
	{
		if (entries != nullptr)
		{
			NXBasics::Runtime_Free(entries);
			entries = nullptr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_0048ab2f
	SStaticAnimalSoundData* CE2StaticDataAnimalSoundManager::GetStaticAnimalSoundDataPtrByTribeType(std::int32_t tribeType)
	{
		const std::uint32_t entryCount = count;

		if (entryCount == 0)
			return nullptr;

		SStaticAnimalSoundData* firstEntry = entries;

		for (std::uint32_t index = 0; index < entryCount; ++index)
		{
			if (firstEntry[index].logicTribeType == tribeType)
				return firstEntry + index;
		}

		return nullptr;
	}
}
