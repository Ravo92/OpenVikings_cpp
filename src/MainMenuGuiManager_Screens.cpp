#include "C2CampaignStaticDataManager.h"
#include "C2CleanMapsManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2SavegamesManager.h"
#include "GameProfile.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "HyperLinkTextSystem.h"
#include "MainMenuGuiManager.h"
#include "MainMenuGuiManager_Access.h"
#include "MasterControlProgram.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XNumericConstants.h"

#include <cstdio>
#include <cstring>

namespace NC2MainMenuGuiManager
{
	using namespace NC2Lifetime;
	using NXBasics::SRectangle;
	using NC2GuiToolsBase::StringTool_GetMainMenuStringPtr;
	using NC2GuiToolsBase::StringTool_GetOdin001StringPtr;
	using NC2GuiToolsBase::StringTool_GetSaga001StringPtr;
	using NC2GuiToolsBase::StringTool_GetUpdate103StringPtr;

	// Complete original region: 00508960..005089CB, terminated by a zero mission.
	struct SCampaignButtonPosition
	{
		std::uint32_t mission; std::int32_t x; std::int32_t y;
	};
	// Campaign node tables. Each original campaign screen owns its own table:
	//   005089F8 (campaign 2, screen 5), 00508A68 (campaign 3, screen 6), 00508AD8 (campaign 4, screen 7).
	// The shared campaign-screen builder selects the table by campaign id.
	// Cultures2.exe 004FC2E0..004FC363, including its sentinel.
	static constexpr SCampaignButtonPosition cCampaignButtonPositionsCampaign1[] = { { 10,290,197 },{ 20,372,276 },{ 30,335,270 },{ 40,418,356 },{ 50,479,342 },{ 60,501,382 },{ 70,497,355 },{ 80,530,421 },{ 100,570,386 },{ 110,438,406 },{ 0,0,0 } };
	static constexpr SCampaignButtonPosition cCampaignButtonPositionsCampaign2[] = { { 10,403,429 },{ 20,375,403 },{ 30,388,326 },{ 40,366,316 },{ 50,377,294 },{ 60,332,179 },{ 70,337,137 },{ 80,383,123 },{ 0,0,0 } };
	static constexpr SCampaignButtonPosition cCampaignButtonPositionsCampaign3[] = { { 10,489,349 },{ 20,603,289 },{ 30,496,281 },{ 40,476,257 },{ 50,475,224 },{ 60,404,230 },{ 70,523,364 },{ 80,406,313 },{ 0,0,0 } };
	static constexpr SCampaignButtonPosition cCampaignButtonPositionsCampaign4[] = { { 10,333,223 },{ 20,392,247 },{ 30,453,209 },{ 40,519,372 },{ 50,470,199 },{ 0,0,0 } };

	static const SCampaignButtonPosition* CampaignButtonPositionsFor(std::uint32_t campaignId)
	{
		switch (campaignId)
		{
			case 1: return cCampaignButtonPositionsCampaign1;
			case 2: return cCampaignButtonPositionsCampaign2;
			case 3: return cCampaignButtonPositionsCampaign3;
			case 4: return cCampaignButtonPositionsCampaign4;
			default: return nullptr;
		}
	}

