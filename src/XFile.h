#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CFile;

	struct SArchiveFileEntry
	{
		char* name;               // +0x00
		std::uint8_t nameCheck;   // +0x04
		std::uint8_t padding[3];  // +0x05
		std::uint32_t fileOffset; // +0x08
		std::uint32_t fileSize;   // +0x0C
	};

	struct SArchiveDirectoryEntry
	{
		char* name;          // +0x00
		std::uint32_t value; // +0x04; meaning not established
	};

	struct SArchiveState
	{
		std::uint32_t entryCount;                 // +0x00
		SArchiveFileEntry* entries;               // +0x04
		std::uint32_t directoryEntryCount;        // +0x08
		SArchiveDirectoryEntry* directoryEntries; // +0x0C
		CFile* archiveFile;                       // +0x10
	};

	static_assert(sizeof(SArchiveFileEntry) == (sizeof(void*) == 8 ? 24 : 16), "Unexpected archive file entry size");
	static_assert(offsetof(SArchiveFileEntry, nameCheck) == sizeof(void*), "Unexpected name check offset");
	static_assert(offsetof(SArchiveFileEntry, fileOffset) == sizeof(void*) + 4, "Unexpected file offset field");
	static_assert(offsetof(SArchiveFileEntry, fileSize) == sizeof(void*) + 8, "Unexpected file size field");
	static_assert(sizeof(SArchiveDirectoryEntry) == sizeof(void*) * 2, "Unexpected secondary entry size");
	static_assert(offsetof(SArchiveDirectoryEntry, value) == sizeof(void*), "Unexpected directory value offset");
	static_assert(sizeof(SArchiveState) == sizeof(void*) * 5, "Unexpected native archive state layout");
	static_assert(offsetof(SArchiveState, entries) == sizeof(void*), "Unexpected archive entries offset");
	static_assert(offsetof(SArchiveState, directoryEntryCount) == sizeof(void*) * 2, "Unexpected directory count offset");
	static_assert(offsetof(SArchiveState, directoryEntries) == sizeof(void*) * 3, "Unexpected directory table offset");
	static_assert(offsetof(SArchiveState, archiveFile) == sizeof(void*) * 4, "Unexpected archive file offset");

	struct SFileSystemPathName
	{
		std::uint8_t enabled;      // +0x000
		char systemPathName[260];  // +0x001
		std::uint8_t padding[3];   // +0x105
		std::uint32_t nameLength;  // +0x108
		char replacementPath[260]; // +0x10C
	};

	struct SFileAdditionalLoadPath
	{
		std::uint8_t enabled; // +0x000
		char path[260];       // +0x001
	};

	struct SFileStaticVars
	{
		SFileSystemPathName systemPathNames[5];         // +0x000
		SFileAdditionalLoadPath additionalLoadPaths[5]; // +0xA50
		std::uint8_t padding[3];                        // +0xF69
		void* fileLibraries[5];                         // +0xF6C
	};

	static_assert(sizeof(SFileSystemPathName) == 528, "Unexpected system path record size");
	static_assert(offsetof(SFileSystemPathName, systemPathName) == 1, "Unexpected system path name offset");
	static_assert(offsetof(SFileSystemPathName, nameLength) == 0x108, "Unexpected system path length offset");
	static_assert(offsetof(SFileSystemPathName, replacementPath) == 0x10C, "Unexpected replacement path offset");
	static_assert(sizeof(SFileAdditionalLoadPath) == 261, "Unexpected additional load path size");
	static_assert(offsetof(SFileStaticVars, additionalLoadPaths) == 0xA50, "Unexpected additional load paths offset");
	static_assert(offsetof(SFileStaticVars, fileLibraries) == (sizeof(void*) == 8 ? 0xF70 : 0xF6C), "Unexpected file libraries offset");
	static_assert(sizeof(SFileStaticVars) == (sizeof(void*) == 8 ? 3992 : 3968), "Unexpected native file system state layout");

	class CFile
	{
	  public:
		// Original function: FUN_00405d8b
		CFile(const char* fileName, bool openForReading);
		CFile(const CFile&) = delete;
		CFile& operator=(const CFile&) = delete;

		// Original function: FUN_00405eda; deleting wrapper: FUN_00405eb8.
		virtual ~CFile();
		void* Delete(std::uint32_t flags);

		// Original function: FUN_00405ee5
		bool OpenForReading(bool reportErrors, bool searchPaths);

		// Original function: FUN_004060ca
		bool OpenForWriting(bool reportError);

		// Original function: FUN_00406126
		void Close();

		// Original function: FUN_00406170
		void Read(void* destination, std::uint32_t byteCount);

		// Original function: FUN_004061bc
		void Write(const void* data, std::uint32_t byteCount);

		void ReadFlag(std::uint8_t* destination);
		void WriteTrueFlag();
		void WriteFalseFlag();

		void __cdecl WriteString(const char* format, ...);

		// Original function: FUN_00406286
		void SeekToPosition(std::uint32_t position);

		// Original function: FUN_004062ee
		std::uint32_t GetPosition() const;

		// Original function: FUN_0040635a
		std::uint32_t GetSize() const;

		static void operator delete(void* allocation) noexcept;

		// Original data: 0050f758; Mac symbol: NXBasics::CFile::mStaticVars.
		inline static SFileStaticVars mStaticVars{ };

		// Original function: FUN_004063e4
		static void FileSystem_Initialize();

		// Original function: FUN_004063f9
		static std::int32_t FileSystem_SystemPathName_Set(const char* systemPathName, const char* replacementPath);

		// Original function: FUN_0040648a
		static bool FileSystem_SystemPathName_Clear(const char* systemPathName);

		// Original function: FUN_004064ac
		static std::int32_t FileSystem_SystemPathName_GetId(const char* systemPathName);

		// Original function: FUN_004064e4
		static std::int32_t FileSystem_FileLibrary_Add(void* fileLibrary);

		// Original function: FUN_00406510
		static std::int32_t FileSystem_FileLibrary_Remove(void* fileLibrary);

		// Original function: FUN_00406542
		static bool FileSystem_Tool_GetRealFilename(const char* fileName, char* destination);

		// Original function: FUN_0040659c
		static bool FileSystem_Tool_FileExists(const char* fileName, bool searchPaths);

		static void FileSystem_Tool_CreateDirectory(const char* path);

	  private:
		friend struct SFileAccess;

		std::int32_t mDescriptor;   // +0x004
		char mFileName[260];        // +0x008
		std::int32_t mArchiveIndex; // +0x10C
	};

	static_assert(sizeof(CFile) == (sizeof(void*) == 8 ? 280 : 272), "Unexpected native CFile size");
}