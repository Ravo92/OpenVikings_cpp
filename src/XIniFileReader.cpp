#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XMemory.h"
#include "XStorableIO.h"
#include "XStringArray.h"
#include "XTextEncoding.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <list>
#include <memory>
#include <type_traits>

namespace NXBasics
{
	struct SIniFileReaderDefine
	{
		char name[100];
		char replacement[200];
	};
	static_assert(sizeof(SIniFileReaderDefine) == 300);
	static_assert(offsetof(SIniFileReaderDefine, replacement) == 0x64);

	// Transient ordered definitions shared by preliminary files and recursive includes.
	// std::list owns its records; no serialized data or external list layout is involved.
	struct SIniFileReaderDefinitions
	{
		std::list<SIniFileReaderDefine> values;
	};

	bool CIniFileReader::IsLoaded() const
	{
		return mLoaded != 0;
	}

	std::int32_t CIniFileReader::GetCurrentLineType() const
	{
		return mCurrentLineType;
	}

	const char* CIniFileReader::GetCurrentName() const
	{
		return mNameBuffer;
	}

	// Original function: FUN_00424df8
	CIniFileReader::CIniFileReader(const char* fileName, const char* preliminaryFileName, bool createCompiledFile, const char* compiledExtension, ErrorCallback errorCallback)
	{
		static_assert(std::is_standard_layout_v<CIniFileReader>);
		static_assert(offsetof(CIniFileReader, mLoaded) == 0);
		static_assert(offsetof(CIniFileReader, mFileName) == 1);
		static_assert(offsetof(CIniFileReader, mLines) == 0x108);
		static_assert(offsetof(CIniFileReader, mCurrentLine) == (sizeof(void*) == 8 ? 0x110 : 0x10C));
		static_assert(offsetof(CIniFileReader, mCurrentLineType) == (sizeof(void*) == 8 ? 0x114 : 0x110));
		static_assert(offsetof(CIniFileReader, mGroupStartLine) == (sizeof(void*) == 8 ? 0x118 : 0x114));
		static_assert(offsetof(CIniFileReader, mParameterPosition) == (sizeof(void*) == 8 ? 0x120 : 0x118));
		static_assert(offsetof(CIniFileReader, mLineBuffer) == (sizeof(void*) == 8 ? 0x128 : 0x11C));
		static_assert(offsetof(CIniFileReader, mNameBuffer) == (sizeof(void*) == 8 ? 0x8F8 : 0x8EC));
		static_assert(offsetof(CIniFileReader, mStringBuffer) == (sizeof(void*) == 8 ? 0x10C8 : 0x10BC));
		static_assert(offsetof(CIniFileReader, mErrorCallback) == (sizeof(void*) == 8 ? 0x1898 : 0x188C));

		std::memset(static_cast<void*>(this), 0, sizeof(*this));
		std::strncpy(mFileName, fileName, sizeof(mFileName));

		mErrorCallback = errorCallback;

		char compiledFileName[260];

		if (!CFile::FileSystem_Tool_FileExists(fileName, true))
		{
			std::strcpy(compiledFileName, fileName);
			l_CreateCompiledFilename(compiledFileName, compiledExtension);

			if (!CFile::FileSystem_Tool_FileExists(compiledFileName, true))
			{
				return;
			}

			mLines = static_cast<CStringArray*>(XB_Storable_LoadObject(static_cast<const char*>(compiledFileName)));
			mLoaded = 1;
			return;
		}

		std::uint32_t objectId = 0;
		std::uint32_t version = 0;

		XB_Storable_GetObjectIdAndVersion(fileName, &objectId, &version);

		if (objectId == 0x3FD)
		{
			mLines = static_cast<CStringArray*>(XB_Storable_LoadObject(fileName));
			mLoaded = 1;
			return;
		}

		// The original does not test the filename-resolution result.
		CFile::FileSystem_Tool_GetRealFilename(fileName, compiledFileName);
		l_CreateCompiledFilename(compiledFileName, compiledExtension);

		mLines = new (std::nothrow) CStringArray(std::uint8_t{ 1 });

		char drive[4];
		char directory[256];
		char iniFilePath[256];

		// Derive the temporary system path from the original input filename.
		Runtime_SplitPath(fileName, drive, directory, nullptr, nullptr);
		Runtime_MakePath(iniFilePath, drive, directory, nullptr, nullptr);

		CFile::FileSystem_SystemPathName_Set("$inifilepath$", iniFilePath);

		l_LoadIniFile(fileName, preliminaryFileName);

		CFile::FileSystem_SystemPathName_Clear("$inifilepath$");

		mLoaded = 1;

		if (createCompiledFile)
		{
			mLines->Storable_Save(static_cast<const char*>(compiledFileName), false);
		}
	}

