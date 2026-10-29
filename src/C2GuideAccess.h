#pragma once
#include "C2CallbackManager.h"
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Windows guide and scratch-bucket layouts; unused fields are not given speculative meanings.
	struct SGuideConnectionStorage
	{
		std::int32_t distance, guideId;
	};

	struct SGuideStorage
	{
		std::uint8_t allocated;
		std::byte padding[3];
		std::uint32_t playerId, guideId, linkSystemId, uniqueId, mapPosition;
		std::byte unresolved18[32];
		SGuideConnectionStorage connections[8];
		std::uint32_t connectionCount, goodsCounts[66];
	};

	struct SGuideDistanceBucketStorage
	{
		std::uint32_t count;
		void* guides[30];
	};

	struct SGuideManagerStorage
	{
		std::uint32_t nextLinkSystemId;
		SGuideDistanceBucketStorage distanceBuckets[40];
		std::uint8_t* visitedGuides;
	};

	static_assert(sizeof(SGuideConnectionStorage) == 8);
	static_assert(sizeof(SGuideStorage) == 388);
	static_assert(sizeof(SGuideDistanceBucketStorage) == (sizeof(void*) == 8 ? 248 : 124));
	static_assert(sizeof(SGuideManagerStorage) == (sizeof(void*) == 8 ? 9936 : 4968));

	bool Tool_Guide_IsAllowedToBeBuild(std::uint32_t player, const void* point, bool checkSpacing);
	bool Guide_AddConnectionToGuide(void* guide, std::int32_t id);
	bool Guide_TryConnectingFromGuide(void* guide, std::int32_t id);
	void Guide_RemoveConnectionToGuide(void* guide, std::int32_t id);
	void Guide_DisconnectFromGuides(void* guide);
	void Guide_Exit(void* guide);
	bool Guide_Init(void* guide, std::int32_t id, std::uint32_t player, const void* point);
	void Guide_AttachToMap(void* guide);
	void Guide_DetachFromMap(void* guide);
	void Guide_SetLinkId(void* guide, std::uint32_t linkId);
	void Guide_SetLinkIdInvalid(void* guide);
	void Guide_ScanForGoods(void* guide);
	bool __fastcall GuideManager_InsertGuideAtPosition(void* manager, void* unusedEdx, std::uint32_t player, const void* point, std::uint32_t flags);
	void __fastcall GuideManager_RemoveGuide(void* manager, void* unusedEdx, std::uint32_t id);
	void GuideManager_RecalculateGuideConnections(void* manager, std::int32_t id);
	bool __cdecl GuideManager_l_Guides_Callback(TCallbackTypes type, std::uintptr_t userData, std::uintptr_t parameter1, std::uintptr_t parameter2, std::uintptr_t parameter3);
}