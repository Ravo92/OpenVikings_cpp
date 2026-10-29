#pragma once

#include "XRuntimeOwnedList.h"
#include "C2RuntimeConstants.h"
#include "C2RuntimeConstants.h"

namespace NC2E2
{
	struct SStaticMoveableAnimationListTag
	{
	};

	struct SStaticMoveableWalkListTag
	{
	};

	template <class Tag>
	struct SFastMoveableJobAccess
	{
		std::uint32_t count;
		NC2Lifetime::CRuntimeOwnedList<Tag> entries;
	};

	template <class Tag>
	struct SFastMoveableTribeAccess
	{
		std::uint32_t count;
		SFastMoveableJobAccess<Tag> jobs[NC2Runtime::FastMoveableAccessLimits::JobSlots];
	};

	template <class Tag>
	struct SFastMoveableDataAccess
	{
		SFastMoveableTribeAccess<Tag> tribes[NC2Runtime::FastMoveableAccessLimits::TribeSlots];
	};

	using SFastAnimationDataAccess = SFastMoveableDataAccess<SStaticMoveableAnimationListTag>;
	using SFastWalkDataAccess = SFastMoveableDataAccess<SStaticMoveableWalkListTag>;

	struct SStaticMoveableManagerData
	{
		std::uint32_t sequenceLibraryCount;
		void* sequenceLibraries;
		SFastAnimationDataAccess animations;
		SFastWalkDataAccess walks;
	};

	struct SStaticAnimalVehicleGraphics
	{
		std::uint32_t tribe;
		std::uint32_t type;
		void* bodyBobManager;
		void* shadowBobManager;
		std::uint32_t paletteCount;
		std::int32_t paletteIds[5];
	};

	struct SStaticMoveableData
	{
		std::uint32_t animationCount;
		void* animations;
		std::uint32_t walkCount;
		void* walks;
		std::uint32_t animalGraphicsCount;
		SStaticAnimalVehicleGraphics* animalGraphics;
		std::uint32_t vehicleGraphicsCount;
		SStaticAnimalVehicleGraphics* vehicleGraphics;
	};

	static_assert(sizeof(SFastMoveableJobAccess<SStaticMoveableAnimationListTag>) == (sizeof(void*) == 8 ? 48 : 0x18));
	static_assert(sizeof(SFastMoveableTribeAccess<SStaticMoveableAnimationListTag>) == (sizeof(void*) == 8 ? 2696 : 0x544));
	static_assert(sizeof(SFastAnimationDataAccess) == (sizeof(void*) == 8 ? 113232 : 0xDD28));
	static_assert(sizeof(SFastWalkDataAccess) == (sizeof(void*) == 8 ? 113232 : 0xDD28));
	static_assert(sizeof(SStaticMoveableManagerData) == (sizeof(void*) == 8 ? 226480 : 113240));
	static_assert(offsetof(SStaticMoveableManagerData, animations) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticMoveableManagerData, walks) == (sizeof(void*) == 8 ? 113248 : 0xDD30));
	static_assert(sizeof(SStaticAnimalVehicleGraphics) == (sizeof(void*) == 8 ? 48 : 40));
	static_assert(sizeof(SStaticMoveableData) == (sizeof(void*) == 8 ? 64 : 32));

	struct SSequenceEntry
	{
		char name[100];
		std::int32_t firstBob;
	};

	struct SSequenceLibrary
	{
		char imageLibrary[100];
		char shadowLibrary[100];
		std::uint32_t count;
		SSequenceEntry* sequences;
	};

	static_assert(sizeof(SSequenceEntry) == 104);
	static_assert(sizeof(SSequenceLibrary) == (sizeof(void*) == 8 ? 216 : 208));

	struct SMapMoveableFrameList
	{
		std::uint32_t count;
		std::uint32_t* frames;
	};

	struct SE2StaticDataMapMoveableAnimationPart
	{
		std::uint32_t type;
		std::uint32_t startPercent;
		std::uint32_t endPercent;

		union
		{
			std::int32_t walkDirection;
			std::int32_t atomicAction;
			std::int32_t overlayBobId;
			std::int32_t overlayLandscapeId;
		};

		union
		{
			std::int32_t walkGood;
			std::int32_t atomicSubId;
			std::int32_t overlayOffsetX;
		};

		union
		{
			std::int32_t walkEndOffsetX;
			std::int32_t atomicDirection;
			std::int32_t overlayOffsetY;
		};

		std::int32_t walkEndOffsetY;
	};

	struct SE2StaticDataMapMoveableAnimationData
	{
		std::uint32_t tribe;
		std::uint32_t job;
		std::uint32_t action;
		std::uint32_t inHouseSubId;
		std::int32_t animationMode;
		std::uint32_t bodySequenceOrPartCount;
		std::uintptr_t headSequenceOrPartAddress;
		SMapMoveableFrameList frames;
		SMapMoveableFrameList directionFrames[8];
	};

	struct SE2StaticDataMapMoveableWalkData
	{
		std::uint32_t tribe;
		std::uint32_t job;
		std::uint32_t good;
		std::uint32_t walkSpeed;
		std::int32_t bodySequence;
		std::int32_t headSequence;
		SMapMoveableFrameList walkFrames[8];
		SMapMoveableFrameList turnFrames[8];
	};

	void* __fastcall E2StaticDataMapMoveablesManager_Construct(void* manager, void* unusedEdx);
	const SE2StaticDataMapMoveableAnimationData* StaticMapMoveables_GetAnimationDataPtr(void* manager, std::uint32_t tribe, std::uint32_t job, std::uint32_t action, std::uint32_t inHouseSubId);
	const SE2StaticDataMapMoveableWalkData* StaticMapMoveables_GetWalkDataPtr(void* manager, std::uint32_t tribe, std::uint32_t job, std::uint32_t good, std::uint32_t walkSpeed);

	static_assert(sizeof(SMapMoveableFrameList) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(SE2StaticDataMapMoveableAnimationPart) == 28);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, startPercent) == 4);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, endPercent) == 8);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, walkDirection) == 12);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, atomicAction) == 12);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, overlayBobId) == 12);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, overlayLandscapeId) == 12);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, walkGood) == 16);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, atomicSubId) == 16);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, overlayOffsetX) == 16);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, walkEndOffsetX) == 20);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, atomicDirection) == 20);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, overlayOffsetY) == 20);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationPart, walkEndOffsetY) == 24);
	static_assert(sizeof(SE2StaticDataMapMoveableAnimationData) == (sizeof(void*) == 8 ? 176 : 100));
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationData, animationMode) == 0x10);
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationData, frames) == (sizeof(void*) == 8 ? 32 : 0x1C));
	static_assert(offsetof(SE2StaticDataMapMoveableAnimationData, directionFrames) == (sizeof(void*) == 8 ? 48 : 0x24));
	static_assert(sizeof(SE2StaticDataMapMoveableWalkData) == (sizeof(void*) == 8 ? 280 : 152));
	static_assert(offsetof(SE2StaticDataMapMoveableWalkData, walkFrames) == 0x18);
	static_assert(offsetof(SE2StaticDataMapMoveableWalkData, turnFrames) == (sizeof(void*) == 8 ? 152 : 0x58));
}