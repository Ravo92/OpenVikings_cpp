#include "OriginalStrings.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFile.h"
#include "XFileAccess.h"

#include <cstdarg>
#include <cstring>
#include <new>

namespace NXBasics
{

	// Original function: FUN_00405d8b
	CFile::CFile(const char* fileName, bool openForReading)
	{
		File_Initialize(this, fileName, openForReading);
	}

	// Original function: FUN_00405eb8; dynamic CFile allocations use the reconstructed runtime heap.
	void* CFile::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CFile();
		if ((flags & 1u) != 0) Runtime_Free(result);
		return result;
	}

	// Original function: FUN_00405eda.
	CFile::~CFile()
	{
		Close();
	}

	// Original function: FUN_00405ee5
	bool CFile::OpenForReading(bool reportErrors, bool searchPaths)
	{
		return File_OpenForReading(this, reportErrors, searchPaths);
	}

	// Original function: FUN_004060ca
	bool CFile::OpenForWriting(bool reportError)
	{
		Close();

		mDescriptor = NXBasics::Runtime_Open(mFileName, 0x8301, 0x180);

		if (mDescriptor != -1)
		{
			return true;
		}

		if (reportError)
		{
			const auto errorForText = *Runtime_GetErrnoPointer();
			const auto errorText = NXBasics::Runtime_GetErrorText(errorForText);

			// Read errno again after obtaining its description.
			const auto currentError = *Runtime_GetErrnoPointer();

			NXBasics::ReportFatalError(OriginalStrings::FileOpenForWritingError, mFileName, currentError, errorText);
		}

		return false;
	}

	// Original function: FUN_00406126
	void CFile::Close()
	{
		File_Close(this);
	}

	// Original function: FUN_00406170
	void CFile::Read(void* destination, std::uint32_t byteCount)
	{
		File_Read(this, destination, byteCount);
	}

	// Original function: FUN_004061bc
	void CFile::Write(const void* data, std::uint32_t byteCount)
	{
		File_Write(this, data, byteCount);
	}

	// Original function: FUN_00406208; copy the byte verbatim without boolean normalization.
	void CFile::ReadFlag(std::uint8_t* destination)
	{
		Read(destination, 1);
	}

	// Original function: FUN_0040625C.
	void CFile::WriteTrueFlag()
	{
		const std::uint8_t value = 1;
		Write(&value, 1);
	}

	// Original function: FUN_00406271.
	void CFile::WriteFalseFlag()
	{
		const std::uint8_t value = 0;
		Write(&value, 1);
	}

	// Original function: FUN_00406216; variadic member uses the x86 cdecl convention.
	void __cdecl CFile::WriteString(const char* format, ...)
	{
		char text[1000];
		va_list arguments;
		va_start(arguments, format);
		Runtime_FormatString(text, 999, format, arguments);
		va_end(arguments);
		text[999] = '\0';
		Write(text, Runtime_StringLength(text));
	}

	// Original function: FUN_00406286
	void CFile::SeekToPosition(std::uint32_t position)
	{
		File_SeekToPosition(this, position);
	}

	// Original function: FUN_004062ee
	std::uint32_t CFile::GetPosition() const
	{
		return File_GetPosition(this);
	}

	// Original function: FUN_0040635a
	std::uint32_t CFile::GetSize() const
	{
		return File_GetSize(this);
	}

	// Original function: FUN_004063e4
	void CFile::FileSystem_Initialize()
	{
		std::memset(&mStaticVars, 0, sizeof(mStaticVars));
	}

	// Original function: FUN_004063f9
	std::int32_t CFile::FileSystem_SystemPathName_Set(const char* systemPathName, const char* replacementPath)
	{
		// Remaster: reject oversized mappings before changing an existing record.
		if (systemPathName == nullptr || replacementPath == nullptr || std::strlen(systemPathName) >= sizeof(mStaticVars.systemPathNames[0].systemPathName) || std::strlen(replacementPath) >= sizeof(mStaticVars.systemPathNames[0].replacementPath)) return -1;
		const std::int32_t existingIndex = FileSystem_SystemPathName_GetId(systemPathName);

		if (existingIndex != -1)
		{
			std::strcpy(mStaticVars.systemPathNames[existingIndex].replacementPath, replacementPath);
			return existingIndex;
		}

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			auto& record = mStaticVars.systemPathNames[index];

			if (record.enabled != 0)
			{
				continue;
			}

			// Both strings have been validated against their destination capacities.
			std::strcpy(record.systemPathName, systemPathName);
			std::strcpy(record.replacementPath, replacementPath);
			record.nameLength = static_cast<std::uint32_t>(std::strlen(systemPathName));
			record.enabled = 1;

			return static_cast<std::int32_t>(index);
		}

		return -1;
	}

	// Original function: FUN_0040648a
	bool CFile::FileSystem_SystemPathName_Clear(const char* systemPathName)
	{
		const std::int32_t index = FileSystem_SystemPathName_GetId(systemPathName);

		if (index == -1)
		{
			return false;
		}

		// The original retains the strings and cached length.
		mStaticVars.systemPathNames[index].enabled = 0;
		return true;
	}

	// Original function: FUN_004064ac
	std::int32_t CFile::FileSystem_SystemPathName_GetId(const char* systemPathName)
	{
		for (std::int32_t index = 0; index < 5; ++index)
		{
			const auto& record = mStaticVars.systemPathNames[index];

			if (record.enabled == 0)
			{
				continue;
			}

			if (Runtime_CompareStringsIgnoreCase(record.systemPathName, systemPathName) == 0)
			{
				return index;
			}
		}

		return -1;
	}

	// Original function: FUN_004064e4
	std::int32_t CFile::FileSystem_FileLibrary_Add(void* fileLibrary)
	{
		if (fileLibrary == nullptr)
		{
			return -1;
		}

		for (std::int32_t index = 0; index < 5; ++index)
		{
			if (mStaticVars.fileLibraries[index] == nullptr)
			{
				// The original does not reject duplicate registrations.
				mStaticVars.fileLibraries[index] = fileLibrary;
				return index;
			}
		}

		return -1;
	}

	// Original function: FUN_00406510
	std::int32_t CFile::FileSystem_FileLibrary_Remove(void* fileLibrary)
	{
		if (fileLibrary == nullptr)
		{
			return -1;
		}

		const auto* state = static_cast<const SArchiveState*>(fileLibrary);

		// Preserve the original prerequisite for removing a registration.
		if (state->archiveFile == nullptr)
		{
			return -1;
		}

		for (std::int32_t index = 0; index < 5; ++index)
		{
			if (mStaticVars.fileLibraries[index] == fileLibrary)
			{
				mStaticVars.fileLibraries[index] = nullptr;
				return index;
			}
		}

		return -1;
	}

	// Original function: FUN_00406542
	bool CFile::FileSystem_Tool_GetRealFilename(const char* fileName, char* destination)
	{
		CFile file(fileName, false);

		if (!File_OpenForReading(&file, false, true))
		{
			return false;
		}

		const auto* resolvedName = file.mFileName;
		std::strcpy(destination, resolvedName);

		return true;
	}

	// Original function: FUN_0040659c
	bool CFile::FileSystem_Tool_FileExists(const char* fileName, bool searchPaths)
	{
		CFile file(fileName, false);

		return File_OpenForReading(&file, false, searchPaths);
	}

	// Original function: FUN_004065e2
	void CFile::FileSystem_Tool_CreateDirectory(const char* path)
	{
		if (std::strchr(path, '\\') != nullptr)
		{
			std::uint32_t separatorNumber = 1;

			for (;;)
			{
				char partialPath[260];
				std::strcpy(partialPath, path);

				char* separator = partialPath;

				for (std::uint32_t index = 0; index < separatorNumber; ++index)
				{
					separator = std::strchr(separator + 1, '\\');

					if (separator == nullptr)
					{
						break;
					}
				}

				if (separator == nullptr)
				{
					break;
				}

				*separator = '\0';
				Runtime_CreateDirectory(partialPath);
				++separatorNumber;
			}
		}

		Runtime_CreateDirectory(path);
	}

	// Original function: FUN_004e5e48
	void CFile::operator delete(void* allocation) noexcept
	{
		Runtime_Free(allocation);
	}
}
