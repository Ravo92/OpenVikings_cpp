#include "SkirmishMenu.h"
#include "GameSetupMenu.h"
#include "GameSetupSettings.h"
#include "C2CampaignStaticDataManager.h"
#include "C2CleanMapsManager.h"
#include "C2ControlStartup.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2SavegamesManager.h"
#include "GameProfile.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XIniFileReader.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"
#include "XTextEncoding.h"

#include <cstring>

namespace
{
	using namespace NC2Lifetime;
	// Complete Windows token table 004F7E58..004F7E77 and state 005694E8..005694EF.
	const NXBasics::SToken gMultiplayerMapTokens[] = {
		{ "playeroption", SwitchIds::CampaignMenuToken::Playeroption }, { "playerhideinmenu", SwitchIds::CampaignMenuToken::Playerhideinmenu }, { "playerfixcolors", SwitchIds::CampaignMenuToken::Playerfixcolors }, { nullptr, 0 }
	};

	struct SMultiplayerMapTokenState
	{
		const NXBasics::SToken* table;
		std::uint32_t flags;
	};

	SMultiplayerMapTokenState gMultiplayerMapTokenState{ };
	// Complete Windows static region 005694F0..00569504; Mac names it lastKeysBuffer.
	char lastKeysBuffer[21]{ };
	static_assert(sizeof(gMultiplayerMapTokens) == 4 * sizeof(NXBasics::SToken));
	static_assert(sizeof(SMultiplayerMapTokenState) == (sizeof(void*) == 8 ? 16 : 8));

	// Original function: FUN_004d32f4; Windows CD-identification predicate.
	bool MainMenu_HasGameCD()
	{

		return NMasterControlProgram::FindGameCDDrive() != 0;
	}

	// Original function: FUN_004d8454; empty token-list exit callback.
	int __cdecl DestroyMultiplayerMapTokenList()
	{

		return 0;
	}

	// Source helpers for Windows inline row stores and 0x168-byte kick messages.
	std::uint32_t FixedPlayerColor(std::uint32_t slot)
	{

		return slot < 10 ? slot : 9;
	}

	void SendKickToPlayer(std::uint32_t player)
	{
		std::byte message[NC2Layout::RecordSize::MenuNetworkMessage]{ };
		Write<std::uint32_t>(message, NC2Layout::MenuNetworkMessageOffset::Type, 3);
		NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, player, message, sizeof(message));
		GetNetworkInstance()->KickPlayer(player);
	}
}

namespace
{
	// Original function: FUN_004db3a5; remaster reads Unicode clipboard text.
	bool CopyClipboardText(char* destination, std::int32_t capacity)
	{
		if (destination == nullptr || capacity <= 1 || !::OpenClipboard(nullptr))
			return false;

		const HANDLE handle = ::GetClipboardData(CF_UNICODETEXT);
		const auto size = handle != nullptr ? ::GlobalSize(handle) : 0;
		const auto text = size >= sizeof(wchar_t) ? static_cast<const wchar_t*>(::GlobalLock(handle)) : nullptr;
		bool copied = false;
		// Remaster: clipboard text can be movable memory; bound the terminator search.

		if (text != nullptr)
		{
			std::size_t count = 0;

			while (count < size / sizeof(wchar_t) && text[count] != 0)
				++count;

			if (count != 0 && count < size / sizeof(wchar_t))
			{
				const auto utf8 = NXBasics::Text_FromWide(text, count);
				NXBasics::Text_Copy(destination, static_cast<std::size_t>(capacity), utf8.c_str());
				copied = destination[0] != 0;
			}

			::GlobalUnlock(handle);
		}

		::CloseClipboard();

		return copied;
	}
}

