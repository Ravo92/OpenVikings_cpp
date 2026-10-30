#include "XGuiElementAccess.h"
#include "C2CampaignStaticDataManager.h"
#include "C2CleanMapsManager.h"
#include "C2E2AmbientSoundManagerAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "GameProfile.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "MainMenuWorldDisplayElement.h"
#include "SwitchIdentifiers.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XFile.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPaletteAccess.h"
#include "XPicture.h"
#include "XPictureAccess.h"

namespace NC2MainMenuGuiWorldElement
{
	using namespace NC2Lifetime;

	// Original function: FUN_004dac2a.
	void WorldDisplayElement_l_Tool_DarkenBitmap_Exit(void* element)
	{
		auto* palette = Read<NXBasics::CPalette*>(element, NC2Layout::MenuWorldElementOffset::DarkenPalette);

		if (palette != nullptr)
			NXBasics::CPalette::DeleteOriginal(palette, 1);

		Write<NXBasics::CPalette*>(element, NC2Layout::MenuWorldElementOffset::DarkenPalette, nullptr);
	}

	// Original function: FUN_004dac4e; remap the full destination, respecting its stride.
	void WorldDisplayElement_l_Tool_DarkenBitmap_Do(const void* element, NXBasics::CBitmap* bitmap)
	{
		const std::int32_t depth = Read<std::int32_t>(bitmap, NC2Layout::BitmapStorageOffset::Depth);
		const std::uint32_t width = Read<std::uint32_t>(bitmap, NC2Layout::BitmapStorageOffset::Width);
		const std::uint32_t height = Read<std::uint32_t>(bitmap, NC2Layout::BitmapStorageOffset::Height);
		const std::uint32_t stride = Read<std::uint32_t>(bitmap, NC2Layout::BitmapStorageOffset::RowPixelCount);

		if (depth == 2)
		{
			auto* pixels = Read<std::uint32_t*>(bitmap, NC2Layout::BitmapStorageOffset::TrueColorMemory);
			const auto* table = NXBasics::Palette_GetTrueColorTablePtr(Read<void*>(element, NC2Layout::MenuWorldElementOffset::DarkenPalette));

			for (std::uint32_t row = 0; row < height; ++row)
			{
				for (std::uint32_t column = 0; column < width; ++column)
				{
					const std::uint32_t pixel = pixels[column];
					const std::uint32_t blue = pixel & NXBasics::IntegerRange::UInt8Maximum, green = (pixel >> 8) & NXBasics::IntegerRange::UInt8Maximum, red = (pixel >> 16) & NXBasics::IntegerRange::UInt8Maximum;
					const std::uint32_t maximum = red > green ? red : green;
					pixels[column] = table[maximum > blue ? maximum : blue];
				}

				pixels += stride;
			}
		}
	}

