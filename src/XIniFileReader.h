#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	struct SIniFileReaderDefinitions;
	class CStringArray;

	class CIniFileReader
	{
	public:
		using ErrorCallback = void(__cdecl*)(std::int32_t, const char*, const char*);

		// Original data: 00512580. Semantic name; no Mac symbol was identified.
		inline static ErrorCallback sDefaultErrorCallback = nullptr;

		// Original function: FUN_00424df8
		CIniFileReader(const char* fileName, const char* preliminaryFileName, bool createCompiledFile, const char* compiledExtension, ErrorCallback errorCallback);

		// Original function: FUN_00424f6c
		~CIniFileReader();

		CIniFileReader(const CIniFileReader&) = delete;
		CIniFileReader& operator=(const CIniFileReader&) = delete;

		// Original function: FUN_00424f7e
		char* GetFirstLine();

		// Original function: FUN_00424fa7
		char* GetNextLine();

		// Original function: FUN_00424fe7
		void SetCurrentLineToGroupStart();

		// Original function: FUN_00424ff8
		char* GetFirstGroup();

		// Original function: FUN_00425030
		char* GetNextGroup();

		// Original function: FUN_00425071
		char* GetFirstGroup(const char* groupName);

		// Original function: FUN_004250a4
		char* GetNextGroup(const char* groupName);

		// Original function: FUN_004250d0
		bool SearchGroup(const char* groupName);

		// Original function: FUN_00425109
		bool SearchCommand(const char* commandName);

		// Original function: FUN_00425169
		char* GetNextCommandLineInGroup();

		// Original function: FUN_0042518d
		std::uint32_t GetNumberOfGroups(const char* groupName);

		// Original function: FUN_004251c7
		std::uint32_t GetNumberOfCommandsInGroup(const char* commandName);

		// Original function: FUN_0042522e
		char* GetString();

		// Original function: FUN_00425291
		std::int32_t GetInt();

		// Original function: FUN_00425309
		double GetDouble();

		// Original function: FUN_0042538d
		std::int32_t GetTypeOfNextParameter();

		// Field accessors for the property-manager path.
		bool IsLoaded() const;
		std::int32_t GetCurrentLineType() const;
		const char* GetCurrentName() const;

	private:
		// Original function: FUN_004253e0
		void l_LoadIniFile(const char* fileName, const char* preliminaryFileName);

		// Original function: FUN_00425429
		void l_DoLoadIniFile(const char* fileName, SIniFileReaderDefinitions& definitions);

		// Original function: FUN_004258eb
		void l_CopyCurrentLineIntoBuffer();

		// Original function: FUN_00425980
		void l_CreateCompiledFilename(char* fileName, const char* compiledExtension);

		std::uint8_t mLoaded;           // +0x000
		char mFileName[260];          // +0x001
		std::byte mPadding105[3];       // +0x105
		CStringArray* mLines;           // +0x108
		std::uint32_t mCurrentLine;     // +0x10C
		std::int32_t mCurrentLineType;  // +0x110
		std::uint32_t mGroupStartLine;  // +0x114
		char* mParameterPosition;      // +0x118
		char mLineBuffer[2000];         // +0x11C
		char mNameBuffer[2000];         // +0x8EC
		char mStringBuffer[2000];       // +0x10BC
		ErrorCallback mErrorCallback;  // +0x188C
	};

	static_assert(sizeof(CIniFileReader) == (sizeof(void*) == 8 ? 6304 : 6288), "Unexpected native INI reader layout");
	static_assert(alignof(CIniFileReader) == alignof(void*), "Unexpected native INI reader alignment");
}
