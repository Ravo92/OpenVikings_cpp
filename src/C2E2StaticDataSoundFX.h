#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	// Physical Windows wave slots inferred from the fixed allocation and all observed indexing.
	// Remaster parsing validates these physical capacities before writing records.
	struct SStaticSoundWaveStorage
	{
		std::uint32_t waveId, volume;
	};
	struct SStaticSoundFxStorage
	{
		char name[40]; std::uint32_t waveCount; SStaticSoundWaveStorage waves[130];
	};
	// The first wave is also used for pattern loops; landscape selection uses all weighted waves.
	struct SStaticAmbientWaveStorage
	{
		char filename[120]; std::uint32_t waveId, volume, selectionWeight, chanceMultiplier;
	};
	struct SStaticAmbientSoundStorage
	{
		char name[40]; std::uint32_t totalSelectionWeight, soundCount; SStaticAmbientWaveStorage sounds[40];
	};
	static_assert(sizeof(SStaticSoundFxStorage) == 0x43C);
	static_assert(sizeof(SStaticAmbientWaveStorage) == 0x88);
	static_assert(sizeof(SStaticAmbientSoundStorage) == 0x1570);

	struct SJingleSoundData
	{
		std::uint32_t event;
		char fileName[120];
		std::uint32_t volume;
	};

	static_assert(sizeof(SJingleSoundData) == 128);
	static_assert(offsetof(SJingleSoundData, fileName) == 0x04);
	static_assert(offsetof(SJingleSoundData, volume) == 0x7C);

	class CE2StaticDataSoundFXManager
	{
	public:
		// Original global: DAT_00568a28.
		inline static CE2StaticDataSoundFXManager* sTheObjectPtr = nullptr;

		CE2StaticDataSoundFXManager();
		~CE2StaticDataSoundFXManager();
		CE2StaticDataSoundFXManager(const CE2StaticDataSoundFXManager&) = delete;
		CE2StaticDataSoundFXManager& operator=(const CE2StaticDataSoundFXManager&) = delete;

		std::uint32_t GetStaticSoundFxIdByNamePtr(const char* name);

		std::uint32_t staticSoundCount;
		void* staticSounds;
		std::uint32_t staticSoundIdsByType[120];        // +0x008; 0xFFFFFFFF means unassigned.
		std::uint32_t jingleCount;
		SJingleSoundData* jingles;
		std::uint32_t ambientSoundCount;
		void* ambientSounds;
		std::uint32_t patternGroupAmbientIds[100];
		std::uint32_t landscapeGroupAmbientIds[100];

	private:
		static std::uint32_t l_GetSoundCommand(const char* name);
		void l_LoadSoundFxFromIniFile(const char* fileName);
		static void l_Ambient_AddSound(void* ambientRecord, const void* soundData);
	};

	static_assert(sizeof(CE2StaticDataSoundFXManager) == (sizeof(void*) == 8 ? 1328 : 1304));
	static_assert(offsetof(CE2StaticDataSoundFXManager, staticSounds) == (sizeof(void*) == 8 ? 8 : 0x04));
	static_assert(offsetof(CE2StaticDataSoundFXManager, staticSoundIdsByType) == (sizeof(void*) == 8 ? 16 : 0x08));
	static_assert(offsetof(CE2StaticDataSoundFXManager, jingleCount) == (sizeof(void*) == 8 ? 496 : 0x1E8));
	static_assert(offsetof(CE2StaticDataSoundFXManager, jingles) == (sizeof(void*) == 8 ? 504 : 0x1EC));
	static_assert(offsetof(CE2StaticDataSoundFXManager, ambientSoundCount) == (sizeof(void*) == 8 ? 512 : 0x1F0));
	static_assert(offsetof(CE2StaticDataSoundFXManager, ambientSounds) == (sizeof(void*) == 8 ? 520 : 0x1F4));
	static_assert(offsetof(CE2StaticDataSoundFXManager, patternGroupAmbientIds) == (sizeof(void*) == 8 ? 528 : 0x1F8));
	static_assert(offsetof(CE2StaticDataSoundFXManager, landscapeGroupAmbientIds) == (sizeof(void*) == 8 ? 928 : 0x388));
}