// Original function: FUN_004db0d1.
bool __fastcall MainMenuGuiManagerPrimaryMessageHandlerElement_XGui_BE_Message_Handle(void* element, void* unusedEdx, std::uint32_t message, std::uint32_t key, std::uintptr_t characterPayload, std::uintptr_t param3Payload, std::uintptr_t param4Payload)
{
	const auto character = static_cast<std::uint32_t>(characterPayload);
	const auto param3 = static_cast<std::uint32_t>(param3Payload);
	const auto param4 = static_cast<std::uint32_t>(param4Payload);
	using NC2MainMenuGuiManager::CGuiManager;

	if (message != 0x200)
		return true;

	if (NRemaster::GameSetup::Menu::HandleDropdownKey(*CGuiManager::sTheObjectPtr, key))
		return true;

	if ((NC2Lifetime::Read<std::uint32_t>(NXSysKeyManager::CKeyManager::sTheObjectPtr, NC2Layout::KeyManagerOffset::ModifierFlags) & 2) != 0 && key == 0x56)
	{
		char buffer[100];

		if (CopyClipboardText(buffer, sizeof(buffer)))
		{
			const char* cursor = buffer;

			while (*cursor != 0)
				CGuiManager::sTheObjectPtr->StringInput_HandleKeys(0, NXBasics::Utf8_Decode(cursor));
		}

		return true;
	}

	if (CGuiManager::sTheObjectPtr->StringInput_HandleKeys(key, character) || character == 0 || character >= 128)
		return true;

	const char value = static_cast<char>(character);
	std::memmove(lastKeysBuffer + 1, lastKeysBuffer, 19);
	lastKeysBuffer[20] = '\0';
	lastKeysBuffer[0] = value;

	if (NC2Logic::gCampaignStaticDataManager == nullptr || CGuiManager::sTheObjectPtr->currentScreen != SwitchIds::MainMenuScreen::CampaignMap)
		return true;
	// Windows compares case-sensitive prefixes of the newest-first history.
	const char* codes[] = { "esao", "lekaro", "lesniztahcs", "lepmet", "aipmylo", "oarahp", "sitnalta" };
	bool matched = false;

	for (std::uint32_t index = 0; index < 7; ++index)
	{
		if (std::strncmp(lastKeysBuffer, codes[index], std::strlen(codes[index])) == 0)
		{
			NC2Logic::gCampaignStaticDataManager->CampaignSubMission_DoAllow(3, 20 + index * 10, true);
			matched = true;
		}
	}

	if (std::strncmp(lastKeysBuffer, "sursemag", 8) == 0)
	{
		for (std::uint32_t mission = 20; mission <= 80; mission += 10)
		{
			NC2Logic::gCampaignStaticDataManager->CampaignSubMission_DoAllow(3, mission, true);
		}

		matched = true;
	}

	if (matched)
	{
		NC2GuiToolsBase::FXTool_Chat();
		CGuiManager::sTheObjectPtr->Screen_UpdateGui();
		lastKeysBuffer[0] = '\0';
	}

	return true;
}

namespace NC2MainMenuGuiManager
{
	using namespace NC2Lifetime;

	// These network-only method names describe Windows behavior; no matching Mac symbol exists.
	// Original function: FUN_004d71ba.
	bool CGuiManager::Network_SelectServiceProvider(std::uint32_t index)
	{
		if (index >= Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount))
		{
			Debug_ErrorString_Add("Select service provider - id out of range");
			return false;
		}

		Write(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex, index);

