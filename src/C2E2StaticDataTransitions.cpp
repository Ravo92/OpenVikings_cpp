#include "C2E2StaticDataGroups.h"
#include "C2E2StaticDataTerrain.h"
#include "C2LifetimeGlobals.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XGuiElementAccess.h"
#include "XIniFileReader.h"

#include <cstring>

namespace NC2E2
{
	namespace
	{
		// Complete token tables, 004F49D0..004F4A1F, and lazy state 00568A10..00568A1B.
		const NXBasics::SToken gPointTypeTokens[] = { { "name", SwitchIds::TransitionPointToken::Name }, { "patterngroup", SwitchIds::TransitionPointToken::Patterngroup }, { nullptr, 0 } };
		const NXBasics::SToken gTransitionTokens[] = { { "name", SwitchIds::TransitionPointToken::Name }, { "pointtype", SwitchIds::TransitionToken::PointType }, { "gfxtexture", SwitchIds::TransitionToken::GfxTexture }, { "gfxtexturealpha", SwitchIds::TransitionToken::Gfxtexturealpha }, { "gfxcoordsa", SwitchIds::TransitionToken::GfxCoordsA }, { "gfxcoordsb", SwitchIds::TransitionToken::GfxCoordsB }, { nullptr, 0 } };
		const NXBasics::SToken* gTransitionTokenTable = nullptr;
		std::uint32_t gTransitionTokenInitFlags = 0;
		const NXBasics::SToken* gPointTypeTokenTable = nullptr;

		// Original function: FUN_0048db87.
		int __cdecl DestroyTransitionTokenList()
		{

			return 0;
		}

		// Original function: FUN_0048db88.
		int __cdecl DestroyPointTypeTokenList()
		{

			return 0;
		}
	}

	// Original function: FUN_0048db89. Windows constructs both arrays of six three-point triangles.
	static void* StaticTransition_Construct(void* transition)
	{
		// ICF entry 0048E569 is also the identity constructor for each eight-byte point.

		for (std::uint32_t triangle = 0; triangle < 6; ++triangle)
		{
			for (std::uint32_t point = 0; point < 3; ++point)
			{
				NXBaseGui::BaseElement_GetRealActiveElementPtr(static_cast<std::byte*>(transition) + 8u + triangle * 0x18u + point * 8u);
			}
		}

		for (std::uint32_t triangle = 0; triangle < 6; ++triangle)
		{
			for (std::uint32_t point = 0; point < 3; ++point)
			{
				NXBaseGui::BaseElement_GetRealActiveElementPtr(static_cast<std::byte*>(transition) + 0x98u + triangle * 0x18u + point * 8u);
			}
		}

		return transition;
	}

	// Original function: FUN_0048d714.
	void* __fastcall E2StaticDataTransitionsManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataTransitionsManager = manager;
		std::memset(manager, 0, 1);
		std::memset(&gStaticTransitionStorage, 0, sizeof(gStaticTransitionStorage));
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\patterntransitions\\transitions.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		auto& storage = gStaticTransitionStorage;
		storage.pointTypeCount = reader.GetNumberOfGroups("pointtype");

		if (storage.pointTypeCount == 0)
			return manager;

