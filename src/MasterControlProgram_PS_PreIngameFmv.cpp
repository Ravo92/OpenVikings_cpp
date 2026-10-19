#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "GameProfile.h"
#include "MasterControlProgram.h"
#include "MasterPropertyManager.h"
#include "WindowsVideoPlayer.h"
#include "X2DApplication.h"
#include "X2DWindow.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XMKeyManager.h"
#include "XMMouseManager.h"
#include <cstring>
#include <new>

namespace
{
	using namespace NC2Lifetime;
	// Complete 0050F638..0050F63F region: cleared byte, padding, manager pointer.
	struct SPreIngameFmvStaticVars
	{
		std::uint8_t state; std::byte padding[3]; void* manager;
	};
	static SPreIngameFmvStaticVars gPreIngameFmv{ };
	// Original function: FUN_00403406.
	static void PreIngameFmv_GfxData_Load(void* manager)
	{
		auto* memory = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		NXBasics::CBitmap* bitmap = nullptr;
		if (memory != nullptr)
		{
			auto* heightApplication = NXSys2DApp::CApplication::sTheObjectPtr;
			auto* widthApplication = heightApplication;
			const auto depth = NXBasics::ConvertBitsToTDepth(heightApplication->GetDisplayDepth());
			const auto height = heightApplication->GetDisplayHeight();
			const auto width = widthApplication->GetDisplayWidth();
			bitmap = ::new (memory) NXBasics::CBitmap(width, height, depth);
		}
		Write(manager, NC2Layout::PreIngameFmvOffset::ScreenBitmap, bitmap);
		bitmap->Fill(NXBasics::SColorRGB(0, 0, 0));
	}

	// Original function: FUN_00403475; the stored pointer is not cleared.
	static void PreIngameFmv_GfxData_Free(void* manager)
	{
		if (auto* bitmap = Read<NXBasics::CBitmap*>(manager, NC2Layout::PreIngameFmvOffset::ScreenBitmap)) delete bitmap;
	}