		return true;
	}

	// Original function: FUN_004d71dc; only the -1 sentinel is checked.
	const GUID* CGuiManager::Network_GetSelectedServiceProvider() const
	{
		const auto index = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex);

		return index == NXBasics::IntegerRange::UInt32Maximum ? nullptr : reinterpret_cast<const GUID*>(reinterpret_cast<const std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + index * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned);
	}

	// Original function: FUN_004d787b.
	void CGuiManager::Network_SetAllowConnections(bool allow)
	{
		GetNetworkInstance()->SetAllowConnections(allow);
		Screen_UpdateGui();
	}

	// Original function: FUN_004d78a2; send the original kick message to a selected remote player.
	void CGuiManager::Network_KickSelectedPlayer()
	{
		const auto player = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId);

		if (player == 0)
			return;

		const auto index = Network_GetPlayerListIndex(player);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return;

		if (Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerHostFlag + index * sizeof(NC2MainMenuGuiManager::SMenuNetworkPlayerStorage)) != 0)
		{

			return;
		}

		SendKickToPlayer(player);
	}

	// Original function: FUN_004d7922.
	void CGuiManager::Network_KickUnassignedPlayers()
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
		{
			const auto player = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * sizeof(NC2MainMenuGuiManager::SMenuNetworkPlayerStorage));
			std::uint32_t slot = 0;

			while (slot < 20 && Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) != player)
				++slot;

			if (slot == 20)
				SendKickToPlayer(player);
		}
	}

	// Original function: FUN_004d7d19.
	void CGuiManager::Network_SelectPlayer(std::uint32_t playerId)
	{
		const auto index = Network_GetPlayerListIndex(playerId);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
		{
			Write<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, 0);
		}
		else
		{
			Write(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, playerId);
			Screen_UpdateGui();
		}
	}

	// Original function: FUN_004d7e5d; slot bounds remain unchecked.
	void CGuiManager::Network_AssignSelectedPlayer(std::uint32_t slot)
	{
		const auto player = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId);

		if (player == 0 || (Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerOptions + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) & 2) == 0)
		{

			return;
		}

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			const auto offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;

			if (Read<std::uint32_t>(this, offset) != Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId))
				continue;

			Write<std::uint32_t>(this, offset, 0);
			Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 0);

			if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0)
				Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(index) : 0u);

			if (index != slot && (Read<std::uint8_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Options) & 4) != 0)
			{
				if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0)
					Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(index) : Network_FindNextPlayerColor(NXBasics::IntegerRange::UInt32Maximum));

				Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 2);
			}
		}

		const auto offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
		Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 0);

		if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0)
			Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : Network_FindNextPlayerColor(NXBasics::IntegerRange::UInt32Maximum));

		Write(this, offset, Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId));
		Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 1);
		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d7f7e.
	void CGuiManager::Network_ChangePlayerType(std::uint32_t slot)
	{
		const auto offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
		const auto type = Read<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type);
		const auto flags = Read<std::uint8_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Options);

		if (type == 1)
		{
			bool changed = false;

			if ((flags & 4) != 0)
			{
				Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 2);
				Write<std::uint32_t>(this, offset, 0);
				changed = true;
			}

			if ((flags & 1) != 0)
			{
				Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 0);
				Write<std::uint32_t>(this, offset, 0);
				changed = true;
			}

			if (!changed)
				return;
		}
		else if (type == 2)
		{
			if ((flags & 1) == 0)
				return;

			Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 0);
			Write<std::uint32_t>(this, offset, 0);
		}
		else
		{
			if ((flags & 4) == 0)
			{

				return;
			}

			if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0)
				Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : Network_FindNextPlayerColor(NXBasics::IntegerRange::UInt32Maximum));

			Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 2);
			Write<std::uint32_t>(this, offset, 0);
		}

		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d804f.
	void CGuiManager::Network_ChangePlayerColor(std::uint32_t slot)
	{
		if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) != 0)
		{

			return;
		}

		const auto color = Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : Network_FindNextPlayerColor(Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerColor + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned));
		Write(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerColor + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned, color);
		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d80a7; a full cycle returns color zero.
	std::uint32_t CGuiManager::Network_FindNextPlayerColor(std::uint32_t color) const
	{
		const auto stop = color == NXBasics::IntegerRange::UInt32Maximum ? 9u : color;

		for (;;)
		{
			++color;

			if (color == 10)
				color = 0;

			if (color == stop)
				return 0;

			bool used = false;

			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto offset = NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerType + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
				const auto type = Read<std::uint32_t>(this, offset);

				if ((type == 1 || type == 2) && (Read<std::uint8_t>(this, offset + NC2Layout::MenuLobbyPlayerTypeViewOffset::OptionsHighByte) & NXBasics::BitMask::Bit7Signed) == 0 && Read<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerTypeViewOffset::Color) == color)
				{
					used = true;
					break;
				}
			}

			if (!used)
			{

				return color;
			}
		}
	}

	// Original function: FUN_004d80f6.
	void CGuiManager::Network_SetLobbySelection(std::uint32_t selection, bool keepData)
	{
		if (selection == lobbyValue)
		{

			return;
		}

		lobbyValue = selection;

		if (selection == 2)
			NC2Logic::gSavegamesManager->SavegameList_Update(2, true);

		if (!keepData)
			std::memset(lobbyData, 0, sizeof(lobbyData));

		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d814c.
	void CGuiManager::Network_SetLobbyDifficulty(std::uint32_t difficulty)
	{
		Write(this, NC2Layout::MainMenuManagerOffset::LobbyDifficulty, difficulty);
		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d81c7.
	void CGuiManager::Network_SelectMap(const NC2Logic::SCleanMapGuid& guid)
	{
		NRemaster::GameSetup::Reset();

		if (NRemaster::GameSetup::IsLocal(*this))
			Write<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors, 0);

		const auto difficulty = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyDifficulty);
		std::memset(lobbyData, 0, sizeof(lobbyData));
		Write<std::uint32_t>(lobbyData, NC2Layout::MenuLobbyOffset::Mode, 1);
		std::memcpy(lobbyData + NC2Layout::MenuLobbyOffset::MapGuid, &guid, sizeof(guid));

		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			Write(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerColor + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : 0u);
		}

		auto* map = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(guid);
		void* archive = nullptr;

		if (map->userMap != 0)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::SArchiveState), 1);
			archive = allocation == nullptr ? nullptr : NXBasics::Archive_Initialize(allocation, map->archiveFilename);
			NXBasics::CFile::FileSystem_FileLibrary_Add(archive);
		}

		NXBasics::CFile::FileSystem_SystemPathName_Set("$maproot$", map->mapRoot);
		{
			NXBasics::CIniFileReader reader("$maproot$\\map.ini", nullptr, false, nullptr, nullptr);
			NXBasics::CFile::FileSystem_SystemPathName_Clear("$maproot$");

			if (reader.IsLoaded())
				NRemaster::GameSetup::ReadMapMetadata(reader);

			if (reader.IsLoaded() && reader.SearchGroup("multiplayer"))
			{
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gMultiplayerMapTokenState.flags & 1u) == 0)
					{
						gMultiplayerMapTokenState.flags |= 1u;
						gMultiplayerMapTokenState.table = gMultiplayerMapTokens;
						NXBasics::Runtime_AtExit(DestroyMultiplayerMapTokenList);
					}

					NXBasics::CTokenList tokens(gMultiplayerMapTokenState.table);

					switch (tokens.FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::CampaignMenuToken::Playeroption:
						{
							const auto slot = reader.GetInt();

							if (slot >= 20)
								break;

							std::uint32_t flags = 0;

							while (reader.GetTypeOfNextParameter() == 3)
							{
								const auto option = static_cast<std::uint32_t>(reader.GetInt());

								if (option <= 2)
									flags |= 1u << option;
							}

							Write(this, (static_cast<std::uint32_t>(slot) + NC2Layout::MenuLobbyScaledOffset::OptionsIndexBiasUnsigned) * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned, flags);
							break;
						}
						case SwitchIds::CampaignMenuToken::Playerhideinmenu:
						{
							const auto slot = reader.GetInt();

							if (slot < 20)
							{
								const auto offset = (static_cast<std::uint32_t>(slot) + NC2Layout::MenuLobbyScaledOffset::OptionsIndexBiasUnsigned) * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned + NC2Layout::MenuLobbyOptionsViewOffset::HighByteUnsigned;
								Write<std::uint8_t>(this, offset, Read<std::uint8_t>(this, offset) | NXBasics::BitMask::Bit7Signed);
							}

							break;
						}
						case SwitchIds::CampaignMenuToken::Playerfixcolors:
							Write<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors, reader.GetInt() != 0);
							break;
					}
				}
			}

			if (map->userMap != 0)
				NXBasics::CFile::FileSystem_FileLibrary_Remove(archive);
		}

		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;

			if ((Read<std::uint8_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Options) & 4) == 0)
				continue;

			Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : Network_FindNextPlayerColor(NXBasics::IntegerRange::UInt32Maximum));
			Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 2);
			Write<std::uint32_t>(this, offset, 0);
		}

		Write(this, NC2Layout::MainMenuManagerOffset::LobbyDifficulty, difficulty);
		NRemaster::GameSetup::AutoAssignPlayers(*this);
		Network_SendLobbyData();
	}

	// Original function: FUN_004d8455.
	void CGuiManager::Network_SelectSavegame(std::uint32_t id)
	{
		NRemaster::GameSetup::Reset();
		std::memset(lobbyData, 0, sizeof(lobbyData));
		Write<std::uint32_t>(lobbyData, NC2Layout::MenuLobbyOffset::Mode, 2);
		Write(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId, id);
		auto* saved = NC2Logic::gSavegamesManager->GetSavegameInfoPtrByUniqueId(id);
		NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(saved->header.mapGuid);

		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto offset = NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
			Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : 0u);

			if (saved->header.multiplayer == 0)
				continue;

			const auto type = saved->header.players[slot].playerType;

			if (type == 1 || type == 2)
			{
				Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Type, 2);
				Write<std::uint32_t>(this, offset + NC2Layout::MenuLobbyPlayerOffset::Options, type == 1 ? 6 : 4);
			}

			Write(this, offset + NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? FixedPlayerColor(slot) : saved->header.players[slot].colorId);
		}

		if (saved->header.multiplayer != 0)
		{
			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto& player = saved->header.players[slot];

				if (player.playerType != 1 || std::strlen(player.name) == 0)
					continue;

				for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
				{
					if (NXBasics::Runtime_CompareStringsIgnoreCase(reinterpret_cast<const char*>(this) + NC2Layout::MainMenuManagerOffset::PlayerRows + index * sizeof(NC2MainMenuGuiManager::SMenuNetworkPlayerStorage), player.name) != 0)
						continue;

					Write(this, NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned, Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * sizeof(NC2MainMenuGuiManager::SMenuNetworkPlayerStorage)));
					Write<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerType + slot * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned, 1);
					break;
				}
			}
		}

		Screen_UpdateGui();
		Network_SendLobbyData();
	}
}

