#pragma once

#include "XNumericConstants.h"

#include "OriginalStrings.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFile.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace NXBasics
{
	// Native member access replaces offsets that depended on the Win32 vtable width.
	struct SFileAccess
	{
		static std::int32_t& Descriptor(CFile& file) noexcept
		{

			return file.mDescriptor;
		}

		static const std::int32_t& Descriptor(const CFile& file) noexcept
		{

			return file.mDescriptor;
		}

		static std::int32_t& ArchiveIndex(CFile& file) noexcept
		{

			return file.mArchiveIndex;
		}

		static const std::int32_t& ArchiveIndex(const CFile& file) noexcept
		{

			return file.mArchiveIndex;
		}

		static char (&FileName(CFile& file) noexcept)[260]
		{
			return file.mFileName;
		}

		static const char (&FileName(const CFile& file) noexcept)[260]
		{
			return file.mFileName;
		}
	};

	inline std::uint8_t Archive_CalculateNameCheck(const char* fileName);
	inline std::uint32_t Archive_FindFileIndex(void* archive, const char* fileName);
	inline bool Archive_IsFileInLibrary(void* archive, const char* fileName);
	inline std::uint32_t Archive_GetFileOffset(void* archive, const char* fileName);
	inline std::uint32_t Archive_GetFileSize(void* archive, const char* fileName);
	inline void File_SeekToPosition(void* file, std::uint32_t position);

	// Original function: FUN_00406126; NXBasics::CFile::Close().
	inline void File_Close(void* file)
	{
		auto* fileObject = static_cast<CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{

			return;
		}

		const std::int32_t result = Runtime_Close(static_cast<std::uint32_t>(descriptor));

		if (static_cast<std::uint32_t>(result) == NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto firstError = *NXBasics::Runtime_GetErrnoPointer();
			const auto errorText = NXBasics::Runtime_GetErrorText(firstError);

			// Read errno again after obtaining its description.
			const auto currentError = *NXBasics::Runtime_GetErrnoPointer();
			const auto* fileName = SFileAccess::FileName(*fileObject);

			NXBasics::ReportFatalError(OriginalStrings::FileCloseError, fileName, currentError, errorText);
		}

		const std::int32_t invalidIndex = -1;
		SFileAccess::Descriptor(*fileObject) = invalidIndex;
		SFileAccess::ArchiveIndex(*fileObject) = static_cast<std::int32_t>(invalidIndex);
	}

	// Original function: FUN_00406170
	inline void File_Read(void* file, void* destination, std::uint32_t byteCount)
	{
		const auto* fileObject = static_cast<const CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{
			return;
		}

		const std::int32_t bytesRead = Runtime_Read(static_cast<std::uint32_t>(descriptor), destination, byteCount);

		if (static_cast<std::uint32_t>(bytesRead) == byteCount)
		{

			return;
		}

		const auto firstError = *Runtime_GetErrnoPointer();
		const auto errorText = NXBasics::Runtime_GetErrorText(firstError);
		const auto currentError = *Runtime_GetErrnoPointer();
		const auto* fileName = SFileAccess::FileName(*fileObject);

		NXBasics::ReportFatalError("NXBasics::CFile::Read\n\nfile %s - errno %d: %s", fileName, currentError, errorText);
	}

	// Original function: FUN_00406286
	inline void File_SeekToPosition(void* file, std::uint32_t position)
	{
		const auto* fileObject = static_cast<const CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{

			return;
		}

		std::uint32_t archiveIndex;
		archiveIndex = static_cast<std::uint32_t>(SFileAccess::ArchiveIndex(*fileObject));

		const auto* fileName = SFileAccess::FileName(*fileObject);

		if (archiveIndex != NXBasics::IntegerRange::UInt32Maximum)
		{
			void* archive = CFile::mStaticVars.fileLibraries[archiveIndex];

			const std::uint32_t entryOffset = Archive_GetFileOffset(archive, fileName);
			position += entryOffset;
		}

		// Reload the descriptor after resolving the archive offset.
		descriptor = SFileAccess::Descriptor(*fileObject);

		std::int32_t seekOffset;
		std::memcpy(&seekOffset, &position, sizeof(seekOffset));

		const std::uint32_t result = Runtime_Seek(static_cast<std::uint32_t>(descriptor), seekOffset, 0);

		if (result == NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto firstError = *Runtime_GetErrnoPointer();
			const auto errorText = NXBasics::Runtime_GetErrorText(firstError);
			const auto currentError = *Runtime_GetErrnoPointer();

			NXBasics::ReportFatalError("NXBasics::CFile::SeekToPosition\n\nfile %s - errno %d: %s", fileName, currentError, errorText);
		}
	}

	// Original function: FUN_004062ee
	inline std::uint32_t File_GetPosition(const void* file)
	{
		const auto* fileObject = static_cast<const CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{
			return 0;
		}

		std::uint32_t position = Runtime_Seek(static_cast<std::uint32_t>(descriptor), 0, 1);
		const auto* fileName = SFileAccess::FileName(*fileObject);

		if (position == NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto firstError = *Runtime_GetErrnoPointer();
			const auto errorText = NXBasics::Runtime_GetErrorText(firstError);
			const auto currentError = *Runtime_GetErrnoPointer();

			NXBasics::ReportFatalError("NXBasics::CFile::Position\n\nfile %s - errno %d: %s", fileName, currentError, errorText);
		}

		std::uint32_t archiveIndex;
		archiveIndex = static_cast<std::uint32_t>(SFileAccess::ArchiveIndex(*fileObject));

		if (archiveIndex != NXBasics::IntegerRange::UInt32Maximum)
		{
			void* archive = CFile::mStaticVars.fileLibraries[archiveIndex];

			const std::uint32_t entryOffset = Archive_GetFileOffset(archive, fileName);
			position -= entryOffset;
		}

		return position;
	}

	// Original function: FUN_0040635a
	inline std::uint32_t File_GetSize(const void* file)
	{
		const auto* fileObject = static_cast<const CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{
			return 0;
		}

		std::uint32_t archiveIndex;
		archiveIndex = static_cast<std::uint32_t>(SFileAccess::ArchiveIndex(*fileObject));

		const auto* fileName = SFileAccess::FileName(*fileObject);

		if (archiveIndex != NXBasics::IntegerRange::UInt32Maximum)
		{
			void* archive = CFile::mStaticVars.fileLibraries[archiveIndex];

			return Archive_GetFileSize(archive, fileName);
		}

		const std::uint32_t size = Runtime_GetFileSize(static_cast<std::uint32_t>(descriptor));

		if (size == NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto firstError = *Runtime_GetErrnoPointer();
			const auto errorText = NXBasics::Runtime_GetErrorText(firstError);
			const auto currentError = *Runtime_GetErrnoPointer();

			NXBasics::ReportFatalError("NXBasics::CFile::Size\n\nfile %s - errno %d: %s", fileName, currentError, errorText);
		}

		return size;
	}

	// Original function: FUN_0040665d
	// Mac symbol: NXBasics::CSimpleFileLibrary::CSimpleFileLibrary(char const*).
	inline void* Archive_Initialize(void* archive, const char* fileName)
	{
		auto* state = ::new (archive) SArchiveState{ };
		// Remaster: construct a destructible empty archive on every failure path.
		const auto fail = [state]() -> void*
		{
			for (std::uint32_t i = 0; i < state->directoryEntryCount; ++i)
			{
				Runtime_Free(state->directoryEntries[i].name);
			}

			for (std::uint32_t i = 0; i < state->entryCount; ++i)
			{
				Runtime_Free(state->entries[i].name);
			}

			Runtime_Free(state->directoryEntries);
			Runtime_Free(state->entries);

			if (state->archiveFile != nullptr)
			{
				state->archiveFile->~CFile();
				Runtime_Free(state->archiveFile);
			}

			std::memset(state, 0, sizeof(*state));
			ReportFatalError("NXBasics::CSimpleFileLibrary: invalid, truncated or unavailable archive");

			return state;
		};
		void* fileStorage = Runtime_AllocateWithNewHandler(sizeof(CFile), 1);

		if (fileStorage == nullptr)
			return fail();

		state->archiveFile = ::new (fileStorage) CFile(fileName, true);
		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*state->archiveFile);

		if (descriptor == -1)
			return fail();

		const auto archiveSize = File_GetSize(state->archiveFile);

		if (archiveSize < 12u || archiveSize == UINT32_MAX)
			return fail();

		std::uint32_t remaining = archiveSize;
		const auto read = [descriptor, &remaining](void* data, std::uint32_t bytes) -> bool
		{
			if (bytes > remaining || bytes > INT32_MAX || (bytes != 0 && Runtime_Read(static_cast<std::uint32_t>(descriptor), data, bytes) != static_cast<std::int32_t>(bytes)))
				return false;

			remaining -= bytes;

			return true;
		};
		std::uint32_t ignoredHeader = 0, directoryCount = 0, fileCount = 0;

		if (!read(&ignoredHeader, 4) || !read(&directoryCount, 4) || !read(&fileCount, 4))
			return fail();
		// Each serialized directory needs at least 8 bytes and each file at least 12.

		if (directoryCount > remaining / 8u || fileCount > (remaining - directoryCount * 8u) / 12u || directoryCount > SIZE_MAX / sizeof(SArchiveDirectoryEntry) || fileCount > SIZE_MAX / sizeof(SArchiveFileEntry))
			return fail();

		if (directoryCount != 0)
		{
			state->directoryEntries = static_cast<SArchiveDirectoryEntry*>(Runtime_AllocateArrayWithNewHandler(directoryCount, sizeof(SArchiveDirectoryEntry)));

			if (state->directoryEntries == nullptr)
				return fail();

			std::memset(state->directoryEntries, 0, directoryCount * sizeof(SArchiveDirectoryEntry));
		}

		if (fileCount != 0)
		{
			state->entries = static_cast<SArchiveFileEntry*>(Runtime_AllocateArrayWithNewHandler(fileCount, sizeof(SArchiveFileEntry)));

			if (state->entries == nullptr)
				return fail();

			std::memset(state->entries, 0, fileCount * sizeof(SArchiveFileEntry));
		}

		for (std::uint32_t i = 0; i < directoryCount; ++i)
		{
			std::uint32_t nameLength = 0;

			if (!read(&nameLength, 4) || remaining < 4 || nameLength > remaining - 4u || nameLength == UINT32_MAX)
				return fail();

			auto& entry = *::new (state->directoryEntries + i) SArchiveDirectoryEntry{ };
			entry.name = static_cast<char*>(Runtime_AllocateWithNewHandler(nameLength + 1u, 1));

			if (entry.name == nullptr)
				return fail();

			++state->directoryEntryCount;
			entry.name[nameLength] = '\0';

			if (!read(entry.name, nameLength) || !read(&entry.value, 4))
				return fail();
		}

		for (std::uint32_t i = 0; i < fileCount; ++i)
		{
			std::uint32_t nameLength = 0;

			if (!read(&nameLength, 4) || remaining < 8 || nameLength > remaining - 8u || nameLength == UINT32_MAX)
				return fail();

			auto& entry = *::new (state->entries + i) SArchiveFileEntry{ };
			entry.name = static_cast<char*>(Runtime_AllocateWithNewHandler(nameLength + 1u, 1));

			if (entry.name == nullptr)
				return fail();

			++state->entryCount;
			entry.name[nameLength] = '\0';

			if (!read(entry.name, nameLength) || !read(&entry.fileOffset, 4) || !read(&entry.fileSize, 4) || entry.fileOffset > archiveSize || entry.fileSize > archiveSize - entry.fileOffset)
				return fail();

			entry.nameCheck = Archive_CalculateNameCheck(entry.name);
		}

		return state;
	}

	// Original function: FUN_004067d7
	// Mac symbol: NXBasics::CSimpleFileLibrary::~CSimpleFileLibrary().
	inline void Archive_Destroy(void* archive)
	{
		auto* state = static_cast<SArchiveState*>(archive);

		for (std::uint32_t index = 0; index < state->entryCount; ++index)
		{
			Runtime_Free(state->entries[index].name);
		}

		Runtime_Free(state->entries);

		for (std::uint32_t index = 0; index < state->directoryEntryCount; ++index)
		{
			Runtime_Free(state->directoryEntries[index].name);
		}

		Runtime_Free(state->directoryEntries);

		CFile* archiveFile = state->archiveFile;

		if (archiveFile != nullptr)
		{
			archiveFile->~CFile();
			Runtime_Free(archiveFile);
		}

		// The caller frees the archive allocation after this function returns.
		std::memset(state, 0, sizeof(*state));
	}

	// Original function: FUN_00406841
	// Mac symbol: NXBasics::CSimpleFileLibrary::IsFileInLibrary(char const*) const.
	inline bool Archive_IsFileInLibrary(void* archive, const char* fileName)
	{
		const auto* state = static_cast<const SArchiveState*>(archive);

		if (state->entries == nullptr)
		{
			return false;
		}

		return Archive_FindFileIndex(archive, fileName) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_00406861
	inline std::uint32_t Archive_GetFileSize(void* archive, const char* fileName)
	{
		const auto* state = static_cast<const SArchiveState*>(archive);

		if (state->entries == nullptr)
		{
			return 0;
		}

		const std::uint32_t entryIndex = Archive_FindFileIndex(archive, fileName);

		if (entryIndex == NXBasics::IntegerRange::UInt32Maximum)
		{
			return 0;
		}

		return state->entries[entryIndex].fileSize;
	}

	// Original function: FUN_0040688a
	inline std::uint32_t Archive_GetFileOffset(void* archive, const char* fileName)
	{
		const auto* state = static_cast<const SArchiveState*>(archive);

		if (state->entries == nullptr)
		{
			return 0;
		}

		const std::uint32_t entryIndex = Archive_FindFileIndex(archive, fileName);

		if (entryIndex == NXBasics::IntegerRange::UInt32Maximum)
		{
			return 0;
		}

		return state->entries[entryIndex].fileOffset;
	}

	// Original function: FUN_004068b3
	inline std::uint32_t Archive_FindFileIndex(void* archive, const char* fileName)
	{
		const std::uint8_t nameCheck = Archive_CalculateNameCheck(fileName);
		const auto* state = static_cast<const SArchiveState*>(archive);

		for (std::uint32_t index = 0; index < state->entryCount; ++index)
		{
			const auto& entry = state->entries[index];

			if (entry.nameCheck != nameCheck)
			{
				continue;
			}

			if (Runtime_CompareStringsIgnoreCase(fileName, entry.name) == 0)
			{
				return index;
			}
		}

		return NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_00406903
	inline std::uint8_t Archive_CalculateNameCheck(const char* fileName)
	{
		const std::uint32_t lengthBits = static_cast<std::uint32_t>(std::strlen(fileName));

		std::int32_t length;
		std::memcpy(&length, &lengthBits, sizeof(length));

		std::uint8_t result = 0;

		for (std::int32_t index = 0; index < length; ++index)
		{
			const std::uint8_t character = static_cast<std::uint8_t>(fileName[index]);
			const std::int32_t signedCharacter = character < 0x80 ? static_cast<std::int32_t>(character) : static_cast<std::int32_t>(character) - 0x100;
			const std::uint8_t convertedCharacter = static_cast<std::uint8_t>(Runtime_ToLower(signedCharacter));

			result = static_cast<std::uint8_t>(result + convertedCharacter);
		}

		return result;
	}

	inline bool File_OpenForReading(void* file, bool reportErrors, bool searchPaths);

	inline void File_InitObject(void* file);

	// Initialization body of FUN_00405d8b; the native constructor supplies the vtable.
	inline void File_Initialize(void* file, const char* inputFileName, bool openForReading)
	{
		auto* fileObject = static_cast<CFile*>(file);
		auto& fileName = SFileAccess::FileName(*fileObject);

		File_InitObject(file);

		if (inputFileName == nullptr)
			return;

		const char* prefix = "";
		const char* suffix = inputFileName;

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			const auto& record = CFile::mStaticVars.systemPathNames[index];

			if (record.enabled == 0)
			{
				continue;
			}

			if (Runtime_CompareStringsIgnoreCaseN(inputFileName, record.systemPathName, record.nameLength) != 0)
			{
				continue;
			}

			prefix = record.replacementPath;
			suffix = inputFileName + record.nameLength;
			break;
		}
		// Remaster: reject an unrepresentable path instead of opening a truncated name.
		const auto prefixLength = std::strlen(prefix);
		const auto suffixLength = std::strlen(suffix);

		if (prefixLength >= sizeof(fileName) || suffixLength >= sizeof(fileName) - prefixLength)
		{
			ReportFatalError("NXBasics::CFile: expanded filename exceeds the supported path length");

			return;
		}

		std::memcpy(fileName, prefix, prefixLength);
		std::memcpy(fileName + prefixLength, suffix, suffixLength + 1);

		std::int32_t length = static_cast<std::int32_t>(std::strlen(fileName));

		// Remove one leading dot and backslash, including moving the terminator.

		if (length > 2 && fileName[0] == '.' && fileName[1] == '\\')
		{
			length -= 2;
			// Windows 00405E3F calls the reconstructed overlap-safe move at 004E71F0.
			Runtime_MoveMemory(fileName, fileName + 2, static_cast<std::uint32_t>(length) + 1u);
		}

		// Prefix removal can reduce the length to one or two; no slash pair is scanned then.

		if (length > 2)
		{
			// Start at index one to preserve an initial UNC backslash pair.

			for (std::int32_t index = 1; index < length - 1; ++index)
			{
				if (fileName[index] != '\\' || fileName[index + 1] != '\\')
				{
					continue;
				}

				for (std::int32_t position = index; position < length; ++position)
				{
					fileName[position] = fileName[position + 1];
				}

				--length;
				--index;
			}
		}

		if (openForReading)
		{
			File_OpenForReading(file, true, true);
		}
	}

	// Original function: FUN_00405ee5; NXBasics::CFile::OpenForReading().
	inline bool File_OpenForReading(void* file, bool reportErrors, bool searchPaths)
	{
		auto* fileObject = static_cast<CFile*>(file);
		auto* fileName = SFileAccess::FileName(*fileObject);

		const auto getDescriptor = [fileObject]() -> std::int32_t
		{
			std::int32_t descriptor;
			descriptor = SFileAccess::Descriptor(*fileObject);

			return descriptor;
		};

		const auto storeDescriptor = [fileObject](std::int32_t descriptor)
		{
			SFileAccess::Descriptor(*fileObject) = descriptor;
		};

		const auto tryOpen = [&storeDescriptor](const char* name) -> bool
		{
			const std::int32_t descriptor = Runtime_Open(name, 0x8000u, 0u);
			storeDescriptor(descriptor);

			return descriptor != -1;
		};

		const auto reportFailure = [reportErrors, fileName]() -> bool
		{
			if (reportErrors)
			{
				const auto firstError = *NXBasics::Runtime_GetErrnoPointer();
				const auto errorText = NXBasics::Runtime_GetErrorText(firstError);
				const auto currentError = *NXBasics::Runtime_GetErrnoPointer();

				NXBasics::ReportFatalError(OriginalStrings::FileOpenForReadingError, fileName, currentError, errorText);
			}

			return false;
		};

		if (getDescriptor() != -1)
		{
			File_Close(file);
		}

		if (tryOpen(fileName))
		{
			return true;
		}

		// The original checks only for a colon in the second character.

		if (std::strlen(fileName) >= 2 && fileName[1] == ':')
		{
			return reportFailure();
		}

		const auto getArchiveFileName = [](void* archive) -> const char*
		{
			const auto* state = static_cast<const SArchiveState*>(archive);
			const auto* fileObject = state->archiveFile;

			return SFileAccess::FileName(*fileObject);
		};

		char temporaryName[260];

		if (searchPaths)
		{
			std::strcpy(temporaryName, fileName);

			for (std::uint32_t index = 0; index < 5; ++index)
			{
				const auto& pathRecord = CFile::mStaticVars.additionalLoadPaths[index];

				if (pathRecord.enabled == 0)
				{
					continue;
				}

				const char* prefix = pathRecord.path;

				if (std::strlen(prefix) + std::strlen(temporaryName) >= 260u)
					continue;

				std::strcpy(fileName, prefix);
				std::strcat(fileName, temporaryName);

				if (tryOpen(fileName))
				{
					// Keep the successfully expanded name.
					return true;
				}
			}

			std::strcpy(fileName, temporaryName);
		}

		std::uint32_t selectedArchiveIndex = 0;

		for (; selectedArchiveIndex < 5; ++selectedArchiveIndex)
		{
			void* archive = CFile::mStaticVars.fileLibraries[selectedArchiveIndex];

			if (archive != nullptr && Archive_IsFileInLibrary(archive, fileName))
			{
				break;
			}
		}

		if (selectedArchiveIndex == 5)
		{
			return reportFailure();
		}

		if (!tryOpen(getArchiveFileName(CFile::mStaticVars.fileLibraries[selectedArchiveIndex])))
		{
			if (!searchPaths)
				return reportFailure();
			// Remaster: every search prefix applies to the selected archive, not a different slot.

			for (std::uint32_t index = 0; index < 5; ++index)
			{
				const auto& pathRecord = CFile::mStaticVars.additionalLoadPaths[index];

				if (pathRecord.enabled == 0)
				{
					continue;
				}

				const char* prefix = pathRecord.path;

				const auto* archiveName = getArchiveFileName(CFile::mStaticVars.fileLibraries[selectedArchiveIndex]);

				if (std::strlen(prefix) + std::strlen(archiveName) >= sizeof(temporaryName))
					continue;

				std::strcpy(temporaryName, prefix);
				std::strcat(temporaryName, archiveName);

				if (tryOpen(temporaryName))
				{
					break;
				}
			}

			if (getDescriptor() == -1)
			{
				return reportFailure();
			}
		}

		// Use the originally selected archive for the entry offset.
		const auto entryOffset = Archive_GetFileOffset(CFile::mStaticVars.fileLibraries[selectedArchiveIndex], fileName);
		File_SeekToPosition(file, entryOffset);

		SFileAccess::ArchiveIndex(*fileObject) = static_cast<std::int32_t>(selectedArchiveIndex);

		return true;
	}

	// Original function: FUN_004061bc; NXBasics::CFile::Write().
	inline void File_Write(void* file, const void* data, std::uint32_t byteCount)
	{
		const auto* fileObject = static_cast<const CFile*>(file);

		std::int32_t descriptor;
		descriptor = SFileAccess::Descriptor(*fileObject);

		if (descriptor == -1)
		{
			return;
		}

		const std::int32_t written = Runtime_Write(static_cast<std::uint32_t>(descriptor), data, byteCount);

		// The original compares all 32 bits of the result.

		if (static_cast<std::uint32_t>(written) == byteCount)
		{

			return;
		}

		const auto firstError = *NXBasics::Runtime_GetErrnoPointer();
		const auto errorText = NXBasics::Runtime_GetErrorText(firstError);

		// Read the error value again after obtaining its description.
		const auto currentError = *NXBasics::Runtime_GetErrnoPointer();
		const auto* fileName = SFileAccess::FileName(*fileObject);

		NXBasics::ReportFatalError("NXBasics::CFile::Write\n\nfile %s - errno %d: %s", fileName, currentError, errorText);
	}

	// Original function: FUN_004063c0; NXBasics::CFile::l_InitObject().
	inline void File_InitObject(void* file)
	{
		auto* fileObject = static_cast<CFile*>(file);

		std::memset(SFileAccess::FileName(*fileObject), 0, sizeof(SFileAccess::FileName(*fileObject)));

		const std::int32_t invalidIndex = -1;
		SFileAccess::Descriptor(*fileObject) = invalidIndex;
		SFileAccess::ArchiveIndex(*fileObject) = static_cast<std::int32_t>(invalidIndex);
	}

}