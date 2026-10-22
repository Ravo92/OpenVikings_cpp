#pragma once

#include "XStorable.h"

#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
}

namespace NC2MainMenuGuiManager
{
	class CGuiManager;
}

namespace NXBasics
{
	class CMemory : public CStorable
	{
	public:
		// Original function: LAB_0040c775
		std::uint32_t Storable_GetVersion() override;

		// Original function: FUN_0042f805
		explicit CMemory(std::uint32_t size);
		CMemory(const CMemory&) = delete;
		CMemory& operator=(const CMemory&) = delete;

		// Original function: FUN_0042f84b
		CMemory(std::uint32_t size, std::uint8_t fillValue);

		// Original function: FUN_0042f88a
		explicit CMemory(const char* fileName);

		// Original function: FUN_0042f8f4.
		virtual ~CMemory();
		void* Delete(std::uint32_t flags);

		// Original function: FUN_0042f91a
		CMemory(void* file, std::uint32_t version);

		// Original function: FUN_0042f95e
		void Storable_SaveData(void* file) override;

		// Original function: LAB_0042f9b0
		std::uint32_t Storable_GetId() override;

		// Original function: FUN_0042f9b6
		void Fill(std::uint8_t value);

		// Original function: FUN_0042f9cd
		bool ChangeBufferSize(std::uint32_t newSize, bool fillAdditionalBytes, std::uint8_t fillValue);

		// Original function: FUN_0042fa37
		void Encrypt(std::int32_t parameter);

		// Original function: FUN_0042fa4c
		void Decrypt(std::int32_t parameter);

		std::uint32_t GetSize() const noexcept { return mSize; }
		std::uint8_t* GetData() noexcept { return mData; }
		const std::uint8_t* GetData() const noexcept { return mData; }

		// Original function: FUN_004e5e48
		static void operator delete(void* allocation) noexcept;
		static void operator delete(void* allocation, const std::nothrow_t&) noexcept;

	private:
		friend class CStringArray;
		friend class CIniFileReader;
		friend class CBobManager;
		friend class CPicture;
		friend class NC2MainMenuGuiManager::CGuiManager;
		friend class NC2Logic::CIoHelper;

		// Original function: FUN_0042f984
		void l_AllocateMemory(std::uint32_t size);

		std::uint32_t mSize;       // +0x04
		std::uint8_t* mData;       // +0x08
	};

	static_assert(sizeof(CMemory) == (sizeof(void*) == 8 ? 24 : 12), "Unexpected native CMemory layout");
}
