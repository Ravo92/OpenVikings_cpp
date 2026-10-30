#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2E2AnimalSoundManager.h"
#include "C2E2StaticDataAnimalSounds.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "DSManager.h"
#include "XCrtFileAccess.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2E2
{
	// Original function: FUN_004860ab
	CE2AnimalSoundManager::CE2AnimalSoundManager()
	{
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_004860c8
	CE2AnimalSoundManager::~CE2AnimalSoundManager()
	{
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004860d0
	void CE2AnimalSoundManager::PlayAnimalSounds(void* display)
	{
		const auto readWord = [](const void* base, std::uint32_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(value));

			return value;
		};

		const auto readShort = [](const void* base, std::uint32_t offset) -> std::int16_t
		{
			std::int16_t value;
			std::memcpy(&value, static_cast<const std::byte*>(base) + offset, sizeof(value));

			return value;
		};

		const auto addressAt = [](void* base, std::size_t offset) -> std::byte*
		{
			const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(base) + offset;

			return reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(address));
		};

		if (NDSManager::CManager::sTheObjectPtr == nullptr)
			return;

		std::uint8_t initialized;
		std::memcpy(&initialized, reinterpret_cast<const std::byte*>(NDSManager::CManager::sTheObjectPtr) + NC2Layout::DirectSoundManagerOffset::Initialized, sizeof(initialized));

		if (initialized == 0)
			return;

		const std::uint32_t update = NC2Logic::gGameTime.tick;

		if (lastUpdate == update)
		{

			return;
		}

		lastUpdate = update;

		// Inline initialization performed by 0042ae54.
		alignas(4) std::int16_t rectangle[4]{ };

		DisplayDescription_GetMapRectangle(display, readWord(display, NC2Layout::EngineDisplayOffset::WorldPixelX), readWord(display, NC2Layout::EngineDisplayOffset::WorldPixelY), readWord(display, NC2Layout::EngineDisplayOffset::BufferWidth), readWord(display, NC2Layout::EngineDisplayOffset::BufferHeight), rectangle);

		// Preserve the signed 16-bit shifts and the final 16-bit stores.
		const std::uint16_t centerXBits = static_cast<std::uint16_t>(static_cast<std::int32_t>(rectangle[0]) + (static_cast<std::int32_t>(rectangle[2]) >> 1));
		const std::uint16_t centerYBits = static_cast<std::uint16_t>(static_cast<std::int32_t>(rectangle[1]) + (static_cast<std::int32_t>(rectangle[3]) >> 1));

		std::int16_t centerX;
		std::int16_t centerY;
		std::memcpy(&centerX, &centerXBits, sizeof(centerX));
		std::memcpy(&centerY, &centerYBits, sizeof(centerY));

		// 0042a98a receives false; 0040ad55 copies this point by value.
		const std::uint32_t centerPoint = static_cast<std::uint32_t>(centerXBits) | (static_cast<std::uint32_t>(centerYBits) << 16);
		const std::int32_t searchWidth = rectangle[2];
		const std::uint32_t playerId = readWord(display, NC2Layout::EngineDisplayOffset::PlayerId);

		for (std::uint32_t countIndex = 0; countIndex < 34u; ++countIndex)
		{
			if (animalCounts[countIndex] == 0)
				continue;

			const std::int32_t tribeType = static_cast<std::int32_t>(countIndex + 8u);
			SStaticAnimalSoundData* soundData = CE2StaticDataAnimalSoundManager::sTheObjectPtr->GetStaticAnimalSoundDataPtrByTribeType(tribeType);

			if (soundData == nullptr)
				continue;

			if (animalCounts[countIndex] < soundData->minimumCount)
				continue;

			const std::int32_t chance = NXBasics::Runtime_Rand() % 1000;

			if (static_cast<std::uint32_t>(chance) > soundData->probabilityThreshold)
				continue;

			std::byte* selectedAnimal = nullptr;

			for (std::uint32_t searchPass = 0; searchPass < 21u; ++searchPass)
			{
				NC2Logic::CAnimalIterator iterator;
				std::int32_t animalIndex = iterator.GetFirstSectorAnimalId(searchPass, centerPoint, searchWidth, false, false);

				while (animalIndex != -1)
				{
					std::byte* animal = addressAt(NC2Logic::CAnimalArray::mStaticVars, static_cast<std::uint32_t>(animalIndex) * NC2Logic::CAnimalArray::RuntimeElementSize);
					bool accepted = readWord(animal, 0x14) == static_cast<std::uint32_t>(tribeType);

					if (accepted)
					{
						std::uint8_t displayFlags;
						std::memcpy(&displayFlags, static_cast<const std::byte*>(display) + 0x70, sizeof(displayFlags));

						if ((displayFlags & 4u) != 0)
						{
							const void* position = NC2Logic::Animal_GetMapPositionRef(animal);
							const void* visibilityData = NC2Logic::GameMap_GetConstMapMicroPointPtr(position);

							if (playerId < 16u)
							{
								std::uint16_t visibility;
								std::memcpy(&visibility, static_cast<const std::byte*>(visibilityData) + 8, sizeof(visibility));
								accepted = (visibility & (1u << playerId)) != 0;
							}
						}
					}

					if (accepted)
					{
						selectedAnimal = animal;
						break;
					}

					animalIndex = iterator.GetNextSectorAnimalId();
				}

				if (selectedAnimal != nullptr)
					break;
			}

			if (selectedAnimal == nullptr)
				continue;

			auto* staticManager = CE2StaticDataSoundFXManager::sTheObjectPtr;

			if (staticManager == nullptr || staticManager->staticSounds == nullptr || soundData->staticSoundId >= staticManager->staticSoundCount)
				continue;

			std::byte* staticRecord = addressAt(staticManager->staticSounds, soundData->staticSoundId * 0x43Cu);

			if (readWord(staticRecord, 0x28) == 0)
				continue;

			const std::uint32_t randomValue = static_cast<std::uint32_t>(NXBasics::Runtime_Rand());
			const std::uint32_t variant = randomValue % readWord(staticRecord, 0x28);
			std::byte* soundEntry = addressAt(staticRecord, 0x2Cu + variant * 8u);

			const void* position = NC2Logic::Animal_GetMapPositionRef(selectedAnimal);
			const std::int32_t horizontal = ((static_cast<std::int32_t>(readShort(position, 0)) - centerX) * 128) / rectangle[2];

			position = NC2Logic::Animal_GetMapPositionRef(selectedAnimal);
			const std::int32_t vertical = ((static_cast<std::int32_t>(readShort(position, 2)) - centerY) * 128) / rectangle[3];

			const std::int32_t absoluteHorizontal = horizontal < 0 ? -horizontal : horizontal;
			const std::int32_t absoluteVertical = vertical < 0 ? -vertical : vertical;
			const std::int32_t distance = (absoluteHorizontal + absoluteVertical) / 2;

			if (distance > 64)
				continue;

			const std::uint32_t baseVolume = readWord(soundEntry, 4);

			if (NDSManager::CManager::sTheObjectPtr->PreloadWave_IsRunning(readWord(soundEntry, 0)))
				continue;

			const std::uint32_t pan = static_cast<std::uint32_t>(horizontal + 64);
			const std::uint32_t volume = ((64u - static_cast<std::uint32_t>(distance)) * baseVolume) >> 6;

			NDSManager::CManager::sTheObjectPtr->PreloadWave_Play(readWord(soundEntry, 0), volume, 1, pan, false, 0);
		}
	}
}