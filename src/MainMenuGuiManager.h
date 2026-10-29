#pragma once

#include <windows.h>

#include "StringInputHandler.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBobManager;
	class CPalette;
	class CBitmap;
}

namespace NC2Logic
{
	struct SCleanMapGuid;
}

namespace NNetwork
{
	class CFileTransferManager;
}

namespace NC2MainMenuGuiManager
{
	bool StartNetworkForMenu(void* guiManager);

	struct SMenuGraphicResources
	{
		NXBasics::CBobManager* menuLogos;           // Original address: 005694c4
		NXBasics::CBobManager* smallMenuLogos;      // Original address: 005694c8
		NXBasics::CPalette* funaticsPalette;        // Original address: 005694cc
		NXBasics::CPalette* campaignMapPalette;     // Original address: 005694d0
		NXBasics::CPalette* campaignButtonsPalette; // Original address: 005694d4
	};

	struct SGuiManagerStaticVars
	{
		std::uint8_t displayInitialized; // Original address: 005694b0
		std::byte padding01[3];
		std::int32_t displayWidth;      // Original address: 005694b4
		std::int32_t displayHeight;     // Original address: 005694b8
		std::uint32_t displayDepth;     // Original address: 005694bc
		std::uint8_t desktopUpdateFlag; // Original address: 005694c0
		std::byte padding11[3];
		SMenuGraphicResources resources;
		std::uint8_t singlePlayerDisabled; // Original address: 005694D8
		std::byte unclassified29[3];
	};

	static_assert(sizeof(SMenuGraphicResources) == 20);
	static_assert(offsetof(SGuiManagerStaticVars, displayHeight) == 0x08);
	static_assert(offsetof(SGuiManagerStaticVars, desktopUpdateFlag) == 0x10);
	static_assert(offsetof(SGuiManagerStaticVars, resources) == 0x14);
	static_assert(offsetof(SGuiManagerStaticVars, singlePlayerDisabled) == 0x28);
	static_assert(sizeof(SGuiManagerStaticVars) == 44);

	// Physical Windows network/lobby records. Runtime access and ownership remain unchanged.
	struct SMenuNamedGuidRowStorage
	{
		char name[100];
		std::byte unresolved64[4];
		GUID guid;
	};

	struct SMenuProviderListStorage
	{
		std::uint32_t count;
		SMenuNamedGuidRowStorage rows[10];
		std::uint32_t selectedIndex;
	};

	struct SMenuSessionListStorage
	{
		std::uint32_t count;
		SMenuNamedGuidRowStorage rows[10];
	};

	struct SMenuNetworkPlayerStorage
	{
		char name[102];
		std::uint8_t host;
		std::byte padding67;
		std::uint32_t networkPlayerId;
	};

	struct SMenuPlayerListStorage
	{
		std::uint32_t count;
		SMenuNetworkPlayerStorage rows[10];
		std::uint32_t changeCount, selectedPlayerId;
	};

	struct SMenuLobbyPlayerStorage
	{
		std::uint32_t networkPlayerId, type, options, color;
	};

	struct SMenuLobbyStorage
	{
		std::uint32_t mode;
		GUID mapGuid;
		std::uint32_t savegameId;
		SMenuLobbyPlayerStorage players[20];
		std::uint8_t fixedPlayerColors;
		std::byte padding159[3];
		std::uint32_t difficulty;
	};

	struct SMenuNetworkMessageStorage
	{
		std::uint32_t type, retainedWord;
		std::byte payload[352];
	};

	class CGuiManager
	{
	  public:
		CGuiManager();
		~CGuiManager();

		void Desktop_Open(std::int32_t width, std::int32_t height, std::uint32_t depth);
		void Desktop_Close();

		static void ls_GraphicData_Load();
		static void ls_GraphicData_Free();

		inline static CGuiManager* sTheObjectPtr = nullptr;
		inline static SGuiManagerStaticVars mStaticVars{ };

		void Exploration_Set(bool enabled);
		void System_UpdateGui();
		void EngineWorldDisplayGuiElement_CenterDisplay() const;
		bool StringInput_HandleKeys(std::uint32_t key, std::uint32_t character);
		void Debug_ErrorString_Add(const char* text);
		void Screen_ChangeTo(std::int32_t screen, bool shutDownNetwork);
		void l_ScreenData_Exit(bool shutDownNetwork);
		void l_ScreenData_Init(std::int32_t screen);
		bool Screen_IsMultiPlayerScreen() const;
		char* Network_GetCallsign();
		char* Network_GetSessionName();
		void Chat_AddText(std::uint32_t type, const char* text);

