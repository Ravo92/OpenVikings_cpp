#pragma once
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

	bool __fastcall AIManager_IO_InitFromScript(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	bool AIManager_Tool_IsAIEnabled(const void* manager, std::int32_t player);
	void AIManager_System_DoOneGameTick(void* manager);
	void AIManager_DisableAI(void* manager, std::int32_t player);
}