#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "GameProfile.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>


namespace NC2Logic
{
	// Original function: FUN_004402b3; no player filter is installed.
	std::int32_t CAnimalIterator::GetFirstAnimalId()
	{
		flags = 0;
		currentObjectId = l_GetNext(CAnimalArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_004402cd; allocation eviction scans in array order.
	std::int32_t CAnimalIterator::GetFirstAnimalId(std::uint32_t player)
	{
		selection = player;
		flags = 1;
		currentObjectId = l_GetNext(CAnimalArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_004402f3; this scans the array, not sector chains.
	std::int32_t CAnimalIterator::GetFirstAnimalId(std::uint32_t player, const void* point, std::int32_t searchRadius)
	{
		selection = player;
		flags = 3;
		packedCenter = NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		radius = searchRadius;
		currentObjectId = l_GetNext(CAnimalArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_004403ec.
	std::int32_t CAnimalIterator::GetNextAnimalId()
	{
		if (currentObjectId == -1) return -1;
		currentObjectId = l_GetNext(currentObjectId);
		return currentObjectId;
	}

	// Original function: FUN_00440408; array and sector iteration are distinct.
	std::int32_t CAnimalIterator::l_GetNext(std::int32_t previousAnimalId)
	{
		using namespace NC2Lifetime;
		for (auto id = previousAnimalId + 1; id <= CAnimalArray::sTheObjectPtr->mLastScanIndex; ++id)
		{
			const auto* animal = CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize;
			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || ((flags & 1u) != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) != selection)) continue;
			if ((flags & 2u) != 0)
			{
				const auto* point = Animal_GetMapPositionRef(animal);
				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), static_cast<std::int16_t>(packedCenter), static_cast<std::int16_t>(packedCenter >> 16))) > static_cast<std::uint32_t>(radius)) continue;
			}
			return id;
		}
		return -1;
	}

	// Original function: FUN_004402a3
	CAnimalIterator::CAnimalIterator()
	{
		l_InitIteratorFields();
		currentObjectId = -1;
	}

	// Original function: FUN_00440cad; shared field initialization leaves flags, selection, radius and current ID untouched.
	CAnimalIterator* CAnimalIterator::l_InitIteratorFields()
	{
		// Game.exe has WORD stores at +0Ah then +08h; the text export misdecodes the first store.
		NC2Lifetime::Write<std::uint16_t>(this, NC2Layout::AnimalIteratorOffset::CenterY, 0);
		NC2Lifetime::Write<std::uint16_t>(this, NC2Layout::AnimalIteratorOffset::CenterX, 0);
		firstSector.x = 0;
		firstSector.y = 0;
		lastSector.x = 0;
		lastSector.y = 0;
		currentSector.x = 0;
		currentSector.y = 0;
		return this;
	}

	// Original function: FUN_00440329
	std::int32_t CAnimalIterator::GetFirstSectorAnimalId(std::uint32_t selectionValue, std::uint32_t centerValue, std::int32_t radiusValue, bool flag1, bool flag2)
	{
		selection = selectionValue;
		packedCenter = centerValue;
		radius = radiusValue;

		const std::uint32_t radiusBits = static_cast<std::uint32_t>(radiusValue);
		const std::uint16_t lowerX = static_cast<std::uint16_t>((centerValue & NXBasics::IntegerRange::UInt16Maximum) - radiusBits);
		const std::uint16_t lowerY = static_cast<std::uint16_t>((centerValue >> 16) - radiusBits);
		std::uint32_t corner = static_cast<std::uint32_t>(lowerX) | (static_cast<std::uint32_t>(lowerY) << 16);

		firstSector = SSectorGridCursor(corner);

		const std::uint32_t diameterBits = radiusBits * 2u;
		const std::uint16_t upperX = static_cast<std::uint16_t>((corner & NXBasics::IntegerRange::UInt16Maximum) + diameterBits);
		const std::uint16_t upperY = static_cast<std::uint16_t>((corner >> 16) + diameterBits);
		corner = static_cast<std::uint32_t>(upperX) | (static_cast<std::uint32_t>(upperY) << 16);

		lastSector = SSectorGridCursor(corner);

		firstSector.CutInsideMap();
		lastSector.CutInsideMap();

		currentSector = firstSector;

		flags = flag1 ? 1u : 0u;

		if (flag2)
			flags += 2u;

		const std::int32_t animalId = l_GetNextSectorAnimalId(-1);
		currentObjectId = animalId;
		return animalId;
	}

