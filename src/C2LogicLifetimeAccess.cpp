#include "C2HumanMerchantAccess.h"
#include "ApplicationGlobals.h"
#include "CivilizationBuildings.h"
#include "C2CampaignStaticDataManager.h"
#include "C2CleanMapsManager.h"
#include "C2EngineTerrainData.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapSectorAttachment.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2SavegamesManager.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticHouseTypeData.h"
#include "C2StaticLandscapeTypeData.h"
#include "C2StaticStringsManager.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "C2StaticTribeTypeData.h"
#include "C2StaticVehicleTypeData.h"
#include "SwitchIdentifiers.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XIniFileReader.h"

#include <new>

namespace NC2Logic
{
	// Original function: FUN_00418cc6; the manager itself does not participate in validation.
	bool GameAssistantManager_l_Assistant_IsPlayerIdOk(const void*, std::int32_t player)
	{
		return player >= 0 && player < 20 && NC2Lifetime::Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) != 0;
	}
	// Original function: FUN_00418ae1; additions wrap before signed clamping to 0..20.
	void GameAssistantManager_UserInput_Save(void* manager, std::int32_t player, std::int32_t command, std::int32_t value)
	{
		using namespace NC2Lifetime;
		if (player < 0 || player >= 20 || Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0) return;
		const auto base = static_cast<std::uint32_t>(player) * 0x38u;
		std::uint32_t offset;
		switch (command)
		{
			case SwitchIds::GameAssistantCommand::RequestBoys: offset = 0; break;
			case SwitchIds::GameAssistantCommand::RequestGirls: offset = 4; break;
			case SwitchIds::GameAssistantCommand::RequestUnarmedSoldiers: offset = 8; break;
			case SwitchIds::GameAssistantCommand::RequestSpearmen: offset = 12; break;
			case SwitchIds::GameAssistantCommand::RequestArchers: offset = 16; break;
			case SwitchIds::GameAssistantCommand::RequestSwordsmen: offset = 20; break;
			case SwitchIds::GameAssistantCommand::EquipCivilianShoes: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipCivilianShoes, value != 0); return;
			case SwitchIds::GameAssistantCommand::EquipCivilianWoodenTools: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipCivilianWoodenTools, value != 0); return;
			case SwitchIds::GameAssistantCommand::EquipCivilianIronTools: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipCivilianIronTools, value != 0); return;
			case SwitchIds::GameAssistantCommand::EquipCivilianMead: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipCivilianMead, value != 0); return;
			case SwitchIds::GameAssistantCommand::EquipSoldierShoes: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipSoldierShoes, value != 0); return;
			case SwitchIds::GameAssistantCommand::EquipSoldierMead: Write<std::uint8_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::EquipSoldierMead, value != 0); return;
			default: return;
		}
		const auto previous = Read<std::uint32_t>(manager, base + offset);
		auto adjusted = NXBasics::Runtime_TimeSigned32(previous + static_cast<std::uint32_t>(value));
		if (adjusted <= 0) adjusted = 0;
		if (adjusted >= 20) adjusted = 20;
		const auto delta = static_cast<std::uint32_t>(adjusted) - previous;
		Write(manager, base + offset, previous + delta);
		if (command == 11)
		{
			Write(manager, base + NC2Layout::GameAssistantPlayerOffset::GirlRequestBudget, Read<std::uint32_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::GirlRequestBudget) + delta);
			return;
		}
		Write(manager, base + NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget, Read<std::uint32_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::BoyRequestBudget) + delta);
		if (command == 10) return;
		Write(manager, base + NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget, Read<std::uint32_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::RecruitRequestBudget) + delta);
		if (command >= 21) Write(manager, base + NC2Layout::GameAssistantPlayerOffset::SpearmanRequestBudgetUnsigned + static_cast<std::uint32_t>(command - SwitchIds::GameAssistantCommand::RequestSpearmen) * NC2Layout::RecordSize::AssistantRequestValueUnsigned, Read<std::uint32_t>(manager, base + NC2Layout::GameAssistantPlayerOffset::SpearmanRequestBudgetUnsigned + static_cast<std::uint32_t>(command - SwitchIds::GameAssistantCommand::RequestSpearmen) * NC2Layout::RecordSize::AssistantRequestValueUnsigned) + delta);
	}
	using namespace NC2Lifetime;
	SMapMoveableArrayStorage gMapMoveableArrayStorage{ };
	extern bool __cdecl AIManager_l_AI_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameAssistantManager_ls_LogicCallback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl MultiPlayerGoalManager_l_LogicCallback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl HappinessManager_l_Happiness_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl HumanMerchantArray_l_LogicCallback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl MissionManager_l_Mission_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_ValidateLandscapes_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_Continents_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_ResistanceSet_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_WalkFlagSet_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_MaxMoveableSize_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl MapFishManager_l_FishManager_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

	class CUserNetworkCommandList final : public NXBasics::CListBase
	{
	public:
		explicit CUserNetworkCommandList(bool deleteObjects) : CListBase(deleteObjects)
		{}
		~CUserNetworkCommandList() override;
		void* Delete(std::uint32_t flags);
	protected:
		void l_DeleteData(void* data) override
		{
			NXBasics::Runtime_Free(data);
		}
	};
	// Original function: FUN_00411b6d; CList<SUserNetworkCommand> destructor.
	CUserNetworkCommandList::~CUserNetworkCommandList()
	{
		if (mDeleteObjects != 0) DeleteAllElements();
	}
	// Original function: FUN_00411b8a; native scalar deleting-destructor adapter.
	void* CUserNetworkCommandList::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CUserNetworkCommandList();
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(result);
		return result;
	}

	// Original function: FUN_004121a9; CStaticDependenciesManager constructor.
	void* __fastcall StaticDependenciesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gStaticDependenciesManager = manager;
		std::memset(gStaticDependencyGoodByLandscape, 0, sizeof(gStaticDependencyGoodByLandscape));
		for (std::uint32_t good = 0; good <= 55; ++good) gStaticDependencyGoodByLandscape[static_cast<std::uint32_t>(gStaticGoodTypeData[good].landscapeType)] = static_cast<std::int32_t>(good);
		gStaticDependencyGoodByLandscape[0x5C] = 5;
		gStaticDependencyGoodByLandscape[0x5E] = 5;
		gStaticDependencyGoodByLandscape[0x64] = 2;
		gStaticDependencyGoodByLandscape[0x65] = 2;
		gStaticDependencyGoodByLandscape[0x67] = 3;
		gStaticDependencyGoodByLandscape[0x68] = 3;
		gStaticDependencyGoodByLandscape[0x6A] = 6;
		gStaticDependencyGoodByLandscape[0x6B] = 6;
		gStaticDependencyGoodByLandscape[0x6D] = 7;
		gStaticDependencyGoodByLandscape[0x6E] = 7;
		gStaticDependencyGoodByLandscape[0x79] = 13;
		gStaticDependencyGoodByLandscape[0x7A] = 13;
		gStaticDependencyGoodByLandscape[0x7C] = 14;
		return manager;
	}

	// Original function: FUN_0040b94a.
	void* __fastcall MarkerArray_Construct(void* manager, void* /*unusedEdx*/)
	{
		auto* array = ::new (manager) CArrayHandler();
		gMarkerArray = manager;
		std::memset(manager, 0, sizeof(CArrayHandler));
		gMarkerEntries = nullptr;
		gMarkerEntries = NXBasics::Runtime_Allocate(24000);
		std::memset(gMarkerEntries, 0, 24000);
		array->System_InitArray(1000, static_cast<std::byte*>(gMarkerEntries), 0x18, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		return manager;
	}

	// Original function: FUN_0040bbaa; embedded moveable state constructor.
	static void MapMoveable_ConstructMembers(void* moveable)
	{
		Write<std::uint16_t>(moveable, NC2Layout::MapMoveableOffset::MapPositionY, 0);
		Write<std::uint16_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition, 0);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::SectorPosition, 0);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::SectorPositionY, 0);
	}

	// Original function: FUN_0040bb10.
	void* __fastcall MapMoveableArray_Construct(void* manager, void* /*unusedEdx*/)
	{
		auto* array = ::new (manager) CArrayHandler();
		gMapMoveableArrayStorage.manager = array;
		std::memset(manager, 0, sizeof(CArrayHandler));
		gMapMoveableArrayStorage.entries = nullptr;
		auto* entries = static_cast<std::byte*>(NXBasics::Runtime_Allocate(176000));
		if (entries != nullptr) for (std::uint32_t index = 0; index < 2000; ++index) MapMoveable_ConstructMembers(entries + index * 0x58u);
		gMapMoveableArrayStorage.entries = entries;
		std::memset(entries, 0, 176000);
		array->System_InitArray(2000, entries, 0x58, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		return manager;
	}

	// Original function: FUN_0041714b.
	void* __fastcall MissionOnscreenInfoManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gMissionOnscreenInfoManager = manager;
		std::memset(manager, 0, 4000);
		return manager;
	}

	// Original function: FUN_004175ec.
	void* __fastcall GameAssistantManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gGameAssistantManager = manager;
		std::memset(manager, 0, 1120);
		CCallbackManager::sTheObjectPtr->RegisterCallback(1, GameAssistantManager_ls_LogicCallback, 0x0C, 1);
		return manager;
	}

	// Original function: FUN_00419032.
	void* __fastcall MultiPlayerGoalManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gMultiPlayerGoalManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::MultiPlayerGoalManager);
		CCallbackManager::sTheObjectPtr->RegisterCallback(SwitchIds::CallbackType::GameTick, MultiPlayerGoalManager_l_LogicCallback, NC2Runtime::MultiPlayerGoalCallback::RegistrationTag, NC2Runtime::MultiPlayerGoalCallback::Priority);
		return manager;
	}

	// Original function: FUN_004196cd.
	void* __fastcall HappinessManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gHappinessManager = manager;
		std::memset(manager, 0, 2720);
		for (std::uint32_t type : {1u, 0x27u, 0x28u}) CCallbackManager::sTheObjectPtr->RegisterCallback(type, HappinessManager_l_Happiness_Callback, 0x0C, 1);
		return manager;
	}

	// Original function: FUN_00419bc5.
	void* __fastcall EmbeddedSavegameManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gEmbeddedSavegameManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::EmbeddedSavegameManager);
		return manager;
	}

	// Original function: FUN_00419ddb.
	void* __fastcall TributeManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		for (std::uint32_t index = 0; index < 40; ++index) Write<std::uint8_t>(manager, index * NC2Layout::RecordSize::TributeUnsigned, 0);
		gTributeManager = manager;
		std::memset(manager, 0, 2400);
		return manager;
	}

	// Original function: FUN_00419f41.
	void* __fastcall DelayedActionManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		auto* array = ::new (manager) CArrayHandler();
		gDelayedActionManager = manager;
		std::memset(manager, 0, NC2Layout::ObjectSize::DelayedActionManager);
		void* entries = NXBasics::Runtime_Allocate(4800);
		Write<void*>(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries, entries);
		std::memset(entries, 0, 4800);
		array->System_InitArray(100, static_cast<std::byte*>(entries), 0x30, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		return manager;
	}

	// Original function: FUN_0041c3aa.
	void* __fastcall HumanMerchantArray_Construct(void* manager, void* /*unusedEdx*/)
	{
		static_assert(offsetof(SMerchantManagerStorage, entries) == NC2Layout::MerchantManagerOffset::Entries);
		static_assert(offsetof(SMerchantManagerStorage, tradeAgreementCount) == NC2Layout::MerchantManagerOffset::TradeAgreementCount);
		static_assert(offsetof(SMerchantManagerStorage, tradeAgreements) == NC2Layout::MerchantManagerOffset::TradeAgreementsUnsigned);
		static_assert(sizeof(SMerchantManagerStorage) == NC2Layout::ObjectSize::HumanMerchantArray);
		auto* array = ::new (manager) CArrayHandler();
		gHumanMerchantArray = manager;
		std::memset(manager, 0, NC2Layout::RecordSize::MerchantArrayHeader);
		void* entries = NXBasics::Runtime_Allocate(NC2Layout::RecordSize::MerchantDefaultPool);
		Write<void*>(manager, NC2Layout::MerchantManagerOffset::Entries, entries);
		std::memset(entries, 0, NC2Layout::RecordSize::MerchantDefaultPool);
		std::memset(static_cast<std::byte*>(manager) + NC2Layout::MerchantManagerOffset::TradeAgreementCount, 0, NC2Layout::RecordSize::MerchantTradeAgreementBlock);
		array->System_InitArray(NC2Runtime::MerchantLimits::DefaultCapacity, static_cast<std::byte*>(entries), NC2Layout::RecordSize::Merchant, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		CCallbackManager::sTheObjectPtr->RegisterCallback(0x1A, HumanMerchantArray_l_LogicCallback, 0xDE, 0x32);
		return manager;
	}

	// Original function: FUN_0041cdcc.
	static void MapPathfinderEntry_Construct(void* entry)
	{
		Write<std::uint16_t>(entry, NC2Layout::MapPathfinderOffset::CurrentPositionY, 0);
		Write<std::uint16_t>(entry, NC2Layout::MapPathfinderOffset::CurrentPosition, 0);
		Write<std::uint16_t>(entry, NC2Layout::MapPathfinderOffset::DestinationPositionY, 0);
		Write<std::uint16_t>(entry, NC2Layout::MapPathfinderOffset::DestinationPosition, 0);
	}

	// Original function: FUN_0041cd2f.
	void* __fastcall MapPathfinderManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		auto* array = ::new (manager) CArrayHandler();
		gMapPathfinderManager = manager;
		std::memset(manager, 0, sizeof(CArrayHandler));
		gMapPathfinderEntries = nullptr;
		void* entries = NXBasics::Runtime_Allocate(264000);
		if (entries != nullptr) for (std::uint32_t index = 0; index < 2000; ++index) MapPathfinderEntry_Construct(static_cast<std::byte*>(entries) + index * 0x84u);
		gMapPathfinderEntries = entries;
		std::memset(entries, 0, 264000);
		array->System_InitArray(2000, static_cast<std::byte*>(entries), 0x84, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);
		return manager;
	}

	// Original functions: FUN_00410d14 and FUN_00410d3c.
	void* __fastcall UserNetworkCommandManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		for (std::uint32_t offset : {NC2Layout::UserNetworkCommandManagerOffset::SystemQueueUnsigned, NC2Layout::UserNetworkCommandManagerOffset::DeferredQueueUnsigned, NC2Layout::UserNetworkCommandManagerOffset::OrdinaryQueueUnsigned})
		{
			::new (static_cast<std::byte*>(manager) + offset) CUserNetworkCommandList(false);
		}
		gUserNetworkCommandManager = manager;
		std::memset(manager, 0, NC2Layout::RecordSize::UserNetworkCommandState);
		return manager;
	}

	static void UserCommandList_Destruct(void* list)
	{
		static_cast<CUserNetworkCommandList*>(list)->~CUserNetworkCommandList();
	}

	// Original function: FUN_00410d8d. Destruction reverses the three member lists.
	static void UserCommandQueues_Destruct(void* queues)
	{
		UserCommandList_Destruct(static_cast<std::byte*>(queues) + NC2Layout::UserCommandQueuesOffset::Ordinary);
		UserCommandList_Destruct(static_cast<std::byte*>(queues) + NC2Layout::UserCommandQueuesOffset::Deferred);
		UserCommandList_Destruct(queues);
	}

	// Original function: FUN_00410da9.
	void UserNetworkCommandManager_EmptyQueue(void* manager)
	{
		for (std::uint32_t offset : {NC2Layout::UserNetworkCommandManagerOffset::SystemQueueUnsigned, NC2Layout::UserNetworkCommandManagerOffset::DeferredQueueUnsigned, NC2Layout::UserNetworkCommandManagerOffset::OrdinaryQueueUnsigned})
		{
			DrainRuntimeList(static_cast<std::byte*>(manager) + offset);
		}
		std::memset(manager, 0, NC2Layout::RecordSize::UserNetworkCommandState);
	}

	// Original function: FUN_00410d71.
	static void UserNetworkCommandManager_Destruct(void* manager)
	{
		UserNetworkCommandManager_EmptyQueue(manager);
		gUserNetworkCommandManager = nullptr;
		UserCommandQueues_Destruct(static_cast<std::byte*>(manager) + NC2Layout::UserNetworkCommandManagerOffset::SystemQueue);
	}

	// Original function: FUN_00411bc4.
	static void GameMapModifyManager_Destruct(void*)
	{
		gGameMapModifyManager = nullptr;
	}

	// Original function: FUN_0041217b.
	static void StaticHumanJobExperienceTypeDataManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticHumanJobExperienceTable00510A3C);
		gStaticHumanJobExperienceTable00510A3C = nullptr;
		NXBasics::Runtime_Free(gStaticHumanJobExperienceData);
		gStaticHumanJobExperienceData = nullptr;
		gStaticHumanJobExperienceTypeDataManager = nullptr;
	}

	// Original function: FUN_00412240.
	static void StaticDependenciesManager_Destruct(void*)
	{
		gStaticDependenciesManager = nullptr;
	}

	// Original function: FUN_00412761.
	static void StaticAnimalTypeDataManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticAnimalTypeData);
		gStaticAnimalTypeData = nullptr;
		gStaticAnimalTypeDataManager = nullptr;
	}

	// Original function: FUN_00412a42.
	static void StaticArmorTypeDataManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticArmorTypeData);
		gStaticArmorTypeData = nullptr;
		gStaticArmorTypeDataManager = nullptr;
	}

	// Original function: FUN_004134b6.
	static void StaticJobTypeDataManager_Destruct(void*)
	{
		NXBasics::Runtime_Free(gStaticJobTypeData);
		gStaticJobTypeData = nullptr;
		gStaticJobTypeDataManager = nullptr;
	}

	// Original function: FUN_00412fe0.
	static void StaticWeaponTypeDataManager_Destruct(void*)
	{
		for (void*& table : gStaticWeaponTypeArrays)
		{
			if (table != nullptr)
			{
				NXBasics::Runtime_Free(table);
				table = nullptr;
			}
		}
		gStaticWeaponTypeDataManager = nullptr;
	}

	// Original function: FUN_00417168.
	static void MissionOnscreenInfoManager_Destruct(void*)
	{
		gMissionOnscreenInfoManager = nullptr;
	}
	// Original function: FUN_0041761f.
	static void GameAssistantManager_Destruct(void*)
	{
		Unregister(GameAssistantManager_ls_LogicCallback, 0x0C, { 1 });
		gGameAssistantManager = nullptr;
	}
	// Original function: FUN_00419065.
	static void MultiPlayerGoalManager_Destruct(void*)
	{
		Unregister(MultiPlayerGoalManager_l_LogicCallback, NC2Runtime::MultiPlayerGoalCallback::RegistrationTag, { SwitchIds::CallbackType::GameTick });
		gMultiPlayerGoalManager = nullptr;
	}
	// Original function: FUN_00419727.
	static void HappinessManager_Destruct(void*)
	{
		Unregister(HappinessManager_l_Happiness_Callback, 0x0C, { 0x28, 0x27, 1 });
		gHappinessManager = nullptr;
	}
	// Original function: FUN_00419bdf.
	static void EmbeddedSavegameManager_Destruct(void* manager)
	{
		if (Read<std::uint32_t>(manager, NC2Layout::EmbeddedSavegameOffset::Size) != 0)
		{
			void* data = Read<void*>(manager, NC2Layout::EmbeddedSavegameOffset::Data);
			Write<std::uint32_t>(manager, NC2Layout::EmbeddedSavegameOffset::Size, 0);
			NXBasics::Runtime_Free(data);
			Write<void*>(manager, NC2Layout::EmbeddedSavegameOffset::Data, nullptr);
		}
		gEmbeddedSavegameManager = nullptr;
	}
	// Original function: FUN_00419e06.
	static void TributeManager_Destruct(void*)
	{
		gTributeManager = nullptr;
	}


	// Original function: FUN_0041a059.
	static void DelayedActionManager_FreeAll(void* manager)
	{
		FreeAllocatedRecords(manager, 0x30);
	}
	// Original function: FUN_00419f91.
	static void DelayedActionManager_Destruct(void* manager)
	{
		DelayedActionManager_FreeAll(manager);
		FreeAndClearField(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries);
		gDelayedActionManager = nullptr;
		Write<std::uint8_t>(manager, NC2Layout::ArrayHandlerOffset::Initialized, 0); // CArrayHandler::~CArrayHandler, 00430CA4.
	}
	// Original function: FUN_0041aaf1.
	static void PlayerStatisticManager_Destruct(void* manager)
	{
		for (std::uint32_t index = 0; index < 20; ++index)
		{
			if (Read<void*>(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayer + index * NC2Layout::RecordSize::StatisticPlayerPointer) != nullptr) FreeField(manager, NC2Layout::PlayerStatisticManagerOffset::HistoryByPlayer + index * NC2Layout::RecordSize::StatisticPlayerPointer);
			if (Read<void*>(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayer + index * NC2Layout::RecordSize::StatisticPlayerPointer) != nullptr) FreeField(manager, NC2Layout::PlayerStatisticManagerOffset::CurrentByPlayer + index * NC2Layout::RecordSize::StatisticPlayerPointer);
		}
		gPlayerStatisticManager = nullptr;
	}
	// Original function: FUN_0041ae53; CGuideManager constructs and owns CGuideArray.
	void* __fastcall GuideManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gGuideManager = manager;
		Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::NextLinkSystemId, 0);
		gGuideManagerStaticVars.scanGuideId = -1;
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(CGuideArray), 1);
		if (allocation != nullptr) ::new (allocation) CGuideArray();
		for (std::uint32_t type : {1u, 0x33u, 0x31u, 0x32u}) CCallbackManager::sTheObjectPtr->RegisterCallback(type, GuideManager_l_Guides_Callback, 0x64, 0);
		Write(manager, NC2Layout::GuideManagerOffset::VisitedGuides, NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mCapacity), 1));
		return manager;
	}

	// Original function: FUN_0041aedf.
	static void GuideManager_Destruct(void* manager)
	{
		FreeField(manager, NC2Layout::GuideManagerOffset::VisitedGuides);
		Unregister(GuideManager_l_Guides_Callback, 0x64, { 1, 0x33, 0x31, 0x32 });
		DestroyOwned(CGuideArray::sTheObjectPtr, [] (CGuideArray* array)
		{
			array->~CGuideArray();
		});
		gGuideManager = nullptr;
	}

	// Original function: FUN_0041af52; NC2Logic::CGuideManager::IO_Savegame_DoSave.
	void __fastcall GuideManager_IO_Savegame_DoSave(void* manager, void* /*unusedEdx*/, CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameChunk::Guides, 1);
		CGuideArray::sTheObjectPtr->IO_Savegame_DoSave(io);
		io->IO_Memory_WriteBinary(manager, 4);
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_0041af8a; CGuideManager loading constructor.
	void* __fastcall GuideManager_ConstructFromSavegame(void* manager, void* /*unusedEdx*/, CIoHelper* io)
	{
		gGuideManager = manager;
		if (io->mReadHeader.version != 1)
		{
			io->IO_File_Chunk_Skip();
			return manager;
		}
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(CGuideArray), 1);
		if (allocation != nullptr) ::new (allocation) CGuideArray(io);
		io->IO_File_ReadBinary(manager, 4);
		gGuideManagerStaticVars.scanGuideId = -1;
		Write(manager, NC2Layout::GuideManagerOffset::VisitedGuides, NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mCapacity), 1));
		for (std::uint32_t type : {1u, 0x33u, 0x31u, 0x32u}) CCallbackManager::sTheObjectPtr->RegisterCallback(type, GuideManager_l_Guides_Callback, 0x64, 0);
		return manager;
	}
	// Original function: FUN_0040b9af.
	static void MarkerArray_Destruct(void* array)
	{
		NXBasics::Runtime_Free(gMarkerEntries);
		gMarkerEntries = nullptr;
		gMarkerArray = nullptr;
		Write<std::uint8_t>(array, NC2Layout::ArrayHandlerOffset::Initialized, 0);
	}
	// Original function: FUN_0041c428.
	static void HumanMerchantArray_Destruct(void* array)
	{
		Unregister(HumanMerchantArray_l_LogicCallback, 0xDE, { 0x1A });
		if (Read<void*>(array, NC2Layout::MerchantManagerOffset::Entries) != nullptr) FreeAndClearField(array, NC2Layout::MerchantManagerOffset::Entries);
		gHumanMerchantArray = nullptr;
		Write<std::uint8_t>(array, NC2Layout::ArrayHandlerOffset::Initialized, 0);
	}
	// Original function: FUN_0041cde1.
	static void MapPathfinderManager_Destruct(void* array)
	{
		NXBasics::Runtime_Free(gMapPathfinderEntries);
		gMapPathfinderEntries = nullptr;
		gMapPathfinderManager = nullptr;
		Write<std::uint8_t>(array, NC2Layout::ArrayHandlerOffset::Initialized, 0);
	}
	// Original function: FUN_0040bbbd.
	static void MapMoveableArray_Destruct(CArrayHandler* array)
	{
		NXBasics::Runtime_Free(gMapMoveableArrayStorage.entries);
		gMapMoveableArrayStorage.entries = nullptr;
		gMapMoveableArrayStorage.manager = nullptr;
		array->mInitialized = 0;
	}
	// Original function: FUN_00429546; CAIGuideManager releases its owned calculation grid.
	static void AIGuideManager_Destruct(void* object)
	{
		if (Read<void*>(object, NC2Layout::AIGuideManagerOffset::GridCells) != nullptr) FreeAndClearField(object, NC2Layout::AIGuideManagerOffset::GridCells);
	}
	// Original function: FUN_00407f55.
	static void AIManager_Destruct(void* manager)
	{
		Unregister(AIManager_l_AI_Callback, 0x0B, { 0x2A, 1 });
		for (std::uint32_t index = 0; index < NC2Runtime::AIManagerLimits::Players; ++index)
		{
			if (Read<void*>(manager, index * NC2Layout::RecordSize::AIPlayerHandlerPointer) != nullptr)
			{
				FreeAndClearField(manager, static_cast<std::size_t>(index) * NC2Layout::RecordSize::AIPlayerHandlerPointer);
				void* player = Read<void*>(manager, NC2Layout::AIManagerOffset::HighLevelHandlers + index * NC2Layout::RecordSize::AIPlayerHandlerPointer);
				if (player != nullptr)
				{
					AIGuideManager_Destruct(static_cast<std::byte*>(player) + NC2Layout::AIHighLevelOffset::GuideManager);
					NXBasics::Runtime_Free(player);
					Write<void*>(manager, NC2Layout::AIManagerOffset::HighLevelHandlers + index * NC2Layout::RecordSize::AIPlayerHandlerPointer, nullptr);
				}
			}
		}
		gAIManager = nullptr;
	}
	// Original function: FUN_0041cff0.
	static void MissionManager_Destruct(void*)
	{
		Unregister(MissionManager_l_Mission_Callback, 0x0A, { 1 });
		gMissionManager = nullptr;
	}
	// Original function: FUN_00409a9e.
	static void GameControl_Destruct(void*)
	{
		Unregister(GameControl_ls_LogicCallback, 0, { 0x28 });
		gGameControl = nullptr;
	}


	// Bridge for existing raw-record callers of SGameControlStartupData::Reset.
	void GameControlStartupData_Clear(void* data)
	{
		static_cast<SGameControlStartupData*>(data)->Reset();
	}

	// Original function: FUN_004071c3.
	void Game_ShutDown()
	{
		if (gGameInitialized == 0) return;
		gGameInitialized = 0;
		DestroyOwned(gMissionOnscreenInfoManager, MissionOnscreenInfoManager_Destruct);
		DestroyOwned(gGameAssistantManager, GameAssistantManager_Destruct);
		DestroyOwned(gMultiPlayerGoalManager, MultiPlayerGoalManager_Destruct);
		DestroyOwned(gHappinessManager, HappinessManager_Destruct);
		DestroyOwned(gEmbeddedSavegameManager, EmbeddedSavegameManager_Destruct);
		DestroyOwned(gTributeManager, TributeManager_Destruct);
		DestroyOwned(gDelayedActionManager, DelayedActionManager_Destruct);
		DestroyOwned(gPlayerStatisticManager, PlayerStatisticManager_Destruct);
		DestroyOwned(gGuideManager, GuideManager_Destruct);
		DestroyOwned(CHouseArray::sTheObjectPtr, [] (CHouseArray* array)
		{
			array->~CHouseArray();
		});
		DestroyOwned(gMarkerArray, MarkerArray_Destruct);
		DestroyOwned(CVehicleArray::sTheObjectPtr, [] (CVehicleArray* array)
		{
			array->~CVehicleArray();
		});
		DestroyOwned(CAnimalArray::sTheObjectPtr, [] (CAnimalArray* array)
		{
			array->~CAnimalArray();
		});
		DestroyOwned(gHumanMerchantArray, HumanMerchantArray_Destruct);
		DestroyOwned(CHumanArray::sTheObjectPtr, [] (CHumanArray* array)
		{
			array->~CHumanArray();
		});
		DestroyOwned(gMapPathfinderManager, MapPathfinderManager_Destruct);
		DestroyOwned(gMapMoveableArrayStorage.manager, MapMoveableArray_Destruct);
		DestroyOwned(gAIManager, AIManager_Destruct);
		DestroyOwned(gMissionManager, MissionManager_Destruct);
		DestroyOwned(gGameMap, GameMap_Destruct);
		DestroyOwned(gGameControl, GameControl_Destruct);
		if (NC2Logic::gGameStaticVars.retainedMapIni != nullptr)
		{
			DestroyOwned(NC2Logic::gGameStaticVars.retainedMapIni, [] (NXBasics::CIniFileReader* reader)
			{
				reader->~CIniFileReader();
			});
			NC2Logic::gGameStaticVars.retainedMapIni = nullptr;
		}
		if (NC2Logic::gGameStaticVars.mapFileLibrary != nullptr)
		{
			NXBasics::CFile::FileSystem_FileLibrary_Remove(NC2Logic::gGameStaticVars.mapFileLibrary);
			DestroyOwned(NC2Logic::gGameStaticVars.mapFileLibrary, NXBasics::Archive_Destroy);
			NC2Logic::gGameStaticVars.mapFileLibrary = nullptr;
		}
		gGameControlStartupData.Reset();
		UserNetworkCommandManager_EmptyQueue(gUserNetworkCommandManager);
		NRemaster::CivilizationBuildings::End();
	}

	// Original function: FUN_00406d85.
	void Game_Destruct(void*)
	{
		if (gGameInitialized != 0) Game_ShutDown();
		DestroyOwned(gUserNetworkCommandManager, UserNetworkCommandManager_Destruct);
		DestroyOwned(gGameMapModifyManager, GameMapModifyManager_Destruct);
		DestroyOwned(CCallbackManager::sTheObjectPtr, [] (CCallbackManager* manager)
		{
			manager->~CCallbackManager();
		});
		DestroyOwned(gStaticHumanJobExperienceTypeDataManager, StaticHumanJobExperienceTypeDataManager_Destruct);
		DestroyOwned(gStaticDependenciesManager, StaticDependenciesManager_Destruct);
		DestroyOwned(gStaticAnimalTypeDataManager, StaticAnimalTypeDataManager_Destruct);
		DestroyOwned(gStaticWeaponTypeDataManager, StaticWeaponTypeDataManager_Destruct);
		DestroyOwned(gStaticArmorTypeDataManager, StaticArmorTypeDataManager_Destruct);
		DestroyOwned(gStaticJobTypeDataManager, StaticJobTypeDataManager_Destruct);
		DestroyOwned(gStaticTribeTypeDataManager, [] (CStaticTribeTypeDataManager* manager)
		{
			manager->~CStaticTribeTypeDataManager();
		});
		DestroyOwned(gStaticAtomicAnimationDataManager, [] (CStaticAtomicAnimationDataManager* manager)
		{
			manager->~CStaticAtomicAnimationDataManager();
		});
		DestroyOwned(gStaticVehicleTypeDataManager, [] (CStaticVehicleTypeDataManager* manager)
		{
			manager->~CStaticVehicleTypeDataManager();
		});
		DestroyOwned(gStaticHouseTypeDataManager, [] (CStaticHouseTypeDataManager* manager)
		{
			manager->~CStaticHouseTypeDataManager();
		});
		DestroyOwned(gStaticGoodTypeDataManager, [] (CStaticGoodTypeDataManager* manager)
		{
			manager->~CStaticGoodTypeDataManager();
		});
		DestroyOwned(gStaticTrianglePatternTypeDataManager, [] (CStaticTrianglePatternTypeDataManager* manager)
		{
			manager->~CStaticTrianglePatternTypeDataManager();
		});
		DestroyOwned(gStaticLandscapeTypeDataManager, [] (CStaticLandscapeTypeDataManager* manager)
		{
			manager->~CStaticLandscapeTypeDataManager();
		});
		DestroyOwned(gSavegamesManager, [] (CSavegamesManager* manager)
		{
			manager->~CSavegamesManager();
		});
		DestroyOwned(gCleanMapsManager, [] (CCleanMapsManager* manager)
		{
			manager->~CCleanMapsManager();
		});
		DestroyOwned(gCampaignStaticDataManager, [] (CCampaignStaticDataManager* manager)
		{
			manager->~CCampaignStaticDataManager();
		});
		DestroyOwned(gStaticStringsManager, [] (CStaticStringsManager* manager)
		{
			manager->~CStaticStringsManager();
		});
		gGame = nullptr;
	}
}
