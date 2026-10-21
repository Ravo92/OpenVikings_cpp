#include "RemasterHumanSprites.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataPalettes.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2StaticAtomicAnimationData.h"
#include "GameProfile.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBobManager.h"
#include "XPalette.h"
#include "XPaletteAccess.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace NRemaster::HumanSprites
{
	namespace
	{
		using NC2Lifetime::Read;
		constexpr auto Longbow = SwitchIds::HumanJob::SoldierBowLong;
		constexpr auto Longsword = SwitchIds::HumanJob::SoldierSwordLong;
		constexpr std::uint32_t FrameCount = 64;
		constexpr std::uint32_t NativeToSheet[8] = { 6, 7, 1, 2, 3, 5, 4, 0 }; // Windows direction table 0042A84A.
		struct Blob { const std::byte* data = nullptr; std::uint32_t size = 0; };
		struct Definition { std::uint32_t tribe; std::uint32_t job; const char* stem; const char* palettePatch; std::uint32_t fallbackJob; bool blueGreyMask = false; };
		constexpr Definition Definitions[] = {
			{ 4, Longbow, "saracen_longbow", "Sar_Man_Base", SwitchIds::HumanJob::SoldierBowShort },
			{ 3, Longsword, "byzantine_longsword", "Vik_Man_Soldier", SwitchIds::HumanJob::SoldierSwordShort },
			{ 4, Longsword, "saracen_longsword", "Sar_Man_Base", SwitchIds::HumanJob::SoldierSwordShort },
			{ 7, Longsword, "egypt_longsword", "Egy_Soldier_Base", SwitchIds::HumanJob::SoldierSwordShort, true }
		};
		struct SpriteSet
		{
			bool blueGreyMask = false;
			std::unique_ptr<NXBasics::CBobManager> body;
			std::unique_ptr<NXBasics::CPalette> palette;
			std::array<std::unique_ptr<NXBasics::CPalette>, 20> playerPalettes;
			std::array<std::uint32_t, 20> playerColors{};
		};
		std::array<SpriteSet, std::size(Definitions)> gSprites;
		bool gEnabled = false;
		bool gExperimental = false;

		std::size_t FindDefinition(std::uint32_t tribe, std::uint32_t job)
		{
			if (tribe == 7 && gGameVariant == EGameVariant::Cultures2) return std::size(Definitions);
			if (job == Longbow || job == Longsword)
				for (std::size_t index = 0; index < std::size(Definitions); ++index)
					if (Definitions[index].tribe == tribe && Definitions[index].job == job) return index;
			return std::size(Definitions);
		}

		bool Take(Blob& input, std::uint32_t size, Blob& output)
		{
			if (size > input.size) return false;
			output = { input.data, size }; input.data += size; input.size -= size;
			return true;
		}

		bool Number(Blob& input, std::uint32_t& value)
		{
			Blob number;
			if (!Take(input, 4, number)) return false;
			std::memcpy(&value, number.data, 4);
			return true;
		}

		bool ReadArchive(const std::filesystem::path& path, std::vector<std::byte>& bytes)
		{
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file) return false;
			const auto size = file.tellg();
			// This is a bounded optional art package, independent of the game's file-size ABI.
			if (size < 12 || size > 4 * 1024 * 1024) return false;
			bytes.resize(static_cast<std::size_t>(size));
			file.seekg(0);
			return static_cast<bool>(file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())));
		}

		bool FindEntries(const std::vector<std::byte>& bytes, const char* stem, Blob& body, Blob& palette)
		{
			Blob table{ bytes.data(), static_cast<std::uint32_t>(bytes.size()) };
			std::uint32_t version, directories, files;
			if (!Number(table, version) || !Number(table, directories) || !Number(table, files) || version != 0 || directories != 0 || files > 128) return false;
			const auto bodyName = std::string("data\\remaster\\") + stem + ".bmd";
			const auto paletteName = std::string("data\\remaster\\") + stem + ".pcx";
			for (std::uint32_t i = 0; i < files; ++i)
			{
				std::uint32_t length, offset, size; Blob name;
				if (!Number(table, length) || length > 128 || !Take(table, length, name) || !Number(table, offset) || !Number(table, size) || offset > bytes.size() || size > bytes.size() - offset) return false;
				const std::string_view text(reinterpret_cast<const char*>(name.data), name.size);
				if (text == bodyName) { if (body.data != nullptr) return false; body = { bytes.data() + offset, size }; }
				if (text == paletteName) { if (palette.data != nullptr) return false; palette = { bytes.data() + offset, size }; }
			}
			const auto tableSize = bytes.size() - table.size;
			return body.data != nullptr && palette.data != nullptr && body.data >= bytes.data() + tableSize && palette.data >= bytes.data() + tableSize;
		}

		bool ReadBob(Blob input, std::array<Blob, 3>& blocks)
		{
			std::uint32_t id, version, first, count, packedSize; Blob reserved;
			if (!Number(input, id) || !Number(input, version) || !Number(input, first) || !Number(input, count) || !Number(input, packedSize) || !Take(input, 16, reserved) || id != 0x3F4 || version != 0 || first != 0 || count != FrameCount) return false;
			for (auto& block : blocks)
			{
				std::uint32_t memoryId, memoryVersion, size;
				if (!Number(input, memoryId) || !Number(input, memoryVersion) || !Number(input, size) || memoryId != 0x3E9 || memoryVersion != 0 || !Take(input, size, block)) return false;
			}
			if (input.size != 0 || blocks[0].size != FrameCount * 24 || blocks[1].size != packedSize || blocks[2].size % 4 != 0) return false;
			for (std::uint32_t frame = 0; frame < FrameCount; ++frame)
			{
				const auto* record = blocks[0].data + frame * 24;
				const auto width = Read<std::uint32_t>(record, 12), height = Read<std::uint32_t>(record, 16), firstLine = Read<std::uint32_t>(record, 20);
				if (Read<std::uint32_t>(record, 0) != 4 || width == 0 || width > 96 || height == 0 || height > 96 || firstLine > blocks[2].size / 4 || height > blocks[2].size / 4 - firstLine) return false;
				const auto x = Read<std::int32_t>(record, 4), y = Read<std::int32_t>(record, 8);
				if (x < -48 || x > 48 - static_cast<std::int32_t>(width) || y < -88 || y > 8 - static_cast<std::int32_t>(height)) return false;
				for (std::uint32_t row = 0; row < height; ++row)
				{
					const auto descriptor = Read<std::uint32_t>(blocks[2].data, (firstLine + row) * 4);
					if (descriptor == UINT32_MAX) continue;
					auto column = descriptor >> 22, position = descriptor & 0x3FFFFFu;
					for (;;)
					{
						if (position >= blocks[1].size || column > width) return false;
						const auto token = std::to_integer<std::uint8_t>(blocks[1].data[position++]);
						if (token == 0) break;
						const auto run = token & 127u;
						if (run == 0 || run > width - column) return false;
						column += run;
						if ((token & 128u) == 0) { if (run * 2 > blocks[1].size - position) return false; position += run * 2; }
					}
				}
			}
			return true;
		}

		bool Load(const std::filesystem::path& path)
		{
			std::vector<std::byte> bytes;
			if (!ReadArchive(path, bytes)) return false;
			bool loaded = false;
			for (std::size_t index = 0; index < std::size(Definitions); ++index)
			{
				Blob body, palette; std::array<Blob, 3> blocks;
				if (!FindEntries(bytes, Definitions[index].stem, body, palette) || !ReadBob(body, blocks) || palette.size < 897 || palette.data[palette.size - 769] != std::byte{ 12 }) continue;
				auto newBody = std::make_unique<NXBasics::CBobManager>(0, FrameCount, blocks[0].data, blocks[0].size, blocks[1].data, blocks[1].size, blocks[2].data, blocks[2].size);
				auto newPalette = std::make_unique<NXBasics::CPalette>();
				auto* colors = reinterpret_cast<NXBasics::SColorRGB*>(reinterpret_cast<std::byte*>(newPalette.get()) + NC2Layout::PaletteOffset::Colors);
				const auto* rgb = reinterpret_cast<const std::uint8_t*>(palette.data + palette.size - 768);
				for (std::uint32_t i = 0; i < 256; ++i) colors[i].SetColors(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
				NXBasics::Palette_PaletteChanged(newPalette.get());
				gSprites[index].body = std::move(newBody); gSprites[index].palette = std::move(newPalette);
				gSprites[index].blueGreyMask = Definitions[index].blueGreyMask;
				loaded = true;
			}
			return loaded;
		}

		std::uint32_t AttackTick(const void* moveable, std::uint32_t total)
		{
			const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
			const auto* human = NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::ElementSize;
			const auto animation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId);
			if (animation < NC2Logic::gStaticAtomicAnimationStorage.count)
			{
				const auto& atomic = NC2Logic::gStaticAtomicAnimationStorage.records[animation];
				for (std::uint32_t i = 0; i < (std::min)(atomic.eventCount, 20u); ++i)
					if (atomic.events[i].type == SwitchIds::AtomicAnimationEvent::Attack && atomic.events[i].time >= 0) return static_cast<std::uint32_t>(atomic.events[i].time);
			}
			return total * 3u / 4u;
		}
	}

	void Initialize(bool enabled, bool experimental)
	{
		Shutdown();
		gEnabled = enabled; gExperimental = experimental;
		if (!enabled) return;
		// The draft generated figures were rejected in-game. Native faction animation is the default.
		if (!experimental) return;
		try
		{
			std::array<wchar_t, 32768> module{};
			const auto length = ::GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
			if (length != 0 && length < module.size() && Load(std::filesystem::path(module.data()).parent_path() / L"remaster.lib")) return;
			if (Load(std::filesystem::path(L"assets/remaster/remaster.lib"))) return;
		}
		catch (const std::exception&) { Shutdown(); gEnabled = enabled; }
		::OutputDebugStringA("OpenVikings: optional remaster sprite pack unavailable or invalid; using original graphics.\n");
	}

	void Shutdown()
	{
		gEnabled = false; gExperimental = false;
		for (auto& sprite : gSprites)
		{
			for (auto& palette : sprite.playerPalettes) palette.reset();
			sprite.palette.reset(); sprite.body.reset();
		}
	}

	bool Supports(std::uint32_t tribe, std::uint32_t job)
	{
		const auto index = FindDefinition(tribe, job);
		return gEnabled && index < gSprites.size();
	}

	std::uint32_t ResolveAppearanceTribe(std::uint32_t tribe, std::uint32_t job, std::int32_t player)
	{
		// Older setup maps retained the Viking logic tribe for unsupported weapon jobs.
		// Resolve their appearance only; saved units and combat permissions remain unchanged.
		if (tribe == 1 && (job == Longbow || job == Longsword) && player >= 0 && player < 20 && NC2Logic::gGameControl != nullptr)
		{
			const auto ownerTribe = Read<std::uint32_t>(NC2Logic::gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TribeType);
			if (Supports(ownerTribe, job)) return ownerTribe;
		}
		return tribe;
	}

	std::uint32_t NativeFallbackJob(std::uint32_t tribe, std::uint32_t job)
	{
		const auto index = FindDefinition(tribe, job);
		return index < std::size(Definitions) ? Definitions[index].fallbackJob : job;
	}

	void PrepareNativeFallback(NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job)
	{
		const auto* graphics = static_cast<const NC2E2::SHumanJobBaseGraphics*>(NC2E2::StaticHumanMisc_JobGraphicsBase_GetDataPtr(tribe, NativeFallbackJob(tribe, job)));
		if (graphics->graphics.bodyCount != 0)
		{
			const auto& body = graphics->graphics.bodies[0];
			dynamic.bodyBobManager = body.bodyBobManager; dynamic.shadowBobManager = body.shadowBobManager;
		}
		dynamic.headBobManager = graphics->graphics.headCount != 0 ? graphics->graphics.heads[dynamic.actorData.human.head % graphics->graphics.headCount].headBobManager : nullptr;
		dynamic.animation = nullptr; dynamic.walk = nullptr;
	}

	void PrepareNativeFallbackPalettes(std::uint32_t tribe, std::uint32_t job, std::int32_t player, NXBasics::CPalette& body, NXBasics::CPalette& head)
	{
		const auto* graphics = static_cast<const NC2E2::SHumanJobBaseGraphics*>(NC2E2::StaticHumanMisc_JobGraphicsBase_GetDataPtr(tribe, NativeFallbackJob(tribe, job)));
		const auto copyBase = [] (NXBasics::CPalette& destination, std::uint32_t count, const std::int32_t* ids)
		{
			if (count != 0 && ids[0] >= 0 && ids[0] < NC2E2::gStaticPalettes.palette256Count && NC2E2::gStaticPalettes.palettes256[ids[0]].palette != nullptr) destination = *NC2E2::gStaticPalettes.palettes256[ids[0]].palette;
		};
		copyBase(body, graphics->bodyPaletteCount, graphics->bodyPaletteIds);
		copyBase(head, graphics->headPaletteCount, graphics->headPaletteIds);
		const auto playerPatch = player >= 0 && player < 20 && NC2Logic::gGameControl != nullptr ? NC2E2::StaticHumanMisc_ColorIds_GetPlayerRandomPatchId(player, false) : -1;
		if (playerPatch != -1) NC2E2::StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(playerPatch), &body, &head, true);
		for (const auto& definition : Definitions) if (definition.tribe == tribe)
		{
			const auto patch = NC2E2::StaticHumanMisc_RandomPalette_GetPatchId(definition.palettePatch);
			if (patch != -1) NC2E2::StaticHumanMisc_RandomPalette_Execute(static_cast<std::uint32_t>(patch), &body, &head, true);
			break;
		}
	}

	bool SelectFrame(const void* moveable, NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, bool atomicActive, std::uint32_t action, std::uint32_t done, std::uint32_t total)
	{
		if (!gExperimental || !Supports(tribe, job) || good != 0 || gSprites[FindDefinition(tribe, job)].body == nullptr) return false;
		const bool turning = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0;
		const bool walking = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0;
		if (!turning && !walking && atomicActive && (action < 2 || action > 7) && action != 81 && !(job == Longsword && action == 82)) return false;
		const auto direction = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u;
		if (direction >= 8) return false;
		std::uint32_t pose = 0;
		if (walking && !turning) pose = 1u + (NC2Logic::gGameTime.tick % 12u) / 3u;
		else if (!turning && atomicActive && (action == 81 || action == 82))
		{
			const auto impact = AttackTick(moveable, total);
			if (job == Longbow) pose = done >= impact ? 7u : (done >= impact / 3u ? 6u : 5u);
			else pose = done < impact ? 5u : (done - impact < (std::max)(1u, (total > impact ? total - impact : 1u) / 3u) ? 6u : 7u);
		}
		dynamic.bodyBobManager = gSprites[FindDefinition(tribe, job)].body.get(); dynamic.headBobManager = nullptr; dynamic.shadowBobManager = nullptr;
		dynamic.bodyBobId = pose * 8u + NativeToSheet[direction]; dynamic.headBobId = UINT32_MAX;
		return true;
	}

	NXBasics::CPalette* GetPalette(const NC2E2::SE2DynamicDataMapMoveableData& dynamic, std::int32_t player, NXBasics::CPalette* fallback)
	{
		SpriteSet* sprite = nullptr;
		for (auto& candidate : gSprites) if (candidate.body != nullptr && dynamic.bodyBobManager == candidate.body.get()) { sprite = &candidate; break; }
		if (sprite == nullptr) return fallback;
		if (player < 0 || player >= 20) return sprite->palette.get();
		const auto& color = *NC2E2::Tool_PlayerColors_GetColorRef(player);
		const auto key = static_cast<std::uint32_t>(color.red) << 16 | static_cast<std::uint32_t>(color.green) << 8 | color.blue;
		auto& palette = sprite->playerPalettes[static_cast<std::size_t>(player)];
		if (palette == nullptr || sprite->playerColors[static_cast<std::size_t>(player)] != key)
		{
			palette = std::make_unique<NXBasics::CPalette>(*sprite->palette);
			auto* colors = reinterpret_cast<NXBasics::SColorRGB*>(reinterpret_cast<std::byte*>(palette.get()) + NC2Layout::PaletteOffset::Colors);
			for (std::uint32_t i = 0; i < 256; ++i)
			{
				const auto original = colors[i];
				// Match the source garment hue; retain neutral steel, skin and gold colors.
				const bool garment = sprite->blueGreyMask ? original.green > 20 && original.blue * 10u > original.red * 12u && original.green * 20u > original.red * 21u && original.blue * 10u > original.green * 11u : original.green > 30 && original.green * 10u > original.red * 13u && original.blue * 10u > original.red * 12u;
				if (garment)
				{
					const auto value = (std::max)(original.green, original.blue);
					colors[i].SetColors(static_cast<std::uint8_t>(color.red * value / 255u), static_cast<std::uint8_t>(color.green * value / 255u), static_cast<std::uint8_t>(color.blue * value / 255u));
				}
			}
			NXBasics::Palette_PaletteChanged(palette.get());
			sprite->playerColors[static_cast<std::size_t>(player)] = key;
		}
		return palette.get();
	}
}
