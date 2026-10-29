#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	class CE2AnimalSoundManager
	{
	  public:
		// Original global: DAT_0056841c.
		inline static CE2AnimalSoundManager* sTheObjectPtr = nullptr;

		CE2AnimalSoundManager();
		~CE2AnimalSoundManager();

		void PlayAnimalSounds(void* display);

		std::uint32_t lastUpdate;
		std::uint32_t animalCounts[34];
	};

	static_assert(sizeof(CE2AnimalSoundManager) == 140);
	static_assert(offsetof(CE2AnimalSoundManager, animalCounts) == 4);
}