#pragma once

#include "XBasicTypes.h"
#include "XBTokenList.h"

namespace NC2Logic
{
	struct SStaticTrianglePatternTypeData
	{
		std::uint8_t humanCanWalkOn;
		std::uint8_t houseCanBeBuiltOn;
		std::uint8_t bioCanGrowOn;
		std::uint8_t bioCanPlantOn;
		std::uint8_t isLand;
		std::uint8_t isWater;
		std::uint8_t isBorder;
		std::uint8_t unknown07;
		std::uint32_t moveResistance;
		char debugName[30];
		std::byte padding2A[2];
		NXBasics::SColorRGB debugColor;
	};

	class CStaticTrianglePatternTypeDataManager
	{
	  public:
		CStaticTrianglePatternTypeDataManager();
		~CStaticTrianglePatternTypeDataManager();
		CStaticTrianglePatternTypeDataManager(const CStaticTrianglePatternTypeDataManager&) = delete;
		CStaticTrianglePatternTypeDataManager& operator=(const CStaticTrianglePatternTypeDataManager&) = delete;

	  private:
		std::uint8_t mObjectByte;
	};

	// Complete original static region: 005112D8..00511347.
	struct SStaticTrianglePatternTypeStorage
	{
		std::uint8_t tokenInitFlags;
		std::byte padding01[3];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		SStaticTrianglePatternTypeData* records;
		CStaticTrianglePatternTypeDataManager* owner;
		NXBasics::SToken tokens[12];
	};

	extern SStaticTrianglePatternTypeStorage gStaticTrianglePatternTypeStorage;

	static_assert(sizeof(SStaticTrianglePatternTypeData) == 48);
	static_assert(offsetof(SStaticTrianglePatternTypeData, humanCanWalkOn) == 0);
	static_assert(offsetof(SStaticTrianglePatternTypeData, houseCanBeBuiltOn) == 1);
	static_assert(offsetof(SStaticTrianglePatternTypeData, bioCanGrowOn) == 2);
	static_assert(offsetof(SStaticTrianglePatternTypeData, bioCanPlantOn) == 3);
	static_assert(offsetof(SStaticTrianglePatternTypeData, isLand) == 4);
	static_assert(offsetof(SStaticTrianglePatternTypeData, isWater) == 5);
	static_assert(offsetof(SStaticTrianglePatternTypeData, isBorder) == 6);
	static_assert(offsetof(SStaticTrianglePatternTypeData, unknown07) == 7);
	static_assert(offsetof(SStaticTrianglePatternTypeData, moveResistance) == 8);
	static_assert(offsetof(SStaticTrianglePatternTypeData, debugName) == 0x0C);
	static_assert(offsetof(SStaticTrianglePatternTypeData, padding2A) == 0x2A);
	static_assert(offsetof(SStaticTrianglePatternTypeData, debugColor) == 0x2C);
	static_assert(sizeof(CStaticTrianglePatternTypeDataManager) == 1);
	static_assert(sizeof(SStaticTrianglePatternTypeStorage) == (sizeof(void*) == 8 ? 224 : 112));
	static_assert(offsetof(SStaticTrianglePatternTypeStorage, tokenList) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticTrianglePatternTypeStorage, records) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticTrianglePatternTypeStorage, owner) == (sizeof(void*) == 8 ? 24 : 0x0C));
	static_assert(offsetof(SStaticTrianglePatternTypeStorage, tokens) == (sizeof(void*) == 8 ? 32 : 0x10));
}