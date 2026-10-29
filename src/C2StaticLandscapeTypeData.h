#pragma once

#include "XBasicTypes.h"
#include "XBTokenList.h"

namespace NC2Logic
{
	struct SStaticLandscapeTransition
	{
		std::uint32_t type;
		std::uint32_t targetLandscapeType;
		std::uint32_t valencyOperation;
		std::uint32_t valencyValue;
		std::uint32_t resultValue; // Good ID for collection; duration ticks for automatic transitions.
	};

	struct SStaticLandscapeTypeData
	{
		char name[30];
		std::uint8_t allowedOnLand;
		std::uint8_t allowedOnWater;
		std::uint8_t allowedOnEverything;
		std::byte unknown21[3];
		std::uint32_t maximumValency;
		std::uint8_t playerIdAllowed;
		std::byte unknown29[3];
		std::uint32_t transitionMask;
		std::uint32_t transitionCount;
		SStaticLandscapeTransition transitions[5];
		NXBasics::SColorRGB debugColor;
	};

	class CStaticLandscapeTypeDataManager
	{
	public:
		CStaticLandscapeTypeDataManager();
		~CStaticLandscapeTypeDataManager();
		CStaticLandscapeTypeDataManager(const CStaticLandscapeTypeDataManager&) = delete;
		CStaticLandscapeTypeDataManager& operator=(const CStaticLandscapeTypeDataManager&) = delete;

	private:
		std::uint8_t mObjectByte;
	};

	// Complete original static region: 00511348..005113A7.
	struct SStaticLandscapeTypeStorage
	{
		SStaticLandscapeTypeData* records;
		std::uint8_t tokenInitFlags;
		std::byte padding05[sizeof(void*) - 1];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		CStaticLandscapeTypeDataManager* owner;
		NXBasics::SToken tokens[10];
	};
	extern SStaticLandscapeTypeStorage gStaticLandscapeTypeStorage;

	static_assert(sizeof(SStaticLandscapeTransition) == 20);
	static_assert(offsetof(SStaticLandscapeTransition, targetLandscapeType) == 4);
	static_assert(offsetof(SStaticLandscapeTransition, valencyOperation) == 8);
	static_assert(offsetof(SStaticLandscapeTransition, valencyValue) == 0x0C);
	static_assert(offsetof(SStaticLandscapeTransition, resultValue) == 0x10);
	static_assert(sizeof(SStaticLandscapeTypeData) == 156);
	static_assert(offsetof(SStaticLandscapeTypeData, allowedOnLand) == 0x1E);
	static_assert(offsetof(SStaticLandscapeTypeData, allowedOnWater) == 0x1F);
	static_assert(offsetof(SStaticLandscapeTypeData, allowedOnEverything) == 0x20);
	static_assert(offsetof(SStaticLandscapeTypeData, maximumValency) == 0x24);
	static_assert(offsetof(SStaticLandscapeTypeData, playerIdAllowed) == 0x28);
	static_assert(offsetof(SStaticLandscapeTypeData, transitionMask) == 0x2C);
	static_assert(offsetof(SStaticLandscapeTypeData, transitionCount) == 0x30);
	static_assert(offsetof(SStaticLandscapeTypeData, transitions) == 0x34);
	static_assert(offsetof(SStaticLandscapeTypeData, debugColor) == 0x98);
	static_assert(sizeof(CStaticLandscapeTypeDataManager) == 1);
	static_assert(sizeof(SStaticLandscapeTypeStorage) == (sizeof(void*) == 8 ? 192 : 96));
	static_assert(offsetof(SStaticLandscapeTypeStorage, tokenInitFlags) == sizeof(void*));
	static_assert(offsetof(SStaticLandscapeTypeStorage, tokenList) == 2 * sizeof(void*));
	static_assert(offsetof(SStaticLandscapeTypeStorage, owner) == 3 * sizeof(void*));
	static_assert(offsetof(SStaticLandscapeTypeStorage, tokens) == 4 * sizeof(void*));
}