	// Original function: FUN_004403d0
	std::int32_t CAnimalIterator::GetNextSectorAnimalId()
	{
		const std::int32_t previousAnimalId = currentObjectId;

		if (previousAnimalId == -1)
			return -1;

		const std::int32_t animalId = l_GetNextSectorAnimalId(previousAnimalId);
		currentObjectId = animalId;
		return animalId;
	}

	// Original function: FUN_0044048d
	std::int32_t CAnimalIterator::l_GetNextSectorAnimalId(std::int32_t previousAnimalId)
	{
		const auto addressAt = [] (const void* base, std::uint32_t offset) -> const std::byte*
		{
			const std::uint32_t address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(base)) + offset;
			return reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));
		};

		const auto readWord = [] (const void* source) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, source, sizeof(value));
			return value;
		};

		const auto readId = [] (const void* source) -> std::int32_t
		{
			std::int32_t value;
			std::memcpy(&value, source, sizeof(value));
			return value;
		};

		const auto incrementCoordinate = [] (std::int32_t& coordinate)
		{
			std::uint32_t bits;
			std::memcpy(&bits, &coordinate, sizeof(bits));
			++bits;
			std::memcpy(&coordinate, &bits, sizeof(coordinate));
		};

		std::int32_t animalId = previousAnimalId;

		for (;;)
		{
			if (animalId == -1)
			{
				const auto* grid = CGameMapSectorAttachmentManager::sTheObjectPtr;
				const std::uint32_t gridWidth = static_cast<std::uint32_t>(grid->GetGridWidth());
				const std::byte* sectorData = grid->GetSectorData();

				const std::uint32_t sectorIndex = gridWidth * static_cast<std::uint32_t>(currentSector.y) + static_cast<std::uint32_t>(currentSector.x);
				const std::uint32_t recordOffset = (sectorIndex * 21u + selection) << 6;
				const std::uint32_t headOffset = (flags & 1u) != 0 ? 0x38u : 0x30u;

				animalId = readId(addressAt(sectorData, recordOffset + headOffset));
			}
			else
			{
				const std::uint32_t animalOffset = static_cast<std::uint32_t>(animalId) * 0x98u;
				animalId = readId(addressAt(NC2Logic::CAnimalArray::mStaticVars, animalOffset + 0x88u));
			}

			if (animalId == -1)
			{
				incrementCoordinate(currentSector.x);

				if (currentSector.x <= lastSector.x)
					continue;

				incrementCoordinate(currentSector.y);
				currentSector.x = firstSector.x;

				if (currentSector.y > lastSector.y)
					return -1;

				continue;
			}

			const std::uint32_t animalOffset = static_cast<std::uint32_t>(animalId) * 0x98u;
			const std::byte* animal = addressAt(NC2Logic::CAnimalArray::mStaticVars, animalOffset);

			if ((flags & 2u) != 0)
			{
				const std::uint32_t animalType = readWord(addressAt(animal, 0x14));
				const std::uint32_t typeFlagOffset = ((animalType - NRemaster::GetFirstAnimalTribe()) << 6) + 8u;

				std::uint8_t typeFlag;
				std::memcpy(&typeFlag, addressAt(gStaticAnimalTypeData, typeFlagOffset), sizeof(typeFlag));

				if (typeFlag == 0)
					continue;
			}

			const void* position = Animal_GetMapPositionRef(animal);

			std::int16_t animalX;
			std::int16_t animalY;
			std::memcpy(&animalX, position, sizeof(animalX));
			std::memcpy(&animalY, addressAt(position, 2), sizeof(animalY));

			const std::uint16_t centerXBits = static_cast<std::uint16_t>(packedCenter);
			const std::uint16_t centerYBits = static_cast<std::uint16_t>(packedCenter >> 16);

			std::int16_t centerX;
			std::int16_t centerY;
			std::memcpy(&centerX, &centerXBits, sizeof(centerX));
			std::memcpy(&centerY, &centerYBits, sizeof(centerY));

			const std::uint32_t distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(animalX, animalY, centerX, centerY));

			if (distance <= static_cast<std::uint32_t>(radius))
				return animalId;
		}
	}
}
