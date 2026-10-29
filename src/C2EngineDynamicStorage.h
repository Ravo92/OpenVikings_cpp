#pragma once

#include "XPalette.h"
#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	// Original Windows atom prefix: ten scalar DWORDs followed by five borrowed pointers.
	struct SDisplayAtomStorage
	{
		std::uint32_t bodyBobId, headBobId, animationTribe, animationJob, animationAction, animationStartCount;
		std::uint32_t walkTribe, walkJob, walkGood, walkSpeed;
		void* bodyBobManager; void* headBobManager; void* shadowBobManager;
		void* animation; void* walk;
	};
	struct SDynamicHumanStorage
	{
		SDisplayAtomStorage atom;
		std::uint32_t bodyVariant, headVariant;
		alignas(NXBasics::CPalette) std::byte bodyPalette[sizeof(NXBasics::CPalette)];
		alignas(NXBasics::CPalette) std::byte headPalette[sizeof(NXBasics::CPalette)];
		char name[100];
		std::uint32_t hasUserMessage;
		std::uint8_t selected, highlighted;
		std::byte unresolved8C6[2];
	};
	struct SDynamicAnimalVehicleStorage
	{
		SDisplayAtomStorage atom;
		std::uint32_t paletteId;
		std::uint8_t selected, highlighted;
		std::byte unresolved42[2];
	};
	inline constexpr std::size_t DynamicHumanArrayCookieBytes = alignof(SDynamicHumanStorage);
	static_assert(sizeof(SDisplayAtomStorage) == (sizeof(void*) == 8 ? 80 : 60));
	static_assert(offsetof(SDisplayAtomStorage, bodyBobManager) == 40);
	static_assert(offsetof(SDisplayAtomStorage, animation) == (sizeof(void*) == 8 ? 64 : 52));
	static_assert(sizeof(SDynamicHumanStorage) == (sizeof(void*) == 8 ? 2296 : 2248));
	static_assert(offsetof(SDynamicHumanStorage, bodyPalette) == (sizeof(void*) == 8 ? 88 : 68));
	static_assert(offsetof(SDynamicHumanStorage, headPalette) - offsetof(SDynamicHumanStorage, bodyPalette) == sizeof(NXBasics::CPalette));
	static_assert(offsetof(SDynamicHumanStorage, name) == (sizeof(void*) == 8 ? 2184 : 2140));
	static_assert(offsetof(SDynamicHumanStorage, hasUserMessage) == (sizeof(void*) == 8 ? 2284 : 2240));
	static_assert(sizeof(SDynamicAnimalVehicleStorage) == (sizeof(void*) == 8 ? 88 : 68));
}
