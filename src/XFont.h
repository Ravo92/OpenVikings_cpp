#pragma once

#include "XStorable.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	namespace FontStorageOffset
	{
		inline constexpr auto SerializedData = sizeof(void*);
		inline constexpr auto Height = sizeof(void*) + 4;
		inline constexpr auto CharacterSpacing = sizeof(void*) + 8;
		inline constexpr auto BobManager = sizeof(void*) == 8 ? 24 : 16;
		inline constexpr auto Palette = sizeof(void*) == 8 ? 32 : 20;
	}

	class CBobManager;
	class CPalette;

	class CFont : public CStorable
	{
	public:
		// Original function: FUN_004397ea
		CFont(void* file, std::uint32_t version);
		CFont(const CFont&) = delete;
		CFont& operator=(const CFont&) = delete;

		// Original function: FUN_00439690.
		virtual ~CFont();
		void* Delete(std::uint32_t flags);

		// Original function: LAB_0043983c
		std::uint32_t Storable_GetId() override
		{
			return 0x3F5;
		}

		// Original function: LAB_0040c775
		std::uint32_t Storable_GetVersion() override
		{
			return 0;
		}

		// Original function: FUN_0043981a
		void Storable_SaveData(void* file) override;

	private:
		void l_InitObject();

		std::byte mSerializedData[8];           // +0x04; field semantics unresolved.
		std::int32_t mCharacterSpacing;           // +0x0C
		CBobManager* mBobManager;                 // +0x10; owned.
		CPalette* mPalette;                       // +0x14; borrowed.
	};

	static_assert(sizeof(CFont) == (sizeof(void*) == 8 ? 40 : 24), "Unexpected native font layout");
}
