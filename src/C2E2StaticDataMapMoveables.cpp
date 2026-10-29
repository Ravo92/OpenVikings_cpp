#include "C2DataIds.h"
#include "C2E2StaticDataMapMoveables.h"
#include "C2LifetimeAccess.h"
#include "C2E2StaticDataPalettes.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineLifetimeAccess.h"
#include "C2GameState.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>

namespace NC2E2
{
	// Original function: FUN_0048cc8f; the original destructor is an empty RET.
	static void __cdecl SequenceTokenList_AtExit()
	{
	}

	// Original function: FUN_0048cc8e; the original destructor is an empty RET.
	static void __cdecl AnimationTokenList_AtExit()
	{
	}

	// Original function: FUN_0048cc8d; the original destructor is an empty RET.
	static void __cdecl WalkTokenList_AtExit()
	{
	}

	// Original function: FUN_0048cc8c; the original destructor is an empty RET.
	static void __cdecl AnimalJobTokenList_AtExit()
	{
	}

	// Original function: FUN_0048cc8b; the original destructor is an empty RET.
	static void __cdecl VehicleJobTokenList_AtExit()
	{
	}

	// Original function: FUN_0048d026; the original destructor is an empty RET.
	static void __cdecl AnimationSelectionRandom_AtExit()
	{
	}

	template <void(__cdecl* AtExit)()>
	static NXBasics::CTokenList& GetLazyTokenList(const NXBasics::SToken* tokens)
	{
		static bool initialized = false;
		alignas(NXBasics::CTokenList) static std::byte storage[sizeof(NXBasics::CTokenList)];

		if (!initialized)
		{
			initialized = true;
			::new (storage) NXBasics::CTokenList(tokens);
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(AtExit));
		}