	namespace
	{
		std::int32_t WrappedAdd(std::int32_t value, std::int32_t delta)
		{

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) + static_cast<std::uint32_t>(delta));
		}

		std::int32_t WrappedSubtract(std::int32_t value, std::int32_t subtrahend)
		{

			return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) - static_cast<std::uint32_t>(subtrahend));
		}

		bool QueryMapUnderMouse(std::uint32_t& type, std::uintptr_t& mapGuid, std::uintptr_t& extra)
		{
			void* hovered = NXBaseGui::CDesktop::sTheObjectPtr->Element_GetUnderMousePtr();

			if (hovered == nullptr)
				return false;

			type = 0;
			mapGuid = 0;
			extra = 0;
			using Handler = NXBaseGui::TGuiMessage;
			const auto handler = Read<Handler>(Read<void*>(hovered, NC2Layout::GuiBaseElementOffset::VirtualTable), NC2Layout::GuiVirtualTableOffset::MessageHandle);

			return handler(hovered, 0x800, 0x7F0, reinterpret_cast<std::uintptr_t>(&type), reinterpret_cast<std::uintptr_t>(&mapGuid), reinterpret_cast<std::uintptr_t>(&extra));
		}

		void PrintDescription(NXBasics::CBitmap* bitmap, const NC2Logic::SCleanMapInfoData* map)
		{
			if (map->Tool_GetMapDescriptionPtr() != nullptr)
				NC2GuiToolsBase::DrawTool_PrintStringWrapping(bitmap, 4, 0, WrappedAdd(bitmap->GetRectangle().width, -4), NC2GuiToolsBase::DrawTool_GetFont10Ptr(), NC2GuiToolsBase::DrawTool_GetFontColorRef(), map->Tool_GetMapDescriptionPtr());
		}

		void DrawMinimap(NXBasics::CBitmap* bitmap, const NC2Logic::SCleanMapInfoData* map, std::int32_t y)
		{
			// Windows uses an unchecked formatter into this 200-byte stack region.
			char filename[200];
			NXBasics::Runtime_PrintString(filename, "%sminimap\\minimap.pcx", map->mapRoot);

			if (NXBasics::CFile::FileSystem_Tool_FileExists(filename, false))
			{
				NXBasics::CPicture picture(filename);
				picture.GetBitmap()->CopyIntoBitmap_ColorKeyed(bitmap, 0x1B8, y, 0);
			}
		}
	}
}

// Original function: FUN_004da5bf; scalar deleting destructor.
void* __fastcall MainMenuWorldDisplayElement_Delete(void* element, void* unusedEdx, std::uint32_t flags)
{
	(void)unusedEdx;
	NC2MainMenuGuiWorldElement::WorldDisplayElement_Destruct(element);

	if ((flags & 1u) != 0)
		NXBasics::Runtime_Free(element);

	return element;
}

