#include "C2GameControlNetwork.h"
#include "C2GameState.h"
#include "C2GameTime.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "C2UserNetworkCommandAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiNetworkInfoWindow.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include <array>
#include <cstdarg>
#include <cstring>
#include <new>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using namespace NC2Logic;
	// The Windows window family has no corresponding class declaration in the Mac header.
	static void* __fastcall NetworkInfoWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall NetworkInfoWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3);
	static void __fastcall NetworkInfoWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall NetworkInfoWindow_ToolTip_Available(void* window, void*, const char** output);
	// Complete Windows virtual table 004F7560..004F75AB.
	static const std::array<std::uintptr_t, 19> gNetworkInfoWindowVirtualTable = []
	{
		std::array<std::uintptr_t, 19> t{ };
		std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
		t[0] = reinterpret_cast<std::uintptr_t>(&NetworkInfoWindow_Delete);
		t[2] = reinterpret_cast<std::uintptr_t>(&NetworkInfoWindow_Message);
		t[3] = reinterpret_cast<std::uintptr_t>(&NetworkInfoWindow_Draw);
		t[13] = reinterpret_cast<std::uintptr_t>(&NetworkInfoWindow_ToolTip_Available);
		return t;
	}();
	static CStaticToolTipHandler* Tooltips(void* window)
	{
		return std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(window) + 0x74));
	}
	static const NXBasics::SColorRGB* White()
	{
		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + 0x400);
	}
	static const char* Text(std::uint32_t id)
	{
		return StringTool_GetGameGuiMiscWindowStringPtr(id);
	}
	static std::uint32_t LocalPlayer()
	{
		return NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetLocalPlayerId();
	}
	// Original function: FUN_004c5219; Windows allocation 0x2F58.
	void* NetworkInfoWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const NXBasics::SRectangle area(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u) / 2, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u) / 2, 500, 420);
		BaseToolWindow_Construct(window, &area);
		new (static_cast<std::byte*>(window) + 0x74) CStaticToolTipHandler;
		std::memset(static_cast<std::byte*>(window) + 0x70, 0, 12008);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gNetworkInfoWindowVirtualTable.data());
		NetworkInfoWindow_l_GuiButtons_Add(window);
		return window;
	}
	// Original function: FUN_004c52aa.
	static void NetworkInfoWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gNetworkInfoWindowVirtualTable.data());
		Tooltips(window)->~CStaticToolTipHandler();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}
	// Original function: FUN_004c528e.
	static void* __fastcall NetworkInfoWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		NetworkInfoWindow_Destruct(window);
		if (flags & 1u) NXBasics::Runtime_Free(window);
		return window;
	}
	// Original function: FUN_004c52ca.
	static bool __fastcall NetworkInfoWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		if (message != 0x800) return message == 0x1000 && command == 0x7EC;
		if (command >= 10000 && command < 10100)
		{
			Write(window, NC2Layout::NetworkInfoWindowOffset::Page, command - 10000);
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			NetworkInfoWindow_l_GuiButtons_Add(window);
		}
		else if (command >= 14000 && command <= 14005)
		{
			constexpr std::uint32_t frequency[6] = { 1,2,3,4,6,12 };
			std::byte packet[20]{ };
			packet[0] = std::byte{ 0x8B };
			Write(packet, NC2Layout::UserNetworkCommandOffset::ObjectId, frequency[command - 14000]);
			UserNetworkCommandManager_AddCommand(gUserNetworkCommandManager, nullptr, packet, true, false);
			gGameTime.autoAdjustNetworkFrequency = 0;
		}
		else if (command == 14010) gGameTime.autoAdjustNetworkFrequency = gGameTime.autoAdjustNetworkFrequency == 0;
		return true;
	}
	// Original function: FUN_004c5f94.
	static bool __fastcall NetworkInfoWindow_ToolTip_Available(void* window, void*, const char** output)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
		BaseElement_GlobalToLocalCoordinates(window, point);
		const auto* caption = Tooltips(window)->Get(point);
		if (caption == nullptr) return false;
		*output = caption;
		return true;
	}
	static void AddTextButton(void* window, const NXBasics::SRectangle& area, const char* caption, std::uint32_t command, bool toggle = false, bool selected = false)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		void* button = memory == nullptr ? nullptr : toggle ? BaseToolTextButton_ConstructToggle(memory, nullptr, &area, caption, command, window, selected) : BaseToolTextButton_Construct(memory, nullptr, &area, caption, command, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);
	}
	// Original function: FUN_004c5fd5.
	void NetworkInfoWindow_l_GuiButtons_Add(void* window)
	{
		NXBasics::SRectangle area(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowMoverButton_Construct(memory, &area, Text(100)));
		area = NXBasics::SRectangle(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowCloserButton_Construct(memory, nullptr, &area));
		const auto page = Read<std::uint32_t>(window, NC2Layout::NetworkInfoWindowOffset::Page);
		area = NXBasics::SRectangle(4, 22, 121, 22);
		for (std::uint32_t index = 0; index < 4; ++index)
		{
			AddTextButton(window, area, Text(101 + index), 10000 + index, true, page == index);
			area.x += area.width + 2;
		}
		if (page == 2)
		{
			NNetwork::Network_IsInSession();
			return;
		}
		if (page != 3 || !NNetwork::Network_IsInSession() || !NNetwork::Network_IsHost()) return;
		area = NXBasics::SRectangle(4, 120, 492, 20);
		AddTextButton(window, area, Text(105), 14010);
		area.y += 34;
		constexpr std::uint32_t frequencies[6] = { 1,2,3,4,6,12 };
		for (std::uint32_t index = 0; index < 6; ++index)
		{
			char caption[200];
			NXBasics::Runtime_PrintString(caption, Text(106), frequencies[index]);
			AddTextButton(window, area, caption, 14000 + index);
			area.y += 24;
		}
	}
	// Original function: FUN_004c6436; native bounded formatting and right alignment.
	static void __cdecl NetworkInfoWindow_PrintRightAligned(void*, NXBasics::CFont* font, const NXBasics::SColorRGB* color, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, const char* format, ...)
	{
		char text[100];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(text, 99, format, arguments);
		va_end(arguments);
		text[99] = 0;
		bitmap->Text_Print(font, color, x - NXBasics::Font_GetPixelWidth(font, text), y, text);
	}
	static void DrawGoals(NXBasics::CBitmap* bitmap)
	{
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		auto* large = CGuiBaseDataManager::mStaticVars.resources.font10;
		auto* manager = gMultiPlayerGoalManager;
		if (Read<std::uint8_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::Enabled) == 0)
		{
			bitmap->Text_PrintV(large, White(), 4, 60, Text(139));
			return;
		}
		bitmap->Text_PrintV(large, White(), 4, 60, "%s:", Text(123));
		std::int32_t y = 85;
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(manager, NC2Layout::MultiPlayerGoalManagerOffset::GoalCount); ++index)
		{
			const auto* goal = static_cast<const std::byte*>(manager) + NC2Layout::MultiPlayerGoalManagerOffset::Goals + index * NC2Layout::RecordSize::MultiPlayerGoalUnsigned;
			const auto type = Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoalOffset::Type);
			if (type == SwitchIds::MultiplayerGoal::Survive || type == SwitchIds::MultiplayerGoal::PlayersWin || type == SwitchIds::MultiplayerGoal::PlayersLose)
			{
				bitmap->Text_Print(font, White(), 4, y, Text(type == SwitchIds::MultiplayerGoal::Survive ? 124 : type == SwitchIds::MultiplayerGoal::PlayersWin ? 128 : 129));
				y += 20;
			}
			else if (type == SwitchIds::MultiplayerGoal::CollectGoods)
			{
				bitmap->Text_PrintV(font, White(), 4, y, "%s: ", Text(125));
				y += 14;
				std::byte goods[NC2Layout::RecordSize::PlayerGoodsCounts]{ };
				Tool_PlayerStatistics_CountGoods(LocalPlayer(), goods);
				for (std::int32_t good = 0; good < Read<std::int32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::RequirementCount); ++good)
				{
					const auto id = Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstGoodId + static_cast<std::uint32_t>(good) * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned), amount = Read<std::uint32_t>(goal, NC2Layout::MultiPlayerGoodsGoalOffset::FirstRequiredAmount + static_cast<std::uint32_t>(good) * NC2Layout::RecordSize::MultiPlayerGoalGoodRequirementUnsigned);
					char text[200];
					NXBasics::Runtime_PrintString(text, "%d %s (%d)", amount, StringAccess_GetGoodNamePtr(id, amount), Read<std::uint16_t>(goods, NC2Layout::PlayerGoodsCountOffset::Amounts + id * NC2Layout::RecordSize::PlayerGoodCountUnsigned));
					bitmap->Text_Print(font, White(), 34, y, text);
					y += 14;
				}
				y += 6;
			}
			else if (type == SwitchIds::MultiplayerGoal::Population)
			{
				std::uint16_t humans[NC2Runtime::HumanStatisticIndex::Entries]{ };
				Tool_PlayerStatistics_CountHumans(LocalPlayer(), humans);
				const bool soldiers = Read<std::uint8_t>(goal, NC2Layout::MultiPlayerPopulationGoalOffset::SoldiersOnly) != 0;
				bitmap->Text_PrintV(font, White(), 4, y, "%s: %d (%d)", Text(soldiers ? 126 : 127), Read<std::uint32_t>(goal, NC2Layout::MultiPlayerPopulationGoalOffset::RequiredCount), humans[soldiers ? NC2Runtime::HumanStatisticIndex::Soldiers : NC2Runtime::HumanStatisticIndex::All]);
				y += 20;
			}
		}
		y += 30;
		bitmap->Text_PrintV(large, White(), 4, y, "%s:", Text(130));
		y += 25;
		struct SRank
		{
			std::uint32_t player;
			std::int32_t key;
		};
		SRank ranking[NC2Runtime::MultiPlayerGoalLimits::Players];
		std::uint32_t count = 0;
		for (std::uint32_t player = 0; player < NC2Runtime::MultiPlayerGoalLimits::Players; ++player)
		{
			const auto* record = static_cast<const std::byte*>(manager) + player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned;
			if (Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags) != 0 || Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags) != 0)
			{
				const auto key = Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags) != 0 ? Read<std::uint32_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::WinOrders) + 1000u : 2000u - (Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags) != 0 ? Read<std::uint32_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::LossOrders) : NXBasics::IntegerRange::UInt32Maximum);
				ranking[count++] = { player, static_cast<std::int32_t>(key) };
			}
		}
		if (count > 1) for (std::uint32_t pass = 0; pass < count - 1; ++pass) for (std::uint32_t index = 0; index < count - 1; ++index) if (ranking[index].key > ranking[index + 1].key)
		{
			const auto saved = ranking[index];
			ranking[index] = ranking[index + 1];
			ranking[index + 1] = saved;
		}
		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto* record = static_cast<const std::byte*>(manager) + ranking[index].player * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned;
			char name[200];
			if (Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags) != 0)
			{
				GameControl_Misc_GetPlayerName(gGameControl, ranking[index].player, false, name);
				bitmap->Text_PrintV(font, White(), 4, y, "#%d: '%s' %s - %s: %d", index + 1, name, Text(131), Text(132), Read<std::uint32_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::WinOrders) + 1u);
				y += 20;
			}
			if (Read<std::uint8_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags) != 0)
			{
				GameControl_Misc_GetPlayerName(gGameControl, ranking[index].player, false, name);
				bitmap->Text_PrintV(font, White(), 4, y, "'%s' %s - %s: %d", name, Text(133), Text(134), Read<std::uint32_t>(record, NC2Layout::MultiPlayerGoalManagerOffset::LossOrders) + 1u);
				y += 20;
			}
		}
		y += 40;
		const auto* local = static_cast<const std::byte*>(manager) + LocalPlayer() * NC2Layout::RecordSize::MultiPlayerGoalPlayerResultUnsigned;
		if (Read<std::uint8_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::WonFlags) != 0)
		{
			bitmap->Text_PrintV(large, White(), 4, y, "%s! %s: %d", Text(135), Text(136), Read<std::uint32_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::WinOrders) + 1u);
			y += 20;
		}
		if (Read<std::uint8_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags) != 0) bitmap->Text_PrintV(large, White(), 4, y, "%s! %s: %d", Text(137), Text(138), Read<std::uint32_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::LossOrders) + 1u);
	}
	// Original function: FUN_004c53c0; all four native pages.
	static void __fastcall NetworkInfoWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		auto* tips = Tooltips(window);
		tips->ClearAll();
		const auto page = Read<std::uint32_t>(window, NC2Layout::NetworkInfoWindowOffset::Page);
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		auto* large = CGuiBaseDataManager::mStaticVars.resources.font10;
		if (page == 0)
		{
			DrawGoals(bitmap);
			return;
		}
		if (page == 1)
		{
			bitmap->Text_Print(font, White(), 270 - NXBasics::Font_GetPixelWidth(font, Text(140)), 60, Text(140));
			bitmap->Text_Print(font, White(), 490 - NXBasics::Font_GetPixelWidth(font, Text(141)), 60, Text(141));
			constexpr std::uint32_t icons[6] = { 85,162,85,162,80,71 };
			constexpr std::int32_t x[6] = { 210,260,330,380,430,480 };
			constexpr std::int32_t tooltipX[6] = { 200,250,312,362,412,462 };
			for (std::uint32_t index = 0; index < 6; ++index)
			{
				DrawTool_DrawIcon(icons[index], bitmap, x[index], 100);
				tips->Add(NXBasics::SRectangle(tooltipX[index], 90, 20, index < 2 ? 16 : 30), Text(142 + index));
			}
			std::int32_t y = 120;
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				const auto* control = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
				if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::Type) != 1) continue;
				char name[200];
				GameControl_Misc_GetPlayerName(gGameControl, player, true, name);
				bitmap->Text_PrintV(large, White(), 4, y, name);
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 220, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::SoldiersDied));
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 270, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::HumansDied) - Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::SoldiersDied));
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 340, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::SoldiersKilled));
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 390, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::CiviliansKilled));
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 440, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::HousesDestroyed));
				NetworkInfoWindow_PrintRightAligned(window, font, White(), bitmap, 490, y, "%d", Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::VehiclesDestroyed));
				y += 20;
			}
			return;
		}
		if (page != 2 && page != 3) return;
		if (!NNetwork::Network_IsInSession())
		{
			bitmap->Text_PrintV(large, White(), 4, 60, Text(122));
			return;
		}
		if (page == 3)
		{
			if (!NNetwork::Network_IsHost())
			{
				bitmap->Text_PrintV(font, White(), 4, 60, Text(115), gGameTime.networkFrequency);
				bitmap->Text_PrintV(font, White(), 4, 75, Text(116));
			}
			else
			{
				bitmap->Text_Print(font, White(), 4, 60, Text(gGameTime.autoAdjustNetworkFrequency ? 117 : 118));
				bitmap->Text_PrintV(font, White(), 4, 75, Text(115), gGameTime.networkFrequency);
				const auto frequency = GameTime_Network_LatencyToFrequency(GameTime_Network_GetHighestStoredLatency());
				const auto latency = GameTime_Network_GetHighestStoredLatency();
				bitmap->Text_PrintV(font, White(), 4, 90, Text(119), latency, frequency);
			}
			return;
		}
		const auto* players = static_cast<const NNetwork::SPlayerInfo*>(gDynamicNetworkData.playerInfoRecords);
		auto* network = GetNetworkInstance();
		std::int32_t y = 60;
		for (std::uint32_t index = 0; index < gDynamicNetworkData.playerCount; ++index)
		{
			const auto& player = players[index];
			char caption[200], converted[200];
			NXBasics::Runtime_PrintString(caption, Text(107), index);
			NNetwork::ConvertNetworkWideToAnsi(converted, player.playerName, static_cast<std::int32_t>(std::size(converted)));
			std::strcat(caption, " ");
			std::strcat(caption, converted);
			std::int32_t gamePlayer = -1;
			for (std::int32_t candidate = 0; candidate < 20; ++candidate)
			{
				const auto* control = static_cast<const std::byte*>(gGameControl) + static_cast<std::uint32_t>(candidate) * NC2Layout::RecordSize::GameControlPlayer;
				if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::NetworkPlayerId) == player.playerId)
				{
					gamePlayer = candidate;
					break;
				}
			}
			if (gamePlayer != -1)
			{
				std::strcat(caption, " - ");
				NXBasics::Runtime_PrintString(converted, Text(108), gamePlayer);
				std::strcat(caption, converted);
			}
			else
			{
				std::strcpy(caption, " - ");
				std::strcat(caption, Text(109));
			}
			// The Windows fallback overwrites the previously formatted name.
			bitmap->Text_Print(font, White(), 4, y, caption);
			y += 12;
			NXBasics::Runtime_PrintString(caption, "%s: %8.8x", Text(110), player.playerId);
			if (player.isHost)
			{
				std::strcat(caption, " - ");
				std::strcat(caption, Text(111));
			}
			if (player.isLocal)
			{
				std::strcat(caption, " - ");
				std::strcat(caption, Text(112));
			}
			bitmap->Text_PrintV(font, White(), 4, y, caption);
			y += 12;
			if (player.playerId != 0)
			{
				*caption = 0;
				if (player.playerId != NNetwork::Network_GetLocalPlayerId())
				{
					NetworkConnectionInfo connection;
					if (network->GetConnectionData(player.playerId, &connection)) NXBasics::Runtime_PrintString(caption, Text(113), connection.dwRoundTripLatencyMS, connection.dwThroughputBPS, connection.dwPeakThroughputBPS);
					else std::strcpy(caption, Text(114));
				}
				bitmap->Text_PrintV(font, White(), 4, y, caption);
				y += 12;
			}
			y += 6;
		}
		bitmap->Text_PrintV(font, White(), 4, 384, "%s: %d", Text(120), gGameTime.tick / 12u);
		bitmap->Text_PrintV(font, White(), 4, 400, "%s: %d", Text(121), Read<std::uint32_t>(gUserNetworkCommandManager, NC2Layout::UserNetworkCommandManagerOffset::LastVerifiedTick) / 12u);
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a23b1.
	void CInGameGuiManager::MiscWindows_NetworkInfoWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7EC, 0, 0, 0) != nullptr) return;
		MiscWindows_CloseAllLargeOnes(true);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(12120, 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiMiscWindows::NetworkInfoWindow_Construct(memory)), false);
	}
	// Original function: FUN_004a2402.
	void CInGameGuiManager::MiscWindows_NetworkInfoWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7EC, 0, 0, 0);
		if (window != nullptr) desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		else MiscWindows_NetworkInfoWindow_Open();
	}
}