	// Original function: FUN_004031f7.
	static bool PreIngameFmv_StartVideo(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested) == 0) return false;
		char pattern[260], fileName[260];
		// Requested installation layout: loose films are stored in DataX\FMV.
		NXBasics::Runtime_PrintString(pattern, "datax\\FMV\\%%s\\%s.%s", static_cast<const char*>(manager) + 1, NRemaster::GetFmvFileExtension());
		if (!NC2Logic::LanguageTool_GetFilename(pattern, fileName, false)) return false;
		Read<NXBasics::CBitmap*>(manager, NC2Layout::PreIngameFmvOffset::ScreenBitmap)->Fill(NXBasics::SColorRGB(0, 0, 0));
		if (NWindowsVideo::CVideoPlayer::sTheObjectPtr == nullptr)
		{
			if (auto* memory = NXBasics::Runtime_AllocateWithNewHandler(44, 1)) new (memory) NWindowsVideo::CVideoPlayer();
		}
		auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr;
		const bool enableAudio = !NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("fx_off");
		if (video != nullptr && video->Open(fileName, enableAudio))
		{
			video->Start();
			Write<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive, video->IsPlaying() ? 1 : 0);
		}
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive) == 0) return false;
		NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(true);
		Write<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::CursorHideRequested, 1);
		return true;
	}

	// Original function: FUN_0040330d; the force argument is unused by Windows.
	static void PreIngameFmv_UpdateDisplay(void* manager, bool /*force*/)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive) != 0)
		{
			auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr;
			if (video != nullptr && video->IsPlaying())
			{
				video->Update(NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay());
				return;
			}
		}
		else Read<NXBasics::CBitmap*>(manager, NC2Layout::PreIngameFmvOffset::ScreenBitmap)->Fill(NXBasics::SColorRGB(0, 0, 0));
		NXSys2DApp::CApplication::sTheObjectPtr->BitmapToScreen(Read<NXBasics::CBitmap*>(manager, NC2Layout::PreIngameFmvOffset::ScreenBitmap), 0, 0);
	}

	// Original function: FUN_0040339b.
	static void PreIngameFmv_ToNextMode(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive) != 0)
		{
			NWindowsVideo::CVideoPlayer::sTheObjectPtr->Stop();
			Write<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive, 0);
		}
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested) != 0)
		{
			NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->DisplayResolution_ResetToUserResolution();
			PreIngameFmv_GfxData_Free(manager);
			PreIngameFmv_GfxData_Load(manager);
		}
		for (std::uint32_t frame = 0; frame < 5; ++frame)
		{
			PreIngameFmv_UpdateDisplay(manager, true);
			NXSys2DApp::CApplication::sTheObjectPtr->UpdateDisplay();
		}
		NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->System_ProgramState_ChangeNextTimePossible(NMasterControlProgram::TProgramState::Game, false);
	}

	// Original function: FUN_00403029; filename copying is intentionally unbounded.
	static void* PreIngameFmv_Construct(void* manager)
	{
		gPreIngameFmv.manager = manager;
		std::memset(manager, 0, 208);
		gPreIngameFmv.state = 0;
		Write<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested, 0);
		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("start_pre_ingame_fmv"))
		{
			const char* caption = NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_GetStringValuePtr("start_pre_ingame_fmv");
			Write<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested, 1);
			std::strcpy(static_cast<char*>(manager) + 1, caption);
			NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_Remove("start_pre_ingame_fmv");
		}
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested) != 0)
		{
			NMasterControlProgram::CMasterControlProgram::sTheObjectPtr->DisplayResolution_SetDirectly(640, 480, 32, false, false, true);
		}
		PreIngameFmv_GfxData_Load(manager);
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested) != 0) PreIngameFmv_StartVideo(manager);
		return manager;
	}

	// Original function: FUN_0040313f.
	static void PreIngameFmv_Destruct(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::CursorHideRequested) != 0) NXSysMouseManager::CMouseManager::sTheObjectPtr->SetHideSystemMouseCursorWhenFullscreenState(false);
		if (auto* video = NWindowsVideo::CVideoPlayer::sTheObjectPtr)
		{
			video->~CVideoPlayer();
			NXBasics::Runtime_Free(video);
		}
		PreIngameFmv_GfxData_Free(manager);
		gPreIngameFmv.manager = nullptr;
	}

	// Original function: FUN_00403182.
	static void PreIngameFmv_System_Update(void* manager)
	{
		if (Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvRequested) == 0 || Read<std::uint8_t>(manager, NC2Layout::PreIngameFmvOffset::FmvActive) == 0 || !NWindowsVideo::CVideoPlayer::sTheObjectPtr->IsPlaying())
		{
			PreIngameFmv_ToNextMode(manager);
			return;
		}
		NXSysKeyManager::SKeyMessage message;
		if ((NXSysKeyManager::CKeyManager::sTheObjectPtr->GetKey(message) && message.state != 0 && (message.TextCharacter() == 0x20 || message.keyCode == 0x1B)) || NXSysMouseManager::CMouseManager::sTheObjectPtr->IsAnyButtonDown())
		{
			NXSysKeyManager::CKeyManager::sTheObjectPtr->Init();
			PreIngameFmv_ToNextMode(manager);
		}
		else PreIngameFmv_UpdateDisplay(manager, false);
	}
}

namespace NMasterControlProgram
{
	// Original function: FUN_004022f0.
	bool CMasterControlProgram::System_ProgramState_PreIngameFmv_StartUp()
	{
		mProgramState = 14;
		if (auto* memory = NXBasics::Runtime_AllocateWithNewHandler(208, 1)) PreIngameFmv_Construct(memory);
		mProgramState = static_cast<std::int32_t>(TProgramState::PreIngameFmv);
		return true;
	}

	// Original function: FUN_00402319.
	void CMasterControlProgram::System_ProgramState_PreIngameFmv_ShutDown()
	{
		mProgramState = 16;
		if (void* manager = gPreIngameFmv.manager)
		{
			PreIngameFmv_Destruct(manager);
			NXBasics::Runtime_Free(manager);
		}
		mProgramState = static_cast<std::int32_t>(TProgramState::None);
	}

	// Original function: FUN_00402344.
	void CMasterControlProgram::System_ProgramState_PreIngameFmv_Update()
	{
		PreIngameFmv_System_Update(gPreIngameFmv.manager);
	}
}
