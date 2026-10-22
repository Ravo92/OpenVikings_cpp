#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "GameProfile.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "SwitchIdentifiers.h"
#include "WindowsVideoPlayer.h"
#include "X2DApplication.h"
#include "X2DWindow.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XFont.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPicture.h"
#include "XStorableIO.h"

#include <cstring>
#include <new>

namespace
{
	using namespace NC2Lifetime;
	// Complete 0050F630..0050F637 region: unclassified cleared byte, padding, manager pointer.
	// Windows has no read xref for the cleared byte; its legacy field name is retained.
	struct STitleScreenStaticVars
	{
		std::uint8_t state;
		std::byte padding[3];
		void* manager;
	};
	static STitleScreenStaticVars gTitleScreen{ };
	// Repeated original bitmap allocation sequence; callers retain their lifetimes.
	static NXBasics::CBitmap* TitleScreen_CreateBitmap(std::int32_t width, std::int32_t height, std::int32_t depth, void* memory)
	{
		return memory != nullptr ? ::new (memory) NXBasics::CBitmap(width, height, depth) : nullptr;
	}

	// Original function: FUN_00402e88.
	static void TitleScreen_GfxData_Load(void* manager)
	{
		char fileName[260];
		NC2Logic::LanguageTool_GetFilename("data\\gui\\lang\\%s\\pictures\\titlescreen.bmp", fileName, false);
		{
			NXBasics::CPicture picture(fileName);
			auto* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::Bitmap, 1);
			NXBasics::CBitmap* bitmap = nullptr;
			if (memory != nullptr)
			{
				auto* widthBitmap = picture.GetBitmap();
				auto* heightBitmap = widthBitmap;
				const auto depth = NXBasics::ConvertBitsToTDepth(NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth());
				bitmap = TitleScreen_CreateBitmap(widthBitmap->GetRectangle().width, heightBitmap->GetRectangle().height, depth, memory);
			}
			Write(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap, bitmap);
			picture.GetBitmap()->CopyIntoBitmap(bitmap, 0, 0);
		}
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::Bitmap, 1);
		NXBasics::CBitmap* bitmap = nullptr;
		if (memory != nullptr)
		{
			auto* heightApplication = NXSys2DApp::CApplication::sTheObjectPtr;
			auto* widthApplication = heightApplication;
			const auto depth = NXBasics::ConvertBitsToTDepth(heightApplication->GetDisplayDepth());
			const auto height = heightApplication->GetDisplayHeight();
			const auto width = widthApplication->GetDisplayWidth();
			bitmap = TitleScreen_CreateBitmap(width, height, depth, memory);
		}
		Write(manager, NC2Layout::TitleScreenManagerOffset::ScreenBitmap, bitmap);
		bitmap->Fill(NXBasics::SColorRGB(0, 0, 0));
		auto* font = static_cast<NXBasics::CFont*>(NXBasics::XB_Storable_LoadObject("data\\gui\\fonts\\font10.fnt"));
		Write(manager, NC2Layout::TitleScreenManagerOffset::Font, font);
		Write<std::uint32_t>(font, NC2Layout::FontOffset::CharacterSpacing, NXBasics::IntegerRange::UInt32Maximum);
		memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::Palette, 1);
		auto* palette = memory != nullptr ? ::new (memory) NXBasics::CPalette() : nullptr;
		Write(manager, NC2Layout::TitleScreenManagerOffset::GrayscalePalette, palette);
		for (std::uint32_t index = 0; index < NC2Runtime::TitleScreenLimits::PaletteColors; ++index) reinterpret_cast<NXBasics::SColorRGB*>(reinterpret_cast<std::byte*>(palette) + NC2Layout::PaletteOffset::Colors + index * NC2Layout::RecordSize::PaletteColor)->SetColors(static_cast<std::uint8_t>(index), static_cast<std::uint8_t>(index), static_cast<std::uint8_t>(index));
	}

	// Original function: FUN_00402fcb; resource fields remain unchanged.
	static void TitleScreen_GfxData_Free(void* manager)
	{
		if (auto* palette = Read<NXBasics::CPalette*>(manager, NC2Layout::TitleScreenManagerOffset::GrayscalePalette)) delete palette;
		if (auto* font = Read<NXBasics::CFont*>(manager, NC2Layout::TitleScreenManagerOffset::Font)) delete font;
		if (auto* bitmap = Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::ScreenBitmap)) delete bitmap;
		if (auto* bitmap = Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap)) delete bitmap;
	}

	// Original function: FUN_00402af2.
	static bool TitleScreen_FMV_Start(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) == 0) return false;
		char pattern[260], fileName[260];
		// Requested installation layout: loose films are stored in DataX\FMV.
		NXBasics::Runtime_PrintString(pattern, "datax\\FMV\\%%s\\%s.%s", static_cast<const char*>(manager) + NC2Layout::TitleScreenManagerOffset::FmvStem, NRemaster::GetFmvFileExtension());
		if (!NC2Logic::LanguageTool_GetFilename(pattern, fileName, false)) return false;
		Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::ScreenBitmap)->Fill(NXBasics::SColorRGB(0, 0, 0));
		if (NWindowsVideo::CVideoPlayer::sTheObjectPtr == nullptr)
		{
			if (auto* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NWindowsVideo::CVideoPlayer), 1)) new (memory) NWindowsVideo::CVideoPlayer();
		}
		auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr;
		const bool enableAudio = !NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("fx_off");
		if (video != nullptr && video->Open(fileName, enableAudio))
		{
			video->Start();
			Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive, video->IsPlaying() ? 1 : 0);
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive) == 0) return false;
		NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(true);
		Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::CursorHideRequested, 1);
		return true;
	}

	// Original function: FUN_00402c08; the force argument is unused by Windows.
	static void TitleScreen_UpdateDisplay(void* manager, bool /*force*/)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive) != 0)
		{
			auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr;
			if (video != nullptr && video->IsPlaying())
			{
				video->Update(NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay());
				return;
			}
		}
		else Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap)->CopyIntoBitmap_FitIn(Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::ScreenBitmap));
		NXSys2DApp::CApplication::sTheObjectPtr->BitmapToScreen(Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::ScreenBitmap), 0, 0);
	}

	// Original function: FUN_00402c90.
	static void TitleScreen_ToNextMode(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive) != 0)
		{
			NWindowsVideo::CVideoPlayer::sTheObjectPtr->Stop();
			Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive, 0);
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::NextIntroPending) != 0)
		{
			Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::NextIntroPending, 0);
			std::strcpy(static_cast<char*>(manager) + NC2Layout::TitleScreenManagerOffset::FmvStem, "intro");
			if (TitleScreen_FMV_Start(manager)) return;
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) != 0)
		{
			NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->DisplayResolution_ResetToUserResolution();
			TitleScreen_GfxData_Free(manager);
			TitleScreen_GfxData_Load(manager);
		}
		bool forceChange = false;
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence) != 0 || Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PostPictureSequence) != 0)
		{
			const auto index = Read<std::uint32_t>(manager, NC2Layout::TitleScreenManagerOffset::PictureIndex) + 1u;
			Write(manager, NC2Layout::TitleScreenManagerOffset::PictureIndex, index);
			const char* prefix = Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence) != 0 ? "pre" : "post";
			char fileName[200];
			NXBasics::Runtime_PrintString(fileName, "datax\\pictures\\%s%2.2d.bmp", prefix, NXBasics::Runtime_TimeSigned32(index));
			if (NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true))
			{
				if (auto* bitmap = Read<NXBasics::CBitmap*>(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap)) delete bitmap;
				Write<void*>(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap, nullptr);
				NXBasics::CPicture picture(fileName);
				auto* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::Bitmap, 1);
				NXBasics::CBitmap* bitmap = nullptr;
				if (memory != nullptr)
				{
					auto* heightBitmap = picture.GetBitmap();
					auto* widthBitmap = heightBitmap;
					const auto depth = NXBasics::ConvertBitsToTDepth(NXSys2DApp::CApplication::sTheObjectPtr->GetDisplayDepth());
					bitmap = TitleScreen_CreateBitmap(widthBitmap->GetRectangle().width, heightBitmap->GetRectangle().height, depth, memory);
				}
				Write(manager, NC2Layout::TitleScreenManagerOffset::PictureBitmap, bitmap);
				picture.GetBitmap()->CopyIntoBitmap(bitmap, 0, 0);
				Write<std::uint32_t>(manager, NC2Layout::TitleScreenManagerOffset::PictureStartedAtMilliseconds, timeGetTime());
				return;
			}
			if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("game_start_pre_intro"))
			{
				NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("game_start_pre_intro");
				NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_ProgramState_ChangeNextTimePossible(NMasterControlProgram::TProgramState::TitleScreen, true);
				return;
			}
			if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PostPictureSequence) != 0)
			{
				Write<std::uint8_t>(NMasterControlProgram::CMasterControlProgram::sTheObjectPtr, NC2Layout::MasterControlOffset::ExitRequested, 1);
				return;
			}
			forceChange = true;
		}
		for (std::uint32_t frame = 0; frame < NC2Runtime::TitleScreenLimits::TransitionDisplayUpdates; ++frame)
		{
			TitleScreen_UpdateDisplay(manager, forceChange);
			NXSys2DApp::CApplication::sTheObjectPtr->UpdateDisplay();
		}
		const bool startGame = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("start_map") || NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("skip_main_menu");
		NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_ProgramState_ChangeNextTimePossible(startGame ? NMasterControlProgram::TProgramState::Game : NMasterControlProgram::TProgramState::MainMenu, false);
	}

	// Original function: FUN_004027d2.
	static void* TitleScreen_Construct(void* manager)
	{
		gTitleScreen.manager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::TitleScreenManager);
		gTitleScreen.state = 0;
		Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode, 0);
		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("game_start_pre_picture_sequence"))
		{
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("game_start_pre_picture_sequence");
			if (NXBasics::CFile::FileSystem_Tool_FileExists("datax\\pictures\\pre00.bmp", true))
			{
				Write<std::uint32_t>(manager, NC2Layout::TitleScreenManagerOffset::PictureIndex, NXBasics::IntegerRange::UInt32Maximum);
				Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence, 1);
				NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("game_start_pre_intro", false, 0, nullptr);
			}
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence) == 0)
		{
			if (!NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("intro_skip"))
			{
				Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode, 1);
				std::strcpy(static_cast<char*>(manager) + NC2Layout::TitleScreenManagerOffset::FmvStem, "intro_00");
				Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::NextIntroPending, 1);
			}
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Add("intro_skip", false, 0, nullptr);
			if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("start_fmv"))
			{
				const char* caption = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetStringValuePtr("start_fmv");
				Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode, 1);
				std::strcpy(static_cast<char*>(manager) + NC2Layout::TitleScreenManagerOffset::FmvStem, caption);
				NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("start_fmv");
			}
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) == 0 && NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("game_start_post_picture_sequence"))
		{
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("game_start_post_picture_sequence");
			// Requested missing-asset fallback: the native sequence end already
			// requests program exit when the next post picture does not exist.
			Write<std::uint32_t>(manager, NC2Layout::TitleScreenManagerOffset::PictureIndex, NXBasics::IntegerRange::UInt32Maximum);
			Write<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PostPictureSequence, 1);
		}
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) != 0)
		{
			NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->DisplayResolution_SetDirectly(NC2Runtime::TitleScreenGeometry::IntroWidth, NC2Runtime::TitleScreenGeometry::IntroHeight, NC2Runtime::DisplayDepthBits::TrueColor, false, false, true);
		}
		TitleScreen_GfxData_Load(manager);
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) != 0) TitleScreen_FMV_Start(manager);
		else if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence) != 0 || Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PostPictureSequence) != 0 || NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("tincat") || NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("game_pre_stuff_skip")) TitleScreen_ToNextMode(manager);
		return manager;
	}

	// Original function: FUN_00402a15.
	static void TitleScreen_Destruct(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::CursorHideRequested) != 0) NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(false);
		if (auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr)
		{
			video->~CVideoPlayer();
			NXBasics::Runtime_Free(video);
		}
		TitleScreen_GfxData_Free(manager);
		gTitleScreen.manager = nullptr;
	}

	// Original function: FUN_00402a58.
	static void TitleScreen_System_Update(void* manager)
	{
		bool advance = Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvMode) != 0 && (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::FmvActive) == 0 || !NWindowsVideo::CVideoPlayer::sTheObjectPtr->IsPlaying());
		if (!advance && (Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PostPictureSequence) != 0 || Read<std::uint8_t>(manager, NC2Layout::TitleScreenManagerOffset::PrePictureSequence) != 0)) advance = timeGetTime() > Read<std::uint32_t>(manager, NC2Layout::TitleScreenManagerOffset::PictureStartedAtMilliseconds) + NC2Runtime::TitleScreenTiming::PictureDurationMillisecondsUnsigned;
		if (advance)
		{
			TitleScreen_ToNextMode(manager);
			return;
		}
		NXSysKeyManager::SKeyMessage message;
		if ((NXSysKeyManager::CKeyManager::sTheObjectPtr->GetKey(message) && message.state != 0 && (message.TextCharacter() == SwitchIds::TitleScreenSkipKey::Space || message.keyCode == SwitchIds::TitleScreenSkipKey::Escape)) || NXSysMouseManager::CMouseManager::sTheObjectPtr->IsAnyButtonDown())
		{
			NXSysKeyManager::CKeyManager::sTheObjectPtr->Init();
			TitleScreen_ToNextMode(manager);
		}
		else TitleScreen_UpdateDisplay(manager, false);
	}
}

namespace NMasterControlProgram
{
	// Original function: FUN_00402773.
	bool CMasterControlProgram::System_ProgramState_TitleScreen_StartUp()
	{
		mProgramState = SwitchIds::ProgramState::TitleScreenStartUp;
		if (auto* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::TitleScreenManager, 1)) TitleScreen_Construct(memory);
		mProgramState = static_cast<std::int32_t>(TProgramState::TitleScreen);
		return true;
	}

	// Original function: FUN_0040279c.
	void CMasterControlProgram::System_ProgramState_TitleScreen_ShutDown()
	{
		mProgramState = SwitchIds::ProgramState::TitleScreenShutDown;
		if (void* manager = gTitleScreen.manager)
		{
			TitleScreen_Destruct(manager);
			NXBasics::Runtime_Free(manager);
		}
		mProgramState = static_cast<std::int32_t>(TProgramState::None);
	}

	// Original function: FUN_004027c7.
	void CMasterControlProgram::System_ProgramState_TitleScreen_Update()
	{
		TitleScreen_System_Update(gTitleScreen.manager);
	}
}
