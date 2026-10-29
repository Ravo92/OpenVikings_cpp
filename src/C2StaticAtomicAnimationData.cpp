#include "C2LifetimeGlobals.h"
#include "C2StaticAtomicAnimationData.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>

namespace NC2Logic
{
	SStaticAtomicAnimationStorage gStaticAtomicAnimationStorage{ };
	SStaticAtomicAnimationData*& gStaticAtomicAnimationData = gStaticAtomicAnimationStorage.records;
	CStaticAtomicAnimationDataManager*& gStaticAtomicAnimationDataManager = gStaticAtomicAnimationStorage.owner;

	namespace
	{
		// Original read-only token region: 004F3768..004F37A7.
		const NXBasics::SToken gAtomicAnimationTokens[8] = {
			{ "name", SwitchIds::AtomicAnimationToken::Name },
			{ "length", SwitchIds::AtomicAnimationToken::Length },
			{ "interruptable", SwitchIds::AtomicAnimationToken::Interruptable },
			{ "event", SwitchIds::AtomicAnimationToken::Event },
			{ "eventx", SwitchIds::AtomicAnimationToken::EventX },
			{ "startdirection", SwitchIds::AtomicAnimationToken::StartDirection },
			{ "enddirection", SwitchIds::AtomicAnimationToken::EndDirection },
			{ nullptr, 0 }
		};
		static_assert(sizeof(gAtomicAnimationTokens) == 64);

		// Original function: FUN_00414408, an empty token-list exit callback.
		// The runtime callback interface requires an int; its return is ignored.
		int __cdecl AtomicAnimationTokens_OnExit()
		{

			return 0;
		}

		NXBasics::CTokenList& AtomicAnimationTokens()
		{
			auto& storage = gStaticAtomicAnimationStorage;

			if ((storage.tokenInitFlags & 1) == 0)
			{
				storage.tokenInitFlags |= 1;
				::new (storage.tokenList) NXBasics::CTokenList(gAtomicAnimationTokens);
				NXBasics::Runtime_AtExit(AtomicAnimationTokens_OnExit);
			}

			return *std::launder(reinterpret_cast<NXBasics::CTokenList*>(storage.tokenList));
		}

		void Remaster_RepairVikingBakerCakeVisibility(SStaticAtomicAnimationData& animation)
		{
			// Original cake production omits the house exit/entry events present in bread production.
			constexpr SStaticAtomicAnimationEventData original[6] = { { 77, 34, 14 }, { 80, 1, -100 }, { 80, 1, -100 }, { 80, 2, -100 }, { 143, 34, 15 }, { 199, 21, 0 } };
			static_assert(sizeof(animation.events) / sizeof(animation.events[0]) >= 8);

			if (std::strcmp(animation.name, "viking_baker_produce_candy") != 0 || animation.length != 200 || animation.eventCount != 6 || animation.startDirection != -1 || animation.endDirection != -1 || animation.interruptable != 0 || std::memcmp(animation.events, original, sizeof(original)) != 0)
			{

				return;
			}

			std::memmove(animation.events + 1, animation.events, sizeof(original));
			animation.events[0] = { 0, SwitchIds::AtomicAnimationEvent::LeaveHouse, 0 };
			animation.events[7] = { 199, SwitchIds::AtomicAnimationEvent::EnterHouse, 0 };
			animation.eventCount = 8;
		}
	}

	// Original function: FUN_0041418E.
	CStaticAtomicAnimationDataManager::CStaticAtomicAnimationDataManager()
	{
		gStaticAtomicAnimationDataManager = this;
		std::memset(this, 0, 1);
		gStaticAtomicAnimationStorage.count = 0;
		gStaticAtomicAnimationData = nullptr;
		NXBasics::CIniFileReader reader("data\\logic\\atomicanimations\\atomicanimations.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return;

		gStaticAtomicAnimationStorage.count = reader.GetNumberOfGroups("atomicanimation");

		if (gStaticAtomicAnimationStorage.count == 0)
		{

			return;
		}

		const std::uint32_t bytes = gStaticAtomicAnimationStorage.count * 0x138u;
		gStaticAtomicAnimationData = static_cast<SStaticAtomicAnimationData*>(NXBasics::Runtime_AllocateWithNewHandler(bytes, 1));
		// Windows clears the allocation unconditionally, retaining DWORD size wrap.
		std::memset(gStaticAtomicAnimationData, 0, gStaticAtomicAnimationStorage.count * 312u);
		auto* record = gStaticAtomicAnimationData;

		for (char* group = reader.GetFirstGroup("atomicanimation"); group != nullptr; ++record, group = reader.GetNextGroup("atomicanimation"))
		{
			record->startDirection = -1;
			record->endDirection = -1;

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				const auto token = AtomicAnimationTokens().FindToken(reader.GetCurrentName());

				switch (token)
				{
					case SwitchIds::AtomicAnimationToken::Name:
						std::strncpy(record->name, reader.GetString(), 49);
						break;
					case SwitchIds::AtomicAnimationToken::Length:
						record->length = reader.GetInt();
						break;
					case SwitchIds::AtomicAnimationToken::Interruptable:
						record->interruptable = reader.GetInt() != 0;
						break;
					case SwitchIds::AtomicAnimationToken::StartDirection:
						record->startDirection = reader.GetInt();
						break;
					case SwitchIds::AtomicAnimationToken::EndDirection:
						record->endDirection = reader.GetInt();
						break;
					case SwitchIds::AtomicAnimationToken::Event:
					case SwitchIds::AtomicAnimationToken::EventX:
					{
						if (record->eventCount == 20)
							break;

						std::int32_t time;

						if (token == SwitchIds::AtomicAnimationToken::EventX)
						{
							if (record->length == 0)
								break;

							const auto percentage = static_cast<std::uint32_t>(reader.GetInt());
							// Keep the low DWORD product followed by unsigned division.
							time = NXBasics::Runtime_TimeSigned32((static_cast<std::uint32_t>(record->length) * percentage) / 100u);
						}
						else
						{
							time = reader.GetInt();
						}

						record->events[record->eventCount].time = time;
						record->events[record->eventCount].type = reader.GetInt();
						record->events[record->eventCount].parameter = reader.GetInt();
						++record->eventCount;
						break;
					}
				}
			}

			Remaster_RepairVikingBakerCakeVisibility(*record);
		}
	}

	// Original function: FUN_00414409; moved from the lifetime module.
	CStaticAtomicAnimationDataManager::~CStaticAtomicAnimationDataManager()
	{
		NXBasics::Runtime_Free(gStaticAtomicAnimationData);
		gStaticAtomicAnimationData = nullptr;
		gStaticAtomicAnimationDataManager = nullptr;
	}

	// Original function: FUN_00414424.
	std::uint32_t CStaticAtomicAnimationDataManager::GetAtomicId(const char* name)
	{
		for (std::int32_t index = 0; index < NXBasics::Runtime_TimeSigned32(gStaticAtomicAnimationStorage.count); ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(gStaticAtomicAnimationData[index].name, name) == 0)
				return static_cast<std::uint32_t>(index);
		}

		return NXBasics::IntegerRange::UInt32Maximum;
	}
}