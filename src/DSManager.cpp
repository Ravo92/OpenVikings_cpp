#include "ApplicationGlobals.h"
#include "C2DataIds.h"
#include "C2E2StaticDataSoundFX.h"
#include "DMManager.h"
#include "DSManager.h"
#include "GameProfile.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XNumericConstants.h"
#include "XResultIdentifiers.h"
#include "XWindowsResultCodes.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dsound.h>
#include <windows.h>
#include <mmreg.h>
#include <msacm.h>

namespace NDSManager
{
	namespace
	{
		// Remaster-only state for the singleton mixer; native object and save layouts stay unchanged.
		struct SRemasterMixState
		{
			std::uint32_t preloadVolume[1000];
			std::uint32_t preloadPan[1000];
			std::uint32_t transientVolume[40];
			std::uint32_t transientMultiplier[40];
			bool transientOverride[40];
		};
		SRemasterMixState gRemasterMix{};

		std::uint32_t ClampRemasterVolume(std::uint32_t volume)
		{
			return volume > 100u ? 100u : volume;
		}

		void ApplyRemasterSoundMix(void* sound, std::uint32_t volume, std::uint32_t multiplier, std::uint32_t pan)
		{
			if (sound == nullptr) return;
			const auto effectiveVolume = ClampRemasterVolume(volume) * ClampRemasterVolume(multiplier) / 100u;
			const auto attenuation = effectiveVolume == 0 ? DSBVOLUME_MIN : (static_cast<std::int32_t>(effectiveVolume) - 100) * 20;
			const auto position = (static_cast<std::int32_t>(pan > 128u ? 128u : pan) - 64) * 156;
			Sound_SetVolumeAndPan(sound, attenuation, position);
		}
	}

	// Original function: FUN_0040471f
	CManager::CManager(void* window, bool initialize)
	{
		std::memset(this, 0, sizeof(*this));
		std::memset(&gRemasterMix, 0, sizeof(gRemasterMix));
		for (std::uint32_t index = 0; index < 1000u; ++index)
		{
			gRemasterMix.preloadVolume[index] = 100;
			gRemasterMix.preloadPan[index] = 64;
		}

		sTheObjectPtr = this;
		mWindow = window;
		mMainVolume = 100;
		mNextSequence = 1;
		mFrequencyPercent = 100;

		if (initialize)
			Quality_Set(1);
	}