// Original function: FUN_004d6bbb; CGuiManagerDefaultMessageHandlerElement::XGui_BE_Message_Handle.
bool __fastcall MainMenuGuiManagerDefaultMessageHandlerElement_XGui_BE_Message_Handle(void* /*element*/, void* /*unusedEdx*/, std::uint32_t message, std::uint32_t command, std::uintptr_t /*sender*/, std::uintptr_t data, std::uintptr_t /*extra*/)
{
	using namespace NC2Lifetime;
	using namespace NC2MainMenuGuiManager;
	using NMasterControlProgram::CMasterControlProgram;
	using NMasterControlProgram::TProgramState;

	if (message != 0x800)
		return true;

	auto* gui = CGuiManager::sTheObjectPtr;
	auto* master = CMasterControlProgram::sTheObjectPtr;
	auto changeScreen = [gui](std::int32_t screen, bool shutdown = false)
	{
		gui->Screen_ChangeTo(screen, shutdown);
	};
	auto startup = [master]()
	{

		return reinterpret_cast<NC2Logic::SGameControlStartupData*>(reinterpret_cast<std::byte*>(master) + NC2Layout::MasterControlProgramOffset::StartupData);
	};
	auto failNetwork = [gui, &changeScreen](const char* error)
	{
		changeScreen(SwitchIds::MainMenuScreen::NetworkError, true);
		gui->Debug_ErrorString_Add(error);
	};

	const auto numericData = static_cast<std::uint32_t>(data);

	if (NRemaster::GameSetup::Skirmish::HandleCommand(*gui, command, numericData))
		return true;

	if (NRemaster::GameSetup::Menu::HandleCommand(*gui, command, numericData))
		return true;

	switch (command)
	{
		case SwitchIds::MainMenuCommand::ShowExitConfirmation:
			changeScreen(SwitchIds::MainMenuScreen::ExitConfirmation);
			break;
		case SwitchIds::MainMenuCommand::ExitGame:
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("game_start_post_picture_sequence", false, 0, nullptr);
			master->System_ProgramState_ChangeNextTimePossible(TProgramState::TitleScreen, false);
			break;
		case SwitchIds::MainMenuCommand::ShowMainMenu:
			changeScreen(SwitchIds::MainMenuScreen::MainMenu);
			break;
		case SwitchIds::MainMenuCommand::ShowSinglePlayerMenu:
			changeScreen(SwitchIds::MainMenuScreen::SinglePlayerMenu);
			break;
		case SwitchIds::MainMenuCommand::ShowMultiplayerCallsign:
			changeScreen(SwitchIds::MainMenuScreen::EnterCallsign);
			break;
		case SwitchIds::MainMenuCommand::PlayIntro:
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("intro_skip");
			master->System_ProgramState_ChangeNextTimePossible(TProgramState::TitleScreen, false);
			break;
		case SwitchIds::MainMenuCommand::InternetLobby:
			break;
		case SwitchIds::MainMenuCommand::ShowCredits:
			changeScreen(SwitchIds::MainMenuScreen::Credits);
			break;
		case SwitchIds::MainMenuCommand::MainMenuAfterCdCheck:
			if (MainMenu_HasGameCD())
				changeScreen(SwitchIds::MainMenuScreen::MainMenu);

			break;
		case SwitchIds::MainMenuCommand::DisableSinglePlayer:
			CGuiManager::mStaticVars.singlePlayerDisabled = 1;
			changeScreen(SwitchIds::MainMenuScreen::MainMenu);
			break;
		case SwitchIds::MainMenuCommand::ShowMapList:
			changeScreen(SwitchIds::MainMenuScreen::MapList);
			break;
		case SwitchIds::MainMenuCommand::ShowCampaignMap:
			changeScreen(SwitchIds::MainMenuScreen::CampaignMap);
			break;
		case SwitchIds::MainMenuCommand::ShowCampaignMap2:
			changeScreen(SwitchIds::MainMenuScreen::CampaignMapSecond);
			break;
		case SwitchIds::MainMenuCommand::ShowCampaignMap3:
			changeScreen(SwitchIds::MainMenuScreen::CampaignMapThird);
			break;
		case SwitchIds::MainMenuCommand::ShowUserCampaignMenu:
			changeScreen(SwitchIds::MainMenuScreen::UserCampaignMenu);
			break;
		case SwitchIds::MainMenuCommand::ShowUserCampaign00:
			changeScreen(SwitchIds::MainMenuScreen::UserCampaignList00);
			break;
		case SwitchIds::MainMenuCommand::ShowUserCampaign01:
			changeScreen(SwitchIds::MainMenuScreen::UserCampaignList01);
			break;
		case SwitchIds::MainMenuCommand::ShowTutorialMapList:
			changeScreen(SwitchIds::MainMenuScreen::TutorialMapList);
			break;
		case SwitchIds::MainMenuCommand::ShowLoadGame:
			changeScreen(SwitchIds::MainMenuScreen::LoadGame);
			break;
		case SwitchIds::MainMenuCommand::StartSelectedMap:
		case SwitchIds::MainMenuCommand::StartSelectedCampaignMap:
			startup()->Reset();
			Write<std::uint32_t>(startup(), NC2Layout::GameControlStartupOffset::Mode, 1);
			std::memcpy(reinterpret_cast<std::byte*>(startup()) + NC2Layout::GameControlStartupOffset::SingleMapGuid, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(data)), 16);
			// Cultures2.exe 004C5180..004C51C4 starts the selected map in state 11 directly.

			if (command == SwitchIds::MainMenuCommand::StartSelectedCampaignMap && NRemaster::HasDifficultySelection())
			{
				changeScreen(SwitchIds::MainMenuScreen::DifficultyOptions);
			}
			else
			{
				master->System_ProgramState_ChangeNextTimePossible(TProgramState::Game, false);
			}

			break;
		case SwitchIds::MainMenuCommand::LoadSelectedSavegame:
			startup()->Reset();
			Write<std::uint32_t>(startup(), NC2Layout::GameControlStartupOffset::Mode, 2);
			Write<std::uint32_t>(startup(), NC2Layout::GameControlStartupOffset::SavegameId, numericData);
			master->System_ProgramState_ChangeNextTimePossible(TProgramState::Game, false);
			break;
		case SwitchIds::MainMenuCommand::SortSavegamesByDate:
			NC2Logic::gSavegamesManager->SortType_Set(2);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::SortSavegamesByMapName:
			NC2Logic::gSavegamesManager->SortType_Set(1);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::SortSavegamesBySlot:
			NC2Logic::gSavegamesManager->SortType_Set(3);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::DifficultyNormal:
			Write<std::uint32_t>(master, NC2Layout::MasterControlProgramOffset::StartupDifficulty, 0);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::DifficultyEasy:
			Write<std::uint32_t>(master, NC2Layout::MasterControlProgramOffset::StartupDifficulty, 1);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::DifficultyEasiest:
			Write<std::uint32_t>(master, NC2Layout::MasterControlProgramOffset::StartupDifficulty, 2);
			gui->Screen_UpdateGui();
			break;
		case SwitchIds::MainMenuCommand::StartPreGameVideo:
			master->System_ProgramState_ChangeNextTimePossible(TProgramState::PreIngameFmv, false);
			break;
		case SwitchIds::MainMenuCommand::ReturnMainMenuAndShutDownNetwork:
			changeScreen(SwitchIds::MainMenuScreen::MainMenu, true);
			break;
		case SwitchIds::MainMenuCommand::ConfirmCallsign:
			if (std::strlen(NNetwork::Network_GetCallsign()) == 0)
			{
				gui->textInput.StartEditing();
			}
			else
			{
				changeScreen(SwitchIds::MainMenuScreen::SelectServiceProvider);
			}

			break;
		case SwitchIds::MainMenuCommand::SelectServiceProvider:
			if (gui->Network_SelectServiceProvider(numericData))
			{
				changeScreen(SwitchIds::MainMenuScreen::HostOrJoin);
			}
			else
			{
				failNetwork("Could not switch into select host or client mode");
			}

			break;
		case SwitchIds::MainMenuCommand::HostSession:
			if (gui->Network_HostDefaultSession())
			{
				changeScreen(SwitchIds::MainMenuScreen::Lobby);
			}
			else
			{
				failNetwork("Could not switch into select host or client mode");
			}

			break;
		case SwitchIds::MainMenuCommand::PrepareJoinSession:
			if (gui->Network_GetSelectedServiceProvider() == nullptr)
			{
				failNetwork("No service provider selected");
			}
			else if (gui->Network_SelectedProviderIsTcpIp())
			{
				changeScreen(SwitchIds::MainMenuScreen::EnterSessionName);
			}
			else if (gui->Network_StartSessionSearch(gui->Network_GetSessionName()))
			{
				changeScreen(SwitchIds::MainMenuScreen::SessionList);
			}
			else
			{
				failNetwork("Could not switch to enum sessions mode");
			}

			break;
		case SwitchIds::MainMenuCommand::SearchSessions:
			if (gui->Network_StartSessionSearch(gui->Network_GetSessionName()))
			{
				changeScreen(SwitchIds::MainMenuScreen::SessionList);
			}
			else
			{
				failNetwork("Could not switch to enum sessions mode");
			}

			break;
		case SwitchIds::MainMenuCommand::JoinSelectedSession:
			if (gui->Network_JoinSession(numericData))
			{
				changeScreen(SwitchIds::MainMenuScreen::Lobby);
			}
			else
			{
				failNetwork("Could not switch into wait for game start mode");
			}

			break;
		case SwitchIds::MainMenuCommand::LobbySelectMap:
			gui->Network_SetLobbySelection(1, true);
			break;
		case SwitchIds::MainMenuCommand::LobbySelectSavegame:
			gui->Network_SetLobbySelection(2, true);
			break;
		case SwitchIds::MainMenuCommand::LobbyShowPlayers:
			gui->Network_SetLobbySelection(0, true);
			break;
		case SwitchIds::MainMenuCommand::LobbyChooseMap:
			gui->Network_SelectMap(*reinterpret_cast<const NC2Logic::SCleanMapGuid*>(static_cast<std::uintptr_t>(data)));
			gui->Network_SetLobbySelection(0, true);
			break;
		case SwitchIds::MainMenuCommand::LobbyDifficultyNormal:
			gui->Network_SetLobbyDifficulty(0);
			break;
		case SwitchIds::MainMenuCommand::LobbyDifficultyEasy:
			gui->Network_SetLobbyDifficulty(1);
			break;
		case SwitchIds::MainMenuCommand::LobbyDifficultyEasiest:
			gui->Network_SetLobbyDifficulty(2);
			break;
		case SwitchIds::MainMenuCommand::LobbyChooseSavegame:
			gui->Network_SelectSavegame(numericData);
			gui->Network_SetLobbySelection(0, true);
			break;
		case SwitchIds::MainMenuCommand::ToggleConnections:
			gui->Network_SetAllowConnections(!gui->Network_AllowsConnections());
			break;
		case SwitchIds::MainMenuCommand::StartNetworkGame:
		{
			char error[200];

			if (!gui->Network_CanStartGame(error))
			{
				gui->Chat_AddText(2, error);
			}
			else
			{
				GetNetworkInstance()->SetAllowConnections(false);
				gui->Network_KickUnassignedPlayers();
				changeScreen(SwitchIds::MainMenuScreen::PlayerConfiguration);
			}

			break;
		}
		case SwitchIds::MainMenuCommand::SelectNetworkPlayer:
			gui->Network_SelectPlayer(numericData);
			break;
		case SwitchIds::MainMenuCommand::KickPlayer:
			if (NNetwork::Network_IsHost())
				gui->Network_KickSelectedPlayer();

			break;
		case SwitchIds::MainMenuCommand::AssignPlayerSlot:
			if (NNetwork::Network_IsHost())
				gui->Network_AssignSelectedPlayer(numericData);

			break;
		case SwitchIds::MainMenuCommand::ChangePlayerType:
			if (NNetwork::Network_IsHost())
				gui->Network_ChangePlayerType(numericData);

			break;
		case SwitchIds::MainMenuCommand::ChangePlayerColor:
			if (NNetwork::Network_IsHost())
				gui->Network_ChangePlayerColor(numericData);

			break;
	}

	return true;
}