	static std::int32_t AddCoordinate(std::int32_t value, std::int32_t delta)
	{
		return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) + static_cast<std::uint32_t>(delta));
	}

	static void AdvanceRow(SRectangle& rectangle, std::int32_t gap)
	{
		rectangle.y = AddCoordinate(AddCoordinate(rectangle.y, rectangle.height), gap);
	}

	template<class T> static void* CreateText(const SRectangle& rectangle, T getText, bool headline)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(256, 1);
		void* element = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextElement_Construct(allocation, nullptr, &rectangle, getText(), false, false);
		// The Windows title paths write this even when allocation failed.
		if (headline) Write<std::uint8_t>(element, NC2Layout::GuiTextElementOffset::CenteredFlag, 0);
		return element;
	}

	template<class T> static void* CreateSimpleButton(const SRectangle& rectangle, T getText, std::uint32_t command, void* owner)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		return allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_Construct(allocation, nullptr, &rectangle, getText(), command, owner);
	}

	using TStringGetter = char* (*)(std::uint32_t);
	static void* CreateButton(const SRectangle& rectangle, std::uint32_t caption, std::uint32_t tooltip, std::uint32_t command, void* owner, TStringGetter getString, bool toggle = false, bool selected = false)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
		if (allocation == nullptr) return nullptr;
		const char* tooltipText = getString(tooltip);
		const char* captionText = getString(caption);
		return toggle ? NC2InGameGuiBase::BaseToolTextButton_ConstructToggleWithToolTip(allocation, nullptr, &rectangle, captionText, tooltipText, command, owner, selected) : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &rectangle, captionText, tooltipText, command, owner);
	}

	static void* CreateListBox(const SRectangle& rectangle)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(116, 1);
		return allocation == nullptr ? nullptr : ToolListBoxElement_Construct(allocation, nullptr, &rectangle);
	}

	static void* CreateListController(const SRectangle& rectangle, void* list)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(60, 1);
		return allocation == nullptr ? nullptr : ToolListBoxControllerElement_Construct(allocation, nullptr, &rectangle, list, false, true);
	}

	static void SetButtonValue(void* button, std::uint32_t value)
	{
		Write(button, NC2Layout::GuiTextButtonOffset::MessageParameter1, value);
		Write<std::uint32_t>(button, NC2Layout::GuiTextButtonOffset::MessageParameter2, 0);
	}

	// Original function: FUN_004d3a6d.
	// Complete Windows screen jump table: 004D6A49..004D6AA4.
	void CGuiManager::Screen_UpdateGui()
	{
		l_ScreenGuiElement_RemoveAll();
		const auto halfHeight = mStaticVars.displayHeight / 2;
		const SRectangle upperTitle{ 30, AddCoordinate(halfHeight, -245), 500, 20 };
		const SRectangle upperContent{ 56, AddCoordinate(halfHeight, -210), 280, 20 };
		const SRectangle title{ 30, AddCoordinate(halfHeight, -105), 500, 20 };
		const SRectangle content{ 56, AddCoordinate(halfHeight, -70), 280, 20 };
		const SRectangle back{ 30, AddCoordinate(halfHeight, 220), 100, 30 };
		void* owner = defaultMessageHandlerStorage;
		auto add = [this] (void* element)
		{
			l_ScreenGuiElement_Add(element);
		};
		auto heading = [&] (const SRectangle& rectangle, std::uint32_t id, TStringGetter getter = StringTool_GetMainMenuStringPtr)
		{
			add(CreateText(rectangle, [=]
			{
				return getter(id);
			}, true));
		};
		auto button = [&] (const SRectangle& rectangle, std::uint32_t caption, std::uint32_t tooltip, std::uint32_t command, TStringGetter getter = StringTool_GetMainMenuStringPtr)
		{
			add(CreateButton(rectangle, caption, tooltip, command, owner, getter));
		};
		auto returnToMain = [&]
		{
			button(back, 0x2F, 0x30, 0x138B);
		};
		auto networkBack = [&]
		{
			button(back, 0x46, 0x47, 0x13A0);
		};
		auto attachList = [&] (SRectangle rectangle, void* list)
		{
			add(list);
			rectangle.x = AddCoordinate(AddCoordinate(rectangle.x, rectangle.width), 4);
			rectangle.width = 20;
			add(CreateListController(rectangle, list));
		};
		switch (currentScreen)
		{
			case SwitchIds::MainMenuScreen::MainMenu:
				{
					auto rectangle = content;
					void* element = CreateButton(rectangle, 0, 1, 0x138B, owner, StringTool_GetMainMenuStringPtr);
					Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, mStaticVars.singlePlayerDisabled);
					add(element);
					AdvanceRow(rectangle, 20); button(rectangle, 2, 3, 0x138C);
					AdvanceRow(rectangle, 30); button(rectangle, 6, 7, 0x138D);
					AdvanceRow(rectangle, 10); button(rectangle, 8, 9, 0x138F);
					button(back, 0xA, 0xB, 0x1388);
					break;
				}
			case SwitchIds::MainMenuScreen::ExitConfirmation:
				heading(title, 0x1E);
				button(content, 0x1F, 0x20, 0x1389);
				button(back, 0x14, 0x15, 0x138A);
				break;
			case SwitchIds::MainMenuScreen::SinglePlayerMenu:
				{
					heading(title, 0x28);
					auto rectangle = content;
					if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2)
					{
						// Cultures2.exe 004C0070..004C0231 uses only the main-menu table.
						// Route its actions through the shared engine's semantic command IDs.
						button(rectangle, 0x29, 0x2A, SwitchIds::MainMenuCommand::ShowMapList);
						AdvanceRow(rectangle, 10); button(rectangle, 0x2B, 0x2C, SwitchIds::MainMenuCommand::ShowCampaignMap);
						AdvanceRow(rectangle, 10); button(rectangle, 0x2D, 0x2E, SwitchIds::MainMenuCommand::ShowLoadGame);
					}
					else if (NRemaster::HasMultipleCampaigns())
					{
						// Die Saga single-player menu, original order:
						//   tutorial, free play, 'Nordland', 'Weltwunder', 'Saga', user campaigns,
						//   load game. The campaign captions come from the saga table at 10..19.
						button(rectangle, 0x14, 0x15, SwitchIds::MainMenuCommand::ShowTutorialMapList, StringTool_GetOdin001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 0x29, 0x2A, SwitchIds::MainMenuCommand::ShowMapList);
						AdvanceRow(rectangle, 10); button(rectangle, 10, 11, SwitchIds::MainMenuCommand::ShowCampaignMap, StringTool_GetSaga001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 12, 13, SwitchIds::MainMenuCommand::ShowCampaignMap2, StringTool_GetSaga001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 14, 15, SwitchIds::MainMenuCommand::ShowCampaignMap3, StringTool_GetSaga001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 16, 17, SwitchIds::MainMenuCommand::ShowUserCampaignMenu, StringTool_GetSaga001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 0x2D, 0x2E, SwitchIds::MainMenuCommand::ShowLoadGame);
					}
					else
					{
						button(rectangle, 0x14, 0x15, 0x1394, StringTool_GetOdin001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 0x29, 0x2A, 0x1392);
						AdvanceRow(rectangle, 10); button(rectangle, 0x16, 0x17, 0x1393, StringTool_GetOdin001StringPtr);
						AdvanceRow(rectangle, 10); button(rectangle, 0x2D, 0x2E, 0x1395);
					}
					button(back, 0x14, 0x15, 0x138A);
					break;
				}
			case SwitchIds::MainMenuScreen::MapList:
			case SwitchIds::MainMenuScreen::TutorialMapList:
				{
					heading(title, currentScreen == 4 ? 0x32 : 0x14, currentScreen == 4 ? StringTool_GetMainMenuStringPtr : StringTool_GetOdin001StringPtr);
					if (NC2Logic::gCleanMapsManager != nullptr)
					{
						SRectangle rectangle = content;
						rectangle.width = 350; rectangle.height = 280;
						void* list = CreateListBox(rectangle);
						SRectangle row{ 0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22 };
						NC2Logic::gCleanMapsManager->CleanMapList_Tool_SortByNames();
						NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
						auto mapButton = [&] (NC2Logic::SCleanMapInfoData* map, std::uint32_t command)
						{
							char name[200]; map->Tool_GetMapName(name);
							char tooltip[1] = { 0 };
							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
							void* element = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &row, name, tooltip, command, owner);
							SetButtonValue(element, reinterpret_cast<std::uint32_t>(&map->guid));
							BaseListBoxElement_Element_Add(list, nullptr, element);
							AdvanceRow(row, 2);
						};
						if (currentScreen == 8)
						{
							for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
								if (map->hiddenFromSelection == 0 && (map->mapTypes & 2) != 0 && map->campaignId == 100) mapButton(map, 0x1396);
						}
						else
						{
							const std::uint32_t groups[] = { 2, 5, 4, 6 };
							const std::uint32_t captions[] = { 0x33, 0x18, 0x34, 0x19 };
							for (std::uint32_t group = 0; group < 4; ++group)
							{
								if (NC2Logic::gCleanMapsManager->mMapTypeCounts[groups[group]] == 0) continue;
								row.y = AddCoordinate(row.y, group == 0 ? 10 : (group == 2 ? 0 : 6));
								auto getter = (group == 1 || group == 3) ? StringTool_GetOdin001StringPtr : StringTool_GetMainMenuStringPtr;
								BaseListBoxElement_Element_Add(list, nullptr, CreateText(row, [=]
								{
									return getter(captions[group]);
								}, false));
								AdvanceRow(row, 2);
								for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
									if (map->hiddenFromSelection == 0 && (map->mapTypes & (1u << groups[group])) != 0 && (group < 2 || map->multiplayerOnly == 0)) mapButton(map, 0x1397);
								if (group != 3) row.y = AddCoordinate(row.y, 10);
							}
						}
						attachList(rectangle, list);
					}
					else add(CreateText(content, []
					{
						return "CLEAN MAP MANAGER NOT CREATED!";
					}, false));
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::CampaignMap:
			case SwitchIds::MainMenuScreen::CampaignMapSecond:
			case SwitchIds::MainMenuScreen::CampaignMapThird:
				{
					if (NC2Logic::gCleanMapsManager != nullptr)
					{
						// Original: screens 5, 6 and 7 are three separate handlers, each pinned to one
						// campaign id (Die Saga 004D3167 cmp ecx,2 / 004D32F2 cmp ecx,3 / 004D347D cmp ecx,4),
						// each with its own node table. The remaster keeps the same three screens; the
						// campaign id follows from which of them is showing.
						const std::uint32_t campaignScreenIndex = currentScreen == SwitchIds::MainMenuScreen::CampaignMap ? 0u
							: currentScreen == SwitchIds::MainMenuScreen::CampaignMapSecond ? 1u : 2u;
						const std::uint32_t campaignId = NRemaster::GetCampaignIdForCampaignScreen(campaignScreenIndex);
						if (campaignId == 0) break;
						NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
						for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
						{
							if (map->hiddenFromSelection != 0 || (map->mapTypes & 2) == 0 || map->campaignMapId == 0) continue;
							if (map->campaignId != campaignId) continue;
							if (!NC2Logic::gCampaignStaticDataManager->CampaignSubMission_IsAllowed(map->campaignId, map->campaignMapId)) continue;
							const auto* position = CampaignButtonPositionsFor(campaignId);
							if (position == nullptr) continue;
							while (position->mission != 0 && position->mission != map->campaignMapId) ++position;
							if (position->mission == 0 || position->x == -1) continue;
							const auto x = AddCoordinate(position->x, AddCoordinate(mStaticVars.displayWidth, -800) / 2);
							const auto y = AddCoordinate(position->y, AddCoordinate(mStaticVars.displayHeight, -600) / 2);
							char name[400]; map->Tool_GetMapName(name);
							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
							void* element = nullptr;
							if (allocation != nullptr)
							{
								SRectangle rectangle{ AddCoordinate(x, -10), AddCoordinate(y, -10), 20, 20 };
								element = NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, 0x29, name, 0x1397, owner);
							}
							Write(element, NC2Layout::GuiGfxButtonOffset::MessageParameter1, &map->guid); Write<std::uint32_t>(element, NC2Layout::GuiGfxButtonOffset::MessageParameter2, 0); Write<std::uint8_t>(element, NC2Layout::GuiGfxButtonOffset::ImmediateTooltip, 1);
							add(element);
						}
					}
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::UserCampaignMenu:
				{
					// Original: Die Saga screen 8 lists the user campaign packages. Strings 16..19
					// carry the outer heading and its tooltip; the two packages use 18..21.
					heading(title, 16, StringTool_GetSaga001StringPtr);
					auto rectangle = content;
					button(rectangle, 18, 19, SwitchIds::MainMenuCommand::ShowUserCampaign00, StringTool_GetSaga001StringPtr);
					AdvanceRow(rectangle, 10); button(rectangle, 20, 21, SwitchIds::MainMenuCommand::ShowUserCampaign01, StringTool_GetSaga001StringPtr);
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::UserCampaignList00:
			case SwitchIds::MainMenuScreen::UserCampaignList01:
				{
					// Original: screens 9 and 10 list one user campaign package each, selected by the
					// package's map type bit (004D380C mask 0x80 for campaign00, 004D39E2 mask 1 on
					// byte +0x121 for campaign01). Both are plain packages, so they start like a map.
					const std::uint32_t userScreenIndex = currentScreen == SwitchIds::MainMenuScreen::UserCampaignList00 ? 0u : 1u;
					const std::uint32_t campaignId = NRemaster::GetUserCampaignIdForUserScreen(userScreenIndex);
					// The saga table names the two packages at 18/19 and 20/21.
					heading(title, userScreenIndex == 0 ? 18 : 20, StringTool_GetSaga001StringPtr);
					if (NC2Logic::gCleanMapsManager != nullptr && campaignId != 0)
					{
						SRectangle rectangle = content;
						rectangle.width = 350; rectangle.height = 280;
						void* list = CreateListBox(rectangle);
						SRectangle row{ 0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22 };
						NC2Logic::gCleanMapsManager->CleanMapList_Tool_SortByNames();
						NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
						for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
						{
							// The package's map type bit, exactly as the original tests it.
							if (map->hiddenFromSelection != 0 || (map->mapTypes & (1u << campaignId)) == 0) continue;
							char name[200]; map->Tool_GetMapName(name);
							char tooltip[1] = { 0 };
							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
							void* element = allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &row, name, tooltip, 0x1397, owner);
							SetButtonValue(element, reinterpret_cast<std::uint32_t>(&map->guid));
							BaseListBoxElement_Element_Add(list, nullptr, element);
							AdvanceRow(row, 2);
						}
						attachList(rectangle, list);
					}
					else add(CreateText(content, []
					{
						return "CLEAN MAP MANAGER NOT CREATED!";
					}, false));
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::LoadGame:
				{
					heading(title, 0x35);
					if (NC2Logic::gSavegamesManager != nullptr)
					{
						auto rectangle = content; rectangle.width = 171; rectangle.height = 18;
						const auto sort = NC2Logic::gSavegamesManager->mSortType;
						const std::uint32_t values[] = { 2, 1, 3 };
						for (std::uint32_t index = 0; index < 3; ++index)
						{
							add(CreateButton(rectangle, 0x36 + index * 2, 0x37 + index * 2, 0x1399 + index, owner, StringTool_GetMainMenuStringPtr, true, sort == values[index]));
							if (index != 2) rectangle.x = AddCoordinate(AddCoordinate(rectangle.x, rectangle.width), 2);
						}
						rectangle.x = content.x; AdvanceRow(rectangle, 8); rectangle.width = 496; rectangle.height = 250;
						void* list = CreateListBox(rectangle);
						Tool_Options_FillSavegameListboxForLoading(list, owner, 0x1398, false, 0, false);
						attachList(rectangle, list);
					}
					else add(CreateText(content, []
					{
						return "SAVEGAME MANAGER NOT CREATED!";
					}, false));
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::DifficultyOptions:
				{
					heading(title, 0x1A, StringTool_GetOdin001StringPtr);
					auto rectangle = content;
					for (std::uint32_t index = 0; index < 3; ++index)
					{
						const auto selected = Read<std::uint32_t>(NMasterControlProgram::CMasterControlProgram::sTheObjectPtr, NC2Layout::MasterControlOffset::DifficultyLevel) == index;
						add(CreateButton(rectangle, 0x1B + index * 2, 0x1C + index * 2, 0x139C + index, owner, StringTool_GetOdin001StringPtr, true, selected));
						AdvanceRow(rectangle, 10);
					}
					AdvanceRow(rectangle, 20);
					add(CreateButton(rectangle, 0x21, 0x22, 0x139F, owner, StringTool_GetOdin001StringPtr, true, false));
					returnToMain();
					break;
				}
			case SwitchIds::MainMenuScreen::EnterCallsign:
			case SwitchIds::MainMenuScreen::EnterSessionName:
				{
					heading(title, currentScreen == 9 ? 0x48 : 0x50);
					auto rectangle = content; rectangle.y = AddCoordinate(rectangle.y, 120);
					const auto id = currentScreen == 9 ? 0x49u : 0x51u;
					add(CreateSimpleButton(rectangle, [=]
					{
						return StringTool_GetMainMenuStringPtr(id);
					}, currentScreen == 9 ? 0x13A1 : 0x13A5, owner));
					networkBack();
					break;
				}
			case SwitchIds::MainMenuScreen::SelectServiceProvider:
				{
					heading(title, 0x4A);
					auto rectangle = content; rectangle.width = AddCoordinate(rectangle.width, 100);
					Write<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex, NXBasics::IntegerRange::UInt32Maximum);
					for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount); ++index)
					{
						auto* name = reinterpret_cast<const char*>(this) + 0x6C + index * 0x78u;
						void* element = CreateSimpleButton(rectangle, [=]
						{
							return name;
						}, 0x13A2, owner);
						SetButtonValue(element, index); add(element); AdvanceRow(rectangle, 10);
					}
					networkBack();
					break;
				}
			case SwitchIds::MainMenuScreen::HostOrJoin:
				{
					heading(title, 0x4B);
					auto rectangle = content;
					void* element = CreateButton(rectangle, 0x4C, 0x4D, 0x13A3, owner, StringTool_GetMainMenuStringPtr);
					Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, mStaticVars.singlePlayerDisabled); add(element);
					AdvanceRow(rectangle, 10); button(rectangle, 0x4E, 0x4F, 0x13A4);
					networkBack();
					break;
				}
			case SwitchIds::MainMenuScreen::SessionList:
				{
					heading(title, 0x52);
					auto rectangle = content; rectangle.width = AddCoordinate(rectangle.width, 200);
					if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SessionCount) != 0)
					{
						for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SessionCount); ++index)
						{
							auto* name = reinterpret_cast<const char*>(this) + 0x58C + index * 0x78u;
							void* element = CreateSimpleButton(rectangle, [=]
							{
								return name;
							}, 0x13A6, owner);
							SetButtonValue(element, index); add(element); AdvanceRow(rectangle, 10);
						}
					}
					else
					{
						add(CreateText(rectangle, []
						{
							return StringTool_GetMainMenuStringPtr(0x53);
						}, false)); AdvanceRow(rectangle, 10);
					}
					rectangle.width = AddCoordinate(rectangle.width, 200); rectangle.y = AddCoordinate(rectangle.y, 20);
					char text[100]; NXBasics::Runtime_PrintString(text, StringTool_GetUpdate103StringPtr(0x1E), 1, 0);
					add(CreateText(rectangle, [&]
					{
						return text;
					}, true));
					networkBack();
					break;
				}
			case SwitchIds::MainMenuScreen::Lobby:
				{
					if (lobbyValue == 2)
					{
						heading(upperTitle, 0x78);
						auto rectangle = upperContent; rectangle.width = 227;
						const auto sort = NC2Logic::gSavegamesManager->mSortType;
						const std::uint32_t values[] = { 2, 1, 3 };
						for (std::uint32_t index = 0; index < 3; ++index)
						{
							add(CreateButton(rectangle, 0x36 + index * 2, 0x37 + index * 2, 0x1399 + index, owner, StringTool_GetMainMenuStringPtr, true, sort == values[index]));
							if (index != 2) rectangle.x = AddCoordinate(AddCoordinate(rectangle.x, rectangle.width), 2);
						}
						rectangle = upperContent; rectangle.y = AddCoordinate(rectangle.y, 30); rectangle.width = 660; rectangle.height = 250;
						void* list = CreateListBox(rectangle);
						Tool_Options_FillSavegameListboxForLoading(list, owner, 0x13AE, true, Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId), false);
						attachList(rectangle, list);
						button(back, 0x79, 0x7A, 0x13A9);
						break;
					}
					if (lobbyValue == 1)
					{
						heading(upperTitle, 0x77);
						auto rectangle = upperContent; rectangle.width = 350; rectangle.height = 280;
						void* list = CreateListBox(rectangle);
						if (NC2Logic::gCleanMapsManager != nullptr)
						{
							NC2Logic::gCleanMapsManager->CleanMapList_Tool_SortByNames();
							SRectangle row{ 0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22 };
							NXBasics::CListBase::Iterator iterator(NC2Logic::gCleanMapsManager->mMaps);
							for (std::uint32_t group = 0; group < 2; ++group)
							{
								const auto type = group == 0 ? 4u : 6u;
								if (NC2Logic::gCleanMapsManager->mMapTypeCounts[type] == 0) continue;
								row.y = AddCoordinate(row.y, 6);
								BaseListBoxElement_Element_Add(list, nullptr, CreateText(row, [=]
								{
									return StringTool_GetOdin001StringPtr(0x24 + group);
								}, false));
								AdvanceRow(row, 2);
								for (auto* map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.First()); map != nullptr; map = static_cast<NC2Logic::SCleanMapInfoData*>(iterator.Next()))
								{
									if (map->hiddenFromSelection != 0 || (map->mapTypes != 0 && (map->mapTypes & (1u << type)) == 0)) continue;
									char name[200]; map->Tool_GetMapName(name);
									void* element = CreateSimpleButton(row, [&]
									{
										return name;
									}, 0x13AA, owner);
									SetButtonValue(element, reinterpret_cast<std::uint32_t>(&map->guid));
									if (reinterpret_cast<const NC2Logic::SCleanMapGuid*>(lobbyData + 4)->IsEqualTo(map->guid)) Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 1);
									BaseListBoxElement_Element_Add(list, nullptr, element); AdvanceRow(row, 2);
								}
							}
						}
						attachList(rectangle, list);
						auto difficultyTitle = upperTitle; difficultyTitle.y = AddCoordinate(difficultyTitle.y, 324);
						heading(difficultyTitle, 0x1A, StringTool_GetOdin001StringPtr);
						rectangle = upperContent; rectangle.y = AddCoordinate(rectangle.y, 324);
						for (std::uint32_t index = 0; index < 3; ++index)
						{
							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
							void* element = nullptr;
							if (allocation != nullptr)
							{
								const bool selected = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyDifficulty) == index;
								const char* tooltip = StringTool_GetOdin001StringPtr(0x1C + index * 2);
								const char* caption = StringTool_GetOdin001StringPtr(0x1B + index * 2);
								element = NC2InGameGuiBase::BaseToolTextButton_ConstructToggleWithToolTip(allocation, nullptr, &rectangle, caption, tooltip, 0x13AB + index, owner, selected);
							}
							add(element);
							if (index != 2) AdvanceRow(rectangle, 4);
						}
						button(back, 0x79, 0x7A, 0x13A9);
						break;
					}
					if (lobbyValue != 0) break;
					heading(upperTitle, 0x64);
					auto players = upperContent;
					const auto firstRowY = players.y;
					heading(players, 0x65); players.y = AddCoordinate(players.y, players.height);
					for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
					{
						char description[260]; Network_FormatPlayerDescription(index, description);
						void* element = CreateSimpleButton(players, [&]
						{
							return description;
						}, 0x13B1, owner);
						Write(this, NC2Layout::MainMenuManagerOffset::PlayerButtons + index * NC2Layout::RecordSize::GuiElementPointer, element);
						const auto id = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::RecordSize::MenuNetworkPlayerUnsigned);
						SetButtonValue(element, id);
						if (id == Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId)) Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 1);
						Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, !NNetwork::Network_IsHost());
						add(element); AdvanceRow(players, 5);
					}
					if (NNetwork::Network_IsHost())
					{
						players.y = AddCoordinate(players.y, 5);
						void* element = CreateButton(players, 0x66, 0x67, 0x13B2, owner, StringTool_GetMainMenuStringPtr);
						const auto index = Network_GetPlayerListIndex(Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId));
						if (index == NXBasics::IntegerRange::UInt32Maximum || Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerHostFlag + index * NC2Layout::RecordSize::MenuNetworkPlayerUnsigned) != 0) Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, 1);
						add(element); AdvanceRow(players, 5);
					}
					auto slots = players;
					slots.y = firstRowY; slots.x = AddCoordinate(AddCoordinate(slots.x, slots.width), 20);
					heading(slots, 0x68); slots.y = AddCoordinate(slots.y, slots.height);
					for (std::uint32_t index = 0; index < 20; ++index)
					{
						const auto offset = 0xF28 + index * 0x10u;
						const auto flags = Read<std::uint32_t>(this, offset + NC2Layout::MenuPlayerSlotOffset::Flags);
						if (flags == 0 || (flags & NXBasics::IntegerRange::Bit31) != 0) continue;
						const auto kind = Read<std::uint32_t>(this, offset + NC2Layout::MenuPlayerSlotOffset::Type);
						void* element;
						if (kind == 1)
						{
							char label[260];
							const auto playerId = Read<std::uint32_t>(this, offset);
							const char* name = playerId == 0 ? nullptr : Network_GetPlayerName(playerId);
							if (name != nullptr) std::strcpy(label, name);
							else NXBasics::Runtime_PrintString(label, "<%s>", StringTool_GetMainMenuStringPtr(playerId == 0 ? 0x69 : 0x6A));
							element = CreateSimpleButton(slots, [&]
							{
								return label;
							}, 0x13B3, owner);
						}
						else element = CreateSimpleButton(slots, [=]
						{
							return StringTool_GetMainMenuStringPtr(kind == 2 ? 0x6B : 0x6C);
						}, 0x13B3, owner);
						SetButtonValue(element, index); Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, !NNetwork::Network_IsHost()); add(element);
						auto change = slots; change.x = AddCoordinate(AddCoordinate(change.x, change.width), 10); change.width = 50;
						element = CreateSimpleButton(change, []
						{
							return StringTool_GetMainMenuStringPtr(0x6D);
						}, 0x13B4, owner);
						SetButtonValue(element, index); Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, !NNetwork::Network_IsHost()); add(element);
						if (kind == 1 || kind == 2)
						{
							auto color = slots; color.x = AddCoordinate(AddCoordinate(color.x, color.width), 70); color.width = 50;
							element = CreateSimpleButton(color, []
							{
								return " ";
							}, 0x13B5, owner);
							SetButtonValue(element, index);
							NC2InGameGuiBase::BaseToolTextButton_SetBackgroundColor(element, NC2E2::StaticHumanMisc_ColorIds_GetColorRef(Read<std::uint32_t>(this, offset + NC2Layout::MenuPlayerSlotOffset::ColorId)));
							bool disabled = true;
							if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0 && Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) == 0 && NNetwork::Network_IsHost()) disabled = false;
							Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, disabled); add(element);
						}
						AdvanceRow(slots, 5);
					}
					if (NNetwork::Network_IsHost())
					{
						players.y = AddCoordinate(players.y, 15);
						heading(players, 0x76); players.y = AddCoordinate(players.y, players.height);
						auto selection = players; selection.width = AddCoordinate(selection.width / 2, -2);
						auto choose = [&] (std::uint32_t caption, std::uint32_t command)
						{
							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
							add(allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructToggle(allocation, nullptr, &selection, StringTool_GetMainMenuStringPtr(caption), command, owner, false));
						};
						choose(0x6E, 0x13A7); selection.x = AddCoordinate(AddCoordinate(selection.x, selection.width), 4); choose(0x6F, 0x13A8);
						AdvanceRow(players, 15); heading(players, 0x70); players.y = AddCoordinate(players.y, players.height);
						const auto allowed = Network_AllowsConnections();
						void* element = CreateButton(players, allowed ? 0x73 : 0x71, allowed ? 0x74 : 0x72, 0x13AF, owner, StringTool_GetMainMenuStringPtr);
						if (allowed) Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 1);
						add(element); AdvanceRow(players, 10);
						element = CreateSimpleButton(players, []
						{
							return StringTool_GetMainMenuStringPtr(0x75);
						}, 0x13B0, owner); add(element);
						char error[200];
						if (!sTheObjectPtr->Network_CanStartGame(error))
						{
							Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::DisabledFlag, 1);
							NC2InGameGuiBase::BaseToolTextButton_SetToolTipString(element, error);
						}
					}
					networkBack();
					break;
				}
			case SwitchIds::MainMenuScreen::PlayerConfiguration:
				{
					heading(title, 0x82); auto rectangle = content; heading(rectangle, 0x83);
					rectangle.y = AddCoordinate(rectangle.y, rectangle.height);
					for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
					{
						char description[260]; Network_FormatPlayerDescription(index, description);
						void* element = CreateSimpleButton(rectangle, [&]
						{
							return description;
						}, 0x13B1, owner);
						Write(this, NC2Layout::MainMenuManagerOffset::PlayerButtons + index * NC2Layout::RecordSize::GuiElementPointer, element);
						SetButtonValue(element, Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::RecordSize::MenuNetworkPlayerUnsigned));
						add(element); AdvanceRow(rectangle, 5);
					}
					break;
				}
			case SwitchIds::MainMenuScreen::FileUpload: heading(title, 0x8C); break;
			case SwitchIds::MainMenuScreen::FileDownload: heading(title, 0x23, StringTool_GetOdin001StringPtr); break;
			case SwitchIds::MainMenuScreen::NetworkError: heading(title, 0xA0); networkBack(); break;
			case SwitchIds::MainMenuScreen::Disconnected: heading(title, 0x8D); networkBack(); break;
			case SwitchIds::MainMenuScreen::Credits:
				{
					SRectangle rectangle{ AddCoordinate(AddCoordinate(mStaticVars.displayWidth, -800) / 2, 168), AddCoordinate(AddCoordinate(mStaticVars.displayHeight, -600) / 2, 115), 444, 357 };
					rectangle.Inflate(-3, -3);
					char directory[260]; NC2Logic::LanguageTool_GetDirectoryName("data\\text\\%s\\hypertext\\credits", directory, "index.hlt");
					void* allocation = NXBasics::Runtime_AllocateWithNewHandler(72, 1);
					void* element = allocation == nullptr ? nullptr : HyperLinkTextGuiElementBase_Construct(allocation, nullptr, &rectangle, 3, 0x100, directory, "index.hlt", nullptr, nullptr);
					Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::FillBackground, 0); HyperLinkTextGuiElementBase_SetAutoScrollSpeed(element, nullptr, 20.0f, 1000); Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::WheelScrollingEnabled, 0); add(element);
					button(back, 0x14, 0x15, 0x138A);
					break;
				}
			case SwitchIds::MainMenuScreen::InsertGameCd:
				{
					heading(title, 0xAF); auto rectangle = content; rectangle.y = AddCoordinate(rectangle.y, 100);
					for (std::uint32_t index = 0; index < 2; ++index)
					{
						void* allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
						add(allocation == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &rectangle, StringTool_GetMainMenuStringPtr(0xB0 + index), nullptr, 0x1390 + index, owner));
						if (index == 0) AdvanceRow(rectangle, 30);
					}
					button(back, 0x1F, 0x20, 0x1389);
					break;
				}
			default:
				add(CreateText(title, []
				{
					return "UNKNOWN OR UNIMPLEMENTED SCREEN!";
				}, true));
				add(CreateText(content, []
				{
					return "NOT IMPLEMENTET YET!";
				}, false));
				button(back, 0x14, 0x15, 0x138A);
				break;
		}
	}
}
