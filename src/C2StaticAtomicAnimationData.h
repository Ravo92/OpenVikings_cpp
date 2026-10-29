#pragma once

#include "XBTokenList.h"

namespace NC2Logic
{
	struct SStaticAtomicAnimationEventData
	{
		std::int32_t time;
		std::int32_t type;
		std::int32_t parameter;
	};

	struct SStaticAtomicAnimationData
	{
		char name[49];
		std::byte unknown31[3];
		std::int32_t length;
		std::uint8_t interruptable;
		std::byte padding39[3];
		std::uint32_t eventCount;
		SStaticAtomicAnimationEventData events[20];
		std::int32_t startDirection;
		std::int32_t endDirection;
	};

	class CStaticAtomicAnimationDataManager
	{
	  public:
		CStaticAtomicAnimationDataManager();
		~CStaticAtomicAnimationDataManager();
		CStaticAtomicAnimationDataManager(const CStaticAtomicAnimationDataManager&) = delete;
		CStaticAtomicAnimationDataManager& operator=(const CStaticAtomicAnimationDataManager&) = delete;
		static std::uint32_t GetAtomicId(const char* name);

	  private:
		std::uint8_t mObjectByte;
	};

	// Complete original writable region: 00511110..00511123.
	struct SStaticAtomicAnimationStorage
	{
		std::uint8_t tokenInitFlags;
		std::byte padding01[3];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		std::uint32_t count;
		SStaticAtomicAnimationData* records;
		CStaticAtomicAnimationDataManager* owner;
	};

	extern SStaticAtomicAnimationStorage gStaticAtomicAnimationStorage;

	static_assert(sizeof(SStaticAtomicAnimationEventData) == 12);
	static_assert(offsetof(SStaticAtomicAnimationEventData, type) == 4);
	static_assert(offsetof(SStaticAtomicAnimationEventData, parameter) == 8);
	static_assert(sizeof(SStaticAtomicAnimationData) == 312);
	static_assert(offsetof(SStaticAtomicAnimationData, length) == 0x34);
	static_assert(offsetof(SStaticAtomicAnimationData, interruptable) == 0x38);
	static_assert(offsetof(SStaticAtomicAnimationData, eventCount) == 0x3C);
	static_assert(offsetof(SStaticAtomicAnimationData, events) == 0x40);
	static_assert(offsetof(SStaticAtomicAnimationData, startDirection) == 0x130);
	static_assert(offsetof(SStaticAtomicAnimationData, endDirection) == 0x134);
	static_assert(sizeof(CStaticAtomicAnimationDataManager) == 1);
	static_assert(sizeof(SStaticAtomicAnimationStorage) == 20);
	static_assert(offsetof(SStaticAtomicAnimationStorage, tokenList) == 4);
	static_assert(offsetof(SStaticAtomicAnimationStorage, count) == 8);
	static_assert(offsetof(SStaticAtomicAnimationStorage, records) == 0x0C);
	static_assert(offsetof(SStaticAtomicAnimationStorage, owner) == 0x10);
}