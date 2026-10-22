#include "XCrtFileAccess.h"

#include <cerrno>
#include <cctype>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <list>
#include <mutex>
#include <new>
#include <new.h>
#include <share.h>

namespace NXBasics
{
	namespace
	{
		bool CopyLegacyFindData(const _finddata32i64_t& native, void* destination)
		{
			// Remaster: keep the 280-byte ABI without truncating large file sizes.
			if (native.size < 0 || static_cast<std::uint64_t>(native.size) >= UINT32_MAX) { errno = EOVERFLOW; return false; }
			auto* result = static_cast<_finddata32_t*>(destination);
			result->attrib = native.attrib;
			result->time_create = native.time_create;
			result->time_access = native.time_access;
			result->time_write = native.time_write;
			result->size = static_cast<_fsize_t>(native.size);
			std::memcpy(result->name, native.name, sizeof(result->name));
			result->name[sizeof(result->name) - 1] = '\0';
			return true;
		}

		// Stable records retain the legacy pointer/EOF contract while FILE owns buffering.
		// Records are reused only after close, as in the original stream table.
		struct StreamStorage
		{
			std::mutex mutex;
			std::list<RuntimeFormatStream> records;
			~StreamStorage()
			{
				for (auto& stream : records)
					if (stream.nativeFile != nullptr) std::fclose(stream.nativeFile);
			}
		};

		// Construct before WinMain so application on-exit callbacks precede its destruction.
		StreamStorage gStreamStorage;

		RuntimeFormatStream* WrapStream(std::FILE* file)
		{
			if (file == nullptr) return nullptr;
			try
			{
				auto& storage = gStreamStorage;
				const std::lock_guard<std::mutex> lock(storage.mutex);
				for (auto& stream : storage.records)
					if (stream.nativeFile == nullptr)
					{
						stream.nativeFile = file;
						stream.flags = 0;
						return &stream;
					}
				storage.records.push_back({ file, {}, 0, {} });
				return &storage.records.back();
			}
			catch (const std::bad_alloc&)
			{
				std::fclose(file);
				errno = ENOMEM;
				return nullptr;
			}
		}

		std::FILE* NativeStream(RuntimeFormatStream* stream)
		{
			if (stream == nullptr || stream->nativeFile == nullptr)
			{
				errno = EBADF;
				return nullptr;
			}
			return stream->nativeFile;
		}

		void UpdateStreamStatus(RuntimeFormatStream* stream)
		{
			stream->flags = (std::feof(stream->nativeFile) ? 0x10u : 0u) | (std::ferror(stream->nativeFile) ? 0x20u : 0u);
		}
	}

	// FUN_004E6F4C: retain truncation and the masked-invalid integer result, without FPU state changes.
	std::int64_t Runtime_TruncateDoubleToInt64(double value)
	{
		if (!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63) return INT64_MIN;
		return static_cast<std::int64_t>(value);
	}

	// Numeric sign reinterpretation used by native wrapping integer arithmetic.
	std::int32_t Runtime_TimeSigned32(std::uint32_t bits)
	{
		std::int32_t value;
		std::memcpy(&value, &bits, sizeof(value));
		return value;
	}

	std::uint64_t __stdcall Runtime_MultiplyUInt64(std::uint64_t first, std::uint64_t second) { return first * second; }
	void* Runtime_Allocate(std::size_t requestedBytes) { return std::malloc(requestedBytes == 0 ? 1u : requestedBytes); }
	void Runtime_Free(void* memory) { std::free(memory); }
	RuntimeNewHandler Runtime_SetNewHandler(RuntimeNewHandler handler) { return _set_new_handler(handler); }
	std::int32_t Runtime_SetNewHandlerMode(std::int32_t mode) { return _set_new_mode(mode); }

