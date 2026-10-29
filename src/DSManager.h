#pragma once

#include <cstddef>
#include <cstdint>

namespace NDSManager
{
	bool ls_WaveFile_Load(void* soundDevice, const char* fileName, void** sound, std::uint8_t** waveData);
	void SoundDevice_Destroy(void* soundDevice);
	std::int32_t SoundDevice_Initialize(void* soundDevice, void* window, std::uint32_t cooperativeLevel, std::uint32_t channels, std::uint32_t samplesPerSecond, std::uint32_t bitsPerSample);
	std::int32_t SoundDevice_SetPrimaryFormat(void* soundDevice, std::uint32_t channels, std::uint32_t samplesPerSecond, std::uint32_t bitsPerSample);
	std::int32_t Sound_CreateFromMemory(void* soundDevice, void** sound, std::uint8_t* samples, std::uint32_t sampleBytes, const void* waveFormat, std::uint32_t flags);
	void Sound_Destroy(void* sound);
	std::int32_t Sound_FillBuffer(void* sound, void* buffer);
	std::int32_t Sound_RestoreBuffer(void* buffer, std::uint32_t* restored);
	std::int32_t Sound_Play(void* sound, std::uint32_t priority, std::uint32_t flags);
	std::int32_t Sound_SetFrequency(void* sound, std::uint32_t frequency);
	std::int32_t Sound_SetVolumeAndPan(void* sound, std::int32_t volume, std::int32_t pan);
	std::int32_t Sound_Stop(void* sound);
	std::int32_t Sound_ResetPosition(void* sound);
	std::uint32_t Sound_IsPlaying(void* sound);
	void* Wave_Construct(void* waveContext);
	std::int32_t Wave_Reset(void* waveContext);
	std::int32_t Wave_Read(void* waveContext, void* destination, std::uint32_t requestedBytes, std::uint32_t* bytesRead);

	struct SPreloadWave
	{
		void Exit();
		void FreeData();

		std::uint8_t active;
		std::byte padding01[3];
		char* fileName;
		std::uint8_t* waveData;
		void* sound;
	};

	static_assert(sizeof(SPreloadWave) == (sizeof(void*) == 8 ? 32 : 16));
	static_assert(offsetof(SPreloadWave, fileName) == (sizeof(void*) == 8 ? 8 : 0x04));
	static_assert(offsetof(SPreloadWave, waveData) == (sizeof(void*) == 8 ? 16 : 0x08));
	static_assert(offsetof(SPreloadWave, sound) == (sizeof(void*) == 8 ? 24 : 0x0C));

	struct SFireAndForgetWave
	{
		void Exit();

		std::uint8_t active;
		std::byte padding01[3];
		std::uint32_t sequence;
		std::uint32_t identifier;
		void* sound;
		std::uint8_t* waveData;
	};

	static_assert(sizeof(SFireAndForgetWave) == (sizeof(void*) == 8 ? 32 : 20));
	static_assert(offsetof(SFireAndForgetWave, sequence) == 0x04);
	static_assert(offsetof(SFireAndForgetWave, identifier) == 0x08);
	static_assert(offsetof(SFireAndForgetWave, sound) == (sizeof(void*) == 8 ? 16 : 0x0C));
	static_assert(offsetof(SFireAndForgetWave, waveData) == (sizeof(void*) == 8 ? 24 : 0x10));

	struct SVolumeTransition
	{
		std::uint32_t identifier;
		std::int32_t targetVolume;
		double currentVolume;
		double changePerMillisecond;
		std::uint32_t lastUpdate;
		std::uint8_t active;
		std::byte padding1D[3];
	};

	static_assert(sizeof(SVolumeTransition) == 32);
	static_assert(offsetof(SVolumeTransition, targetVolume) == 0x04);
	static_assert(offsetof(SVolumeTransition, currentVolume) == 0x08);
	static_assert(offsetof(SVolumeTransition, changePerMillisecond) == 0x10);
	static_assert(offsetof(SVolumeTransition, lastUpdate) == 0x18);
	static_assert(offsetof(SVolumeTransition, active) == 0x1C);

	class CManager
	{
	  public:
		CManager(void* window, bool initialize);
		~CManager();
		CManager(const CManager&) = delete;
		CManager& operator=(const CManager&) = delete;

		inline static CManager* sTheObjectPtr = nullptr;

		void System_Update();
		bool Quality_Set(std::int32_t quality);
		void MainVolume_Set(std::int32_t volume);
		bool PreloadWave_Load(const char* fileName, std::uint32_t* identifier);
		bool PreloadWave_Play(std::uint32_t identifier, std::uint32_t volume, std::uint32_t repeatCount, std::uint32_t pan, bool transitionRequested, std::uint32_t durationMilliseconds);
		bool PreloadWave_IsRunning(std::uint32_t identifier);
		bool PreloadWave_ChangeVolume(std::uint32_t identifier, std::uint32_t volume, bool transitionRequested, std::uint32_t durationMilliseconds);
		bool PreloadWave_Stop(std::uint32_t identifier);
		bool PreloadWave_Unload(std::uint32_t identifier);
		bool FireAndForget_LoadAndPlay(const char* fileName, std::uint32_t volume, std::uint32_t identifier, std::uint32_t repeatCount, bool deferPlayback, bool useVolumeOverride, std::uint32_t volumeOverride);
		bool FireAndForget_IsRunning(std::uint32_t identifier);
		bool FireAndForget_StopAll();
		bool FireAndForget_Stop(std::uint32_t identifier);
		std::uint32_t l_FireAndForget_GetFreeSlot();
		static void Jingle_Play(std::uint32_t event);

	  private:
		bool l_Device_Init();
		void l_Device_Exit();
		static std::uint32_t l_Jingle_FindIndex(const void* staticSoundData, std::uint32_t event);

		void* mWindow;
		std::uint8_t mInitialized;
		std::byte padding0005[3];
		void* mSoundDevice;
		std::int32_t mQuality;
		std::uint32_t mMainVolume;
		std::uint32_t mFrequencyPercent;
		std::uint32_t mNextSequence;
		SPreloadWave mPreloadWaves[1000];
		SFireAndForgetWave mFireAndForgetWaves[40];
		std::byte unknown41BC[4];
		SVolumeTransition mVolumeTransitions[10];
	};

	static_assert(sizeof(CManager) == (sizeof(void*) == 8 ? 33648 : 17152));
}