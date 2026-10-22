#include "CivilizationBuildings.h"
#include "C2E2StaticDataHouses.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapArea.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "GameProfile.h"
#include "XDiagnostics.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <type_traits>
#include <vector>

namespace NRemaster::CivilizationBuildings
{
	using namespace NC2Lifetime;
	using NC2E2::SEngineHouseTypeStorage;
	using HouseType = NC2Logic::EHouseType;
	namespace
	{
		constexpr auto FirstPurpose = static_cast<std::uint32_t>(HouseType::Headquarters);
		constexpr auto LastPurpose = static_cast<std::uint32_t>(HouseType::Tower01);
		constexpr auto PurposeCount = LastPurpose + 1;
		// Copies borrow all native areas, overlays and graphics. Only this vector owns the outer records.
		std::vector<SEngineHouseTypeStorage> sessionHouses;
		SEngineHouseTypeStorage* nativeHouses = nullptr;
		std::int32_t nativeCount = 0;
		std::uint32_t activeProfileVersion = 2;
		std::array<std::array<std::int32_t, PurposeCount>, 8> purposeIds;
		std::array<std::array<std::uint32_t, PurposeCount>, 8> purposeStages;
		static_assert(sizeof(SEngineHouseTypeStorage) == NC2Layout::RecordSize::EngineHouseType);
		static_assert(std::is_trivially_copyable_v<SEngineHouseTypeStorage>);

		std::int32_t Find(std::uint32_t tribe, std::uint32_t logic, std::uint32_t& stage)
		{
			for (std::int32_t index = 0; index < nativeCount; ++index)
				if (nativeHouses[index].tribeType == tribe)
					for (std::uint32_t level = 0; level < (std::min)(5u, nativeHouses[index].developmentStageCount); ++level)
						if (nativeHouses[index].logicTypes[level] == logic) { stage = level; return index; }
			return -1;
		}
		std::uint32_t Footprint(const SEngineHouseTypeStorage& house)
		{
			const auto* area = static_cast<const NC2Logic::CMapArea*>(house.buildArea);
			return area == nullptr ? 0u : area->pointCount;
		}
		std::int32_t ChooseHeadquartersModel(std::uint32_t tribe, std::uint32_t& stage)
		{
			if (tribe == 7)
			{
				const auto pyramid = Find(1, static_cast<std::uint32_t>(HouseType::WonderPyramidOfGizeh), stage);
				if (pyramid != -1) stage = (std::min)(5u, nativeHouses[pyramid].developmentStageCount) - 1u;
				return pyramid;
			}
			const auto* name = tribe == 2 ? "frank castle" : tribe == 3 ? "byzantine cathedral" : tribe == 4 ? "saracen palace" : nullptr;
			if (name == nullptr) return -1;
			for (std::int32_t index = 0; index < nativeCount; ++index)
				if (nativeHouses[index].tribeType == tribe && nativeHouses[index].developmentStageCount != 0 && std::strcmp(nativeHouses[index].editName, name) == 0) { stage = 0; return index; }
			return -1;
		}
		std::int32_t ChooseModel(std::uint32_t tribe, const SEngineHouseTypeStorage& prototype, std::uint32_t& stage)
		{
			// Older saves retain the exact footprints and doors used when their map was saved.
			if (activeProfileVersion >= 2 && static_cast<HouseType>(prototype.logicTypes[0]) == HouseType::Headquarters)
				if (const auto headquarters = ChooseHeadquartersModel(tribe, stage); headquarters != -1) return headquarters;
			// Prefer a real counterpart anywhere in the original upgrade family.
			for (std::uint32_t level = 0; level < prototype.developmentStageCount; ++level)
				if (const auto model = Find(tribe, prototype.logicTypes[level], stage); model != -1) return model;
			const auto choose = [&] (std::initializer_list<HouseType> roles)
			{
				for (auto role : roles) if (const auto model = Find(tribe, static_cast<std::uint32_t>(role), stage); model != -1) return model;
				return -1;
			};
			std::int32_t preferred = -1;
			switch (static_cast<HouseType>(prototype.logicTypes[0]))
			{
				case HouseType::Headquarters: case HouseType::Stock00: preferred = choose({ HouseType::Stock02, HouseType::Stock01, HouseType::Stock00 }); break;
				case HouseType::Farm: preferred = choose({ HouseType::Breedery, HouseType::Bakery00 }); break;
				case HouseType::Mill: preferred = choose({ HouseType::Bakery00, HouseType::Joinery00, HouseType::Smithy00, HouseType::Breedery }); break;
				case HouseType::Bakery00: case HouseType::Brewery: preferred = choose({ HouseType::Bakery00, HouseType::Sewery00, HouseType::Breedery, HouseType::Pottery00 }); break;
				case HouseType::Sewery00: preferred = choose({ HouseType::Sewery01, HouseType::Sewery00, HouseType::Joinery00, HouseType::Pottery00 }); break;
				case HouseType::Pottery00: case HouseType::Joinery00: preferred = choose({ HouseType::Pottery00, HouseType::Joinery00, HouseType::Sewery00, HouseType::Bakery00 }); break;
				case HouseType::Armory00: preferred = choose({ HouseType::Armory01, HouseType::Smithy00, HouseType::Joinery00, HouseType::Pottery00 }); break;
				case HouseType::MasonHut00: case HouseType::Smithy00: case HouseType::CoinMint: preferred = choose({ HouseType::MasonHut00, HouseType::Smithy00, HouseType::Pottery00, HouseType::Joinery00 }); break;
				case HouseType::HerbHut: preferred = choose({ HouseType::Breedery, HouseType::Sewery00, HouseType::Bakery00 }); break;
				case HouseType::DruidHut00: case HouseType::Temple: case HouseType::School: preferred = choose({ HouseType::Temple, HouseType::Sewery01, HouseType::Smithy00, HouseType::Pottery00 }); break;
				case HouseType::Barracks: preferred = choose({ HouseType::Barracks, HouseType::Tower00, HouseType::Stock02, HouseType::Stock00, HouseType::Pottery00 }); break;
				case HouseType::Tower00: preferred = choose({ HouseType::Tower00, HouseType::Barracks }); break;
			}
			if (preferred != -1) return preferred;
			// Last resort: closest footprint within the same purpose category, then a civilian building.
			std::uint32_t bestScore = UINT32_MAX;
			std::int32_t best = -1;
			const auto wanted = NC2Logic::gStaticHouseTypeStorage.records[prototype.logicTypes[0]].mainType;
			for (std::int32_t index = 0; index < nativeCount; ++index)
			{
				const auto& candidate = nativeHouses[index];
				if (candidate.tribeType != tribe || candidate.developmentStageCount == 0 || candidate.logicTypes[0] < FirstPurpose || candidate.logicTypes[0] > LastPurpose) continue;
				const auto category = NC2Logic::gStaticHouseTypeStorage.records[candidate.logicTypes[0]].mainType;
				const auto a = Footprint(candidate), b = Footprint(prototype);
				const auto penalty = category == wanted ? 0u : category == 2 ? 10000u : 20000u;
				const auto score = penalty + (a > b ? a - b : b - a);
				if (score < bestScore) { bestScore = score; best = index; stage = 0; }
			}
			return best;
		}
	}

