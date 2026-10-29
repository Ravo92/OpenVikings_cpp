#include "C2E2LevelOfDetailManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "MasterPropertyManager.h"
#include "XCrtFileAccess.h"

#include <cstdint>
#include <new>

namespace NC2E2
{
	namespace
	{
		// Original data: DAT_004f4748 through DAT_004f4750.
		constexpr std::uint8_t levelOfDetailFlags[9] = {
			1, 0, 1,
			1, 0, 0,
			1, 1, 0
		};
	}

	// Original function: FUN_00489f75.
	CLevelOfDetailManager::CLevelOfDetailManager()
	{
		// Original globals: 005684c8 and 005684c4.
		sTheObjectPtr = this;
		mState = 0;
		sLevelOfDetail = 0;

		auto* properties = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;

		if (properties->Property_DoesExists("engine_lod"))
		{
			sLevelOfDetail = properties->Property_GetIntegerValue("engine_lod");
		}
		else
		{
			sLevelOfDetail = 2;
		}
	}

	// Original function: FUN_00489fd1
	CLevelOfDetailManager::~CLevelOfDetailManager()
	{
		sTheObjectPtr = nullptr;
	}

	// Lifecycle helper for the allocation and construction at 0047de59–0047de66.
	void CLevelOfDetailManager::CreateInstance()
	{
		void* storage = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(sizeof(CLevelOfDetailManager)), 1);

		if (storage == nullptr)
		{

			return;
		}

		::new (storage) CLevelOfDetailManager();
	}

	// Lifecycle helper for the destruction and release at 0047e46b–0047e482.
	void CLevelOfDetailManager::DestroyInstance()
	{
		CLevelOfDetailManager* instance = sTheObjectPtr;

		if (instance == nullptr)
		{

			return;
		}

		instance->~CLevelOfDetailManager();
		NXBasics::Runtime_Free(instance);
	}

	// Original function: FUN_00489fd9.
	void CLevelOfDetailManager::SetLevelOfDetail(std::int32_t levelOfDetail)
	{
		if (levelOfDetail == sLevelOfDetail)
		{

			return;
		}

		C2DEngineDisplayManager* displayManager = C2DEngineDisplayManager::sTheObjectPtr;
		sLevelOfDetail = levelOfDetail;

		displayManager->InformThatStaticMapChanged();
		NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Options_Save_InGame();
	}

	// Original function: FUN_0048a001
	bool CLevelOfDetailManager::GetUseLandscapeAlphaBobsFlag()
	{
		const std::uint32_t offset = static_cast<std::uint32_t>(sLevelOfDetail) * 3u;

		return levelOfDetailFlags[offset] != 0;
	}

	// Original function: FUN_0048a00e
	bool CLevelOfDetailManager::GetUseMatrixFXBobsFlag()
	{
		const std::uint32_t offset = static_cast<std::uint32_t>(sLevelOfDetail) * 3u;

		return levelOfDetailFlags[offset + 1u] != 0;
	}

	// Original function: FUN_0048a01b
	bool CLevelOfDetailManager::GetSkipUnimportantVoidLandscapesFlag()
	{
		const std::uint32_t offset = static_cast<std::uint32_t>(sLevelOfDetail) * 3u;

		return levelOfDetailFlags[offset + 2u] != 0;
	}
}