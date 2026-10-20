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
	namespace
	{
		// Copies borrow all native areas, overlays and graphics. Only this vector owns the outer records.
		std::vector<SEngineHouseTypeStorage> sessionHouses;
		SEngineHouseTypeStorage* nativeHouses = nullptr;
		std::int32_t nativeCount = 0;
		std::array<std::array<std::int32_t, 42>, 8> purposeIds;
		std::array<std::array<std::uint32_t, 42>, 8> purposeStages;
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
		std::int32_t ChooseModel(std::uint32_t tribe, const SEngineHouseTypeStorage& prototype, std::uint32_t& stage)
		{
			// Prefer a real counterpart anywhere in the original upgrade family.
			for (std::uint32_t level = 0; level < prototype.developmentStageCount; ++level)
				if (const auto model = Find(tribe, prototype.logicTypes[level], stage); model != -1) return model;
			const auto choose = [&] (std::initializer_list<std::uint32_t> roles)
			{
				for (auto role : roles) if (const auto model = Find(tribe, role, stage); model != -1) return model;
				return -1;
			};
			std::int32_t preferred = -1;
			switch (prototype.logicTypes[0])
			{
				case 1: case 7: preferred = choose({ 9, 8, 7 }); break;
				case 10: preferred = choose({ 7, 9, 2 }); break;
				case 11: case 12: preferred = choose({ 17, 14 }); break;
				case 13: preferred = choose({ 14, 23, 31, 17 }); break;
				case 14: case 16: preferred = choose({ 14, 18, 17, 20 }); break;
				case 18: preferred = choose({ 19, 18, 23, 20 }); break;
				case 20: case 23: preferred = choose({ 20, 23, 18, 14 }); break;
				case 27: preferred = choose({ 28, 31, 23, 20 }); break;
				case 29: case 31: case 33: preferred = choose({ 29, 31, 20, 23 }); break;
				case 34: preferred = choose({ 17, 18, 14 }); break;
				case 35: case 37: case 38: preferred = choose({ 37, 19, 31, 20 }); break;
				case 39: preferred = choose({ 39, 40, 9, 7, 20 }); break;
				case 40: preferred = choose({ 40, 39 }); break;
			}
			if (preferred != -1) return preferred;
			// Last resort: closest footprint within the same purpose category, then a civilian building.
			std::uint32_t bestScore = UINT32_MAX;
			std::int32_t best = -1;
			const auto wanted = NC2Logic::gStaticHouseTypeStorage.records[prototype.logicTypes[0]].mainType;
			for (std::int32_t index = 0; index < nativeCount; ++index)
			{
				const auto& candidate = nativeHouses[index];
				if (candidate.tribeType != tribe || candidate.developmentStageCount == 0 || candidate.logicTypes[0] == 0 || candidate.logicTypes[0] > 41) continue;
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
		if (!IsExpandedTribe(tribe) || logic == 0 || logic > 41) return -1;
		const auto id = purposeIds[tribe][logic];
		if (id != -1 && stage != nullptr) *stage = purposeStages[tribe][logic];
		return id;
	}
	void Begin()
	{
		if (nativeHouses != nullptr) return;
		if (NC2E2::gStaticHouseStorage.entries == nullptr || NC2E2::gStaticHouseStorage.count <= 0 || NC2Logic::gStaticHouseTypeStorage.records == nullptr) NXBasics::ReportFatalError("Network civilization building data is unavailable.");
		nativeHouses = static_cast<SEngineHouseTypeStorage*>(NC2E2::gStaticHouseStorage.entries);
		nativeCount = NC2E2::gStaticHouseStorage.count;
		for (auto& ids : purposeIds) ids.fill(-1);
		// Build every supported tribe in stable order, independent of active slots, for deterministic saves/peers.
		std::vector<SEngineHouseTypeStorage> records(nativeHouses, nativeHouses + nativeCount);
		records.reserve(static_cast<std::size_t>(nativeCount) + 4u * 41u);
		for (auto tribe : { 2u, 3u, 4u, 7u })
		{
			if (!IsExpandedTribe(tribe)) continue;
			std::vector<std::int32_t> families(static_cast<std::size_t>(nativeCount), -1);
			for (std::uint32_t logic = 1; logic <= 41; ++logic)
			{
				std::uint32_t vikingStage = 0;
				const auto viking = Find(1, logic, vikingStage);
				if (viking == -1) continue;
				if (families[viking] == -1)
				{
					const auto& prototype = nativeHouses[viking];
					if (prototype.developmentStageCount == 0 || prototype.developmentStageCount > 5) NXBasics::ReportFatalError("Invalid network civilization building stages.");
					std::uint32_t modelStage = 0;
					const auto modelId = ChooseModel(tribe, prototype, modelStage);
					if (modelId == -1) NXBasics::ReportFatalError("No building model for network civilization %u.", tribe);
					const auto& model = nativeHouses[modelId];
					auto copy = model;
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
		std::vector<SEngineHouseTypeStorage>().swap(sessionHouses);
	}
	void WriteSaveManifest(NC2Logic::CIoHelper* io)
	{
		if (nativeHouses == nullptr) return;
		io->IO_Tool_Chunk_StartCreation(SaveChunk, 1);
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(nativeCount));
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(sessionHouses.size()));
		io->IO_Tool_Chunk_FinishAndWrite();
	}
	void ReadSaveManifest(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.version != 1 || io->mReadHeader.payloadSize != 8) NXBasics::ReportFatalError("Unsupported network civilization building profile.");
		const auto original = io->IO_File_ReadLong(), total = io->IO_File_ReadLong();
		Begin();
		if (original != static_cast<std::uint32_t>(nativeCount) || total != sessionHouses.size()) NXBasics::ReportFatalError("Network savegame building definitions do not match the installed game data.");
	}
}