	// FUN_004E5DDD: placement-construction callers require null on allocation failure.
	void* Runtime_AllocateWithNewHandler(std::size_t requestedBytes, std::int32_t useNewHandler)
	{
		for (;;)
		{
			void* allocation = Runtime_Allocate(requestedBytes);
			if (allocation != nullptr || useNewHandler == 0 || _query_new_mode() != 0) return allocation;
			const auto handler = _query_new_handler();
			if (handler == nullptr || handler(requestedBytes) == 0) return nullptr;
		}
	}
	void* Runtime_AllocateWithNewHandler(std::size_t requestedBytes) { return Runtime_AllocateWithNewHandler(requestedBytes, 1); }
	void* Runtime_AllocateArrayWithNewHandler(std::size_t count, std::size_t elementSize, std::int32_t useNewHandler)
	{
		if (elementSize != 0 && count > SIZE_MAX / elementSize) { errno = ENOMEM; return nullptr; }
		return Runtime_AllocateWithNewHandler(count * elementSize, useNewHandler);
	}

	// UCRT owns callback storage, synchronization, reverse order and process termination.
	RuntimeOnExitCallback Runtime_RegisterOnExit(RuntimeOnExitCallback callback) { return _onexit(callback); }
	int Runtime_AtExit(RuntimeOnExitCallback callback) { return Runtime_RegisterOnExit(callback) != nullptr ? 0 : -1; }
	void Runtime_Abort() { std::abort(); }
	int Runtime_RaiseSignal(std::int32_t signalNumber) { return std::raise(signalNumber); }

	void* Runtime_CopyMemory(void* destination, const void* source, std::size_t byteCount) { return std::memmove(destination, source, byteCount); }
	void* Runtime_MoveMemory(void* destination, const void* source, std::size_t byteCount) { return std::memmove(destination, source, byteCount); }
	void* Runtime_FillMemory(void* destination, std::int32_t value, std::size_t byteCount) { return std::memset(destination, value, byteCount); }
	std::int32_t Runtime_CompareMemory(const void* first, const void* second, std::size_t byteCount) { return std::memcmp(first, second, byteCount); }
	char* Runtime_CopyString(char* destination, const char* source) { return std::strcpy(destination, source); }
	std::uint32_t Runtime_StringLength(const char* text) { return static_cast<std::uint32_t>(std::strlen(text)); }
	char* Runtime_FindLastCharacter(const char* text, std::int32_t character) { return const_cast<char*>(std::strrchr(text, character)); }
	char* Runtime_DuplicateString(const char* source) { return _strdup(source); }
	wchar_t* Runtime_DuplicateWideString(const wchar_t* source) { return _wcsdup(source); }
	std::int32_t Runtime_CompareStringsIgnoreCase(const char* first, const char* second) { return _stricmp(first, second); }
	std::int32_t Runtime_CompareStringsIgnoreCaseN(const char* first, const char* second, std::uint32_t count) { return _strnicmp(first, second, count); }
	char* Runtime_LowercaseString(char* text) { return _strlwr(text); }
	std::int32_t Runtime_ToLower(std::int32_t character) { return character >= 0 && character <= 255 ? std::tolower(character) : character; }
	bool Runtime_IsLetter(char character) { return std::isalpha(static_cast<unsigned char>(character)) != 0; }
	bool Runtime_IsDigit(char character) { return std::isdigit(static_cast<unsigned char>(character)) != 0; }
	bool Runtime_IsLetterOrNumber(char character) { return std::isalnum(static_cast<unsigned char>(character)) != 0; }
	bool Runtime_IsPropertyNameCharacter(char character) { return Runtime_IsLetterOrNumber(character) || character == '_'; }
	std::int32_t Runtime_StringToInteger(const char* text) { return std::atoi(text); }
	std::int32_t Runtime_Atoi(const char* text) { return std::atoi(text); }
	double Runtime_StringToDouble(const char* text) { return std::strtod(text, nullptr); }
	char* Runtime_LongToString(std::int32_t value, char* destination, std::int32_t radix) { return _ltoa(value, destination, radix); }
	std::int32_t Runtime_Rand() { return std::rand(); }
	void Runtime_QuickSort(void* base, std::size_t count, std::size_t width, RuntimeSortCompare compare) { if (count > 1 && width != 0) std::qsort(base, count, width, compare); }

