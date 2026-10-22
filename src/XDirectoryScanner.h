#pragma once

#include "XNumericConstants.h"

#include "SwitchIdentifiers.h"

#include "XCrtFileAccess.h"
#include "XFile.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <io.h>

namespace NXBasics
{
	// Runtime-only search state: handles use the native process width; results retain the fixed-width file contract.
	struct SDirectoryScannerState
	{
		std::uint32_t mode;                  // +0x000
		char path[260];                    // +0x004
		char extension[256];               // +0x108
		std::uint8_t directoriesOnly;        // +0x208
		std::uint8_t padding[3];             // +0x209
		std::uint32_t libraryIndex;          // +0x20C
		std::uint32_t entryIndex;            // +0x210
		std::intptr_t searchHandle;
		_finddata32_t nativeFindData;
		char currentName[260];             // +0x330
	};

	static_assert(offsetof(SDirectoryScannerState, path) == 0x004, "Unexpected scanner path offset");
	static_assert(offsetof(SDirectoryScannerState, extension) == 0x108, "Unexpected scanner extension offset");
	static_assert(offsetof(SDirectoryScannerState, directoriesOnly) == 0x208, "Unexpected scanner directory flag offset");
	static_assert(offsetof(SDirectoryScannerState, libraryIndex) == 0x20C, "Unexpected scanner library index offset");
	static_assert(offsetof(SDirectoryScannerState, entryIndex) == 0x210, "Unexpected scanner entry index offset");
	static_assert(offsetof(SDirectoryScannerState, searchHandle) == (sizeof(void*) == 8 ? 0x218 : 0x214), "Unexpected native search state offset");
	static_assert(offsetof(SDirectoryScannerState, currentName) == (sizeof(void*) == 8 ? 0x338 : 0x330), "Unexpected scanner result offset");
	static_assert(sizeof(SDirectoryScannerState) == (sizeof(void*) == 8 ? 1088 : 1076), "Unexpected scanner state size");

	inline bool DirectoryScanner_GetFirstMatchingFile(void* scanner, const char* path, const char* extension, bool directoriesOnly);
	inline bool DirectoryScanner_GetNextMatchingFile(void* scanner);
	inline bool DirectoryScanner_DosSearching_IsCurrentlyFoundEntryMatchingParameter(void* scanner);
	inline bool DirectoryScanner_LibrarySearching_GetNext(void* scanner, bool first);
	inline char* DirectoryScanner_GetCurrentNamePtr(void* scanner);
	inline void DirectoryScanner_Stop(void* scanner);

	// Original function: FUN_0043fe46
	// Mac symbol: NXBasics::CDirectoryScanner::CDirectoryScanner().
	inline void* DirectoryScanner_Initialize(void* scanner)
	{
		auto* state = ::new (scanner) SDirectoryScannerState;
		std::memset(state, 0, sizeof(*state));
		state->searchHandle = -1;
		return state;
	}

	// Original function: FUN_0043fe5d
	// Mac symbol: NXBasics::CDirectoryScanner::~CDirectoryScanner().
	inline void DirectoryScanner_Destroy(void* scanner)
	{
		DirectoryScanner_Stop(scanner);
	}

	// Original function: FUN_0043fe62
	// Mac symbol: NXBasics::CDirectoryScanner::GetFirstMatchingFile(char const*, char const*, bool).
	inline bool DirectoryScanner_GetFirstMatchingFile(void* scanner, const char* path, const char* extension, bool directoriesOnly)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		DirectoryScanner_Stop(scanner);
		// A failed native close retains ownership; do not overwrite that handle.
		if (state->mode == 2 || path == nullptr || (!directoriesOnly && extension == nullptr)) return false;
		const std::size_t pathLength = std::strlen(path);
		const auto slash = pathLength == 0 || path[pathLength - 1] != '\\' ? 1u : 0u;
		const auto extensionLength = directoriesOnly ? 2u : std::strlen(extension) + (extension[0] == '.' ? 0u : 1u);
		if (pathLength >= sizeof(state->path) - slash || extensionLength >= sizeof(state->extension) || pathLength + slash + 1u + extensionLength >= sizeof(state->path)) return false;
		std::memmove(state->path, path, pathLength + 1);

		if (pathLength == 0 || state->path[pathLength - 1] != '\\')
		{
			std::strcat(state->path, "\\");
		}

		state->directoriesOnly = static_cast<std::uint8_t>(directoriesOnly);

