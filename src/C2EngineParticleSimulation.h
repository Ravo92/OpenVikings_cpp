#pragma once

#include <cstdint>
#include <cstddef>

namespace NC2E2
{
	struct SParticelGeneratorManagerStorage
	{
		std::uint32_t count;
		std::byte* entries;
	};
	struct SParticelDrawRowStorage
	{
		std::int32_t row;
		std::uint32_t count, indices[100];
	};
	struct SParticelManagerStorage
	{
		SParticelGeneratorManagerStorage* generatorManager;
		void* wind;
		std::uint32_t drawRowCount;
		SParticelDrawRowStorage drawRows[100];
		std::byte unresolved9F6C[4];
		std::byte particles[10000 * 304];
		std::uint32_t lastParticleIndex;
		std::byte unresolved2F0274[4];
		std::byte generators[1000 * 32];
		std::uint32_t lastGeneratorIndex;
		std::byte unresolved2F7F7C[4];
	};
	static_assert(sizeof(SParticelGeneratorManagerStorage) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(SParticelDrawRowStorage) == 408);
	static_assert(sizeof(SParticelManagerStorage) == (sizeof(void*) == 8 ? 3112840 : 3112832));
	static_assert(offsetof(SParticelManagerStorage, particles) == (sizeof(void*) == 8 ? 40824 : 40816));
	static_assert(offsetof(SParticelManagerStorage, generators) - offsetof(SParticelManagerStorage, particles) == 3040008);
	static_assert(sizeof(SParticelManagerStorage::generators) + sizeof(SParticelManagerStorage::lastGeneratorIndex) == 0x7D04);
	void __fastcall E2ParticelManager_DoOneGameTick(void* manager, void* unusedEdx);
	void* E2ParticelManager_GetCleanParticel(void* manager);
	void E2ParticelManager_CreateFallingSmokeParticel(void* manager, const void* mapPoint, std::uint32_t height, std::uint32_t lifetime);
	void E2ParticelManager_CreateFallingSpawnParticel(void* manager, const void* source);
	void E2ParticelManager_CreateMunitionParticel(void* manager, const void* sourcePoint, std::uint32_t height, const void* targetPoint, const void* sourcePixel, const void* targetPixel, std::uint32_t munitionType, std::uint32_t duration);
	void E2ParticelManager_StartParticelEffect(void* manager, std::uint32_t effectId, const void* mapPoint, std::uint32_t player);
	void E2ParticelManager_StartParticelEffectIntoDirection(void* manager, std::uint32_t effectId, const void* sourcePoint, const void* targetPoint, std::uint32_t player);
	void E2ParticelManager_l_CreateEffectParticel(void* manager, std::uint32_t effectId, std::uint32_t mapPoint, std::uint32_t valency, std::uint32_t player);
	void E2ParticelManager_AddParticelGenerator(void* manager, std::uint32_t generatorId, const void* mapPoint, const void* pixelPoint, std::uint32_t chance, std::uint32_t identifier);
	void E2ParticelManager_EnableGenerator(void* manager, std::uint32_t identifier);
	void E2ParticelManager_DestroyGenerator(void* manager, std::uint32_t identifier);
	void E2ParticelManager_l_GeneratorTick(void* manager);
	void E2ParticelGenerator_GenerateParticel(void* generator, const void* mapPoint, const void* pixelPoint);
	void E2ParticelWind_ChangeWind(void* wind);
	void LogicParticelObject_SetParticelId(void* particle, std::uint32_t id);
	void LogicParticelObject_SetStartPosition(void* particle, const void* mapPoint, const void* pixelPoint, std::uint32_t height);
	void LogicParticelObject_SetTargetPosition(void* particle, const void* mapPoint, const void* pixelPoint, std::uint32_t height);
	void LogicParticelObject_CalculateMovingVector(void* particle);
	void LogicParticelObject_CalculateMovingVector(void* particle, std::uint32_t duration);
	void LogicParticelObject_CalculateParrableHeight(void* particle);
}