	// Existing callers use the legacy count-of-characters contract, with a separate terminator slot.
	std::int32_t Runtime_FormatString(char* destination, std::int32_t capacity, const char* format, va_list arguments)
	{
		if (destination == nullptr || format == nullptr || capacity < 0) { errno = EINVAL; return -1; }
		return _vsnprintf(destination, static_cast<std::size_t>(capacity), format, arguments);
	}
	std::int32_t Runtime_PrintString(char* destination, const char* format, ...)
	{
		va_list arguments;
		va_start(arguments, format);
		const auto result = std::vsprintf(destination, format, arguments);
		va_end(arguments);
		return result;
	}
	std::int32_t Runtime_PrintStringN(char* destination, std::uint32_t capacity, const char* format, ...)
	{
		va_list arguments;
		va_start(arguments, format);
		const auto result = capacity <= INT32_MAX ? Runtime_FormatString(destination, static_cast<std::int32_t>(capacity), format, arguments) : -1;
		va_end(arguments);
		return result;
	}

	std::int32_t Runtime_Open(const char* fileName, std::uint32_t flags, std::uint32_t permissions) { return _sopen(fileName, static_cast<int>(flags), _SH_DENYNO, static_cast<int>(permissions)); }
	std::int32_t Runtime_Close(std::uint32_t descriptor) { return _close(static_cast<int>(descriptor)); }
	std::int32_t Runtime_Read(std::uint32_t descriptor, void* destination, std::uint32_t byteCount) { return _read(static_cast<int>(descriptor), destination, byteCount); }
	std::int32_t Runtime_Write(std::uint32_t descriptor, const void* data, std::uint32_t byteCount) { return _write(static_cast<int>(descriptor), data, byteCount); }
	std::uint32_t Runtime_Seek(std::uint32_t descriptor, std::int32_t offset, std::uint32_t origin) { return static_cast<std::uint32_t>(_lseek(static_cast<int>(descriptor), offset, static_cast<int>(origin))); }
	std::uint32_t Runtime_GetFileSize(std::uint32_t descriptor)
	{
		const auto size = _filelengthi64(static_cast<int>(descriptor));
		if (size < 0 || size >= UINT32_MAX) { if (size >= UINT32_MAX) errno = EOVERFLOW; return UINT32_MAX; }
		return static_cast<std::uint32_t>(size);
	}
	DWORD* Runtime_GetErrnoPointer()
	{
		static_assert(sizeof(DWORD) == sizeof(errno));
		return reinterpret_cast<DWORD*>(_errno());
	}
	char* Runtime_GetErrorText(std::int32_t errorCode) { return std::strerror(errorCode); }
	std::int32_t Runtime_CreateDirectory(const char* path) { return _mkdir(path); }
	std::int32_t Runtime_RemoveFile(const char* fileName) { return std::remove(fileName); }
	std::int32_t Runtime_RenameFile(const char* oldFileName, const char* newFileName) { return std::rename(oldFileName, newFileName); }
	void Runtime_SplitPath(const char* path, char* drive, char* directory, char* baseName, char* extension) { _splitpath(path, drive, directory, baseName, extension); }
	void Runtime_MakePath(char* path, const char* drive, const char* directory, const char* baseName, const char* extension) { _makepath(path, drive, directory, baseName, extension); }
	std::intptr_t Runtime_FindFirst(const char* pattern, void* result)
	{
		static_assert(sizeof(_finddata32_t) == 280 && offsetof(_finddata32_t, name) == 20);
		if (pattern == nullptr || result == nullptr) { errno = EINVAL; return -1; }
		_finddata32i64_t native{};
		const auto handle = _findfirst32i64(pattern, &native);
		if (handle == -1) return -1;
		do { if (CopyLegacyFindData(native, result)) return handle; } while (_findnext32i64(handle, &native) == 0);
		_findclose(handle);
		return -1;
	}
	std::int32_t Runtime_FindNext(std::intptr_t searchHandle, void* result)
	{
		if (result == nullptr) { errno = EINVAL; return -1; }
		_finddata32i64_t native{};
		while (_findnext32i64(searchHandle, &native) == 0) if (CopyLegacyFindData(native, result)) return 0;
		return -1;
	}
	std::int32_t Runtime_FindClose(std::intptr_t searchHandle) { return _findclose(searchHandle); }
	char* Runtime_GetEnvironmentVariableLocked(const char* name) { return std::getenv(name); }
	char* Runtime_GetLocalTimeString(char* destination)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		std::sprintf(destination, "%02u:%02u:%02u", static_cast<unsigned>(time.wHour), static_cast<unsigned>(time.wMinute), static_cast<unsigned>(time.wSecond));
		return destination;
	}

	RuntimeFormatStream* Runtime_OpenStream(const char* fileName, const char* mode) { return WrapStream(std::fopen(fileName, mode)); }
	RuntimeFormatStream* Runtime_CreateTemporaryStream()
	{
		// UCRT tmpfile uses the drive root. Use the user's writable temp directory instead.
		wchar_t directory[MAX_PATH];
		wchar_t path[MAX_PATH];
		const auto length = GetTempPathW(MAX_PATH, directory);
		if (length == 0 || length >= MAX_PATH || GetTempFileNameW(directory, L"OVK", 0, path) == 0) { errno = EIO; return nullptr; }
		const auto handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (handle == INVALID_HANDLE_VALUE) { DeleteFileW(path); errno = EIO; return nullptr; }
		const auto descriptor = _open_osfhandle(reinterpret_cast<std::intptr_t>(handle), _O_RDWR | _O_BINARY);
		if (descriptor == -1) { CloseHandle(handle); return nullptr; }
		const auto file = _fdopen(descriptor, "w+b");
		if (file == nullptr) { _close(descriptor); return nullptr; }
		return WrapStream(file);
	}
	std::int32_t Runtime_CloseStream(RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return EOF;
		auto& storage = gStreamStorage;
		const std::lock_guard<std::mutex> lock(storage.mutex);
		stream->nativeFile = nullptr;
		return std::fclose(file);
	}
	std::int32_t Runtime_FlushStream(RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return EOF;
		const auto result = std::fflush(file);
		UpdateStreamStatus(stream);
		return result;
	}
	std::uint32_t Runtime_ReadStream(void* destination, std::uint32_t elementSize, std::uint32_t elementCount, RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return 0;
		const auto result = std::fread(destination, elementSize, elementCount, file);
		UpdateStreamStatus(stream);
		return static_cast<std::uint32_t>(result);
	}
	std::uint32_t Runtime_WriteStream(const void* data, std::uint32_t elementSize, std::uint32_t elementCount, RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return 0;
		const auto result = std::fwrite(data, elementSize, elementCount, file);
		UpdateStreamStatus(stream);
		return static_cast<std::uint32_t>(result);
	}
	std::int32_t Runtime_ReadStreamCharacter(RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return EOF;
		const auto result = std::fgetc(file);
		UpdateStreamStatus(stream);
		return result;
	}
	std::int32_t Runtime_WriteStreamCharacter(std::int32_t character, RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return EOF;
		const auto result = std::fputc(character, file);
		UpdateStreamStatus(stream);
		return result;
	}
	std::int32_t Runtime_SeekStream(RuntimeFormatStream* stream, std::int32_t offset, std::uint32_t origin)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return -1;
		const auto result = std::fseek(file, offset, static_cast<int>(origin));
		UpdateStreamStatus(stream);
		return result;
	}
	std::int32_t Runtime_TellStream(RuntimeFormatStream* stream)
	{
		const auto file = NativeStream(stream);
		return file == nullptr ? -1 : std::ftell(file);
	}
	std::int32_t Runtime_PrintStream(RuntimeFormatStream* stream, const char* format, ...)
	{
		const auto file = NativeStream(stream);
		if (file == nullptr) return -1;
		va_list arguments;
		va_start(arguments, format);
		const auto result = std::vfprintf(file, format, arguments);
		va_end(arguments);
		UpdateStreamStatus(stream);
		return result;
	}
}