// Original function: FUN_004da615.
void __fastcall MainMenuWorldDisplayElement_XGui_BE_Element_Draw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
{
	(void)unusedEdx;
	using namespace NC2Lifetime;
	using namespace NC2MainMenuGuiWorldElement;
	using NC2MainMenuGuiManager::CGuiManager;
	void* display = Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display);
	NC2E2::Display_UpdateDisplay(display);
	auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
	const bool darken = Read<std::uint8_t>(CGuiManager::sTheObjectPtr, NC2Layout::MainMenuManagerOffset::SuppressPreviewDarkening) == 0;

	if (presentation->IsMainWorld(element))
	{
		// Render the animated scenery at native resolution; logos, campaign maps,
		// descriptions and controls below remain on the reference-size GUI plane.
		auto* background = presentation->GetWorldComposition();
		Read<NXBasics::CBitmap*>(element, NC2Layout::MenuWorldElementOffset::Bitmap)->CopyIntoBitmap(background, 0, 0);

		if (darken)
			WorldDisplayElement_l_Tool_DarkenBitmap_Do(element, background);

		presentation->BitmapToScreen(background, 0, 0);
	}
	else
	{
		NC2InGameGuiBase::BaseBitmapToolElement_Draw(element, bitmap, rectangle);

		if (darken)
			WorldDisplayElement_l_Tool_DarkenBitmap_Do(element, bitmap);
	}

	if (Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::ActiveDisplayFlag) == 0)
	{

		return;
	}

	NC2E2::AmbientSoundManager_UpdateAmbientSounds(NC2E2::gE2AmbientSoundManager, display);
	const auto& resources = CGuiManager::mStaticVars.resources;
	auto* logos = resources.menuLogos;
	const std::int32_t width = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width), height = Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Height);
	const std::int32_t campaignX = WrappedAdd(width, -800) / 2, campaignY = WrappedAdd(height, -600) / 2;
	// Original: the three campaign screens are siblings. 004D9912 tests screen 5 and the
	// following cases test 6 and 7 into the same branch, so all three draw the campaign map
	// and their own unlocked mission markers.
	const bool campaignScreen = NRemaster::IsCampaignMapScreen(CGuiManager::sTheObjectPtr->currentScreen);

	if (campaignScreen)
	{
		NXBasics::BobManager_PrintBobUsingTransparency(logos, 2, bitmap, campaignX, campaignY, resources.campaignMapPalette);

		struct SCampaignOverlayInfo
		{
			std::uint32_t subMissionId, bobId;
		};

		// Complete C2 table 004FCB48..004FCB97 and EightWonders table 004F7E88..004F7EC7.
		static const SCampaignOverlayInfo ls_Campaign01Info_Array[] = { { 20, 3 }, { 30, 4 }, { 40, 5 }, { 50, 6 }, { 60, 7 }, { 70, 8 }, { 80, 9 }, { 100, 10 }, { 110, 11 }, { 0, 0 } };
		static const SCampaignOverlayInfo ls_Campaign03Info_Array[] = { { 20, 13 }, { 30, 14 }, { 40, 15 }, { 50, 16 }, { 60, 17 }, { 70, 18 }, { 80, 19 }, { 0, 0 } };
		const std::uint32_t campaignId = NRemaster::GetCampaignIdForCampaignScreen(NRemaster::GetCampaignScreenIndex(CGuiManager::sTheObjectPtr->currentScreen));
		const auto* overlays = NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2 ? ls_Campaign01Info_Array : ls_Campaign03Info_Array;

		if (campaignId != 0)
			for (const auto* info = overlays; info->subMissionId != 0; ++info)
			{
				if (NC2Logic::gCampaignStaticDataManager->CampaignSubMission_IsAllowed(campaignId, info->subMissionId))
					NXBasics::BobManager_PrintBobUsingTransparency(logos, info->bobId, bitmap, campaignX, campaignY, resources.campaignButtonsPalette);
			}
	}

	if (CGuiManager::sTheObjectPtr->currentScreen == 4 || campaignScreen)
	{
		const bool campaign = campaignScreen;
		const NXBasics::SRectangle area(campaign ? WrappedAdd(width / 2, -240) : 0x1B8, WrappedAdd(height / 2, campaign ? 220 : 100), campaign ? 0x262 : 0x15E, campaign ? 0x40 : 0x6E);
		NXBasics::CBitmap panel(bitmap, area);
		panel.Tool_Darken();
		NC2GuiToolsBase::DrawTool_DrawFrame(nullptr, &panel, &panel.GetRectangle(), 2);
		std::uint32_t type;
		std::uintptr_t mapGuid, extra;

		if (QueryMapUnderMouse(type, mapGuid, extra) && (type == 0x1396 || type == 0x1397))
		{
			const auto* map = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(*reinterpret_cast<const NC2Logic::SCleanMapGuid*>(static_cast<std::uintptr_t>(mapGuid)));

			if (map != nullptr)
				PrintDescription(&panel, map);

			if (CGuiManager::sTheObjectPtr->currentScreen != 5 && map != nullptr)
				DrawMinimap(bitmap, map, WrappedAdd(height / 2, -70));
		}
	}

	if (CGuiManager::sTheObjectPtr->currentScreen == 0x0E && CGuiManager::sTheObjectPtr->lobbyValue == 1)
	{
		const NXBasics::SRectangle area(0x1B8, WrappedAdd(height / 2, -40), 0x15E, 0x6E);
		NXBasics::CBitmap panel(bitmap, area);
		panel.Tool_Darken();
		NC2GuiToolsBase::DrawTool_DrawFrame(nullptr, &panel, &panel.GetRectangle(), 2);
		std::uint32_t type;
		std::uintptr_t mapGuid, extra;

		if (QueryMapUnderMouse(type, mapGuid, extra) && type == 0x13AA)
		{
			const auto* map = NC2Logic::gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(*reinterpret_cast<const NC2Logic::SCleanMapGuid*>(static_cast<std::uintptr_t>(mapGuid)));

			if (map != nullptr)
			{
				DrawMinimap(bitmap, map, WrappedAdd(height / 2, -210));
				PrintDescription(&panel, map);
			}
		}
	}

	const std::int32_t screen = CGuiManager::sTheObjectPtr->currentScreen;
	// Original 004D9DF1..004D9E08 suppresses the wide Funatics logo on screens 5, 6, 7,
	// 0x1B and 0x13. The campaign and user campaign screens carry the artwork instead,
	// so the logo there would overlap it.

	if (!NRemaster::IsCampaignMapScreen(screen) && screen != 0x16 && screen != 0x0E && screen != 0x1B && screen != 0x13)
	{
		const auto* bob = static_cast<const NXBasics::SRectangle*>(NXBasics::BobManager_GetBobAreaRectanglePtr(logos, 0));
		NXBasics::BobManager_PrintBobUsingTransparency(logos, 0, bitmap, WrappedSubtract(width, bob->width) / 2, 10, resources.funaticsPalette);
	}

	if (CGuiManager::sTheObjectPtr->currentScreen == 1)
	{
		const auto* bob = static_cast<const NXBasics::SRectangle*>(NXBasics::BobManager_GetBobAreaRectanglePtr(logos, 1));
		const std::int32_t y = WrappedAdd(WrappedSubtract(height, bob->height), -12);
		bob = static_cast<const NXBasics::SRectangle*>(NXBasics::BobManager_GetBobAreaRectanglePtr(logos, 1));
		NXBasics::BobManager_PrintBobUsingTransparency(logos, 1, bitmap, WrappedSubtract(width, bob->width), y, resources.funaticsPalette);
	}

	if (CGuiManager::sTheObjectPtr->currentScreen == 0x16)
		NXBasics::BobManager_PrintBobUsingTransparency(logos, 12, bitmap, campaignX, campaignY, resources.campaignMapPalette);
}

