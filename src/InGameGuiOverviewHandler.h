#pragma once

#include "XGuiLists.h"

#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiOverviewDisplay
{
	class CWorldOverviewBitmapGuiElement;

	class CWorldOverviewElementHandler final
	{
	public:
		struct SVars
		{
			std::int16_t changedAreaMinimum[2]; // 00569418
			std::int16_t changedAreaMaximum[2]; // 0056941C
			bool fullRebuildRequested;          // 00569420
			std::byte padding09[3];
			std::uint32_t redrawFrameCounter;   // 00569424
			std::int32_t zoomByView[3];         // 00569428
		};

		CWorldOverviewElementHandler();
		~CWorldOverviewElementHandler();
		static void InitializeStaticChangedArea();
		static bool __cdecl l_MapOverview_Callback(std::uint32_t type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
		void System_RegisterElement(CWorldOverviewBitmapGuiElement* element);
		void System_UnRegisterElement(CWorldOverviewBitmapGuiElement* element);
		void System_UpdateAllElements();
		void System_ControllingPlayerChanged(std::int32_t playerId);
		void System_SetExplorationState(bool enabled);
		void System_ScanTextures();
		void System_RedrawAllElements();
		void Marker_AddInfoMarker(std::int32_t playerId, const std::uint32_t* packedMapPoint);

		inline static CWorldOverviewElementHandler* sTheObjectPtr = nullptr;
		inline static SVars mStaticVars{ };

	private:
		NXBasics::CList<CWorldOverviewBitmapGuiElement> mElements;
	};

	static_assert(sizeof(CWorldOverviewElementHandler) == sizeof(NXBasics::CListBase));
	static_assert(sizeof(CWorldOverviewElementHandler::SVars) == 28);
	static_assert(offsetof(CWorldOverviewElementHandler::SVars, changedAreaMaximum) == 0x04);
	static_assert(offsetof(CWorldOverviewElementHandler::SVars, fullRebuildRequested) == 0x08);
	static_assert(offsetof(CWorldOverviewElementHandler::SVars, redrawFrameCounter) == 0x0C);
	static_assert(offsetof(CWorldOverviewElementHandler::SVars, zoomByView) == 0x10);

	void WorldOverviewElement_UpdateArea(void* element, const std::int16_t* minimum, const std::int16_t* maximum);
	void WorldOverviewElement_MarkerAddInfo(void* element, std::int32_t playerId, const std::uint32_t* packedMapPoint);
}