	// Original function: FUN_00404762
	CManager::~CManager()
	{
		// Remaster: a failed quality/device change can leave owned preload names.
		for (SPreloadWave& entry : mPreloadWaves) entry.Exit();
		for (SFireAndForgetWave& slot : mFireAndForgetWaves) slot.Exit();
		l_Device_Exit();

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_00404797
	void CManager::System_Update()
	{
		static_assert(offsetof(CManager, mFireAndForgetWaves) == 0x3E9C);
		static_assert(offsetof(CManager, mVolumeTransitions) == 0x41C0);

		if (mInitialized == 0)
			return;

		for (SFireAndForgetWave& slot : mFireAndForgetWaves)
		{
			if (slot.active != 0)
			{
				if (slot.sound == nullptr || Sound_IsPlaying(slot.sound) == 0)
					slot.Exit();
			}
		}

		for (SVolumeTransition& transition : mVolumeTransitions)
		{
			if (transition.active == 0)
				continue;

			if (!PreloadWave_IsRunning(transition.identifier))
			{
				transition.active = 0;
				continue;
			}

			const auto now = ::timeGetTime();
			const auto elapsed = now - transition.lastUpdate;
			transition.lastUpdate = now;
			const auto target = static_cast<double>(transition.targetVolume);
			auto next = transition.currentVolume + static_cast<double>(elapsed) * transition.changePerMillisecond;
			const bool finished = transition.changePerMillisecond >= 0.0 ? next >= target : next <= target;
			if (finished) next = target;
			transition.currentVolume = next;
			const auto identifier = transition.identifier;
			const auto volume = next > 0.0 && next < 1.0 ? 1u : static_cast<std::uint32_t>(next);
			gRemasterMix.preloadVolume[identifier] = volume;
			ApplyRemasterSoundMix(mPreloadWaves[identifier].sound, volume, mMainVolume, gRemasterMix.preloadPan[identifier]);
			if (finished)
			{
				transition.active = 0;
				if (transition.targetVolume == 0) PreloadWave_Stop(identifier);
			}
		}
	}

	// Original function: FUN_004048d7
	bool CManager::Quality_Set(std::int32_t quality)
	{
		if (mInitialized != 0)
		{
			FireAndForget_StopAll();

			for (SPreloadWave& entry : mPreloadWaves)
			{
				if (entry.active != 0)
					entry.FreeData();
			}

			l_Device_Exit();
		}

		mQuality = quality;

		if (!l_Device_Init())
			return false;

		for (SPreloadWave& entry : mPreloadWaves)
		{
			if (entry.active == 0)
				continue;

			char resolvedFileName[260];

			if (!NXBasics::CFile::FileSystem_Tool_GetRealFilename(entry.fileName, resolvedFileName))
			{
				entry.Exit();
				continue;
			}

			if (!ls_WaveFile_Load(mSoundDevice, resolvedFileName, &entry.sound, &entry.waveData))
			{
				entry.Exit();
				continue;
			}

			if (entry.sound != nullptr)
			{
				const std::uint32_t attenuationBits = (mMainVolume - 100u) * 5u * 4u;
				Sound_SetVolumeAndPan(entry.sound, NXBasics::Runtime_TimeSigned32(attenuationBits), 0);
			}
		}

		return true;
	}

	// Remaster: decode the installed IMA-ADPCM effects through Windows ACM.
	static bool DecodeImaAdpcm(IMAADPCMWAVEFORMAT source, std::uint8_t* samples, std::uint32_t sampleBytes, std::uint32_t factFrames, WAVEFORMATEX& format, std::uint8_t*& decoded, std::uint32_t& decodedBytes)
	{
		const auto channels = static_cast<std::uint32_t>(source.wfx.nChannels);
		const auto block = static_cast<std::uint32_t>(source.wfx.nBlockAlign);
		const auto headerBytes = channels * 4u;
		if (source.wfx.wFormatTag != WAVE_FORMAT_DVI_ADPCM || channels == 0 || channels > 2 || source.wfx.wBitsPerSample != 4 || source.wfx.cbSize != 2 || source.wfx.nSamplesPerSec == 0 || block < headerBytes || block % headerBytes != 0 || sampleBytes == 0) return false;
		const auto framesPerBlock = (block - headerBytes) * 2u / channels + 1u;
		if (source.wSamplesPerBlock != framesPerBlock) return false;
		const auto remainder = sampleBytes % block;
		if (remainder != 0 && (remainder < headerBytes || (channels == 2 && remainder % 8u != 0))) return false;
		const auto frameCount = static_cast<std::uint64_t>(sampleBytes / block) * framesPerBlock + (remainder == 0 ? 0u : (remainder - headerBytes) * 2u / channels + 1u);
		const auto outputAlignment = channels * 2u;
		const auto wantedFrames = factFrames == 0 ? frameCount : factFrames;
		if (wantedFrames == 0 || wantedFrames > frameCount || wantedFrames > UINT32_MAX / outputAlignment || source.wfx.nSamplesPerSec > UINT32_MAX / outputAlignment) return false;
		const auto paddedSize64 = (static_cast<std::uint64_t>(sampleBytes / block) + (remainder == 0 ? 0u : 1u)) * block;
		if (paddedSize64 > UINT32_MAX) return false;
		const auto paddedSize = static_cast<std::uint32_t>(paddedSize64);
		for (std::uint32_t offset = 0; offset < sampleBytes; offset += block)
			for (std::uint32_t channel = 0; channel < channels; ++channel)
				if (samples[offset + channel * 4u + 2u] > 88u) return false;
		std::uint8_t* padded = nullptr;
		if (remainder != 0)
		{
			// ACM consumes but omits a short final block; decode a padded block and trim to fact.
			padded = static_cast<std::uint8_t*>(NXBasics::Runtime_AllocateWithNewHandler(paddedSize, 1));
			if (padded == nullptr) return false;
			std::memcpy(padded, samples, sampleBytes);
			std::memset(padded + sampleBytes, 0, paddedSize - sampleBytes);
		}
		WAVEFORMATEX destination{};
		destination.wFormatTag = WAVE_FORMAT_PCM;
		destination.nChannels = source.wfx.nChannels;
		destination.nSamplesPerSec = source.wfx.nSamplesPerSec;
		destination.nBlockAlign = static_cast<WORD>(outputAlignment);
		destination.wBitsPerSample = 16;
		destination.nAvgBytesPerSec = destination.nSamplesPerSec * outputAlignment;
		HACMSTREAM stream = nullptr;
		if (acmStreamOpen(&stream, nullptr, &source.wfx, &destination, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME) != MMSYSERR_NOERROR)
		{
			NXBasics::Runtime_Free(padded);
			return false;
		}
		DWORD capacity = 0;
		const auto wantedBytes = static_cast<std::uint32_t>(wantedFrames * outputAlignment);
		const bool sized = acmStreamSize(stream, paddedSize, &capacity, ACM_STREAMSIZEF_SOURCE) == MMSYSERR_NOERROR && capacity >= wantedBytes;
		auto* output = sized ? static_cast<std::uint8_t*>(NXBasics::Runtime_AllocateWithNewHandler(capacity, 1)) : nullptr;
		ACMSTREAMHEADER header{};
		header.cbStruct = sizeof(header);
		header.pbSrc = padded == nullptr ? samples : padded;
		header.cbSrcLength = paddedSize;
		header.pbDst = output;
		header.cbDstLength = capacity;
		bool converted = false;
		if (output != nullptr && acmStreamPrepareHeader(stream, &header, 0) == MMSYSERR_NOERROR)
		{
			converted = acmStreamConvert(stream, &header, ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_END) == MMSYSERR_NOERROR && header.cbSrcLengthUsed == paddedSize && header.cbDstLengthUsed >= wantedBytes && header.cbDstLengthUsed <= capacity && header.cbDstLengthUsed % outputAlignment == 0;
			if (acmStreamUnprepareHeader(stream, &header, 0) != MMSYSERR_NOERROR) converted = false;
		}
		if (acmStreamClose(stream, 0) != MMSYSERR_NOERROR) converted = false;
		NXBasics::Runtime_Free(padded);
		if (!converted) { NXBasics::Runtime_Free(output); return false; }
		format = destination;
		decoded = output;
		decodedBytes = wantedBytes;
		return true;
	}

	// Original function: FUN_0040499b
	bool ls_WaveFile_Load(void* soundDevice, const char* fileName, void** sound, std::uint8_t** waveData)
	{
		if (sound == nullptr || waveData == nullptr) return false;
		*sound = nullptr;
		*waveData = nullptr;
		if (!NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true)) return false;
		NXBasics::CFile file(fileName, true);
		const auto fileSize = file.GetSize();
		if (fileSize < 12u) return false;
		*waveData = static_cast<std::uint8_t*>(NXBasics::Runtime_AllocateWithNewHandler(fileSize, 1));
		if (*waveData == nullptr) return false;
		file.Read(*waveData, fileSize);
		if (file.GetPosition() != fileSize)
		{
			NXBasics::Runtime_Free(*waveData);
			*waveData = nullptr;
			return false;
		}
		const auto readTag = [] (const std::uint8_t* source) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, source, sizeof(value));
			return value;
		};
		// Remaster: walk bounded RIFF chunks, including optional chunks and odd-byte padding.
		const auto* data = *waveData;
		WAVEFORMATEX format{};
		IMAADPCMWAVEFORMAT compressedFormat{};
		std::uint8_t* samples = nullptr;
		std::uint32_t sampleBytes = 0;
		std::uint32_t factFrames = 0;
		bool factFound = false;
		bool formatFound = false, valid = true;
		const auto riffSize = readTag(data + 4);
		valid = valid && readTag(data) == NC2DataIds::WaveTag::Riff && readTag(data + 8) == NC2DataIds::WaveTag::Wave && riffSize >= 4u && riffSize <= fileSize - 8u;
		if (valid)
		{
			const auto end = riffSize + 8u;
			std::uint32_t offset = 12;
			while (offset < end)
			{
				if (end - offset < 8u) { valid = false; break; }
				const auto tag = readTag(data + offset);
				const auto size = readTag(data + offset + 4);
				offset += 8;
				if (size > end - offset) { valid = false; break; }
				if (tag == NC2DataIds::WaveTag::Format && !formatFound)
				{
					if (size < 16u) { valid = false; break; }
					std::memcpy(&format, data + offset, 16);
					if (format.wFormatTag == WAVE_FORMAT_DVI_ADPCM)
					{
						if (size < sizeof(compressedFormat)) { valid = false; break; }
						std::memcpy(&compressedFormat, data + offset, sizeof(compressedFormat));
					}
					formatFound = true;
				}
				if (tag == 0x74636166u && !factFound) // RIFF 'fact': decoded frames per channel.
				{
					if (size < 4u) { valid = false; break; }
					factFrames = readTag(data + offset);
					factFound = true;
				}
				if (tag == NC2DataIds::WaveTag::Data && samples == nullptr)
				{
					samples = *waveData + offset;
					sampleBytes = size;
				}
				offset += size;
				if ((size & 1u) != 0)
				{
					if (offset == end) { valid = false; break; }
					++offset;
				}
			}
		}
		if (valid && formatFound && format.wFormatTag == WAVE_FORMAT_DVI_ADPCM && samples != nullptr && sampleBytes != 0)
		{
			std::uint8_t* decoded = nullptr;
			std::uint32_t decodedBytes = 0;
			valid = (!factFound || factFrames != 0) && DecodeImaAdpcm(compressedFormat, samples, sampleBytes, factFrames, format, decoded, decodedBytes);
			if (valid)
			{
				NXBasics::Runtime_Free(*waveData);
				*waveData = decoded;
				samples = decoded;
				sampleBytes = decodedBytes;
			}
		}
		const auto alignment = static_cast<std::uint32_t>(format.nChannels) * (format.wBitsPerSample / 8u);
		const auto byteRate = static_cast<std::uint64_t>(alignment) * format.nSamplesPerSec;
		valid = valid && formatFound && format.wFormatTag == WAVE_FORMAT_PCM && format.nChannels != 0 && format.nSamplesPerSec != 0 && (format.wBitsPerSample == 8 || format.wBitsPerSample == 16 || format.wBitsPerSample == 24 || format.wBitsPerSample == 32) && alignment != 0 && alignment == format.nBlockAlign && byteRate == format.nAvgBytesPerSec && samples != nullptr && sampleBytes != 0;
		if (valid && sampleBytes % alignment == 0)
		{
			if (Sound_CreateFromMemory(soundDevice, sound, samples, sampleBytes, &format, 0xE0u) < 0) *sound = nullptr;
		}
		if (*sound != nullptr) return true;
		NXBasics::Runtime_Free(*waveData);
		*waveData = nullptr;
		return false;
	}

	// Original function: FUN_00404aac
	void CManager::MainVolume_Set(std::int32_t volume)
	{
		if (volume < 0)
			volume = 0;
		else if (volume > 100)
			volume = 100;

		mMainVolume = static_cast<std::uint32_t>(volume);

		if (mInitialized == 0)
			return;

		for (std::uint32_t identifier = 0; identifier < 1000u; ++identifier)
		{
			SPreloadWave& entry = mPreloadWaves[identifier];
			if (entry.active != 0) ApplyRemasterSoundMix(entry.sound, gRemasterMix.preloadVolume[identifier], mMainVolume, gRemasterMix.preloadPan[identifier]);
		}
		for (std::uint32_t index = 0; index < 40u; ++index)
		{
			SFireAndForgetWave& slot = mFireAndForgetWaves[index];
			if (slot.active == 0) continue;
			const auto multiplier = gRemasterMix.transientOverride[index] ? gRemasterMix.transientMultiplier[index] : mMainVolume;
			ApplyRemasterSoundMix(slot.sound, gRemasterMix.transientVolume[index], multiplier, 64);
		}
	}

	// Original function: FUN_00404b62
	// Descriptive reconstruction name; the original method name is unknown.
	bool CManager::l_Device_Init()
	{
		std::uint32_t channels = 1;
		std::uint32_t samplesPerSecond = 11025;
		std::uint32_t bitsPerSample = 8;

		if (mQuality == 1)
		{
			samplesPerSecond = 22050;
			bitsPerSample = 16;
		}
		else if (mQuality == 2)
		{
			channels = 2;
			samplesPerSecond = 44100;
			bitsPerSample = 16;
		}

		void* wrapper = NXBasics::Runtime_AllocateWithNewHandler(4u, 1);

		if (wrapper != nullptr)
		{
			// Inline the four-byte initialization from FUN_004dffdf.
			IDirectSound8* device = nullptr;
			std::memcpy(wrapper, &device, sizeof(device));
		}

		mSoundDevice = wrapper;
		if (wrapper == nullptr) return false;
		const std::int32_t result = SoundDevice_Initialize(wrapper, mWindow, 2u, channels, samplesPerSecond, bitsPerSample);

		if (result < 0)
		{
			l_Device_Exit();
			return false;
		}

		mInitialized = 1;
		return true;
	}

	// Original function: FUN_00404bdb
	// Descriptive reconstruction name; the original method name is unknown.
	void CManager::l_Device_Exit()
	{
		void* deviceToRelease = mSoundDevice;

		if (deviceToRelease != nullptr)
		{
			SoundDevice_Destroy(deviceToRelease);
			NXBasics::Runtime_Free(deviceToRelease);
		}

		mSoundDevice = nullptr;
		mInitialized = 0;
	}

	// Original function: FUN_00404bff
	bool CManager::PreloadWave_Load(const char* fileName, std::uint32_t* identifier)
	{
		*identifier = NXBasics::IntegerRange::UInt32Maximum;

		if (mInitialized == 0)
			return false;

		char resolvedFileName[260];

		if (!NXBasics::CFile::FileSystem_Tool_GetRealFilename(fileName, resolvedFileName))
			return false;

		std::uint32_t firstFree = 1000;
		for (std::uint32_t candidate = 0; candidate < 1000u; ++candidate)
		{
			const SPreloadWave& entry = mPreloadWaves[candidate];
			if (entry.active == 0)
			{
				if (firstFree == 1000u) firstFree = candidate;
				continue;
			}
			if (entry.fileName != nullptr && NXBasics::Runtime_CompareStringsIgnoreCase(fileName, entry.fileName) == 0)
			{
				*identifier = candidate;
				return true;
			}
		}
		const auto index = firstFree;

		if (index == 1000u)
			return false;

		SPreloadWave& entry = mPreloadWaves[index];
		std::memset(&entry, 0, sizeof(entry));
		gRemasterMix.preloadVolume[index] = 100;
		gRemasterMix.preloadPan[index] = 64;

		if (!ls_WaveFile_Load(mSoundDevice, resolvedFileName, &entry.sound, &entry.waveData))
		{
			entry.Exit();
			return false;
		}

		if (entry.sound != nullptr)
		{
			const std::uint32_t attenuationBits = (mMainVolume - 100u) * 5u * 4u;
			Sound_SetVolumeAndPan(entry.sound, NXBasics::Runtime_TimeSigned32(attenuationBits), 0);
		}

		entry.active = 1;

		// Inline FUN_00410b59 with its original runtime allocation semantics.
		char* duplicatedName = nullptr;

		if (fileName != nullptr)
		{
			const std::uint32_t nameBytes = static_cast<std::uint32_t>(std::strlen(fileName)) + 1u;
			duplicatedName = static_cast<char*>(NXBasics::Runtime_AllocateWithNewHandler(nameBytes, 1));

			if (duplicatedName == nullptr) { entry.Exit(); return false; }
			std::strcpy(duplicatedName, fileName);
		}

		entry.fileName = duplicatedName;
		*identifier = index;

		return true;
	}

	// Original function: FUN_00404ced
	bool CManager::PreloadWave_Play(std::uint32_t identifier, std::uint32_t volume, std::uint32_t repeatCount, std::uint32_t pan, bool transitionRequested, std::uint32_t durationMilliseconds)
	{
		if (mInitialized == 0 || identifier >= 1000u)
			return false;

		const std::uint32_t entryOffset = identifier << 4;
		auto* storage = reinterpret_cast<std::byte*>(this);
		auto* entry = reinterpret_cast<SPreloadWave*>(storage + entryOffset + 0x1C);

		if (entry->active == 0)
			return false;

		volume = ClampRemasterVolume(volume);
		const std::uint32_t volumeProduct = mMainVolume * volume;
		const std::uint32_t effectiveVolume = volumeProduct / 100u;

		if (effectiveVolume == 0)
			return true;

		if (entry->sound != nullptr && Sound_IsPlaying(entry->sound) != 0)
			return false;

		gRemasterMix.preloadVolume[identifier] = volume;
		gRemasterMix.preloadPan[identifier] = pan > 128u ? 128u : pan;
		for (SVolumeTransition& transition : mVolumeTransitions)
			if (transition.identifier == identifier) transition.active = 0;

		bool transitionCreated = false;

		if (transitionRequested && durationMilliseconds != 0)
		{
			std::uint32_t index = 0;

			while (index < 10u && mVolumeTransitions[index].active != 0)
				++index;

			if (index != 10u)
			{
				SVolumeTransition* transition = &mVolumeTransitions[index];
				const std::uint32_t now = ::timeGetTime();

				transition->currentVolume = 0.0;
				transition->lastUpdate = now;
				transition->identifier = identifier;
				transition->targetVolume = NXBasics::Runtime_TimeSigned32(volume);
				transition->active = 1;

				// Zero-duration requests bypass transition creation.
				transition->changePerMillisecond = static_cast<double>(volume) / static_cast<double>(durationMilliseconds);

				transitionCreated = true;
			}
		}

		if (transitionCreated) gRemasterMix.preloadVolume[identifier] = 0;
		ApplyRemasterSoundMix(entry->sound, gRemasterMix.preloadVolume[identifier], mMainVolume, gRemasterMix.preloadPan[identifier]);

		if (entry->sound != nullptr)
		{
			const std::uint32_t frequencyProduct = mFrequencyPercent * NRemaster::GetSoundPlaybackBaseRate();
			Sound_SetFrequency(entry->sound, frequencyProduct / 100u);
		}

		if (entry->sound != nullptr)
		{
			const std::uint32_t looping = NXBasics::Runtime_TimeSigned32(repeatCount) > 1 ? 1u : 0u;
			Sound_Play(entry->sound, 0u, looping);
		}

		return true;
	}

	// Original function: FUN_00404e2d
	bool CManager::PreloadWave_IsRunning(std::uint32_t identifier)
	{
		if (mInitialized == 0)
			return false;

		// Preserve the original index calculation without adding a range check.
		const std::uint32_t entryOffset = identifier << 4;
		const auto* storage = reinterpret_cast<const std::byte*>(this);

		std::uint8_t active;
		std::memcpy(&active, storage + entryOffset + 0x1C, sizeof(active));

		if (active == 0)
			return false;

		void* sound;
		std::memcpy(&sound, storage + entryOffset + 0x28, sizeof(sound));

		if (sound == nullptr)
			return false;

		return Sound_IsPlaying(sound) != 0;
	}

	// Original function: FUN_00404e5c; remaster fades start from the current logical gain and retain pan.
	bool CManager::PreloadWave_ChangeVolume(std::uint32_t identifier, std::uint32_t volume, bool transitionRequested, std::uint32_t durationMilliseconds)
	{
		if (identifier >= 1000u || !PreloadWave_IsRunning(identifier)) return false;
		volume = ClampRemasterVolume(volume);
		SVolumeTransition* selected = nullptr;
		if (transitionRequested && durationMilliseconds != 0)
		{
			for (SVolumeTransition& transition : mVolumeTransitions)
			{
				if (transition.active != 0 && transition.identifier == identifier)
				{
					selected = &transition; break;
				}
				if (selected == nullptr && transition.active == 0) selected = &transition;
			}
			if (selected != nullptr)
			{
				const auto current = selected->active != 0 ? selected->currentVolume : static_cast<double>(gRemasterMix.preloadVolume[identifier]);
				selected->identifier = identifier;
				selected->targetVolume = static_cast<std::int32_t>(volume);
				selected->currentVolume = current;
				selected->changePerMillisecond = (static_cast<double>(volume) - current) / static_cast<double>(durationMilliseconds);
				selected->lastUpdate = ::timeGetTime();
				selected->active = 1;
				return true;
			}
		}
		// Immediate changes and a full fade table apply the requested value directly.
		for (SVolumeTransition& transition : mVolumeTransitions)
			if (transition.identifier == identifier) transition.active = 0;
		gRemasterMix.preloadVolume[identifier] = volume;
		if (volume == 0) return PreloadWave_Stop(identifier);
		ApplyRemasterSoundMix(mPreloadWaves[identifier].sound, volume, mMainVolume, gRemasterMix.preloadPan[identifier]);
		return true;
	}

	// Original function: FUN_00404f5c
	bool CManager::PreloadWave_Stop(std::uint32_t identifier)
	{
		if (mInitialized == 0 || identifier >= 1000u)
			return false;

		for (SVolumeTransition& transition : mVolumeTransitions)
			if (transition.identifier == identifier) transition.active = 0;

		// Preserve the native storage layout after validating the identifier.
		const std::uint32_t entryOffset = identifier << 4;
		const auto* storage = reinterpret_cast<const std::byte*>(this);

		std::uint8_t active;
		std::memcpy(&active, storage + entryOffset + 0x1C, sizeof(active));

		if (active == 0)
			return false;

		void* sound;
		std::memcpy(&sound, storage + entryOffset + 0x28, sizeof(sound));

		if (sound != nullptr)
		{
			Sound_Stop(sound);
			Sound_ResetPosition(sound);
		}

		return true;
	}

	// Original function: FUN_00404f91
	bool CManager::PreloadWave_Unload(std::uint32_t identifier)
	{
		static_assert(offsetof(CManager, mPreloadWaves) == 0x1C);
		static_assert(sizeof(mPreloadWaves) == 16000);

		if (mInitialized == 0)
			return false;

		// Preserve the original index calculation without adding a range check.
		const std::uint32_t entryOffset = identifier << 4;
		auto* storage = reinterpret_cast<std::byte*>(this);
		auto* entry = reinterpret_cast<SPreloadWave*>(storage + entryOffset + 0x1C);

		if (entry->active != 0)
			entry->Exit();

		return true;
	}

	// Original function: FUN_00404fb7
	void SPreloadWave::Exit()
	{
		FreeData();

		char* nameToRelease = fileName;

		if (nameToRelease != nullptr)
		{
			NXBasics::Runtime_Free(nameToRelease);
			fileName = nullptr;
		}

		active = 0;
	}

	// Original function: FUN_00404fd6
	void SPreloadWave::FreeData()
	{
		void* soundToRelease = sound;

		if (soundToRelease != nullptr)
		{
			Sound_Destroy(soundToRelease);
			NXBasics::Runtime_Free(soundToRelease);
			sound = nullptr;
		}

		std::uint8_t* dataToRelease = waveData;

		if (dataToRelease != nullptr)
		{
			NXBasics::Runtime_Free(dataToRelease);
			waveData = nullptr;
		}
	}

	// Original function: FUN_00405008
	bool CManager::FireAndForget_LoadAndPlay(const char* fileName, std::uint32_t volume, std::uint32_t identifier, std::uint32_t repeatCount, bool deferPlayback, bool useVolumeOverride, std::uint32_t volumeOverride)
	{
		static_assert(offsetof(CManager, mInitialized) == 0x04);
		static_assert(offsetof(CManager, mSoundDevice) == 0x08);
		static_assert(offsetof(CManager, mMainVolume) == 0x10);
		static_assert(offsetof(CManager, mFrequencyPercent) == 0x14);
		static_assert(offsetof(CManager, mNextSequence) == 0x18);

		if (mInitialized == 0)
			return false;

		char resolvedFileName[260];

		if (!NXBasics::CFile::FileSystem_Tool_GetRealFilename(fileName, resolvedFileName))
			return false;

		volume = ClampRemasterVolume(volume);
		const std::uint32_t volumeMultiplier = useVolumeOverride ? ClampRemasterVolume(volumeOverride) : mMainVolume;
		const std::uint32_t volumeProduct = volume * volumeMultiplier;
		const std::uint32_t effectiveVolume = volumeProduct / 100u;

		if (effectiveVolume == 0)
			return true;

		const std::uint32_t slotIndex = l_FireAndForget_GetFreeSlot();
		SFireAndForgetWave& slot = mFireAndForgetWaves[slotIndex];

		if (!ls_WaveFile_Load(mSoundDevice, resolvedFileName, &slot.sound, &slot.waveData))
		{
			slot.Exit();
			return false;
		}

		gRemasterMix.transientVolume[slotIndex] = volume;
		gRemasterMix.transientMultiplier[slotIndex] = volumeMultiplier;
		gRemasterMix.transientOverride[slotIndex] = useVolumeOverride;
		ApplyRemasterSoundMix(slot.sound, volume, volumeMultiplier, 64);

		if (!deferPlayback)
		{
			if (slot.sound != nullptr)
			{
				const std::uint32_t frequencyProduct = mFrequencyPercent * NRemaster::GetSoundPlaybackBaseRate();
				const std::uint32_t frequency = frequencyProduct / 100u;

				Sound_SetFrequency(slot.sound, frequency);
			}

			if (slot.sound != nullptr)
			{
				// The original tests the repeat count with a signed comparison.
				const std::uint32_t looping = NXBasics::Runtime_TimeSigned32(repeatCount) > 1 ? 1u : 0u;

				Sound_Play(slot.sound, 0u, looping);
			}
		}

		slot.active = 1;
		slot.sequence = mNextSequence;
		slot.identifier = identifier;
		++mNextSequence;

		return true;
	}

	// Original function: FUN_00405119
	bool CManager::FireAndForget_IsRunning(std::uint32_t identifier)
	{
		for (const SFireAndForgetWave& slot : mFireAndForgetWaves)
		{
			if (slot.active != 0 && slot.identifier == identifier)
				return true;
		}

		return false;
	}

	// Original function: FUN_00405141
	bool CManager::FireAndForget_StopAll()
	{
		if (mInitialized == 0)
			return false;

		for (SFireAndForgetWave& slot : mFireAndForgetWaves)
		{
			if (slot.active != 0)
				slot.Exit();
		}

		return true;
	}

	// Original function: FUN_0040516c
	bool CManager::FireAndForget_Stop(std::uint32_t identifier)
	{
		if (mInitialized == 0)
			return false;

		for (SFireAndForgetWave& slot : mFireAndForgetWaves)
		{
			if (slot.active != 0 && slot.identifier == identifier)
				slot.Exit();
		}

		return true;
	}

	// Original function: FUN_004051a3
	std::uint32_t CManager::l_FireAndForget_GetFreeSlot()
	{
		static_assert(offsetof(CManager, mFireAndForgetWaves) == 0x3E9C);

		std::uint32_t oldestIndex = 0;
		std::uint32_t oldestAge = 0;
		std::uint32_t index = 0;

		while (index < 40u)
		{
			const SFireAndForgetWave& slot = mFireAndForgetWaves[index];

			if (slot.active == 0)
				break;

			// Remaster: unsigned age handles both the signed boundary and counter wrap.
			const auto age = mNextSequence - slot.sequence;
			if (age > oldestAge)
			{
				oldestAge = age;
				oldestIndex = index;
			}

			++index;
		}

		if (index == 40u)
		{
			mFireAndForgetWaves[oldestIndex].Exit();
			index = oldestIndex;
		}

		SFireAndForgetWave& slot = mFireAndForgetWaves[index];

		std::memset(&slot, 0, sizeof(slot));
		slot.identifier = NXBasics::IntegerRange::UInt32Maximum;

		return index;
	}

	// Original function: FUN_00405207
	void SFireAndForgetWave::Exit()
	{
		void* soundToRelease = sound;

		if (soundToRelease != nullptr)
		{
			Sound_Destroy(soundToRelease);
			NXBasics::Runtime_Free(soundToRelease);
			sound = nullptr;
		}

		std::uint8_t* dataToRelease = waveData;

		if (dataToRelease != nullptr)
		{
			NXBasics::Runtime_Free(dataToRelease);
			waveData = nullptr;
		}

		active = 0;
	}

	// Original function: FUN_0040523c; release the interface without freeing wrapper storage.
	// No corresponding Mac class name is established for this Windows-only wrapper.
	void SoundDevice_Destroy(void* soundDevice)
	{
		IDirectSound8* device;
		std::memcpy(&device, soundDevice, sizeof(device));
		if (device != nullptr)
		{
			device->Release();
			device = nullptr;
			std::memcpy(soundDevice, &device, sizeof(device));
		}
	}

	// Original function: FUN_00405250
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t SoundDevice_Initialize(void* soundDevice, void* window, std::uint32_t cooperativeLevel, std::uint32_t channels, std::uint32_t samplesPerSecond, std::uint32_t bitsPerSample)
	{
		auto* storage = static_cast<std::byte*>(soundDevice);

		IDirectSound8* device;
		std::memcpy(&device, storage, sizeof(device));

		if (device != nullptr)
		{
			device->Release();

			device = nullptr;
			std::memcpy(storage, &device, sizeof(device));
		}

		HRESULT result = ::DirectSoundCreate8(nullptr, &device, nullptr);
		std::memcpy(storage, &device, sizeof(device));

		if (result < 0)
			return result;

		result = device->SetCooperativeLevel(static_cast<HWND>(window), cooperativeLevel);

		if (result < 0)
			return result;

		const std::int32_t formatResult = SoundDevice_SetPrimaryFormat(soundDevice, channels, samplesPerSecond, bitsPerSample);

		if (formatResult < 0)
			return formatResult;

		return 0;
	}

	// Original function: FUN_004052a0
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t SoundDevice_SetPrimaryFormat(void* soundDevice, std::uint32_t channels, std::uint32_t samplesPerSecond, std::uint32_t bitsPerSample)
	{
		static_assert(sizeof(DSBUFFERDESC) == 36);
		static_assert(sizeof(WAVEFORMATEX) == 18);

		IDirectSound8* device;
		std::memcpy(&device, soundDevice, sizeof(device));

		if (device == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		IDirectSoundBuffer* primaryBuffer = nullptr;

		DSBUFFERDESC description;
		std::memset(&description, 0, sizeof(description));

		description.dwSize = 0x24;
		description.dwFlags = 1;
		description.dwBufferBytes = 0;
		description.lpwfxFormat = nullptr;

		const HRESULT createResult = device->CreateSoundBuffer(&description, &primaryBuffer, nullptr);

		if (createResult < 0)
			return createResult;

		WAVEFORMATEX format;
		std::memset(&format, 0, sizeof(format));

		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = static_cast<WORD>(channels);
		format.nSamplesPerSec = samplesPerSecond;
		format.wBitsPerSample = static_cast<WORD>(bitsPerSample);

		// Preserve the original low-word shift and truncation before byte-rate calculation.
		const std::uint32_t bytesPerSample = static_cast<std::uint16_t>(bitsPerSample) >> 3;
		format.nBlockAlign = static_cast<WORD>(bytesPerSample * channels);
		format.nAvgBytesPerSec = static_cast<std::uint32_t>(format.nBlockAlign) * samplesPerSecond;

		const HRESULT formatResult = primaryBuffer->SetFormat(&format);
		if (primaryBuffer != nullptr)
			primaryBuffer->Release();
		return formatResult;
	}

	// Original function: FUN_00405358
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_CreateFromMemory(void* soundDevice, void** sound, std::uint8_t* samples, std::uint32_t sampleBytes, const void* waveFormat, std::uint32_t flags)
	{
		static_assert(sizeof(DSBUFFERDESC) == 36);
		static_assert(offsetof(DSBUFFERDESC, guid3DAlgorithm) == 0x14);
		static_assert(sizeof(WAVEFORMATEX) == 18);

		auto* deviceStorage = static_cast<std::byte*>(soundDevice);

		IDirectSound* device;
		std::memcpy(&device, deviceStorage, sizeof(device));

		if (device == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		if (samples == nullptr || sound == nullptr)
			return NXBasics::Runtime_TimeSigned32(0x80070057u);

		auto* context = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(84u, 1));

		if (context == nullptr)
			return NXBasics::Runtime_TimeSigned32(0x8007000Eu);

		Wave_Construct(context);
		const std::uint32_t one = 1;

		std::memcpy(context, waveFormat, 18);
		std::memcpy(context + 0x48, &samples, sizeof(samples));
		std::memcpy(context + 0x4C, &samples, sizeof(samples));
		std::memcpy(context + 0x50, &sampleBytes, sizeof(sampleBytes));
		std::memcpy(context + 0x44, &one, sizeof(one));

		DSBUFFERDESC description;
		std::memset(&description, 0, sizeof(description));

		description.dwSize = 0x24;
		description.dwFlags = flags;
		description.dwBufferBytes = sampleBytes;
		description.lpwfxFormat = const_cast<WAVEFORMATEX*>(static_cast<const WAVEFORMATEX*>(waveFormat));
		// Original data: 004f3358; all 16 GUID bytes are zero.
		description.guid3DAlgorithm = GUID{ };

		IDirectSoundBuffer* buffer = nullptr;

		// Reload the device pointer immediately before the COM call.
		std::memcpy(&device, deviceStorage, sizeof(device));

		const HRESULT result = device->CreateSoundBuffer(&description, &buffer, nullptr);

		if (result < 0)
		{
			NXBasics::Runtime_Free(context);
			return result;
		}

		auto* wrapper = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(12u, 1));

		if (wrapper == nullptr)
		{
			buffer->Release();
			NXBasics::Runtime_Free(context);
			return NXBasics::Runtime_TimeSigned32(0x8007000Eu);
		}
		std::memcpy(wrapper, &buffer, sizeof(buffer));
		std::memcpy(wrapper + 0x04, &sampleBytes, sizeof(sampleBytes));
		std::memcpy(wrapper + 0x08, &context, sizeof(context));
		const auto fillResult = Sound_FillBuffer(wrapper, buffer);
		if (fillResult < 0)
		{
			Sound_Destroy(wrapper);
			NXBasics::Runtime_Free(wrapper);
			return fillResult;
		}
		buffer->SetCurrentPosition(0);
		*sound = wrapper;
		return 0;
	}

	// Original function: FUN_00405464
	// Descriptive reconstruction name; the original Windows class name is unknown.
	void Sound_Destroy(void* sound)
	{
		auto* storage = static_cast<std::byte*>(sound);

		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer != nullptr)
			buffer->Stop();

		// The original reloads the interface pointer before each operation.
		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer != nullptr)
			buffer->SetCurrentPosition(0);

		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer != nullptr)
		{
			buffer->Release();

			buffer = nullptr;
			std::memcpy(storage, &buffer, sizeof(buffer));
		}

		void* waveContext;
		std::memcpy(&waveContext, storage + 0x08, sizeof(waveContext));

		if (waveContext != nullptr)
		{
			auto* contextStorage = static_cast<std::byte*>(waveContext);

			HMMIO file;
			std::memcpy(&file, contextStorage + 0x14, sizeof(file));

			// The original calls mmioClose even when the handle is null.
			::mmioClose(file, 0);

			file = nullptr;
			std::memcpy(contextStorage + 0x14, &file, sizeof(file));

			NXBasics::Runtime_Free(waveContext);

			waveContext = nullptr;
			std::memcpy(storage + 0x08, &waveContext, sizeof(waveContext));
		}
	}

	// Original function: FUN_004054b4
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_FillBuffer(void* sound, void* buffer)
	{
		auto* soundStorage = static_cast<std::byte*>(sound);
		auto* directSoundBuffer = static_cast<IDirectSoundBuffer*>(buffer);

		void* destination = nullptr;
		DWORD lockedBytes = 0;
		std::uint32_t bytesRead = 0;

		if (directSoundBuffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		const std::int32_t restoreResult = Sound_RestoreBuffer(directSoundBuffer, nullptr);

		if (restoreResult < 0)
			return restoreResult;

		std::uint32_t bufferBytes;
		std::memcpy(&bufferBytes, soundStorage + 0x04, sizeof(bufferBytes));

		const HRESULT lockResult = directSoundBuffer->Lock(0, bufferBytes, &destination, &lockedBytes, nullptr, nullptr, 0);

		if (lockResult < 0)
			return lockResult;

		void* waveContext;
		std::memcpy(&waveContext, soundStorage + 0x08, sizeof(waveContext));

		const auto resetResult = Wave_Reset(waveContext);
		if (resetResult < 0)
		{
			directSoundBuffer->Unlock(destination, lockedBytes, nullptr, 0);
			return resetResult;
		}

		std::memcpy(&waveContext, soundStorage + 0x08, sizeof(waveContext));

		const std::int32_t readResult = Wave_Read(waveContext, destination, lockedBytes, &bytesRead);

		if (readResult < 0)
		{
			directSoundBuffer->Unlock(destination, lockedBytes, nullptr, 0);
			return readResult;
		}

		if (bytesRead == 0 || bytesRead < lockedBytes)
		{
			std::memcpy(&waveContext, soundStorage + 0x08, sizeof(waveContext));

			std::uint16_t bitsPerSample;
			std::memcpy(&bitsPerSample, static_cast<const std::byte*>(waveContext) + 0x0E, sizeof(bitsPerSample));

			const int silence = bitsPerSample == 8u ? 0x80 : 0;
			void* fillDestination = destination;

			if (bytesRead != 0)
				fillDestination = static_cast<std::byte*>(destination) + bytesRead;

			std::memset(fillDestination, silence, lockedBytes - bytesRead);
		}

		return directSoundBuffer->Unlock(destination, lockedBytes, nullptr, 0);
	}

	// Original function: FUN_00405591
	// The original does not use its this pointer.
	std::int32_t Sound_RestoreBuffer(void* buffer, std::uint32_t* restored)
	{
		auto* directSoundBuffer = static_cast<IDirectSoundBuffer*>(buffer);

		if (directSoundBuffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		if (restored != nullptr)
			*restored = 0;

		DWORD status;
		const HRESULT statusResult = directSoundBuffer->GetStatus(&status);

		if (statusResult < 0)
			return statusResult;

		if ((status & NXBasics::BitMask::Bit1) == 0)
			return 1;

		// Remaster: one attempt per request. A device error must not block the game thread.
		const HRESULT restoreResult = directSoundBuffer->Restore();
		if (FAILED(restoreResult)) return restoreResult;

		if (restored != nullptr)
			*restored = 1;

		return 0;
	}

	// Original function: FUN_004055f7
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_Play(void* sound, std::uint32_t priority, std::uint32_t flags)
	{
		auto* storage = static_cast<std::byte*>(sound);

		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		std::uint32_t restored;
		const std::int32_t restoreResult = Sound_RestoreBuffer(buffer, &restored);

		if (restoreResult < 0)
			return restoreResult;

		if (restored != 0)
		{
			const std::int32_t fillResult = Sound_FillBuffer(sound, buffer);

			if (fillResult < 0)
				return fillResult;

			IDirectSoundBuffer* currentBuffer;
			std::memcpy(&currentBuffer, storage, sizeof(currentBuffer));

			if (currentBuffer != nullptr)
				currentBuffer->SetCurrentPosition(0);
		}

		// The final operation uses the buffer saved on entry.
		return buffer->Play(0, priority, flags);
	}

	// Original function: FUN_00405650
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_SetFrequency(void* sound, std::uint32_t frequency)
	{
		auto* storage = static_cast<std::byte*>(sound);

		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		std::uint32_t restored;
		const std::int32_t restoreResult = Sound_RestoreBuffer(buffer, &restored);

		if (restoreResult < 0)
			return restoreResult;

		if (restored != 0)
		{
			const std::int32_t fillResult = Sound_FillBuffer(sound, buffer);

			if (fillResult < 0)
				return fillResult;

			IDirectSoundBuffer* currentBuffer;
			std::memcpy(&currentBuffer, storage, sizeof(currentBuffer));

			if (currentBuffer != nullptr)
				currentBuffer->SetCurrentPosition(0);
		}

		// The final operation uses the buffer saved on entry.
		return buffer->SetFrequency(frequency);
	}

	// Original function: FUN_004056b1
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_SetVolumeAndPan(void* sound, std::int32_t volume, std::int32_t pan)
	{
		auto* storage = static_cast<std::byte*>(sound);

		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, storage, sizeof(buffer));

		if (buffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		std::uint32_t restored;
		const std::int32_t restoreResult = Sound_RestoreBuffer(buffer, &restored);

		if (restoreResult < 0)
			return restoreResult;

		if (restored != 0)
		{
			const std::int32_t fillResult = Sound_FillBuffer(sound, buffer);

			if (fillResult < 0)
				return fillResult;

			IDirectSoundBuffer* currentBuffer;
			std::memcpy(&currentBuffer, storage, sizeof(currentBuffer));

			if (currentBuffer != nullptr)
				currentBuffer->SetCurrentPosition(0);
		}

		// Both operations use the saved buffer; their results are ignored.
		buffer->SetVolume(volume);
		buffer->SetPan(pan);

		return 0;
	}

	// Original function: FUN_00405710
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_Stop(void* sound)
	{
		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, sound, sizeof(buffer));

		if (buffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		return buffer->Stop();
	}

	// Original function: FUN_00405723
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Sound_ResetPosition(void* sound)
	{
		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, sound, sizeof(buffer));

		if (buffer == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		return buffer->SetCurrentPosition(0);
	}

	// Original function: FUN_00405738
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::uint32_t Sound_IsPlaying(void* sound)
	{
		IDirectSoundBuffer* buffer;
		std::memcpy(&buffer, sound, sizeof(buffer));

		if (buffer == nullptr)
			return 0;

		DWORD status = 0;

		// The original ignores the HRESULT and evaluates the status output.
		buffer->GetStatus(&status);

		return status & 1u;
	}

	// Original function: FUN_0040575a; initialize only the native constructor fields.
	// No corresponding Mac class name is established for this Windows-only context.
	void* Wave_Construct(void* waveContext)
	{
		auto* storage = static_cast<std::byte*>(waveContext);
		std::memset(storage, 0, 18);
		const std::uint32_t zero = 0;
		std::memcpy(storage + 0x14, &zero, sizeof(zero));
		std::memcpy(storage + 0x40, &zero, sizeof(zero));
		std::memcpy(storage + 0x44, &zero, sizeof(zero));
		return waveContext;
	}

	// Original function: FUN_0040577a
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Wave_Reset(void* waveContext)
	{
		static_assert(sizeof(MMCKINFO) == 20);

		auto* storage = static_cast<std::byte*>(waveContext);

		std::uint32_t memoryMode;
		std::memcpy(&memoryMode, storage + 0x44, sizeof(memoryMode));

		if (memoryMode != 0)
		{
			std::uint32_t startAddress;
			std::memcpy(&startAddress, storage + 0x48, sizeof(startAddress));
			std::memcpy(storage + 0x4C, &startAddress, sizeof(startAddress));
			return 0;
		}

		HMMIO file;
		std::memcpy(&file, storage + 0x14, sizeof(file));

		if (file == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		std::uint32_t parentDataOffset;
		std::memcpy(&parentDataOffset, storage + 0x38, sizeof(parentDataOffset));

		const std::int32_t seekPosition = NXBasics::Runtime_TimeSigned32(parentDataOffset + 4u);

		if (::mmioSeek(file, seekPosition, SEEK_SET) == -1)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::Failure);

		const std::uint32_t dataTag = NC2DataIds::WaveTag::Data;
		std::memcpy(storage + 0x18, &dataTag, sizeof(dataTag));

		MMCKINFO chunk;
		MMCKINFO parent;
		std::memcpy(&chunk, storage + 0x18, sizeof(chunk));
		std::memcpy(&parent, storage + 0x2C, sizeof(parent));
		std::memcpy(&file, storage + 0x14, sizeof(file));

		const MMRESULT result = ::mmioDescend(file, &chunk, &parent, MMIO_FINDCHUNK);

		// Preserve output changes even when mmioDescend reports failure.
		std::memcpy(storage + 0x18, &chunk, sizeof(chunk));

		if (result != 0)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::Failure);

		return 0;
	}

	// Original function: FUN_004057d4
	// Descriptive reconstruction name; the original Windows class name is unknown.
	std::int32_t Wave_Read(void* waveContext, void* destination, std::uint32_t requestedBytes, std::uint32_t* bytesRead)
	{
		static_assert(sizeof(void*) == 4);
		static_assert(sizeof(MMIOINFO) == 72);
		static_assert(offsetof(MMIOINFO, pchNext) == 0x1C);
		static_assert(offsetof(MMIOINFO, pchEndRead) == 0x20);

		auto* storage = static_cast<std::byte*>(waveContext);

		std::uint32_t memoryMode;
		std::memcpy(&memoryMode, storage + 0x44, sizeof(memoryMode));

		if (memoryMode != 0)
		{
			std::uint32_t cursorAddress;
			std::memcpy(&cursorAddress, storage + 0x4C, sizeof(cursorAddress));

			if (cursorAddress == 0)
				return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

			if (bytesRead != nullptr)
				*bytesRead = 0;

			std::uint32_t totalBytes;
			std::uint32_t startAddress;
			std::memcpy(&totalBytes, storage + 0x50, sizeof(totalBytes));
			std::memcpy(&startAddress, storage + 0x48, sizeof(startAddress));
			std::memcpy(&cursorAddress, storage + 0x4C, sizeof(cursorAddress));

			if (cursorAddress < startAddress || cursorAddress - startAddress > totalBytes || (destination == nullptr && requestedBytes != 0)) return NXBasics::Runtime_TimeSigned32(0x80070057u);
			const auto remaining = totalBytes - (cursorAddress - startAddress);
			const auto copiedBytes = requestedBytes < remaining ? requestedBytes : remaining;

			const void* source = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(cursorAddress));
			if (copiedBytes != 0) std::memmove(destination, source, copiedBytes);

			if (bytesRead != nullptr)
				*bytesRead = copiedBytes;

			cursorAddress += copiedBytes;
			std::memcpy(storage + 0x4C, &cursorAddress, sizeof(cursorAddress));
			return 0;
		}

		HMMIO file;
		std::memcpy(&file, storage + 0x14, sizeof(file));

		if (file == nullptr)
			return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::NotInitialized);

		if (destination == nullptr || bytesRead == nullptr)
			return NXBasics::Runtime_TimeSigned32(0x80070057u);

		*bytesRead = 0;

		std::uint32_t remainingBytes;
		std::memcpy(&remainingBytes, storage + 0x1C, sizeof(remainingBytes));
		const auto wanted = requestedBytes < remainingBytes ? requestedBytes : remainingBytes;
		if (wanted > INT32_MAX) return NXBasics::Runtime_TimeSigned32(0x80070057u);
		if (wanted == 0) return 0;
		// Remaster: let MMIO own buffering and commit only the bytes actually read.
		const auto actual = ::mmioRead(file, static_cast<char*>(destination), static_cast<LONG>(wanted));
		if (actual < 0) return NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::Failure);
		*bytesRead = static_cast<std::uint32_t>(actual);
		remainingBytes -= *bytesRead;
		std::memcpy(storage + 0x1C, &remainingBytes, sizeof(remainingBytes));
		return *bytesRead == wanted ? 0 : NXBasics::Runtime_TimeSigned32(NXBasics::ComResult::Failure);
	}

	// Original function: FUN_00485fdd
	void CManager::Jingle_Play(std::uint32_t event)
	{
		std::int32_t programState;
		std::memcpy(&programState, NMasterControlProgram::CMasterControlProgram::sTheObjectPtr, sizeof(programState));

		if (programState == 7 || programState == 8 || programState == 9)
			return;

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("fx_jingles_off") != 0)
			return;

		const std::uint32_t jingleEvent = event + 0x15u;

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode") == 2)
		{
			if (gDirectMusicManager != nullptr)
				NDMManager::ProcessEvent(gDirectMusicManager, NXBasics::Runtime_TimeSigned32(jingleEvent), 0, 2);
		}

		if (sTheObjectPtr == nullptr || sTheObjectPtr->mInitialized == 0)
			return;

		const std::uint32_t index = l_Jingle_FindIndex(NC2E2::CE2StaticDataSoundFXManager::sTheObjectPtr, jingleEvent);
		if (index == NXBasics::IntegerRange::UInt32Maximum) return;
		const std::uint32_t identifier = index + 0x87E85u;

		if (sTheObjectPtr->FireAndForget_IsRunning(identifier))
			return;

		const std::uint32_t recordsAddress = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(NC2E2::CE2StaticDataSoundFXManager::sTheObjectPtr->jingles));

		const std::uint32_t recordAddress = recordsAddress + (index << 7);

		if (recordAddress == 0)
			return;

		const auto* record = reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(recordAddress));
		std::uint32_t volume;
		std::memcpy(&volume, record + 0x7C, sizeof(volume));

		sTheObjectPtr->FireAndForget_LoadAndPlay(reinterpret_cast<const char*>(record + 4), volume, identifier, 1, false, false, 100);
	}

	// Original function: FUN_0048e1e1
	std::uint32_t CManager::l_Jingle_FindIndex(const void* staticSoundData, std::uint32_t event)
	{
		if (staticSoundData == nullptr || sTheObjectPtr == nullptr || sTheObjectPtr->mInitialized == 0)
			return NXBasics::IntegerRange::UInt32Maximum;

		const auto* storage = static_cast<const std::byte*>(staticSoundData);
		std::uint32_t count;
		std::memcpy(&count, storage + 0x1E8, sizeof(count));

		if (count == 0)
			return NXBasics::IntegerRange::UInt32Maximum;

		std::uint32_t recordAddress;
		std::memcpy(&recordAddress, storage + 0x1EC, sizeof(recordAddress));
		if (recordAddress == 0) return NXBasics::IntegerRange::UInt32Maximum;

		for (std::uint32_t index = 0; index < count; ++index)
		{
			std::uint32_t recordEvent;
			std::memcpy(&recordEvent, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(recordAddress)), sizeof(recordEvent));

			if (recordEvent == event)
				return index;

			recordAddress += 0x80u;
		}

		return NXBasics::IntegerRange::UInt32Maximum;
	}
}