		if (directoriesOnly)
		{
			std::strcpy(state->extension, ".*");
		}
		else
		{
			state->extension[0] = '\0';

			if (extension[0] != '.')
			{
				std::strcat(state->extension, ".");
			}

			std::strcat(state->extension, extension);
		}

		state->mode = 1;
		return DirectoryScanner_GetNextMatchingFile(scanner);
	}

	// Original function: FUN_0043fefc
	// Mac symbol: NXBasics::CDirectoryScanner::GetNextMatchingFile().
	inline bool DirectoryScanner_GetNextMatchingFile(void* scanner)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		for (;;)
		{
			switch (state->mode)
			{
				case SwitchIds::DirectoryScanMode::FileSystemFirst:
					{
						char searchPattern[260];

						std::strcpy(searchPattern, state->path);
						std::strcat(searchPattern, "*");
						std::strcat(searchPattern, state->extension);

						// Open dependency: original CRT find-first routine.
						const auto searchHandle = Runtime_FindFirst(searchPattern, &state->nativeFindData);
						state->searchHandle = searchHandle;

						if (searchHandle == -1)
						{
							state->mode = 3;
							break;
						}

						state->mode = 2;

						if (DirectoryScanner_DosSearching_IsCurrentlyFoundEntryMatchingParameter(scanner))
						{
							return true;
						}

						break;
					}

				case SwitchIds::DirectoryScanMode::FileSystemNext:
					{
						const auto searchHandle = state->searchHandle;

						// Open dependency: original CRT find-next routine.
						const std::int32_t result = Runtime_FindNext(searchHandle, &state->nativeFindData);

						if (result != 0)
						{
							// Remaster: release the native search before archive fallback.
							if (Runtime_FindClose(searchHandle) != 0) return false;
							state->searchHandle = -1;
							state->mode = 3;
							break;
						}

						if (DirectoryScanner_DosSearching_IsCurrentlyFoundEntryMatchingParameter(scanner))
						{
							return true;
						}

						break;
					}

				case SwitchIds::DirectoryScanMode::LibraryFirst:
					if (!DirectoryScanner_LibrarySearching_GetNext(scanner, true))
					{
						state->mode = 0;
						return false;
					}

					state->mode = 4;
					return true;

				case SwitchIds::DirectoryScanMode::LibraryNext:
					if (!DirectoryScanner_LibrarySearching_GetNext(scanner, false))
					{
						state->mode = 0;
						return false;
					}

					return true;

				default:
					// Remaster: completed, stopped and invalid scanners have no next entry.
					return false;
			}
		}
	}

	// Original function: FUN_0043ffde
	// Mac symbol: NXBasics::CDirectoryScanner::Stop().
	inline void DirectoryScanner_Stop(void* scanner)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		if (state->mode == 2)
		{
			const auto searchHandle = state->searchHandle;

			if (searchHandle != -1)
			{
				if (Runtime_FindClose(searchHandle) != 0) return;
				state->searchHandle = -1;
			}
		}

		state->mode = 0;
	}

	// Original function: FUN_00440008
	// Mac symbol: NXBasics::CDirectoryScanner::GetCurrentNamePtr().
	inline char* DirectoryScanner_GetCurrentNamePtr(void* scanner)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		if (state->mode == 4)
		{
			if (state->libraryIndex >= 5) return nullptr;
			const auto* archive = static_cast<const SArchiveState*>(CFile::mStaticVars.fileLibraries[state->libraryIndex]);
			if (archive == nullptr) return nullptr;
			const char* const* nameSlot = nullptr;

			if (state->directoriesOnly != 0)
			{
				if (state->entryIndex < archive->directoryEntryCount)
				{
					nameSlot = &archive->directoryEntries[state->entryIndex].name;
				}
			}
			else if (state->entryIndex < archive->entryCount)
			{
				nameSlot = &archive->entries[state->entryIndex].name;
			}

			if (nameSlot == nullptr || *nameSlot == nullptr || std::strlen(*nameSlot) >= sizeof(state->currentName)) return nullptr;
			std::strcpy(state->currentName, *nameSlot);
			return state->currentName;
		}

		if (state->mode == 2)
		{
			// Read the named result field instead of assuming a handle-sized prefix.
			const char* nativeName = state->nativeFindData.name;
			const auto pathLength = std::strlen(state->path);
			const auto nameLength = std::strlen(nativeName);
			const bool appendSlash = state->directoriesOnly != 0 && (nameLength == 0 || nativeName[nameLength - 1] != '\\');
			if (pathLength + nameLength + (appendSlash ? 1u : 0u) >= sizeof(state->currentName)) return nullptr;

			std::strcpy(state->currentName, state->path);
			std::strcat(state->currentName, nativeName);

			if (appendSlash)
			{
				std::strcat(state->currentName, "\\");
			}

			return state->currentName;
		}

		return nullptr;
	}

	// Original function: FUN_004400c1
	// Mac symbol: NXBasics::CDirectoryScanner::l_DosSearching_IsCurrentlyFoundEntryMatchingParameter().
	inline bool DirectoryScanner_DosSearching_IsCurrentlyFoundEntryMatchingParameter(void* scanner)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		if (state->mode != 2)
		{
			return false;
		}

		char directory[260];
		char baseName[260];
		char extension[260];

		const char* currentName = DirectoryScanner_GetCurrentNamePtr(scanner);
		if (currentName == nullptr || std::strlen(currentName) >= sizeof(directory)) return false;
		Runtime_SplitPath(currentName, nullptr, directory, baseName, extension);

		const auto attributes = state->nativeFindData.attrib;

		if ((attributes & NXBasics::BitMask::Bit4) != 0 && extension[0] == '.')
		{
			return false;
		}

		return state->directoriesOnly == static_cast<std::uint8_t>((attributes >> 4) & 1u);
	}

	// Original function: FUN_00440124
	// Mac symbol: NXBasics::CDirectoryScanner::l_LibrarySearching_GetNext(bool).
	inline bool DirectoryScanner_LibrarySearching_GetNext(void* scanner, bool first)
	{
		auto* state = static_cast<SDirectoryScannerState*>(scanner);

		if (first)
		{
			state->libraryIndex = 0;
			state->entryIndex = 0;
		}
		else
		{
			++state->entryIndex;
		}

		for (; state->libraryIndex < 5;)
		{
			const auto* archive = static_cast<const SArchiveState*>(CFile::mStaticVars.fileLibraries[state->libraryIndex]);

			if (archive != nullptr)
			{
				if (state->directoriesOnly != 0)
				{
					if (state->entryIndex < archive->directoryEntryCount)
					{
						char parentPath[260];
						const auto* name = archive->directoryEntries[state->entryIndex].name;
						if (name == nullptr || std::strlen(name) >= sizeof(parentPath)) { ++state->entryIndex; continue; }
						std::strcpy(parentPath, archive->directoryEntries[state->entryIndex].name);

						const std::uint32_t positionBits = static_cast<std::uint32_t>(std::strlen(parentPath)) - 2u;
						std::int32_t position;
						std::memcpy(&position, &positionBits, sizeof(position));

						while (position >= 0 && parentPath[position] != '\\')
						{
							--position;
						}

						if (position >= 0)
						{
							parentPath[position + 1] = '\0';

							if (Runtime_CompareStringsIgnoreCase(parentPath, state->path) == 0)
							{
								return true;
							}
						}

						++state->entryIndex;
						continue;
					}
				}
				else if (state->entryIndex < archive->entryCount)
				{
					char directory[260];
					char baseName[260];
					char extension[260];
					const auto* name = archive->entries[state->entryIndex].name;
					if (name == nullptr || std::strlen(name) >= sizeof(directory)) { ++state->entryIndex; continue; }

					// Open dependency: original path splitting routine.
					Runtime_SplitPath(archive->entries[state->entryIndex].name, nullptr, directory, baseName, extension);

					if (Runtime_CompareStringsIgnoreCase(extension, state->extension) == 0)
					{
						if (Runtime_CompareStringsIgnoreCase(directory, state->path) == 0)
						{
							return true;
						}
					}

					++state->entryIndex;
					continue;
				}
			}

			++state->libraryIndex;

			if (state->libraryIndex == 5)
			{
				return false;
			}

			state->entryIndex = 0;
		}
		return false;
	}
	// Original function: FUN_00440240; count every matching entry, then stop the local search.
	inline std::int32_t DirectoryScanner_GetNumberOfMatchingFiles(const char* path, const char* extension, bool directoriesOnly)
	{
		SDirectoryScannerState scanner;
		Runtime_FillMemory(&scanner, 0, sizeof(scanner));
		std::uint32_t count = 0;
		for (bool found = DirectoryScanner_GetFirstMatchingFile(&scanner, path, extension, directoriesOnly); found; found = DirectoryScanner_GetNextMatchingFile(&scanner)) ++count;
		DirectoryScanner_Stop(&scanner);
		return Runtime_TimeSigned32(count);
	}
}
