#pragma once

#include "XBasicTypes.h"
#include "XBTokenList.h"

namespace NC2Logic
{
	struct SStaticVehicleTypeData
	{
		char name[30];
		std::byte unknown1E[2];
		std::int32_t logicSize;
		std::int32_t stockSlots;
		std::uint32_t goodTypeCount;
		std::uint8_t allowedGoodTypes[66];
		std::byte padding6E[2];
		std::int32_t stockVectorX;
		std::int32_t stockVectorY;
		std::int32_t goodTypeIndices[66];
		std::int32_t passengerSlots;
		std::uint8_t allowedPassengerTypes[56];
		std::int32_t commanderType;
		std::int32_t passengerVectorX;
		std::int32_t passengerVectorY;
		std::int32_t vehicleSlots;
		std::int32_t draggingAnimalTribe;
		std::int32_t transformVehicleType;
		NXBasics::SColorRGB debugColor;
		std::int32_t debugRadius;
	};

	class CStaticVehicleTypeDataManager
	{
	  public:
		CStaticVehicleTypeDataManager();
		~CStaticVehicleTypeDataManager();
		CStaticVehicleTypeDataManager(const CStaticVehicleTypeDataManager&) = delete;
		CStaticVehicleTypeDataManager& operator=(const CStaticVehicleTypeDataManager&) = delete;

	  private:
		std::uint8_t mObjectByte;
	};

	// Complete original static region: 00511128..005111B7.
	struct SStaticVehicleTypeStorage
	{
		std::uint8_t tokenInitFlags;
		std::byte padding01[3];
		alignas(NXBasics::CTokenList) std::byte tokenList[sizeof(NXBasics::CTokenList)];
		SStaticVehicleTypeData* records;
		CStaticVehicleTypeDataManager* owner;
		NXBasics::SToken tokens[16];
	};

	extern SStaticVehicleTypeStorage gStaticVehicleTypeStorage;

	static_assert(sizeof(SStaticVehicleTypeData) == 476);
	static_assert(offsetof(SStaticVehicleTypeData, logicSize) == 0x20);
	static_assert(offsetof(SStaticVehicleTypeData, stockSlots) == 0x24);
	static_assert(offsetof(SStaticVehicleTypeData, goodTypeCount) == 0x28);
	static_assert(offsetof(SStaticVehicleTypeData, allowedGoodTypes) == 0x2C);
	static_assert(offsetof(SStaticVehicleTypeData, stockVectorX) == 0x70);
	static_assert(offsetof(SStaticVehicleTypeData, stockVectorY) == 0x74);
	static_assert(offsetof(SStaticVehicleTypeData, goodTypeIndices) == 0x78);
	static_assert(offsetof(SStaticVehicleTypeData, passengerSlots) == 0x180);
	static_assert(offsetof(SStaticVehicleTypeData, allowedPassengerTypes) == 0x184);
	static_assert(offsetof(SStaticVehicleTypeData, commanderType) == 0x1BC);
	static_assert(offsetof(SStaticVehicleTypeData, passengerVectorX) == 0x1C0);
	static_assert(offsetof(SStaticVehicleTypeData, passengerVectorY) == 0x1C4);
	static_assert(offsetof(SStaticVehicleTypeData, vehicleSlots) == 0x1C8);
	static_assert(offsetof(SStaticVehicleTypeData, draggingAnimalTribe) == 0x1CC);
	static_assert(offsetof(SStaticVehicleTypeData, transformVehicleType) == 0x1D0);
	static_assert(offsetof(SStaticVehicleTypeData, debugColor) == 0x1D4);
	static_assert(offsetof(SStaticVehicleTypeData, debugRadius) == 0x1D8);
	static_assert(sizeof(CStaticVehicleTypeDataManager) == 1);
	static_assert(sizeof(SStaticVehicleTypeStorage) == (sizeof(void*) == 8 ? 288 : 144));
	static_assert(offsetof(SStaticVehicleTypeStorage, tokenList) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticVehicleTypeStorage, records) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticVehicleTypeStorage, owner) == (sizeof(void*) == 8 ? 24 : 0x0C));
	static_assert(offsetof(SStaticVehicleTypeStorage, tokens) == (sizeof(void*) == 8 ? 32 : 0x10));
}