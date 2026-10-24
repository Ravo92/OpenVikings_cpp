#include "RemasterAILog.h"
#include "RemasterAIEconomy.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "RemasterLog.h"
#include "NetworkTypes.h"

#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <Windows.h>

namespace NC2Logic::RemasterAILog
{
	using namespace NC2Lifetime;
	namespace
	{
		struct SRecord { std::string text, topic; std::uint32_t tick = 0, repeated = 0, player = UINT32_MAX; std::int32_t subject = -1; };
		struct SProgress { std::uint64_t value = 0; std::uint32_t since = 0; };
		struct SLogger
		{
			std::atomic<int> level{ 0 };
			std::uint32_t interval = 120, world = 0;
			std::array<std::uint32_t, 20> lastSnapshot{};
			std::array<bool, 20> seenPlayer{};
			std::array<std::unordered_map<std::int32_t, std::uint32_t>, 20> houses, humans;
			// Only the simulation thread touches records/progress. The writer owns only queued strings.
			std::unordered_map<std::string, SRecord> records;
			std::unordered_map<std::string, SProgress> progress;

		};
		SLogger& Log() { static SLogger logger; return logger; }
		std::string Escape(const char* value)
		{
			std::string text;
			if (value == nullptr) return text;
			for (const auto* p = reinterpret_cast<const unsigned char*>(value); *p != 0; ++p)
			{
				if (*p == '"' || *p == '\\') { text += '\\'; text += static_cast<char>(*p); }
				else if (*p < 32 || *p >= 127) { char escaped[7]; std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(*p)); text += escaped; }
				else text += static_cast<char>(*p);
			}
			return text;
		}
		std::string Line(std::uint32_t player, const char* topic, std::int32_t subject, const std::string& message, std::uint32_t repeated)
		{
			SYSTEMTIME time; GetLocalTime(&time);
			char prefix[256];
			std::snprintf(prefix, sizeof(prefix), "{\"time\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03u\",\"world\":%u,\"tick\":%u,\"player\":%d,\"subject\":%d,\"repeated\":%u,\"event\":\"", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, Log().world, gGameTime.tick, static_cast<std::int32_t>(player), subject, repeated);
			return std::string(prefix) + Escape(topic) + "\",\"message\":\"" + Escape(message.c_str()) + "\"}\n";
		}
		void Queue(std::string line)
		{
			NRemaster::Logging::Write(NRemaster::Logging::ECategory::AI, std::move(line));
		}
		void FlushRepetitions()
		{
			for (auto& entry : Log().records)
			{
				auto& record = entry.second;
				if (record.repeated == 0) continue;
				Queue(Line(record.player, record.topic.c_str(), record.subject, record.text.c_str(), record.repeated));
				record.repeated = 0;
			}
		}
		std::uint32_t Waiting(const std::string& key, std::uint64_t value)
		{
			auto& entries = Log().progress;
			if (entries.size() > 16384) entries.clear();
			const auto found = entries.find(key);
			if (found == entries.end()) { entries.emplace(key, SProgress{ value, gGameTime.tick }); return 0; }
			if (found->second.value != value) found->second = SProgress{ value, gGameTime.tick };
			return gGameTime.tick - found->second.since;
		}
		std::uint64_t Mix(std::uint64_t hash, std::uint32_t value) { return (hash ^ value) * 1099511628211ull; }
		const char* Good(std::uint32_t id) { return id < 66 && gStaticGoodTypeStorage.records != nullptr ? gStaticGoodTypeStorage.records[id].name : "unknown"; }
		bool Recorded(std::uint32_t player, const char* topic, std::int32_t subject)
		{
			return Log().records.find(std::to_string(player) + ":" + topic + ":" + std::to_string(subject)) != Log().records.end();
		}
		void Removed(std::uint32_t player, const char* topic, std::unordered_map<std::int32_t, std::uint32_t>& previous, std::unordered_map<std::int32_t, std::uint32_t>& current)
		{
			for (const auto& entry : previous)
			{
				const auto found = current.find(entry.first);
				if (found == current.end() || found->second != entry.second) Event(player, topic, entry.first, "revision=%u no_longer_present_in_owner_snapshot; removed, ownership changed or slot reused", entry.second);
			}
			previous.swap(current);
		}
		void Snapshot(std::uint32_t player)
		{
			std::unordered_map<std::int32_t, std::uint32_t> currentHouses, currentHumans;
			const auto* single = Read<const void*>(gAIManager, player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
			const auto* high = Read<const void*>(gAIManager, NC2Layout::AIManagerOffset::HighLevelHandlers + player * NC2Layout::RecordSize::AIPlayerHandlerPointer);
			const auto w = RemasterAIEconomy::Inspect(player);
			Event(player, "heartbeat", -1, "population=%u workers=%u builders=%u women=%u children=%u soldiers=%u food=%u sites=%u vacant_workplaces=%u food_vacancies=%u single_enabled=%u high_enabled=%u unit_limit=%u max_unit_limit=%u", w.population, w.workers, w.builders, w.women, w.children, w.soldiers, w.foodStock, w.constructionSites, w.vacantWorkplaces, w.foodVacancies, single != nullptr ? Read<std::uint8_t>(single, NC2Layout::AISinglePlayerOffset::Enabled) : 0, high != nullptr ? Read<std::uint8_t>(high, NC2Layout::AIHighLevelOffset::Enabled) : 0, single != nullptr ? Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::UnitLimit) : 0, single != nullptr ? Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::MaxUnitLimit) : 0);
			if (w.builders == 0) Event(player, "blocker", 0, "no_builder: construction cannot advance until an eligible worker changes job");
			if (w.foodStock == 0) Event(player, "blocker", 1, "no_stored_food: recruitment and births may wait for food delivery");
			if (high != nullptr && Read<std::uint8_t>(high, NC2Layout::AIHighLevelOffset::Enabled) != 0)
			{
				for (std::uint32_t i = 0; i < 8; ++i)
				{
					const auto* slot = static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::Villages + i * NC2Layout::RecordSize::AIVillageSlot;
					if (Read<std::uint8_t>(slot, NC2Layout::AIVillageSlotViewOffset::Active) == 0) continue;
					const auto* village = slot + NC2Layout::AIVillageSlotViewOffset::Village;
					const auto* center = village + NC2Layout::AIVillageOffset::CenterPoint;
					const auto* build = village + NC2Layout::AIVillageOffset::BuildHouse;
					Event(player, "village", static_cast<std::int32_t>(i), "type=%u center=(%d,%d) radius=%u warehouses=%u has_home=%u starter_home_priority=%u build_active=%u build_house=%d build_revision=%u", Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Type), Read<std::int16_t>(center, 0), Read<std::int16_t>(center, 2), Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::Radius), Read<std::uint32_t>(village, NC2Layout::AIVillageOffset::StockCount), Read<std::uint8_t>(village, NC2Layout::AIVillageOffset::Houses + NC2Layout::AIHouseManagerOffset::HasHomeFlag), RemasterAIEconomy::NeedsStarterHome(village), Read<std::uint8_t>(build, 0), Read<std::int32_t>(build, 4), Read<std::uint32_t>(build, 8));
					for (std::uint32_t good = 0; good < 66; ++good)
					{
						const auto* entry = village + NC2Layout::AIVillageOffset::Goods + good * NC2Layout::RecordSize::AIGood;
						if (Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount) == 0 && Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag) == 0 && Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount) == 0 && !Recorded(player, "goods", static_cast<std::int32_t>(i * 66 + good))) continue;
						Event(player, "goods", static_cast<std::int32_t>(i * 66 + good), "village=%u good=%u name=%.30s need=%u producing=%u confirmations=%u forced=%u importing=%u carrier_import=%u producer_count=%u human=%d human_revision=%u house=%d house_revision=%u", i, good, Good(good), Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::NeedCount), Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ProducingFlag), Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProductionConfirmationCount), Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ForceProducingFlag), Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::ImportingFlag), Read<std::uint8_t>(entry, NC2Layout::AIGoodOffset::TraderBringFlag), Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::ProducerCount), Read<std::int32_t>(entry, NC2Layout::AIGoodOffset::Producer), Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::Producer + 4), Read<std::int32_t>(entry, NC2Layout::AIGoodOffset::Producer + 8), Read<std::uint32_t>(entry, NC2Layout::AIGoodOffset::Producer + 12));
					}
				}
				const auto* military = static_cast<const std::byte*>(high) + NC2Layout::AIHighLevelOffset::Military;
				Event(player, "military", -1, "target_player=%d target_point=%d soldiers=%u military_limit=%u recruitment_vacancies=%u civilian_workers=%u food=%u", Read<std::int32_t>(military, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyPlayer), Read<std::int32_t>(military, NC2Layout::AIMilitaryManagerOffset::TargetedEnemyMilitaryPointIndex), w.soldiers, RemasterAIEconomy::MilitaryLimit(), w.recruitmentVacancies, w.workers, w.foodStock);
			}
			if (single != nullptr)
			{
				for (std::uint32_t i = 0; i < 20; ++i)
				{
					const auto* request = static_cast<const std::byte*>(single) + NC2Layout::AISinglePlayerOffset::HumanRequests + i * NC2Layout::RecordSize::AIHumanRequest;
					if (Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::Type) == 0 && !Recorded(player, "worker_request", static_cast<std::int32_t>(i))) continue;
					Event(player, "worker_request", static_cast<std::int32_t>(i), "type=%u target=%u target_revision=%u human=%d human_revision=%u required_job=%u", Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::Type), Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::TargetHouseIdOrPoint), Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::TargetRevision), Read<std::int32_t>(request, NC2Layout::AIHumanRequestOffset::HumanId), Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::HumanRevision), Read<std::uint32_t>(request, NC2Layout::AIHumanRequestOffset::RequiredJob));
				}
				const auto last = Read<std::int32_t>(single, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex);
				for (std::int32_t i = 0; i <= last && i < NC2Runtime::AISinglePlayerLimits::MainTasks; ++i)
				{
					const auto* task = static_cast<const std::byte*>(single) + NC2Layout::AISinglePlayerOffset::MainTasks + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::AIMainTask;
					Event(player, "military_task", i, "disabled=%u type=%u priority=%u active_priority=%u target=(%d,%d) radius=%u soldiers_min=%u soldiers_max=%u rally=(%d,%d) created_tick=%u", Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::Disabled), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ConfiguredPriority), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::ActivePriority), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPositionY), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Radius), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::MinimumSoldiers), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::MaximumSoldiers), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPositionY), Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::CreatedTick));
				}
				const auto combined = Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
				for (std::uint32_t i = 0; i < combined && i < 50; ++i)
				{
					const auto* task = static_cast<const std::byte*>(single) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
					const auto allocated = Read<std::uint32_t>(single, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + i * NC2Layout::RecordSize::AITaskIndexValue);
					const auto minimum = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers);
					Event(player, "wave", static_cast<std::int32_t>(i), "type=%u phase=%u allocated=%u minimum=%u maximum=%u rally=(%d,%d) status=%s", Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::TaskType), Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase), allocated, minimum, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers), Read<std::int16_t>(task, NC2Layout::AICombinedTaskOffset::FormationPosition), Read<std::int16_t>(task, NC2Layout::AICombinedTaskOffset::FormationPositionY), allocated < minimum ? "waiting_for_required_soldiers" : "minimum_allocated; inspect phase/rally and soldier state");
				}
			}
			CHouseIterator houses;
			for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
			{
				const auto* house = GetHousePtr(id); const auto* type = House_GetHouseTypeDataPtr(house);
				const auto revision = Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId);
				currentHouses.emplace(id, revision);
				std::uint64_t progress = Mix(revision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone));
				std::uint32_t attached = 0;
				for (std::uint32_t i = 0; i < 20; ++i) if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::Attachments + i * NC2Layout::RecordSize::HouseAttachment) != 0) ++attached;
				// Read real stock slots, including workshop output. The public stockOnly getter hides output slots.
				for (std::uint32_t slot = 0; slot < type->stockCount && slot < 66; ++slot)
				{
					const auto good = static_cast<std::uint32_t>(type->stock[slot].goodType);
					const auto amount = Read<std::uint32_t>(house, NC2Layout::HouseOffset::StockAmounts + slot * NC2Layout::RecordSize::HouseStockValueUnsigned);
					const auto incoming = Read<std::uint32_t>(house, NC2Layout::HouseOffset::FutureStockAmounts + slot * NC2Layout::RecordSize::HouseStockValueUnsigned);
					const auto requests = Read<std::uint32_t>(house, NC2Layout::HouseOffset::StockRequestCounters + slot * NC2Layout::RecordSize::HouseStockValueUnsigned);
					progress = Mix(progress, amount);
					if (amount == 0 && incoming == 0 && requests == 0 && !Recorded(player, "stock", id * 66 + static_cast<std::int32_t>(slot))) continue;
					Event(player, "stock", id * 66 + static_cast<std::int32_t>(slot), "house=%d revision=%u slot=%u good=%u name=%.30s amount=%u incoming=%u requests=%u minimum=%u output=%u", id, revision, slot, good, Good(good), amount, incoming, requests, Read<std::uint32_t>(house, NC2Layout::HouseOffset::StockMinimumAmounts + slot * NC2Layout::RecordSize::HouseStockValueUnsigned), type->stock[slot].isOutputStock);
				}
				const auto wait = Waiting("house:" + std::to_string(id) + ":" + std::to_string(revision), progress);
				Event(player, "house", id, "revision=%u logic=%u name=%.50s position=(%d,%d) state=%u steps=%u/%u goods_delivered=%u/%u attached_people=%u production_stopped=%u production_counter=%u no_progress_ticks=%u", revision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), type->debugName, Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPosition), Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPosition + 2), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsDone), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionStepsRequired), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsDelivered), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsRequired), attached, Read<std::uint8_t>(house, NC2Layout::HouseOffset::ProductionStoppedFlag), Read<std::uint32_t>(house, NC2Layout::HouseOffset::ProductionCounter), wait);
				for (std::int32_t i = 0; i < type->workerCount && i < 10; ++i)
				{
					const auto job = static_cast<std::uint32_t>(type->workers[i].jobType);
					Event(player, "workplace", id * 10 + i, "house=%d revision=%u job=%u attached=%u capacity=%d", id, revision, job, House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job), type->workers[i].maximumWorkers);
				}
				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
				{
					const auto count = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionGoodsCount);
					for (std::uint32_t i = 0; i < count && i < 10; ++i)
					{
						const auto* entry = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::ConstructionGoodEntries + i * sizeof(SHouseConstructionGoodsEntry);
						Event(player, "construction_material", id * 10 + static_cast<std::int32_t>(i), "house=%d good=%u required=%u delivered=%u incoming=%u consumed=%u", id, Read<std::uint32_t>(entry, 0), Read<std::uint32_t>(entry, 4), Read<std::uint32_t>(entry, 8), Read<std::uint32_t>(entry, 12), Read<std::uint32_t>(entry, 16));
					}
				}
			}
			CHumanIterator humans;
			for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
			{
				const auto* human = GetHumanPtr(id); const auto* position = Human_GetMapPositionRef(human);
				const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);
				currentHumans.emplace(id, revision);
				std::uint64_t progress = Mix(revision, Read<std::uint32_t>(position, 0));
				for (const auto offset : { NC2Layout::HumanOffset::JobType, NC2Layout::HumanOffset::CurrentTask, NC2Layout::HumanOffset::CurrentTaskStep, NC2Layout::HumanOffset::CarriedGood }) progress = Mix(progress, Read<std::uint32_t>(human, offset));
				for (std::uint32_t good = 0; good < 66; ++good) progress = Mix(progress, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * 4));
				const auto wait = Waiting("human:" + std::to_string(id) + ":" + std::to_string(revision), progress);
				Event(player, "human", id, "revision=%u job=%u produced_good=%u position=(%d,%d) work_center=(%d,%d) task=%u status=%u step=%u atomic=%u carried_good=%u requested_good=%u work_house=%d home=%d spouse=%d food=%u stamina=%u retries=%u no_progress_ticks=%u", revision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType), Read<std::int16_t>(position, 0), Read<std::int16_t>(position, 2), Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenter), Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenter + 2), Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask), Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus), Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep), Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction), Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId), Read<std::int32_t>(human, NC2Layout::HumanOffset::HomeHouseId), Read<std::int32_t>(human, NC2Layout::HumanOffset::SpouseId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::Food), Read<std::uint32_t>(human, NC2Layout::HumanOffset::Stamina), Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount), wait);
				for (std::uint32_t i = 0; i < 87; ++i)
				{
					const auto* command = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::UserCommandQueue + i * NC2Layout::RecordSize::UserNetworkCommand;
					const auto opcode = Read<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode);
					if (opcode != 0 || Recorded(player, "pending_command", id * 87 + static_cast<std::int32_t>(i))) Event(player, "pending_command", id * 87 + static_cast<std::int32_t>(i), "human=%d opcode=%u parameter1=%u parameter2=%u", id, opcode, Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1), Read<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2));
				}
				Event(player, "human_target", id, "revision=%u target=(%d,%d) target_object=%d target_revision=%u task_parameter=%u failed_tasks=%u weapon=%u armor=%u home_revision=%u work_revision=%u", revision, Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetPositionX), Read<std::int16_t>(human, NC2Layout::HumanOffset::TargetPositionY), Read<std::int32_t>(human, NC2Layout::HumanOffset::TargetObjectId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision), Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter), Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount), Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon), Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseRevision), Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision));
				const auto moveableId = Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId);
				if (moveableId >= 0 && gMapMoveableArrayStorage.manager != nullptr && moveableId < gMapMoveableArrayStorage.manager->mCapacity && gMapMoveableArrayStorage.entries != nullptr)
				{
					const auto* moveable = GetMapMoveablePtr(moveableId);
					Event(player, "movement", id, "moveable=%d pathfinder=%d walking=%u direction=%u step_progress=%u turning=%u", moveableId, Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId), Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag), Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection), Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress), Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag));
				}
			}
			Removed(player, "house_left_snapshot", Log().houses[player], currentHouses);
			Removed(player, "human_left_snapshot", Log().humans[player], currentHumans);
		}
	}
	void Initialize() noexcept
	{
		if (!NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::AI)) return;
		Log().level.store(2);
		try { Queue(Line(UINT32_MAX, "log_start", -1, "AI diagnostics schema=2; full snapshots; ticks=12/second; background FIFO; process=" + std::to_string(GetCurrentProcessId()) + "; build=" __DATE__ " " __TIME__, 0)); }
		catch (...) { OutputDebugStringA("OpenVikings: could not initialize AI diagnostics.\n"); }
	}
	void Shutdown() noexcept
	{
		try { if (Log().level.load() != 0) { FlushRepetitions(); Queue(Line(UINT32_MAX, "log_end", -1, "AI producers stopped; shared logger drains at application shutdown", 0)); } Log().level.store(0); }
		catch (...) { Log().level.store(0); OutputDebugStringA("OpenVikings: could not finalize AI diagnostic summaries.\n"); }
	}
	bool Enabled(std::uint32_t player, int level) noexcept
	{
		if (!NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::AI) || Log().level.load() < level || player >= 20 || gGameControl == nullptr || gAIManager == nullptr) return false;
		const auto* data = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;
		return Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) != 0 && Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type) == 2;
	}
	void ResetWorld(const char* reason) noexcept
	{
		if (Log().level.load() == 0) return;
		try { auto& log = Log(); FlushRepetitions(); ++log.world; log.records.clear(); log.progress.clear(); log.seenPlayer.fill(false); for (auto& entries : log.houses) entries.clear(); for (auto& entries : log.humans) entries.clear(); Queue(Line(UINT32_MAX, "world_reset", -1, reason, 0)); } catch (...) { }
	}
	void Event(std::uint32_t player, const char* topic, std::int32_t subject, const char* format, ...) noexcept
	{
		if (player == UINT32_MAX ? Log().level.load() == 0 : !Enabled(player)) return;
		try
		{
			va_list args; va_start(args, format); const auto count = _vscprintf(format, args); va_end(args);
			if (count < 0) return;
			std::string message(static_cast<std::size_t>(count) + 1, '\0');
			va_start(args, format); std::vsnprintf(message.data(), message.size(), format, args); va_end(args); message.resize(static_cast<std::size_t>(count));
			auto& log = Log(); if (log.records.size() > 32768) { FlushRepetitions(); log.records.clear(); }
			const auto key = std::to_string(player) + ":" + topic + ":" + std::to_string(subject);
			auto& record = log.records[key]; record.player = player; record.subject = subject; record.topic = topic;
			const bool full = log.level.load() >= 2 && (std::string(topic) == "human" || std::string(topic) == "house" || std::string(topic) == "stock" || std::string(topic) == "goods");
			if (!full && record.text == message && gGameTime.tick - record.tick < 720) { ++record.repeated; return; }
			if (record.text != message && record.repeated != 0) Queue(Line(player, topic, subject, record.text.c_str(), record.repeated));
			Queue(Line(player, topic, subject, message, record.text == message ? record.repeated : 0));
			record.text = message; record.tick = gGameTime.tick; record.repeated = 0;
		}
		catch (...) { OutputDebugStringA("OpenVikings: AI record formatting/snapshot failed.\n"); }
	}
	void Update(std::uint32_t player) noexcept
	{
		if (!Enabled(player)) return;
		try
		{
			auto& log = Log(); const auto tick = gGameTime.tick;
			if (log.seenPlayer[player] && tick < log.lastSnapshot[player]) ResetWorld("simulation clock moved backwards (reload/restart)");
			if (log.seenPlayer[player] && tick - log.lastSnapshot[player] < log.interval) return;
			log.seenPlayer[player] = true; log.lastSnapshot[player] = tick;
			char guid[33]; for (std::size_t i = 0; i < 16; ++i) std::snprintf(guid + i * 2, 3, "%02x", gGameStaticVars.mapGuid.bytes[i]);
			Event(player, "snapshot", -1, "snapshot tick=%u protocol=%u network_game=%u map_guid=%s civilization=%u interval_ticks=%u", tick, NNetwork::NetworkProtocolVersion, gGameStaticVars.networkGame, guid, Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType), log.interval);
			Snapshot(player);
		}
		catch (...) { OutputDebugStringA("OpenVikings: AI record formatting/snapshot failed.\n"); }
	}
}
