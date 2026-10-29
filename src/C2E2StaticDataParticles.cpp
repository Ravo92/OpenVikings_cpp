#include "C2DataIds.h"
#include "C2E2StaticDataPalettes.h"
#include "C2E2StaticDataParticles.h"
#include "C2EngineLifetimeAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>

namespace NC2E2
{
	using namespace NC2Lifetime;

	namespace
	{
		// Original work regions: 00568D58..00568DF7 and 00568DF8..0056904F.
		// The second extent is inferred from the next independently referenced global.
		SParticelFrames* gParticelFrameWork[40]{ };
		std::uint32_t gParticelFrameIds[150]{ };
		constexpr NXBasics::SToken gParticleTokens[] = { { "name", SwitchIds::ParticleToken::Name }, { "palette", SwitchIds::ParticleToken::Palette }, { "bobmanager", SwitchIds::ParticleToken::Bobmanager }, { "maxvalency", SwitchIds::ParticleToken::MaxValency }, { "gfxframes", SwitchIds::ParticleToken::GfxFrames }, { "animloop", SwitchIds::ParticleToken::Animloop }, { "bobtype", SwitchIds::ParticleToken::Bobtype }, { "munitiontype", SwitchIds::ParticleToken::MunitionType }, { "valencyisdirection", SwitchIds::ParticleToken::ValencyIsDirection }, { "specialtype", SwitchIds::ParticleToken::SpecialType }, { "spawnparticelId", SwitchIds::ParticleToken::SpawnparticelId }, { nullptr, 0 } };
		constexpr NXBasics::SToken gEffectTokens[] = { { "logictype", NC2DataIds::ParticlesToken::LogicType }, { "particelname", SwitchIds::ParticleEffectToken::Particelname }, { "lifetime", SwitchIds::ParticleEffectToken::LifeTime }, { "movement", SwitchIds::ParticleEffectToken::Movement }, { "particelspeed", SwitchIds::ParticleEffectToken::Particelspeed }, { "usemapheights", SwitchIds::ParticleEffectToken::Usemapheights }, { "usewind", SwitchIds::ParticleEffectToken::Usewind }, { "deltaheight", SwitchIds::ParticleEffectToken::Deltaheight }, { "pixeloffset", SwitchIds::ParticleEffectToken::PixelOffset }, { "drawbottom", SwitchIds::ParticleEffectToken::Drawbottom }, { "migPointoffset", SwitchIds::ParticleEffectToken::MigPointoffset }, { "startLifeTime", SwitchIds::ParticleEffectToken::StartLifeTime }, { nullptr, 0 } };

		// Original function: FUN_0048d589; SGFXValency owned frame allocation.
		void ParticelFrames_Allocate(SParticelFrames* entry)
		{
			if (entry->count == 0)
			{

				return;
			}

			entry->frames = static_cast<std::uint32_t*>(NXBasics::Runtime_AllocateArrayWithNewHandler(entry->count, sizeof(std::uint32_t)));
			std::memcpy(entry->frames, gParticelFrameIds, static_cast<std::size_t>(entry->count) * sizeof(std::uint32_t));
		}

		// Original function: FUN_0048d668; SGFXValency deep copy via native scratch.
		void ParticelFrames_Assign(SParticelFrames* destination, const SParticelFrames* source)
		{
			if (destination->frames != nullptr)
			{
				NXBasics::Runtime_Free(destination->frames);
				destination->frames = nullptr;
			}

			destination->valency = source->valency;

			for (std::uint32_t index = 0; index < source->count; ++index)
			{
				gParticelFrameIds[destination->count++] = source->frames[index];
			}

			ParticelFrames_Allocate(destination);
		}

