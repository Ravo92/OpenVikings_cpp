#include "ApplicationStartup.h"
#include "GameApplication.h"
#include "MasterControlProgram.h"
#include "RemasterLog.h"
#include "X2DApplication.h"
#include "XCrtFileAccess.h"

#include <cstdint>
#include <new>
#include <sal.h>
#include <Windows.h>

namespace
{
	HANDLE g_singleInstanceMutex = nullptr;

	struct SLoggingLifetime
	{
		~SLoggingLifetime()
		{
			NRemaster::Logging::Shutdown();
		}
	};

	// Original function: FUN_0047a9a4
	bool InitializeSingleInstanceGuard(LPCSTR mutexName)
	{
		if (g_singleInstanceMutex != nullptr)
		{
			return false;
		}

		g_singleInstanceMutex = CreateMutexA(nullptr, FALSE, mutexName);

		// The original intentionally allows startup if mutex creation fails.

		if (g_singleInstanceMutex == nullptr)
		{
			return true;
		}

		return GetLastError() != ERROR_ALREADY_EXISTS;
	}

	// Original function: FUN_0047a9d9
	void ReleaseSingleInstanceGuard()
	{
		if (g_singleInstanceMutex != nullptr)
		{
			CloseHandle(g_singleInstanceMutex);
		}
	}

	// Original function: FUN_00403487; the native WinMain entry uses RET 0x10.
	int WINAPI WindowsApplicationEntry(HINSTANCE applicationInstance, HINSTANCE applicationPreviousInstance, LPSTR applicationCommandLine, int applicationShowCommand)
	{
		if (!InitializeSingleInstanceGuard(NRemaster::GetGameMutexName()))
		{
			return 0;
		}

		NRemaster::Logging::Initialize(applicationCommandLine);
		const SLoggingLifetime loggingLifetime;

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NMasterControlProgram::CMasterControlProgram));

		if (allocation != nullptr)
			::new (allocation) NMasterControlProgram::CMasterControlProgram(applicationInstance, applicationPreviousInstance, applicationCommandLine, applicationShowCommand);

		auto* masterControlProgram = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;

		if (*reinterpret_cast<const std::int32_t*>(masterControlProgram) == 2 && masterControlProgram->System_ProgramState_StartUpSystem())
		{
			do
			{
				if (!NXSys2DApp::CApplication::sTheObjectPtr->DoMessages())
				{
					break;
				}
			} while (!NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_Update());

			NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_ProgramState_EndCurrent();
		}

		auto* object = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;

		if (object != nullptr)
		{
			object->~CMasterControlProgram();
			NXBasics::Runtime_Free(object);
		}

		// Network, renderer and AI producers have stopped. Drain before allowing another instance.
		NRemaster::Logging::Shutdown();
		ReleaseSingleInstanceGuard();

		return 0;
	}
}

// Remaster entry: use the supported MSVC startup instead of reconstructing FUN_004e709e.
int NRemaster::RunGame(EGameVariant variant, HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
	gGameVariant = variant;
	// No Northland loading behavior is inferred from the Weltwunder reconstruction.

	if (variant == EGameVariant::Northland)
	{
		MessageBoxW(nullptr, L"The Northland launcher is ready, but its game-data loading is not implemented yet.", L"OpenVikings - Northland", MB_OK | MB_ICONINFORMATION);
		return ERROR_NOT_SUPPORTED;
	}
	// Remaster: use physical monitor pixels, including mixed-DPI desktops.
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	// The hosting UCRT owns heap, descriptors, environment, exceptions and exit callbacks.
	// Only the game's explicit initializers remain in their established order.

	for (const auto initialize : NApplicationStartup::gApplicationInitializers)
	{
		if (initialize != nullptr)
			initialize();
	}

	return WindowsApplicationEntry(instance, previousInstance, commandLine, showCommand);
}