		return *reinterpret_cast<NXBasics::CTokenList*>(storage);
	}

	namespace
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;
		using AnimationList = NC2Lifetime::CRuntimeOwnedList<SStaticMoveableAnimationListTag>;
		using WalkList = NC2Lifetime::CRuntimeOwnedList<SStaticMoveableWalkListTag>;

		// Complete Windows token tables 004F4858..004F495F and 00506708..00506727.
		constexpr NXBasics::SToken gSequenceTokens[] = { { "imagelib", SwitchIds::AnimationSequenceToken::Imagelib }, { "shadowlib", SwitchIds::AnimationSequenceToken::Shadowlib }, { "seq", NC2DataIds::MapMoveablesToken::Seq }, { nullptr, 0 } };
		constexpr NXBasics::SToken gAnimationTokens[] = { { "logictribe", SwitchIds::AnimalVehicleJobToken::LogicTribe }, { "logicjob", SwitchIds::AnimalVehicleJobToken::LogicJob }, { "logicatomicaction", SwitchIds::HumanAnimationToken::LogicAtomicAction }, { "logicinhouseatomicsubid", SwitchIds::HumanAnimationToken::LogicInHouseAtomicSubId }, { "gfxanimmode", SwitchIds::HumanAnimationToken::Gfxanimmode }, { "gfxbobseqbody", SwitchIds::HumanAnimationToken::Gfxbobseqbody }, { "gfxbobseqhead", SwitchIds::HumanAnimationToken::Gfxbobseqhead }, { "gfxanimframelist", NC2DataIds::MapMoveablesToken::GraphicsAnimationFrameList }, { "gfxanimframelistdir", SwitchIds::HumanAnimationToken::Gfxanimframelistdir }, { "gfxinhousewalk", SwitchIds::HumanAnimationToken::GfxInHouseWalk }, { "gfxinhouseanim", SwitchIds::HumanAnimationToken::Gfxinhouseanim }, { "gfxinhouseoverlaybob", SwitchIds::HumanAnimationToken::Gfxinhouseoverlaybob }, { "gfxinhouseoverlaylandscape", SwitchIds::HumanAnimationToken::GfxInHouseOverlayLandscape }, { nullptr, 0 } };
		constexpr NXBasics::SToken gWalkTokens[] = { { "logictribe", SwitchIds::AnimalVehicleJobToken::LogicTribe }, { "logicjob", SwitchIds::AnimalVehicleJobToken::LogicJob }, { "logicgoodtype", SwitchIds::HumanWalkAnimationToken::LogicGoodType }, { "logicwalkspeed", SwitchIds::HumanWalkAnimationToken::LogicWalkSpeed }, { "gfxbobseqbody", SwitchIds::HumanAnimationToken::Gfxbobseqbody }, { "gfxbobseqhead", SwitchIds::HumanAnimationToken::Gfxbobseqhead }, { "gfxwalkframelist", NC2DataIds::MapMoveablesToken::GraphicsWalkFrameList }, { "gfxturnframelist", SwitchIds::HumanWalkAnimationToken::Gfxturnframelist }, { nullptr, 0 } };
		constexpr NXBasics::SToken gAnimalJobTokens[] = { { "logictribe", SwitchIds::AnimalVehicleJobToken::LogicTribe }, { "logicjob", SwitchIds::AnimalVehicleJobToken::LogicJob }, { "gfxbobmanagerbody", SwitchIds::AnimalVehicleJobToken::Gfxbobmanagerbody }, { "gfxpalettebody", SwitchIds::AnimalVehicleJobToken::GfxPaletteBody }, { nullptr, 0 } };
		constexpr NXBasics::SToken gVehicleJobTokens[] = { { "logictribe", SwitchIds::AnimalVehicleJobToken::LogicTribe }, { "logicvehicle", SwitchIds::AnimalVehicleJobToken::LogicVehicle }, { "gfxbobmanagerbody", SwitchIds::AnimalVehicleJobToken::Gfxbobmanagerbody }, { "gfxpalettebody", SwitchIds::AnimalVehicleJobToken::GfxPaletteBody }, { nullptr, 0 } };
		static_assert(sizeof(gAnimationTokens) == 14 * sizeof(NXBasics::SToken));
		static_assert(sizeof(gWalkTokens) == 9 * sizeof(NXBasics::SToken));

		// Original random object 00568988..0056898F; independent from game randomness.
		NC2Logic::SGameRandomState gAnimationSelectionRandom{ };
		bool gAnimationSelectionRandomInitialized = false; // Original initialization bit 005689BD & 1.

		// Original function: FUN_0048cce2; the preceding count is not initialized here.
		void* StaticMoveableAnimation_Construct(void* group)
		{
			::new (static_cast<std::byte*>(group) + NC2Layout::FastMoveableJobOffset::Entries) AnimationList(false);

			return group;
		}

		// Original function: FUN_0048ccfd.
		void* StaticMoveableWalk_Construct(void* group)
		{
			::new (static_cast<std::byte*>(group) + NC2Layout::FastMoveableJobOffset::Entries) WalkList(false);

			return group;
		}

		// Original function: FUN_0048cc90; SFastAnimationDataAccess constructor.
		void FastAnimationDataAccess_Construct(void* groups)
		{
			for (std::uint32_t tribe = 0; tribe < NC2Runtime::FastMoveableAccessLimits::TribeSlots; ++tribe)
			{
				for (std::uint32_t job = 0; job < NC2Runtime::FastMoveableAccessLimits::JobSlots; ++job)
				{
					StaticMoveableAnimation_Construct(static_cast<std::byte*>(groups) + NC2Layout::FastMoveableTribeOffset::Jobs + tribe * NC2Layout::RecordSize::FastMoveableTribeUnsigned + job * NC2Layout::RecordSize::FastMoveableJobUnsigned);
				}
			}
		}

		// Original function: FUN_0048ccb9; SFastWalkDataAccess constructor.
		void FastWalkDataAccess_Construct(void* groups)
		{
			for (std::uint32_t tribe = 0; tribe < NC2Runtime::FastMoveableAccessLimits::TribeSlots; ++tribe)
			{
				for (std::uint32_t job = 0; job < NC2Runtime::FastMoveableAccessLimits::JobSlots; ++job)
				{
					StaticMoveableWalk_Construct(static_cast<std::byte*>(groups) + NC2Layout::FastMoveableTribeOffset::Jobs + tribe * NC2Layout::RecordSize::FastMoveableTribeUnsigned + job * NC2Layout::RecordSize::FastMoveableJobUnsigned);
				}
			}
		}

		template <class T>
		T* Allocate(std::uint32_t count)
		{

			return static_cast<T*>(NXBasics::Runtime_AllocateArrayWithNewHandler(count, sizeof(T), 1));
		}

		template <class T>
		T* AllocateCleared(std::uint32_t count)
		{
			T* result = Allocate<T>(count);
			std::memset(result, 0, static_cast<std::size_t>(count) * sizeof(T));

			return result;
		}

		void CopyFrames(SMapMoveableFrameList& target, const std::uint32_t* source, std::uint32_t count)
		{
			if (count == 0)
			{

				return;
			}

			target.count = count;
			target.frames = Allocate<std::uint32_t>(count);
			std::memcpy(target.frames, source, count * 4u);
		}

		// Original function: FUN_0048cea1; l_Generation_SearchBobSequence.
		std::int32_t SearchBobSequence(const void* manager, const char* name)
		{
			const auto* libraries = Read<SSequenceLibrary*>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraries);

			for (std::uint32_t library = 0; library < Read<std::uint32_t>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraryCount); ++library)
			{
				for (std::uint32_t index = 0; index < libraries[library].count; ++index)
				{
					if (std::strcmp(libraries[library].sequences[index].name, name) == 0)
						return libraries[library].sequences[index].firstBob;
				}
			}

			return -1;
		}

		void Remaster_RepairVikingWoodenSpearmanSleepFrames(const void* manager, const SE2StaticDataMapMoveableAnimationData& animation, std::uint32_t* frames, std::uint32_t& count)
		{
			// The original wooden-spearman list omits 159 resting frames from a 237-tick sleep.
			// Restore the hold used by the iron spearman with the same body sequence.
			constexpr std::uint32_t originalCount = 78;
			constexpr std::uint32_t correctedCount = 237;
			constexpr std::uint32_t holdStart = 42;
			constexpr std::uint32_t missingFrames = correctedCount - originalCount;
			static_assert(correctedCount <= NC2Runtime::MoveableAnimationLimits::FramesPerList);

			if (animation.tribe != 1 || animation.job != SwitchIds::HumanJob::SoldierSpearWooden || animation.action != 8 || animation.inHouseSubId != 0 || animation.animationMode != NC2Runtime::MoveableAnimationMode::ProgressFrames || animation.headSequenceOrPartAddress != NXBasics::IntegerRange::UInt32Maximum || count != originalCount)
				return;

			const auto sequence = SearchBobSequence(manager, "human_man_Warrior_spear_sleep");

			if (sequence < 0 || animation.bodySequenceOrPartCount != static_cast<std::uint32_t>(sequence))
				return;
			// Match every original frame so corrected or customized assets remain untouched.

			for (std::uint32_t index = 0; index < 35; ++index)
			{
				if (frames[index] != index || frames[originalCount - index - 1u] != index)
					return;
			}

			for (std::uint32_t index = 35; index < 43; ++index)
			{
				if (frames[index] != ((index - 35u) % 2u == 0 ? 36u : 35u))
				{

					return;
				}
			}

			std::memmove(frames + holdStart + missingFrames, frames + holdStart, (originalCount - holdStart) * sizeof(*frames));

			for (std::uint32_t index = holdStart; index < holdStart + missingFrames; ++index)
			{
				frames[index] = 36;
			}

			count = correctedCount;
		}

		template <std::size_t Count>
		bool Remaster_MatchesCompoundParts(const SE2StaticDataMapMoveableAnimationPart* parts, const SE2StaticDataMapMoveableAnimationPart (&original)[Count])
		{
			for (std::size_t index = 0; index < Count; ++index)
			{
				// The loader leaves the final word uninitialized for non-walking parts.

				if (std::memcmp(parts + index, original + index, offsetof(SE2StaticDataMapMoveableAnimationPart, walkEndOffsetY)) != 0)
					return false;

				if (original[index].type == NC2Runtime::MoveableAnimationPartType::Walk && parts[index].walkEndOffsetY != original[index].walkEndOffsetY)
					return false;
			}

			return true;
		}

		void Remaster_RepairVikingPotterBrickFire(const SE2StaticDataMapMoveableAnimationData& animation, SE2StaticDataMapMoveableAnimationPart* parts, std::uint32_t& count)
		{
			// Original brick production omits the fire used by its matching crockery/tile sequences.

			if (animation.tribe != 1 || animation.job != SwitchIds::HumanJob::Potter || animation.action != 52 || animation.inHouseSubId != 0 || animation.animationMode != NC2Runtime::MoveableAnimationMode::Compound || count != 7)
				return;

			constexpr SE2StaticDataMapMoveableAnimationPart original[7] = {
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 0, 0, { 2 }, { 2 }, { -57 }, 22 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 0, 10, { 7 }, { 2 }, { -59 }, 35 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 10, 20, { 0 }, { 2 }, { -31 }, 40 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 20, 75, { 56 }, { 1 }, { 5 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 75, 85, { 22 }, { 0 }, { 6 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 85, 95, { 3 }, { 24 }, { -59 }, 35 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 95, 100, { 5 }, { 24 }, { -57 }, 22 }
			};
			static_assert(8 <= NC2Runtime::MoveableAnimationLimits::CompoundParts);

			if (!Remaster_MatchesCompoundParts(parts, original))
				return;

			const auto fire = E2StaticDataLandscapesManager_GetLandscapeId("fx fire small");

			if (fire < 0)
			{

				return;
			}

			std::memmove(parts + 1, parts, sizeof(original));
			parts[0] = { NC2Runtime::MoveableAnimationPartType::LandscapeOverlayUnsigned, 1, 99, { fire }, { 93 }, { 36 }, 0 };
			++count;
		}

		void Remaster_RepairVikingBakerCakeCarry(const SE2StaticDataMapMoveableAnimationData& animation, SE2StaticDataMapMoveableAnimationPart* parts, std::uint32_t count)
		{
			if (animation.tribe != 1 || animation.job != SwitchIds::HumanJob::Baker || animation.action != 48 || animation.inHouseSubId != 0 || animation.animationMode != NC2Runtime::MoveableAnimationMode::Compound || count != 10)
				return;

			constexpr SE2StaticDataMapMoveableAnimationPart original[10] = {
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 0, 0, { 3 }, { 11 }, { 75 }, 29 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 0, 2, { 2 }, { 11 }, { 64 }, 44 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 2, 9, { 3 }, { 11 }, { 18 }, 25 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 9, 25, { 47 }, { 1 }, { 5 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 25, 30, { 3 }, { 20 }, { -42 }, 16 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 30, 60, { 47 }, { 2 }, { 5 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 60, 70, { 5 }, { 0 }, { 5 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned, 70, 80, { 47 }, { 2 }, { 5 }, 0 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 80, 95, { 0 }, { 20 }, { 64 }, 44 },
				{ NC2Runtime::MoveableAnimationPartType::WalkUnsigned, 95, 100, { 5 }, { 19 }, { 75 }, 29 }
			};

			if (!Remaster_MatchesCompoundParts(parts, original))
			{

				return;
			}

			parts[9].walkGood = 20; // Keep carrying cake instead of switching to bread for the final step.
		}

		template <class T>
		void InsertSorted(void* manager, std::uint32_t base, T* record, bool sortByWalkSpeed)
		{
			const auto tribeOffset = record->tribe * NC2Layout::RecordSize::FastMoveableTribeUnsigned;
			Write(manager, base + tribeOffset, Read<std::uint32_t>(manager, base + tribeOffset) + 1u);
			const auto slot = base + tribeOffset + NC2Layout::FastMoveableTribeOffset::JobsUnsigned + record->job * NC2Layout::RecordSize::FastMoveableJobUnsigned;
			Write(manager, slot, Read<std::uint32_t>(manager, slot) + 1u);
			auto* list = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + slot + NC2Layout::FastMoveableJobOffset::Entries);

			if (list->GetCount() == 0)
			{
				list->InsertAtStart(record);

				return;
			}

			NXBasics::CListBase::Iterator iterator(*list);
			void* position = iterator.First();
			const auto key = Read<std::uint32_t>(record, NC2Layout::StaticMoveableSortOffset::ActionOrGood);

			while (position != nullptr)
			{
				const auto previous = Read<std::uint32_t>(position, NC2Layout::StaticMoveableSortOffset::ActionOrGood);

				if (NXBasics::Runtime_TimeSigned32(previous) > NXBasics::Runtime_TimeSigned32(key) || (sortByWalkSpeed && previous == key && Read<std::uint32_t>(position, NC2Layout::StaticMoveableSortOffset::WalkSpeed) > Read<std::uint32_t>(record, NC2Layout::StaticMoveableSortOffset::WalkSpeed)))
					break;

				position = iterator.Next();
			}

			if (position != nullptr)
			{
				list->InsertBefore(position, record);
			}
			else
			{
				list->InsertAtEnd(record);
			}
		}

		void LoadJobGraphics(NXBasics::CIniFileReader& reader, std::uint32_t countOffset, const NXBasics::SToken* tokens)
		{
			if (!reader.IsLoaded())
				return;

			const auto count = reader.GetNumberOfGroups("jobgraphics");
			Write(gStaticMapMoveableData, countOffset, count);

			if (count == 0)
			{

				return;
			}

			auto* records = AllocateCleared<std::byte>(count * NC2Layout::RecordSize::StaticAnimalVehicleGraphicsUnsigned);
			Write(gStaticMapMoveableData, countOffset + NC2Layout::CountedArrayOffset::Entries, records);
			auto* record = records;

			for (char* group = reader.GetFirstGroup("jobgraphics"); group != nullptr; group = reader.GetNextGroup("jobgraphics"), record += NC2Layout::RecordSize::StaticAnimalVehicleGraphics)
			{
				while (reader.GetNextCommandLineInGroup() != nullptr)
					switch ((tokens == gAnimalJobTokens ? GetLazyTokenList<AnimalJobTokenList_AtExit>(tokens) : GetLazyTokenList<VehicleJobTokenList_AtExit>(tokens)).FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::AnimalVehicleJobToken::LogicTribe:
							Write<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Tribe, static_cast<std::uint32_t>(reader.GetInt()));
							break;
						case SwitchIds::AnimalVehicleJobToken::LogicJob:
						case SwitchIds::AnimalVehicleJobToken::LogicVehicle:
							Write<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::Type, static_cast<std::uint32_t>(reader.GetInt()));
							break;
						case SwitchIds::AnimalVehicleJobToken::Gfxbobmanagerbody:
							Write(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::BodyBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
							Write(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::ShadowBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
							break;
						case SwitchIds::AnimalVehicleJobToken::GfxPaletteBody:
						{
							const auto used = Read<std::uint32_t>(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount);

							if (used == 5)
								break;

							Write(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteIds + used * NC2Layout::RecordSize::MoveablePaletteIdUnsigned, E2StaticDataPalettesManager_GetPalette256Id(reader.GetString()));
							Write(record, NC2Layout::StaticAnimalVehicleGraphicsOffset::PaletteCount, used + 1u);
							break;
						}
					}
			}
		}
	}

	// Original function: FUN_0048bd66; Windows layout and parser order are authoritative.
	void* __fastcall E2StaticDataMapMoveablesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;
		// Original constructors 0048CC90/0048CCB9 precede the complete manager clear.
		FastAnimationDataAccess_Construct(static_cast<std::byte*>(manager) + NC2Layout::StaticMoveableManagerOffset::AnimationAccess);
		FastWalkDataAccess_Construct(static_cast<std::byte*>(manager) + NC2Layout::StaticMoveableManagerOffset::WalkAccess);
		gE2StaticDataMapMoveablesManager = manager;
		std::memset(manager, 0, NC2Layout::RecordSize::StaticMoveableManager);
		std::memset(gStaticMapMoveableData, 0, NC2Layout::RecordSize::StaticMoveableData);
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\mapmoveableanimations\\animations.ini", nullptr, false, nullptr, nullptr);

			if (reader.IsLoaded())
			{
				const auto libraryCount = reader.GetNumberOfGroups("bobseq");
				Write(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraryCount, libraryCount);

				if (libraryCount != 0)
				{
					auto* libraries = AllocateCleared<SSequenceLibrary>(libraryCount);
					Write(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraries, libraries);
					auto* library = libraries;

					for (char* group = reader.GetFirstGroup("bobseq"); group != nullptr; group = reader.GetNextGroup("bobseq"), ++library)
					{
						library->count = reader.GetNumberOfCommandsInGroup("seq");
						library->sequences = AllocateCleared<SSequenceEntry>(library->count);
						reader.SetCurrentLineToGroupStart();
						std::uint32_t sequence = 0;

						while (reader.GetNextCommandLineInGroup() != nullptr)
							switch (GetLazyTokenList<SequenceTokenList_AtExit>(gSequenceTokens).FindToken(reader.GetCurrentName()))
							{
								case SwitchIds::AnimationSequenceToken::Imagelib:
									std::strncpy(library->imageLibrary, reader.GetString(), 99);
									break;
								case SwitchIds::AnimationSequenceToken::Shadowlib:
									std::strncpy(library->shadowLibrary, reader.GetString(), 99);
									break;
								case SwitchIds::AnimationSequenceToken::Sequence:
									std::strncpy(library->sequences[sequence].name, reader.GetString(), 99);
									library->sequences[sequence++].firstBob = reader.GetInt();
									break;
							}
					}
				}

				const auto animationCount = reader.GetNumberOfGroups("gfxanimatomic");
				Write(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimationCount, animationCount);

				if (animationCount != 0)
				{
					auto* animations = AllocateCleared<SE2StaticDataMapMoveableAnimationData>(animationCount);
					Write(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations, animations);
					auto* animation = animations;

					for (char* group = reader.GetFirstGroup("gfxanimatomic"); group != nullptr; group = reader.GetNextGroup("gfxanimatomic"), ++animation)
					{
						animation->animationMode = NC2Runtime::MoveableAnimationMode::Unspecified;
						std::uint32_t frames[NC2Runtime::MoveableAnimationLimits::FramesPerList], directionFrames[8][NC2Runtime::MoveableAnimationLimits::FramesPerList], frameCount = 0, directionCounts[8]{ };
						SE2StaticDataMapMoveableAnimationPart parts[NC2Runtime::MoveableAnimationLimits::CompoundParts];
						std::uint32_t partCount = 0;
						bool gotFrames = false, gotDirectionFrames = false;

						while (reader.GetNextCommandLineInGroup() != nullptr)
						{
							const auto token = GetLazyTokenList<AnimationTokenList_AtExit>(gAnimationTokens).FindToken(reader.GetCurrentName());

							switch (token)
							{
								case SwitchIds::HumanAnimationToken::LogicTribe:
									if (animation->tribe == 0)
										animation->tribe = static_cast<std::uint32_t>(reader.GetInt());

									break;
								case SwitchIds::HumanAnimationToken::LogicJob:
									animation->job = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanAnimationToken::LogicAtomicAction:
									if (animation->tribe != 0 && animation->job != 0)
										animation->action = static_cast<std::uint32_t>(reader.GetInt());

									break;
								case SwitchIds::HumanAnimationToken::LogicInHouseAtomicSubId:
									animation->inHouseSubId = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanAnimationToken::Gfxanimmode:
									animation->animationMode = reader.GetInt();

									if (animation->animationMode >= NC2Runtime::MoveableAnimationMode::ProgressFrames && animation->animationMode <= NC2Runtime::MoveableAnimationMode::CyclicFrames)
									{
										animation->bodySequenceOrPartCount = 0;
										animation->headSequenceOrPartAddress = NXBasics::IntegerRange::UInt32Maximum;
									}

									break;
								case SwitchIds::HumanAnimationToken::Gfxbobseqbody:
								case SwitchIds::HumanAnimationToken::Gfxbobseqhead:
									if (animation->animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animation->animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames)
									{
										const auto sequence = static_cast<std::uint32_t>(SearchBobSequence(manager, reader.GetString()));

										if (token == SwitchIds::HumanAnimationToken::Gfxbobseqbody)
										{
											animation->bodySequenceOrPartCount = sequence;
										}
										else
										{
											animation->headSequenceOrPartAddress = sequence;
										}
									}

									break;
								case SwitchIds::HumanAnimationToken::GfxAnimFrameList:
									if ((animation->animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animation->animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames) && !gotDirectionFrames)
									{
										while (reader.GetTypeOfNextParameter() == 3 && frameCount != NC2Runtime::MoveableAnimationLimits::FramesPerList)
											frames[frameCount++] = static_cast<std::uint32_t>(reader.GetInt());

										gotFrames = true;
									}

									break;
								case SwitchIds::HumanAnimationToken::Gfxanimframelistdir:
									if ((animation->animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animation->animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames) && !gotFrames)
									{
										const auto direction = static_cast<std::uint32_t>(reader.GetInt());

										while (reader.GetTypeOfNextParameter() == 3 && directionCounts[direction] != NC2Runtime::MoveableAnimationLimits::FramesPerList)
											directionFrames[direction][directionCounts[direction]++] = static_cast<std::uint32_t>(reader.GetInt());

										gotDirectionFrames = true;
									}

									break;
								case SwitchIds::HumanAnimationToken::GfxInHouseWalk:
								case SwitchIds::HumanAnimationToken::Gfxinhouseanim:
								case SwitchIds::HumanAnimationToken::Gfxinhouseoverlaybob:
								case SwitchIds::HumanAnimationToken::GfxInHouseOverlayLandscape:
									if (animation->animationMode == NC2Runtime::MoveableAnimationMode::Compound && partCount != NC2Runtime::MoveableAnimationLimits::CompoundParts)
									{
										auto& part = parts[partCount];

										if (token == SwitchIds::HumanAnimationToken::GfxInHouseWalk)
										{
											part.type = NC2Runtime::MoveableAnimationPartType::WalkUnsigned;
										}
										else if (token == SwitchIds::HumanAnimationToken::Gfxinhouseanim)
										{
											part.type = NC2Runtime::MoveableAnimationPartType::AtomicAnimationUnsigned;
										}
										else if (token == SwitchIds::HumanAnimationToken::Gfxinhouseoverlaybob)
										{
											part.type = NC2Runtime::MoveableAnimationPartType::HouseBobOverlayUnsigned;
										}
										else
										{
											part.type = NC2Runtime::MoveableAnimationPartType::LandscapeOverlayUnsigned;
										}

										if (part.type == NC2Runtime::MoveableAnimationPartType::Walk)
										{
											part.walkDirection = reader.GetInt();
											part.walkGood = reader.GetInt();
											part.walkEndOffsetX = reader.GetInt();
											part.walkEndOffsetY = reader.GetInt();
										}
										else if (part.type == NC2Runtime::MoveableAnimationPartType::AtomicAnimation)
										{
											part.atomicAction = reader.GetInt();
											part.atomicSubId = reader.GetInt();
											part.atomicDirection = reader.GetInt();
										}
										else
										{
											if (part.type == NC2Runtime::MoveableAnimationPartType::LandscapeOverlay)
											{
												part.overlayLandscapeId = E2StaticDataLandscapesManager_GetLandscapeId(reader.GetString());

												if (part.overlayLandscapeId == -1)
													break;
											}
											else
											{
												part.overlayBobId = reader.GetInt();
											}

											part.overlayOffsetX = reader.GetInt();
											part.overlayOffsetY = reader.GetInt();
										}

										part.startPercent = static_cast<std::uint32_t>(reader.GetInt());
										part.endPercent = static_cast<std::uint32_t>(reader.GetInt());
										++partCount;
									}

									break;
							}
						}

						if (animation->animationMode == NC2Runtime::MoveableAnimationMode::ProgressFrames || animation->animationMode == NC2Runtime::MoveableAnimationMode::CyclicFrames)
						{
							Remaster_RepairVikingWoodenSpearmanSleepFrames(manager, *animation, frames, frameCount);
							CopyFrames(animation->frames, frames, frameCount);

							for (std::uint32_t direction = 0; direction < 8; ++direction)
							{
								CopyFrames(animation->directionFrames[direction], directionFrames[direction], directionCounts[direction]);
							}
						}

						if (animation->animationMode == NC2Runtime::MoveableAnimationMode::Compound && partCount != 0)
						{
							Remaster_RepairVikingPotterBrickFire(*animation, parts, partCount);
							Remaster_RepairVikingBakerCakeCarry(*animation, parts, partCount);
							animation->bodySequenceOrPartCount = partCount;
							auto* data = Allocate<SE2StaticDataMapMoveableAnimationPart>(partCount);
							std::memcpy(data, parts, partCount * NC2Layout::RecordSize::MoveableAnimationPartUnsigned);
							animation->headSequenceOrPartAddress = reinterpret_cast<std::uintptr_t>(data);
						}

						if (animation->inHouseSubId == 0)
							InsertSorted(manager, NC2Layout::StaticMoveableManagerOffset::AnimationAccess, animation, false);
					}
				}

				const auto walkCount = reader.GetNumberOfGroups("gfxwalkatomic");
				Write(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::WalkCount, walkCount);

				if (walkCount != 0)
				{
					auto* walks = AllocateCleared<SE2StaticDataMapMoveableWalkData>(walkCount);
					Write(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Walks, walks);
					auto* walk = walks;

					for (char* group = reader.GetFirstGroup("gfxwalkatomic"); group != nullptr; group = reader.GetNextGroup("gfxwalkatomic"), ++walk)
					{
						walk->walkSpeed = 100000;
						walk->bodySequence = 0;
						walk->headSequence = -1;
						std::uint32_t walkFrames[8][NC2Runtime::FastMoveableAccessLimits::FramesPerWalkList], turnFrames[8][NC2Runtime::FastMoveableAccessLimits::FramesPerWalkList], walkCounts[8]{ }, turnCounts[8]{ };

						while (reader.GetNextCommandLineInGroup() != nullptr)
						{
							const auto token = GetLazyTokenList<WalkTokenList_AtExit>(gWalkTokens).FindToken(reader.GetCurrentName());

							switch (token)
							{
								case SwitchIds::HumanWalkAnimationToken::LogicTribe:
									walk->tribe = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanWalkAnimationToken::LogicJob:
									walk->job = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanWalkAnimationToken::LogicGoodType:
									walk->good = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanWalkAnimationToken::LogicWalkSpeed:
									walk->walkSpeed = static_cast<std::uint32_t>(reader.GetInt());
									break;
								case SwitchIds::HumanWalkAnimationToken::Gfxbobseqbody:
									walk->bodySequence = SearchBobSequence(manager, reader.GetString());
									break;
								case SwitchIds::HumanWalkAnimationToken::Gfxbobseqhead:
									walk->headSequence = SearchBobSequence(manager, reader.GetString());
									break;
								case SwitchIds::HumanWalkAnimationToken::GfxWalkFrameList:
								case SwitchIds::HumanWalkAnimationToken::Gfxturnframelist:
								{
									const auto direction = static_cast<std::uint32_t>(reader.GetInt());
									auto& count = token == SwitchIds::HumanAnimationToken::GfxAnimFrameList ? walkCounts[direction] : turnCounts[direction];
									auto* output = token == SwitchIds::HumanAnimationToken::GfxAnimFrameList ? walkFrames[direction] : turnFrames[direction];

									while (reader.GetTypeOfNextParameter() == 3 && count != NC2Runtime::FastMoveableAccessLimits::FramesPerWalkList)
										output[count++] = static_cast<std::uint32_t>(reader.GetInt());

									break;
								}
							}
						}

						for (std::uint32_t direction = 0; direction < 8; ++direction)
						{
							CopyFrames(walk->walkFrames[direction], walkFrames[direction], walkCounts[direction]);
							CopyFrames(walk->turnFrames[direction], turnFrames[direction], turnCounts[direction]);
						}

						InsertSorted(manager, NC2Layout::StaticMoveableManagerOffset::WalkAccess, walk, true);
					}
				}

				const auto* libraries = Read<SSequenceLibrary*>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraries);

				for (std::uint32_t library = 0; library < Read<std::uint32_t>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraryCount); ++library)
				{
					NXBasics::Runtime_Free(libraries[library].sequences);
				}

				NXBasics::Runtime_Free(Read<void*>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraries));
				Write<std::uint32_t>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraryCount, 0);
				Write<void*>(manager, NC2Layout::StaticMoveableManagerOffset::SequenceLibraries, nullptr);
			}
		}
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\animals\\jobgraphics.ini", nullptr, false, nullptr, nullptr);
			LoadJobGraphics(reader, NC2Layout::StaticMoveableDataOffset::AnimalGraphicsCount, gAnimalJobTokens);
		}
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\vehicles\\jobgraphics.ini", nullptr, false, nullptr, nullptr);
			LoadJobGraphics(reader, NC2Layout::StaticMoveableDataOffset::VehicleGraphicsCount, gVehicleJobTokens);
		}

		return manager;
	}

	// Original function: FUN_0048cf18; independent random selection among at most ten equal actions.
	const SE2StaticDataMapMoveableAnimationData* StaticMapMoveables_GetAnimationDataPtr(void* manager, std::uint32_t tribe, std::uint32_t job, std::uint32_t action, std::uint32_t inHouseSubId)
	{
		if (inHouseSubId != 0)
		{
			const auto* entries = NC2Lifetime::Read<SE2StaticDataMapMoveableAnimationData*>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::Animations);

			for (std::uint32_t index = 0; index < NC2Lifetime::Read<std::uint32_t>(gStaticMapMoveableData, NC2Layout::StaticMoveableDataOffset::AnimationCount); ++index)
			{
				if (entries[index].tribe == tribe && entries[index].job == job && entries[index].action == action && entries[index].inHouseSubId == inHouseSubId)
					return entries + index;
			}

			return nullptr;
		}

		auto* list = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + tribe * NC2Layout::RecordSize::FastMoveableTribeUnsigned + job * NC2Layout::RecordSize::FastMoveableJobUnsigned + NC2Layout::StaticMoveableManagerOffset::FirstAnimationList);
		NXBasics::CListBase::Iterator iterator(*list);
		auto* record = static_cast<const SE2StaticDataMapMoveableAnimationData*>(iterator.First());

		while (record != nullptr && record->action != action)
			record = static_cast<const SE2StaticDataMapMoveableAnimationData*>(iterator.Next());

		if (record == nullptr)
			return nullptr;

		const SE2StaticDataMapMoveableAnimationData* choices[NC2Runtime::FastMoveableAccessLimits::RandomAnimationChoices]{ record };
		auto* next = static_cast<const SE2StaticDataMapMoveableAnimationData*>(iterator.Next());

		if (next == nullptr || next->action != action)
			return record;

		std::uint32_t count = 1;
		do
		{
			choices[count++] = next;

			if (count == NC2Runtime::FastMoveableAccessLimits::RandomAnimationChoices)
				break;

			next = static_cast<const SE2StaticDataMapMoveableAnimationData*>(iterator.Next());
		} while (next != nullptr && next->action == action);

		if (!gAnimationSelectionRandomInitialized)
		{
			gAnimationSelectionRandomInitialized = true;
			gAnimationSelectionRandom.currentState = NC2Runtime::RandomSeed::DisplayEffects;
			gAnimationSelectionRandom.seed = NC2Runtime::RandomSeed::DisplayEffects;
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(AnimationSelectionRandom_AtExit));
		}

		return choices[NC2Logic::Random_Get(gAnimationSelectionRandom) % count];
	}

	// Original function: FUN_0048d027; first matching good with sufficient walk speed.
	const SE2StaticDataMapMoveableWalkData* StaticMapMoveables_GetWalkDataPtr(void* manager, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, std::uint32_t walkSpeed)
	{
		auto* list = reinterpret_cast<NXBasics::CListBase*>(static_cast<std::byte*>(manager) + tribe * NC2Layout::RecordSize::FastMoveableTribeUnsigned + job * NC2Layout::RecordSize::FastMoveableJobUnsigned + NC2Layout::StaticMoveableManagerOffset::FirstWalkList);
		NXBasics::CListBase::Iterator iterator(*list);
		auto* record = static_cast<const SE2StaticDataMapMoveableWalkData*>(iterator.First());

		while (record != nullptr && record->good != good)
			record = static_cast<const SE2StaticDataMapMoveableWalkData*>(iterator.Next());

		while (record != nullptr && record->good == good)
		{
			if (walkSpeed <= record->walkSpeed)
				return record;

			record = static_cast<const SE2StaticDataMapMoveableWalkData*>(iterator.Next());
		}

		return nullptr;
	}
}