#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XSystemManager.h"

#include <cstdarg>
#include <cstring>
#include <Windows.h>

namespace NXBasics
{
	// Original function: FUN_00406935
	void __cdecl ReportFatalError(const char* format, ...)
	{
		char buffer[500];

		va_list arguments;
		va_start(arguments, format);

		Runtime_FormatString(buffer, 499, format, arguments);

		va_end(arguments);

		using FatalErrorCallback = void(__cdecl*)(const char*);

		FatalErrorCallback callback;
		std::memcpy(&callback, &CXBSystemManager::sFatalErrorFunctionPtr, sizeof(callback));

		buffer[499] = '\0';

		if (callback != nullptr)
		{
			callback(buffer);

			return;
		}

		OutputDebugStringA("\n\nNXBasics: FATAL ERROR:\n\n");
		OutputDebugStringA(buffer);
		OutputDebugStringA("\n\n\n");

		Runtime_Abort();
	}

	// Original function: FUN_00406995
	void __cdecl ReportWarning(const char* format, ...)
	{
		char buffer[500];

		va_list arguments;
		va_start(arguments, format);

		Runtime_FormatString(buffer, 499, format, arguments);

		va_end(arguments);

		using WarningCallback = void(__cdecl*)(const char*);

		WarningCallback callback;
		std::memcpy(&callback, &NXBasics::CXBSystemManager::sWarningErrorFunctionPtr, sizeof(callback));

		buffer[499] = '\0';

		if (callback != nullptr)
		{
			callback(buffer);

			return;
		}

		OutputDebugStringA("\n\nNXBasics: WARNING:\n\n");
		OutputDebugStringA(buffer);
		OutputDebugStringA("\n\n\n");
	}
}