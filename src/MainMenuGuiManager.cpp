#include <algorithm>
#include "XTextEncoding.h"
#include "C2CallbackManager.h"
#include "XTextEncoding.h"
#include "C2CleanMapsManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameEngineStartup.h"
#include "C2HumanIterator.h"
#include "C2LanguageTools.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicIOAccess.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2SavegamesManager.h"
#include "CDAudioManager.h"
#include "FileCompressor.h"
#include "FileTransferManager.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "NetworkShutdown.h"
#include "OriginalStrings.h"
#include "StringInputHandler.h"
#include "SwitchIdentifiers.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XIniFileReader.h"
#include "XMemory.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XPictureAccess.h"
#include "XStorableIO.h"


#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <windows.h>

// Original constant: 004f5254; bytes 00 00 40 41 in Game.exe.
static constexpr float cMenuGameSpeed = 12.0f;

// Original constant: 004f7e48; matches CLSID_DP8SP_TCPIP.
static constexpr GUID cTcpIpServiceProviderGuid = { 0xEBFE7BA0u, 0x628Du, 0x11D2u,{ 0xAE, 0x0F, 0, 0x60, 0x97, 0xB0, 0x14, 0x11 } };

// Original data: DAT_005694e0.
std::uint32_t gLastPlayerListUpdateTick = 0;

// Original data: DAT_005694e4.
std::uint32_t gLastHostMessage6Tick = 0;

namespace
{
	void DestroyMainMenuStorableResource(void* object)
	{
		if (object == nullptr)
			return;

		using TDeletingDestructor = void* (__thiscall*)(void*, unsigned int);

		TDeletingDestructor* methods;
		std::memcpy(&methods, object, sizeof(methods));
		methods[3](object, 1u);
	}
}

namespace NC2MainMenuGuiManager
{
	// Original callback target: 0042c8fd
	bool __cdecl CGuiManager::ls_Logic_CallbackFunction(std::uint32_t /*type*/, std::uint32_t /*userData*/, std::uint32_t /*param1*/, std::uint32_t /*param2*/, std::uint32_t /*param3*/)
	{
		return true;
	}

	// Original function: FUN_004d2b2d
	CGuiManager::CGuiManager() : textInput((GuiObjectVars_Construct(reinterpret_cast<std::byte*>(this) + 0x04), DefaultMessageHandler_Construct(defaultMessageHandlerStorage), NC2InGameGuiBaseTextInputElement::CStringInputHandler()))
	{
		sTheObjectPtr = this;

		std::memset(static_cast<void*>(this), 0, offsetof(CGuiManager, defaultMessageHandlerStorage));
		mStaticVars = { };

		ls_GraphicData_Load();
	}

	// Original function: FUN_004d2ba8
	CGuiManager::~CGuiManager()
	{
		ls_GraphicData_Free();
		sTheObjectPtr = nullptr;

		textInput.Exit();
		DefaultMessageHandler_Destruct(defaultMessageHandlerStorage);
		GuiObjectVars_Destruct(this);
	}

	// Original function: FUN_004d2bf2
	void CGuiManager::System_UpdateGui()
	{
		const auto& guiResources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		const auto readWord = [] (const void* object, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
			return value;
		};

		const auto writeWord = [] (void* object, std::size_t offset, std::uint32_t value)
		{
			std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
		};

		const auto readPointer = [] (const void* object, std::size_t offset) -> void*
		{
			void* value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
			return value;
		};

		// Dispatch registered elements before idle camera updates.
		l_ScreenGuiElement_FrameCall_Do();

		if (readWord(this, 0x58) == 0)
			writeWord(this, 0x58, timeGetTime());

		const std::uint32_t mouseActivity = readWord(NXSysMouseManager::CMouseManager::sTheObjectPtr, 0x24);
		const std::uint32_t keyboardActivity = readWord(NXSysKeyManager::CKeyManager::sTheObjectPtr, 4);
		const std::uint32_t latestInput = mouseActivity > keyboardActivity ? mouseActivity : keyboardActivity;
		const std::uint32_t previousActivity = readWord(this, 0x58);

		writeWord(this, 0x58, previousActivity > latestInput ? previousActivity : latestInput);

		// Preserve the original time-query sequence.
		timeGetTime();

		if (readWord(this, 0x60) == 0)
			writeWord(this, 0x60, timeGetTime());

		const std::uint32_t elapsedSeconds = (timeGetTime() - readWord(this, 0x60)) / 1000u;
		const std::uint32_t intervalSeconds = readWord(this, 0x64) != 0 ? 30u : 0u;

		if (elapsedSeconds > intervalSeconds)
		{
			writeWord(this, 0x60, timeGetTime());
			timeGetTime();

			// Storage covering the original CHumanIterator stack object.
			NC2Logic::CHumanIterator iterator;
			std::int32_t candidates[40];
			std::uint32_t candidateCount = 0;


			std::int32_t humanId = iterator.GetFirstHumanId();

			while (humanId != -1)
			{
				const std::uint32_t humanOffset = static_cast<std::uint32_t>(humanId) * 0xC58u;

				if (readWord(NC2Logic::CHumanArray::mStaticVars, humanOffset + 0xB80u) == 10000u)
				{
					candidates[candidateCount++] = humanId;

					if (candidateCount == 40u)
						break;
				}

				humanId = iterator.GetNextHumanId();
			}

			if (candidateCount != 0)
			{
				const std::int32_t selectedHuman = candidates[timeGetTime() % candidateCount];

				if (selectedHuman != -1)
					NC2MainMenuGuiWorldElement::WorldDisplayElement_FollowHuman(readPointer(this, 4), selectedHuman);
			}

			writeWord(this, 0x64, readWord(this, 0x64) + 1u);
		}

		l_Network_Update();

		if (readPointer(this, 0x30) != nullptr && !Network_PlayerListHasChanged())
		{
			const std::uint32_t currentTime = timeGetTime();

			// Preserve the original unsigned comparison and addition.
			if (currentTime >= gLastPlayerListUpdateTick + 500u)
			{
				gLastPlayerListUpdateTick = currentTime;

				GetNetworkInstance();

				for (std::uint32_t index = 0; index < 10; ++index)
				{
					if (readPointer(this, 0x30 + index * 4u) != nullptr)
					{
						char text[168];

						Network_FormatPlayerDescription(index, text);
						NC2InGameGuiBase::BaseToolTextButton_SetCaptionString(readPointer(this, 0x30 + index * 4u), text);
					}
				}
			}
		}

		if (NNetwork::Network_IsHost() && currentScreen == 14)
		{
			const std::uint32_t currentTime = timeGetTime();

			if (currentTime >= gLastHostMessage6Tick + 1000u)
			{
				gLastHostMessage6Tick = currentTime;

				std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
				message[0] = 6;

				NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message, sizeof(message));
			}
		}

		// Complete desktop input, close-list and drawing before network overlays.
		NXBaseGui::CDesktop::sTheObjectPtr->Desktop_DoWork(false, mStaticVars.desktopUpdateFlag != 0);
		mStaticVars.desktopUpdateFlag = 0;

		l_Network_Draw();

		const auto* objectBytes = reinterpret_cast<const std::byte*>(this);

