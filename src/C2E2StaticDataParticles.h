#pragma once

#include <cstdint>
#include <cstddef>

namespace NC2E2
{
	struct SParticelFrames
	{
		std::uint32_t valency, count;
		std::uint32_t* frames;
	};

	struct SParticelGraphicsStorage
	{
		std::uint32_t count, maximumValency;
		SParticelFrames** entries;
	};

	struct SStaticParticelStorageEntry
	{
		char name[32];
		std::uint8_t hasSpawnParticle;
		std::byte unresolved21[3];
		std::uint32_t spawnParticleId;
		void* bobManager;
		std::uint32_t paletteId;
		std::uint8_t animationLoops, valencyIsDirection;
		std::byte unresolved32[2];
		SParticelGraphicsStorage graphics;
		std::uint32_t bobType, specialType;
	};

	inline constexpr std::size_t StaticParticelArrayCookieBytes = alignof(SStaticParticelStorageEntry);
	static_assert(sizeof(SParticelFrames) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(sizeof(SParticelGraphicsStorage) == (sizeof(void*) == 8 ? 16 : 12));
	static_assert(sizeof(SStaticParticelStorageEntry) == (sizeof(void*) == 8 ? 80 : 72));
	static_assert(offsetof(SStaticParticelStorageEntry, graphics) == (sizeof(void*) == 8 ? 56 : 52));
	static_assert(offsetof(SStaticParticelStorageEntry, bobManager) == 40);
	void* __fastcall E2StaticDataParticelManager_Construct(void* manager, void* unusedEdx);
	void* __fastcall E2StaticDataParticelEffectManager_Construct(void* manager, void* unusedEdx);
	std::int32_t E2StaticDataParticelManager_GetParticelId(const char* name);
	void* ParticelGraphics_GetValency(const void* list, std::uint32_t valency);
}