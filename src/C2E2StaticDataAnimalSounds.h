#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	struct SStaticAnimalSoundData
	{
		std::int32_t logicTribeType;
		std::uint32_t minimumCount;
		std::uint32_t probabilityThreshold;
		std::uint32_t staticSoundId;
	};

	static_assert(sizeof(SStaticAnimalSoundData) == 16);
	static_assert(offsetof(SStaticAnimalSoundData, staticSoundId) == 0x0C);

	class CE2StaticDataAnimalSoundManager
	{
	  public:
		// Original global: DAT_00568570.
		inline static CE2StaticDataAnimalSoundManager* sTheObjectPtr = nullptr;

		CE2StaticDataAnimalSoundManager();
		~CE2StaticDataAnimalSoundManager();

		SStaticAnimalSoundData* GetStaticAnimalSoundDataPtrByTribeType(std::int32_t tribeType);

		std::uint32_t count;
		SStaticAnimalSoundData* entries;
	};

	static_assert(sizeof(CE2StaticDataAnimalSoundManager) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(CE2StaticDataAnimalSoundManager, entries) == (sizeof(void*) == 8 ? 8 : 4));
}