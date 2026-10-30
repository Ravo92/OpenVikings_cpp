#include "InGameGuiStorage.h"
#include "XPalette.h"
#include "C2GameControlNetwork.h"
#include "C2GameState.h"
#include "C2GameTime.h"
#include "C2LanguageTools.h"
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
#include "ModernNetwork.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <new>
#include <string>

namespace NC2InGameGuiMiscWindows
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiBase;
	using namespace NC2GuiToolsBase;
	using namespace NC2Logic;
	// The Windows window family has no corresponding class declaration in the Mac header.
	static void* __fastcall NetworkInfoWindow_Delete(void* window, void*, std::uint32_t flags);
	static bool __fastcall NetworkInfoWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload);
	static void __fastcall NetworkInfoWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall NetworkInfoWindow_ToolTip_Available(void* window, void*, const char** output);

	// Complete Windows virtual table 004F7560..004F75AB.
	static const std::array<std::uintptr_t, 19>& NetworkInfoWindowVirtualTable()
	{
		static const std::array<std::uintptr_t, 19> methods = []
		{
			std::array<std::uintptr_t, 19> t{ };
			std::memcpy(t.data(), gBaseToolWindowVirtualTable, sizeof(t));
			t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NetworkInfoWindow_Delete>());
			t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NetworkInfoWindow_Message>());
			t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NetworkInfoWindow_Draw>());
			t[13] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NetworkInfoWindow_ToolTip_Available>());

			return t;
		}();

		return methods;
	}

	static CStaticToolTipHandler* Tooltips(void* window)
	{

		return std::launder(reinterpret_cast<CStaticToolTipHandler*>(static_cast<std::byte*>(window) + offsetof(SNetworkInfoWindowStorage, tooltips)));
	}

	static const NXBasics::SColorRGB* White()
	{

		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	static const char* Text(std::uint32_t id)
	{

		return StringTool_GetGameGuiMiscWindowStringPtr(id);
	}

	static const char* NetworkText(const char* english, const char* german)
	{

		return LanguageTool_GetCurrentLanguage() == 0 ? german : english;
	}

	static std::uint32_t gConnectionPage = 0;

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
		new (static_cast<std::byte*>(window) + offsetof(SNetworkInfoWindowStorage, tooltips)) CStaticToolTipHandler;
		std::memset(static_cast<std::byte*>(window) + offsetof(SNetworkInfoWindowStorage, page), 0, sizeof(SNetworkInfoWindowStorage) - offsetof(SNetworkInfoWindowStorage, page));
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NetworkInfoWindowVirtualTable().data());
		Write<std::uint32_t>(window, NC2Layout::NetworkInfoWindowOffset::Page, 2);
		gConnectionPage = 0;
		NetworkInfoWindow_l_GuiButtons_Add(window);

		return window;
	}

	// Original function: FUN_004c52aa.
	static void NetworkInfoWindow_Destruct(void* window)
	{
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NetworkInfoWindowVirtualTable().data());
		Tooltips(window)->~CStaticToolTipHandler();
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004c528e.
	static void* __fastcall NetworkInfoWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		NetworkInfoWindow_Destruct(window);

		if (flags & 1u)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004c52ca.
	static bool __fastcall NetworkInfoWindow_Message(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (message != 0x800)
			return message == 0x1000 && command == 0x7EC;

		if (command >= 10000 && command < 10004)
		{
			Write(window, NC2Layout::NetworkInfoWindowOffset::Page, command - 10000);
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			NetworkInfoWindow_l_GuiButtons_Add(window);
		}
		else if (command == 14100 || command == 14101)
		{
			if (command == 14100)
			{
				if (gConnectionPage != 0)
					--gConnectionPage;
			}
			else if (gConnectionPage < NNetwork::MaximumNetworkPlayers / 4u)
			{
				++gConnectionPage;
			}
		}

		return true;
	}

	// Original function: FUN_004c5f94.
	static bool __fastcall NetworkInfoWindow_ToolTip_Available(void* window, void*, const char** output)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
		BaseElement_GlobalToLocalCoordinates(window, point);
		const auto* caption = Tooltips(window)->Get(point);

		if (caption == nullptr)
			return false;

		*output = caption;

		return true;
	}

	static void AddTextButton(void* window, const NXBasics::SRectangle& area, const char* caption, std::uint32_t command, bool toggle = false, bool selected = false)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::STextButtonStorage), 1);
		void* button = memory == nullptr ? nullptr : toggle ? BaseToolTextButton_ConstructToggle(memory, nullptr, &area, caption, command, window, selected)
		                                                    : BaseToolTextButton_Construct(memory, nullptr, &area, caption, command, window);
		NXBaseGui::BaseWindow_Element_Add(window, button);
	}

	// Original function: FUN_004c5fd5.
	void NetworkInfoWindow_l_GuiButtons_Add(void* window)
	{
		NXBasics::SRectangle area(2, 2, Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 22, 14);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiWindowMoverStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowMoverButton_Construct(memory, &area, Text(100)));
		area = NXBasics::SRectangle(Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width) - 18, 2, 16, 14);
		memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiBase::SGuiGfxButtonStorage), 1);
		NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : WindowCloserButton_Construct(memory, nullptr, &area));
		const auto page = Read<std::uint32_t>(window, NC2Layout::NetworkInfoWindowOffset::Page);
		area = NXBasics::SRectangle(4, 22, 121, 22);

		for (std::uint32_t index = 0; index < 4; ++index)
		{
			const auto* caption = index == 2 ? NetworkText("Connections", "Verbindungen") : index == 3 ? NetworkText("Automatic", "Automatik")
			                                                                                           : Text(101 + index);
			AddTextButton(window, area, caption, 10000 + index, true, page == index);
			area.x += area.width + 2;
		}

		if (page == 2)
		{
			AddTextButton(window, NXBasics::SRectangle(16, 392, 86, 22), NetworkText("Previous", "Zurück"), 14100);
			AddTextButton(window, NXBasics::SRectangle(398, 392, 86, 22), NetworkText("Next", "Weiter"), 14101);

			return;
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

		if (count > 1)
			for (std::uint32_t pass = 0; pass < count - 1; ++pass)
			{
				for (std::uint32_t index = 0; index < count - 1; ++index)
				{
					if (ranking[index].key > ranking[index + 1].key)
					{
						const auto saved = ranking[index];
						ranking[index] = ranking[index + 1];
						ranking[index + 1] = saved;
					}
				}
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

		if (Read<std::uint8_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::LostFlags) != 0)
			bitmap->Text_PrintV(large, White(), 4, y, "%s! %s: %d", Text(137), Text(138), Read<std::uint32_t>(local, NC2Layout::MultiPlayerGoalManagerOffset::LossOrders) + 1u);
	}

	static std::string FitNetworkCaption(NXBasics::CFont* font, std::string text, std::int32_t width)
	{
		if (NXBasics::Font_GetPixelWidth(font, text.c_str()) <= width)
			return text;

		while (!text.empty() && NXBasics::Font_GetPixelWidth(font, (text + "...").c_str()) > width)
		{
			auto end = text.size() - 1;

			while (end != 0 && (static_cast<unsigned char>(text[end]) & 0xC0u) == 0x80u)
				--end;

			text.resize(end);
		}

		return text + "...";
	}

	static void DrawSynchronizationMode(NXBasics::CBitmap* bitmap)
	{
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		const auto ticks = GameTime_Network_GetTicksPerCycle();
		bitmap->Text_PrintV(font, White(), 16, 60, NetworkText("Automatic mode: %u Hz | %u ticks per cycle", "Automatischer Modus: %u Hz | %u Ticks pro Zyklus"), gGameTime.networkFrequency, ticks);

		if (gGameTime.speed >= 0.01)
		{
			bitmap->Text_PrintV(font, White(), 16, 78, NetworkText("Target cadence: %.1f cycles/s | %.0f ms per cycle", "Zieltakt: %.1f Zyklen/s | %.0f ms pro Zyklus"), gGameTime.speed / ticks, gGameTime.tickDuration * ticks);
		}
		else
		{
			bitmap->Text_Print(font, White(), 16, 78, NetworkText("Simulation paused; connection measurements remain active.", "Simulation pausiert; Verbindungsmessung bleibt aktiv."));
		}
	}

	static void DrawConnectionGraphs(NXBasics::CBitmap* bitmap)
	{
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		auto* peer = GetNetworkInstance()->peer;
		const auto* players = static_cast<const NNetwork::SPlayerInfo*>(gDynamicNetworkData.playerInfoRecords);
		std::array<const NNetwork::SPlayerInfo*, NNetwork::MaximumNetworkPlayers> remote{ };
		std::uint32_t count = 0;

		for (std::uint32_t index = 0; index < gDynamicNetworkData.playerCount && count < remote.size(); ++index)
		{
			if (players[index].playerId != 0 && !players[index].isLocal)
				remote[count++] = &players[index];
		}

		const auto pages = (std::max)(1u, (count + 3u) / 4u);
		gConnectionPage = (std::min)(gConnectionPage, pages - 1);
		bitmap->Text_Print(font, White(), 16, 98, NetworkText("RTT history: last 60 probes | red gaps: no reply", "RTT-Verlauf: letzte 60 Messungen | rote Lücken: keine Antwort"));

		if (count == 0)
		{
			bitmap->Text_Print(font, White(), 16, 142, NetworkText("No remote players connected.", "Keine entfernten Spieler verbunden."));
			bitmap->Text_Print(font, White(), 16, 164, NetworkText("AI players run locally and need no network connection.", "KI-Spieler laufen lokal und benötigen keine Netzwerkverbindung."));
		}

		for (std::uint32_t row = 0; row < 4 && gConnectionPage * 4 + row < count; ++row)
		{
			const auto& player = *remote[gConnectionPage * 4 + row];
			NNetwork::ModernConnectionQuality quality;
			const bool available = peer != nullptr && peer->ConnectionQuality(player.playerId, quality);
			const bool fresh = available && quality.measured && quality.age < 3000u;
			const bool timeout = quality.count != 0 && quality.samples[quality.count - 1] == UINT32_MAX;
			const bool weak = quality.latency > 250 || quality.jitter > 80;
			const auto y = 122 + static_cast<std::int32_t>(row) * 66;
			char name[256]{ };
			NNetwork::ConvertNetworkWideToUtf8(name, player.playerName, static_cast<std::int32_t>(std::size(name)));
			const auto title = FitNetworkCaption(font, std::string(name) + (player.isHost ? " (Host)" : ""), 225);
			bitmap->Text_Print(font, White(), 16, y, title.c_str());
			const auto* status = !available ? NetworkText("Unavailable", "Nicht verfügbar") : !fresh                                    ? NetworkText("Measuring / waiting", "Messung / warte auf Antwort")
			                                                                              : timeout                                     ? NetworkText("Delayed", "Verzögert")
			                                                                              : weak                                        ? NetworkText("Weak", "Schwach")
			                                                                              : quality.latency > 80 || quality.jitter > 30 ? NetworkText("Variable", "Schwankend")
			                                                                                                                            : NetworkText("Good", "Gut");
			NXBasics::SColorRGB color = *White();
			color.red = !fresh || timeout || weak ? 230 : quality.latency > 80 || quality.jitter > 30 ? 235
			                                                                                          : 100;
			color.green = !fresh || timeout || weak ? 100 : 205;
			color.blue = 100;
			bitmap->Text_PrintV(font, &color, 254, y, fresh ? "%s | %u ms" : "%s", status, quality.latency);
			std::uint32_t scale = 100;

			for (std::uint32_t sample = 0; sample < quality.count; ++sample)
			{
				if (quality.samples[sample] != UINT32_MAX)
					scale = (std::max)(scale, ((quality.samples[sample] + 99u) / 100u) * 100u);
			}

			auto grid = *White();
			grid.red = 78;
			grid.green = 83;
			grid.blue = 78;
			constexpr std::int32_t left = 64, right = 482, height = 27;
			const auto top = y + 18, bottom = top + height;

			for (auto line : { top, top + height / 2, bottom })
			{
				bitmap->Draw_Line(left, line, right, line, grid);
			}

			bitmap->Text_PrintV(font, White(), 16, top - 3, "%u", scale);
			bitmap->Text_Print(font, White(), 16, bottom - 7, "0 ms");
			bool previous = false;
			std::int32_t lastX = 0, lastY = 0;

			for (std::uint32_t sample = 0; sample < quality.count; ++sample)
			{
				const auto x = left + static_cast<std::int32_t>(60u - quality.count + sample) * (right - left) / 59;
				const auto latency = quality.samples[sample];

				if (latency == UINT32_MAX)
				{
					auto red = *White();
					red.red = 230;
					red.green = 80;
					red.blue = 80;
					bitmap->Draw_Line(x, top, x, bottom, red);
					previous = false;
					continue;
				}

				const auto position = bottom - static_cast<std::int32_t>(latency * height / scale);

				if (previous)
				{
					bitmap->Draw_Line(lastX, lastY, x, position, color);
				}
				else
				{
					bitmap->Draw_SetPixel(x, position, color);
				}

				previous = true;
				lastX = x;
				lastY = position;
			}

			if (fresh)
			{
				bitmap->Text_PrintV(font, White(), 254, y + 48, NetworkText("Variation: %u ms", "Schwankung: %u ms"), quality.jitter);
			}
			else
			{
				bitmap->Text_Print(font, White(), 254, y + 48, NetworkText("No current RTT available", "Aktuell keine gemessene RTT"));
			}
		}

		bitmap->Text_PrintV(font, White(), 185, 398, NetworkText("Page %u / %u", "Seite %u / %u"), gConnectionPage + 1, pages);
	}

	static void DrawAutomaticSynchronization(NXBasics::CBitmap* bitmap)
	{
		auto* font = CGuiBaseDataManager::mStaticVars.resources.font08;
		bitmap->Text_Print(font, White(), 16, 116, NetworkText("The host chooses a shared mode for all players.", "Der Host wählt einen gemeinsamen Modus für alle Spieler."));
		bitmap->Text_Print(font, White(), 16, 138, NetworkText("RTT, recent peaks and variation determine the reserve.", "RTT, letzte Spitzen und Schwankungen bestimmen die Reserve."));
		bitmap->Text_Print(font, White(), 16, 160, NetworkText("Relayed routes include both connections through the host.", "Weitergeleitete Routen berücksichtigen beide Host-Verbindungen."));
		bitmap->Text_Print(font, White(), 16, 196, NetworkText("Reduce promptly; increase after stable measurements.", "Bei Problemen langsamer; bei stabilen Messungen wieder schneller."));
		bitmap->Text_Print(font, White(), 16, 218, NetworkText("Available modes: 1 / 2 / 3 / 4 / 6 / 12 Hz.", "Verfügbare Modi: 1 / 2 / 3 / 4 / 6 / 12 Hz."));
		bitmap->Text_Print(font, White(), 16, 254, NetworkText("Normal simulation: 12 ticks per second.", "Normale Simulation: 12 Spielticks pro Sekunde."));
		bitmap->Text_Print(font, White(), 16, 276, NetworkText("One synchronization per tick is the useful normal-speed limit.", "Ein Abgleich pro Tick ist bei normalem Tempo die sinnvolle Grenze."));

		if (NNetwork::Network_IsHost())
			bitmap->Text_PrintV(font, White(), 16, 316, NetworkText("Current route budget: %u ms", "Aktuelle Zeitreserve für die Route: %u ms"), GameTime_Network_GetSynchronizationBudget());

		bitmap->Text_Print(font, White(), 16, 352, NetworkText("See Connections for each player's measured history.", "Unter Verbindungen steht der gemessene Verlauf je Spieler."));
	}

	// Original function: FUN_004c53c0; retain goals/statistics, modernize the network pages.
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
			constexpr std::uint32_t icons[6] = { 85, 162, 85, 162, 80, 71 };
			constexpr std::int32_t x[6] = { 210, 260, 330, 380, 430, 480 };
			constexpr std::int32_t tooltipX[6] = { 200, 250, 312, 362, 412, 462 };

			for (std::uint32_t index = 0; index < 6; ++index)
			{
				DrawTool_DrawIcon(icons[index], bitmap, x[index], 100);
				tips->Add(NXBasics::SRectangle(tooltipX[index], 90, 20, index < 2 ? 16 : 30), Text(142 + index));
			}

			std::int32_t y = 120;

			for (std::uint32_t player = 0; player < 20; ++player)
			{
				const auto* control = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;

				if (Read<std::uint8_t>(control, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint32_t>(control, NC2Layout::GameControlPlayerOffset::Type) != 1)
					continue;

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

		if (page != 2 && page != 3)
			return;

		if (!NNetwork::Network_IsInSession())
		{
			bitmap->Text_PrintV(large, White(), 4, 60, Text(122));

			return;
		}

		DrawSynchronizationMode(bitmap);

		if (page == 2)
		{
			DrawConnectionGraphs(bitmap);
		}
		else
		{
			DrawAutomaticSynchronization(bitmap);
		}
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a23b1.
	void CInGameGuiManager::MiscWindows_NetworkInfoWindow_Open()
	{
		l_Selection_RemoveAllGuiElements();

		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(0x7EC, 0, 0, 0) != nullptr)
		{

			return;
		}

		MiscWindows_CloseAllLargeOnes(true);
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2InGameGuiMiscWindows::SNetworkInfoWindowStorage), 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(reinterpret_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiMiscWindows::NetworkInfoWindow_Construct(memory)), false);
	}

	// Original function: FUN_004a2402.
	void CInGameGuiManager::MiscWindows_NetworkInfoWindow_Toggle()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;
		auto* window = desktop->Message_SendToAllWindows(0x7EC, 0, 0, 0);

		if (window != nullptr)
		{
			desktop->Window_AddToCloseList(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
		}
		else
		{
			MiscWindows_NetworkInfoWindow_Open();
		}
	}
}