		// Original function: FUN_0048d5f2; SGFXValencyHandler::AddGraphicsStructureToList.
		void GFXValencyHandler_AddGraphicsStructureToList(void* list, const SParticelFrames* frames)
		{
			const auto count = Read<std::uint32_t>(list, NC2Layout::ParticleGraphicsOffset::EntryCount);
			std::uint32_t index = 0;

			while (index < count && gParticelFrameWork[index]->valency <= frames->valency)
				++index;

			for (auto end = count; end > index; --end)
			{
				gParticelFrameWork[end] = gParticelFrameWork[end - 1u];
			}

			auto* allocation = static_cast<SParticelFrames*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SParticelFrames), 1));

			if (allocation != nullptr)
				std::memset(allocation, 0, sizeof(SParticelFrames));

			gParticelFrameWork[index] = allocation;
			ParticelFrames_Assign(allocation, frames);
			Write(list, NC2Layout::ParticleGraphicsOffset::EntryCount, count + 1u);
		}

		// Original function: FUN_0048d6b2; SGFXValencyHandler seals its pointer array.
		void ParticelFrames_FinishList(void* list)
		{
			const auto count = Read<std::uint32_t>(list, NC2Layout::ParticleGraphicsOffset::EntryCount);

			if (count == 0)
			{

				return;
			}

			void* allocation = NXBasics::Runtime_AllocateArrayWithNewHandler(count, sizeof(SParticelFrames*));
			Write(list, NC2Layout::ParticleGraphicsOffset::Entries, allocation);
			std::memcpy(allocation, gParticelFrameWork, static_cast<std::size_t>(count) * sizeof(SParticelFrames*));
			std::memset(gParticelFrameWork, 0, sizeof(gParticelFrameWork));
		}

		// Original function: FUN_0048d28b.
		void E2StaticDataParticelManager_l_Initialize(const char* filename)
		{
			NXBasics::CIniFileReader reader(filename, nullptr, false, nullptr, nullptr);
			reader.GetNumberOfGroups("Particel"); // Native capacity remains thirty regardless of this result.

			for (auto* group = reader.GetFirstGroup("Particel"); group != nullptr; group = reader.GetNextGroup("Particel"))
			{
				void* entry = static_cast<std::byte*>(gStaticParticelStorage.entries) + static_cast<std::uint32_t>(gStaticParticelStorage.count++) * NC2Layout::RecordSize::StaticParticleUnsigned;
				std::memset(entry, 0, sizeof(SStaticParticelStorageEntry));

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					switch (NXBasics::XB_Token_FindTokenInArray(gParticleTokens, reader.GetCurrentName()))
					{
						case SwitchIds::ParticleToken::Name:
						{
							const char* name = reader.GetString();

							if (*name != '\0')
								std::strcpy(static_cast<char*>(entry), name);

							break;
						}
						case SwitchIds::ParticleToken::Palette:
						{
							const char* name = reader.GetString();

							if (*name != '\0')
								Write(entry, NC2Layout::StaticParticleOffset::PaletteId, E2StaticDataPalettesManager_GetPalette256Id(name));

							break;
						}
						case SwitchIds::ParticleToken::Bobmanager:
						{
							const char* name = reader.GetString();

							if (*name != '\0')
								Write(entry, NC2Layout::StaticParticleOffset::BobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, name));

							break;
						}
						case SwitchIds::ParticleToken::MaxValency:
							Write(entry, NC2Layout::StaticParticleOffset::MaximumValency, reader.GetInt());
							break;
						case SwitchIds::ParticleToken::Animloop:
							Write<std::uint8_t>(entry, NC2Layout::StaticParticleOffset::AnimationLoops, reader.GetInt() != 0);
							break;
						case SwitchIds::ParticleToken::Bobtype:
							Write(entry, NC2Layout::StaticParticleOffset::BobType, reader.GetInt());
							break;
						case SwitchIds::ParticleToken::MunitionType:
							gStaticParticelStorage.munitionIds[reader.GetInt()] = gStaticParticelStorage.count - 1;
							break;
						case SwitchIds::ParticleToken::ValencyIsDirection:
							Write<std::uint8_t>(entry, NC2Layout::StaticParticleOffset::ValencyIsDirection, reader.GetInt() != 0);
							break;
						case SwitchIds::ParticleToken::SpecialType:
							Write(entry, NC2Layout::StaticParticleOffset::SpecialType, reader.GetInt());
							break;
						case SwitchIds::ParticleToken::SpawnparticelId:
							Write<std::uint8_t>(entry, NC2Layout::StaticParticleOffset::HasSpawnParticle, 1);
							Write(entry, NC2Layout::StaticParticleOffset::SpawnParticleId, reader.GetInt());
							break;
						case SwitchIds::ParticleToken::GfxFrames:
						{
							SParticelFrames frames{ };
							frames.valency = static_cast<std::uint32_t>(reader.GetInt());

							while (reader.GetTypeOfNextParameter() == 3)
								gParticelFrameIds[frames.count++] = static_cast<std::uint32_t>(reader.GetInt());

							ParticelFrames_Allocate(&frames);
							GFXValencyHandler_AddGraphicsStructureToList(static_cast<std::byte*>(entry) + NC2Layout::StaticParticleOffset::Graphics, &frames);

							if (frames.frames != nullptr)
								NXBasics::Runtime_Free(frames.frames);

							break;
						}
					}
				}

				ParticelFrames_FinishList(static_cast<std::byte*>(entry) + NC2Layout::StaticParticleOffset::Graphics);
			}
		}
	}

	// Original function: FUN_0048d194.
	void* __fastcall E2StaticDataParticelManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataParticelManager = manager;
		std::memset(&gStaticParticelStorage, 0, sizeof(gStaticParticelStorage));
		auto* allocation = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(StaticParticelArrayCookieBytes + 30u * sizeof(SStaticParticelStorageEntry), 1));

		if (allocation != nullptr)
		{
			Write<std::uint32_t>(allocation, NC2Layout::ObjectArrayCookieOffset::ElementCount, 30);

			for (std::uint32_t index = 0; index < 30; ++index)
			{
				std::memset(allocation + StaticParticelArrayCookieBytes + index * NC2Layout::RecordSize::StaticParticleUnsigned + NC2Layout::StaticParticleOffset::Graphics, 0, sizeof(SParticelGraphicsStorage));
			}

			gStaticParticelStorage.entries = allocation + StaticParticelArrayCookieBytes;
		}

		for (auto& id : gStaticParticelStorage.munitionIds)
		{
			id = -1;
		}

		E2StaticDataParticelManager_l_Initialize("Data\\Engine2D\\Inis\\Particel\\Particel.ini");

		return manager;
	}

	// Original function: FUN_0048d54e.
	std::int32_t E2StaticDataParticelManager_GetParticelId(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticParticelStorage.count; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(static_cast<const char*>(gStaticParticelStorage.entries) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::StaticParticleUnsigned, name) == 0)
				return index;
		}

		return -1;
	}

	// Original function: FUN_00486f68; semantic reconstruction name, original helper name unresolved.
	void* ParticelGraphics_GetValency(const void* list, std::uint32_t valency)
	{
		for (auto index = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(list, NC2Layout::ParticleGraphicsOffset::EntryCount) - 1u); index >= 0; --index)
		{
			void* entry = Read<void*>(Read<void*>(list, NC2Layout::ParticleGraphicsOffset::Entries), static_cast<std::size_t>(index) * sizeof(SParticelFrames*));

			if (Read<std::uint32_t>(entry, NC2Layout::ParticleFramesOffset::MinimumValency) <= valency)
				return entry;
		}

		return nullptr;
	}

	// Original function: FUN_0048a49d.
	static void ParticelEffect_Construct(void* effect)
	{
		Write<std::uint16_t>(effect, NC2Layout::ParticleEffectOffset::MapOffsetX, 0);
		Write<std::uint16_t>(effect, NC2Layout::ParticleEffectOffset::MapOffsetY, 0);
		std::memset(effect, 0, 52);
	}

	// Original function: FUN_0048a028.
	void* __fastcall E2StaticDataParticelEffectManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataParticelEffectManager = manager;
		gStaticParticelEffects = nullptr;
		auto* effects = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(364, 1));

		if (effects != nullptr)
			for (std::uint32_t index = 0; index < 7; ++index)
			{
				ParticelEffect_Construct(effects + index * NC2Layout::RecordSize::ParticleEffectUnsigned);
			}

		gStaticParticelEffects = effects;
		NXBasics::CIniFileReader reader("Data\\Engine2D\\Inis\\Particel\\ParticelEffects.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		for (auto* group = reader.GetFirstGroup("ParticelEffect"); group != nullptr; group = reader.GetNextGroup("ParticelEffect"))
		{
			std::uint32_t index = 0;
			reader.GetNextCommandLineInGroup();

			if (NXBasics::Runtime_CompareStringsIgnoreCase(reader.GetCurrentName(), "logictype") == 0)
				index = static_cast<std::uint32_t>(reader.GetInt());

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				auto* entry = effects + index * NC2Layout::RecordSize::ParticleEffectUnsigned;

				switch (NXBasics::XB_Token_FindTokenInArray(gEffectTokens, reader.GetCurrentName()))
				{
					case SwitchIds::ParticleEffectToken::Particelname:
						Write(entry, NC2Layout::ParticleEffectOffset::ParticleId, E2StaticDataParticelManager_GetParticelId(reader.GetString()));
						break;
					case SwitchIds::ParticleEffectToken::LifeTime:
						Write(entry, NC2Layout::ParticleEffectOffset::LifetimeTicks, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::StartLifeTime:
						Write(entry, NC2Layout::ParticleEffectOffset::StartLifetimeTicks, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::Movement:
						Write(entry, NC2Layout::ParticleEffectOffset::MovementMode, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::Particelspeed:
						Write(entry, NC2Layout::ParticleEffectOffset::SpeedMapX, reader.GetInt());
						Write(entry, NC2Layout::ParticleEffectOffset::SpeedHeight, reader.GetInt());
						Write(entry, NC2Layout::ParticleEffectOffset::SpeedMapY, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::Usemapheights:
						Write<std::uint8_t>(entry, NC2Layout::ParticleEffectOffset::UseMapHeights, reader.GetInt() != 0);
						break;
					case SwitchIds::ParticleEffectToken::Usewind:
						Write<std::uint8_t>(entry, NC2Layout::ParticleEffectOffset::UseWind, reader.GetInt() != 0);
						break;
					case SwitchIds::ParticleEffectToken::Deltaheight:
						Write(entry, NC2Layout::ParticleEffectOffset::HeightOffset, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::PixelOffset:
						Write(entry, NC2Layout::ParticleEffectOffset::PixelPoint, reader.GetInt());
						Write(entry, NC2Layout::ParticleEffectOffset::PixelY, reader.GetInt());
						break;
					case SwitchIds::ParticleEffectToken::Drawbottom:
						Write<std::uint8_t>(entry, NC2Layout::ParticleEffectOffset::DrawBottom, reader.GetInt() != 0);
						break;
					case SwitchIds::ParticleEffectToken::MigPointoffset:
						Write<std::uint16_t>(entry, NC2Layout::ParticleEffectOffset::MapOffsetX, static_cast<std::uint16_t>(reader.GetInt()));
						Write<std::uint16_t>(entry, NC2Layout::ParticleEffectOffset::MapOffsetY, static_cast<std::uint16_t>(reader.GetInt()));
						break;
				}
			}
		}

		return manager;
	}
}