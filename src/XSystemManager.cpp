#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFile.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPixelFormatConverter.h"
#include "XSystemManager.h"

namespace
{
	// Original data: 005106F4; saved by construction, retained after destruction.
	NXBasics::RuntimeNewHandler gPreviousNewHandler = nullptr;

	// Original function: FUN_00406aa2
	int __cdecl SystemManagerNewHandler(std::size_t bytesMissing)
	{
		NXBasics::ReportWarning("new_handler called - out of memory - %zu bytes missing", bytesMissing);

		return 0;
	}
}

namespace NXBasics
{
	// Original allocation wrapper 004E5DDD always enables the new handler.
	void* CXBSystemManager::operator new(std::size_t size)
	{

		return Runtime_AllocateWithNewHandler(size, 1);
	}

	void* CXBSystemManager::operator new(std::size_t size, const std::nothrow_t&) noexcept
	{

		return Runtime_AllocateWithNewHandler(size, 1);
	}

	void CXBSystemManager::operator delete(void* allocation) noexcept
	{
		Runtime_Free(allocation);
	}

	void CXBSystemManager::operator delete(void* allocation, const std::nothrow_t&) noexcept
	{
		Runtime_Free(allocation);
	}

	// Original function: FUN_004069f3
	CXBSystemManager::CXBSystemManager()
	{
		if (sTheObjectPtr != nullptr)
		{

			return;
		}

		sTheObjectPtr = this;
		gPreviousNewHandler = Runtime_SetNewHandler(SystemManagerNewHandler);
		Runtime_SetNewHandlerMode(1);
		CFile::FileSystem_Initialize();

		void* allocation = Runtime_AllocateWithNewHandler(sizeof(CPalette), 1);
		sPalettePtr = allocation != nullptr ? ::new (allocation) CPalette() : nullptr;

		allocation = Runtime_AllocateWithNewHandler(sizeof(C16BitPixelConverter), 1);
		g16BitPixelConverter = allocation != nullptr ? ::new (allocation) C16BitPixelConverter() : nullptr;

		allocation = Runtime_AllocateWithNewHandler(sizeof(C32BitPixelConverter), 1);
		g32BitPixelConverter = allocation != nullptr ? ::new (allocation) C32BitPixelConverter() : nullptr;
		// Windows calls this even when the preceding allocation fails.
		g32BitPixelConverter->SetColorMasks(0x00FF0000, 0x0000FF00, NXBasics::IntegerRange::UInt8MaximumSigned);
	}

	// Original function: FUN_00406ab5
	CXBSystemManager::~CXBSystemManager()
	{
		if (g32BitPixelConverter != nullptr)
		{
			Runtime_Free(g32BitPixelConverter);
			g32BitPixelConverter = nullptr;
		}

		if (g16BitPixelConverter != nullptr)
		{
			auto* converter = g16BitPixelConverter;
			converter->~C16BitPixelConverter();
			Runtime_Free(converter);
			g16BitPixelConverter = nullptr;
		}

		if (sPalettePtr != nullptr)
		{
			delete static_cast<CPalette*>(sPalettePtr);
			sPalettePtr = nullptr;
		}

		Runtime_SetNewHandler(gPreviousNewHandler);
		sTheObjectPtr = nullptr;
	}
}