#include "RemasterCrashDumps.h"

#if defined(_DEBUG)
#include <Windows.h>
#include <DbgHelp.h>
#include <strsafe.h>
#include <crtdbg.h>
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iterator>

namespace NRemaster::CrashDumps
{
	// Debug events suspend the target while this separate instance writes its memory.
	// Keep DbgHelp outside the damaged game's heap and loader state whenever possible.
	namespace
	{
		constexpr DWORD SnapshotException = 0xE04F5601u;
		constexpr std::size_t MessageCapacity = 1024;
		constexpr char HelperPrefix[] = "--openvikings-crash-writer ";
#if defined(_WIN64)
		constexpr char Architecture[] = "x64";
#else
		constexpr char Architecture[] = "x86";
#endif
		wchar_t gWorkingDirectory[32768]{ };
		HANDLE gHelperProcess = nullptr;
		volatile LONG gInitialized = 0;
		volatile LONG gFatalSnapshotWritten = 0;
		volatile LONG gDumpSequence = 0;
		volatile LONG gFallbackWriting = 0;

		bool WriteDump(HANDLE process, DWORD processId, DWORD threadId, EXCEPTION_RECORD& record, CONTEXT* context, const char* reason) noexcept
		{
			SYSTEMTIME time{ };
			GetLocalTime(&time);
			wchar_t path[32768];
			const auto sequence = InterlockedIncrement(&gDumpSequence);

			if (FAILED(StringCchPrintfW(path, std::size(path), L"%s\\OpenVikings-%S-%04u%02u%02u-%02u%02u%02u-%03u-p%lu-t%lu-%ld.dmp", gWorkingDirectory, Architecture, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, processId, threadId, sequence)))
				return false;

			HANDLE file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			DWORD error = file == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
			BOOL written = FALSE;

			if (file != INVALID_HANDLE_VALUE)
			{
				// Local copies describe the stopped target thread; no target-heap code runs here.
				EXCEPTION_POINTERS pointers{ &record, context };
				MINIDUMP_EXCEPTION_INFORMATION exception{ threadId, &pointers, FALSE };
				std::size_t reasonLength = 0;
				StringCchLengthA(reason, MessageCapacity, &reasonLength);
				MINIDUMP_USER_STREAM comment{ CommentStreamA, static_cast<ULONG>(reasonLength + 1), const_cast<char*>(reason) };
				MINIDUMP_USER_STREAM_INFORMATION streams{ 1, &comment };
				const auto flags = static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpIgnoreInaccessibleMemory);
				written = MiniDumpWriteDump(process, processId, file, flags, context != nullptr ? &exception : nullptr, &streams, nullptr);
				error = written != FALSE ? ERROR_SUCCESS : GetLastError();
				FlushFileBuffers(file);
				CloseHandle(file);
			}

			std::size_t pathLength = 0;
			StringCchLengthW(path, std::size(path), &pathLength);
			StringCchCopyW(path + pathLength - 4, 5, L".txt");
			file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (file != INVALID_HANDLE_VALUE)
			{
				char report[2048];
				StringCchPrintfA(report, std::size(report), "OpenVikings Debug diagnostic\r\nArchitecture: %s\r\nBuild: %s %s\r\nProcess: %lu\r\nThread: %lu\r\nException: 0x%08lX\r\nAddress: %p\r\nException context: %s\r\nDump: %s\r\nWriter error: 0x%08lX\r\n\r\n%s\r\n", Architecture, __DATE__, __TIME__, processId, threadId, record.ExceptionCode, record.ExceptionAddress, context != nullptr ? "captured" : "unavailable", written != FALSE ? "written" : "FAILED", error, reason);
				std::size_t length = 0;
				StringCchLengthA(report, std::size(report), &length);
				DWORD bytes = 0;
				WriteFile(file, report, static_cast<DWORD>(length), &bytes, nullptr);
				FlushFileBuffers(file);
				CloseHandle(file);
			}

