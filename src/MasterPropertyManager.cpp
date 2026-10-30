#include "ApplicationGlobals.h"
#include "C2E2LevelOfDetailManager.h"
#include "C2LogicIOAccess.h"
#include "C2ObjectLayout.h"
#include "DMManager.h"
#include "DSManager.h"
#include "InGameGuiManager.h"
#include "MasterPropertyManager.h"
#include "OriginalStrings.h"
#include "X2DApplication.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XStringMemory.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace NMasterPropertyManager
{
	// List construction within FUN_004e1408.
	CPropertyManager::SObjectVars::SObjectVars(std::uint8_t destroyValues) : CListBase(destroyValues != 0)
	{
	}

	// Original function: LAB_00479e17
	void CPropertyManager::SObjectVars::l_DeleteData(void* value)
	{
		// Release storage only; nested string allocations are not released here.
		NXBasics::Runtime_Free(value);
	}

	// Original object allocation/free thunks: 004E5DDD / 004E5E48.
	void* CPropertyManager::operator new(std::size_t size)
	{

		return NXBasics::Runtime_AllocateWithNewHandler(size, 1);
	}

	void* CPropertyManager::operator new(std::size_t size, const std::nothrow_t&) noexcept
	{

		return NXBasics::Runtime_AllocateWithNewHandler(size, 1);
	}

	void CPropertyManager::operator delete(void* allocation) noexcept
	{
		NXBasics::Runtime_Free(allocation);
	}

	void CPropertyManager::operator delete(void* allocation, const std::nothrow_t&) noexcept
	{
		NXBasics::Runtime_Free(allocation);
	}

	// Original function: FUN_004e1408
	CPropertyManager::CPropertyManager() : mProperties(false)
	{
		static_assert(offsetof(CPropertyManager, mState) == 0);
		static_assert(offsetof(CPropertyManager, mProperties) == sizeof(void*));
		sTheObjectPtr = this;
		mState = 0;
	}

	// Original function: FUN_004e1436
	CPropertyManager::~CPropertyManager()
	{
		while (true)
		{
			auto* property = static_cast<SProperty*>(mProperties.l_Base_GetStartElement());

			if (property == nullptr)
			{
				break;
			}

			l_Property_RemoveAndDelete(property);
		}

		sTheObjectPtr = nullptr;

		// mProperties is destroyed automatically after the destructor body.
	}

	// Original function: FUN_004e1463
	void CPropertyManager::Property_WorkOnCommandLine(const char* commandLine)
	{
		if (commandLine == nullptr)
		{
			return;
		}

		const char* cursor = commandLine;
		char propertyName[200];
		char stringValue[200];

		const auto isNameCharacter = [](char character) -> bool
		{

			return NXBasics::Runtime_IsPropertyNameCharacter(character);
		};

		for (;;)
		{
			while (*cursor != '\0' && !isNameCharacter(*cursor))
			{
				++cursor;
			}

			if (*cursor == '\0')
			{

				return;
			}

			char* nameOutput = propertyName;

			do
			{
				if (!isNameCharacter(*cursor))
				{
					break;
				}

				*nameOutput++ = *cursor++;
			} while (*cursor != '\0');

			*nameOutput = '\0';

			bool isInteger = false;
			std::int32_t integerValue = 0;
			const char* textValue = nullptr;

			if (*cursor == ':')
			{
				++cursor;

				if (*cursor != '\0')
				{
					if (*cursor == '"')
					{
						++cursor;
						char* textOutput = stringValue;

						while (*cursor != '\0' && *cursor != '"')
						{
							*textOutput++ = *cursor++;
						}

						*textOutput = '\0';
						textValue = stringValue;

						if (*cursor == '"')
						{
							++cursor;
						}
					}
					else
					{
						isInteger = true;
						integerValue = NXBasics::Runtime_StringToInteger(cursor);

						// Only a literal space terminates this input segment.
						const char* separator = std::strchr(cursor, ' ');

						if (separator != nullptr)
						{
							cursor = separator;
						}
						else
						{
							cursor += std::strlen(cursor);
						}
					}
				}
			}

			Property_Add(propertyName, isInteger, integerValue, textValue);
		}
	}

	// Original function: FUN_004e1551
	void CPropertyManager::Property_WorkOnIniFile(const char* fileName)
	{
		if (!NXBasics::CFile::FileSystem_Tool_FileExists(fileName, false))
		{

			return;
		}

		NXBasics::CIniFileReader reader(fileName, nullptr, false, nullptr, nullptr);

		if (reader.IsLoaded())
		{
			if (reader.GetFirstLine() != nullptr)
			{
				do
				{
					if (reader.GetCurrentLineType() != 1)
					{
						bool isInteger = false;
						std::int32_t integerValue = 0;
						const char* stringValue = nullptr;

						if (reader.GetTypeOfNextParameter() == 3)
						{
							isInteger = true;
							integerValue = reader.GetInt();
						}
						else if (reader.GetTypeOfNextParameter() == 2)
						{
							stringValue = reader.GetString();
						}

						// Read the name after the parameter-access calls.
						const char* propertyName = reader.GetCurrentName();
						Property_Add(propertyName, isInteger, integerValue, stringValue);
					}
				} while (reader.GetNextLine() != nullptr);
			}
		}
	}

	// Original function: FUN_004e163b
	std::int32_t CPropertyManager::Property_GetIntegerValue(const char* propertyName)
	{
		const SProperty* property = l_Property_GetPtr(propertyName);

		if (property == nullptr || property->isInteger == 0)
		{
			return 0;
		}

		return property->integerValue;
	}

	// Original function: FUN_004e1658
	const char* CPropertyManager::Property_GetStringValuePtr(const char* propertyName)
	{
		const SProperty* property = l_Property_GetPtr(propertyName);

		return property != nullptr ? property->stringValue : nullptr;
	}

	// Original function: FUN_004e166b
	bool CPropertyManager::Property_DoesExists(const char* propertyName)
	{

		return l_Property_GetPtr(propertyName) != nullptr;
	}

	// Original function: FUN_004e167d
	void CPropertyManager::Property_Add(const char* propertyName, bool isInteger, std::int32_t integerValue, const char* stringValue)
	{
		SProperty* property = l_Property_GetPtr(propertyName);

		if (property != nullptr)
		{
			// Preserve the original order: release before duplicating.
			NXBasics::Runtime_Free(property->stringValue);

			property->isInteger = static_cast<std::uint8_t>(isInteger);
			property->integerValue = integerValue;
			property->stringValue = NXBasics::StringDuplicate(stringValue);

			return;
		}

		property = static_cast<SProperty*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SProperty), 1));
		std::memset(property, 0, sizeof(*property));

		property->name = NXBasics::StringDuplicate(propertyName);
		property->isInteger = static_cast<std::uint8_t>(isInteger);
		property->integerValue = integerValue;
		property->stringValue = NXBasics::StringDuplicate(stringValue);

		mProperties.InsertAtEnd(property);
	}

	// Original function: FUN_004e16fb
	bool CPropertyManager::Property_Remove(const char* propertyName)
	{
		SProperty* property = l_Property_GetPtr(propertyName);

		if (property == nullptr)
		{
			return false;
		}

		l_Property_RemoveAndDelete(property);

		return true;
	}

	// Original function: FUN_004e178f
	void CPropertyManager::Options_Load()
	{
		char fileName[260];
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::GlobalOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());
		Property_WorkOnIniFile(fileName);
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::CallsignOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());
		Property_WorkOnIniFile(fileName);
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::GameOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());
		Property_WorkOnIniFile(fileName);
	}

	// Original function: FUN_004e1816
	void CPropertyManager::Options_Save_Global()
	{
		NXBasics::CFile::FileSystem_Tool_CreateDirectory(NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());

		char fileName[260];
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::GlobalOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());

		{
			NXBasics::CFile file(fileName, false);
			file.OpenForWriting(true);

			const std::int32_t width = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayWidth();
			Property_Add("gfx_screen_width", true, width, nullptr);
			file.WriteString("gfx_screen_width %d\r\n", width);

			const std::int32_t height = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayHeight();
			Property_Add("gfx_screen_height", true, height, nullptr);
			file.WriteString("gfx_screen_height %d\r\n", height);

			const std::int32_t depth = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth();
			Property_Add("gfx_screen_depth", true, depth, nullptr);
			file.WriteString("gfx_screen_depth %d\r\n", depth);
		}

		// The first file is closed before the callsign file is constructed.
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::CallsignOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());
		NXBasics::CFile file(fileName, false);
		file.OpenForWriting(true);
		file.WriteString("[player data]\r\n");

		if (l_Property_GetPtr("multiplayer_callsign") != nullptr)
		{
			// Preserve the second lookup and the nullable stored string.
			const SProperty* property = l_Property_GetPtr("multiplayer_callsign");
			file.WriteString("multiplayer_callsign = \"%s\"\r\n", property != nullptr ? property->stringValue : nullptr);
		}
	}

	// Original function: FUN_004e19aa
	void CPropertyManager::Options_Save_InGame()
	{
		NXBasics::CFile::FileSystem_Tool_CreateDirectory(NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());

		char fileName[260];
		NXBasics::Runtime_PrintString(fileName, OriginalStrings::GameOptionsFileNameFormat, NC2Logic::IO_Tool_GetPlayerDataDirectoryNamePtr());

		NXBasics::CFile file(fileName, false);
		file.OpenForWriting(true);

		const auto readInteger = [](const void* object, std::size_t offset) -> std::int32_t
		{
			std::int32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

			return value;
		};

		const auto readByte = [](const void* object, std::size_t offset) -> std::int32_t
		{
			std::uint8_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

			return value;
		};

		const auto writeInteger = [&file](const char* format, std::int32_t value)
		{
			file.WriteString(format, value);
		};

		const auto saveInteger = [this, &writeInteger](const char* name, const char* format, std::int32_t value)
		{
			Property_Add(name, true, value, nullptr);
			writeInteger(format, value);
		};

		saveInteger("engine_lod", "engine_lod %d\r\n", NC2E2::CLevelOfDetailManager::sLevelOfDetail);

		writeInteger("music_mode %d\r\n", Property_GetIntegerValue("music_mode"));

		if (NDSManager::CManager::sTheObjectPtr != nullptr)
		{
			saveInteger("fx_volume", "fx_volume %d\r\n", readInteger(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::MainVolume));
			saveInteger("fx_quality", "fx_quality %d\r\n", readInteger(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Quality));
		}

		writeInteger("fx_jingles_off %d\r\n", Property_GetIntegerValue("fx_jingles_off"));

		if (gDirectMusicManager != nullptr)
		{
			saveInteger("dm_volume", "dm_volume %d\r\n", NDMManager::MainVolume_Get(gDirectMusicManager));
		}

		// Original getter: FUN_004a2b18.
		saveInteger("gui_scroll_speed", "gui_scroll_speed %d\r\n", readInteger(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::ScrollSpeed));
		saveInteger("gui_main_mode", "gui_main_mode %d\r\n", readInteger(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::MainMode));
		saveInteger("gui_expert_flag", "gui_expert_flag %d\r\n", 1);
		saveInteger("gui_tooltipsoff_flag", "gui_tooltipsoff_flag %d\r\n", readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::TooltipsDisabled));
		saveInteger("gui_scroll_on_third_button", "gui_scroll_on_third_button %d\r\n", readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::ThirdButtonScroll));
		saveInteger("gui_scroll_on_border", "gui_scroll_on_border %d\r\n", readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::BorderScroll));
		saveInteger("gui_mouse_software", "gui_mouse_software %d\r\n", readByte(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::SoftwareMouse));
	}

	// Original function: FUN_004e1ca0
	void CPropertyManager::System_AddDefaultProperties()
	{
		if (l_Property_GetPtr("music_mode") == nullptr)
		{
			Property_Add("music_mode", true, 2, nullptr);
		}

		if (l_Property_GetPtr("tincat") != nullptr || l_Property_GetPtr("game_pre_stuff_skip") != nullptr)
		{
			Property_Add("intro_skip", false, 0, nullptr);
		}
		else
		{
			Property_Add("game_start_pre_picture_sequence", false, 0, nullptr);
		}
	}

	// Original function: FUN_004e171d
	SProperty* CPropertyManager::l_Property_GetPtr(const char* propertyName)
	{
		NXBasics::CListBase::Iterator iterator(mProperties);
		auto* property = static_cast<SProperty*>(iterator.First());

		while (property != nullptr)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(propertyName, property->name) == 0)
			{
				return property;
			}

			property = static_cast<SProperty*>(iterator.Next());
		}

		return nullptr;
	}

	// Original function: FUN_004e1765
	void CPropertyManager::l_Property_RemoveAndDelete(SProperty* property)
	{
		mProperties.l_Base_RemoveElement(property);

		NXBasics::Runtime_Free(property->name);
		NXBasics::Runtime_Free(property->stringValue);
		NXBasics::Runtime_Free(property);
	}

	// Original function: FUN_004e1d1d; scalar deletion is separate from the ordinary list destructor.
	void* CPropertyManager::SObjectVars::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~SObjectVars();

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(result);

		return result;
	}

	// Original function: FUN_004e1d00
	CPropertyManager::SObjectVars::~SObjectVars()
	{
		if (mDeleteObjects != 0)
		{
			DeleteAllElements();
		}

		// The base destructor runs automatically afterward.
	}
}