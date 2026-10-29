#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <cstddef>
#include <cstdint>

namespace NDSManager
{
	struct SWaveContextStorage
	{
		WAVEFORMATEX format;
		HMMIO file;
		MMCKINFO dataChunk;
		MMCKINFO parentChunk;
		std::uint32_t unclassified40;
		std::uint32_t memoryMode;
		std::uint8_t* sampleStart;
		std::uint8_t* sampleCursor;
		std::uint32_t sampleBytes;
	};

	struct SSoundStorage
	{
		void* buffer;
		std::uint32_t sampleBytes;
		SWaveContextStorage* waveContext;
	};

	static_assert(sizeof(WAVEFORMATEX) == 18);
	static_assert(sizeof(MMCKINFO) == 20);
	static_assert(sizeof(SWaveContextStorage) == (sizeof(void*) == 8 ? 104 : 84));
	static_assert(offsetof(SWaveContextStorage, file) == (sizeof(void*) == 8 ? 24 : 20));
	static_assert(offsetof(SWaveContextStorage, dataChunk) == (sizeof(void*) == 8 ? 32 : 24));
	static_assert(offsetof(SWaveContextStorage, parentChunk) == (sizeof(void*) == 8 ? 52 : 44));
	static_assert(offsetof(SWaveContextStorage, sampleStart) == (sizeof(void*) == 8 ? 80 : 72));
	static_assert(offsetof(SWaveContextStorage, sampleCursor) == (sizeof(void*) == 8 ? 88 : 76));
	static_assert(sizeof(SSoundStorage) == (sizeof(void*) == 8 ? 24 : 12));
}