// Original function: FUN_004daac1; mutating commands return false, frame calls return true.
bool __fastcall MainMenuWorldDisplayElement_XGui_BE_Message_Handle(void* element, void* unusedEdx, std::uint32_t message, std::uint32_t command, std::uintptr_t param1Payload, std::uintptr_t param2Payload, std::uintptr_t param3Payload)
{
	const auto param1 = static_cast<std::uint32_t>(param1Payload);
	const auto param2 = static_cast<std::uint32_t>(param2Payload);
	const auto param3 = static_cast<std::uint32_t>(param3Payload);
	(void)unusedEdx;
	using namespace NC2Lifetime;

	if (message == 0x1000)
	{
		switch (command)
		{
			case SwitchIds::WorldDisplayCommand::AssignDisplayDefinition:
				NC2E2::Display_AssignNewDisplayDefinitionValues(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), Read<float>(&param1, 0), Read<float>(&param2, 0), Read<float>(&param3, 0), true);
				break;
			case SwitchIds::WorldDisplayCommand::ToggleDrawFlags:
				NC2E2::DE_ToDraw_ToggleFlagset(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), param1);
				break;
			case SwitchIds::WorldDisplayCommand::ScrollByPixelDelta:
				if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit9Unsigned) == 0)
					NC2E2::DE_SetWantedPositionAddDeltaPixelPoint(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), NXBasics::Runtime_TimeSigned32(param1), NXBasics::Runtime_TimeSigned32(param2));

				break;
			case SwitchIds::WorldDisplayCommand::ToggleZoom:
				if (Read<std::uint8_t>(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), NC2Layout::EngineDisplayOffset::FontSigned) != 0)
					NC2E2::DE_Zoom_ToggleLevel(Read<void*>(element, NC2Layout::MenuWorldElementOffset::Display), false);

				break;
		}
	}
	else if (message == 0x800 && command == 0x7EE)
	{
		NC2MainMenuGuiWorldElement::WorldDisplayElement_l_FollowMode_Update(element);
		return true;
	}

	return false;
}

// Original function: FUN_004dab77; remaster retains only the 32-bit palette remap.
void __fastcall MainMenuWorldDisplayElement_l_Tool_DarkenBitmap_Init(void* element)
{
	using namespace NC2Lifetime;
	auto* palette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\menu_remap.pcx");
	Write(element, NC2Layout::MenuWorldElementOffset::DarkenPalette, palette);
}