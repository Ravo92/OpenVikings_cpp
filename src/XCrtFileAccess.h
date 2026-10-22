#pragma once

#include "XNumericConstants.h"

#include <Windows.h>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <type_traits>

// Remaster runtime facade. The historical include name keeps existing callers stable.
// Implementation uses the supported MSVC/UCRT services, not the reconstructed CRT.
namespace NXBasics
{
	using RuntimeVoidCallback = void(__cdecl*)();
	using RuntimeOnExitCallback = int(__cdecl*)();
	using RuntimeNewHandler = int(__cdecl*)(std::size_t);
	using RuntimeSortCompare = std::int32_t(__cdecl*)(const void*, const void*);

	// Keep the EOF flag's existing offset for stream consumers on both architectures.
	// The remaining native stream machinery is owned by the UCRT FILE object.
	struct RuntimeFormatStream
	{
		std::FILE* nativeFile;
		std::byte reserved04[12 - sizeof(std::FILE*)];
		std::uint32_t flags;
		std::byte reserved10[16];
	};
	static_assert(sizeof(RuntimeFormatStream) == 32);
	static_assert(offsetof(RuntimeFormatStream, flags) == 0x0C);

	std::int64_t Runtime_TruncateDoubleToInt64(double value);
	std::uint64_t __stdcall Runtime_MultiplyUInt64(std::uint64_t first, std::uint64_t second);
	bool Runtime_IsLetter(char character);
	bool Runtime_IsDigit(char character);
	bool Runtime_IsLetterOrNumber(char character);
	bool Runtime_IsPropertyNameCharacter(char character);
	std::int32_t Runtime_StringToInteger(const char* text);
	std::int32_t Runtime_Atoi(const char* text);
	RuntimeOnExitCallback Runtime_RegisterOnExit(RuntimeOnExitCallback callback);
	int Runtime_AtExit(RuntimeOnExitCallback callback);
	std::int32_t Runtime_Close(std::uint32_t descriptor);
	std::int32_t Runtime_Write(std::uint32_t descriptor, const void* data, std::uint32_t byteCount);
	std::int32_t Runtime_Open(const char* fileName, std::uint32_t flags, std::uint32_t permissions);
	std::int32_t Runtime_Read(std::uint32_t descriptor, void* destination, std::uint32_t byteCount);
	std::uint32_t Runtime_Seek(std::uint32_t descriptor, std::int32_t offset, std::uint32_t origin);
	std::uint32_t Runtime_GetFileSize(std::uint32_t descriptor);
	std::int32_t Runtime_CompareStringsIgnoreCaseN(const char* first, const char* second, std::uint32_t count);
	char* Runtime_GetErrorText(std::int32_t errorCode);
	DWORD* Runtime_GetErrnoPointer();
	std::int32_t Runtime_FormatString(char* destination, std::int32_t capacity, const char* format, va_list arguments);
	std::int32_t Runtime_CreateDirectory(const char* path);
	std::intptr_t Runtime_FindFirst(const char* pattern, void* result);
	std::int32_t Runtime_FindNext(std::intptr_t searchHandle, void* result);
	std::int32_t Runtime_FindClose(std::intptr_t searchHandle);
	std::int32_t Runtime_ToLower(std::int32_t character);
	void Runtime_Abort();
	std::int32_t Runtime_SetNewHandlerMode(std::int32_t mode);
	RuntimeNewHandler Runtime_SetNewHandler(RuntimeNewHandler handler);
	void Runtime_SplitPath(const char* path, char* drive, char* directory, char* baseName, char* extension);
	std::int32_t Runtime_CompareStringsIgnoreCase(const char* first, const char* second);
	void Runtime_MakePath(char* path, const char* drive, const char* directory, const char* baseName, const char* extension);
	double Runtime_StringToDouble(const char* text);
	int Runtime_RaiseSignal(std::int32_t signalNumber);
	void Runtime_Free(void* memory);
	void* Runtime_Allocate(std::size_t requestedBytes);
	char* Runtime_CopyString(char* destination, const char* source);
	std::uint32_t Runtime_StringLength(const char* text);
	char* Runtime_FindLastCharacter(const char* text, std::int32_t character);
	std::int32_t Runtime_CompareMemory(const void* first, const void* second, std::size_t byteCount);
	void* Runtime_CopyMemory(void* destination, const void* source, std::size_t byteCount);
	void* Runtime_MoveMemory(void* destination, const void* source, std::size_t byteCount);
	void* Runtime_AllocateWithNewHandler(std::size_t requestedBytes);
	void* Runtime_FillMemory(void* destination, std::int32_t value, std::size_t byteCount);
	void* Runtime_AllocateWithNewHandler(std::size_t requestedBytes, std::int32_t useNewHandler);
	// Native allocation sizes are independent of fixed-width game/save counts.
	void* Runtime_AllocateArrayWithNewHandler(std::size_t count, std::size_t elementSize, std::int32_t useNewHandler = 1);

	struct RuntimeBufferDeleter
	{
		template<class T> void operator()(T* memory) const noexcept
		{
			static_assert(std::is_trivially_destructible_v<T>, "Runtime buffers cannot own objects requiring destruction");
			Runtime_Free(memory);
		}
	};
	template<class T> using RuntimeBuffer = std::unique_ptr<T[], RuntimeBufferDeleter>;
	wchar_t* Runtime_DuplicateWideString(const wchar_t* source);
	char* Runtime_DuplicateString(const char* source);
	std::int32_t Runtime_TimeSigned32(std::uint32_t bits);
	char* Runtime_LowercaseString(char* text);
	void Runtime_QuickSort(void* base, std::size_t count, std::size_t width, RuntimeSortCompare compare);
	std::int32_t Runtime_CloseStream(RuntimeFormatStream* stream);
	std::int32_t Runtime_FlushStream(RuntimeFormatStream* stream);
	std::int32_t Runtime_PrintString(char* destination, const char* format, ...);
	std::int32_t Runtime_PrintStringN(char* destination, std::uint32_t capacity, const char* format, ...);
	char* Runtime_GetLocalTimeString(char* destination);
	char* Runtime_LongToString(std::int32_t value, char* destination, std::int32_t radix);
	std::int32_t Runtime_PrintStream(RuntimeFormatStream* stream, const char* format, ...);
	RuntimeFormatStream* Runtime_OpenStream(const char* fileName, const char* mode);
	char* Runtime_GetEnvironmentVariableLocked(const char* name);
	std::int32_t Runtime_RemoveFile(const char* fileName);
	std::int32_t Runtime_Rand();
	std::int32_t Runtime_RenameFile(const char* oldFileName, const char* newFileName);
	std::int32_t Runtime_SeekStream(RuntimeFormatStream* stream, std::int32_t offset, std::uint32_t origin);
	std::int32_t Runtime_WriteStreamCharacter(std::int32_t character, RuntimeFormatStream* stream);
	std::int32_t Runtime_TellStream(RuntimeFormatStream* stream);
	RuntimeFormatStream* Runtime_CreateTemporaryStream();
	std::int32_t Runtime_ReadStreamCharacter(RuntimeFormatStream* stream);
	std::uint32_t Runtime_WriteStream(const void* data, std::uint32_t elementSize, std::uint32_t elementCount, RuntimeFormatStream* stream);
	std::uint32_t Runtime_ReadStream(void* destination, std::uint32_t elementSize, std::uint32_t elementCount, RuntimeFormatStream* stream);
}