	// Original function: FUN_00424f6c
	CIniFileReader::~CIniFileReader()
	{
		delete mLines;
	}

	// Original function: FUN_00424f7e
	char* CIniFileReader::GetFirstLine()
	{
		if (mLines->mStringCount == 0)
		{
			return nullptr;
		}

		mCurrentLine = 0;
		l_CopyCurrentLineIntoBuffer();

		return mLineBuffer;
	}

	// Original function: FUN_00424fa7
	char* CIniFileReader::GetNextLine()
	{
		const std::uint32_t lineCount = mLines->mStringCount;

		if (lineCount == 0)
		{
			return nullptr;
		}

		const std::uint32_t nextIndex = mCurrentLine + std::uint32_t{ 1 };

		if (nextIndex == lineCount)
		{
			return nullptr;
		}

		if (mLines->GetStringPtr(nextIndex) == nullptr)
		{
			return nullptr;
		}

		mCurrentLine = nextIndex;
		l_CopyCurrentLineIntoBuffer();

		return mLineBuffer;
	}

	// Original function: FUN_00424fe7
	void CIniFileReader::SetCurrentLineToGroupStart()
	{
		mCurrentLine = mGroupStartLine;
		l_CopyCurrentLineIntoBuffer();
	}

	// Original function: FUN_00424ff8
	char* CIniFileReader::GetFirstGroup()
	{
		char* line = GetFirstLine();

		while (line != nullptr)
		{
			if (mCurrentLineType == 1)
			{
				mGroupStartLine = mCurrentLine;
				return line;
			}

			line = GetNextLine();
		}

		// The original leaves the previous group-start index unchanged.
		return nullptr;
	}

	// Original function: FUN_00425030
	char* CIniFileReader::GetNextGroup()
	{
		SetCurrentLineToGroupStart();

		char* line;

		while ((line = GetNextLine()) != nullptr)
		{
			if (mCurrentLineType == 1)
			{
				mGroupStartLine = mCurrentLine;
				return line;
			}
		}

		mGroupStartLine = 0;
		return nullptr;
	}

	// Original function: FUN_00425071
	char* CIniFileReader::GetFirstGroup(const char* groupName)
	{
		char* line = GetFirstGroup();

		while (line != nullptr && Runtime_CompareStringsIgnoreCase(line, groupName) != 0)
		{
			line = GetNextGroup();
		}

		return line;
	}

	// Original function: FUN_004250a4
	char* CIniFileReader::GetNextGroup(const char* groupName)
	{
		char* line = GetNextGroup();

		while (line != nullptr && Runtime_CompareStringsIgnoreCase(line, groupName) != 0)
		{
			line = GetNextGroup();
		}

		return line;
	}

	// Original function: FUN_004250d0
	bool CIniFileReader::SearchGroup(const char* groupName)
	{
		return GetFirstGroup(groupName) != nullptr;
	}

	// Original function: FUN_00425109
	bool CIniFileReader::SearchCommand(const char* commandName)
	{
		if (mLines->mStringCount == 0)
		{
			return false;
		}

		if (mCurrentLineType == 1)
		{
			if (GetNextLine() == nullptr || mCurrentLineType == 1)
			{
				return false;
			}
		}

		for (;;)
		{
			if (Runtime_CompareStringsIgnoreCase(mNameBuffer, commandName) == 0)
			{
				return true;
			}

			if (GetNextLine() == nullptr || mCurrentLineType == 1)
			{
				return false;
			}
		}
	}

	// Original function: FUN_00425169
	char* CIniFileReader::GetNextCommandLineInGroup()
	{
		char* line;

		while ((line = GetNextLine()) != nullptr)
		{
			if (mCurrentLineType == 1)
			{
				return nullptr;
			}

			if (mCurrentLineType == 2)
			{
				return line;
			}
		}

		return nullptr;
	}

