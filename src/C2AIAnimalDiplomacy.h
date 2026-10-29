#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Windows quantities, per-species request bytes, scout prefix and target remain separate.
	struct SAIAnimalCountStorage
	{
		std::int32_t quantity;
		std::uint32_t tribeType;
		std::uint8_t requested;
		std::byte unresolved09[3];
	};

	struct SAIAnimalCountsStorage
	{
		std::int32_t count;
		SAIAnimalCountStorage entries[2];
	};

	struct SAIAnimalCatcherStorage
	{
		std::int32_t humanId;
		std::uint32_t humanRevision;
		std::uint8_t humanRequested;
		std::byte unresolved09[3];
		SAIAnimalCountsStorage counts;
		std::uint32_t target[3];
		std::int32_t militaryPointIndex;
	};

	struct SAIDiplomacyStorage
	{
		std::uint32_t playerCursor, intrusionCounts[20];
	};

	bool AIAnimalTypeCounts_IsRequested(const void* counts, std::uint32_t type);
	bool AIAnimalTypeCounts_IsKnownOrSatisfied(const void* counts, std::uint32_t type);
	bool AIAnimalCatcher_IsType10Available(const void* catcher);
	bool AIAnimalCatcher_IsType19Available(const void* catcher);
	void AIAnimalCatcher_Init(void* catcher);
	void AIAnimalCatcher_DoOneLogicTick(void* catcher, std::uint32_t player, const void* center, void* military);
	void AIDiplomacy_DoOneLogicTick(void* diplomacy, std::uint32_t player, const void* villages);
}