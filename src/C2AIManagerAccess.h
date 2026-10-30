#pragma once
#include "C2AIGuideAccess.h"
#include "C2AISinglePlayerAccess.h"
#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
	// Complete Windows CAIManager owner; each slot owns its two player handlers.
	// Only the shared enable prefix is classified; the remaining handler state is separate.
	struct SAIHandlerEnablePrefixStorage
	{
		std::uint32_t playerId;
		std::uint8_t enabled;
		std::byte unclassified05[3];
	};

	struct SAIManagerStorage
	{
		void* singlePlayerHandlers[20];
		void* highLevelHandlers[20];
	};

	// The complete single-player state is saved as fixed Windows bytes; it contains IDs rather than runtime pointers.
	struct SAISinglePlayerStorage
	{
		SAISinglePlayerPrefixStorage prefix;
		std::byte fixedWindowsState[0xB068 - sizeof(SAISinglePlayerPrefixStorage)];
	};

	struct SAIHighLevelStorage
	{
		std::byte fixedWindowsState[43544];
		SAIGuideManagerStorage guideManager;
	};

	static_assert(sizeof(SAISinglePlayerStorage) == 0xB068);
	static_assert(sizeof(SAIManagerStorage) == 40 * sizeof(void*));
	static_assert(offsetof(SAIManagerStorage, highLevelHandlers) == 20 * sizeof(void*));
	static_assert(offsetof(SAIHighLevelStorage, guideManager) == 43544);
	static_assert(sizeof(SAIHighLevelStorage) == (sizeof(void*) == 8 ? 43584 : 43580));

	bool __fastcall AIManager_IO_InitFromScript(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	bool AIManager_Tool_IsAIEnabled(const void* manager, std::int32_t player);
	void AIManager_System_DoOneGameTick(void* manager);
	void AIManager_DisableAI(void* manager, std::int32_t player);
}