	// Original function: FUN_0042518d
	std::uint32_t CIniFileReader::GetNumberOfGroups(const char* groupName)
	{
		std::uint32_t count = 0;

		for (char* line = GetFirstGroup(); line != nullptr; line = GetNextGroup())
		{
			if (Runtime_CompareStringsIgnoreCase(line, groupName) == 0)
			{
				++count;
			}
		}

		return count;
	}

	// Original function: FUN_004251c7
	std::uint32_t CIniFileReader::GetNumberOfCommandsInGroup(const char* commandName)
	{
		const std::uint32_t savedLine = mCurrentLine;

		SetCurrentLineToGroupStart();

		std::uint32_t count = 0;

		while (GetNextCommandLineInGroup() != nullptr)
		{
			if (Runtime_CompareStringsIgnoreCase(commandName, mNameBuffer) == 0)
			{
				++count;
			}
		}

		mCurrentLine = savedLine;

		// Reloading restores the line but resets its parameter position.
		l_CopyCurrentLineIntoBuffer();

		return count;
	}

	// Original function: FUN_0042522e
	char* CIniFileReader::GetString()
	{
		char* openingQuote = std::strchr(mParameterPosition, '"');

		if (openingQuote == nullptr)
		{
			return nullptr;
		}

		char* stringStart = openingQuote + 1;
		char* closingQuote = std::strchr(stringStart, '"');

		if (closingQuote == nullptr)
		{
			return nullptr;
		}

		const std::size_t stringLength = static_cast<std::size_t>(closingQuote - stringStart);

		std::strncpy(mStringBuffer, stringStart, stringLength);
		mStringBuffer[stringLength] = '\0';

		mParameterPosition = closingQuote + 1;

		return mStringBuffer;
	}

	// Original function: FUN_00425291
	std::int32_t CIniFileReader::GetInt()
	{
		char* position = mParameterPosition;

		while (*position != '\0')
		{
			if (*position == '-' || Runtime_IsDigit(*position))
			{
				break;
			}

			++position;
		}

		if (*position == '\0')
		{
			return 0;
		}

		const std::int32_t value = Runtime_StringToInteger(position);
		char* separator = std::strchr(position, ' ');

		if (separator != nullptr)
		{
			mParameterPosition = separator + 1;
		}
		else
		{
			mParameterPosition = mLineBuffer + std::strlen(mLineBuffer);
		}

		return value;
	}

	// Original function: FUN_00425309
	double CIniFileReader::GetDouble()
	{
		char* position = mParameterPosition;

		while (*position != '\0')
		{
			if (*position == '-' || *position == '.' || Runtime_IsDigit(*position))
			{
				break;
			}

			++position;
		}

		if (*position == '\0')
		{
			// The original does not update the parameter position here.
			return 0.0;
		}

		const double value = Runtime_StringToDouble(position);
		char* separator = std::strchr(position, ' ');

		if (separator != nullptr)
		{
			mParameterPosition = separator + 1;
		}
		else
		{
			mParameterPosition = mLineBuffer + std::strlen(mLineBuffer);
		}

		return value;
	}

	// Original function: FUN_0042538d
	std::int32_t CIniFileReader::GetTypeOfNextParameter()
	{
		for (;;)
		{
			const char character = *mParameterPosition;

			if (character == '\0')
			{
				return 0;
			}

			if (character == '"')
			{
				return 2;
			}

			if (character == '-' || character == '.')
			{
				return 3;
			}

			if (Runtime_IsDigit(character))
			{
				return 3;
			}

			if (character != ' ')
			{
				return 1;
			}

			++mParameterPosition;
		}
	}

	// Original function: FUN_004253e0
	void CIniFileReader::l_LoadIniFile(const char* fileName, const char* preliminaryFileName)
	{
		SIniFileReaderDefinitions definitions;

		if (preliminaryFileName != nullptr)
		{
			l_DoLoadIniFile(preliminaryFileName, definitions);
		}

		l_DoLoadIniFile(fileName, definitions);
	}

