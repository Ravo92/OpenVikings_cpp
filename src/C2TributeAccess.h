#pragma once

#include "XNumericConstants.h"
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct STributeData
	{
		struct SGoodAmount
		{
			std::uint32_t good;
			std::uint32_t amount;
		};

		std::uint8_t allocated;
		std::uint8_t paid;
		std::byte unknown002[2];
		std::uint32_t goodCount;
		std::uint32_t sourcePlayer;
		std::uint32_t targetPlayer;
		std::uint32_t descriptionStringId;
		SGoodAmount goods[5];
	};

	static_assert(sizeof(STributeData) == 60);
	static_assert(offsetof(STributeData, goodCount) == 4);
	static_assert(offsetof(STributeData, sourcePlayer) == 8);
	static_assert(offsetof(STributeData, targetPlayer) == 0x0C);
	static_assert(offsetof(STributeData, descriptionStringId) == 0x10);
	static_assert(offsetof(STributeData, goods) == 0x14);
	void TributeManager_CreateTribute(void* manager, std::uint32_t sourcePlayer, std::uint32_t targetPlayer, std::uint32_t slot);
	void TributeManager_AddTribute(void* manager, std::uint32_t slot, std::uint32_t good, std::uint32_t amount);
	void TributeData_Init(STributeData* tribute, std::uint32_t sourcePlayer, std::uint32_t targetPlayer);
	bool TributeData_AddTribute(STributeData* tribute, std::uint32_t good, std::uint32_t amount);
	std::uint32_t TributeManager_CountGoodAmountForTribute(std::uint32_t player, std::uint32_t good);
	bool TributeData_Possible(const void* tribute);
	bool TributeData_GetTributeFromHouse(void* tribute, std::int32_t id);
	bool __fastcall TributeData_Pay(void* tribute, void* unusedEdx);

	class CTributeIterator
	{
	  public:
		CTributeIterator(std::uint32_t source, std::uint32_t target) : mIndex(NXBasics::IntegerRange::UInt32Maximum), mSource(source), mTarget(target), mCurrent(nullptr)
		{
		}

		void operator++(int);

		void* GetCurrent() const
		{

			return mCurrent;
		}

		std::uint32_t GetIndex() const
		{

			return mIndex;
		}

	  private:
		std::uint32_t mIndex, mSource, mTarget;
		void* mCurrent;
	};

	static_assert(sizeof(CTributeIterator) == (sizeof(void*) == 8 ? 24 : 16));
}