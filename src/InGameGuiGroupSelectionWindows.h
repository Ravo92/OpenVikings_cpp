#pragma once
#include "InGameGuiSelectionWindowAccess.h"

namespace NC2InGameGuiSelection
{
	struct SMultipleHumanSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SMultipleHumanSelectionWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	struct SMultipleVehicleSelectionWindowStorage
	{
		NC2InGameGuiBase::SBaseToolWindowStorage base;
		std::uint8_t unresolved70;
		std::byte unresolved71[3];
	};

	static_assert(sizeof(SMultipleVehicleSelectionWindowStorage) == (sizeof(void*) == 8 ? 192 : 116));

	struct SGroupSelectionElementStorage
	{
		NXBaseGui::SBaseElementStorage base;
		std::int32_t hoveredObjectId;
		char caption[200];
		std::uint32_t pressedCommand, actionCommand;
		void* receiver;
	};

	struct SMultipleHumanSelectionElementStorage
	{
		SGroupSelectionElementStorage group;
		NC2InGameGuiBase::CStaticToolTipHandler tooltips;
	};

	static_assert(sizeof(SGroupSelectionElementStorage) == (sizeof(void*) == 8 ? 288 : 260));
	static_assert(sizeof(SMultipleHumanSelectionElementStorage) == (sizeof(void*) == 8 ? 12296 : 12264));

	void* SelectionMultipleHumanWindow_Construct(void* window);
	void* SelectionMultipleVehiclesWindow_Construct(void* window);
}