	// Original function: FUN_00425429
	void CIniFileReader::l_DoLoadIniFile(const char* fileName, SIniFileReaderDefinitions& definitions)
	{
		std::unique_ptr<CMemory> memory;

		{
			CFile file(fileName, true);
			const std::uint32_t fileSize = file.GetSize();

			memory.reset(new (std::nothrow) CMemory(fileSize + std::uint32_t{ 1 }, std::uint8_t{ 0 }));

			char* destination = reinterpret_cast<char*>(memory->mData);
			file.Read(destination, fileSize);
		}

		char* fileData = reinterpret_cast<char*>(memory->mData);

		char rawLine[2000];
		char normalizedLine[2000];
		char storedLine[2000];
		char defineName[2000];
		char includedFileName[260];

		char* cursor = fileData;
		const bool utf8 = Utf8_IsValid(fileData);
		if (std::strncmp(cursor, "\xEF\xBB\xBF", 3) == 0) cursor += 3;

		while (*cursor != '\0')
		{
			while (*cursor == '\n' || *cursor == '\r' || *cursor == ' ' || *cursor == '\t')
			{
				++cursor;
			}

			if (*cursor == '\0')
			{
				break;
			}

			char* lineStart = cursor;

			while (*cursor != '\0' && *cursor != '\n' && *cursor != '\r')
			{
				++cursor;
			}

			std::size_t lineLength = static_cast<std::size_t>(cursor - lineStart);

			if (lineLength >= 0x7CE)
			{
				lineLength = 0x7CE;
			}

			if (utf8) lineLength = Utf8_PrefixBytes(lineStart, lineLength);
			std::memcpy(rawLine, lineStart, lineLength);
			rawLine[lineLength] = '\0';

			bool insideQuotes = false;
			bool pendingSpace = false;
			char* output = normalizedLine;

			for (const char* input = rawLine; *input != '\0'; ++input)
			{
				char character = *input;

				if (character == '"')
				{
					insideQuotes = !insideQuotes;
				}

				if (!insideQuotes && (character == '=' || character == ','))
				{
					character = ' ';
				}

				if (!insideQuotes && (character == '\t' || character == ' '))
				{
					pendingSpace = true;
					continue;
				}

				if (pendingSpace)
				{
					*output++ = ' ';
				}

				*output++ = character;
				pendingSpace = false;
			}

			*output = '\0';

			char* comment = std::strstr(normalizedLine, "//");

			if (comment != nullptr)
			{
				*comment = '\0';
			}

			if (normalizedLine[0] == '\0')
			{
				continue;
			}

			if (normalizedLine[0] == '#')
			{
				if (std::strncmp(normalizedLine + 1, "include", 7) == 0)
				{
					char* openingQuote = std::strchr(normalizedLine, '"');

					if (openingQuote == nullptr)
					{
						continue;
					}

					char* includedName = openingQuote + 1;
					char* closingQuote = std::strchr(includedName, '"');

					if (closingQuote == nullptr)
					{
						continue;
					}

					const std::size_t nameLength = static_cast<std::size_t>(closingQuote - includedName);

					std::memcpy(includedFileName, includedName, nameLength);
					includedFileName[nameLength] = '\0';

					l_DoLoadIniFile(includedFileName, definitions);
					continue;
				}

				if (std::strncmp(normalizedLine + 1, "define", 6) != 0)
				{
					continue;
				}

				char* input = normalizedLine + 8;

				if (!Runtime_IsPropertyNameCharacter(*input))
				{
					continue;
				}

				SIniFileReaderDefine value{};
				auto* definition = &value;

				char* nameOutput = definition->name;
				char character;

				for (;;)
				{
					character = *input++;

					if (character == ' ' || character == '\0')
					{
						break;
					}

					*nameOutput++ = character;
				}

				*nameOutput = '\0';

				if (character != '\0')
				{
					bool replacementInsideQuotes = false;
					char* replacementOutput = definition->replacement;

					for (;;)
					{
						character = *input++;

						if (character == '\0')
						{
							break;
						}

						if (character == '"')
						{
							replacementInsideQuotes = !replacementInsideQuotes;
						}

						if (character == ' ' && !replacementInsideQuotes)
						{
							break;
						}

						*replacementOutput++ = character;
					}

					// Preserve termination even with an unmatched quote.
					*replacementOutput = '\0';
				}

				definitions.values.push_back(value);
				continue;
			}

			if (normalizedLine[0] == '[')
			{
				normalizedLine[0] = '\x01';

				char* closingBracket = std::strchr(normalizedLine + 1, ']');

				if (closingBracket == nullptr)
				{
					continue;
				}

				*closingBracket = '\0';

				mLines->InsertString(normalizedLine);
				continue;
			}

			if (!Runtime_IsLetter(normalizedLine[0]))
			{
				continue;
			}

			storedLine[0] = '\x02';

			char* input = normalizedLine;
			char* storedOutput = storedLine + 1;
			bool hasName = false;

			while (Runtime_IsPropertyNameCharacter(*input))
			{
				*storedOutput++ = *input++;
				hasName = true;
			}

			if (!hasName)
			{
				continue;
			}

			for (;;)
			{
				char character = *input++;

				if (character == '\0')
				{
					break;
				}

				if (character == ' ')
				{
					continue;
				}

				if (character == '"')
				{
					*storedOutput++ = ' ';
					*storedOutput++ = '"';

					for (;;)
					{
						character = *input++;

						if (character == '\0' || character == '"')
						{
							break;
						}

						*storedOutput++ = character;
					}

					*storedOutput++ = '"';
				}
				else if (character == '-' || character == '+' || character == '.' || Runtime_IsDigit(character))
				{
					*storedOutput++ = ' ';

					for (;;)
					{
						*storedOutput++ = character;
						character = *input++;

						if (!Runtime_IsDigit(character) && character != '.')
						{
							break;
						}

						if (character == '\0')
						{
							break;
						}
					}

					// The character after the number has already been consumed.
				}
				else if (character == '#')
				{
					char* nameOutput = defineName;

					for (;;)
					{
						character = *input++;

						if (character == ' ' || character == '\0')
						{
							break;
						}

						*nameOutput++ = character;
					}

					*nameOutput = '\0';

					SIniFileReaderDefine* matchingDefinition = nullptr;
					for (auto& value : definitions.values)
					{
						auto* definition = &value;
						if (Runtime_CompareStringsIgnoreCase(definition->name, defineName) == 0 && definition->replacement[0] != '\0')
						{
							matchingDefinition = definition;
							break;
						}
					}

					if (matchingDefinition != nullptr)
					{
						*storedOutput++ = ' ';
						std::strcpy(storedOutput, matchingDefinition->replacement);
						storedOutput += std::strlen(storedOutput);
					}
					else
					{
						ReportWarning("NXBasics::CIniFileReader::l_DoLoadIniFile  #define could not be solved - <%s> in file <%s>", defineName, fileName);
					}
				}
				else
				{
					ErrorCallback errorCallback = mErrorCallback;

					if (errorCallback == nullptr)
					{
						// Original global fallback callback: DAT_00512580
						errorCallback = sDefaultErrorCallback;
					}

					if (errorCallback != nullptr)
					{
						errorCallback(1, normalizedLine, input - 1);
					}
				}

				if (character == '\0')
				{
					break;
				}
			}

			*storedOutput = '\0';

			if (storedOutput != storedLine + 1)
			{
				mLines->InsertString(storedLine);
			}
		}

	}

