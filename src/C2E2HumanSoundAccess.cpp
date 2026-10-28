#include "C2E2HumanSoundAccess.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataSoundFX.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDisplayManager.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2EngineObjectData.h"
#include "C2EngineParticleSimulation.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "DSManager.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2E2
{
	using NC2Lifetime::Read;

	// Original function: FUN_0047ec5b.
	bool __cdecl E2Manager_sl_Callback_Misc(std::uint32_t type, std::uintptr_t, std::uintptr_t callbackValueParameter1, std::uintptr_t callbackValueParameter2, std::uintptr_t callbackValueParameter3)
	{
		std::uint32_t parameter1 = static_cast<std::uint32_t>(callbackValueParameter1);
		std::uint32_t parameter2 = static_cast<std::uint32_t>(callbackValueParameter2);
		std::uint32_t parameter3 = static_cast<std::uint32_t>(callbackValueParameter3);
		switch (type)
		{
			case SwitchIds::CallbackType::HouseWalkAreaChanged:
				{
					const void* house = NC2Logic::CHouseArray::mStaticVars + parameter1 * NC2Logic::CHouseArray::ElementSize;
					if (NC2Logic::House_GetHouseMainType(house) == 6) return true;
					const auto* area = static_cast<const NC2Logic::CMapArea*>(NC2Logic::House_Misc_WalkBlockArea_GetPtr(house));
					const auto root = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
					for (std::uint32_t rowIndex = 0; rowIndex < area->rowCount; ++rowIndex)
					{
						const auto* row = area->rows + rowIndex;
						std::uint32_t point;
						NC2Logic::MapMigPoint_ConstructOffset(&point, &root, row->x, row->y, false);
						for (std::uint32_t column = 0; column < row->width; ++column)
						{
							if (NC2Logic::MapMigPoint_IsInsideMap(&point))
							{
								EngineMap_OverlayPattern_HouseAreaChanged(&point, false);
								auto neighbor = point;
								NC2Logic::MapMigPoint_MoveCursorUnwrapped(neighbor, 4);
								for (std::uint32_t direction = 0; direction < 6; ++direction)
								{
									if (NC2Logic::MapMigPoint_IsInsideMap(&neighbor)) EngineMap_OverlayPattern_HouseAreaChanged(&neighbor, false);
									NC2Logic::MapMigPoint_MoveCursorUnwrapped(neighbor, direction);
								}
							}
							NC2Lifetime::Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
						}
					}
					std::int16_t rectangle[4];
					NC2Logic::MapMigRectangle_Construct(rectangle, static_cast<std::int16_t>(Read<std::uint16_t>(&root, NC2Layout::MapCoordinatePairOffset::X) - 20u), static_cast<std::int16_t>(Read<std::uint16_t>(&root, NC2Layout::MapCoordinatePairOffset::Y) - 20u), 40, 40);
					C2DEngineDisplayManager::sTheObjectPtr->InformThatStaticMapChanged(rectangle, NXBasics::IntegerRange::UInt32Maximum);
					return true;
				}
			case SwitchIds::CallbackType::ResetHouseDynamicGraphics:
				std::memset(static_cast<std::byte*>(gDynamicDataHouses.entries) + parameter1 * NC2Layout::RecordSize::DynamicHouseUnsigned, 0, NC2Layout::RecordSize::DynamicHouseSavedPrefix);
				return true;
			case SwitchIds::CallbackType::HumanPaletteChanged:
				{
					std::int32_t patch;
					if (parameter2 == 2)
					{
						const void* human = NC2Logic::GetHumanPtr(static_cast<std::int32_t>(parameter1));
						if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit10) != 0) return true;
						patch = StaticHumanMisc_ColorIds_GetPlayerRandomPatchId(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), NC2Logic::Human_IsSexFemale(human));
					}
					else if (parameter2 == 1)
					{
						// Windows dereferences the null result for an out-of-range good ID.
						const void* good = static_cast<std::int32_t>(parameter3) < 66 ? static_cast<const std::byte*>(gStaticGoodData) + parameter3 * 4u : nullptr;
						patch = Read<std::int32_t>(good, NC2Layout::GoodGraphicsOffset::PalettePatchId);
					}
					else return true;
					if (patch != -1)
					{
						auto* human = static_cast<std::byte*>(gDynamicDataHumans.entries) + parameter1 * NC2Layout::RecordSize::DynamicHumanUnsigned;
						StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), reinterpret_cast<NXBasics::CPalette*>(human + NC2Layout::DynamicHumanOffset::BodyPalette), reinterpret_cast<NXBasics::CPalette*>(human + NC2Layout::DynamicHumanOffset::HeadPalette), false);
					}
					return true;
				}
			case SwitchIds::CallbackType::HumanMiscPaletteChanged:
				{
					char name[128];
					NXBasics::Runtime_PrintString(name, "human_misc_%3.3d", parameter2);
					const auto patch = StaticHumanMisc_RandomPalette_GetPatchId(name);
					if (patch != -1)
					{
						auto* human = static_cast<std::byte*>(gDynamicDataHumans.entries) + parameter1 * NC2Layout::RecordSize::DynamicHumanUnsigned;
						StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), reinterpret_cast<NXBasics::CPalette*>(human + NC2Layout::DynamicHumanOffset::BodyPalette), reinterpret_cast<NXBasics::CPalette*>(human + NC2Layout::DynamicHumanOffset::HeadPalette), false);
					}
					return true;
				}
			case SwitchIds::CallbackType::HumanJobChanged: E2DynamicDataHumanManager_HumanJobChanged(parameter1, parameter2, parameter3);
				return true;
			case SwitchIds::CallbackType::HumanArmorChanged: E2DynamicDataHumanManager_HumanArmorChanged(parameter1, parameter2);
				return true;
			case SwitchIds::CallbackType::PlayPlayerJingle:
				{
					const void* display = C2DEngineDisplayManager::sTheObjectPtr->GetActiveDisplay();
					if (display != nullptr && Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId) == parameter1) NDSManager::CManager::Jingle_Play(parameter2);
					return true;
				}
			case SwitchIds::CallbackType::UpdateHouseOverlayParticles:
				{
					const void* house = NC2Logic::CHouseArray::mStaticVars + parameter1 * NC2Logic::CHouseArray::ElementSize;
					NC2Logic::House_GetHouseTypeDataPtr(house);
					const void* data = static_cast<const std::byte*>(gStaticHouseEntries) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
					bool found = false;
					if (parameter3 != 0)
					{
						for (std::uint32_t index = 0; index < Read<std::uint32_t>(data, NC2Layout::EngineHouseTypeOffset::ActionOverlayCount); ++index)
						{
							const void* entry = static_cast<const std::byte*>(Read<void*>(data, NC2Layout::EngineHouseTypeOffset::ActionOverlays)) + index * 0xE4u;
							if (Read<std::uint32_t>(entry, NC2Layout::EngineHouseActionOverlayOffset::Action) != 3 || Read<std::uint32_t>(entry, NC2Layout::EngineHouseActionOverlayOffset::DevelopmentStage) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage)) continue;
							auto position = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
							NC2Logic::MapMigPoint_MoveCursorUnwrapped(position, 2);
							const std::int32_t pixel[2] = { NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(entry, NC2Layout::EngineHouseActionOverlayOffset::OffsetX) + 17u), NXBasics::Runtime_TimeSigned32(0u - Read<std::uint32_t>(entry, NC2Layout::EngineHouseActionOverlayOffset::OffsetY) + 19u) };
							E2ParticelManager_AddParticelGenerator(gE2ParticelManager, 0, &position, pixel, 5, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
							found = true;
						}
					}
					if (found) E2ParticelManager_EnableGenerator(gE2ParticelManager, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
					else E2ParticelManager_DestroyGenerator(gE2ParticelManager, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
					return true;
				}
			case SwitchIds::CallbackType::CreateMunitionParticle:
				{
					const void* data = reinterpret_cast<const void*>(callbackValueParameter1);
					E2ParticelManager_CreateMunitionParticel(gE2ParticelManager, data, Read<std::uint32_t>(data, NC2Layout::MunitionCallbackOffset::SourceHeight), static_cast<const std::byte*>(data) + 4, static_cast<const std::byte*>(data) + 0xC, static_cast<const std::byte*>(data) + 0x14, Read<std::uint32_t>(data, NC2Layout::MunitionCallbackOffset::MunitionType), Read<std::uint32_t>(data, NC2Layout::MunitionCallbackOffset::FlightTicks));
					return true;
				}
			case SwitchIds::CallbackType::CreateFallingSmokeParticle: E2ParticelManager_CreateFallingSmokeParticel(gE2ParticelManager, &parameter1, parameter2, parameter3);
				return true;
			case SwitchIds::CallbackType::StartParticleEffect: E2ParticelManager_StartParticelEffect(gE2ParticelManager, parameter1, &parameter2, parameter3);
				return true;
			case SwitchIds::CallbackType::StartDirectionalParticleEffect: E2ParticelManager_StartParticelEffectIntoDirection(gE2ParticelManager, parameter1, &parameter2, &parameter3, 0);
				return true;
			case SwitchIds::CallbackType::StartDirectionalParticleEffectFromRecord:
				{
					const void* data = reinterpret_cast<const void*>(callbackValueParameter1);
					E2ParticelManager_StartParticelEffectIntoDirection(gE2ParticelManager, 1, static_cast<const std::byte*>(data) + 4, static_cast<const std::byte*>(data) + 8, Read<std::uint32_t>(data, NC2Layout::DirectionalParticleCallbackOffset::PlayerId));
					E2HumanSoundManager_PlayHumanScreamSound(gE2HumanSoundManager, Read<void*>(data, NC2Layout::DirectionalParticleCallbackOffset::Human));
					return true;
				}
		}
		const auto soundType = parameter1, point = parameter2, skipRunning = parameter3;
		if (type != 0x17 || NDSManager::CManager::sTheObjectPtr == nullptr) return true;
		const auto* display = C2DEngineDisplayManager::sTheObjectPtr->GetActiveDisplay();
		if (display == nullptr) return true;
		const auto x = Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y);
		const auto left = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleX), top = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleY), width = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleWidth), height = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleHeight);
		if (width <= 0 || height <= 0) return true;
		if (x < left || x > static_cast<std::int16_t>(left + width - 1) || y < top || y > static_cast<std::int16_t>(top + height - 1)) return true;
		const auto player = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		if ((Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 4u) != 0 && player < 16 && (Read<std::uint16_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0) return true;
		const auto pan = (x - width / 2 - left) * 128 / width, vertical = (y - height / 2 - top) * 128 / height;
		const auto distance = ((pan < 0 ? -pan : pan) + (vertical < 0 ? -vertical : vertical)) / 2;
		if (distance > 64) return true;
		auto* sounds = CE2StaticDataSoundFXManager::sTheObjectPtr;
		if (sounds == nullptr || sounds->staticSounds == nullptr || soundType >= 120u) return true;
		const auto soundId = sounds->staticSoundIdsByType[soundType];
		if (soundId >= sounds->staticSoundCount) return true;
		const auto* sound = static_cast<const std::byte*>(sounds->staticSounds) + soundId * NC2Layout::RecordSize::StaticSoundFxUnsigned;
		const auto count = Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WaveCount);
		if (count == 0) return true;
		const auto variant = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % count;
		const auto wave = Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WavesUnsigned + variant * NC2Layout::RecordSize::StaticSoundWaveUnsigned);
		auto* manager = NDSManager::CManager::sTheObjectPtr;
		if (skipRunning != 0 && manager->PreloadWave_IsRunning(wave)) return true;
		manager->PreloadWave_Play(wave, ((64u - static_cast<std::uint32_t>(distance)) * Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::FirstWaveVolumeUnsigned + variant * NC2Layout::RecordSize::StaticSoundWaveUnsigned)) >> 6, 1, static_cast<std::uint32_t>(pan) + 64u, false, 0);
		return true;
	}

	// Original function: FUN_004856f9. This helper is inlined in the Mac reference.
	static std::uint32_t HumanSound_GetSexAgeIndex(const void* human)
	{
		if (!NC2Logic::Human_IsAgeAdult(human)) return 0;
		return NC2Logic::Human_IsSexMale(human) ? 2u : 1u;
	}

	// Original function: FUN_004856b4.
	void E2HumanSoundManager_PlayHumanScreamSound(const void* manager, const void* human)
	{
		const void* display = C2DEngineDisplayManager::sTheObjectPtr->GetActiveDisplay();
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const auto category = HumanSound_GetSexAgeIndex(human);
		const auto soundId = Read<std::uint32_t>(manager, NC2Layout::HumanSoundManagerViewOffset::FirstScreamSoundIdUnsigned + (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + category) * NC2Layout::RecordSize::HumanSoundRecordUnsigned);
		Tool_Sound_PlaySoundOrientedOnDisplay(NC2Logic::Human_GetMapPositionRef(human), soundId, display, true, true);
	}

	// Original function: FUN_0048571d; eight tribes, three sex/age counters each.
	void E2HumanSoundManager_ClearSeenHumanCounter(void* manager)
	{
		for (std::uint32_t tribe = 0; tribe < NC2Runtime::HumanSoundLimits::Tribes; ++tribe)
			for (std::uint32_t category = 0; category < NC2Runtime::HumanSoundLimits::SexAgeCategories; ++category) NC2Lifetime::Write<std::uint32_t>(manager, NC2Layout::HumanSoundManagerViewOffset::FirstSeenCountUnsigned + (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + category) * NC2Layout::RecordSize::HumanSoundRecordUnsigned, 0);
	}

	// Original function: FUN_00485735.
	void E2HumanSoundManager_IncreaseSeenCounter(void* manager, const void* human)
	{
		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const auto offset = NC2Layout::HumanSoundManagerViewOffset::FirstSeenCountUnsigned + (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + HumanSound_GetSexAgeIndex(human)) * NC2Layout::RecordSize::HumanSoundRecordUnsigned;
		NC2Lifetime::Write(manager, offset, Read<std::uint32_t>(manager, offset) + 1u);
	}

	// Original function: FUN_0048575f; one chance value and at most one sound per game tick.
	void E2HumanSoundManager_PlayGenericSounds(void* manager, const void* display)
	{
		if (NDSManager::CManager::sTheObjectPtr == nullptr || Read<std::uint8_t>(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Initialized) == 0) return;
		const auto tick = NC2Logic::gGameTime.tick;
		if (Read<std::uint32_t>(manager, NC2Layout::HumanSoundManagerOffset::LastGenericSoundTick) == tick) return;
		NC2Lifetime::Write(manager, NC2Layout::HumanSoundManagerOffset::LastGenericSoundTick, tick);
		std::int16_t rectangle[4]{ };
		DisplayDescription_GetMapRectangle(display, Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth), Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight), rectangle);
		const std::uint32_t center = static_cast<std::uint16_t>(rectangle[0] + (rectangle[2] >> 1)) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(rectangle[1] + (rectangle[3] >> 1))) << 16);
		const auto player = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
		const auto chance = static_cast<std::uint32_t>(NXBasics::Runtime_Rand() % NC2Runtime::HumanSoundLimits::GenericChanceRange);
		for (std::uint32_t tribe = 0; tribe < NC2Runtime::HumanSoundLimits::Tribes; ++tribe)
		{
			for (std::uint32_t category = 0; category < NC2Runtime::HumanSoundLimits::SexAgeCategories; ++category)
			{
				const auto offset = (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + category) * NC2Layout::RecordSize::HumanSoundRecordUnsigned;
				if (chance >= Read<std::uint32_t>(manager, offset + NC2Layout::HumanSoundManagerViewOffset::FirstSeenCountUnsigned) || Read<std::uint32_t>(manager, offset + NC2Layout::HumanSoundManagerViewOffset::FirstGenericSoundIdUnsigned) == NXBasics::IntegerRange::UInt32Maximum) continue;
				NC2Logic::CHumanIterator iterator;
				const void* selected = nullptr;
				for (auto id = iterator.GetFirstSectorHumanId(player, center, rectangle[2], false); id != -1; id = iterator.GetNextSectorHumanId())
				{
					const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize;
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == tribe && HumanSound_GetSexAgeIndex(human) == category)
					{
						selected = human;
						break;
					}
				}
				if (selected == nullptr && category == 2)
				{
					for (auto id = iterator.GetFirstSectorHumanId(player, center, rectangle[2], true); id != -1; id = iterator.GetNextSectorHumanId())
					{
						const auto* human = NC2Logic::CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * NC2Logic::CHumanArray::ElementSize;
						if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == tribe && HumanSound_GetSexAgeIndex(human) == 2)
						{
							selected = human;
							break;
						}
					}
				}
				if (selected == nullptr) continue;
				Tool_Sound_PlaySoundOrientedOnDisplay(NC2Logic::Human_GetMapPositionRef(selected), Read<std::uint32_t>(manager, offset + NC2Layout::HumanSoundManagerViewOffset::FirstGenericSoundIdUnsigned), display, true, true);
				return;
			}
		}
	}

	// Original function: FUN_00485a73. The Mac reference inlines this response-list access.
	static std::uint32_t HumanSound_GetRespondingSoundId(const void* record, std::uint32_t responseType, std::uint32_t selector)
	{
		const std::uint32_t offset = responseType * NC2Layout::RecordSize::HumanSoundResponseGroupUnsigned;
		const std::uint32_t count = Read<std::uint32_t>(record, offset + NC2Layout::HumanSoundScreamViewOffset::ResponseCountUnsigned);
		return count == 0 ? NXBasics::IntegerRange::UInt32Maximum : Read<std::uint32_t>(record, offset + NC2Layout::HumanSoundScreamViewOffset::ResponseSoundIdsUnsigned + (selector % count) * NC2Layout::RecordSize::StaticSoundIdUnsigned);
	}

	// Original function: FUN_00485931; CE2HumanSoundManager::PlayRespondingSound.
	void E2HumanSoundManager_PlayRespondingSound(const void* manager, std::uint32_t responseType, const void* human, const void* display)
	{
		if (NDSManager::CManager::sTheObjectPtr == nullptr || Read<std::uint8_t>(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Initialized) == 0) return;
		std::uint32_t selector = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId);
		if (NC2Logic::Human_IsJobHero(human)) selector = 0;
		const std::uint32_t tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const std::uint32_t job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
		if ((tribe == 2u && job == 0x2Au) || (tribe == 3u && (job == 0x2Au || job == 0x20u))) return;

		const auto* record = static_cast<const std::byte*>(manager) + NC2Layout::HumanSoundManagerViewOffset::FirstScreamSoundIdUnsigned + (tribe * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + HumanSound_GetSexAgeIndex(human)) * NC2Layout::RecordSize::HumanSoundRecordUnsigned;
		const std::uint32_t soundId = HumanSound_GetRespondingSoundId(record, responseType, selector);
		if (CE2StaticDataSoundFXManager::sTheObjectPtr == nullptr || CE2StaticDataSoundFXManager::sTheObjectPtr->staticSounds == nullptr || soundId >= CE2StaticDataSoundFXManager::sTheObjectPtr->staticSoundCount) return;
		const auto* sound = static_cast<const std::byte*>(CE2StaticDataSoundFXManager::sTheObjectPtr->staticSounds) + soundId * NC2Layout::RecordSize::StaticSoundFxUnsigned;
		if (Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WaveCount) == 0) return;
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WaveCount); ++index)
		{
			if (NDSManager::CManager::sTheObjectPtr->PreloadWave_IsRunning(Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WavesUnsigned + index * NC2Layout::RecordSize::StaticSoundWaveUnsigned))) return;
		}
		const auto* currentRecord = static_cast<const std::byte*>(manager) + NC2Layout::HumanSoundManagerViewOffset::FirstScreamSoundIdUnsigned + (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * NC2Runtime::HumanSoundLimits::SexAgeCategoriesUnsigned + HumanSound_GetSexAgeIndex(human)) * NC2Layout::RecordSize::HumanSoundRecordUnsigned;
		const std::uint32_t currentSoundId = HumanSound_GetRespondingSoundId(currentRecord, responseType, selector);
		Tool_Sound_PlaySoundOrientedOnDisplay(NC2Logic::Human_GetMapPositionRef(human), currentSoundId, display, true, false);
	}

	// Original function: FUN_00485a4e; the human-id overload uses the active display.
	void E2HumanSoundManager_PlayRespondingSound(const void* manager, std::uint32_t responseType, std::int32_t humanId)
	{
		const void* display = C2DEngineDisplayManager::sTheObjectPtr->GetActiveDisplay();
		const void* human = static_cast<const std::byte*>(NC2Logic::CHumanArray::mStaticVars) + static_cast<std::uint32_t>(humanId) * NC2Logic::CHumanArray::ElementSize;
		E2HumanSoundManager_PlayRespondingSound(manager, responseType, human, display);
	}

	// Original function: FUN_00490eea. Visibility/attenuation are optional; pan is always calculated.
	void Tool_Sound_PlaySoundOrientedOnDisplay(const void* point, std::uint32_t soundId, const void* display, bool skipRunning, bool attenuate)
	{
		if (point == nullptr || display == nullptr || CE2StaticDataSoundFXManager::sTheObjectPtr == nullptr || CE2StaticDataSoundFXManager::sTheObjectPtr->staticSounds == nullptr || soundId >= CE2StaticDataSoundFXManager::sTheObjectPtr->staticSoundCount) return;
		if (NDSManager::CManager::sTheObjectPtr == nullptr || Read<std::uint8_t>(NDSManager::CManager::sTheObjectPtr, NC2Layout::DirectSoundManagerOffset::Initialized) == 0 || soundId == NXBasics::IntegerRange::UInt32Maximum) return;
		const std::int32_t x = Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X);
		const std::int32_t y = Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y);
		const std::int32_t left = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleX);
		const std::int32_t top = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleY);
		const std::int32_t width = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleWidth);
		const std::int32_t height = Read<std::int16_t>(display, NC2Layout::EngineDisplayOffset::MapRectangleHeight);
		if (width <= 0 || height <= 0) return;
		if (attenuate)
		{
			const std::int16_t right = static_cast<std::int16_t>(left + width - 1);
			const std::int16_t bottom = static_cast<std::int16_t>(top + height - 1);
			if (x < left || x > right || y < top || y > bottom) return;
			if ((Read<std::uint8_t>(display, NC2Layout::EngineDisplayOffset::DrawFlags) & 4u) != 0)
			{
				const std::uint32_t player = Read<std::uint32_t>(display, NC2Layout::EngineDisplayOffset::PlayerId);
				if (player < 16u && (Read<std::uint16_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) == 0) return;
			}
		}
		// The Windows routine requires a nonzero display width and height.
		std::int32_t pan = (x - width / 2 - left) * 128 / width;
		const std::int32_t vertical = (y - top - height / 2) * 128 / height;
		const std::int32_t distance = ((pan < 0 ? -pan : pan) + (vertical < 0 ? -vertical : vertical)) / 2;
		if (distance > 64 && attenuate) return;
		const auto* sound = static_cast<const std::byte*>(CE2StaticDataSoundFXManager::sTheObjectPtr->staticSounds) + soundId * NC2Layout::RecordSize::StaticSoundFxUnsigned;
		const std::uint32_t count = Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WaveCount);
		if (count == 0) return;
		const std::uint32_t variant = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % count;
		const std::uint32_t waveId = Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::WavesUnsigned + variant * NC2Layout::RecordSize::StaticSoundWaveUnsigned);
		if (skipRunning && NDSManager::CManager::sTheObjectPtr->PreloadWave_IsRunning(waveId)) return;
		std::uint32_t volume = Read<std::uint32_t>(sound, NC2Layout::StaticSoundFxOffset::FirstWaveVolumeUnsigned + variant * NC2Layout::RecordSize::StaticSoundWaveUnsigned);
		if (attenuate) volume = ((64u - static_cast<std::uint32_t>(distance)) * volume) >> 6;
		else
		{
			if (pan < -64) pan = -64;
			if (pan > 63) pan = 63;
		}
		NDSManager::CManager::sTheObjectPtr->PreloadWave_Play(waveId, volume, 1, static_cast<std::uint32_t>(pan) + 64u, false, 0);
	}
}