		if (currentScreen == 1 && objectBytes[NC2Layout::MainMenuManagerOffset::SuppressPreviewDarkening] == std::byte{ 0 })
		{
			char text[348];

			// Preserve the original localization lookup order.
			const char* suffix = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0x1B);
			const char* versionLabel = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0x1A);
			const char* title = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0x19);

			NXBasics::Runtime_PrintString(text, "%s - %s %d.%2.2d - %s Nov 29 2003 17:37:45", title, versionLabel, 1, 0, suffix);

			auto* bitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
			auto* font = guiResources.font08;
			const auto* colour = reinterpret_cast<const std::byte*>(guiResources.fontWhitePalette) + 0x400;

			const std::uint32_t textHeight = static_cast<std::uint32_t>(NXBasics::Font_GetPixelHeight(font, text));
			const std::uint32_t y = readWord(bitmap, 0x14) - textHeight - 1u;

			const std::uint32_t textWidth = static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, text));
			const std::uint32_t x = readWord(bitmap, 0x10) - textWidth - 1u;

			reinterpret_cast<NXBasics::CBitmap*>(bitmap)->Text_Print(font, reinterpret_cast<const NXBasics::SColorRGB*>(colour), NXBasics::Runtime_TimeSigned32(x), NXBasics::Runtime_TimeSigned32(y), text);
		}
	}

	// Original function: FUN_004d2e77
	void CGuiManager::Desktop_Open(std::int32_t width, std::int32_t height, std::uint32_t depth)
	{
		if (mStaticVars.displayInitialized != 0)
			return;

		// Remaster: share the in-game GUI scale while retaining a native background.
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
		presentation->ConfigureHud(width, height);
		mStaticVars.displayWidth = width;
		mStaticVars.displayHeight = height;
		mStaticVars.displayDepth = depth;

		const std::int32_t bitmapDepth = NXBasics::ConvertBitsToTDepth(NXBasics::Runtime_TimeSigned32(depth));
		void* desktopAllocation = NXBasics::Runtime_AllocateWithNewHandler(316, 1);
		if (desktopAllocation != nullptr) new (desktopAllocation) NXBaseGui::CDesktop(width, height, bitmapDepth);
		if (presentation->GetWorldComposition() != nullptr) presentation->SetHudBitmap(NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap());

		NC2GuiToolsBase::CGuiBaseDataManager::DynamicData_Load();

		void* primaryMessageHandler = NXBasics::Runtime_AllocateWithNewHandler(44, 1);

		if (primaryMessageHandler != nullptr)
			primaryMessageHandler = PrimaryMessageHandler_Construct(primaryMessageHandler);

		NXBaseGui::CDesktop::sTheObjectPtr->PrimaryMessageHandler_Set(static_cast<NXBaseGui::CBaseElement*>(primaryMessageHandler));

		void* mousePointer = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiManagerMousePointer, 1);

		if (mousePointer != nullptr)
			mousePointer = MousePointer_Construct(mousePointer);

		NXBaseGui::CDesktop::sTheObjectPtr->SetMousePointerStorage(mousePointer);

		NC2Logic::CCallbackManager::sTheObjectPtr->RegisterCallback(1u, &CGuiManager::ls_Logic_CallbackFunction, 0u, 10);

		NC2E2::Inform_DesktopXTructed(width, height, depth);

		void* worldElement = NXBasics::Runtime_AllocateWithNewHandler(88, 1);

		if (worldElement != nullptr)
		{
			NXBasics::SRectangle rectangle(0, 0, width, height);
			worldElement = NC2MainMenuGuiWorldElement::WorldDisplayElement_Construct(worldElement, &rectangle, true);
		}

		// Original member offset: +0x04.
		std::memcpy(&worldDisplayElement, &worldElement, sizeof(worldElement));

		NXBaseGui::CDesktop::sTheObjectPtr->Element_Add(static_cast<NXBaseGui::CBaseElement*>(worldElement), true);

		mStaticVars.displayInitialized = 1;
		mStaticVars.desktopUpdateFlag = 1;

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("multiplayer_auto_host"))
		{
			Screen_ChangeTo(19, false);
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("multiplayer_auto_host");
		}
		else if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("multiplayer_auto_join"))
		{
			Screen_ChangeTo(20, false);
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("multiplayer_auto_join");
		}
		else
		{
			Screen_ChangeTo(1, false);
		}
	}

	// Original function: FUN_004d2fdc
	void CGuiManager::Desktop_Close()
	{
		if (mStaticVars.displayInitialized == 0)
			return;

		l_ScreenGuiElement_RemoveAll();

		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;

		if (desktop != nullptr)
		{
			desktop->~CDesktop();
			NXBasics::Runtime_Free(desktop);
		}

		// Clear only the original desktop-related instance range.
		std::memset(reinterpret_cast<std::byte*>(this) + offsetof(CGuiManager, worldDisplayElement), 0, 84);

		NC2E2::Inform_DesktopXTructed(0, 0, 0);

		NC2Logic::CCallbackManager::sTheObjectPtr->UnRegisterCallback(1u, &CGuiManager::ls_Logic_CallbackFunction, 0u);

		NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay()->DisableHud();
		mStaticVars.displayInitialized = 0;
	}

	// Original function: FUN_004d303c
	void CGuiManager::ls_GraphicData_Load()
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2GuiToolsBase::CGuiBaseStringsManager), 1);
		if (allocation != nullptr) new (allocation) NC2GuiToolsBase::CGuiBaseStringsManager();
		allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2GuiToolsBase::CGuiBaseDataManager), 1);
		if (allocation != nullptr) new (allocation) NC2GuiToolsBase::CGuiBaseDataManager();

		auto& resources = mStaticVars.resources;
		char fileName[260];

		NC2Logic::LanguageTool_GetFilename("data\\gui\\lang\\%s\\bobs\\LS_menu_logos.bmd", fileName, false);
		resources.menuLogos = reinterpret_cast<NXBasics::CBobManager*>(NXBasics::XB_Storable_LoadObject(fileName));

		NC2Logic::LanguageTool_GetFilename("data\\gui\\lang\\%s\\bobs\\LS_menu_logos_s.bmd", fileName, false);
		resources.smallMenuLogos = reinterpret_cast<NXBasics::CBobManager*>(NXBasics::XB_Storable_LoadObject(fileName));

		resources.funaticsPalette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\engine2d\\bin\\palettes\\gui\\menu_funatics.pcx");
		resources.campaignMapPalette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\campaignmap.pcx");
		resources.campaignButtonsPalette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\campaignbuttons.pcx");
	}

	// Original function: FUN_004d30e5
	void CGuiManager::ls_GraphicData_Free()
	{
		auto& resources = mStaticVars.resources;

		DestroyMainMenuStorableResource(resources.campaignButtonsPalette);
		DestroyMainMenuStorableResource(resources.campaignMapPalette);
		DestroyMainMenuStorableResource(resources.funaticsPalette);
		DestroyMainMenuStorableResource(resources.menuLogos);
		DestroyMainMenuStorableResource(resources.smallMenuLogos);

		resources = { };

		auto* guiResourceManager = NC2GuiToolsBase::CGuiBaseDataManager::sTheObjectPtr;

		if (guiResourceManager != nullptr)
		{
			guiResourceManager->~CGuiBaseDataManager(); NXBasics::Runtime_Free(guiResourceManager);
		}

		auto* textResourceManager = NC2GuiToolsBase::CGuiBaseStringsManager::sTheObjectPtr;

		if (textResourceManager != nullptr)
		{
			textResourceManager->~CGuiBaseStringsManager(); NXBasics::Runtime_Free(textResourceManager);
		}
	}

	// Original function: FUN_004d31bc
	bool CGuiManager::StringInput_HandleKeys(std::uint32_t key, std::uint32_t character)
	{
		if (textInput.mEditing == 0)
			return false;

		if (textInput.WorkOnKey(key, character) != 3)
			return true;

		if (currentScreen == 9 || currentScreen == 12)
		{
			using MessageHandler = std::uint32_t(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

			const std::byte* vtable;
			std::memcpy(&vtable, defaultMessageHandlerStorage, sizeof(vtable));

			MessageHandler handleMessage;
			std::memcpy(&handleMessage, vtable + 8, sizeof(handleMessage));

			handleMessage(defaultMessageHandlerStorage, 0x800u, currentScreen == 9 ? 0x13A1u : 0x13A5u, 0, 0, 0);
		}
		else if (currentScreen == 14 || currentScreen == 16 || currentScreen == 17)
		{
			if (std::strlen(textInput.GetString()) != 0)
			{
				char formattedText[128];

				NXBasics::Runtime_PrintStringN(formattedText, 127, "%s> %s", NNetwork::Network_GetCallsign(), textInput.GetString());

				wchar_t message[128]{ };
				formattedText[127] = '\0';

				NNetwork::CopyNetworkUtf8ToWide(message + 1, formattedText, 127);

				message[0] = 1;
				NNetwork::SendDynamicNetworkChatMessage(&gDynamicNetworkData, 0, message);
			}

			textInput.SetString("");
		}

		return true;
	}

	// Original function: FUN_004d3308
	void CGuiManager::Debug_ErrorString_Add(const char* text)
	{
		char message[300];

		NXBasics::Runtime_PrintString(message, "PROBLEM: <%s>!", text);
		WriteNetworkLog(GetNetworkLog(), message, 0);

		if (errorCount != 0)
		{
			const std::uint32_t movedCount = errorCount == 10u ? 9u : errorCount;

			std::memmove(errorTexts[1], errorTexts[0], movedCount * 202u);
		}

		NXBasics::Text_Copy(errorTexts[0], 0xC9, text);
		errorTexts[0][0xC8] = '\0';

		if (errorCount != 10u)
			++errorCount;
	}

	// Original function: FUN_004d33c6
	void CGuiManager::Screen_ChangeTo(std::int32_t screen, bool shutDownNetwork)
	{
		if (screen == currentScreen)
			return;

		l_ScreenData_Exit(shutDownNetwork);
		l_ScreenData_Init(screen);

		// Rebuild elements for the newly initialized screen.
		Screen_UpdateGui();
	}

	// Original function: FUN_004d33ef
	void CGuiManager::l_ScreenData_Exit(bool shutDownNetwork)
	{
		NC2Logic::GameTime_SetSpeed(0.0f);

		switch (currentScreen)
		{
			case SwitchIds::MainMenuScreen::EnterCallsign:
				if (shutDownNetwork)
				{
					NNetwork::Network_ShutDown();
				}
				else
				{
					auto* configuration = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;
					const auto callsign = Network_GetCallsign();

					configuration->Property_Add("multiplayer_callsign", false, 0, callsign);
					NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_Global();
				}

				textInput.Exit();
				break;

			case SwitchIds::MainMenuScreen::SelectServiceProvider:
			case SwitchIds::MainMenuScreen::HostOrJoin:
			case SwitchIds::MainMenuScreen::EnterSessionName:
			case SwitchIds::MainMenuScreen::SessionList:
			case SwitchIds::MainMenuScreen::Lobby:
			case SwitchIds::MainMenuScreen::PlayerConfiguration:
			case SwitchIds::MainMenuScreen::AutoHost:
			case SwitchIds::MainMenuScreen::AutoJoin:
				if (shutDownNetwork)
					NNetwork::Network_ShutDown();

				textInput.Exit();
				break;

			case SwitchIds::MainMenuScreen::FileUpload:
			case SwitchIds::MainMenuScreen::FileDownload:
				if (fileTransferManager != nullptr)
				{
					fileTransferManager->~CFileTransferManager();
					NXBasics::Runtime_Free(fileTransferManager);
					fileTransferManager = nullptr;
				}

				if (shutDownNetwork)
					NNetwork::Network_ShutDown();

				break;

			case SwitchIds::MainMenuScreen::NetworkError:
				if (shutDownNetwork)
					NNetwork::Network_ShutDown();

				errorCount = 0;
				break;

			default:
				break;
		}

		currentScreen = 0;
	}

	// Original function: FUN_004d34d2
	void CGuiManager::l_ScreenData_Init(std::int32_t screen)
	{
		if (currentScreen != 0)
			return;

		currentScreen = screen;

		switch (screen)
		{
			case SwitchIds::MainMenuScreen::EnterCallsign:
				if (!StartNetworkForMenu(this))
				{
					currentScreen = 18;
					Debug_ErrorString_Add("Could not init multi player module");
					break;
				}

				Network_InitializeCallsign();
				Chat_ClearAll();
				textInput.Init(NNetwork::Network_GetCallsignBufferSize() - 2, false, true, 0u, 0u);
				textInput.SetString(NNetwork::Network_GetCallsign());
				break;

			case SwitchIds::MainMenuScreen::SelectServiceProvider:
				if (!Network_EnumerateServiceProviders())
				{
					currentScreen = 18;
					Debug_ErrorString_Add("Could not enumerate service provider");
				}

				break;

			case SwitchIds::MainMenuScreen::EnterSessionName:
				std::memset(sessionSearchData, 0, sizeof(sessionSearchData));
				textInput.Init(100u, false, true, 0u, 0u);
				break;

			case SwitchIds::MainMenuScreen::Lobby:
				{
					textInput.Init(60u, true, true, 0u, 0u);

					std::memset(lobbyData, 0, sizeof(lobbyData));
					lobbyValue = 0;

					if (!NNetwork::Network_IsHost())
					{
						std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
						message[0] = 2;

						const NetworkPlayerId hostPlayerId = NNetwork::GetDynamicNetworkHostPlayerId(&gDynamicNetworkData);
						NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, hostPlayerId, message, sizeof(message));
					}

					if (!Network_UpdatePlayerList())
					{
						currentScreen = 18;
						Debug_ErrorString_Add("Update player list failed");
						break;
					}

					if (NNetwork::Network_IsHost() && NC2Logic::gCleanMapsManager != nullptr)
						NC2Logic::gCleanMapsManager->CleanMapList_Update();

					break;
				}

			case SwitchIds::MainMenuScreen::PlayerConfiguration:
				std::memset(reinterpret_cast<std::byte*>(this) + 0xEEC, 0, 36);
				break;

			case SwitchIds::MainMenuScreen::FileUpload:
			case SwitchIds::MainMenuScreen::FileDownload:
				{
					std::memset(reinterpret_cast<std::byte*>(this) + 0xEEC, 0, 36);

					char sourceFileName[260];
					char transferFileName[260];
					char requestedFileName[260];

					if (NNetwork::Network_IsHost())
					{
						if (screen == 17)
						{
							const auto* mapInformation = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(*static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(lobbyData + NC2Layout::MenuLobbyOffset::MapGuid)));

							std::strcpy(sourceFileName, mapInformation->archiveFilename);
							NXBasics::Runtime_PrintString(transferFileName, OriginalStrings::DirectoryAndFileNameFormat, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), "transdat");

							{
								NXBasics::CMemory temporaryFileData(sourceFileName);
								NXBasics::CFile outputFile(transferFileName, false);

								outputFile.OpenForWriting(true);
								outputFile.Write(temporaryFileData.mData, temporaryFileData.mSize);
							}

							transferFlags[4] = 1;

							NXBasics::Runtime_PrintString(transferFileName, OriginalStrings::DirectoryAndFileNameFormat, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), "transdat");
						}
						else
						{
							std::uint32_t saveGameId;
							std::memcpy(&saveGameId, lobbyData + NC2Layout::MenuLobbyOffset::SavegameId, sizeof(saveGameId));

							const auto saveGameName = NC2Logic::gSavegamesManager->GetSavegameInfoPtrByUniqueId(saveGameId);

							NC2Logic::gSavegamesManager->Tool_CreateFullSavegameFilename(reinterpret_cast<const char*>(saveGameName), sourceFileName);
							NC2Logic::gSavegamesManager->Tool_CreateFullSavegameFilename("sys_continue", transferFileName);

							{
								NXBasics::CMemory temporaryFileData(sourceFileName);
								NXBasics::CFile outputFile(transferFileName, false);

								outputFile.OpenForWriting(true);
								outputFile.Write(temporaryFileData.mData, temporaryFileData.mSize);
							}

							transferFlags[1] = 1;

							NC2Logic::gSavegamesManager->Tool_CreateFullSavegameFilename("sys_continue", transferFileName);
						}

						NNetwork::CFileCompressor compressor;
						const std::int32_t compressionResult = compressor.CompressFile(transferFileName);

						if (compressionResult > 0 && compressionResult <= 2)
						{
							currentScreen = 18;

							if (screen == 17)
								Debug_ErrorString_Add("Map compression failed (I/O error)");
							else
								Debug_ErrorString_Add("Save game compression failed (I/O error)");
						}

						if (compressionResult != 0) { currentScreen = 18; Debug_ErrorString_Add("Cannot prepare game transfer archive"); break; }
						void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NNetwork::CFileTransferManager), 1);

						fileTransferManager = memory != nullptr ? new (memory) NNetwork::CFileTransferManager : nullptr;
						if (fileTransferManager == nullptr) { currentScreen = 18; Debug_ErrorString_Add("Cannot allocate file transfer service"); break; }
						fileTransferManager->StartService();
						std::strcat(transferFileName, ".c2p");
						fileTransferManager->SetSharedFile(transferFileName);
					}
					else
					{
						if (screen == 17)
						{
							transferFlags[6] = 0;
							transferFlags[5] = 1;
						}
						else
						{
							transferFlags[3] = 0;
							transferFlags[2] = 1;
						}

						void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NNetwork::CFileTransferManager), 1);

						fileTransferManager = memory != nullptr ? new (memory) NNetwork::CFileTransferManager : nullptr;
						if (fileTransferManager == nullptr) { currentScreen = 18; Debug_ErrorString_Add("Cannot allocate file transfer service"); break; }
						fileTransferManager->StartService();

						if (screen == 17)
						{
							NXBasics::CFile::FileSystem_Tool_CreateDirectory(NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr());

							NXBasics::Runtime_PrintString(requestedFileName, OriginalStrings::DirectoryAndFileNameFormat, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), "transdat");
							std::strcat(requestedFileName, ".c2p");
						}
						else
						{
							NC2Logic::gSavegamesManager->SavegameList_Update(2, false);
							NC2Logic::gSavegamesManager->Tool_CreateFullSavegameFilename("sys_continue", requestedFileName);
							NXBasics::CFile::FileSystem_Tool_CreateDirectory(NC2Logic::gSavegamesManager->Tool_GetCurrentPlayerSavegameDirectoryNamePtr());

							std::strcat(requestedFileName, ".c2p");
						}

						fileTransferManager->StartTransfer(requestedFileName, &transferId);
					}

					transferStartTick = timeGetTime();
					transferIsHost = static_cast<std::uint8_t>(NNetwork::Network_IsHost());

					NC2Logic::GameTime_SetSpeed(0.0f);
					return;
				}

			case SwitchIds::MainMenuScreen::AutoHost:
				if (!Network_AutoHost())
				{
					currentScreen = 18;
					Debug_ErrorString_Add("Could not auto host session");
				}
				else
				{
					currentScreen = 0;
					l_ScreenData_Init(14);
				}

				break;

			case SwitchIds::MainMenuScreen::AutoJoin:
				{
					const auto session = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetStringValuePtr("multiplayer_auto_join");

					if (!Network_AutoJoin(session))
					{
						currentScreen = 18;
						Debug_ErrorString_Add("Could not auto join session");
					}
					else
					{
						currentScreen = 0;
						l_ScreenData_Init(14);
					}

					break;
				}

			default:
				break;
		}

		NC2Logic::GameTime_SetSpeed(cMenuGameSpeed);
	}

	// Original function: FUN_004d6b4a
	bool CGuiManager::Screen_IsMultiPlayerScreen() const
	{
		return currentScreen >= 9 && currentScreen <= 20;
	}

	// Original function: FUN_004d7092
	bool StartNetworkForMenu(void* guiManager)
	{
		auto* objectBytes = static_cast<std::byte*>(guiManager);

		// Original helper FUN_00410b46 forwards to memset with a zero value.
		std::memset(objectBytes + NC2Layout::MainMenuManagerOffset::ProviderCount, 0, NC2Layout::RecordSize::MenuProviderListWithSelection);

		if (!NNetwork::Network_StartUp())
		{
			NNetwork::Network_ShutDown();
			return false;
		}

		return true;
	}

	// Original function: FUN_004d7389
	char* CGuiManager::Network_GetCallsign()
	{
		if (currentScreen == 9)
		{
			NNetwork::Network_SetCallsign(textInput.GetString());
		}

		return NNetwork::Network_GetCallsign();
	}

	// Original function: FUN_004d73b0
	char* CGuiManager::Network_GetSessionName()
	{
		char* sessionName = reinterpret_cast<char*>(sessionSearchData);

		if (currentScreen == 12)
			std::strcpy(sessionName, textInput.GetString());

		return sessionName;
	}

	// Original function: FUN_004d7ca9
	void CGuiManager::Chat_AddText(std::uint32_t type, const char* text)
	{
		std::uint32_t count;
		std::memcpy(&count, chatHistoryStorage, sizeof(count));

		if (count != 0)
		{
			const std::uint32_t movedCount = count == 20u ? 19u : count;

			std::memmove(chatHistoryStorage + 4 + 0x74, chatHistoryStorage + 4, movedCount * 116u);
		}

		// Each record contains a four-byte type followed by its text storage.
		std::memset(chatHistoryStorage + 4, 0, 116);
		NXBasics::Text_Copy(reinterpret_cast<char*>(chatHistoryStorage + 8), 0x6E, text);

		std::memcpy(chatHistoryStorage + 4, &type, sizeof(type));

		// Reload the count after inserting the record, as the original does.
		std::memcpy(&count, chatHistoryStorage, sizeof(count));

		if (count != 20u)
		{
			++count;
			std::memcpy(chatHistoryStorage, &count, sizeof(count));
		}
	}

	// Original function: FUN_004d85cf
	void CGuiManager::l_Network_Update()
	{
		// Access unresolved fields without changing the established object layout.
		const auto readWord = [this] (std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, reinterpret_cast<const std::byte*>(this) + offset, sizeof(value));
			return value;
		};

		const auto writeWord = [this] (std::size_t offset, std::uint32_t value)
		{
			std::memcpy(reinterpret_cast<std::byte*>(this) + offset, &value, sizeof(value));
		};

		const auto fail = [this] (const char* text)
		{
			Screen_ChangeTo(18, true);
			Debug_ErrorString_Add(text);
		};

		if (Screen_IsMultiPlayerScreen() && gDynamicNetworkData.active != 0)
		{
			NNetwork::UpdateDynamicNetwork(&gDynamicNetworkData, false);

			if (gDynamicNetworkData.receivedChatRecordCount != 0)
			{
				void* chatBuffer = nullptr;
				std::uint32_t chatCount = 0;

				NNetwork::TakeDynamicNetworkTextRecords(&gDynamicNetworkData, &chatBuffer, &chatCount);

				const auto* records = static_cast<const std::byte*>(chatBuffer);

				for (std::uint32_t index = 0; index < chatCount; ++index)
				{
					const std::byte* record = records + index * 0x104u;

					std::uint16_t textType;
					std::memcpy(&textType, record + 4, sizeof(textType));

					if (textType == 1)
					{
						char text[128] = { };

						NNetwork::ConvertNetworkWideToUtf8(text, reinterpret_cast<const wchar_t*>(record + 6), static_cast<std::int32_t>(std::size(text)));
						Chat_AddText(0, text);
						NC2GuiToolsBase::FXTool_Chat();
					}
				}

				if (chatBuffer != nullptr)
					NXBasics::Runtime_Free(chatBuffer);
			}

			l_Network_ProcessDataMessages();

			// Processing a data message can shut down the network.
			if (gDynamicNetworkData.active != 0)
			{
				for (NNetwork::SEventNode* event = gDynamicNetworkData.events.first; event != nullptr; event = event->next)
				{
					char playerName[100] = { };
					char text[300];
					text[0] = '\0';

					const auto* name = reinterpret_cast<const wchar_t*>(reinterpret_cast<const std::byte*>(event) + 4);

					if (*name != L'\0')
						NNetwork::CopyNetworkWideToUtf8(playerName, name, static_cast<std::int32_t>(std::size(playerName)));
					else
						NXBasics::Runtime_PrintString(playerName, OriginalStrings::UnknownPlayerName);

					std::uint32_t eventType;
					std::memcpy(&eventType, reinterpret_cast<const std::byte*>(event) + 0x208, sizeof(eventType));

					if (eventType >= 1u && eventType <= 13u)
					{
						static constexpr std::uint32_t textIds[13] =
						{
							0x59, 0x58, 0x57, 0x57, 0x57, 0x57, 0x57,
							0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F
						};

						const char* description = NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(textIds[eventType - 1u]);
						const char* format = eventType >= 2u && eventType <= 7u ? "%s %s!" : ">> %s %s!";

						NXBasics::Runtime_PrintString(text, format, playerName, description);
					}
					else
					{
						char description[200] = { };

						NNetwork::FormatNetworkEvent(&gDynamicNetworkData.events, event, description, sizeof(description));
						NXBasics::Runtime_PrintString(text, ">> NET: '%s'", description);
					}

					if (text[0] != '\0')
						Chat_AddText(1, text);
				}

				NNetwork::ClearDynamicNetworkEvents(&gDynamicNetworkData);
			}
		}

		const std::uint32_t networkFlags = NNetwork::Network_GetFlags();

		switch (currentScreen)
		{
			case SwitchIds::MainMenuScreen::SelectServiceProvider:
				{
					if ((networkFlags & 1u) == 0)
					{
						fail("Wrong network state while selecting service provider");
						return;
					}

					std::uint32_t providerIndex = 0;

					for (; providerIndex < readWord(NC2Layout::MainMenuManagerOffset::ProviderCount); ++providerIndex)
					{
						const std::byte* providerGuid = reinterpret_cast<const std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + providerIndex * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned;

						if (std::memcmp(providerGuid, &cTcpIpServiceProviderGuid, sizeof(GUID)) == 0)
							break;
					}

					if (providerIndex == readWord(NC2Layout::MainMenuManagerOffset::ProviderCount))
					{
						Debug_ErrorString_Add("No TCPIP service provider found");
						return;
					}

					writeWord(NC2Layout::MainMenuManagerOffset::SelectedProviderIndex, providerIndex);

					sTheObjectPtr->Screen_ChangeTo(11, false);
					return;
				}

			case SwitchIds::MainMenuScreen::HostOrJoin:
				if ((networkFlags & 1u) == 0)
					fail("Wrong network state while selecting host or client");

				return;

			case SwitchIds::MainMenuScreen::SessionList:
				if ((networkFlags & 4u) == 0)
				{
					fail("Wrong network state while searching sessions");
					return;
				}

				if (Network_UpdateSessionList())
					Screen_UpdateGui();

				return;

			default:
				if (currentScreen < 14 || currentScreen > 17)
					return;

				break;
		}

		if (NNetwork::Network_IsHost())
		{
			if ((networkFlags & 2u) == 0)
			{
				fail("Wrong network state while waiting for game start while hosting");
				return;
			}
		}
		else
		{
			if ((networkFlags & 8u) == 0)
			{
				fail("Wrong network state while waiting for game start while connected");
				return;
			}
		}

		if (readWord(NC2Layout::MainMenuManagerOffset::PlayerChangeCount) != GetNetworkInstance()->playerChangeCount)
		{
			if (currentScreen == 16 || currentScreen == 17)
			{
				fail("Player left session while transfering savegame");
				return;
			}

			if (!Network_UpdatePlayerList())
			{
				fail("Update player list failed");
				return;
			}

			Screen_UpdateGui();
		}

		const auto hasPlayer = [&readWord] (std::uint32_t playerId) -> bool
		{
			const std::uint32_t playerCount = readWord(NC2Layout::MainMenuManagerOffset::PlayerCount);

			for (std::uint32_t index = 0; index < playerCount; ++index)
			{
				if (readWord(NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned) == playerId)
					return true;
			}

			return false;
		};

		const std::uint32_t selectedPlayerId = readWord(NC2Layout::MainMenuManagerOffset::SelectedPlayerId);

		if (selectedPlayerId != 0 && !hasPlayer(selectedPlayerId))
			writeWord(NC2Layout::MainMenuManagerOffset::SelectedPlayerId, 0);

		bool assignmentsChanged = false;

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			const std::size_t offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
			const std::uint32_t playerId = readWord(offset);

			if (playerId != 0 && !hasPlayer(playerId))
			{
				writeWord(offset, 0);
				assignmentsChanged = true;
			}
		}

		if (assignmentsChanged)
		{
			Screen_UpdateGui();
			Network_SendLobbyData();
		}

		if (currentScreen == 14)
		{
			if (transferIsHost != static_cast<std::uint8_t>(NNetwork::Network_IsHost()))
			{
				if (NNetwork::Network_IsHost())
					Network_ClearLobbyData();

				transferIsHost = static_cast<std::uint8_t>(NNetwork::Network_IsHost());
			}
		}

		if (currentScreen == 15 && NNetwork::Network_IsHost())
		{
			bool allPlayersAssigned = true;
			const std::uint32_t playerCount = readWord(NC2Layout::MainMenuManagerOffset::PlayerCount);

			for (std::uint32_t playerIndex = 0; playerIndex < playerCount; ++playerIndex)
			{
				const std::uint32_t playerId = readWord(NC2Layout::MainMenuManagerOffset::FirstPlayerId + playerIndex * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned);
				bool assigned = false;

				for (std::uint32_t slot = 0; slot < 20; ++slot)
				{
					if (readWord(NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) == playerId)
					{
						assigned = true;
						break;
					}
				}

				if (!assigned)
				{
					allPlayersAssigned = false;
					break;
				}
			}

			if (allPlayersAssigned)
			{
				if (readWord(NC2Layout::MainMenuManagerOffset::LobbyMode) == 2u)
				{
					sTheObjectPtr->Screen_ChangeTo(16, false);

					std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
					message[0] = 4;

					NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message, sizeof(message));
				}
				else
				{
					const NC2Logic::SCleanMapInfoData* mapInformation = nullptr;

					if (static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(lobbyData + NC2Layout::MenuLobbyOffset::MapGuid))->IsValid())
						mapInformation = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(*static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(lobbyData + NC2Layout::MenuLobbyOffset::MapGuid)));

					if (mapInformation != nullptr && (mapInformation->mapTypes & NXBasics::BitMask::Bit6) != 0 && mapInformation->userMap != 0)
					{
						sTheObjectPtr->Screen_ChangeTo(17, false);

						std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
						message[0] = 5;

						NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message, sizeof(message));
					}
					else if (transferFlags[0] == 0)
					{
						transferFlags[0] = 1;

						NNetwork::BroadcastControlMessage8(&gDynamicNetworkData);

						std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
						auto* messageBytes = reinterpret_cast<std::byte*>(message);

						message[0] = 1;

						const auto* menu = sTheObjectPtr;
						std::memcpy(messageBytes + 8, menu->lobbyData, sizeof(menu->lobbyData));

						messageBytes[4] = std::byte{ 1 };

						NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message, sizeof(message));
					}
				}
			}
		}

		// This deliberately observes screen changes made above.
		l_Transfer_Update();
	}

	// Original function: FUN_004d85cf
	// Extracted block: 004d86a7 through 004d8917.
	void CGuiManager::l_Network_ProcessDataMessages()
	{
		NNetwork::SDynamicDataMessage* messages = nullptr;
		std::uint32_t messageCount = 0;

		NNetwork::GetDynamicNetworkMessages(&gDynamicNetworkData, &messages, &messageCount);

		for (std::uint32_t index = 0; index < messageCount; ++index)
		{
			const NNetwork::SDynamicDataMessage& message = messages[index];

			if (message.checksumMismatch != 0)
				Debug_ErrorString_Add("CRC-32 check failed - message data is damaged");

			std::uint32_t messageType;
			std::memcpy(&messageType, message.payload, sizeof(messageType));

			switch (messageType)
			{
				case SwitchIds::MenuNetworkMessage::LobbyData:
					{
						if (!NNetwork::Network_IsHost())
							std::memcpy(lobbyData, message.payload + 8, sizeof(lobbyData));

						// Rebuild elements for the newly initialized screen.
						Screen_UpdateGui();

						if (message.payload[4] == 0)
							break;

						std::uint32_t gameMode;
						std::memcpy(&gameMode, lobbyData, sizeof(gameMode));

						if (gameMode == 1u || gameMode == 2u)
						{
							auto* master = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;
							auto* startupData = reinterpret_cast<std::byte*>(master) + 0x2C;

							// Original startup-data reset.
							NC2Logic::GameControlStartupData_Clear(startupData);

							const std::uint32_t startupType = gameMode == 1u ? 3u : 4u;
							std::memcpy(startupData, &startupType, sizeof(startupType));

							if (gameMode == 1u)
								std::memcpy(startupData + 0x18, lobbyData + NC2Layout::MenuLobbyOffset::MapGuid, 16);

							const std::uint32_t localPlayerId = NNetwork::Network_GetLocalPlayerId();
							std::memcpy(startupData + 0x28, &localPlayerId, sizeof(localPlayerId));

							for (std::uint32_t playerIndex = 0; playerIndex < 20; ++playerIndex)
							{
								const std::byte* source = lobbyData + NC2Layout::MenuLobbyOffset::Players + playerIndex * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
								std::byte* destination = startupData + 0x2C + playerIndex * 0x0Cu;

								std::uint32_t firstValue;
								std::uint32_t secondValue;
								std::uint32_t thirdValue;

								std::memcpy(&firstValue, source + 4, sizeof(firstValue));
								std::memcpy(&thirdValue, source, sizeof(thirdValue));

								if (lobbyData[NC2Layout::MenuLobbyOffset::FixedPlayerColors] != std::byte{ 0 })
									secondValue = playerIndex < 10u ? playerIndex : 9u;
								else
									std::memcpy(&secondValue, source + 0x0C, sizeof(secondValue));

								std::memcpy(destination, &firstValue, sizeof(firstValue));
								std::memcpy(destination + 4, &secondValue, sizeof(secondValue));
								std::memcpy(destination + 8, &thirdValue, sizeof(thirdValue));
							}

							if (gameMode == 1u)
								std::memcpy(startupData + 0x11C, lobbyData + NC2Layout::MenuLobbyOffset::Difficulty, 4);

							NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_ProgramState_ChangeNextTimePossible(NMasterControlProgram::TProgramState::Game, false);
						}

						// The original checks the music setting even for other game modes.
						if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetIntegerValue("music_mode") == 2)
							NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("music_mode", true, NCDAudioManager::CCDAudioManager::sTheObjectPtr != nullptr ? 3 : 1, nullptr);

						break;
					}

				case SwitchIds::MenuNetworkMessage::RequestLobbyData:
					{
						std::uint32_t response[360 / sizeof(std::uint32_t)] = { };
						auto* responseBytes = reinterpret_cast<std::byte*>(response);

						response[0] = 1;

						const auto* menu = sTheObjectPtr;
						std::memcpy(responseBytes + 8, menu->lobbyData, sizeof(menu->lobbyData));

						responseBytes[4] = std::byte{ 0 };

						NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, message.sender, response, sizeof(response));
						break;
					}

				case SwitchIds::MenuNetworkMessage::Disconnected:
					Screen_ChangeTo(0x15, true);
					break;

				case SwitchIds::MenuNetworkMessage::StartUpload:
					if (!NNetwork::Network_IsHost())
						Screen_ChangeTo(0x10, false);

					break;

				case SwitchIds::MenuNetworkMessage::StartDownload:
					if (!NNetwork::Network_IsHost())
						Screen_ChangeTo(0x11, false);

					break;

				default:
					break;
			}
		}

		if (messages != nullptr)
			NXBasics::Runtime_Free(messages);
	}

	// Original function: FUN_004d85cf
	// Extracted transfer section beginning at 004d8d5e.
	void CGuiManager::l_Transfer_Update()
	{
		if (currentScreen == 16)
		{
			const std::uint8_t isHost = static_cast<std::uint8_t>(NNetwork::Network_IsHost());

			if (transferIsHost != isHost)
			{
				Screen_ChangeTo(18, true);
				Debug_ErrorString_Add("Host changed while transfering savegame");
				return;
			}

			// Preserve the second host query performed by the original.
			if (NNetwork::Network_IsHost())
				l_Transfer_UpdateUpload(false);
			else
				l_Transfer_UpdateDownload(false);
		}

		// Re-read the screen after the savegame branch.
		if (currentScreen == 17)
		{
			const std::uint8_t isHost = static_cast<std::uint8_t>(NNetwork::Network_IsHost());

			if (transferIsHost != isHost)
			{
				Screen_ChangeTo(18, true);
				Debug_ErrorString_Add("Host changed while transfering user map");
				return;
			}

			if (NNetwork::Network_IsHost())
				l_Transfer_UpdateUpload(true);
			else
				l_Transfer_UpdateDownload(true);
		}
	}

	// Original function: FUN_004d85cf
	// Extracted blocks: 004d8f13 through 004d9053 and 004d924e through 004d950f.
	void CGuiManager::l_Transfer_UpdateDownload(bool userMap)
	{
		const std::uint32_t requestedFlag = userMap ? 5u : 2u;
		const std::uint32_t completedFlag = userMap ? 6u : 3u;

		if (transferFlags[requestedFlag] == 0 || transferFlags[completedFlag] != 0)
			return;

		const auto reportFailure = [this] (const char* text)
		{
			// The original changes the singleton's screen and logs on this object.
			sTheObjectPtr->Screen_ChangeTo(18, true);
			Debug_ErrorString_Add(text);
		};

		if (fileTransferManager == nullptr || fileTransferManager->serviceState != 1)
		{
			reportFailure(userMap ? "Clean map transfer failed! Transfer not started!" : "Savegame transfer failed! Transfer not started!");
			return;
		}

		const NNetwork::SFileTransferInfo* info = fileTransferManager->GetTransferInfo(&transferId);

		if (info == nullptr)
		{
			reportFailure(userMap ? "Clean map transfer failed! Could not get description!" : "Savegame transfer failed! Could not get description!");
			return;
		}

		if (info->fileSize == 0 || info->transferredBytes != info->fileSize)
			return;

		if (info->expectedChecksum != info->checksum)
		{
			reportFailure(userMap ? "Clean map transfer failed! Checksum do not match!" : "Savegame transfer failed! Checksum do not match!");
			return;
		}

		transferFlags[completedFlag] = 1;

		if (fileTransferManager != nullptr)
		{
			fileTransferManager->~CFileTransferManager();
			NXBasics::Runtime_Free(fileTransferManager);
		}

		fileTransferManager = nullptr;

		char compressedFileName[260];

		NNetwork::CFileCompressor compressor;

		if (!userMap)
		{
			NC2Logic::gSavegamesManager->Tool_CreateFullSavegameFilename("sys_continue", compressedFileName);
			std::strcat(compressedFileName, ".c2p");

			const std::int32_t result = compressor.DecompressFile(compressedFileName);

			if (result != 0) { reportFailure("Savegame decompression failed! File I/O error!"); return; }

			// Refresh saved-game metadata only after a successful decode.
			NC2Logic::gSavegamesManager->SavegameList_Update(2, true);
			return;
		}

		std::strcpy(compressedFileName, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr());
		std::strcat(compressedFileName, OriginalStrings::WindowsRootPath);
		std::strcat(compressedFileName, "transdat");
		std::strcat(compressedFileName, ".c2p");

		const std::int32_t result = compressor.DecompressFile(compressedFileName);

		if (result != 0)
		{
			reportFailure("Map decompression failed! File I/O error!");
		}
		else
		{
			char archiveFileName[260];
			char destinationFileName[260];

			NXBasics::Runtime_PrintString(archiveFileName, OriginalStrings::DirectoryAndFileNameFormat, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), "transdat");

			NXBasics::SArchiveState archive;
			NXBasics::Archive_Initialize(&archive, archiveFileName);

			NXBasics::CFile::FileSystem_FileLibrary_Add(&archive);

			{
				NXBasics::CIniFileReader reader("currentusermap\\map.ini", nullptr, false, nullptr, nullptr);

				// The reader has already loaded its input at this point.
				NXBasics::CFile::FileSystem_FileLibrary_Remove(&archive);

				if (reader.IsLoaded() && reader.SearchGroup("logiccontrol") && reader.SearchCommand("mapguid"))
				{
					std::uint8_t mapGuid[16];

					for (std::uint32_t index = 0; index < 16; ++index)
						mapGuid[index] = static_cast<std::uint8_t>(reader.GetInt());

					NXBasics::Runtime_PrintString(destinationFileName, "%s\\usermap_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d.c2m", NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), static_cast<int>(mapGuid[0]), static_cast<int>(mapGuid[1]), static_cast<int>(mapGuid[2]), static_cast<int>(mapGuid[3]), static_cast<int>(mapGuid[4]), static_cast<int>(mapGuid[5]), static_cast<int>(mapGuid[6]), static_cast<int>(mapGuid[7]), static_cast<int>(mapGuid[8]), static_cast<int>(mapGuid[9]), static_cast<int>(mapGuid[10]), static_cast<int>(mapGuid[11]), static_cast<int>(mapGuid[12]), static_cast<int>(mapGuid[13]), static_cast<int>(mapGuid[14]), static_cast<int>(mapGuid[15]));
				}
				else
				{
					NXBasics::Runtime_PrintString(destinationFileName, "%s\\transfered.c2m", NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr());
				}
			}

			NXBasics::Archive_Destroy(&archive);

			// The original ignores the rename result.
			NXBasics::Runtime_RenameFile(archiveFileName, destinationFileName);
		}

		// The original refreshes the map list after either decompression outcome.
		NC2Logic::gCleanMapsManager->CleanMapList_Update();
	}

	// Original function: FUN_004d85cf
	// Extracted host blocks: 004d8d98 through 004d8f13 and 004d908b through 004d924e.
	void CGuiManager::l_Transfer_UpdateUpload(bool userMap)
	{
		const std::uint32_t uploadFlag = userMap ? 4u : 1u;

		if (transferFlags[uploadFlag] == 0 || transferFlags[0] != 0 || fileTransferManager == nullptr)
			return;

		const auto* objectBytes = reinterpret_cast<const std::byte*>(this);

		std::uint32_t playerCount;
		std::memcpy(&playerCount, objectBytes + NC2Layout::MainMenuManagerOffset::PlayerCount, sizeof(playerCount));

		std::uint32_t expectedTransfers = 0;

		// Preserve the original player-record accesses without guessing their types.
		const std::byte* playerFields = objectBytes + NC2Layout::MainMenuManagerOffset::FirstPlayerHostFlag;

		for (std::uint32_t index = 0; index < playerCount; ++index)
		{
			std::uint32_t playerId;
			std::memcpy(&playerId, playerFields + 2, sizeof(playerId));

			if (playerId != 0 && playerFields[0] == std::byte{ 0 })
				++expectedTransfers;

			playerFields += NC2Layout::MainMenuManagerOffset::ProviderRows;
		}

		NNetwork::SFileTransferNode jobs[20];
		std::uint32_t jobCount = 20;

		fileTransferManager->GetTransferList(jobs, &jobCount);

		std::uint32_t completedTransfers = 0;

		for (std::uint32_t index = 0; index < jobCount; ++index)
		{
			if (jobs[index].state == 1)
			{
				++completedTransfers;
			}
			else if (jobs[index].state == 14)
			{
				sTheObjectPtr->Screen_ChangeTo(18, true);
				Debug_ErrorString_Add(userMap ? "Clean map transfer failed!" : "Savegame transfer failed!");

				// The original continues evaluating the copied job list.
			}
		}

		// The original destroys all twenty copied payloads in reverse order.
		for (std::uint32_t index = 20; index != 0;)
		{
			NNetwork::SFileTransferNode& job = jobs[--index];

			if (job.fileName != nullptr)
			{
				NXBasics::Runtime_Free(job.fileName);
				job.fileName = nullptr;
			}

			if (job.path24 != nullptr)
			{
				NXBasics::Runtime_Free(job.path24);
				job.path24 = nullptr;
			}
		}

		if (completedTransfers != expectedTransfers)
			return;

		if (fileTransferManager != nullptr)
		{
			fileTransferManager->~CFileTransferManager();
			NXBasics::Runtime_Free(fileTransferManager);
			fileTransferManager = nullptr;
		}

		if (transferFlags[0] != 0)
			return;

		transferFlags[0] = 1;

		NNetwork::BroadcastControlMessage8(&gDynamicNetworkData);

		std::uint32_t message[360 / sizeof(std::uint32_t)] = { };
		auto* messageBytes = reinterpret_cast<std::byte*>(message);

		message[0] = 1;
		messageBytes[4] = std::byte{ 1 };

		const auto* menu = sTheObjectPtr;
		std::memcpy(messageBytes + 8, menu->lobbyData, sizeof(menu->lobbyData));

		NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message, sizeof(message));

		if (userMap)
		{
			char fileName[260];

			NXBasics::Runtime_PrintString(fileName, OriginalStrings::DirectoryAndFileNameFormat, NC2Logic::IO_Tool_GetUserMapDirectoryNamePtr(), "transdat");
			NXBasics::Runtime_RemoveFile(fileName);

			std::strcat(fileName, ".c2p");
			NXBasics::Runtime_RemoveFile(fileName);
		}
	}

	// Original function: FUN_004d9605
	void CGuiManager::l_Network_Draw()
	{
		if (!Screen_IsMultiPlayerScreen())
			return;

		const auto& guiResources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;

		const auto readWord = [] (const void* object, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
			return value;
		};

		const auto signedWord = [] (std::uint32_t value) -> std::int32_t
		{
			return NXBasics::Runtime_TimeSigned32(value);
		};

		const auto desktop = [this] () -> NXBasics::CBitmap*
		{
			return reinterpret_cast<NXBasics::CBitmap*>(Debug_GetDesktopBitmapPtr());
		};

		const auto defaultColour = [&guiResources] () -> const NXBasics::SColorRGB*
		{
			return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(guiResources.fontWhitePalette) + 0x400);
		};

		const auto drawText = [&] (NXBasics::CBitmap* bitmap, auto* font, std::int32_t x, std::int32_t y, const char* text)
		{
			bitmap->Text_Print(font, defaultColour(), x, y, text);
		};

		const auto drawFormatted = [&] (NXBasics::CBitmap* bitmap, auto* font, std::int32_t x, std::int32_t y, const char* format, auto... arguments)
		{
			bitmap->Text_PrintV(font, defaultColour(), x, y, format, arguments...);
		};

		const auto drawCursor = [&] (auto* font, std::uint32_t x, std::uint32_t y, std::int32_t length, NXBasics::CBitmap* bitmap)
		{
			const std::uint8_t intensity = ((::timeGetTime() / 200u) & 1u) != 0 ? 255 : 100;
			const NXBasics::SColorRGB colour(intensity, intensity, intensity);
			const std::uint32_t cursorX = x + textInput.GetCursorPixelPosition(font);

			if (bitmap == nullptr)
				bitmap = desktop();

			bitmap->Draw_VerticalLine(signedWord(cursorX), y, length, colour);
		};

		auto* inputFont = guiResources.font10;

		if (currentScreen == 9)
		{
			std::int32_t y = signedWord(static_cast<std::uint32_t>(mStaticVars.displayHeight)) / 2 - 50;
			const auto* colour = defaultColour();
			const char* text = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xD1);

			desktop()->Text_Print(inputFont, colour, 56, y, text);

			y += 30;
			colour = defaultColour();
			text = Network_GetCallsign();

			desktop()->Text_Print(inputFont, colour, 66, y, text);
			drawCursor(inputFont, 66, static_cast<std::uint32_t>(y), 16, nullptr);
		}

		if (currentScreen == 12)
		{
			std::int32_t y = signedWord(static_cast<std::uint32_t>(mStaticVars.displayHeight)) / 2 - 50;
			const auto* colour = defaultColour();
			const char* text = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xD2);

			desktop()->Text_Print(inputFont, colour, 56, y, text);

			y += 16;
			colour = defaultColour();
			text = NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xD3);

			desktop()->Text_Print(inputFont, colour, 56, y, text);

			y += 30;
			colour = defaultColour();
			text = Network_GetSessionName();

			desktop()->Text_Print(inputFont, colour, 66, y, text);
			drawCursor(inputFont, 66, static_cast<std::uint32_t>(y), 16, nullptr);
		}

		const std::int32_t chatY = signedWord(readWord(desktop(), 0x14)) / 2 + 80;
		const std::int32_t informationY = signedWord(readWord(desktop(), 0x14)) / 2 + 210;
		auto* informationFont = guiResources.font08;

		const auto drawTransferProgress = [&] (bool userMap)
		{
			std::int32_t y = signedWord(static_cast<std::uint32_t>(mStaticVars.displayHeight)) / 2 - 70;
			NXBasics::CBitmap* bitmap = desktop();
			auto* font = guiResources.font08;

			if (NNetwork::Network_IsHost())
			{
				const std::size_t startedFlag = userMap ? 4 : 1;

				if (transferFlags[startedFlag] != 0 && fileTransferManager != nullptr)
				{
					NNetwork::SFileTransferNode transfers[20];

					for (auto& transfer : transfers)
						std::memset(&transfer.fileName, 0, sizeof(NNetwork::SFileTransferInfo));

					std::uint32_t count = 20;
					fileTransferManager->GetTransferList(transfers, &count);

					const std::uint32_t now = ::timeGetTime();

					for (std::uint32_t index = 0; index < count; ++index)
					{
						char text[100];
						std::strcpy(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC9));

						const auto& transfer = transfers[index];
						const char* playerName = Network_GetPlayerName(transfer.peerId);
						const std::uint32_t transferred = transfer.transferredBytes;
						const std::uint32_t size = transfer.fileSize;

						const std::uint32_t percent = size != 0 ? static_cast<std::uint32_t>((std::min)(std::uint64_t(100), std::uint64_t(transferred) * 100 / size)) : 0;
						const std::uint32_t elapsed = (now - transferStartTick) / 1000u;
						std::uint32_t estimatedTotal = percent != 0 ? (elapsed * 100u) / percent : 3600u;

						if (estimatedTotal >= 3600u)
							estimatedTotal = 3600u;

						if (transfer.state == 1)
						{
							NXBasics::Runtime_PrintString(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCC), playerName);
						}
						else if (transfer.state == 13)
						{
							const std::uint32_t remaining = estimatedTotal > elapsed ? estimatedTotal - elapsed : 0;

							NXBasics::Runtime_PrintString(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCA), playerName, percent, transferred >> 10, size >> 10, elapsed / 60u, elapsed % 60u, remaining / 60u, remaining % 60u);
						}
						else if (transfer.state == 14)
						{
							NXBasics::Runtime_PrintString(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCB), playerName);
						}

						drawText(bitmap, font, 56, y, text);
						y = signedWord(static_cast<std::uint32_t>(y) + 20u);
					}

					// Release all twenty copied subobjects in reverse construction order.
					for (std::uint32_t index = 20; index != 0; --index)
					{
						auto& transfer = transfers[index - 1];

						if (transfer.fileName != nullptr)
						{
							NXBasics::Runtime_Free(transfer.fileName);
							transfer.fileName = nullptr;
						}

						if (transfer.path24 != nullptr)
						{
							NXBasics::Runtime_Free(transfer.path24);
							transfer.path24 = nullptr;
						}
					}
				}
			}
			else
			{
				char text[100];
				std::strcpy(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCD));

				const std::size_t receivingFlag = userMap ? 5 : 2;
				const std::size_t finishedFlag = userMap ? 6 : 3;

				if (transferFlags[receivingFlag] != 0 && transferFlags[finishedFlag] == 0 && fileTransferManager != nullptr && fileTransferManager->serviceState == 1)
				{
					NNetwork::SFileTransferInfo* info = fileTransferManager->GetTransferInfo(&transferId);

					if (info != nullptr)
					{
						if (info->fileSize == 0)
						{
							std::strcpy(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCE));
						}
						else
						{
							const std::uint32_t percent = static_cast<std::uint32_t>((std::min)(std::uint64_t(100), std::uint64_t(info->transferredBytes) * 100 / info->fileSize));
							const std::uint32_t elapsed = (::timeGetTime() - transferStartTick) / 1000u;
							std::uint32_t estimatedTotal = percent != 0 ? (elapsed * 100u) / percent : 3600u;

							if (estimatedTotal >= 3600u)
								estimatedTotal = 3600u;

							const std::uint32_t remaining = estimatedTotal > elapsed ? estimatedTotal - elapsed : 0;

							NXBasics::Runtime_PrintString(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xCF), percent, info->transferredBytes >> 10, info->fileSize >> 10, elapsed / 60u, elapsed % 60u, remaining / 60u, remaining % 60u);
						}
					}
				}

				if (transferFlags[finishedFlag] != 0)
					std::strcpy(text, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xD0));

				drawText(bitmap, font, 56, y, text);
			}

			::Sleep(50);
		};

		{
			const std::int32_t width = signedWord(readWord(desktop(), 0x10) - 370u);
			const NXBasics::SRectangle rectangle(360, informationY, width, 80);
			NXBasics::CBitmap informationBitmap(desktop(), rectangle);

			informationBitmap.Tool_Darken();
			NC2GuiToolsBase::DrawTool_DrawFrame(&informationBitmap, informationBitmap.GetRectangle(), 2);

			::GetNetworkInstance();

			char statusText[100];
			statusText[0] = '\0';

			const std::uint32_t flags = NNetwork::Network_GetFlags();

			const auto appendStatus = [&] (std::uint32_t flag, std::uint32_t textId)
			{
				if ((flags & flag) == 0)
					return;

				if (statusText[0] != '\0')
					std::strcat(statusText, ", ");

				std::strcat(statusText, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(textId));
			};

			appendStatus(1, 0xD6);
			appendStatus(4, 0xD7);
			appendStatus(8, 0xD8);
			appendStatus(2, 0xD9);

			if (statusText[0] == '\0')
				std::strcat(statusText, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xDA));

			drawFormatted(&informationBitmap, informationFont, 4, 2, "%s: <%s>", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xBB), statusText);

			std::uint32_t y = 14;

			for (std::uint32_t index = 0; index < errorCount; ++index)
			{
				drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), "%s %d: <%s>", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xBC), index, errorTexts[index]);
				y += 12;
			}

			if (currentScreen == 13 && Network_SelectedProviderIsTcpIp())
			{
				const char* sessionName = Network_GetSessionName();

				drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), "%s: <%s>", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xBD), sessionName);
				y += 12;
			}

			std::strcpy(statusText, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xBE));
			std::strcat(statusText, ": ");

			char address[32];
			std::uint32_t addressCount = 0;

			while (NNetwork::GetNetworkIPAddress(addressCount, address, 30))
			{
				if (addressCount != 0)
					std::strcat(statusText, ", ");

				std::strcat(statusText, address);
				++addressCount;
			}

			if (addressCount != 0)
			{
				drawText(&informationBitmap, informationFont, 4, signedWord(y), statusText);
				y += 12;
			}

			if (currentScreen == 14)
			{
				void* mapId = lobbyData + NC2Layout::MenuLobbyOffset::MapGuid;

				if (static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(mapId))->IsValid())
				{
					auto* map = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(*static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(mapId)));

					if (map != nullptr)
					{
						char mapName[100];
						map->Tool_GetMapName(mapName);

						drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), "%s: %s", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC2), mapName);
					}
					else
					{
						drawText(&informationBitmap, informationFont, 4, signedWord(y), NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC3));
					}
				}
				else
				{
					const std::uint32_t mapIdValue = readWord(lobbyData, 0x14);

					if (mapIdValue != 0)
					{
						auto* map = NC2Logic::gSavegamesManager->GetSavegameInfoPtrByUniqueId(mapIdValue);

						if (map != nullptr)
						{
							const char* mapName = map->header.mapName;

							drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), "%s: %s", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC4), mapName);
						}
						else
						{
							drawText(&informationBitmap, informationFont, 4, signedWord(y), NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC5));
						}
					}
					else
					{
						// The original passes this localized text as a format string.
						drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xC6));
					}
				}

				y += 12;

				if (static_cast<const NC2Logic::SCleanMapGuid*>(static_cast<const void*>(mapId))->IsValid())
				{
					const std::uint32_t mode = readWord(lobbyData, 0x15C);

					if (mode <= 2)
					{
						const char* modeText = NC2GuiToolsBase::StringTool_GetOdin001StringPtr(0x1B + mode * 2u);

						if (modeText != nullptr)
							drawFormatted(&informationBitmap, informationFont, 4, signedWord(y), "%s: %s", NC2GuiToolsBase::StringTool_GetOdin001StringPtr(0x1A), modeText);
					}
				}
			}

			if (currentScreen == 16)
				drawTransferProgress(false);

			if (currentScreen == 17)
				drawTransferProgress(true);
		}

		if (currentScreen == 14 || currentScreen == 16 || currentScreen == 17)
		{
			const std::int32_t width = signedWord(readWord(desktop(), 0x10) - 370u);
			const NXBasics::SRectangle rectangle(360, chatY, width, 120);
			NXBasics::CBitmap chatBitmap(desktop(), rectangle);

			chatBitmap.Tool_Darken();
			NC2GuiToolsBase::DrawTool_DrawFrame(&chatBitmap, chatBitmap.GetRectangle(), 2);

			auto* font = guiResources.font08;

			drawText(&chatBitmap, font, 4, 2, OriginalStrings::RightArrow);
			drawText(&chatBitmap, font, 14, 2, textInput.GetString());
			drawCursor(font, 14, 4, 12, &chatBitmap);

			font = guiResources.font08;
			std::uint32_t y = 14;
			const std::byte* recordText = chatHistoryStorage + 8;

			for (std::uint32_t index = 0; index < readWord(chatHistoryStorage, 0); ++index)
			{
				NXBasics::SColorRGB colour(0, 0, 0);
				std::memcpy(&colour, defaultColour(), sizeof(colour));

				const std::uint32_t count = readWord(chatHistoryStorage, 0);
				const std::uint32_t type = index < count ? readWord(recordText - 4, 0) : 0;

				if (type == 1)
					std::memcpy(&colour, reinterpret_cast<const std::byte*>(guiResources.fontDimmedPalette) + 0x400, sizeof(colour));
				else if (type == 2)
					std::memcpy(&colour, reinterpret_cast<const std::byte*>(guiResources.fontRedPalette) + 0x400, sizeof(colour));

				const char* text = index < count ? reinterpret_cast<const char*>(recordText) : nullptr;

				chatBitmap.Text_Print(font, &colour, 4, signedWord(y), text);

				y += 10;
				recordText += 0x74;
			}
		}
	}
}