		void l_Network_Update();
		void l_Network_ProcessDataMessages();
		void l_Transfer_Update();
		void l_Transfer_UpdateDownload(bool userMap);
		void l_Transfer_UpdateUpload(bool userMap);
		void l_Network_Draw();
		NXBasics::CBitmap* Debug_GetDesktopBitmapPtr() const;
		void Screen_UpdateGui();
		void l_ScreenGuiElement_RemoveAll();
		void l_ScreenGuiElement_Add(void* element);
		void ScreenGuiElement_FrameCall_Register(void* element);
		void ScreenGuiElement_FrameCall_UnRegister(void* element);
		void l_ScreenGuiElement_FrameCall_Do();
		bool Network_EnumerateServiceProviders();
		bool Network_HostDefaultSession();
		bool Network_HostSession(const char* sessionName, std::uint32_t maximumPlayers);
		bool Network_StartSessionSearch(const char* address);
		bool Network_JoinSession(std::uint32_t index);
		bool Network_UpdateSessionList();
		bool Network_UpdatePlayerList();
		bool Network_PlayerListHasChanged() const;
		bool Network_SelectedProviderIsTcpIp() const;
		bool Network_SelectServiceProvider(std::uint32_t index);
		const GUID* Network_GetSelectedServiceProvider() const;
		void Network_SetAllowConnections(bool allow);
		void Network_KickSelectedPlayer();
		void Network_KickUnassignedPlayers();
		void Network_SelectPlayer(std::uint32_t playerId);
		void Network_AssignSelectedPlayer(std::uint32_t slot);
		void Network_ChangePlayerType(std::uint32_t slot);
		void Network_ChangePlayerColor(std::uint32_t slot);
		std::uint32_t Network_FindNextPlayerColor(std::uint32_t color) const;
		void Network_SetLobbySelection(std::uint32_t selection, bool keepData);
		void Network_SetLobbyDifficulty(std::uint32_t difficulty);
		void Network_SelectMap(const NC2Logic::SCleanMapGuid& guid);
		void Network_SelectSavegame(std::uint32_t id);
		void Network_SendLobbyData();
		void Network_InitializeCallsign();
		bool Network_AutoHost();
		bool Network_AutoJoin(const char* address);
		void Chat_ClearAll();
		const char* Network_GetPlayerName(std::uint32_t playerId) const;
		void Network_FormatPlayerDescription(std::uint32_t index, char* destination) const;
		void Network_ClearLobbyData();
		bool Network_AllowsConnections() const;
		bool Network_CanStartGame(char* errorText) const;
		std::uint32_t Network_GetPlayerListIndex(std::uint32_t playerId) const;

		// Unresolved fields retain their original address spans.
		std::int32_t currentScreen;              // +0x0000
		void* worldDisplayElement;               // +0x0004; borrowed desktop element.
		std::byte screenElementsListStorage[20]; // +0x0008
		std::byte frameCallListStorage[20];      // +0x001C
		void* playerButtons[10];                 // +0x0030
		std::uint32_t lastInputActivityTick;     // +0x0058
		std::uint8_t suppressPreviewDarkening;   // +0x005C
		std::byte unclassified005D[3];
		std::uint32_t lastFollowTargetUpdateTick;  // +0x0060
		std::uint32_t followTargetUpdateCount;     // +0x0064
		SMenuProviderListStorage serviceProviders; // +0x0068
		std::byte sessionSearchData[102];          // +0x0520
		std::byte padding0586[2];
		SMenuSessionListStorage sessions;
		char sessionName[100];
		std::byte unresolved0AA0[4];
		SMenuPlayerListStorage networkPlayers;                           // +0x0AA4
		std::uint32_t lobbyValue;                                        // +0x0EE8
		std::uint8_t transferFlags[8];                                   // +0x0EEC
		NNetwork::CFileTransferManager* fileTransferManager;             // +0x0EF4
		GUID transferId;                                                 // +0x0EF8
		DWORD transferStartTick;                                         // +0x0F08
		std::uint8_t transferIsHost;                                     // +0x0F0C
		std::byte padding0F0D[3];                                        // +0x0F0D
		std::byte lobbyData[352];                                        // +0x0F10
		std::byte chatHistoryStorage[2324];                              // +0x1070; count followed by 20 records of 0x74 bytes.
		std::uint32_t errorCount;                                        // +0x1984
		char errorTexts[10][202];                                        // +0x1988
		std::byte defaultMessageHandlerStorage[44];                      // +0x216C
		NC2InGameGuiBaseTextInputElement::CStringInputHandler textInput; // +0x2198

	  private:
		static bool __cdecl ls_Logic_CallbackFunction(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
	};

	static_assert(sizeof(void*) == 4);
	static_assert(sizeof(CGuiManager) == 8628);
	static_assert(offsetof(CGuiManager, worldDisplayElement) == 0x04);
	static_assert(offsetof(CGuiManager, screenElementsListStorage) == 0x08);
	static_assert(offsetof(CGuiManager, frameCallListStorage) == 0x1C);
	static_assert(offsetof(CGuiManager, playerButtons) == 0x30);
	static_assert(offsetof(CGuiManager, lastInputActivityTick) == 0x58);
	static_assert(offsetof(CGuiManager, suppressPreviewDarkening) == 0x5C);
	static_assert(offsetof(CGuiManager, lastFollowTargetUpdateTick) == 0x60);
	static_assert(offsetof(CGuiManager, followTargetUpdateCount) == 0x64);
	static_assert(offsetof(CGuiManager, serviceProviders) == 0x68);
	static_assert(offsetof(CGuiManager, networkPlayers) == 0xAA4);
	static_assert(offsetof(CGuiManager, sessionSearchData) == 0x520);
	static_assert(offsetof(CGuiManager, lobbyValue) == 0xEE8);
	static_assert(offsetof(CGuiManager, transferFlags) == 0xEEC);
	static_assert(offsetof(CGuiManager, fileTransferManager) == 0xEF4);
	static_assert(offsetof(CGuiManager, transferId) == 0xEF8);
	static_assert(offsetof(CGuiManager, transferStartTick) == 0xF08);
	static_assert(offsetof(CGuiManager, lobbyData) == 0xF10);
	static_assert(offsetof(CGuiManager, errorCount) == 0x1984);
	static_assert(offsetof(CGuiManager, errorTexts) == 0x1988);
	static_assert(offsetof(CGuiManager, defaultMessageHandlerStorage) == 0x216C);
	static_assert(offsetof(CGuiManager, textInput) == 0x2198);
}