		const std::size_t pointBytes = static_cast<std::size_t>(static_cast<std::uint32_t>(storage.pointTypeCount)) * sizeof(STransitionPointType);
		storage.pointTypes = static_cast<STransitionPointType*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(storage.pointTypeCount), sizeof(STransitionPointType)));
		std::memset(storage.pointTypes, 0, pointBytes);
		auto* point = storage.pointTypes;

		for (char* group = reader.GetFirstGroup("pointtype"); group != nullptr; group = reader.GetNextGroup("pointtype"), ++point)
		{
			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gTransitionTokenInitFlags & 1u) == 0)
				{
					gTransitionTokenInitFlags |= 1u;
					gPointTypeTokenTable = gPointTypeTokens;
					NXBasics::Runtime_AtExit(DestroyPointTypeTokenList);
				}

				NXBasics::CTokenList tokens(gPointTypeTokenTable);

				switch (tokens.FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::TransitionPointToken::Name:
						std::strncpy(point->name, reader.GetString(), 39);
						break;
					case SwitchIds::TransitionPointToken::Patterngroup:
						if (point->groupCount != 5)
							point->groupIds[point->groupCount++] = StaticDataGroups_FindIndex(gE2StaticDataPatternManager, reader.GetString());

						break;
				}
			}
		}

		storage.count = reader.GetNumberOfGroups("transition");

		if (storage.count == 0 || static_cast<std::uint32_t>(storage.count) > 42u)
			return manager;

		const std::size_t bytes = static_cast<std::size_t>(static_cast<std::uint32_t>(storage.count)) * sizeof(SE2StaticDataTransition);
		storage.entries = static_cast<SE2StaticDataTransition*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(storage.count), sizeof(SE2StaticDataTransition)));

		for (std::int32_t index = 0; index < storage.count; ++index)
		{
			StaticTransition_Construct(storage.entries + index);
		}

		std::memset(storage.entries, 0, bytes);
		auto* entry = storage.entries;

		for (char* group = reader.GetFirstGroup("transition"); group != nullptr; group = reader.GetNextGroup("transition"), ++entry)
		{
			entry->alphaTextureId = -1;
			unsigned countA = 0, countB = 0;

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gTransitionTokenInitFlags & 2u) == 0)
				{
					gTransitionTokenInitFlags |= 2u;
					gTransitionTokenTable = gTransitionTokens;
					NXBasics::Runtime_AtExit(DestroyTransitionTokenList);
				}

				NXBasics::CTokenList tokens(gTransitionTokenTable);

				switch (tokens.FindToken(reader.GetCurrentName()))
				{
					case SwitchIds::TransitionToken::Name:
						std::strncpy(entry->name, reader.GetString(), 39);
						break;
					case SwitchIds::TransitionToken::PointType:
						entry->pointTypeId = E2StaticDataTransitionsManager_l_GetPointTypeId(reader.GetString());
						break;
					case SwitchIds::TransitionToken::GfxTexture:
						entry->textureId = E2StaticDataTexturesManager_GetTextureId(gE2StaticDataTexturesManager, nullptr, reader.GetString());
						break;
					case SwitchIds::TransitionToken::Gfxtexturealpha:
						entry->alphaTextureId = E2StaticDataTexturesManager_GetTextureId(gE2StaticDataTexturesManager, nullptr, reader.GetString());
						break;
					case SwitchIds::TransitionToken::GfxCoordsA:
						if (countA != 6)
						{
							for (auto& coordinate : entry->coordinatesA[countA])
							{
								coordinate = reader.GetInt();
							}

							++countA;
						}

						break;
					case SwitchIds::TransitionToken::GfxCoordsB:
						if (countB != 6)
						{
							for (auto& coordinate : entry->coordinatesB[countB])
							{
								coordinate = reader.GetInt();
							}

							++countB;
						}

						break;
				}
			}
		}

		storage.roadOverlayIds[0] = E2StaticDataTransitionsManager_GetTransitionId("overlay road 1");
		storage.roadOverlayIds[1] = E2StaticDataTransitionsManager_GetTransitionId("overlay road 2");
		storage.houseOverlayIds[0] = E2StaticDataTransitionsManager_GetTransitionId("overlay house 1");
		storage.houseOverlayIds[1] = E2StaticDataTransitionsManager_GetTransitionId("overlay house 2");

		return manager;
	}

	// Original function: FUN_0048dbfb.
	std::int32_t E2StaticDataTransitionsManager_GetTransitionId(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticTransitionStorage.count; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(gStaticTransitionStorage.entries[index].name, name) == 0)
				return index;
		}

		return -1;
	}

	// Original function: FUN_0048dc39.
	std::int32_t E2StaticDataTransitionsManager_l_GetPointTypeId(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticTransitionStorage.pointTypeCount; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(gStaticTransitionStorage.pointTypes[index].name, name) == 0)
				return index;
		}

		return -1;
	}
}