	// Original function: FUN_004258eb
	void CIniFileReader::l_CopyCurrentLineIntoBuffer()
	{
		const char* storedLine = mLines->GetStringPtr(mCurrentLine);

		const std::uint8_t typeByte = static_cast<std::uint8_t>(storedLine[0]);
		mCurrentLineType = typeByte < 0x80 ? static_cast<std::int32_t>(typeByte) : static_cast<std::int32_t>(typeByte) - 0x100;

		std::strcpy(mLineBuffer, storedLine + 1);

		char* separator = std::strchr(mLineBuffer, ' ');

		if (separator == nullptr)
		{
			std::strcpy(mNameBuffer, mLineBuffer);
			mParameterPosition = mLineBuffer + std::strlen(mLineBuffer);
			return;
		}

		const std::size_t nameLength = static_cast<std::size_t>(separator - mLineBuffer);

		std::strncpy(mNameBuffer, mLineBuffer, nameLength);
		mNameBuffer[nameLength] = '\0';

		mParameterPosition = separator + 1;
	}

	// Original function: FUN_00425980
	void CIniFileReader::l_CreateCompiledFilename(char* fileName, const char* compiledExtension)
	{
		char drive[4];
		char directory[256];
		char baseName[256];

		if (compiledExtension == nullptr)
		{
			compiledExtension = ".cif";
		}

		Runtime_SplitPath(fileName, drive, directory, baseName, nullptr);
		Runtime_MakePath(fileName, drive, directory, baseName, compiledExtension);
	}
}
