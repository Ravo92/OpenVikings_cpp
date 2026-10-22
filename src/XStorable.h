#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

namespace NXBasics
{
	class CStorable
	{
	public:
		static void* operator new(std::size_t size);
		static void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
		static void* operator new(std::size_t, void* allocation) noexcept
		{
			return allocation;
		}
		static void operator delete(void* allocation) noexcept;
		static void operator delete(void* allocation, const std::nothrow_t&) noexcept;
		static void operator delete(void*, void*) noexcept
		{}

		// Original function: FUN_0041067e
		void Storable_Save(void* file);

		// Original function: FUN_004106c2
		bool Storable_Save(const char* fileName, bool openOption);

		// Original virtual slot: +0x00
		virtual std::uint32_t Storable_GetId() = 0;

		// Original virtual slot: +0x04
		virtual std::uint32_t Storable_GetVersion() = 0;

		// Original virtual slot: +0x08
		virtual void Storable_SaveData(void* file) = 0;

	protected:
		// Derived classes add a virtual destructor only where present in the original vtable.
		~CStorable() = default;
	};

	static_assert(sizeof(CStorable) == sizeof(void*), "CStorable must contain only its virtual-table pointer");
}