			return written != FALSE;
		}

		bool WriteDebugEventDump(HANDLE process, const DEBUG_EVENT& event, const char* reason) noexcept
		{
			CONTEXT context{ };
			context.ContextFlags = CONTEXT_ALL;
			HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, event.dwThreadId);
			const BOOL captured = thread != nullptr && GetThreadContext(thread, &context);

			if (thread != nullptr)
				CloseHandle(thread);

			auto record = event.u.Exception.ExceptionRecord;
			record.ExceptionRecord = nullptr;

			return WriteDump(process, event.dwProcessId, event.dwThreadId, record, captured != FALSE ? &context : nullptr, reason);
		}

		DWORD RunHelper(DWORD processId, const char* readyName) noexcept
		{
			const DWORD directoryLength = GetCurrentDirectoryW(static_cast<DWORD>(std::size(gWorkingDirectory)), gWorkingDirectory);

			if (directoryLength == 0 || directoryLength >= std::size(gWorkingDirectory))
				return ERROR_BAD_PATHNAME;

			char cancelName[128];
			StringCchPrintfA(cancelName, std::size(cancelName), "%s.Cancel", readyName);
			HANDLE ready = OpenEventA(EVENT_MODIFY_STATE, FALSE, readyName);
			HANDLE cancel = OpenEventA(SYNCHRONIZE, FALSE, cancelName);
			HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE | SYNCHRONIZE, FALSE, processId);
			DWORD error = ERROR_SUCCESS;

			if (ready == nullptr || cancel == nullptr || process == nullptr)
			{
				error = GetLastError();
			}
			else if (WaitForSingleObject(cancel, 0) != WAIT_TIMEOUT)
			{
				error = ERROR_CANCELLED;
			}
			else if (!DebugActiveProcess(processId))
			{
				error = GetLastError();
			}
			else
			{
				if (!DebugSetProcessKillOnExit(FALSE))
				{
					error = GetLastError();
					DebugActiveProcessStop(processId);
				}
				else
				{
					SetEvent(ready);
					bool initialBreakpoint = true;
					bool fatalSnapshot = false;

					while (WaitForSingleObject(cancel, 0) == WAIT_TIMEOUT)
					{
						DEBUG_EVENT event{ };

						if (!WaitForDebugEventEx(&event, 250))
						{
							if (GetLastError() == ERROR_SEM_TIMEOUT)
								continue;

							break;
						}

						DWORD disposition = DBG_CONTINUE;
						bool exited = false;

						switch (event.dwDebugEventCode)
						{
							case CREATE_PROCESS_DEBUG_EVENT:
								if (event.u.CreateProcessInfo.hFile != nullptr)
									CloseHandle(event.u.CreateProcessInfo.hFile);

								break;
							case LOAD_DLL_DEBUG_EVENT:
								if (event.u.LoadDll.hFile != nullptr)
									CloseHandle(event.u.LoadDll.hFile);

								break;
							case EXCEPTION_DEBUG_EVENT:
							{
								const auto& exception = event.u.Exception;
								const auto& record = exception.ExceptionRecord;
								disposition = DBG_EXCEPTION_NOT_HANDLED;

								if (initialBreakpoint && exception.dwFirstChance != 0 && record.ExceptionCode == EXCEPTION_BREAKPOINT)
								{
									initialBreakpoint = false;
									disposition = DBG_CONTINUE;
								}
								else if (record.ExceptionCode == SnapshotException && exception.dwFirstChance != 0 && record.NumberParameters == 3)
								{
									char message[MessageCapacity]{ };
									const auto length = record.ExceptionInformation[1];
									SIZE_T copied = 0;

									if (length >= std::size(message) || !ReadProcessMemory(process, reinterpret_cast<const void*>(record.ExceptionInformation[0]), message, length, &copied))
										StringCchCopyA(message, std::size(message), "Diagnostic message unavailable");

									const bool written = WriteDebugEventDump(process, event, message);
									fatalSnapshot = fatalSnapshot || (written && record.ExceptionInformation[2] != 0);
									// The target's local SEH handler consumes this diagnostic exception.
								}
								else if (exception.dwFirstChance == 0)
								{
									if (!fatalSnapshot)
										WriteDebugEventDump(process, event, "Unhandled exception (including fail-fast/heap-corruption termination)");
								}

								break;
							}
							case EXIT_PROCESS_DEBUG_EVENT:
								exited = true;
								break;
						}

						if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition) || exited)
							break;
					}

					DebugActiveProcessStop(processId);
				}
			}

			if (process != nullptr)
				CloseHandle(process);

			if (cancel != nullptr)
				CloseHandle(cancel);

			if (ready != nullptr)
				CloseHandle(ready);

			return error;
		}

		LONG WINAPI FallbackExceptionFilter(EXCEPTION_POINTERS* pointers) noexcept
		{
			if (InterlockedCompareExchange(&gFallbackWriting, 1, 0) == 0)
				WriteDump(GetCurrentProcess(), GetCurrentProcessId(), GetCurrentThreadId(), *pointers->ExceptionRecord, pointers->ContextRecord, "Unhandled exception; external dump writer unavailable");

			return EXCEPTION_EXECUTE_HANDLER;
		}

		void CaptureDiagnostic(const char* message, bool fatal) noexcept
		{
			if (gInitialized == 0 || gWorkingDirectory[0] == L'\0' || (gHelperProcess == nullptr && IsDebuggerPresent()) || (!fatal && gFatalSnapshotWritten != 0) || (fatal && InterlockedCompareExchange(&gFatalSnapshotWritten, 1, 0) != 0))
			{

				return;
			}

			std::size_t length = 0;

			if (message == nullptr || FAILED(StringCchLengthA(message, MessageCapacity, &length)))
			{
				message = "Fatal runtime error or assertion";
				length = sizeof("Fatal runtime error or assertion") - 1;
			}

			if (gHelperProcess != nullptr && WaitForSingleObject(gHelperProcess, 0) == WAIT_TIMEOUT && IsDebuggerPresent())
			{
				const ULONG_PTR parameters[]{ reinterpret_cast<ULONG_PTR>(message), length, fatal ? 1u : 0u };
				__try
				{
					RaiseException(SnapshotException, 0, static_cast<DWORD>(std::size(parameters)), parameters);
				}
				__except (GetExceptionCode() == SnapshotException ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
				{
				}
			}
			else if (InterlockedCompareExchange(&gFallbackWriting, 1, 0) == 0)
			{
				CONTEXT context{ };
				RtlCaptureContext(&context);
				EXCEPTION_RECORD record{ };
				record.ExceptionCode = SnapshotException;
#if defined(_WIN64)
				record.ExceptionAddress = reinterpret_cast<void*>(context.Rip);
#else
				record.ExceptionAddress = reinterpret_cast<void*>(context.Eip);
#endif
				WriteDump(GetCurrentProcess(), GetCurrentProcessId(), GetCurrentThreadId(), record, &context, message);
			}
		}

		void __cdecl AbortHandler(int) noexcept
		{
			CaptureDiagnostic("abort()/SIGABRT", true);
		}

		void __cdecl TerminateHandler() noexcept
		{
			CaptureDiagnostic("Uncaught C++ exception or std::terminate()", true);
			std::abort();
		}

		void __cdecl InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) noexcept
		{
			CaptureDiagnostic("Invalid CRT parameter", true);
			std::abort();
		}

		void __cdecl PureCallHandler() noexcept
		{
			CaptureDiagnostic("Pure virtual function call", true);
			std::abort();
		}

		int __cdecl CrtReportHook(int type, char* message, int*) noexcept
		{
			if (type == _CRT_ASSERT || type == _CRT_ERROR)
				CaptureDiagnostic(message, false);

			return FALSE;
		}

		int __cdecl CrtWideReportHook(int type, wchar_t* message, int*) noexcept
		{
			if (type == _CRT_ASSERT || type == _CRT_ERROR)
			{
				char utf8[MessageCapacity];

				if (message != nullptr && WideCharToMultiByte(CP_UTF8, 0, message, -1, utf8, static_cast<int>(std::size(utf8)), nullptr, nullptr) != 0)
				{
					CaptureDiagnostic(utf8, false);
				}
				else
				{
					CaptureDiagnostic("CRT assertion or error", false);
				}
			}

			return FALSE;
		}
	}

	bool RunHelperIfRequested(const char* commandLine, int& exitCode) noexcept
	{
		if (commandLine == nullptr || std::strncmp(commandLine, HelperPrefix, sizeof(HelperPrefix) - 1) != 0)
			return false;

		DWORD processId = 0;
		char readyName[96];
		char extra = 0;

		if (std::sscanf(commandLine + sizeof(HelperPrefix) - 1, "%lu %95s %c", &processId, readyName, &extra) != 2 || processId == 0)
		{
			exitCode = ERROR_INVALID_PARAMETER;
		}
		else
		{
			exitCode = static_cast<int>(RunHelper(processId, readyName));
		}

		return true;
	}

	void Initialize() noexcept
	{
		if (InterlockedCompareExchange(&gInitialized, 1, 0) != 0 || IsDebuggerPresent())
			return;

		const DWORD length = GetCurrentDirectoryW(static_cast<DWORD>(std::size(gWorkingDirectory)), gWorkingDirectory);

		if (length == 0 || length >= std::size(gWorkingDirectory))
			return;

		SetUnhandledExceptionFilter(FallbackExceptionFilter);
		wchar_t executable[32768];
		wchar_t command[32768];
		wchar_t readyName[96];
		wchar_t cancelName[128];
		const DWORD executableLength = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));

		if (executableLength == 0 || executableLength >= std::size(executable))
			return;

		StringCchPrintfW(readyName, std::size(readyName), L"Local\\OpenVikings.CrashDumps.%lu.%llu", GetCurrentProcessId(), GetTickCount64());
		StringCchPrintfW(cancelName, std::size(cancelName), L"%s.Cancel", readyName);

		if (FAILED(StringCchPrintfW(command, std::size(command), L"\"%s\" --openvikings-crash-writer %lu %s", executable, GetCurrentProcessId(), readyName)))
			return;

		HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, readyName);
		HANDLE cancel = CreateEventW(nullptr, TRUE, FALSE, cancelName);
		STARTUPINFOW startup{ };
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION helper{ };

		if (ready != nullptr && cancel != nullptr && CreateProcessW(executable, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, gWorkingDirectory, &startup, &helper))
		{
			CloseHandle(helper.hThread);
			const HANDLE waits[]{ ready, helper.hProcess };

			if (WaitForMultipleObjects(static_cast<DWORD>(std::size(waits)), waits, FALSE, 15000) == WAIT_OBJECT_0)
			{
				gHelperProcess = helper.hProcess;
			}
			else
			{
				SetEvent(cancel);
				CloseHandle(helper.hProcess);
			}
		}

		if (cancel != nullptr)
			CloseHandle(cancel);

		if (ready != nullptr)
			CloseHandle(ready);

		if (gHelperProcess == nullptr)
		{
			OutputDebugStringA("OpenVikings: external dump writer unavailable; unhandled-exception fallback remains active.\n");

			return;
		}

		std::signal(SIGABRT, AbortHandler);
		std::set_terminate(TerminateHandler);
		_set_invalid_parameter_handler(InvalidParameterHandler);
		_set_purecall_handler(PureCallHandler);
		_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtReportHook);
		_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, CrtWideReportHook);
	}

	void WriteFatalError(const char* message) noexcept
	{
		CaptureDiagnostic(message, true);
	}
}
#endif