	bool IsExpandedTribe(std::uint32_t tribe)
	{
		return nativeHouses != nullptr && ((tribe >= 2 && tribe <= 4) || (tribe == 7 && NRemaster::gGameVariant != NRemaster::EGameVariant::Cultures2));
	}
	bool IsActive() { return nativeHouses != nullptr; }
	bool IsExpandedPlayer(std::uint32_t player)
	{
		if (player >= 20 || NC2Logic::gGameControl == nullptr) return false;
		const auto* data = static_cast<const std::byte*>(NC2Logic::gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
		const auto type = Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type);
		return Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && (type == 2 || (type == 1 && NC2Logic::gGameStaticVars.networkGame == 0)) && IsExpandedTribe(Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::TribeType));
	}
	std::int32_t GetHouseId(std::uint32_t tribe, std::uint32_t logic, std::uint32_t* stage)
	{
		if (!IsExpandedTribe(tribe) || logic < FirstPurpose || logic > LastPurpose) return -1;
		const auto id = purposeIds[tribe][logic];
		if (id != -1 && stage != nullptr) *stage = purposeStages[tribe][logic];
		return id;
	}
	void Begin(std::uint32_t profileVersion)
	{
		if (profileVersion < 1 || profileVersion > 2) NXBasics::ReportFatalError("Unsupported civilization building profile version.");
		if (nativeHouses != nullptr)
		{
			if (activeProfileVersion != profileVersion) NXBasics::ReportFatalError("Conflicting civilization building profile versions.");
			return;
		}
		if (NC2E2::gStaticHouseStorage.entries == nullptr || NC2E2::gStaticHouseStorage.count <= 0 || NC2Logic::gStaticHouseTypeStorage.records == nullptr) NXBasics::ReportFatalError("Network civilization building data is unavailable.");
		nativeHouses = static_cast<SEngineHouseTypeStorage*>(NC2E2::gStaticHouseStorage.entries);
		nativeCount = NC2E2::gStaticHouseStorage.count;
		activeProfileVersion = profileVersion;
		for (auto& ids : purposeIds) ids.fill(-1);
		// Build every supported tribe in stable order, independent of active slots, for deterministic saves/peers.
		std::vector<SEngineHouseTypeStorage> records(nativeHouses, nativeHouses + nativeCount);
		records.reserve(static_cast<std::size_t>(nativeCount) + 4u * LastPurpose);
		for (auto tribe : { 2u, 3u, 4u, 7u })
		{
			if (!IsExpandedTribe(tribe)) continue;
			std::vector<std::int32_t> families(static_cast<std::size_t>(nativeCount), -1);
			for (std::uint32_t logic = FirstPurpose; logic <= LastPurpose; ++logic)
			{
				std::uint32_t vikingStage = 0;
				const auto viking = Find(1, logic, vikingStage);
				if (viking == -1) continue;
				if (families[viking] == -1)
				{
					const auto& prototype = nativeHouses[viking];
					if (prototype.developmentStageCount == 0 || prototype.developmentStageCount > 5) NXBasics::ReportFatalError("Invalid network civilization building stages.");
					std::uint32_t modelStage = 0;
					// Wells and beehives retain their original models for every civilization.
					const auto modelId = static_cast<HouseType>(prototype.logicTypes[0]) == HouseType::Well || static_cast<HouseType>(prototype.logicTypes[0]) == HouseType::Hive ? viking : ChooseModel(tribe, prototype, modelStage);
					if (modelId == -1) NXBasics::ReportFatalError("No building model for network civilization %u.", tribe);
					const auto& model = nativeHouses[modelId];
					auto copy = model;
					copy.tribeType = tribe;
					std::snprintf(copy.editName, sizeof(copy.editName), "network-%u-purpose-%u", tribe, prototype.logicTypes[0]);
					copy.developmentStageCount = prototype.developmentStageCount;
					std::copy_n(prototype.logicTypes, 5, copy.logicTypes);
					std::copy_n(prototype.constructionGoods, 5, copy.constructionGoods);
					std::copy_n(prototype.maximumHitpoints, 5, copy.maximumHitpoints);
					for (std::uint32_t level = 0; level < copy.developmentStageCount; ++level)
					{
						copy.walkAreas[level] = model.walkAreas[modelStage]; copy.workAreas[level] = model.workAreas[modelStage];
						copy.doorPoints[level] = model.doorPoints[modelStage]; copy.stageBobIds[level] = model.stageBobIds[modelStage];
						copy.doorOverlays[level] = model.doorOverlays[modelStage]; copy.constructionLayers[level] = model.constructionLayers[modelStage];
						copy.stagePoints[level] = model.stagePoints[modelStage];
						std::copy_n(model.landscapeOverlays[modelStage], 10, copy.landscapeOverlays[level]);
					}
					if (model.logicTypes[modelStage] != prototype.logicTypes[0]) copy.actionOverlayCount = 0;
					families[viking] = static_cast<std::int32_t>(records.size()); records.push_back(copy);
				}
				purposeIds[tribe][logic] = families[viking]; purposeStages[tribe][logic] = vikingStage;
			}
		}
		sessionHouses.swap(records);
		NC2E2::gStaticHouseStorage.entries = sessionHouses.data();
		NC2E2::gStaticHouseStorage.count = static_cast<std::int32_t>(sessionHouses.size());
	}
	void End()
	{
		if (nativeHouses == nullptr) return;
		NC2E2::gStaticHouseStorage.entries = nativeHouses; NC2E2::gStaticHouseStorage.count = nativeCount;
		nativeHouses = nullptr; nativeCount = 0;
		activeProfileVersion = 2;
		std::vector<SEngineHouseTypeStorage>().swap(sessionHouses);
	}
	void WriteSaveManifest(NC2Logic::CIoHelper* io)
	{
		if (nativeHouses == nullptr) return;
		io->IO_Tool_Chunk_StartCreation(SaveChunk, activeProfileVersion);
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(nativeCount));
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(sessionHouses.size()));
		io->IO_Tool_Chunk_FinishAndWrite();
	}
	void ReadSaveManifest(NC2Logic::CIoHelper* io)
	{
		const auto profileVersion = io->mReadHeader.version;
		if (profileVersion < 1 || profileVersion > 2 || io->mReadHeader.payloadSize != 8) NXBasics::ReportFatalError("Unsupported network civilization building profile.");
		const auto original = io->IO_File_ReadLong(), total = io->IO_File_ReadLong();
		Begin(profileVersion);
		if (original != static_cast<std::uint32_t>(nativeCount) || total != sessionHouses.size()) NXBasics::ReportFatalError("Network savegame building definitions do not match the installed game data.");
	}
}
