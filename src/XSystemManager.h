#pragma once

#include <cstddef>
#include <new>

namespace NXBasics
{
	class CXBSystemManager
	{
	  public:
		CXBSystemManager();

		~CXBSystemManager();

		static void* operator new(std::size_t size);
		static void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
		static void operator delete(void* allocation) noexcept;
		static void operator delete(void* allocation, const std::nothrow_t&) noexcept;

		// Original data: 005106d8; Mac: CXBSystemManager::sTheObjectPtr.
		inline static CXBSystemManager* sTheObjectPtr = nullptr;
		// Original data: 005106dc; Mac: CXBSystemManager::sPalettePtr.
		inline static void* sPalettePtr = nullptr;
		// Original data: 005106e8; Mac: CXBSystemManager::sFatalErrorFunctionPtr.
		inline static void* sFatalErrorFunctionPtr = nullptr;
		// Original data: 005106ec; Mac: CXBSystemManager::sWarningErrorFunctionPtr.
		inline static void* sWarningErrorFunctionPtr = nullptr;
		// Original data: 005106f0; Mac: CXBSystemManager::sAssertFunctionPtr.
		inline static void* sAssertFunctionPtr = nullptr;
	};

	static_assert(sizeof(CXBSystemManager) == 1, "CXBSystemManager requires the original empty x86 layout");
}