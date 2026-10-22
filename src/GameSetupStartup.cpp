#include "GameSetupStartup.h"
#include "GameSetupSettings.h"
#include "CivilizationBuildings.h"
#include "C2EngineDisplayAccess.h"
#include "C2E2StaticDataHouses.h"
#include "C2HouseBuildPosition.h"
#include "C2MapPathfinderSearch.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <initializer_list>

namespace NRemaster::GameSetup::Startup
{
	using namespace NC2Lifetime;
	namespace
	{
		SPlayerChoices startupChoices;
		std::array<std::byte, 16> startupMap{};
		bool startupPending = false, loading = false;
	}
	void Reset()
	{
		startupChoices = {}; startupMap.fill(std::byte{}); startupPending = loading = false;
	}
	void Prepare(const void* mapGuid, const SPlayerChoices& choices, bool enabled)
	{
		startupChoices = choices;
		std::memcpy(startupMap.data(), mapGuid, startupMap.size());
		startupPending = enabled;
	}
	CStartupScope::CStartupScope(const void* mapGuid)
	{
		loading = startupPending && mapGuid != nullptr && std::memcmp(startupMap.data(), mapGuid, 16) == 0;
	}
	CStartupScope::~CStartupScope() { loading = false; startupPending = false; }
	std::uint32_t ResolveStartupTribe(std::uint32_t player, std::uint32_t originalTribe)
	{
		return loading && player < 20 && startupChoices.tribes[player] != 0 && startupChoices.mapTribes[player] == originalTribe ? startupChoices.tribes[player] : originalTribe;
	}
	void ResolveStartupHouse(std::uint32_t player, std::uint32_t& engineType, std::uint32_t& stage, std::uint32_t& point)
	{
		if (!loading || player >= 20 || NC2E2::gStaticHouseStorage.entries == nullptr || engineType >= static_cast<std::uint32_t>(NC2E2::gStaticHouseStorage.count)) return;
		const auto* original = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType);
		if (stage >= 5 || stage >= Read<std::uint32_t>(original, NC2Layout::EngineHouseLogicViewOffset::DevelopmentStageCount)) return;
		const auto originalTribe = Read<std::uint32_t>(original, NC2Layout::EngineHouseLogicViewOffset::TribeType);
		const bool expanded = CivilizationBuildings::IsExpandedPlayer(player);
		const auto selected = expanded ? Read<std::uint32_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType) : ResolveStartupTribe(player, originalTribe);
		if (selected == originalTribe && !expanded) return;
		const auto logic = Read<std::uint32_t>(original, stage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		if (logic >= 55 || NC2Logic::gStaticHouseTypeStorage.records == nullptr) return;
		const auto tryType = [&] (std::uint32_t targetLogic)
		{
			std::uint32_t targetStage = 0;
			auto target = NC2E2::Engine2d_EntryPoint_GetHouseId(selected, targetLogic, &targetStage);
			while (target != -1)
			{
				if (NC2Logic::Tool_House_IsAllowedToBeBuild(player, target, targetStage, point, NC2Logic::gStaticHouseTypeStorage.records[targetLogic].ignoreContinents != 0, false))
				{
					engineType = static_cast<std::uint32_t>(target); stage = targetStage; return true;
				}
				if (expanded)
				{
					// A foreign footprint may need a nearby anchor; retain native terrain/occupancy checks.
					std::uint32_t search[4] = { player, static_cast<std::uint32_t>(target), targetStage, 0 };
					if (NC2Logic::CMapPathfinderElement::SearchOverMap(&point, 12, false, NC2Logic::Tool_HouseBuildPointSearchEvaluationFunction, search, false, player))
					{
						engineType = static_cast<std::uint32_t>(target); stage = targetStage; point = search[3]; return true;
					}
				}
				target = NC2E2::Engine2d_EntryPoint_GetNextHouseId(target, selected, targetLogic, &targetStage);
			}
			return false;
		};
		if (tryType(logic)) return;
		// Foreign main houses use warehouse roles rather than Viking headquarters type 1.
		if (NC2Logic::gStaticHouseTypeStorage.records[logic].mainType == 1)
			for (const auto stock : { 9u, 8u, 7u }) if (stock != logic && tryType(stock)) return;
	}
}
