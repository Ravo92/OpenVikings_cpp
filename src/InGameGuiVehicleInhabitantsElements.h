#pragma once
#include "InGameGuiStorage.h"
#include "XBasicTypes.h"

namespace NC2InGameGuiSelection
{
	// Physical Windows widgets share storage, but keep their distinct virtual tables.
	struct SAttachedVehicleSelectionSlotStorage
	{
		std::int32_t objectId;
		std::uint8_t commander;
		std::byte padding05[3];
		std::int32_t x, sortPriority;
	};

	struct SAttachedVehicleSelectionStorage
	{
		NXBaseGui::SBaseElementStorage baseElement;
		std::int32_t vehicleId;
		std::uint32_t vehicleRevision;
		std::int32_t count;
		SAttachedVehicleSelectionSlotStorage slots[30];
		std::int32_t baselineY, hitIndex;
		char tooltip[200];
		std::uint32_t selectCommand, actionCommand;
		void* receiver;
	};

	static_assert(sizeof(SAttachedVehicleSelectionStorage) == (sizeof(void*) == 8 ? 784 : 756));
	static_assert(sizeof(SAttachedVehicleSelectionSlotStorage) == (sizeof(void*) == 8 ? 16 : 16));

	void* SelectionVehicleAttachedHumansElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t vehicleId);
	void* SelectionVehicleAttachedVehiclesElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t vehicleId);
}