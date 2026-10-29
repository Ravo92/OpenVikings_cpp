#pragma once

#include "XGuiStorage.h"
#include "C2EngineDynamicStorage.h"
#include "C2SavegameFormats.h"
#include "C2SavegameOwnerStorage.h"
#include "C2E2StaticDataMapMoveables.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataHouses.h"
#include "C2EngineDisplayStorage.h"
#include "C2EngineParticleSimulation.h"
#include "C2EngineLandscapeAccess.h"
#include "C2E2StaticDataParticles.h"

// Established byte offsets and record sizes of original Windows game objects.
// Values and integer literal types are retained from the Windows reconstruction.
namespace NC2Layout
{
	namespace AnimalOffset
	{
		inline constexpr auto UniqueIdUnsigned = 8u;
		inline constexpr auto ConstructedState = 0x18;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto AnimalId = 4;
		inline constexpr auto UniqueId = 8;
		inline constexpr auto MapMoveableId = 0x0C;
		inline constexpr auto PlayerId = 0x10;
		inline constexpr auto TribeType = 0x14;
		inline constexpr auto AnimalType = 0x18;
		inline constexpr auto CurrentTask = 0x1C;
		inline constexpr auto CurrentTaskStatus = 0x20;
		inline constexpr auto WorkCenter = 0x24;
		inline constexpr auto WorkCenterX = 0x24;
		inline constexpr auto WorkCenterY = 0x26;
		inline constexpr auto StayPoint = 0x28;
		inline constexpr auto StayPointX = 0x28;
		inline constexpr auto StayPointY = 0x2A;
		inline constexpr auto BirthPoint = 0x2C;
		inline constexpr auto BirthPointX = 0x2C;
		inline constexpr auto BirthPointY = 0x2E;
		inline constexpr auto LeaderId = 0x34;
		inline constexpr auto LeaderRevision = 0x38;
		inline constexpr auto GroupSize = 0x3C;
		inline constexpr auto EnvironmentalFoodValue = 0x40;
		inline constexpr auto MilitaryTarget = 0x44;
		inline constexpr auto MilitaryTargetUnsigned = 0x44u;
		inline constexpr auto AttachmentTarget = 0x50;
		inline constexpr auto AttachedObjectId = 0x54;
		inline constexpr auto AttachedObjectRevision = 0x58;
		inline constexpr auto AggressiveFlag = 0x5C;
		inline constexpr auto AggressionClearDelay = 0x60;
		inline constexpr auto Hitpoints = 0x64;
		inline constexpr auto Food = 0x68;
		inline constexpr auto AgeCounter = 0x6C;
		inline constexpr auto AtomicAction = 0x70;
		inline constexpr auto AtomicAnimationRunningFlag = 0x74;
		inline constexpr auto AtomicAnimationId = 0x78;
		inline constexpr auto AtomicAnimationFrame = 0x7C;
		inline constexpr auto AtomicAnimationStartCount = 0x80;
		inline constexpr auto PreviousSectorAnimalId = 0x84;
		inline constexpr auto NextSectorAnimalId = 0x88;
		inline constexpr auto MissionCheckId = 0x8C;
		inline constexpr auto MiscFlags = 0x90;
		inline constexpr auto InsideHouseFlag = 0x94;
	}

	namespace DisplayAtomInformationOffset
	{
		inline constexpr auto BodyBobId = 0;
		inline constexpr auto HeadBobId = 4;
		inline constexpr auto AnimationTribe = 8;
		inline constexpr auto AnimationJob = 0x0C;
		inline constexpr auto AnimationAction = 0x10;
		inline constexpr auto AnimationStartCount = 0x14;
		inline constexpr auto WalkTribe = 0x18;
		inline constexpr auto WalkJob = 0x1C;
		inline constexpr auto WalkGood = 0x20;
		inline constexpr auto WalkSpeed = 0x24;
		inline constexpr auto BodyBobManager = offsetof(NC2E2::SDisplayAtomStorage, bodyBobManager);
		inline constexpr auto HeadBobManager = offsetof(NC2E2::SDisplayAtomStorage, headBobManager);
		inline constexpr auto ShadowBobManager = offsetof(NC2E2::SDisplayAtomStorage, shadowBobManager);
		inline constexpr auto Animation = offsetof(NC2E2::SDisplayAtomStorage, animation);
		inline constexpr auto Walk = offsetof(NC2E2::SDisplayAtomStorage, walk);
		inline constexpr auto ActorSpecificData = sizeof(NC2E2::SDisplayAtomStorage);
	}

	namespace DynamicHumanOffset
	{
		inline constexpr auto BodyVariant = offsetof(NC2E2::SDynamicHumanStorage, bodyVariant);
		inline constexpr auto HeadVariant = offsetof(NC2E2::SDynamicHumanStorage, headVariant);
		inline constexpr auto BodyPalette = offsetof(NC2E2::SDynamicHumanStorage, bodyPalette);
		inline constexpr auto HeadPalette = offsetof(NC2E2::SDynamicHumanStorage, headPalette);
		inline constexpr auto Name = offsetof(NC2E2::SDynamicHumanStorage, name);
		inline constexpr auto NameUnsigned = offsetof(NC2E2::SDynamicHumanStorage, name);
		inline constexpr auto NameTerminator = offsetof(NC2E2::SDynamicHumanStorage, name) + 99;
		inline constexpr auto HasUserMessageFlag = offsetof(NC2E2::SDynamicHumanStorage, hasUserMessage);
		inline constexpr auto SelectedFlag = offsetof(NC2E2::SDynamicHumanStorage, selected);
		inline constexpr auto HighlightedFlag = offsetof(NC2E2::SDynamicHumanStorage, highlighted);
	}

	namespace DynamicAnimalVehicleOffset
	{
		inline constexpr auto PaletteId = offsetof(NC2E2::SDynamicAnimalVehicleStorage, paletteId);
		inline constexpr auto SelectedFlag = offsetof(NC2E2::SDynamicAnimalVehicleStorage, selected);
		inline constexpr auto HighlightedFlag = offsetof(NC2E2::SDynamicAnimalVehicleStorage, highlighted);
	}

	namespace DynamicHouseOffset
	{
		inline constexpr auto FirePointFlagsUnsigned = 4u;

		inline constexpr auto ActiveFirePointCount = 0;

		inline constexpr auto SelectedFlag = 0xCC;
	}

	namespace StaticMoveableDataOffset
	{
		inline constexpr auto AnimationCount = offsetof(NC2E2::SStaticMoveableData, animationCount);
		inline constexpr auto Animations = offsetof(NC2E2::SStaticMoveableData, animations);
		inline constexpr auto WalkCount = offsetof(NC2E2::SStaticMoveableData, walkCount);
		inline constexpr auto Walks = offsetof(NC2E2::SStaticMoveableData, walks);
		inline constexpr auto AnimalGraphicsCount = offsetof(NC2E2::SStaticMoveableData, animalGraphicsCount);
		inline constexpr auto AnimalGraphics = offsetof(NC2E2::SStaticMoveableData, animalGraphics);
		inline constexpr auto VehicleGraphicsCount = offsetof(NC2E2::SStaticMoveableData, vehicleGraphicsCount);
		inline constexpr auto VehicleGraphics = offsetof(NC2E2::SStaticMoveableData, vehicleGraphics);
	}

	namespace StaticMoveableManagerOffset
	{
		inline constexpr auto SequenceLibraryCount = offsetof(NC2E2::SStaticMoveableManagerData, sequenceLibraryCount);
		inline constexpr auto SequenceLibraries = offsetof(NC2E2::SStaticMoveableManagerData, sequenceLibraries);
		inline constexpr auto AnimationAccess = offsetof(NC2E2::SStaticMoveableManagerData, animations);
		inline constexpr auto WalkAccess = offsetof(NC2E2::SStaticMoveableManagerData, walks);
		inline constexpr auto FirstAnimationList = offsetof(NC2E2::SStaticMoveableManagerData, animations) + offsetof(NC2E2::SFastAnimationDataAccess, tribes[0].jobs[0].entries);
		inline constexpr auto FirstWalkList = offsetof(NC2E2::SStaticMoveableManagerData, walks) + offsetof(NC2E2::SFastWalkDataAccess, tribes[0].jobs[0].entries);
	}

	namespace FastMoveableTribeOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Jobs = offsetof(NC2E2::SFastMoveableTribeAccess<NC2E2::SStaticMoveableAnimationListTag>, jobs);
		inline constexpr auto JobsUnsigned = offsetof(NC2E2::SFastMoveableTribeAccess<NC2E2::SStaticMoveableAnimationListTag>, jobs);
		inline constexpr auto ReverseDestructionEnd = sizeof(NC2E2::SFastAnimationDataAccess) + offsetof(NC2E2::SFastMoveableTribeAccess<NC2E2::SStaticMoveableAnimationListTag>, jobs);
	}

	namespace StaticMoveableSortOffset
	{
		inline constexpr auto ActionOrGood = 8;
		inline constexpr auto WalkSpeed = 12;
	}

	namespace FastMoveableJobOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Entries = offsetof(NC2E2::SFastMoveableJobAccess<NC2E2::SStaticMoveableAnimationListTag>, entries);
	}

	namespace StaticAnimalVehicleGraphicsOffset
	{
		inline constexpr auto Tribe = 0;
		inline constexpr auto Type = 4;
		inline constexpr auto BodyBobManager = offsetof(NC2E2::SStaticAnimalVehicleGraphics, bodyBobManager);
		inline constexpr auto ShadowBobManager = offsetof(NC2E2::SStaticAnimalVehicleGraphics, shadowBobManager);
		inline constexpr auto PaletteCount = offsetof(NC2E2::SStaticAnimalVehicleGraphics, paletteCount);
		inline constexpr auto PaletteIds = offsetof(NC2E2::SStaticAnimalVehicleGraphics, paletteIds);
	}

	namespace StaticMoveableAnimationOffset
	{
		inline constexpr auto DirectionFrames = offsetof(NC2E2::SE2StaticDataMapMoveableAnimationData, directionFrames);
		inline constexpr auto Mode = 0x10;
		inline constexpr auto PartArray = offsetof(NC2E2::SE2StaticDataMapMoveableAnimationData, headSequenceOrPartAddress);
		inline constexpr auto FrameArray = offsetof(NC2E2::SE2StaticDataMapMoveableAnimationData, frames) + offsetof(NC2E2::SMapMoveableFrameList, frames);
		inline constexpr auto DirectionFramePointers = offsetof(NC2E2::SE2StaticDataMapMoveableAnimationData, directionFrames) + offsetof(NC2E2::SMapMoveableFrameList, frames);
	}

	namespace StaticMoveableWalkOffset
	{
		inline constexpr auto WalkFrames = offsetof(NC2E2::SE2StaticDataMapMoveableWalkData, walkFrames);
		inline constexpr auto TurnFrames = offsetof(NC2E2::SE2StaticDataMapMoveableWalkData, turnFrames);
		inline constexpr auto WalkFramePointers = offsetof(NC2E2::SE2StaticDataMapMoveableWalkData, walkFrames) + offsetof(NC2E2::SMapMoveableFrameList, frames);
		inline constexpr auto TurnFramePointers = offsetof(NC2E2::SE2StaticDataMapMoveableWalkData, turnFrames) + offsetof(NC2E2::SMapMoveableFrameList, frames);
	}

	namespace WeatherSectorManagerOffset
	{
		inline constexpr auto SectorWidth = 0;
		inline constexpr auto SectorHeight = 4;
		inline constexpr auto Sectors = 8;
	}

	namespace WeatherSectorOffset
	{
		inline constexpr auto RainDensity = 0;
		inline constexpr auto SnowDensity = 4;
		inline constexpr auto SandDensity = 8;
	}

	namespace InputSelectionMemberOffset
	{
		inline constexpr auto LandscapeMapPointX = 0xA9C;
		inline constexpr auto LandscapeMapPointY = 0xA9E;
		inline constexpr auto RoadBuildPointX = 0xAA4;
		inline constexpr auto RoadBuildPointY = 0xAA6;
		inline constexpr auto WallBuildPointX = 0xAA8;
		inline constexpr auto WallBuildPointY = 0xAAA;
	}

	namespace InputRoadWallPointStateOffset
	{
		inline constexpr auto RoadStartPointX = 0;
		inline constexpr auto RoadStartPointY = 2;
		inline constexpr auto WallStartPointX = 4;
		inline constexpr auto WallStartPointY = 6;
		inline constexpr auto TargetPoints = 8;
	}

	namespace InputTargetPointListOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Points = 4u;
	}

	namespace InputManagerOffset
	{
		inline constexpr auto MapHighlightingEnabled = 0xAB4;
		inline constexpr auto CursorMapPoint = 0xAB6;
		inline constexpr auto CursorMapPointY = 0xAB8;
		inline constexpr auto AcceptedTargetPresent = 0xABA;
		inline constexpr auto AcceptedTargetMapPoint = 0xABC;
		inline constexpr auto AcceptedTargetMapPointY = 0xABE;
		inline constexpr auto RoadStartPoint = 0xAC0;
		inline constexpr auto WallStartPoint = 0xAC4;
		inline constexpr auto TargetPointCount = 0xAC8;
		inline constexpr auto TargetPointsUnsigned = 0xACCu;

		inline constexpr auto InputMode = 0;
		inline constexpr auto SelectionType = 4;
		inline constexpr auto SingleHumanId = 8;
		inline constexpr auto SingleHumanUniqueId = 0xC;
		inline constexpr auto HumanGroupCount = 0x10;
		inline constexpr auto HumanGroup = 0x14;
		inline constexpr auto HumanGroupUniqueIds = 0x18;
		inline constexpr auto HumanGroupSecondId = 0x1C;
		inline constexpr auto HumanGroupSecondUniqueId = 0x20;
		inline constexpr auto SingleAnimalId = 0x974;
		inline constexpr auto SingleAnimalUniqueId = 0x978;
		inline constexpr auto SingleVehicleId = 0x97C;
		inline constexpr auto SingleVehicleUniqueId = 0x980;
		inline constexpr auto VehicleGroupCount = 0x984;
		inline constexpr auto VehicleGroup = 0x988;
		inline constexpr auto VehicleGroupUniqueIds = 0x98C;
		inline constexpr auto VehicleGroupSecondId = 0x990;
		inline constexpr auto VehicleGroupSecondUniqueId = 0x994;
		inline constexpr auto HouseId = 0xA78;
		inline constexpr auto HouseUniqueId = 0xA7C;
		inline constexpr auto HouseType = 0xA80;
		inline constexpr auto HouseDevelopmentStage = 0xA84;
		inline constexpr auto HouseSpecialItemType = 0xA88;
		inline constexpr auto HouseSpecialItem = 0xA8C;
		inline constexpr auto GuideId = 0xA90;
		inline constexpr auto GuideUniqueId = 0xA94;
		inline constexpr auto LandscapeType = 0xA98;
		inline constexpr auto LandscapeVariant = 0xA9C;
		inline constexpr auto LandscapeMapPoint = 0xAA0;
		inline constexpr auto LandscapePlayerId = 0xAA4;
		inline constexpr auto RoadBuildPoint = 0xAA8;
		inline constexpr auto WallBuildPoint = 0xAAC;
		inline constexpr auto SelectionChangeCount = 0xAB0;
		inline constexpr auto SingleHumanIdUnsigned = 8u;
		inline constexpr auto SingleHumanUniqueIdUnsigned = 0xCu;
		inline constexpr auto HumanGroupCountUnsigned = 0x10u;
		inline constexpr auto HumanGroupUnsigned = 0x14u;
		inline constexpr auto HumanGroupUniqueIdsUnsigned = 0x18u;
		inline constexpr auto SingleAnimalIdUnsigned = 0x974u;
		inline constexpr auto SingleAnimalUniqueIdUnsigned = 0x978u;
		inline constexpr auto SingleVehicleIdUnsigned = 0x97Cu;
		inline constexpr auto SingleVehicleUniqueIdUnsigned = 0x980u;
		inline constexpr auto VehicleGroupCountUnsigned = 0x984u;
		inline constexpr auto VehicleGroupUnsigned = 0x988u;
		inline constexpr auto VehicleGroupUniqueIdsUnsigned = 0x98Cu;
		inline constexpr auto HouseIdUnsigned = 0xA78u;
		inline constexpr auto HouseUniqueIdUnsigned = 0xA7Cu;
		inline constexpr auto HouseTypeUnsigned = 0xA80u;
		inline constexpr auto HouseDevelopmentStageUnsigned = 0xA84u;
		inline constexpr auto GuideIdUnsigned = 0xA90u;
		inline constexpr auto GuideUniqueIdUnsigned = 0xA94u;
		inline constexpr auto LandscapeTypeUnsigned = 0xA98u;
		inline constexpr auto LandscapeVariantUnsigned = 0xA9Cu;
	}

	namespace DisplayObjectReferenceOffset
	{
		inline constexpr auto HumanIdUnsigned = 8u;
		inline constexpr auto AnimalIdUnsigned = 0x10u;
		inline constexpr auto VehicleIdUnsigned = 0x18u;
		inline constexpr auto Type = 0;
		inline constexpr auto MapPosition = 4;
		inline constexpr auto MapPositionX = 4;
		inline constexpr auto MapPositionY = 6;
		inline constexpr auto HumanId = 8;
		inline constexpr auto HumanUniqueId = 12;
		inline constexpr auto AnimalId = 0x10;
		inline constexpr auto AnimalUniqueId = 0x14;
		inline constexpr auto VehicleId = 0x18;
		inline constexpr auto VehicleUniqueId = 0x1C;
		inline constexpr auto MapMoveableId = 0x20;
		inline constexpr auto MapMoveableUniqueId = 0x24;
		inline constexpr auto HouseId = 0x28;
		inline constexpr auto HouseUniqueId = 0x2C;
		inline constexpr auto GuideId = 0x30;
		inline constexpr auto GuideUniqueId = 0x34;
		inline constexpr auto LandscapeId = 0x38;
		inline constexpr auto LandscapeVariant = 0x3C;
		inline constexpr auto PlayerId = 0x40;
		inline constexpr auto MarkType = 0x44;
		inline constexpr auto BobData = 0x48;
	}

	namespace DisplayMarkingOffset
	{
		inline constexpr auto PrimaryReferenceEnabled = offsetof(NC2E2::SDisplayMarkingState, primaryReferenceEnabled);
		inline constexpr auto PrimaryMarkedObject = offsetof(NC2E2::SDisplayMarkingState, primaryMarkedObject);
		inline constexpr auto SecondaryMarkedObjectCount = offsetof(NC2E2::SDisplayMarkingState, secondaryMarkedObjectCount);
		inline constexpr auto SecondaryMarkedObjects = offsetof(NC2E2::SDisplayMarkingState, secondaryMarkedObjects);
		inline constexpr auto FirstSecondaryMapPositionX = offsetof(NC2E2::SDisplayMarkingState, secondaryMarkedObjects) + offsetof(NC2E2::SDisplayObjectReference, mapPosition);
		inline constexpr auto FirstSecondaryMapPositionY = offsetof(NC2E2::SDisplayMarkingState, secondaryMarkedObjects) + offsetof(NC2E2::SDisplayObjectReference, mapPosition) + 2;
	}

	namespace DisplayDescriptionOffset
	{
		inline constexpr auto VerticalSpacing = 0;
		inline constexpr auto HorizontalSpacing = 4;
		inline constexpr auto HeightScale = 8;
		inline constexpr auto MicroVerticalFixed = 0x0C;
		inline constexpr auto MicroHorizontalFixed = 0x10;
		inline constexpr auto MicroHeightFixed = 0x14;
		inline constexpr auto MicroHalfHorizontalFixed = 0x18;
		inline constexpr auto MicroHalfVerticalFixed = 0x1C;
		inline constexpr auto MicroVerticalPixels = 0x20;
		inline constexpr auto MicroHorizontalPixels = 0x24;
		inline constexpr auto MicroHeightPixels = 0x28;
		inline constexpr auto MicroHalfHorizontalPixels = 0x2C;
		inline constexpr auto MicroHalfVerticalPixels = 0x30;
		inline constexpr auto MacroVerticalFixed = 0x34;
		inline constexpr auto MacroHorizontalFixed = 0x38;
		inline constexpr auto MacroHeightFixed = 0x3C;
		inline constexpr auto MacroHalfHorizontalFixed = 0x40;
		inline constexpr auto MacroHalfVerticalFixed = 0x44;
		inline constexpr auto MacroVerticalPixels = 0x48;
		inline constexpr auto MacroHorizontalPixels = 0x4C;
		inline constexpr auto MacroHeightPixels = 0x50;
		inline constexpr auto MacroHalfHorizontalPixels = 0x54;
		inline constexpr auto MacroHalfVerticalPixels = 0x58;
		inline constexpr auto WorldWidthPixels = 0x5C;
		inline constexpr auto WorldHeightPixels = 0x60;
	}

	namespace EngineDisplayOffset
	{
		inline constexpr auto FontSigned = offsetof(NC2E2::SRuntimeDisplayStorage, active);
		inline constexpr auto DrawFlagsByte1 = offsetof(NC2E2::SRuntimeDisplayStorage, drawFlags) + 1;

		inline constexpr auto DrawCounters = offsetof(NC2E2::SRuntimeDisplayStorage, counters);
		inline constexpr auto SeenLocalAdultMenUnsigned = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalAdultMen);
		inline constexpr auto SeenLocalAdultWomenUnsigned = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalAdultWomen);
		inline constexpr auto SeenLocalChildrenUnsigned = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalChildren);
		inline constexpr auto VisitedMapPoints = offsetof(NC2E2::SRuntimeDisplayStorage, counters.visitedMapPoints);
		inline constexpr auto LandscapeDrawCount = offsetof(NC2E2::SRuntimeDisplayStorage, counters.landscapeDrawCount);
		inline constexpr auto SeenHumans = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenHumans);
		inline constexpr auto SeenLocalAdultMen = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalAdultMen);
		inline constexpr auto SeenLocalAdultWomen = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalAdultWomen);
		inline constexpr auto SeenLocalChildren = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenLocalChildren);
		inline constexpr auto SeenHouses = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenHouses);
		inline constexpr auto SeenDamagedHouses = offsetof(NC2E2::SRuntimeDisplayStorage, counters.seenDamagedHouses);

		inline constexpr auto PrimaryReferenceEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, marking.primaryReferenceEnabled);
		inline constexpr auto PrimaryMarkedObject = offsetof(NC2E2::SRuntimeDisplayStorage, marking.primaryMarkedObject);
		inline constexpr auto SecondaryMarkedObjectCount = offsetof(NC2E2::SRuntimeDisplayStorage, marking.secondaryMarkedObjectCount);
		inline constexpr auto SecondaryMarkedObjects = offsetof(NC2E2::SRuntimeDisplayStorage, marking.secondaryMarkedObjects);
		inline constexpr auto TooltipOverlaysEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, tooltipOverlaysEnabled);

		inline constexpr auto ActiveDisplayFlag = offsetof(NC2E2::SRuntimeDisplayStorage, active);
		inline constexpr auto ZoomCounter = offsetof(NC2E2::SRuntimeDisplayStorage, zoomCounter);
		inline constexpr auto EarthquakeEndTime = offsetof(NC2E2::SRuntimeDisplayStorage, earthquakeEndTime);
		inline constexpr auto EarthquakeOffsetX = offsetof(NC2E2::SRuntimeDisplayStorage, earthquakeOffsetX);
		inline constexpr auto EarthquakeOffsetY = offsetof(NC2E2::SRuntimeDisplayStorage, earthquakeOffsetY);
		inline constexpr auto WantedWorldPixelX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.wantedWorldPixelX);
		inline constexpr auto WantedWorldPixelY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.wantedWorldPixelY);
		inline constexpr auto BufferRectangle = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.bufferRectangle);
		inline constexpr auto StaticBitmap = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.staticBitmap);
		inline constexpr auto StaticBitmapX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.staticBitmapX);
		inline constexpr auto StaticBitmapY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.staticBitmapY);
		inline constexpr auto StaticRebuildRequiredFlag = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.staticRebuildRequired);
		inline constexpr auto MapRectangle = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectangle);
		inline constexpr auto MapRectangleX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectangle);
		inline constexpr auto MapRectangleY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectangle) + 2;
		inline constexpr auto MapRectangleWidth = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectangle) + 4;
		inline constexpr auto MapRectangleHeight = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectangle) + 6;
		inline constexpr auto MapMinimumX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapMinimumX);
		inline constexpr auto MapMaximumX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapMaximumX);
		inline constexpr auto MapMinimumY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapMinimumY);
		inline constexpr auto MapMaximumY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapMaximumY);
		inline constexpr auto MapRectanglePixelX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectanglePixelX);
		inline constexpr auto MapRectanglePixelY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.mapRectanglePixelY);

		inline constexpr auto RainEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, rain);
		inline constexpr auto SnowEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, snow);
		inline constexpr auto SandEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, sand);

		inline constexpr auto WorldPixelX = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.worldPixelX);
		inline constexpr auto WorldPixelY = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.worldPixelY);
		inline constexpr auto BufferWidth = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.bufferRectangle.width);
		inline constexpr auto BufferHeight = offsetof(NC2E2::SRuntimeDisplayStorage, scroll.bufferRectangle.height);

		inline constexpr auto DestinationBitmap = offsetof(NC2E2::SRuntimeDisplayStorage, destinationBitmap);
		inline constexpr auto DrawingBitmap = offsetof(NC2E2::SRuntimeDisplayStorage, drawingBitmap);
		inline constexpr auto DrawFlags = offsetof(NC2E2::SRuntimeDisplayStorage, drawFlags);
		inline constexpr auto HighlightingEnabled = offsetof(NC2E2::SRuntimeDisplayStorage, highlightingEnabled);
		inline constexpr auto PlayerId = offsetof(NC2E2::SRuntimeDisplayStorage, playerId);
		inline constexpr auto GouraudShader = offsetof(NC2E2::SRuntimeDisplayStorage, gouraudShader);
		inline constexpr auto Font = offsetof(NC2E2::SRuntimeDisplayStorage, font);
	}

	namespace PaletteOffset
	{
		inline constexpr auto MissingProductionExperienceColor = sizeof(void*) + 600;

		inline constexpr auto InactiveGoalColor = sizeof(void*) + 740;

		inline constexpr auto LastColor = sizeof(void*) + 1020;

		inline constexpr auto Colors = sizeof(void*);
		inline constexpr auto PlayerColorSampleWithinRange = sizeof(void*) + 40;
	}

	namespace StaticHumanMiscOffset
	{
		inline constexpr auto RandomPaletteSetCount = 0;
		inline constexpr auto RandomPaletteSets = offsetof(NC2E2::SHumanMiscData, randomPaletteSets);
		inline constexpr auto JobBaseGraphicsCount = offsetof(NC2E2::SHumanMiscData, jobBaseGraphicsCount);
		inline constexpr auto JobBaseGraphics = offsetof(NC2E2::SHumanMiscData, jobBaseGraphics);
		inline constexpr auto JobChangeGraphicsCount = offsetof(NC2E2::SHumanMiscData, jobChangeGraphicsCount);
		inline constexpr auto JobChangeGraphics = offsetof(NC2E2::SHumanMiscData, jobChangeGraphics);
		inline constexpr auto TribeNameArrays = offsetof(NC2E2::SHumanMiscData, tribeNames);
		inline constexpr auto TribeNameArraysUnsigned = offsetof(NC2E2::SHumanMiscData, tribeNames);
		inline constexpr auto PlayerColors = offsetof(NC2E2::SHumanMiscData, playerColors);
		inline constexpr auto PlayerColorsUnsigned = offsetof(NC2E2::SHumanMiscData, playerColors);
	}

	namespace HumanJobGraphicsOffset
	{
		inline constexpr auto Tribe = 0;
		inline constexpr auto Job = 4;
		inline constexpr auto BodyCount = 8;
		inline constexpr auto BodyChoices = offsetof(NC2E2::SHumanJobGraphics, bodies);
		inline constexpr auto BodyChoicesHex = offsetof(NC2E2::SHumanJobGraphics, bodies);
		inline constexpr auto BodyBobManagers = offsetof(NC2E2::SHumanJobGraphics, bodies) + offsetof(NC2E2::SHumanBodyGraphics, bodyBobManager);
		inline constexpr auto ShadowBobManagers = offsetof(NC2E2::SHumanJobGraphics, bodies) + offsetof(NC2E2::SHumanBodyGraphics, shadowBobManager);
		inline constexpr auto HeadCount = offsetof(NC2E2::SHumanJobGraphics, headCount);
		inline constexpr auto HeadChoices = offsetof(NC2E2::SHumanJobGraphics, heads);
		inline constexpr auto HeadBobManagers = offsetof(NC2E2::SHumanJobGraphics, heads) + offsetof(NC2E2::SHumanHeadGraphics, headBobManager);
	}

	namespace HumanJobBaseGraphicsOffset
	{
		inline constexpr auto BodyPaletteCount = offsetof(NC2E2::SHumanJobBaseGraphics, bodyPaletteCount);
		inline constexpr auto BodyPaletteCountUnsigned = offsetof(NC2E2::SHumanJobBaseGraphics, bodyPaletteCount);
		inline constexpr auto BodyPaletteIds = offsetof(NC2E2::SHumanJobBaseGraphics, bodyPaletteIds);
		inline constexpr auto HeadPaletteCount = offsetof(NC2E2::SHumanJobBaseGraphics, headPaletteCount);
		inline constexpr auto HeadPaletteCountUnsigned = offsetof(NC2E2::SHumanJobBaseGraphics, headPaletteCount);
		inline constexpr auto HeadPaletteIds = offsetof(NC2E2::SHumanJobBaseGraphics, headPaletteIds);
		inline constexpr auto RandomPatchCount = offsetof(NC2E2::SHumanJobBaseGraphics, randomPatchCount);
		inline constexpr auto RandomPatchCountUnsigned = offsetof(NC2E2::SHumanJobBaseGraphics, randomPatchCount);
		inline constexpr auto RandomPatchIds = offsetof(NC2E2::SHumanJobBaseGraphics, randomPatchIds);
	}

	namespace HumanJobChangeGraphicsOffset
	{
		inline constexpr auto RandomPatchId = offsetof(NC2E2::SHumanJobChangeGraphics, randomPatchId);
	}

	namespace TribeHumanNameArraysOffset
	{
		inline constexpr auto Male = 0;
		inline constexpr auto Female = offsetof(NC2E2::STribeHumanNameArrays, female);
		inline constexpr auto MaleUnsigned = 0u;
		inline constexpr auto FemaleUnsigned = offsetof(NC2E2::STribeHumanNameArrays, female);
	}

	namespace HumanBodyGraphicsOffset
	{
		inline constexpr auto VariantId = 0;
		inline constexpr auto BodyBobManager = offsetof(NC2E2::SHumanBodyGraphics, bodyBobManager);
		inline constexpr auto ShadowBobManager = offsetof(NC2E2::SHumanBodyGraphics, shadowBobManager);
	}

	namespace RandomPaletteSetOffset
	{
		inline constexpr auto Name = 0;
		inline constexpr auto PatchCount = 0x28;
		inline constexpr auto Patches = offsetof(NC2E2::SRandomPaletteSet, patches);
		inline constexpr auto DestinationRangeCount = offsetof(NC2E2::SRandomPaletteSet, destinationRangeCount);
		inline constexpr auto DestinationRanges = offsetof(NC2E2::SRandomPaletteSet, destinationRanges);
	}

	namespace RandomPalettePatchOffset
	{
		inline constexpr auto DestinationRange = 0;
		inline constexpr auto Palette16Id = 4;
		inline constexpr auto SourceRange = 8;
		inline constexpr auto Weight = 12;
	}

	namespace StaticHumanColorOffset
	{
		inline constexpr auto Color = 0;
		inline constexpr auto MaleRandomPatchId = 4;
		inline constexpr auto MaleRandomPatchIdUnsigned = 4u;
		inline constexpr auto FemaleRandomPatchId = 8;
		inline constexpr auto FemaleRandomPatchIdUnsigned = 8u;
		inline constexpr auto InhabitantSigns = 0xC;
		inline constexpr auto SoldierSigns = 0x34;
		inline constexpr auto SoldierSignsUnsigned = 0x34u;
		inline constexpr auto PlayerPaletteId = 0x50;
		inline constexpr auto PlayerPaletteIdUnsigned = 0x50u;
		inline constexpr auto ShipPaletteId = 0x54;
		inline constexpr auto RoadBuildSignLandscapeId = 0x48;
		inline constexpr auto WallBuildSignLandscapeId = 0x4C;
	}

	namespace GameControlOffset
	{
		inline constexpr auto DifficultyLevel = 0x5D200;
	}

	namespace GameControlPlayerOffset
	{
		inline constexpr auto BabiesBorn = 18964;
		inline constexpr auto HumansDied = 18968;
		inline constexpr auto SoldiersDied = 18972;
		inline constexpr auto HappinessValue = 0x4610;

		inline constexpr auto ColorIdUnsigned = 0x10u;

		inline constexpr auto NetworkPlayerIdUnsigned = 0x14u;

		inline constexpr auto HappinessValueUnsigned = 0x4610u;

		inline constexpr auto TribeTypeUnsigned = 0x0Cu;

		inline constexpr auto TribeShortName = 0x49E0;

		inline constexpr auto TribeName = 0x49AC;

		inline constexpr auto PathfinderStartsThisTickUnsigned = 0x4A20u;

		inline constexpr auto DiplomacyStatesUnsigned = 0x45A8u;

		inline constexpr auto NetworkPlayerId = 0x14;

		inline constexpr auto ColorId = 0x10;

		inline constexpr auto NeverDiesFlag = 0x49A8;

		inline constexpr auto Wealth = 0x460C;

		inline constexpr auto ActiveFlag = 0;

		inline constexpr auto CiviliansKilled = 0x4618;

		inline constexpr auto SoldiersKilled = 0x4614;

		inline constexpr auto SuppressPlayerSeenMessagesUnsigned = 0x4994u;

		inline constexpr auto HideDiplomacyDetailsUnsigned = 0x4980u;

		inline constexpr auto HideDiplomacyChangesUnsigned = 0x496Cu;

		inline constexpr auto HousesDestroyed = 0x4620;

		inline constexpr auto VehiclesDestroyed = 0x461C;

		inline constexpr auto Type = 4;

		inline constexpr auto DeadFlagUnsigned = 8u;

		inline constexpr auto GoodsReceivedFromPlayersUnsigned = 0x4A30u;

		inline constexpr auto DiplomacyLockedUnsigned = 0x4958u;

		inline constexpr auto AttackedByPlayersUnsigned = 0x4944u;

		inline constexpr auto CiviliansKilledUnsigned = 0x4618u;

		inline constexpr auto SoldiersKilledUnsigned = 0x4614u;

		inline constexpr auto DeadFlag = 8;

		inline constexpr auto PlayerSeenFlags = 0x45F8;

		inline constexpr auto AllowedEquipment = 0x3C78;

		inline constexpr auto TypeUnsigned = 4u;

		inline constexpr auto SoldiersDiedUnsigned = 0x4A1Cu;

		inline constexpr auto HumansDiedUnsigned = 0x4A18u;

		inline constexpr auto BabiesBornUnsigned = 0x4A14u;

		inline constexpr auto PlayerSeenFlagsUnsigned = 0x45F8u;

		inline constexpr auto AvailableJobs = 0x948;

		inline constexpr auto AvailableGoods = 0x1D4C;

		inline constexpr auto AvailableHouses = 0x3126;

		inline constexpr auto TribeType = 0xC;

		inline constexpr auto AllowedHouses = 0x2820;

		inline constexpr auto AllowedGoods = 0x1278;

		inline constexpr auto AllowedJobs = 0x18;

		inline constexpr auto AllowedThings = 0x18;
		inline constexpr auto AllowedJobsUnsigned = 0x18u;
		inline constexpr auto WomanJobAllowedFlagUnsigned = 0x1Du;
		inline constexpr auto AvailableJobsUnsigned = 0x948u;
		inline constexpr auto AllowedGoodsUnsigned = 0x1278u;
		inline constexpr auto Good16AllowedFlagUnsigned = 0x1288u;
		inline constexpr auto AvailableGoodsUnsigned = 0x1D4Cu;
		inline constexpr auto Good16AvailableFlagUnsigned = 0x1D5Cu;
		inline constexpr auto AllowedHousesUnsigned = 0x2820u;
		inline constexpr auto AvailableHousesUnsigned = 0x3126u;
		inline constexpr auto AllowedVehiclesUnsigned = 0x3A2Cu;
		inline constexpr auto AllowedEquipmentUnsigned = 0x3C78u;
		inline constexpr auto AreaSearchCount = 0x4A24u;
		inline constexpr auto PointPathfinderStartCount = 0x4A28u;
		inline constexpr auto AreaSearchWithoutEarlyExitCount = 0x4A2Cu;
	}

	namespace GameMapOffset
	{
		inline constexpr auto Continents = 52 + 3 * sizeof(void*);

		inline constexpr auto MicroRectangle = 0x10;

		inline constexpr auto MicroWidth = 0;

		inline constexpr auto MacroRectangle = 0x20;

		inline constexpr auto MacroPointCount = 8;

		inline constexpr auto MicroPointCount = 0x0C;

		inline constexpr auto ContinentsUnsigned = Continents;

		inline constexpr auto ContinentCount = 48 + 3 * sizeof(void*);

		inline constexpr auto MicroHeight = 0x02;
		inline constexpr auto MacroWidth = 0x04;
		inline constexpr auto MacroHeight = 0x06;
		inline constexpr auto MacroPointStorage = 0x30;
		inline constexpr auto MicroPointStorage = 48 + sizeof(void*);
		inline constexpr auto PackingScratchBuffer = 48 + 2 * sizeof(void*);
	}

	namespace HouseAttachmentOffset
	{
		inline constexpr auto Flags = 0;

		inline constexpr auto RoleMask = 0;
		inline constexpr auto HumanId = 4;
		inline constexpr auto TrainingJobType = 8;
		inline constexpr auto TrainingGoodType = 12;
	}

	namespace HouseConstructionGoodOffset
	{
		inline constexpr auto GoodType = 0;
		inline constexpr auto RequiredAmount = 4;
		inline constexpr auto DeliveredAmount = 8;
		inline constexpr auto IncomingAmount = 12;
		inline constexpr auto ConsumedAmount = 16;
	}

	namespace HouseOffset
	{
		inline constexpr auto PlayerIdUnsigned = 12u;
		inline constexpr auto NextSectorHouseIdUnsigned = 1956u;
		inline constexpr auto UniqueIdUnsigned = 8u;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto HouseId = 4;
		inline constexpr auto UniqueId = 8;
		inline constexpr auto PlayerId = 0x0C;
		inline constexpr auto EngineType = 0x10;
		inline constexpr auto LogicType = 0x14;
		inline constexpr auto TribeType = 0x18;
		inline constexpr auto MapPosition = 0x1C;
		inline constexpr auto MapPositionX = 0x1C;
		inline constexpr auto MapPositionY = 0x1E;
		inline constexpr auto DevelopmentStage = 0x20;
		inline constexpr auto DoorEntryPoint = 0x24;
		inline constexpr auto DoorEntryPointX = 0x24;
		inline constexpr auto DoorEntryPointY = 0x26;
		inline constexpr auto MappedFlag = 0x28;
		inline constexpr auto ConstructionState = 0x2C;
		inline constexpr auto ConstructionStepsDone = 0x30;
		inline constexpr auto ConstructionStepsRequired = 0x34;
		inline constexpr auto ConstructionGoodsCount = 0x38;
		inline constexpr auto ConstructionGoodEntries = 0x3Cu;
		inline constexpr auto ConstructionGoodEntriesSigned = 0x3C;
		inline constexpr auto ConstructionGoodRequiredAmounts = 0x40u;
		inline constexpr auto ConstructionGoodsRequired = 0x104;
		inline constexpr auto ConstructionGoodsDelivered = 0x108;
		inline constexpr auto ConstructionGoodsConsumed = 0x10C;
		inline constexpr auto ConstructionStepLimit = 0x110;
		inline constexpr auto BuilderRecruitmentCounter = 0x114;
		inline constexpr auto StockAmounts = 0x118u;
		inline constexpr auto StockFractionalAmounts = 0x220u;
		inline constexpr auto FutureStockAmounts = 0x328u;
		inline constexpr auto StockRequestCounters = 0x430u;
		inline constexpr auto StockMinimumAmounts = 0x538u;
		inline constexpr auto Attachments = 0x640u;
		inline constexpr auto AttachmentHumanIds = 0x644u;
		inline constexpr auto HomeQualityEnhancers = 0x780;
		inline constexpr auto HomeQualityEnhancersUnsigned = 0x780u;
		inline constexpr auto HomeQualityEnhancer0Value = 0x784;
		inline constexpr auto HomeQualityEnhancer0ValueUnsigned = 0x784u;
		inline constexpr auto HomeQualityEnhancer1Allowed = 0x788;
		inline constexpr auto HomeQualityEnhancer1Value = 0x78C;
		inline constexpr auto HomeQualityEnhancer2Allowed = 0x790;
		inline constexpr auto HomeQualityEnhancer2Value = 0x794;
		inline constexpr auto ProductionStoppedFlag = 0x798;
		inline constexpr auto ProductionCounter = 0x79C;
		inline constexpr auto PreviousSectorHouseId = 0x7A0;
		inline constexpr auto NextSectorHouseId = 0x7A4;
		inline constexpr auto ActionOverlayCounters = 0x7A8u;
		inline constexpr auto LandscapeOverlayActiveFlags = 0x7BCu;
		inline constexpr auto LandscapeOverlayActiveFlagsSigned = 0x7BC;
		inline constexpr auto Hitpoints = 0x7C8;
		inline constexpr auto MissionCheckId = 0x7CC;
		inline constexpr auto DemolitionFlag = 0x7D0;
		inline constexpr auto DemolitionStepsDone = 0x7D2;
		inline constexpr auto DemolitionStepsRequired = 0x7D4;
		inline constexpr auto DefenceModeFlag = 0x7D8;
		inline constexpr auto BehaviourFlags = 0x7DC;
	}

	namespace HumanOffset
	{
		inline constexpr auto QueuedGoodieCommand = 1092;
		inline constexpr auto FoodProductionNeedCheckDeferrals = 2660;
		inline constexpr auto QueuedAssignJobCommand = 1832;
		inline constexpr auto QueuedBuildGuideCommand = 1252;
		inline constexpr auto QueuedExploreAreaAndUpdateCenterCommand = 1272;
		inline constexpr auto FirstUserCommandParameter1Unsigned = 924u;
		inline constexpr auto WorkAttachmentVehicleId = 0xB48;
		inline constexpr auto UniqueIdUnsigned = 8u;
		inline constexpr auto AllowedProductionGoodFlagsSigned = 0x195;

		inline constexpr auto ProducedGood = 0x1C;

		inline constexpr auto ChildIdUnsigned = 0xB24u;

		inline constexpr auto WaitingHumanIdUnsigned = 0xB1Cu;

		inline constexpr auto SpouseIdUnsigned = 0xB20u;

		inline constexpr auto MotherIdUnsigned = 0xB18u;

		inline constexpr auto FatherIdUnsigned = 0xB14u;

		inline constexpr auto MerchantIdUnsigned = 0xB44u;

		inline constexpr auto AttachedVehicleIdUnsigned = 0xB48u;

		inline constexpr auto VehicleConstructionHouseIdUnsigned = 0xB3Cu;

		inline constexpr auto WorkAttachmentHouseIdUnsigned = 0xB34u;

		inline constexpr auto HomeHouseIdUnsigned = 0xB28u;

		inline constexpr auto UnresolvedSevenEntrySelection = 0x1D7;

		inline constexpr auto MiscFlagsByte1 = 0xB85;

		inline constexpr auto JobExperienceSlot70 = 0x13C;

		inline constexpr auto AllowedLongBowSoldierFlag = 0x185;

		inline constexpr auto AllowedLongSwordSoldierFlag = 0x17F;

		inline constexpr auto AllowedIronSpearSoldierFlag = 0x17D;

		inline constexpr auto InventoryConditionsSigned = 0xAF8;

		inline constexpr auto AllowedEquipment32Flag = 0x200;

		inline constexpr auto AllowedEquipment31Flag = 0x1FF;

		inline constexpr auto AllowedEquipment30Flag = 0x1FE;

		inline constexpr auto JobSelectionEnabledFlag = 0x1DF;
		inline constexpr auto HuntingProductionAllowedFlag = 0x1CD;
		inline constexpr auto WomanJobAllowedFlag = 0x161;
		inline constexpr auto WeaponCategory1Experience = 0x140;
		inline constexpr auto WeaponCategory2Experience = 0x144;
		inline constexpr auto WeaponCategory3Experience = 0x148;
		inline constexpr auto WeaponCategory6Experience = 0x150;
		inline constexpr auto WeaponCategory7Experience = 0x154;
		inline constexpr auto WeaponCategory1ExperienceUnsigned = 0x140u;
		inline constexpr auto WeaponCategory2ExperienceUnsigned = 0x144u;
		inline constexpr auto WeaponCategory3ExperienceUnsigned = 0x148u;
		inline constexpr auto WeaponCategory6ExperienceUnsigned = 0x150u;
		inline constexpr auto WeaponCategory7ExperienceUnsigned = 0x154u;
		inline constexpr auto SoldierWorkExperience = 0x138;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto HumanId = 4;
		inline constexpr auto UniqueId = 8;
		inline constexpr auto MapMoveableId = 0xC;
		inline constexpr auto PlayerId = 0x10;
		inline constexpr auto TribeType = 0x14;
		inline constexpr auto JobType = 0x18;
		inline constexpr auto ProducedGoodType = 28;
		inline constexpr auto CollectionGoodType = 0x20;
		inline constexpr auto JobExperience = 0x24u;
		inline constexpr auto JobExperienceSigned = 0x24;
		inline constexpr auto EducationLevel = 0x158;
		inline constexpr auto AllowedJobFlags = 0x15Cu;
		inline constexpr auto AllowedJobFlagsSigned = 0x15C;
		inline constexpr auto ProductionSelectionMode = 0x194;
		inline constexpr auto AllowedProductionGoodFlags = 0x195u;
		inline constexpr auto AllowedEquipmentFlags = 0x1E0u;
		inline constexpr auto AllowedEquipmentFlagsSigned = 0x1E0;
		inline constexpr auto ProductionRotationIndex = 0x218;
		inline constexpr auto ProductionCounters = 0x21Cu;
		inline constexpr auto CurrentTask = 0x368;
		inline constexpr auto CurrentTaskStatus = 0x36C;
		inline constexpr auto CurrentTaskParameter = 0x370;
		inline constexpr auto CurrentTaskStep = 0x374;
		inline constexpr auto WaitingHumanCount = 0x378;
		inline constexpr auto IdleStartCount = 0x37C;
		inline constexpr auto TaskFailureCount = 0x380;
		inline constexpr auto MapPositionReachedCount = 0x384;
		inline constexpr auto WorkRetriesRequired = 0x388;
		inline constexpr auto WorkRetryCount = 0x38C;
		inline constexpr auto UserCommandQueue = 0x390u;
		inline constexpr auto LastUserCommandIndex = 0xA5C;
		inline constexpr auto UserCommandActiveFlag = 0xA60;
		inline constexpr auto UrgentNeedsCheckedFlag = 0xA61;
		inline constexpr auto InteractionTarget = 0xA68;
		inline constexpr auto TargetPosition = 0xA6C;
		inline constexpr auto TargetPositionX = 0xA6C;
		inline constexpr auto TargetPositionY = 0xA6E;
		inline constexpr auto TargetWorkPosition = 0xA70;
		inline constexpr auto TargetWorkPositionX = 0xA70;
		inline constexpr auto TargetWorkPositionY = 0xA72;
		inline constexpr auto TargetLandscapeType = 0xA74;
		inline constexpr auto TargetGoodType = 0xA78;
		inline constexpr auto TargetObjectId = 0xA7C;
		inline constexpr auto TargetObjectRevision = 0xA80;
		inline constexpr auto SecondaryInteractionTarget = 0xA84;
		inline constexpr auto SecondaryTargetPosition = 0xA88;
		inline constexpr auto SecondaryTargetWorkPosition = 0xA8C;
		inline constexpr auto SecondaryTargetLandscapeType = 0xA90;
		inline constexpr auto SecondaryTargetGoodType = 0xA94;
		inline constexpr auto SecondaryTargetObjectId = 0xA98;
		inline constexpr auto SecondaryTargetObjectRevision = 0xA9C;
		inline constexpr auto Hitpoints = 0xAA0;
		inline constexpr auto AgeCounter = 0xAA4;
		inline constexpr auto Food = 0xAA8;
		inline constexpr auto Stamina = 0xAAC;
		inline constexpr auto Social = 0xAB0;
		inline constexpr auto Religion = 0xAB4;
		inline constexpr auto AtomicAction = 0xAB8;
		inline constexpr auto AtomicAnimationRunningFlag = 0xABC;
		inline constexpr auto AtomicAnimationId = 0xAC0;
		inline constexpr auto AtomicAnimationFrame = 0xAC4;
		inline constexpr auto AtomicAnimationStartCount = 0xAC8;
		inline constexpr auto GoodCirculationMode = 0xACC;
		inline constexpr auto RequestedGood = 0xAD0;
		inline constexpr auto CarriedGood = 0xAD4;
		inline constexpr auto SuspendedGoodCirculationMode = 0xAD8;
		inline constexpr auto ShoeCondition = 0xADC;
		inline constexpr auto ToolCondition = 0xAE0;
		inline constexpr auto EquippedWeapon = 0xAE4;
		inline constexpr auto EquippedArmor = 0xAE8;
		inline constexpr auto EquippedShoes = 0xAEC;
		inline constexpr auto EquippedTools = 0xAF0;
		inline constexpr auto InventorySlots = 0xAF4u;
		inline constexpr auto InventorySlotsSigned = 0xAF4;
		inline constexpr auto InventoryConditions = 0xAF8u;
		inline constexpr auto FatherId = 0xB14;
		inline constexpr auto MotherId = 0xB18;
		inline constexpr auto WaitingHumanId = 0xB1C;
		inline constexpr auto SpouseId = 0xB20;
		inline constexpr auto ChildId = 0xB24;
		inline constexpr auto HomeHouseId = 0xB28;
		inline constexpr auto HomeHouseRevision = 0xB2C;
		inline constexpr auto WorkCenter = 0xB30;
		inline constexpr auto WorkCenterX = 0xB30;
		inline constexpr auto WorkCenterY = 0xB32;
		inline constexpr auto WorkAttachmentHouseId = 0xB34;
		inline constexpr auto WorkAttachmentHouseRevision = 0xB38;
		inline constexpr auto VehicleConstructionHouseId = 0xB3C;
		inline constexpr auto VehicleConstructionWorkshopRevision = 0xB40;
		inline constexpr auto MerchantId = 0xB44;
		inline constexpr auto AttachedVehicleId = 0xB48;
		inline constexpr auto AttachedVehicleRevision = 0xB4C;
		inline constexpr auto InsideHouseFlag = 0xB50;
		inline constexpr auto InsideVehicleFlag = 0xB51;
		inline constexpr auto PreviousSectorHumanId = 0xB54;
		inline constexpr auto NextSectorHumanId = 0xB58;
		inline constexpr auto MilitaryMode = 0xB5C;
		inline constexpr auto MilitaryState = 0xB5C;
		inline constexpr auto MilitaryWorldRefreshmentFlag = 0xB60;
		inline constexpr auto PrimaryMilitaryTarget = 0xB64;
		inline constexpr auto PrimaryMilitaryTargetUnsigned = 0xB64u;
		inline constexpr auto PrimaryMilitaryTargetValue = 0xB68;
		inline constexpr auto PrimaryMilitaryTargetRevision = 0xB6C;
		inline constexpr auto SecondaryMilitaryTarget = 0xB70;
		inline constexpr auto SecondaryMilitaryTargetUnsigned = 0xB70u;
		inline constexpr auto ExplorationRadius = 0xB7C;
		inline constexpr auto MissionCheckId = 0xB80;
		inline constexpr auto MiscFlags = 0xB84;
		inline constexpr auto AssistantData = 0xB88;
		inline constexpr auto AssistantHasCommandHistoryFlag = 0xB8C;
	}

	namespace MapMoveableOffset
	{
		inline constexpr auto MapPositionUnsigned = 18u;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto MoveableId = 4;
		inline constexpr auto ObjectType = 8;
		inline constexpr auto RemovalRequestedFlag = 0x0C;
		inline constexpr auto AttachedToMapFlag = 0x10;
		inline constexpr auto MapPosition = 0x12;
		inline constexpr auto MapPositionY = 0x14;
		inline constexpr auto SectorPosition = 0x18;
		inline constexpr auto SectorPositionY = 0x1C;
		inline constexpr auto Direction = 0x20;
		inline constexpr auto NextCellMoveableId = 0x24;
		inline constexpr auto PathfinderId = 0x28;
		inline constexpr auto WalkingFlag = 0x2C;
		inline constexpr auto WalkingDirection = 0x30;
		inline constexpr auto WalkingProgress = 0x34;
		inline constexpr auto WalkingPositionAdvancedFlag = 0x38;
		inline constexpr auto TurningFlag = 0x39;
		inline constexpr auto TurningElapsedTicks = 0x3C;
		inline constexpr auto TurnTargetDirection = 0x40;
		inline constexpr auto WalkingTicksPerStep = 0x44;
		inline constexpr auto WalkingProgressPerTick = 0x48;
		inline constexpr auto TurningTicksPerDirection = 0x4C;
		inline constexpr auto IncludeVerticalDirectionsFlag = 0x50;
		inline constexpr auto ObjectId = 0x54;
	}

	namespace HumanInventorySlotOffset
	{
		inline constexpr auto GoodType = 0;
		inline constexpr auto PercentageCondition = 4;
	}

	namespace UserNetworkCommandOffset
	{
		inline constexpr auto Opcode = 0;
		inline constexpr auto PlayerId = 1;
		inline constexpr auto ObjectId = 4;
		inline constexpr auto ObjectRevision = 8;
		inline constexpr auto Parameter1 = 12;
		inline constexpr auto Parameter2 = 16;
	}

	namespace ObjectSize
	{
		inline constexpr auto ConstructionSelectionWindow = 936;

		inline constexpr auto MapFishManager = 0x1774;

		inline constexpr auto SelectedHumanActionWindow = 120;

		inline constexpr auto HumanChangeNameWindow = sizeof(void*) == 8 ? 152 : 148;

		inline constexpr auto HumanChangeJobWindow = 128;

		inline constexpr auto GuiManagerMousePointer = 20;

		inline constexpr auto WorldDisplayElement = 580;

		inline constexpr auto GameSafetyBoxWindow = 116;

		inline constexpr auto OptionsWindow = 120;

		inline constexpr auto GuiClockElement = (sizeof(void*) == 8 ? 104 : 80);

		inline constexpr auto WindowsVideoPlayer = 44;

		inline constexpr auto Palette = sizeof(void*) == 8 ? 1048 : 1036;

		inline constexpr auto TitleScreenManager = 232;

		inline constexpr auto GuiListBoxController = 60;

		inline constexpr auto GuiListBox = 116;

		inline constexpr auto GuiWindowCloser = 84;

		inline constexpr auto GuiWindowMover = 248;

		inline constexpr auto GuiTextElement = 256;

		inline constexpr auto WorldGuiElement = 580;

		inline constexpr auto GuiTextButton = 96;

		inline constexpr auto GuiGfxButton = 84;

		inline constexpr auto TraderConfigWindow = 692;

		inline constexpr auto AIHighLevelHandler = 43544 + sizeof(NC2Logic::SAIGuideManagerStorage);

		inline constexpr auto AISinglePlayerHandler = 0xB068;

		inline constexpr auto MissionInfoWindow = 952;

		inline constexpr auto HypertextGuiElement = 72;

		inline constexpr auto EngineDisplay = sizeof(NC2E2::SRuntimeDisplayStorage);

		inline constexpr auto Bitmap = sizeof(void*) == 8 ? 120 : 84;

		inline constexpr auto ObjectGroupManager = 12044;

		inline constexpr auto InputManager = 3964;
		inline constexpr auto EmbeddedSavegameManager = sizeof(NC2Logic::SEmbeddedSavegameManagerStorage);
		inline constexpr auto MacroMapManager = 0x8;
		inline constexpr auto E2MapWeatherManagerSectorManager = sizeof(void*) == 8 ? 16 : 12;
		inline constexpr auto E2StaticDataTexturesManager = sizeof(void*) == 8 ? 40 : 20;
		inline constexpr auto E2StaticDataBobManagersManager = sizeof(void*) == 8 ? 56 : 28;
		inline constexpr auto MapMoveableArray = sizeof(void*) == 8 ? 72 : 48;
		inline constexpr auto MapPathfinderManager = sizeof(void*) == 8 ? 72 : 48;
		inline constexpr auto MarkerArray = sizeof(void*) == 8 ? 72 : 48;
		inline constexpr auto DelayedActionManager = sizeof(void*) == 8 ? 80 : 52;
		inline constexpr auto AmbientSoundManager = (sizeof(void*) == 8 ? 104 : 100);
		inline constexpr auto AIManager = sizeof(NC2Logic::SAIManagerStorage);
		inline constexpr auto PlayerStatisticManager = sizeof(NC2Logic::SPlayerStatisticManagerStorage);
		inline constexpr auto GameAssistantManager = 0x460;
		inline constexpr auto HumanMerchantArray = sizeof(void*) == 8 ? 1528 : 1496;
		inline constexpr auto MultiPlayerGoalManager = 0x740;
		inline constexpr auto TributeManager = 0x960;
		inline constexpr auto UserNetworkCommandManager = 0x9A0;
		inline constexpr auto HappinessManager = 0xAA0;
		inline constexpr auto E2HumanSoundManager = 0xD24;
		inline constexpr auto MissionOnscreenInfoManager = 0xFA0;
		inline constexpr auto E2StaticDataLandscapesManager = 0xFA4;
		inline constexpr auto E2StaticDataPatternManager = 0xFA4;
		inline constexpr auto GuideManager = 0x1368;
		inline constexpr auto E2StaticDataMapMoveablesManager = sizeof(NC2E2::SStaticMoveableManagerData);
		inline constexpr auto GameControl = 0x5D204;
		inline constexpr auto MissionManager = 0x81CAC;
		inline constexpr auto E2ParticelManager = sizeof(NC2E2::SParticelManagerStorage);
	}


	// Windows particle records and pool storage; offsets are relative to the indicated record.
	namespace ParticleOffset
	{
		inline constexpr auto SpeedMapX = 0x98;
		inline constexpr auto SpeedHeight = 0xA0;
		inline constexpr auto SpeedMapY = 0xA8;
		inline constexpr auto MovementMode = 0x90;
		inline constexpr auto UseWind = 0xF8;
		inline constexpr auto RemainingLifetimeTicks = 0x14;
		inline constexpr auto Active = 4;
		inline constexpr auto PendingMovementTicks = 0x110;
		inline constexpr auto ValencyIsDirection = 0x18;
		inline constexpr auto StaticParticleId = 0x128;
		inline constexpr auto FrameIndex = 0x124;
		inline constexpr auto Valency = 0x11C;
		inline constexpr auto Position = 0x28;
		inline constexpr auto PositionMapY = 0x30;
		inline constexpr auto UseMapHeights = 0x20;
		inline constexpr auto CachedGroundHeight = 0x80;
		inline constexpr auto PositionHeight = 0x38;
		inline constexpr auto PositionPixelX = 0x40;
		inline constexpr auto PositionPixelY = 0x48;
		inline constexpr auto MapPoint = 0x78;
		inline constexpr auto TargetPosition = 80;
		inline constexpr auto TargetHeight = 0x60;
		inline constexpr auto TargetPixelX = 0x68;
		inline constexpr auto TargetPixelY = 0x70;
		inline constexpr auto TargetMapY = 0x58;
		inline constexpr auto VectorMapX = 0xB0;
		inline constexpr auto VectorHeight = 0xB8;
		inline constexpr auto VectorMapY = 0xC0;
		inline constexpr auto ParabolaBaselineHeight = 0x108;
		inline constexpr auto TotalLifetimeTicks = 0x10;
		inline constexpr auto StepMapX = 0xC8;
		inline constexpr auto StepHeight = 0xD0;
		inline constexpr auto StepMapY = 0xD8;
		inline constexpr auto StepPixelX = 0xE0;
		inline constexpr auto StepPixelY = 0xE8;
		inline constexpr auto MaximumValency = 0x120;
		inline constexpr auto ValencyMode = 0x118;
		inline constexpr auto KillOnGroundCollision = 0xF9;
		inline constexpr auto FadingSpawn = 0x12D;
		inline constexpr auto DrawBottom = 0x12C;
		inline constexpr auto ParabolaHeight = 0x100;
		inline constexpr auto InitialDirectionValency = 0x1C;
		inline constexpr auto PlayerId = 0;
		inline constexpr auto StoredWindParameter = 0xF0;
		inline constexpr auto PreviousWorldPixelPoint = 136;
	}

	namespace ParticlePositionOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 8;
		inline constexpr auto Height = 0x10;
		inline constexpr auto PixelX = 0x18;
		inline constexpr auto PixelY = 0x20;
	}

	namespace ParticleWindOffset
	{
		inline constexpr auto TicksSinceChange = 0x54;
		inline constexpr auto AngleDegrees = 0x50;
		inline constexpr auto Cosine = 0;
		inline constexpr auto Sine = 8;
		inline constexpr auto HorizontalSpeed = 0x48;
		inline constexpr auto VerticalSpeed = 0x4C;
		inline constexpr auto HorizontalScale = 0x18;
		inline constexpr auto VerticalScale = 0x20;
		inline constexpr auto DeltaMapX = 0x30;
		inline constexpr auto DeltaMapY = 0x38;
	}

	namespace StaticParticleOffset
	{
		inline constexpr auto Graphics = offsetof(NC2E2::SStaticParticelStorageEntry, graphics);
		inline constexpr auto AnimationLoops = offsetof(NC2E2::SStaticParticelStorageEntry, animationLoops);
		inline constexpr auto MaximumValency = offsetof(NC2E2::SStaticParticelStorageEntry, graphics.maximumValency);
		inline constexpr auto SpawnParticleId = offsetof(NC2E2::SStaticParticelStorageEntry, spawnParticleId);
		inline constexpr auto ValencyIsDirection = offsetof(NC2E2::SStaticParticelStorageEntry, valencyIsDirection);
		inline constexpr auto HasSpawnParticle = offsetof(NC2E2::SStaticParticelStorageEntry, hasSpawnParticle);
		inline constexpr auto BobManager = offsetof(NC2E2::SStaticParticelStorageEntry, bobManager);
		inline constexpr auto PaletteId = offsetof(NC2E2::SStaticParticelStorageEntry, paletteId);
		inline constexpr auto SpecialType = offsetof(NC2E2::SStaticParticelStorageEntry, specialType);
		inline constexpr auto BobType = offsetof(NC2E2::SStaticParticelStorageEntry, bobType);
	}

	namespace ParticleFramesOffset
	{
		inline constexpr auto FrameCount = 4;
		inline constexpr auto FrameIds = 8;
		inline constexpr auto MinimumValency = 0;
	}

	namespace ParticleGeneratorOffset
	{
		inline constexpr auto AreaWidth = 0x5C;
		inline constexpr auto AreaHeight = 0x60;
		inline constexpr auto UseRandomArea = 0x58;
		inline constexpr auto ParticleId = 0x20;
		inline constexpr auto MovementMode = 0x30;
		inline constexpr auto SpeedMapX = 0x34;
		inline constexpr auto SpeedHeight = 0x38;
		inline constexpr auto SpeedMapY = 0x3C;
		inline constexpr auto UseMapHeights = 0x40;
		inline constexpr auto UseWind = 0x48;
		inline constexpr auto StoredWindParameter = 0x50;
		inline constexpr auto LifetimeTicks = 0x24;
		inline constexpr auto MaximumGoneLifetime = 0x28;
		inline constexpr auto ValencyMode = 0x2C;
		inline constexpr auto HeightOffset = 0x44;
		inline constexpr auto KillOnGroundCollision = 0x64;
	}

	namespace ParticleEffectOffset
	{
		inline constexpr auto ParticleId = 0;
		inline constexpr auto UseMapHeights = 0x1C;
		inline constexpr auto UseWind = 0x1D;
		inline constexpr auto MovementMode = 0xC;
		inline constexpr auto LifetimeTicks = 4;
		inline constexpr auto StartLifetimeTicks = 8;
		inline constexpr auto SpeedMapX = 0x10;
		inline constexpr auto SpeedHeight = 0x14;
		inline constexpr auto SpeedMapY = 0x18;
		inline constexpr auto MapOffsetX = 0x2E;
		inline constexpr auto MapOffsetY = 0x30;
		inline constexpr auto PixelPoint = 0x24;
		inline constexpr auto HeightOffset = 0x20;
		inline constexpr auto DrawBottom = 0x2C;
		inline constexpr auto PixelY = 0x28;
	}

	namespace ParticleGeneratorSlotOffset
	{
		inline constexpr auto ChancePercent = 0xC;
		inline constexpr auto PixelPoint = 4;
		inline constexpr auto PixelY = 8;
		inline constexpr auto Enabled = 0x14;
		inline constexpr auto GeneratorId = 0x10;
		inline constexpr auto Identifier = 0x18;
		inline constexpr auto MapPoint = 0;
		inline constexpr auto Active = 0x1C;
	}

	namespace ParticleGraphicsOffset
	{
		inline constexpr auto EntryCount = 0;
		inline constexpr auto Entries = 8;
	}

	namespace ParticleGeneratorManagerOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Entries = offsetof(NC2E2::SParticelGeneratorManagerStorage, entries);
	}


	// Windows macro-map storage and section records.
	namespace MacroMapSectionLimits
	{
		inline constexpr auto Count = 13;
	}

	namespace MacroMapByteValueOffset
	{
		inline constexpr auto Value = 4;
	}

	namespace MacroMapOffset
	{
		inline constexpr auto Initialized = 0;
		inline constexpr auto Width = 0x106;
		inline constexpr auto Height = 0x108;
		inline constexpr auto CenterX = 0x10A;
		inline constexpr auto CenterY = 0x10C;
		inline constexpr auto PatternsCount = 0x118;
		inline constexpr auto Patterns = 0x11c;
		inline constexpr auto PatternNames = 0x114;
		inline constexpr auto TransitionsCount = 0x128;
		inline constexpr auto Transitions = 0x12c;
		inline constexpr auto TransitionNames = 0x124;
		inline constexpr auto LandscapesCount = 0x138;
		inline constexpr auto Landscapes = 0x13c;
		inline constexpr auto LandscapeNames = 0x134;
		inline constexpr auto HeightsCount = 0x140;
		inline constexpr auto Heights = 0x144;
		inline constexpr auto VertexColorsCount = 0x148;
		inline constexpr auto VertexColors = 0x14c;
		inline constexpr auto RoadsCount = 0x150;
		inline constexpr auto Roads = 0x154;
		inline constexpr auto HumansCount = 0x158;
		inline constexpr auto Humans = 0x15c;
		inline constexpr auto AnimalsCount = 0x160;
		inline constexpr auto Animals = 0x164;
		inline constexpr auto HousesCount = 0x168;
		inline constexpr auto Houses = 0x16c;
		inline constexpr auto FishSwarmsCount = 0x170;
		inline constexpr auto FishSwarms = 0x174;
		inline constexpr auto PatternNamesCount = 0x110;
		inline constexpr auto TransitionNamesCount = 0x120;
		inline constexpr auto LandscapeNamesCount = 0x130;
		inline constexpr auto Filename = 1;
	}

	namespace MacroMapManagerOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Entries = 4;
	}

	namespace MacroMapNameOffset
	{
		inline constexpr auto StaticDataId = 40;
	}

	namespace MacroMapHouseOffset
	{
		inline constexpr auto PlayerId = 0;
		inline constexpr auto MapX = 4;
		inline constexpr auto MapY = 6;
		inline constexpr auto HouseType = 8;
		inline constexpr auto DevelopmentStage = 12;
		inline constexpr auto Completed = 16;
	}

	namespace MacroMapCreatureOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
		inline constexpr auto Type = 4;
		inline constexpr auto TribeType = 8;
		inline constexpr auto PlayerId = 12;
	}

	namespace MacroMapPatternOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
		inline constexpr auto NameIndex = 8;
		inline constexpr auto TriangleA = 4;
	}

	namespace MacroMapTransitionOffset
	{
		inline constexpr auto Layer = 4;
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
		inline constexpr auto TriangleA = 8;
		inline constexpr auto NameIndex = 12;
	}

	namespace MacroMapLandscapeOffset
	{
		inline constexpr auto PlayerIdUnsigned = 12u;

		inline constexpr auto MapY = 2;
		inline constexpr auto NameIndex = 4;
		inline constexpr auto MapX = 0;
		inline constexpr auto Valency = 8;
		inline constexpr auto PlayerId = 12;
	}

	namespace MacroMapPointOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
	}

	namespace MacroMapFishSwarmOffset
	{
		inline constexpr auto FishCount = 4;
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
	}

	namespace MacroMapHeightOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
		inline constexpr auto Height = 4;
	}

	namespace MacroMapVertexColorOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
		inline constexpr auto ColorIndex = 4;
	}

	namespace MacroMapRoadOffset
	{
		inline constexpr auto MapX = 0;
		inline constexpr auto MapY = 2;
	}


	// Windows AI owners, task records and soldier/military storage.
	namespace AISinglePlayerOffset
	{
		inline constexpr auto HumanRequests = 0x3B80;

		inline constexpr auto InitializationPendingFlag = 5;

		inline constexpr auto FirstGateRepairPendingFlag = 0x5B24;

		inline constexpr auto GateRepairPendingFlag = 0x5B1C;

		inline constexpr auto WallWorkPendingFlag = 0x43A4;

		inline constexpr auto FirstGateStateFlag = 0x5B2A;

		inline constexpr auto FirstGatePosition = 0x5B26;

		inline constexpr auto GateCount = 0x5B20;

		inline constexpr auto FirstWallWorkPendingFlag = 0x43AC;

		inline constexpr auto FirstWallPosition = 0x43AE;

		inline constexpr auto WallCount = 0x43A8;

		inline constexpr auto Enabled = 4;

		inline constexpr auto FirstActivatedConditionFlagUnsigned = 0x24u;

		inline constexpr auto ConditionsUnsigned = 0x0Cu;

		inline constexpr auto FirstConditionTypeUnsigned = 0x18u;

		inline constexpr auto LastConditionIndex = 8;

		inline constexpr auto MaxUnitLimit = 0xAD3C;

		inline constexpr auto UnitLimit = 0xAD38;

		inline constexpr auto SoldierCount = 0x6168;
		inline constexpr auto PlayerId = 0;
		inline constexpr auto CombinedTaskCount = 0x2A48;
		inline constexpr auto FirstCombinedTaskMainIndices = 0x2A64;
		inline constexpr auto CombinedTasks = 0x2A50;
		inline constexpr auto FirstCombinedTaskPhase = 0x2A98;
		inline constexpr auto VehicleCount = 0x5CB4;
		inline constexpr auto Vehicles = 0x5CB8;
		inline constexpr auto AllocatedSoldierCounts = 0xA500;
		inline constexpr auto SoldierAssignedFlags = 0xAC70;
		inline constexpr auto SoldierCandidateCount = 0xA62C;
		inline constexpr auto SoldierCandidateScores = 0xA634;
		inline constexpr auto SoldierCandidateSlots = 0xA630;
		inline constexpr auto TaskAllocationFinished = 0xA5FA;
		inline constexpr auto TaskAllocationWeighted = 0xA5C8;
		inline constexpr auto TaskAllocationEligible = 0xA338;
		inline constexpr auto TaskAllocationPriorityPercent = 0xA370;
		inline constexpr auto DefaultSoldierPositionEnabled = 0xA32C;
		inline constexpr auto DefaultSoldierPosition = 0xA32E;
		inline constexpr auto DefaultSoldierRadius = 0xA334;
		inline constexpr auto Soldiers = 0x616C;
		inline constexpr auto SoldierSelfDefenceOrigins = 0xAD48;
		inline constexpr auto WeaponCategoryCounts = 0xA30C;
		inline constexpr auto FirstMainTaskDescription = 0x1C38;
		inline constexpr auto TowerAttachmentCursor = 0xAD40;
		inline constexpr auto LastAttackedTick = 0xAD44;
		inline constexpr auto SoldierSelfDefenceOriginsUnsigned = 0xAD48u;
		inline constexpr auto LastMainTaskIndex = 0x1C2C;
		inline constexpr auto SoldiersUnsigned = 0x616Cu;
		inline constexpr auto CombinedTasksUnsigned = 0x2A50u;
		inline constexpr auto MainTasksUnsigned = 0x1C30u;
		inline constexpr auto VehiclesUnsigned = 0x5CB8u;
		inline constexpr auto ActiveMainTaskCount = 0x2978;
		inline constexpr auto ActiveMainTaskIndices = 0x297C;
		inline constexpr auto SecondActiveMainTaskIndex = 0x2980;
		inline constexpr auto BuildHouseCount = 0x3D60;
		inline constexpr auto BuildHouseTypes = 0x3D64;
		inline constexpr auto BuildHousePositions = 0x3D68;
		inline constexpr auto BuildHouseStages = 0x3D6C;
		inline constexpr auto BuildHouseMissionCheckIds = 0x3D70;
		inline constexpr auto FirstCombinedTaskPriority = 0x2A54;
		inline constexpr auto FirstCombinedTaskPriorityPercent = 0x2A58;
		inline constexpr auto FirstMainTaskDescriptionUnsigned = 0x1C38u;
		inline constexpr auto MainTasksChanged = 6;
		inline constexpr auto MainTasks = 0x1C30;
		inline constexpr auto FirstMainTaskPosition = 0x1C3C;
		inline constexpr auto BuildHousePositionsUnsigned = 0x3D68u;
		inline constexpr auto BuildHouseTypesUnsigned = 0x3D64u;
		inline constexpr auto BuildHouseStagesUnsigned = 0x3D6Cu;
		inline constexpr auto BuildHouseMissionCheckIdsUnsigned = 0x3D70u;
	}

	namespace AISoldierOffset
	{
		inline constexpr auto HumanId = 0;
		inline constexpr auto HumanUniqueId = 4;
		inline constexpr auto TaskDescription = 20;
		inline constexpr auto CurrentAction = 52;
		inline constexpr auto WeaponType = 8;
		inline constexpr auto ArmorType = 12;
		inline constexpr auto CombinedTaskIndex = 0x38;
		inline constexpr auto UsesDefaultTask = 0x10;
		inline constexpr auto MovementDestination = 60;
		inline constexpr auto MovementRadius = 64;
		inline constexpr auto TaskRadius = 28;
		inline constexpr auto TaskMilitaryMode = 44;
		inline constexpr auto TaskMapPoint = 24;
		inline constexpr auto AttackTarget = 0x48;
		inline constexpr auto SelfDefence = 0x44;
		inline constexpr auto AttackTargetUniqueId = 0x50;
		inline constexpr auto AttackTargetId = 0x4C;
	}

	namespace AIVehicleOffset
	{
		inline constexpr auto VehicleId = 0;
		inline constexpr auto VehicleUniqueId = 4;
		inline constexpr auto TaskIndex = 8;
		inline constexpr auto AttackActive = 12;
		inline constexpr auto AttackTargetUniqueId = 24;
		inline constexpr auto AttackTargetId = 20;
		inline constexpr auto AttackTarget = 16;
	}

	namespace AICombinedTaskOffset
	{
		inline constexpr auto MainTaskIndices = 0x14;
		inline constexpr auto TaskType = 0;
		inline constexpr auto AttackPhase = 0x48;
		inline constexpr auto Radius = 0x44;
		inline constexpr auto FormationRadius = 0x4C;
		inline constexpr auto MaximumSoldiers = 0x40;
		inline constexpr auto MinimumSoldiers = 0x3C;
		inline constexpr auto PriorityPercent = 8;
		inline constexpr auto FormationPosition = 0x50;
		inline constexpr auto Priority = 4;
		inline constexpr auto MainTaskCount = 0x10;
		inline constexpr auto FormationPositionY = 0x52;
		inline constexpr auto NextTaskPriority = 0x5C;
	}

	namespace AIMainTaskOffset
	{
		inline constexpr auto NoWayCorrection = 0x34;

		inline constexpr auto NoWayCorrectionMode = 0x38;

		inline constexpr auto NoWayCorrectionPendingFlag = 0x34;

		inline constexpr auto NoWayTargetPoint = 64;

		inline constexpr auto NoWaySourcePoint = 60;

		inline constexpr auto Description = 8;
		inline constexpr auto PrimaryPosition = 12;
		inline constexpr auto PrimaryPositionY = 14;
		inline constexpr auto Radius = 16;
		inline constexpr auto ConditionId = 0x2C;
		inline constexpr auto Disabled = 0;
		inline constexpr auto ConfiguredPriority = 40;
		inline constexpr auto ActivePriority = 4;
		inline constexpr auto CreatedTick = 0x30;
		inline constexpr auto SecondaryPosition = 28;
		inline constexpr auto SecondaryPositionY = 30;
		inline constexpr auto MilitaryMode = 32;
		inline constexpr auto MinimumSoldiers = 20;
		inline constexpr auto MaximumSoldiers = 24;
	}

	namespace AITaskDescriptionOffset
	{
		inline constexpr auto Type = 0;
		inline constexpr auto MapPoint = 4;
		inline constexpr auto MapPointY = 6;
		inline constexpr auto MinimumSoldiers = 0xC;
		inline constexpr auto MaximumSoldiers = 0x10;
		inline constexpr auto Radius = 8;
		inline constexpr auto FormationPosition = 0x14;
		inline constexpr auto FormationPositionY = 0x16;
	}

	namespace AIMilitaryManagerOffset
	{
		inline constexpr auto WeaponSeventeenGood = 0x1528;

		inline constexpr auto PlayerId = 0;
		inline constexpr auto ArmorGoods = 0x1C9C;
		inline constexpr auto ArmorPreferredWeapon = 0x1CA0;
		inline constexpr auto EnemyScanCursor = 0x1F54;
		inline constexpr auto TargetedEnemyPlayer = 0x1F58;
		inline constexpr auto TargetedEnemyMilitaryPointIndex = 0x1F5C;
		inline constexpr auto WeaponWeights = 0x1458;
		inline constexpr auto ArmorWeights = 0x1C98;
		inline constexpr auto ArmorTwoGood = 0x1CB4;
		inline constexpr auto ArmorOneGood = 0x1CA8;
		inline constexpr auto WeaponGoods = 0x145C;
		inline constexpr auto WeaponFourGood = 0x148C;
		inline constexpr auto WeaponPreferredArmor = 0x1460;
		inline constexpr auto AttackedPositionMilitaryPointIndex = 0x1F64;
		inline constexpr auto EnemyTargets = 0x1CD4;
		inline constexpr auto AttackedPosition = 0x1F60;
		inline constexpr auto AttackedPositionY = 0x1F62;
	}

	namespace AIHighLevelOffset
	{
		inline constexpr auto GlobalGoods = 38176;
		inline constexpr auto FirstGlobalGoodHasProducer = 38216;
		inline constexpr auto PendingVillageConfirmedFlag = 26541;
		inline constexpr auto PendingVillageFlag = 26540;
		inline constexpr auto ShoeScanHumanId = 0xA9C0;

		inline constexpr auto RandomDestructionEnabled = 0x94E0;

		inline constexpr auto InitializationPending = 0x1A;

		inline constexpr auto PlayerId = 0;

		inline constexpr auto Enabled = 4;

		inline constexpr auto Diplomacy = 0xA9C4;

		inline constexpr auto AnimalCatcherHumanRequestedFlag = 0x94F0;

		inline constexpr auto GuidePairSearchExhaustedFlag = 0xAA2C;

		inline constexpr auto GuideScoutRequestedFlag = 0xAA20;

		inline constexpr auto GuideDestination = 0xAA28;

		inline constexpr auto GuideManager = 0xAA18;

		inline constexpr auto Resources = 0x67B4;

		inline constexpr auto ResourcePoints = 0x67B8;

		inline constexpr auto ResourceGoodRequirements = 0x6AB0;

		inline constexpr auto FirstVillageCenter = 52;
		inline constexpr auto Villages = 0x1C;
		inline constexpr auto AnimalCatcher = 0x94E8;
		inline constexpr auto FirstVillageActive = 36;
		inline constexpr auto Trade = 0x74D4;
		inline constexpr auto Military = 0x7578;
	}

	namespace AIMilitaryPointOffset
	{
		inline constexpr auto TargetUniqueId = 8;
		inline constexpr auto TargetIdOrMapPoint = 4;
		inline constexpr auto Target = 0;
		inline constexpr auto TargetPolicy = 12;
		inline constexpr auto MainTaskIndex = 20;
		inline constexpr auto LifetimePolicy = 16;
		inline constexpr auto PlayerId = 28;
		inline constexpr auto Radius = 24;
	}

	namespace AIMilitarySlotAccessOffset
	{
		inline constexpr auto Active = 4;
		inline constexpr auto Role = 8;
		inline constexpr auto TargetPolicy = 24;
		inline constexpr auto MaximumSoldiers = 48;
		inline constexpr auto MinimumSoldiers = 44;
		inline constexpr auto Radius = 36;
		inline constexpr auto MainTaskIndex = 32;
		inline constexpr auto Point = 12;
		inline constexpr auto SecondaryType = 52;
	}


	namespace AIBuildHouseTaskOffset
	{
		inline constexpr auto Position = 12;
		inline constexpr auto PositionY = 14;
		inline constexpr auto HouseType = 16;
		inline constexpr auto DevelopmentStage = 20;
		inline constexpr auto RequestedCount = 24;
	}

	namespace AIDiplomacyTaskOffset
	{
		inline constexpr auto OtherPlayerId = 16;
		inline constexpr auto DiplomacyState = 12;
	}

	namespace AISpawnHumansTaskOffset
	{
		inline constexpr auto HumanCount = 24;
		inline constexpr auto Position = 12;
		inline constexpr auto PositionY = 14;
		inline constexpr auto TribeType = 16;
		inline constexpr auto JobType = 20;
		inline constexpr auto Height = 32;
		inline constexpr auto DisableAfterSpawn = 28;
	}


	// Windows goods search, guide network and job-experience records.
	namespace GuideOffset
	{
		inline constexpr auto UniqueIdUnsigned = 0x10u;
		inline constexpr auto MapPositionY = 0x16;

		inline constexpr auto MapPositionX = 0x14;

		inline constexpr auto GoodsCountsUnsigned = 0x7Cu;
		inline constexpr auto ConnectionCount = 0x78;
		inline constexpr auto MapPosition = 0x14;
		inline constexpr auto GuideId = 8;
		inline constexpr auto PlayerId = 4;
		inline constexpr auto ConnectionsUnsigned = 0x38u;
		inline constexpr auto ConnectionGuideIds = 0x3C;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto ConnectionGuideIdsUnsigned = 0x3Cu;
		inline constexpr auto LinkSystemId = 0x0C;
		inline constexpr auto UniqueId = 0x10;
		inline constexpr auto GoodsCounts = 0x7C;
	}

	namespace GuideManagerOffset
	{
		inline constexpr auto VisitedGuides = 0x1364;
		inline constexpr auto DistanceBucketsUnsigned = 4u;
		inline constexpr auto FirstBucketGuides = 8;
		inline constexpr auto DistanceBuckets = 4;
		inline constexpr auto NextLinkSystemId = 0;
	}

	namespace MapMicroPointOffset
	{
		inline constexpr auto FirstMapMoveableId = 0x0A;

		inline constexpr auto ExplorationMask = 8;

		inline constexpr auto LandscapePlayerId = 4;

		inline constexpr auto ObjectType = 0;
		inline constexpr auto ObjectData = 2;
		inline constexpr auto HouseId = 4;
		inline constexpr auto GuideId = 2;
		inline constexpr auto TerrainFlags = 0x0D;
		inline constexpr auto WalkingFlags = 0x0C;
		inline constexpr auto WalkingDirections = 7;
		inline constexpr auto LandscapeValency = 3;
		inline constexpr auto ContinentId = 6;
	}

	namespace GuideDistanceBucketOffset
	{
		inline constexpr auto GuidesUnsigned = 4u;
	}

	namespace GuideConnectionOffset
	{
		inline constexpr auto Distance = 0;

		inline constexpr auto GuideId = 4;
	}

	namespace JobExperienceTypeListOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto TypesUnsigned = 4u;
	}

	namespace JobExperienceTypeOffset
	{
		inline constexpr auto FirstGoodType = 0x24;
		inline constexpr auto SecondGoodType = 0x28;
		inline constexpr auto ExperienceFactor = 0x2C;
		inline constexpr auto BaseRepeatCounterUnsigned = 0x30u;
	}

	namespace StaticWeaponTypeOffset
	{
		inline constexpr auto HasMunition = 88;
		inline constexpr auto JobTypeUnsigned = 52u;
		inline constexpr auto JobType = 52;
		inline constexpr auto DamageValues = 56;

		inline constexpr auto GoodType = 36;

		inline constexpr auto MaximumRange = 48;

		inline constexpr auto GoodTypeUnsigned = 0x24u;

		inline constexpr auto MainTypeUnsigned = 0x20u;
		inline constexpr auto MainType = 0x20;
	}

	namespace ParticleManagerOffset
	{
		inline constexpr auto FirstParticleActiveUnsigned = offsetof(NC2E2::SParticelManagerStorage, particles) + 4;
		inline constexpr auto FirstGeneratorActiveUnsigned = offsetof(NC2E2::SParticelManagerStorage, generators) + 28;
		inline constexpr auto Wind = offsetof(NC2E2::SParticelManagerStorage, wind);
		inline constexpr auto DrawRowCount = offsetof(NC2E2::SParticelManagerStorage, drawRowCount);
		inline constexpr auto DrawRows = offsetof(NC2E2::SParticelManagerStorage, drawRows);
		inline constexpr auto FirstDrawRowCount = offsetof(NC2E2::SParticelManagerStorage, drawRows) + offsetof(NC2E2::SParticelDrawRowStorage, count);
		inline constexpr auto FirstDrawRowIndices = offsetof(NC2E2::SParticelManagerStorage, drawRows) + offsetof(NC2E2::SParticelDrawRowStorage, indices);
		inline constexpr auto GeneratorManager = offsetof(NC2E2::SParticelManagerStorage, generatorManager);

		inline constexpr auto Particles = offsetof(NC2E2::SParticelManagerStorage, particles);
		inline constexpr auto ParticlesUnsigned = offsetof(NC2E2::SParticelManagerStorage, particles);
		inline constexpr auto LastParticleIndex = offsetof(NC2E2::SParticelManagerStorage, lastParticleIndex);
		inline constexpr auto Generators = offsetof(NC2E2::SParticelManagerStorage, generators);
		inline constexpr auto GeneratorsUnsigned = offsetof(NC2E2::SParticelManagerStorage, generators);
		inline constexpr auto LastGeneratorIndex = offsetof(NC2E2::SParticelManagerStorage, lastGeneratorIndex);
	}

	namespace RecordSize
	{
		inline constexpr auto DiplomacyStateUnsigned = 4u;
		inline constexpr auto PlayerSpecialItemUnsigned = 8u;
		inline constexpr auto HouseDoorOverlayUnsigned = 12u;
		inline constexpr auto MacroTransitionLayer = 2;
		inline constexpr auto EnginePatternId = 2;
		inline constexpr auto UserMessageItemUnsigned = 4u;
		inline constexpr auto FishSwarm = 12;
		inline constexpr auto AITradeEntry = 16;
		inline constexpr auto GuiElementPointer = 4;
		inline constexpr auto MenuNetworkPlayerUnsigned = 108u;
		inline constexpr auto VehicleUnsigned = 1356u;
		inline constexpr auto PaletteChoiceIdUnsigned = 4u;
		inline constexpr auto GoodGraphicsUnsigned = 4u;
		inline constexpr auto NetworkPlayerUnsigned = 152u;
		inline constexpr auto StatisticCountUnsigned = 2u;
		inline constexpr auto SectorPlayerUnsigned = 64u;
		inline constexpr auto GameMapContinentUnsigned = 16u;
		inline constexpr auto MapCoordinateUnsigned = 2u;
		inline constexpr auto StatisticCount = 2;
		inline constexpr auto TribeEnableItemUnsigned = 4u;
		inline constexpr auto TribeGoodProductionUnsigned = 8u;
		inline constexpr auto TribeJobEnablesUnsigned = 1024u;
		inline constexpr auto HouseTypeIndexUnsigned = 4u;
		inline constexpr auto StaticHouseWorkerUnsigned = 8u;
		inline constexpr auto GoodIdUnsigned = 4u;
		inline constexpr auto TechTreeSelectedHouseTypeUnsigned = 4u;
		inline constexpr auto MilitaryTargetValueUnsigned = 4u;
		inline constexpr auto ParticleFrameId = 4;
		inline constexpr auto ParticleFrameIdUnsigned = 4u;
		inline constexpr auto SerializedWordUnsigned = 2u;
		inline constexpr auto JobExperienceTypeIdUnsigned = 4u;
		inline constexpr auto KeyAssignmentUnsigned = 36u;
		inline constexpr auto KeyBindingUnsigned = 8u;
		inline constexpr auto AssistantRequestValue = 4;
		inline constexpr auto MapCoordinateComponentUnsigned = 2u;
		inline constexpr auto HouseArrowSlotUnsigned = 12u;
		inline constexpr auto Tribute = 0x3C;
		inline constexpr auto StaticLandscapeFramesUnsigned = sizeof(void*) == 8 ? 16u : 12u;
		inline constexpr auto PixelChannelValue32 = 4;
		inline constexpr auto MissionOnscreenTextRowUnsigned = 0x191u;
		inline constexpr auto HelpGoodIdUnsigned = 4u;
		inline constexpr auto MissionInfoArgumentUnsigned = 4u;

		inline constexpr auto StaticLandscapeArea = sizeof(void*) == 8 ? 16 : 8;

		inline constexpr auto PixelPoint = 8;

		inline constexpr auto HouseConstructionGoodId = 4;

		inline constexpr auto HouseCounterUnsigned = 4u;

		inline constexpr auto HouseConstructionGoodListUnsigned = 64u;

		inline constexpr auto HouseConstructionGoodList = 64;

		inline constexpr auto MapPlaneWordUnsigned = 2u;

		inline constexpr auto MapPlaneByteUnsigned = 1u;

		inline constexpr auto AssistantRequestValueUnsigned = 4u;

		inline constexpr auto MessagePayloadItemUnsigned = 4u;

		inline constexpr auto FrameIntervalUnsigned = 4u;

		inline constexpr auto DirtyBoundsCoordinate = 2;

		inline constexpr auto StatisticPlayerPointerUnsigned = sizeof(void*);

		inline constexpr auto HouseWindowTabHeight = 4;

		inline constexpr auto StaticLandscapeUnsigned = sizeof(void*) == 8 ? 200u : 140u;

		inline constexpr auto MiscHouseLandscapeLinkUnsigned = sizeof(NC2E2::SMiscHouseLandscapeLinkStorage);

		inline constexpr auto MiscLandscapeLinkUnsigned = 12u;

		inline constexpr auto LandscapeTransitionUnsigned = 8u;

		inline constexpr auto WeightedLandscapeChoiceUnsigned = 8u;

		inline constexpr auto StatisticPlayerPointer = sizeof(void*);

		inline constexpr auto HappinessEventUnsigned = 12u;

		inline constexpr auto HappinessPlayerUnsigned = 0x88u;

		inline constexpr auto GameAssistantPlayerUnsigned = 0x38u;

		inline constexpr auto GameAssistantPlayer = 56;

		inline constexpr auto AIHumanRequest = 24;

		inline constexpr auto TributeGoodAmountUnsigned = 8u;

		inline constexpr auto VehicleRouteControls = 16;

		inline constexpr auto VehicleWindowValue = 4;

		inline constexpr auto PackedMapPointUnsigned = 4u;

		inline constexpr auto HumanReferenceUnsigned = 8u;

		inline constexpr auto HouseWindowStockValue = 4;

		inline constexpr auto HelpHouseEntryUnsigned = 8u;

		inline constexpr auto DiplomacyPlayerSlotUnsigned = 4u;

		inline constexpr auto TribeJobEnableItemUnsigned = 4u;

		inline constexpr auto HumanJobValueUnsigned = 4u;

		inline constexpr auto GuideIdUnsigned = 4u;

		inline constexpr auto ParticleDrawIndexUnsigned = 4u;

		inline constexpr auto HumanGraphicsVariantIdUnsigned = 4u;

		inline constexpr auto HouseActionFrameIdUnsigned = 4u;

		inline constexpr auto EngineHouseDoorOverlayUnsigned = 12u;

		inline constexpr auto MoveablePaletteIdUnsigned = 4u;

		inline constexpr auto PaletteDestinationRangeUnsigned = 4u;

		inline constexpr auto HumanSignGraphicsIdUnsigned = 4u;

		inline constexpr auto HouseActionFrameId = 4;

		inline constexpr auto EngineHouseDoorOverlay = 12;

		inline constexpr auto HouseStageValue = 4;

		inline constexpr auto AIHouseStock = 12;

		inline constexpr auto AISoldierScoreEntry = 8;

		inline constexpr auto AITaskIndexValue = 4;

		inline constexpr auto AIBuildHouseUnsigned = 16u;

		inline constexpr auto AIInvalidArea = 8;

		inline constexpr auto AIHouseTypeId = 4;

		inline constexpr auto AIResourceIndexValue = 4;

		inline constexpr auto ConstructionSelectionEntriesPerEngineUnsigned = 40u;

		inline constexpr auto ConstructionSelectionWindowState = 824;

		inline constexpr auto ConstructionSelectionHouseEntryUnsigned = 8u;

		inline constexpr auto HouseGoodStockIndexUnsigned = 4u;

		inline constexpr auto StaticHouseStockUnsigned = 12u;

		inline constexpr auto StaticHouseTypeUnsigned = 0x624u;

		inline constexpr auto HouseActionOverlayCounterUnsigned = 4u;

		inline constexpr auto HouseStockValueUnsigned = 4u;

		inline constexpr auto HouseConstructionGoodIdUnsigned = 4u;

		inline constexpr auto HouseStageValueUnsigned = 4u;

		inline constexpr auto HouseConstructionGoodsUnsigned = 0x40u;

		inline constexpr auto FishSwarmCount = 4;

		inline constexpr auto LegacyFishSwarms = 0x4B0;

		inline constexpr auto PackedMapPoint = 4;

		inline constexpr auto FishSwarmUnsigned = 12u;

		inline constexpr auto LandscapeTimer = 0x14;

		inline constexpr auto LandscapeTimerUnsigned = 20u;

		inline constexpr auto PlayerGoodsCounts = 136;

		inline constexpr auto PlayerGoodCountUnsigned = 2u;

		inline constexpr auto MultiPlayerGoalGoodRequirementUnsigned = 8u;

		inline constexpr auto MultiPlayerGoalUnsigned = 0x48u;

		inline constexpr auto MultiPlayerGoalPlayerResultUnsigned = 20u;

		inline constexpr auto HumanJobId = 4;

		inline constexpr auto SelectedHumanActionWindowState = 8;

		inline constexpr auto ChangeJobWindowState = 16;

		inline constexpr auto MapAreaRowUnsigned = 8u;

		inline constexpr auto EngineHouseAreaPointerUnsigned = 4u;

		inline constexpr auto GameControlStartupPlayer = 0x0C;

		inline constexpr auto TribeBuildRequirementItemUnsigned = 4u;

		inline constexpr auto PlayerDiplomacyStateUnsigned = 4u;

		inline constexpr auto GameControlStartupPlayerUnsigned = 12u;

		inline constexpr auto HumanProductionCounterUnsigned = 4u;

		inline constexpr auto GuiMousePointerVariables = 16;

		inline constexpr auto SingleHumanExperienceJobId = 4;

		inline constexpr auto SingleHumanExperienceJobIdUnsigned = 4u;

		inline constexpr auto PaletteColorUnsigned = 4u;

		inline constexpr auto SingleHumanChildControlPointers = 44;

		inline constexpr auto HumanJobExperience = 4;

		inline constexpr auto SingleHumanExperiencePageUnsigned = 56u;

		inline constexpr auto SingleHumanRouteControlsUnsigned = 16u;

		inline constexpr auto SingleHumanTabRectangleUnsigned = 16u;

		inline constexpr auto HumanJobExperienceUnsigned = 4u;

		inline constexpr auto SingleHumanTabHeightUnsigned = 4u;

		inline constexpr auto AISoldierSelfDefenceOrigins = 800;

		inline constexpr auto AISinglePlayerVariables = 44360;

		inline constexpr auto HouseConstructionGoods = 64;

		inline constexpr auto StaticHouseStock = 12;

		inline constexpr auto AIBuildHouse = 16;

		inline constexpr auto AISingleGate = 8;

		inline constexpr auto AISingleWall = 6;

		inline constexpr auto EnemyRangeDistanceUnsigned = 4u;

		inline constexpr auto EnemyRangeResultUnsigned = 8u;

		inline constexpr auto ClockInitializedSuffix = 31;

		inline constexpr auto OptionsWindowVars = 8;

		inline constexpr auto PaletteColor = 4;

		inline constexpr auto GuiGfxElementVars = 12;

		inline constexpr auto GuiGfxButtonVars = 40;

		inline constexpr auto GoodId = 4;

		inline constexpr auto TraderConfigWindowVars = 580;

		inline constexpr auto TraderImportButtonPointerUnsigned = 4u;

		inline constexpr auto AISinglePlayerVersion3Prefix = 0xA5B8;

		inline constexpr auto HypertextGuiElementVars = 28;

		inline constexpr auto MissionBriefingHistory = 0xCC;

		inline constexpr auto MissionInfoWindowVars = 840;

		inline constexpr auto EnginePatternIdUnsigned = 2u;

		inline constexpr auto StaticAmbientWaveUnsigned = 0x88u;

		inline constexpr auto StaticAmbientSoundUnsigned = 0x1570u;

		inline constexpr auto AmbientSectorUnsigned = 0x38u;

		inline constexpr auto AmbientLoopEntryUnsigned = 8u;

		inline constexpr auto StaticEngineLandscapeUnsigned = sizeof(void*) == 8 ? 200u : 140u;

		inline constexpr auto StaticEnginePatternUnsigned = 0x70u;

		inline constexpr auto AmbientGroupIdUnsigned = 4u;

		inline constexpr auto AmbientChangeState = 10;

		inline constexpr auto StaticBobCacheEntry = sizeof(void*) == 8 ? 88 : 84;

		inline constexpr auto StaticSoundFxUnsigned = 0x43Cu;

		inline constexpr auto StaticSoundWaveUnsigned = 8u;

		inline constexpr auto StaticSoundIdUnsigned = 4u;

		inline constexpr auto HumanSoundResponseGroupUnsigned = 0x40u;

		inline constexpr auto HumanSoundRecordUnsigned = 0x8Cu;

		inline constexpr auto AITargetReference = 12;

		inline constexpr auto AIDiplomacyCounter = 4;

		inline constexpr auto AIAnimalCount = 12;

		inline constexpr auto AIGoodUnsigned = 0x28u;

		inline constexpr auto AIGoodProducer = 20;

		inline constexpr auto AIGood = 0x28;

		inline constexpr auto AIGlobalGood = 0x50;

		inline constexpr auto AIVillageId = 4;

		inline constexpr auto EngineDirtyBoundsPair = 16;

		inline constexpr auto EngineMapCoordinate = 2;

		inline constexpr auto EngineMapMicroPointUnsigned = 4u;

		inline constexpr auto EngineMapMacroPointUnsigned = 12u;

		inline constexpr auto MacroMapPointer = 4;

		inline constexpr auto MacroMapSection = 8;

		inline constexpr auto MacroMapManager = 8;

		inline constexpr auto MacroMapPointerUnsigned = 4u;

		inline constexpr auto EngineTrianglePatternId = 2;

		inline constexpr auto MacroMapCaptureUsageUnsigned = 8u;

		inline constexpr auto AIGuideGridCellUnsigned = 12u;

		inline constexpr auto AIGuideManager = sizeof(NC2Logic::SAIGuideManagerStorage);

		inline constexpr auto AIGuideGrid = sizeof(NC2Logic::SAIGuideGridStorage);

		inline constexpr auto AIGuideGridCell = 12;

		inline constexpr auto GuiTextButtonVars = 48;

		inline constexpr auto GuiTextElementVars = 206;

		inline constexpr auto HouseInhabitantsSelectionVars = 2232;

		inline constexpr auto HouseInhabitantsSelectionSlot = 20;

		inline constexpr auto MenuProviderListWithSelection = 1208;

		inline constexpr auto MenuChatHistory = 2324;

		inline constexpr auto MenuLobby = 352;

		inline constexpr auto MenuPlayerListWithSelection = 1092;

		inline constexpr auto MenuSessionList = 1204;

		inline constexpr auto SessionSummaryCountCookie = 4;

		inline constexpr auto SessionSummaryUnsigned = 0x14u;

		inline constexpr auto MenuNetworkMessage = 360;

		inline constexpr auto MenuLobbyPlayerRowUnsigned = 0x10u;

		inline constexpr auto MenuNamedGuidRowUnsigned = 0x78u;

		inline constexpr auto GuiMessagePointerRegion = 3 * sizeof(void*);

		inline constexpr auto GuiMessageStatePrefix = 12;

		inline constexpr auto MapMigPointUnsigned = 4u;

		inline constexpr auto HumanUnsigned = 0xC58u;

		inline constexpr auto DeadHumanPool = 0xBB8;

		inline constexpr auto DeadHumanRecordPointer = sizeof(void*);

		inline constexpr auto DeadHumanStatePrefix = 8;

		inline constexpr auto DeadHumanRecord = 60;

		inline constexpr auto DeadHumanRecordUnsigned = 0x3Cu;

		inline constexpr auto ObjectGroupPool = 12040;

		inline constexpr auto ObjectGroupUnsigned = 0x4B4u;

		inline constexpr auto ObjectGroupObjectUnsigned = 12u;

		inline constexpr auto MissionResultParameterUnsigned = 4u;

		inline constexpr auto PlayerTradeCounterUnsigned = 4u;

		inline constexpr auto UserNetworkCommandState = 2404;

		inline constexpr auto TributeUnsigned = 0x3Cu;

		inline constexpr auto NetworkChecksumSample = 12;

		inline constexpr auto UserNetworkCommandSigned = 20;

		inline constexpr auto NetworkCommandPacketHeader = 4;

		inline constexpr auto NetworkCommandPacketHeaderUnsigned = 4u;

		inline constexpr auto NetworkChecksumSampleUnsigned = 12u;

		inline constexpr auto NetworkChecksumPlayerUnsigned = 0x78u;

		inline constexpr auto AttachedVehicleSelectionVars = 712;

		inline constexpr auto AttachedVehicleSelectionSlot = 16;

		inline constexpr auto MapMigPoint = 4;

		inline constexpr auto AIPlayerHandlerPointer = sizeof(void*);

		inline constexpr auto TribeEquipmentEnables = 56;

		inline constexpr auto AIMilitaryTribeEquipment = 264;

		inline constexpr auto ArmorType = 52;

		inline constexpr auto AIEquipmentRow = 12;

		inline constexpr auto AIMilitaryPoint = 32;

		inline constexpr auto StaticWeaponType = 0xC4;

		inline constexpr auto WeaponDamageValue = 4;

		inline constexpr auto WeaponTypeUnsigned = 0xC4u;

		inline constexpr auto ArmorTypeUnsigned = 0x34u;

		inline constexpr auto Human = 3160;

		inline constexpr auto Marker = 24;

		inline constexpr auto HouseTypeUnsigned = 0x624u;

		inline constexpr auto MerchantUnsigned = 0xC4u;

		inline constexpr auto MerchantTradeAgreementBlock = 0x5A4;

		inline constexpr auto MerchantDefaultPool = 196000;

		inline constexpr auto MerchantArrayHeader = sizeof(void*) == 8 ? 80 : 52;

		inline constexpr auto Merchant = 0xC4;

		inline constexpr auto TradeAgreementUnsigned = 0x18u;

		inline constexpr auto MerchantHouseUnsigned = 0x54u;

		inline constexpr auto GuidePointerUnsigned = 4u;

		inline constexpr auto GuideGoodCountUnsigned = 4u;

		inline constexpr auto AIPlayerHandlerPointerUnsigned = sizeof(void*);

		inline constexpr auto AIConditionOperandUnsigned = 4u;

		inline constexpr auto AIConditionOperand = 4;

		inline constexpr auto AIConditionUnsigned = 0x48u;

		inline constexpr auto AICondition = 72;

		inline constexpr auto EnginePixelPointUnsigned = 8u;

		inline constexpr auto DynamicHouseSavedPrefix = NC2SaveFormat::DynamicHouseBytes;

		inline constexpr auto DynamicHouse = 0xD0;

		inline constexpr auto EngineHouseStagePoints = sizeof(NC2E2::SHouseStagePointsStorage);

		inline constexpr auto EngineHouseType = sizeof(NC2E2::SEngineHouseTypeStorage);

		inline constexpr auto EngineHouseConstructionLayerUnsigned = 0x14u;

		inline constexpr auto EngineHouseConstructionLayersUnsigned = 0x68u;

		inline constexpr auto EngineHouseStagePointsUnsigned = sizeof(NC2E2::SHouseStagePointsStorage);

		inline constexpr auto EngineHouseActionOverlayUnsigned = 228u;

		inline constexpr auto DynamicHouseUnsigned = 0xD0u;

		inline constexpr auto AIMainTaskNoWayCorrection = 16;

		inline constexpr auto AIMainTaskPrefix = 0x34;

		inline constexpr auto MapContinent = 16;

		inline constexpr auto MapContinentUnsigned = 16u;

		inline constexpr auto MapContinentRegion = 0xFA4;

		inline constexpr auto MapMicroPointUnsigned = 14u;

		inline constexpr auto MapMacroPointUnsigned = 3u;

		inline constexpr auto MissionData = 0xDD8;

		inline constexpr auto MissionResult = 76;

		inline constexpr auto MissionGoal = 40;

		inline constexpr auto MissionResultUnsigned = 0x4Cu;

		inline constexpr auto MissionGoalUnsigned = 0x28u;

		inline constexpr auto MissionDataUnsigned = 0xDD8u;

		inline constexpr auto HumanListWindowState = 0x18;

		inline constexpr auto StatisticsListPointers = 12;

		inline constexpr auto StatisticsWindowVars = 508;

		inline constexpr auto HumanListButtonVars = 24;

		inline constexpr auto HumanListWindowVars = 4008;

		inline constexpr auto HumanListJobSelectionWindowVars = 8;

		inline constexpr auto DiplomacyPlayerTabs = 108;

		inline constexpr auto DiplomacyWindowVars = 136;

		inline constexpr auto ListBoxScrollAndActiveState = 28;

		inline constexpr auto ListBoxControllerState = 12;

		inline constexpr auto EngineHouseTypeUnsigned = sizeof(NC2E2::SEngineHouseTypeStorage);

		inline constexpr auto HelpWindowVars = 4064;

		inline constexpr auto SelectionHouseWindowVars = 15472;

		inline constexpr auto InputSelectedObject = 8;

		inline constexpr auto InputSelectedObjectUnsigned = 8u;

		inline constexpr auto GuiDisplayedMessageSerializedPrefix = NC2SaveFormat::GuiMessageBytes;

		inline constexpr auto GuiDisplayedMessagePool = 200 * (sizeof(void*) == 8 ? 792 : 784);

		inline constexpr auto GuiMessageHistoryPool = 0x26160;

		inline constexpr auto GuiMessageHistory = 0x30C;

		inline constexpr auto GuiMessageHistoryUnsigned = 0x30Cu;

		inline constexpr auto GuiDisplayedMessage = (sizeof(void*) == 8 ? 792 : 784);

		inline constexpr auto GuiDisplayedMessageUnsigned = static_cast<std::size_t>(sizeof(void*) == 8 ? 792 : 784);

		inline constexpr auto SingleVehicleWindowVars = 15376;

		inline constexpr auto WorldGuiElementState = 532;

		inline constexpr auto SingleHumanWindowVars = 15624;

		inline constexpr auto AIHouseExport = 24;

		inline constexpr auto AIResourceGood = 16;

		inline constexpr auto AIResourcePoint = 20;

		inline constexpr auto AIVillageSlotUnsigned = 0xCF0u;

		inline constexpr auto AIVillageSlot = 0xCF0;

		inline constexpr auto TribeBuildRequirement = 408;
		inline constexpr auto ToUserMessage = 772;
		inline constexpr auto BuildPlayerPointerArray = 20 * sizeof(void*);
		inline constexpr auto BuildPointUnsigned = 6u;
		inline constexpr auto BuildPlayerCount = 4;
		inline constexpr auto RoadBuildPlayer = 0x4B8;
		inline constexpr auto WallBuildPlayer = 0x260;
		inline constexpr auto BuildPoint = 6;
		inline constexpr auto GuideConnectionUnsigned = 8u;
		inline constexpr auto GuideConnection = 8;
		inline constexpr auto GuideDistanceBucketUnsigned = 0x7Cu;
		inline constexpr auto GuideGoodsCounts = 264;
		inline constexpr auto StaticWeaponTypeUnsigned = 0xC4u;
		inline constexpr auto JobExperienceTypeListUnsigned = 0x2Cu;
		inline constexpr auto JobExperienceTypeUnsigned = 0x34u;
		inline constexpr auto AITaskDescription = 32;
		inline constexpr auto AIMilitarySlot = 52;
		inline constexpr auto AISoldier = 84;
		inline constexpr auto AICombinedTask = 0x58;
		inline constexpr auto AIMainTask = 0x44;
		inline constexpr auto AIVehicle = 28;
		inline constexpr auto AISoldierUnsigned = 0x54u;
		inline constexpr auto AICombinedTaskUnsigned = 0x58u;
		inline constexpr auto AIMainTaskUnsigned = 0x44u;
		inline constexpr auto AIVehicleUnsigned = 0x1Cu;
		inline constexpr auto MacroMapPatternUnsigned = 12u;
		inline constexpr auto MacroMapTransitionUnsigned = 16u;
		inline constexpr auto MacroMapLandscapeUnsigned = 16u;
		inline constexpr auto MacroMapHeightUnsigned = 6u;
		inline constexpr auto MacroMapVertexColorUnsigned = 6u;
		inline constexpr auto MacroMapRoadUnsigned = 4u;
		inline constexpr auto MacroMapCreatureUnsigned = 16u;
		inline constexpr auto MacroMapHouseUnsigned = 20u;
		inline constexpr auto MacroMapFishSwarmUnsigned = 8u;
		inline constexpr auto MacroMapNameUnsigned = 44u;
		inline constexpr auto MacroMapName = 44;
		inline constexpr auto MacroMapPattern = 12;
		inline constexpr auto MacroMapTransition = 16;
		inline constexpr auto MacroMapLandscape = 16;
		inline constexpr auto MacroMapHeight = 6;
		inline constexpr auto MacroMapVertexColor = 6;
		inline constexpr auto MacroMapRoad = 4;
		inline constexpr auto MacroMapCreature = 16;
		inline constexpr auto MacroMapHouse = 20;
		inline constexpr auto MacroMapFishSwarm = 8;
		inline constexpr auto MacroMap = 376;
		inline constexpr auto Particle = 304;
		inline constexpr auto ParticlePosition = 40;
		inline constexpr auto ParticlePositionStateCopy = 112;
		inline constexpr auto ParticleUnsigned = 0x130u;
		inline constexpr auto StaticParticleUnsigned = sizeof(NC2E2::SStaticParticelStorageEntry);
		inline constexpr auto ParticleGeneratorUnsigned = 0x68u;
		inline constexpr auto ParticleEffectUnsigned = 0x34u;
		inline constexpr auto ParticleGeneratorSlotUnsigned = 0x20u;
		inline constexpr auto ParticleDrawRowUnsigned = 0x198u;
		inline constexpr auto ParticlePoolClearBytes = 3040008;
		inline constexpr auto ParticleGeneratorPoolBytes = 0x7D04;
		inline constexpr auto ParticleDrawPrefixBytes = offsetof(NC2E2::SParticelManagerStorage, drawRows) + sizeof(NC2E2::SParticelManagerStorage::drawRows);
		inline constexpr auto DisplayDrawCounters = 32;
		inline constexpr auto DisplayObjectReference = 68;
		inline constexpr auto DisplayMarkedObject = 0x48u;
		inline constexpr auto DisplaySearchObject = sizeof(NC2E2::SDisplaySearchObject);
		inline constexpr auto DisplayInitializedTail = offsetof(NC2E2::SRuntimeDisplayStorage, scroll) - offsetof(NC2E2::SRuntimeDisplayStorage, destinationBitmap);
		inline constexpr auto DisplayScrollState = sizeof(NC2E2::SDisplayScrollState);
		inline constexpr auto WeatherSector = 12u;
		inline constexpr auto WeatherSectorDimensions = 8;
		inline constexpr auto WeatherDensity = 4u;
		inline constexpr auto StaticMoveableManager = sizeof(NC2E2::SStaticMoveableManagerData);
		inline constexpr auto StaticMoveableData = sizeof(NC2E2::SStaticMoveableData);
		inline constexpr auto FastMoveableTribeUnsigned = sizeof(NC2E2::SFastMoveableTribeAccess<NC2E2::SStaticMoveableAnimationListTag>);
		inline constexpr auto FastMoveableTribe = sizeof(NC2E2::SFastMoveableTribeAccess<NC2E2::SStaticMoveableAnimationListTag>);
		inline constexpr auto FastMoveableJobUnsigned = sizeof(NC2E2::SFastMoveableJobAccess<NC2E2::SStaticMoveableAnimationListTag>);
		inline constexpr auto FastMoveableJob = sizeof(NC2E2::SFastMoveableJobAccess<NC2E2::SStaticMoveableAnimationListTag>);
		inline constexpr auto StaticMoveableAnimation = sizeof(NC2E2::SE2StaticDataMapMoveableAnimationData);
		inline constexpr auto StaticMoveableWalk = sizeof(NC2E2::SE2StaticDataMapMoveableWalkData);
		inline constexpr auto FrameList = sizeof(NC2E2::SMapMoveableFrameList);
		inline constexpr auto StaticAnimalVehicleGraphics = sizeof(NC2E2::SStaticAnimalVehicleGraphics);
		inline constexpr auto StaticAnimalVehicleGraphicsUnsigned = sizeof(NC2E2::SStaticAnimalVehicleGraphics);
		inline constexpr auto StaticGoodGraphics = 264;
		inline constexpr auto StaticGoodGraphicsHex = 0x108;
		inline constexpr auto HumanJobBaseGraphics = sizeof(NC2E2::SHumanJobBaseGraphics);
		inline constexpr auto HumanJobBaseGraphicsUnsigned = sizeof(NC2E2::SHumanJobBaseGraphics);
		inline constexpr auto HumanJobChangeGraphics = sizeof(NC2E2::SHumanJobChangeGraphics);
		inline constexpr auto HumanJobChangeGraphicsUnsigned = sizeof(NC2E2::SHumanJobChangeGraphics);
		inline constexpr auto HumanBodyGraphicsUnsigned = sizeof(NC2E2::SHumanBodyGraphics);
		inline constexpr auto HumanHeadGraphicsUnsigned = sizeof(NC2E2::SHumanHeadGraphics);
		inline constexpr auto RandomPaletteSet = sizeof(NC2E2::SRandomPaletteSet);
		inline constexpr auto RandomPaletteSetUnsigned = sizeof(NC2E2::SRandomPaletteSet);
		inline constexpr auto RandomPalettePatch = 16;
		inline constexpr auto RandomPalettePatchUnsigned = 16u;
		inline constexpr auto RandomPalettePatchHexUnsigned = 0x10u;
		inline constexpr auto TribeHumanNameArrays = sizeof(NC2E2::STribeHumanNameArrays);
		inline constexpr auto TribeHumanNameArraysUnsigned = sizeof(NC2E2::STribeHumanNameArrays);
		inline constexpr auto StaticHumanColor = 0x58;
		inline constexpr auto StaticHumanColorUnsigned = 0x58u;
		inline constexpr auto MoveableAnimationPartUnsigned = 28u;
		inline constexpr auto DisplayAtomInformation = sizeof(NC2E2::SDisplayAtomStorage);
		inline constexpr auto DynamicAnimalVehicle = sizeof(NC2E2::SDynamicAnimalVehicleStorage);
		inline constexpr auto DynamicAnimalVehicleUnsigned = sizeof(NC2E2::SDynamicAnimalVehicleStorage);
		inline constexpr auto DynamicHuman = sizeof(NC2E2::SDynamicHumanStorage);
		inline constexpr auto DynamicHumanUnsigned = sizeof(NC2E2::SDynamicHumanStorage);
		inline constexpr auto DynamicHumanSavegamePayload = sizeof(NC2SaveFormat::SHumanGraphicsRecord);
		inline constexpr auto InteractionTargetData = 28;
		inline constexpr auto HumanAssistantData = 0xD0;
		inline constexpr auto HumanAssistantCommandHistory = 0x14u;
		inline constexpr auto HumanInventorySlot = 8u;
		inline constexpr auto UserNetworkCommand = 20u;
		inline constexpr auto VehiclePassenger = 12u;
		inline constexpr auto VehiclePassengerSigned = 12;
		inline constexpr auto VehicleStockReservation = 12u;
		inline constexpr auto AtomicAnimation = 0x138u;
		inline constexpr auto StaticVehicleType = 0x1DCu;
		inline constexpr auto TargetData = 12u;
		inline constexpr auto TargetDataSigned = 12;
		inline constexpr auto HouseQualityEnhancer = 8u;
		inline constexpr auto HouseAttachment = 16u;
		inline constexpr auto HouseConstructionGood = 20u;
		inline constexpr auto MapMoveable = 0x58u;
		inline constexpr auto MapMoveableSigned = 0x58;
		inline constexpr auto MapPathfinder = 0x84u;
		inline constexpr auto MapPathfinderSigned = 0x84;
		inline constexpr auto SectorPathfinder = 0x60u;
		inline constexpr auto SectorPathfinderSigned = 0x60;
		inline constexpr auto PathfinderDirectionRun = 2u;
		inline constexpr auto JobType = 0x88u;
		inline constexpr auto JobTypeSigned = 0x88;
		inline constexpr auto GameControlPlayer = 0x4A80u;
		inline constexpr auto GameControlPlayerSigned = 0x4A80;
		inline constexpr auto GameControlPlayerArray = 0x5D200u;
	}

	namespace VehicleOffset
	{
		inline constexpr auto PlayerIdUnsigned = 480u;
		inline constexpr auto UniqueIdUnsigned = 8u;
		inline constexpr auto PrimaryMilitaryTargetRevisionHighWord = 0x52A;

		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto VehicleId = 4;
		inline constexpr auto UniqueId = 8;
		inline constexpr auto Hitpoints = 0xC;
		inline constexpr auto UserCommandQueue = 0x10u;
		inline constexpr auto UserCommandQueueSigned = 0x10;
		inline constexpr auto CurrentTask = 0x1C8;
		inline constexpr auto DoorEntryPoint = 0x1CC;
		inline constexpr auto DockedFlag = 0x1D0;
		inline constexpr auto TargetWorkPosition = 0x1D2;
		inline constexpr auto MilitaryCenter = 0x1D6;
		inline constexpr auto MapMoveableId = 0x1DC;
		inline constexpr auto PlayerId = 0x1E0;
		inline constexpr auto TribeType = 0x1E4;
		inline constexpr auto LogicType = 0x1E8;
		inline constexpr auto JobType = 0x1EC;
		inline constexpr auto HumanPassengers = 0x1F0u;
		inline constexpr auto HumanPassengersSigned = 0x1F0;
		inline constexpr auto HumanPassengerInsideFlags = 0x1F8u;
		inline constexpr auto CommanderAttachedFlag = 0x2EC;
		inline constexpr auto DraggingAnimalInsideFlag = 0x2F0;
		inline constexpr auto DraggingAnimalId = 0x2F4;
		inline constexpr auto DraggingAnimalRevision = 0x2F8;
		inline constexpr auto VehiclePassengers = 0x2FCu;
		inline constexpr auto VehiclePassengersSigned = 0x2FC;
		inline constexpr auto VehiclePassengerInsideFlags = 0x304u;
		inline constexpr auto VehiclePassengerInsideFlagsSigned = 0x304;
		inline constexpr auto StockAmounts = 0x338u;
		inline constexpr auto StockAmountsSigned = 0x338;
		inline constexpr auto FutureStockAmounts = 0x37Au;
		inline constexpr auto WantedStockAmounts = 0x3BCu;
		inline constexpr auto AtomicAction = 0x4FC;
		inline constexpr auto AtomicAnimationRunningFlag = 0x500;
		inline constexpr auto AtomicAnimationId = 0x504;
		inline constexpr auto AtomicAnimationFrame = 0x508;
		inline constexpr auto AtomicAnimationStartCount = 0x50C;
		inline constexpr auto PreviousSectorVehicleId = 0x510;
		inline constexpr auto NextSectorVehicleId = 0x514;
		inline constexpr auto ExplorationRadius = 0x518;
		inline constexpr auto MissionCheckId = 0x51C;
		inline constexpr auto PrimaryMilitaryTarget = 0x520;
		inline constexpr auto PrimaryMilitaryTargetValue = 0x524;
		inline constexpr auto PrimaryMilitaryTargetRevision = 0x528;
		inline constexpr auto SecondaryMilitaryTarget = 0x52C;
		inline constexpr auto SecondaryMilitaryTargetValue = 0x530;
		inline constexpr auto SecondaryMilitaryTargetRevision = 0x534;
		inline constexpr auto ActiveMilitaryTargetIndex = 0x538;
		inline constexpr auto MilitaryMode = 0x53C;
		inline constexpr auto CarrierVehicleId = 0x540;
		inline constexpr auto CarrierVehicleRevision = 0x544;
		inline constexpr auto InsideVehicleFlag = 0x548;
	}

	namespace VehiclePassengerOffset
	{
		inline constexpr auto ObjectId = 0;
		inline constexpr auto ObjectRevision = 4u;
		inline constexpr auto ObjectRevisionSigned = 4;
		inline constexpr auto InsideFlag = 8u;
		inline constexpr auto InsideFlagSigned = 8;
	}

	namespace VehicleStockOffset
	{
		inline constexpr auto HumanReservations = 0xC8u;
		inline constexpr auto ReservationHumanRevisions = 0xCCu;
	}

	namespace VehicleStockReservationOffset
	{
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto HumanRevision = 4;
		inline constexpr auto RetainedAmount = 8;
	}

	namespace AtomicAnimationOffset
	{
		inline constexpr auto InterruptableUnsigned = 0x38u;
		inline constexpr auto LengthInTicks = 0x34;
	}

	namespace AnimalStateOffset
	{
		inline constexpr auto WorkCenterX = 0x0C;
		inline constexpr auto WorkCenterY = 0x0E;
		inline constexpr auto StayPointX = 0x10;
		inline constexpr auto StayPointY = 0x12;
		inline constexpr auto BirthPointX = 0x14;
		inline constexpr auto BirthPointY = 0x16;
		inline constexpr auto MilitaryTarget = 0x2C;
		inline constexpr auto AttachmentTarget = 0x38;
	}

	namespace VehicleVarsOffset
	{
		inline constexpr auto DoorEntryPointX = 0x1BC;
		inline constexpr auto DoorEntryPointY = 0x1BE;
		inline constexpr auto TargetWorkPositionX = 0x1C2;
		inline constexpr auto TargetWorkPositionY = 0x1C4;
		inline constexpr auto MilitaryCenterX = 0x1C6;
		inline constexpr auto MilitaryCenterY = 0x1C8;
	}

	namespace HumanTaskOffset
	{
		inline constexpr auto UserCommandQueue = 0x28u;
	}

	namespace MapPathfinderOffset
	{
		inline constexpr auto DestinationPositionY = 0x46;

		inline constexpr auto CurrentPositionY = 0x42;

		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto SearchState = 4;
		inline constexpr auto TargetMode = 8;
		inline constexpr auto SearchRadius = 0x0C;
		inline constexpr auto RequiredClearance = 0x10;
		inline constexpr auto UseResistanceFlag = 0x14;
		inline constexpr auto UseSectorsFlag = 0x15;
		inline constexpr auto AllowVerticalDirectionsFlag = 0x16;
		inline constexpr auto UseGuidesFlag = 0x17;
		inline constexpr auto DeterministicDirectionChoiceFlag = 0x18;
		inline constexpr auto SourceGuideId = 0x1C;
		inline constexpr auto SourceGuideRevision = 0x20;
		inline constexpr auto CurrentGuideId = 0x24;
		inline constexpr auto CurrentGuideRevision = 0x28;
		inline constexpr auto PlayerId = 0x2C;
		inline constexpr auto PreferredDirection1 = 0x30;
		inline constexpr auto PreferredDirection2 = 0x34;
		inline constexpr auto PreferredDirectionWeight1 = 0x38;
		inline constexpr auto PreferredDirectionWeight2 = 0x3C;
		inline constexpr auto CurrentPosition = 0x40;
		inline constexpr auto DestinationPosition = 0x44;
		inline constexpr auto TargetMoveableId = 0x48;
		inline constexpr auto TargetObjectRevision = 0x4C;
		inline constexpr auto TargetDistance = 0x50;
		inline constexpr auto DirectionRunIndex = 0x54;
		inline constexpr auto BlockedAreaSteps = 0x58;
		inline constexpr auto DirectionRuns = 0x5C;
		inline constexpr auto DirectionRunCounts = 0x5D;
		inline constexpr auto DirectionRunCountsUnsigned = 0x5Du;
	}

	namespace TargetDataOffset
	{
		inline constexpr auto AttackPointAttempts = 10;

		inline constexpr auto AttackBaseDirection = 8;

		inline constexpr auto ObjectRevisionHighWord = 10;

		inline constexpr auto ObjectIdOrMapPoint = 4;

		inline constexpr auto Type = 0;
		inline constexpr auto ObjectIdOrMapPosition = 4;
		inline constexpr auto MapPositionX = 4;
		inline constexpr auto MapPositionY = 6;
		inline constexpr auto ObjectRevision = 8;
	}

	namespace InteractionTargetDataOffset
	{
		inline constexpr auto Type = 0;
		inline constexpr auto RealPosition = 4;
		inline constexpr auto WorkPosition = 8;
		inline constexpr auto LandscapeType = 12;
		inline constexpr auto GoodType = 16;
		inline constexpr auto ObjectId = 0x14;
		inline constexpr auto ObjectRevision = 0x18;
	}

	namespace HumanAssistantDataOffset
	{
		inline constexpr auto InitializedFlag = 0;
		inline constexpr auto HasCommandHistoryFlag = 4;
		inline constexpr auto CommandHistory = 8u;
	}

	namespace HumanAssistantCommandHistoryOffset
	{
		inline constexpr auto Command = 0u;
		inline constexpr auto Parameter1 = 4u;
		inline constexpr auto Parameter2 = 8u;
		inline constexpr auto StartTick = 0x0C;
		inline constexpr auto StartTickUnsigned = 0x0Cu;
		inline constexpr auto DurationTicks = 0x10;
		inline constexpr auto DurationTicksUnsigned = 0x10u;
	}

	namespace HumanMilitaryStateOffset
	{
		inline constexpr auto Targets = 8u;
	}

	namespace SectorPathfinderOffset
	{
		inline constexpr auto SearchState = 0;
		inline constexpr auto SearchRadius = 4;
		inline constexpr auto RequiredClearance = 8;
		inline constexpr auto UseResistanceFlag = 0x0C;
		inline constexpr auto AllowVerticalDirectionsFlag = 0x0D;
		inline constexpr auto PreferredDirection1 = 0x10;
		inline constexpr auto PreferredDirection2 = 0x14;
		inline constexpr auto CurrentPosition = 0x20;
		inline constexpr auto DestinationPosition = 0x24;
		inline constexpr auto SearchRectangle = 0x28;
		inline constexpr auto DirectionRunIndex = 0x30;
		inline constexpr auto BlockedAreaSteps = 0x34;
		inline constexpr auto DirectionRuns = 0x38;
		inline constexpr auto DirectionRunCounts = 0x39;
		inline constexpr auto DirectionRunCountsUnsigned = 0x39u;
	}

	namespace BuildManagerOffset
	{
		inline constexpr auto PlayerRecords = sizeof(void*);
		inline constexpr auto PlayerCount = 0;
	}

	namespace BuildPlayerOffset
	{
		inline constexpr auto FirstPointAssignedUnsigned = 12u;

		inline constexpr auto PointsUnsigned = 8u;
		inline constexpr auto SecondPoint = 14;
		inline constexpr auto FirstPointAssigned = 12;
		inline constexpr auto Points = 8;
		inline constexpr auto AssignedWorkerCount = 4;
		inline constexpr auto PointCount = 0;
	}

	namespace BuildPointOffset
	{
		inline constexpr auto Assigned = 4;
	}

	namespace LandscapeLogicDataOffset
	{
		inline constexpr auto MaximumValency = 4;
		inline constexpr auto LogicType = 0;
	}

	namespace ToUserMessageOffset
	{
		inline constexpr auto HouseItems = 0x200;

		inline constexpr auto SpecialItem = 0x300;

		inline constexpr auto SpecialItemType = 0x2FC;

		inline constexpr auto RelatedPlayerId = 0x0C;

		inline constexpr auto EquipmentItems = 0x2E0;

		inline constexpr auto Type = 0;
		inline constexpr auto FinishedFlag = 4;
		inline constexpr auto PlayerId = 8;
		inline constexpr auto PlayerIdUnsigned = 8u;
		inline constexpr auto HumanId = 0x10;
		inline constexpr auto HumanIdUnsigned = 0x10u;
		inline constexpr auto HumanUniqueId = 0x14;
		inline constexpr auto HouseId = 0x18;
		inline constexpr auto HouseIdUnsigned = 0x18u;
		inline constexpr auto HouseUniqueId = 0x1C;
		inline constexpr auto VehicleId = 0x20;
		inline constexpr auto VehicleIdUnsigned = 0x20u;
		inline constexpr auto VehicleUniqueId = 0x24;
		inline constexpr auto MapPositionX = 0x28;
		inline constexpr auto MapPositionY = 0x2A;
		inline constexpr auto GoodCount = 0x2C;
		inline constexpr auto FirstPayloadItem = 0x30;
		inline constexpr auto FirstPayloadItemUnsigned = 0x30u;
		inline constexpr auto JobCount = 0x138;
		inline constexpr auto JobItems = 0x13C;
		inline constexpr auto JobItemsUnsigned = 0x13Cu;
		inline constexpr auto HouseCount = 0x1FC;
		inline constexpr auto HouseItemsUnsigned = 0x200u;
		inline constexpr auto EquipmentCount = 0x2DC;
		inline constexpr auto EquipmentItemsUnsigned = 0x2E0u;
	}

	namespace StaticJobTypeOffset
	{
		inline constexpr auto ListenNowAllowedUnsigned = 51u;
		inline constexpr auto ListenAllowedUnsigned = 53u;
		inline constexpr auto EatCandyAllowedUnsigned = 47u;
		inline constexpr auto PrayAllowedUnsigned = 48u;
		inline constexpr auto TalkAllowedUnsigned = 50u;
		inline constexpr auto MustHaveFinishedWorkHouse = 132;
		inline constexpr auto CanHaveWorkHouseUnsigned = 130u;
		inline constexpr auto NeedsReligionUnsigned = 128u;
		inline constexpr auto IgnoresHomeHouseUnsigned = 129u;
		inline constexpr auto CanBeTrainedUnsigned = 0x87u;

		inline constexpr auto UserShouldAttachWorkPlaceAfterJobChangeUnsigned = 0x86u;

		inline constexpr auto AllowedAtomicsUnsigned = 0x24u;

		inline constexpr auto BaseJob = 0x20;

		inline constexpr auto MustHaveWorkHouse = 0x83;

		inline constexpr auto AllowedAtomics = 0x24;

		inline constexpr auto UserCanChangeProduction = 0x85;

		inline constexpr auto MustHaveWorkHouseUnsigned = 0x83u;
		inline constexpr auto MustHaveFinishedWorkHouseUnsigned = 0x84u;
		inline constexpr auto UserCanChangeProductionUnsigned = 0x85u;
	}

	namespace TribeJobEnablesOffset
	{
		inline constexpr auto EquipmentUnsigned = 0x338u;
	}

	namespace PlayerAllowedThingsOffset
	{
		inline constexpr auto AllowedHouses = 10248;
		inline constexpr auto AvailableGood1Flag = 0x1D35;
		inline constexpr auto AvailableGood9Flag = 0x1D3D;
		inline constexpr auto AvailableGood10Flag = 0x1D3E;
		inline constexpr auto AvailableGood12Flag = 0x1D40;
		inline constexpr auto AvailableGood21Flag = 0x1D49;
		inline constexpr auto AvailableHouses = 0x310E;
	}

	namespace TribeBuildRequirementOffset
	{
		inline constexpr auto JobsUnsigned = 8u;
		inline constexpr auto GoodsUnsigned = 0xD0u;
	}

	namespace AIVillageOffset
	{
		inline constexpr auto FirstGoodNeedCount = 640;
		inline constexpr auto AllocatedFlag = 0;
		inline constexpr auto VillageId = 4;
		inline constexpr auto PlayerId = 8;
		inline constexpr auto CenterPoint = 12;
		inline constexpr auto CenterY = 14;
		inline constexpr auto Radius = 0x10;
		inline constexpr auto Houses = 28;
		inline constexpr auto AllHousesLostFlag = 36;
		inline constexpr auto HouseBuildAllowedFlags = 0x25;
		inline constexpr auto HouseBuildAllowedFlagsUnsigned = 0x25u;
		inline constexpr auto StockBuildAllowedFlag = 0x2C;
		inline constexpr auto StockCount = 0x5C;
		inline constexpr auto StockConstructionPendingFlag = 0x60;
		inline constexpr auto Goods = 0x274;
		inline constexpr auto BuildHouse = 0xCC4;
		inline constexpr auto UpgradeHouse = 0xCD0;
		inline constexpr auto TowerHouseId = 0xCE0;
		inline constexpr auto Type = 0xCE4;
		inline constexpr auto ResourceGood = 0xCE8;
	}

	namespace AIVillagesOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto NextVillageId = 4;
		inline constexpr auto FirstVillage = 12;
		inline constexpr auto FirstVillagePlayerId = 20;
		inline constexpr auto FirstVillageCenter = 24;
		inline constexpr auto PendingVillageType = 0x6788;
		inline constexpr auto PendingVillageGood = 0x678C;
		inline constexpr auto PendingVillageFlag = 0x6790;
		inline constexpr auto PendingVillageConfirmedFlag = 0x6791;
		inline constexpr auto PendingVillagePoint = 0x6792;
		inline constexpr auto PendingVillageY = 0x6794;
	}

	namespace AIVillageSlotViewOffset
	{
		inline constexpr auto HouseBuildAllowedFlags = 0x31;

		inline constexpr auto Goods = 0x280;

		inline constexpr auto Active = 8;
		inline constexpr auto Village = 12;
		inline constexpr auto VillageId = 16;
		inline constexpr auto Center = 24;
		inline constexpr auto Houses = 40;
		inline constexpr auto AllHousesLostFlag = 0x30;
		inline constexpr auto BarrackHouseId = 0x27C;
		inline constexpr auto VillageType = 0xCF0;
	}

	namespace AIHouseManagerOffset
	{
		inline constexpr auto HasHomeFlag = 0;
		inline constexpr auto PreviousHouseCount = 4;
		inline constexpr auto AllHousesLostFlag = 8;
		inline constexpr auto BuildAllowedFlags = 9;
		inline constexpr auto StockCount = 0x40;
		inline constexpr auto StockConstructionPendingFlag = 0x44;
		inline constexpr auto Stocks = 0x48;
		inline constexpr auto FirstStockHouseId = 0x4C;
		inline constexpr auto FirstStockUniqueId = 0x50;
		inline constexpr auto DefendPointCount = 0xC0;
		inline constexpr auto DefendPoints = 0xC4;
		inline constexpr auto BarrackHouseId = 0x254;
	}

	namespace AIResourceManagerOffset
	{
		inline constexpr auto FirstGoodCheckTerrainFlag = 0x308;
		inline constexpr auto PlayerId = 0;
		inline constexpr auto ResourcePoints = 4;
		inline constexpr auto FirstResourceGood = 12;
		inline constexpr auto LandscapeResourceIndices = 0x194;
		inline constexpr auto ResourceCount = 0x2F4;
		inline constexpr auto ScanIndex = 0x2F8;
		inline constexpr auto GoodRequirements = 0x2FC;
		inline constexpr auto HouseGoods = 0x71C;
		inline constexpr auto HouseExports = 0x7F8;
		inline constexpr auto FirstHouseExportTypes = 0x7FC;
	}

	namespace AIResourceSlotViewOffset
	{
		inline constexpr auto Point = 4;
		inline constexpr auto UpdatedFlag = 8;
		inline constexpr auto OnMapFlag = 9;
		inline constexpr auto GoodType = 12;
		inline constexpr auto MinimumValency = 16;
		inline constexpr auto StoredLandscapeType = 20;
	}

	namespace AIResourceGoodOffset
	{
		inline constexpr auto Type = 0;
		inline constexpr auto WorkerJob = 4;
		inline constexpr auto HouseType = 8;
		inline constexpr auto CheckTerrainFlag = 12;
	}

	namespace AIHouseExportOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto HouseTypes = 4;
	}

	namespace AIResourcePointArrayOffset
	{
		inline constexpr auto LandscapeResourceIndices = 0x190;
	}

	namespace AIResourceSearchOffset
	{
		inline constexpr auto Villages = 0;
		inline constexpr auto ResourcePoints = 4;
		inline constexpr auto ResourceIndex = 8;
		inline constexpr auto ContinentId = 12;
		inline constexpr auto BestVillageId = 16;
		inline constexpr auto BestScore = 20;
		inline constexpr auto PlayerId = 24;
		inline constexpr auto BestPoint = 28;
	}

	namespace AIInvalidAreaManagerOffset
	{
		inline constexpr auto FirstAreaY = 6;

		inline constexpr auto Count = 0;
		inline constexpr auto Areas = 4;
		inline constexpr auto FirstAreaRadius = 8;
	}

	namespace AIInvalidAreaOffset
	{
		inline constexpr auto Center = 0;
		inline constexpr auto Radius = 4;
	}

	namespace AIResourceGoodViewOffset
	{
		inline constexpr auto Type = 0x2FC;
		inline constexpr auto HouseType = 0x304;
		inline constexpr auto CheckTerrainFlag = 0x308;
	}

	namespace AIResourcePointOffset
	{
		inline constexpr auto MinimumValency = 12;
		inline constexpr auto StoredLandscapeType = 16;
	}

	namespace AITowerManagerOffset
	{
		inline constexpr auto LastHouseId = 0;
	}

	namespace SingleHumanWindowOffset
	{
		inline constexpr auto TabEnabledFlags = 0x70;
		inline constexpr auto MilitaryTabEnabled = 0x71;
		inline constexpr auto WorkTabEnabled = 0x72;
		inline constexpr auto ExperienceTabEnabled = 0x73;
		inline constexpr auto JobTabEnabled = 0x74;
		inline constexpr auto TraderTabEnabled = 0x75;
		inline constexpr auto TabCollapsedFlags = 0x76;
		inline constexpr auto MilitaryTabCollapsed = 0x77;
		inline constexpr auto WorkTabCollapsed = 0x78;
		inline constexpr auto ExperienceTabCollapsed = 0x79;
		inline constexpr auto JobTabCollapsed = 0x7A;
		inline constexpr auto TraderTabCollapsed = 0x7B;
		inline constexpr auto TabStateChecksum = 0x7C;
		inline constexpr auto TabRectangles = 0x80;
		inline constexpr auto ComputedWidth = 0xE0;
		inline constexpr auto ComputedHeight = 0xE4;
		inline constexpr auto TabHeightOverrides = 0xE8;
		inline constexpr auto LayoutInvalidFlag = 0x100;
		inline constexpr auto PreviewElement = 0x104;
		inline constexpr auto NameElement = 0x108;
		inline constexpr auto RouteSelectButtons = 0x10C;
		inline constexpr auto RouteSelectButtonsUnsigned = 0x10Cu;
		inline constexpr auto RouteAssignButtons = 0x110;
		inline constexpr auto RouteAssignButtonsUnsigned = 0x110u;
		inline constexpr auto RouteActionButtons = 0x114;
		inline constexpr auto RouteActionButtonsUnsigned = 0x114u;
		inline constexpr auto RouteLabels = 0x118;
		inline constexpr auto RouteLabelsUnsigned = 0x118u;
		inline constexpr auto SecondRouteSelectButton = 0x11C;
		inline constexpr auto SecondRouteAssignButton = 0x120;
		inline constexpr auto SecondRouteActionButton = 0x124;
		inline constexpr auto SecondRouteLabel = 0x128;
		inline constexpr auto TraderConfigButton = 0x12C;
		inline constexpr auto ExperiencePage = 0x130;
		inline constexpr auto ExperienceJobsUnsigned = 0x134u;
		inline constexpr auto Tooltips = 0x39C;
		inline constexpr auto DynamicButtons = 0x3280;
		inline constexpr auto TraderConfigWindow = 0x3D74;
	}

	namespace SingleHumanWindowVarsOffset
	{
		inline constexpr auto Tooltips = 0x32C;
	}

	namespace DynamicSubButtonEntryOffset
	{
		inline constexpr auto ActiveFlag = 0;
	}

	namespace ToolGfxButtonOffset
	{
		inline constexpr auto CommandData1 = 0x48;
		inline constexpr auto CommandData2 = 0x4C;
	}

	namespace ToolTextButtonOffset
	{
		inline constexpr auto CommandData1 = 0x4C;
		inline constexpr auto CommandData2 = 0x50;
		inline constexpr auto CenteredFlag = 0x54;
	}

	namespace GuiBaseElementOffset
	{
		inline constexpr auto MessageMask = offsetof(NXBaseGui::SBaseElementStorage, messageMask);

		inline constexpr auto Serial = offsetof(NXBaseGui::SBaseElementStorage, serial);

		inline constexpr auto VirtualTable = offsetof(NXBaseGui::SBaseElementStorage, virtualTable);
		inline constexpr auto Rectangle = offsetof(NXBaseGui::SBaseElementStorage, rectangle);
		inline constexpr auto PositionX = offsetof(NXBaseGui::SBaseElementStorage, rectangle);
		inline constexpr auto PositionY = offsetof(NXBaseGui::SBaseElementStorage, rectangle) + 4;
		inline constexpr auto Width = offsetof(NXBaseGui::SBaseElementStorage, rectangle) + 8;
		inline constexpr auto Height = offsetof(NXBaseGui::SBaseElementStorage, rectangle) + 12;
		inline constexpr auto InteractionState = offsetof(NXBaseGui::SBaseElementStorage, interactionState);
		inline constexpr auto Flags = offsetof(NXBaseGui::SBaseElementStorage, flags);
		inline constexpr auto FlagsByte1 = offsetof(NXBaseGui::SBaseElementStorage, flags) + 1;
		inline constexpr auto ParentWindow = offsetof(NXBaseGui::SBaseElementStorage, parentWindow);
		inline constexpr auto MessageHandler = offsetof(NXBaseGui::SBaseElementStorage, messageHandler);
	}

	namespace GuiBaseWindowOffset
	{
		inline constexpr auto DesktopListFlags = offsetof(NXBaseGui::SBaseWindowStorage, desktopListFlags);
		inline constexpr auto ClippingRectangle = offsetof(NXBaseGui::SBaseWindowStorage, clippingRectangle);
		inline constexpr auto DrawChildren = offsetof(NXBaseGui::SBaseWindowStorage, drawChildren);
		inline constexpr auto InputChildren = offsetof(NXBaseGui::SBaseWindowStorage, inputChildren);
		inline constexpr auto ActiveChild = offsetof(NXBaseGui::SBaseWindowStorage, activeChild);
	}

	namespace WorldGuiElementOffset
	{
		inline constexpr auto PrimaryCommandParameter1 = 0x1D8;

		inline constexpr auto Bitmap = 0x2C;
		inline constexpr auto Display = 0x30;
		inline constexpr auto InputEnabledFlag = 0x34;
		inline constexpr auto ObjectUnderMouse = 0x38;
		inline constexpr auto HoveredHumanId = 0x40;
		inline constexpr auto HoveredVehicleId = 0x50;
		inline constexpr auto HoveredHouseId = 0x60;
		inline constexpr auto HoveredPlayerId = 0x78;
		inline constexpr auto HumanInteractionAction = 0x1C4;
		inline constexpr auto VehicleInteractionAction = 0x1C8;
		inline constexpr auto PrimaryCommand = 0x1CC;
		inline constexpr auto SecondaryCommand = 0x1E0;
		inline constexpr auto AreaSelectionActiveFlag = 0x1F4;
		inline constexpr auto AreaStartX = 0x1F8;
		inline constexpr auto AreaStartY = 0x1FC;
		inline constexpr auto AreaEndX = 0x200;
		inline constexpr auto AreaEndY = 0x204;
		inline constexpr auto FrameMode = 0x208;
		inline constexpr auto LeftClickHandler = 0x20C;
		inline constexpr auto LeftClickHandlerUnsigned = 0x20Cu;
		inline constexpr auto LeftClickCommand = 0x210;
		inline constexpr auto DoubleClickHandler = 0x214;
		inline constexpr auto DoubleClickHandlerUnsigned = 0x214u;
		inline constexpr auto DoubleClickCommand = 0x218;
		inline constexpr auto LastDoubleClickEventTime = 0x21C;
		inline constexpr auto FollowEnabledFlag = 0x220;
		inline constexpr auto FollowHumanFlag = 0x221;
		inline constexpr auto FollowHumanId = 0x224;
		inline constexpr auto FollowHumanUniqueId = 0x228;
		inline constexpr auto FollowVehicleFlag = 0x22C;
		inline constexpr auto FollowVehicleId = 0x230;
		inline constexpr auto FollowVehicleUniqueId = 0x234;
		inline constexpr auto DarkenVariant = 0x238;
		inline constexpr auto DarkenPalette = 0x23C;
	}

	namespace WorldGuiStateOffset
	{
		inline constexpr auto HoveredMapPointX = 0x0C;
		inline constexpr auto HoveredMapPointY = 0x0E;
		inline constexpr auto PrimaryCommand = 0x19C;
		inline constexpr auto SecondaryCommand = 0x1B0;
	}

	namespace SingleVehicleWindowOffset
	{
		inline constexpr auto UncategorizedStockGoods = 620;
		inline constexpr auto TabEnabledFlags = 0x70;
		inline constexpr auto TraderTabEnabled = 0x71;
		inline constexpr auto HumanPassengersTabEnabled = 0x72;
		inline constexpr auto HumanAndVehiclePassengersTabEnabled = 0x73;
		inline constexpr auto StockTabEnabled = 0x74;
		inline constexpr auto TabCollapsedFlags = 0x75;
		inline constexpr auto TraderTabCollapsed = 0x76;
		inline constexpr auto HumanPassengersTabCollapsed = 0x77;
		inline constexpr auto HumanAndVehiclePassengersTabCollapsed = 0x78;
		inline constexpr auto StockTabCollapsed = 0x79;
		inline constexpr auto TabStateChecksum = 0x7C;
		inline constexpr auto TabRectangles = 0x80;
		inline constexpr auto ComputedWidth = 0xD0;
		inline constexpr auto ComputedHeight = 0xD4;
		inline constexpr auto TabHeightOverrides = 0xD8;
		inline constexpr auto LayoutInvalidFlag = 0xEC;
		inline constexpr auto PreviewElement = 0xF0;
		inline constexpr auto RouteSelectButtons = 0xF4;
		inline constexpr auto RouteSelectButtonsUnsigned = 0xF4u;
		inline constexpr auto RouteAssignButtons = 0xF8;
		inline constexpr auto RouteRemoveButtons = 0xFC;
		inline constexpr auto RouteRemoveButtonsUnsigned = 0xFCu;
		inline constexpr auto RouteLabels = 0x100;
		inline constexpr auto SecondRouteSelectButton = 0x104;
		inline constexpr auto SecondRouteAssignButton = 0x108;
		inline constexpr auto SecondRouteRemoveButton = 0x10C;
		inline constexpr auto SecondRouteLabel = 0x110;
		inline constexpr auto TraderConfigButton = 0x114;
		inline constexpr auto StockPage = 0x118;
		inline constexpr auto StockGoods = 0x11C;
		inline constexpr auto Tooltips = 0x2A4;
		inline constexpr auto DynamicButtons = 0x3188;
		inline constexpr auto TraderConfigWindow = 0x3C7C;
	}

	namespace SingleVehicleWindowVarsOffset
	{
		inline constexpr auto Tooltips = 0x234;
	}

	namespace GuiMessageManagerOffset
	{
		inline constexpr auto NextMessageId = 4;

		inline constexpr auto PresentationWindow = (sizeof(void*) == 8 ? 16 : 0xC);

		inline constexpr auto MinimumPriority = 8;

		inline constexpr auto DisplayedCount = 0;

		inline constexpr auto HistoryMessages = (sizeof(void*) == 8 ? 32 : 0x14);

		inline constexpr auto DisplayedMessages = (sizeof(void*) == 8 ? 24 : 0x10);
	}

	namespace GuiMessageEnvelopeOffset
	{
		inline constexpr auto FirstHouse = 0x208;

		inline constexpr auto FirstGood = 0x38;

		inline constexpr auto MessageUnsigned = 8u;

		inline constexpr auto Type = 8;

		inline constexpr auto PlayerId = 0x10;

		inline constexpr auto MapPoint = 0x30;

		inline constexpr auto HumanUniqueId = 0x1C;

		inline constexpr auto Message = 8;

		inline constexpr auto VehicleId = 0x28;

		inline constexpr auto HouseId = 0x20;

		inline constexpr auto HumanId = 0x18;
	}

	namespace GuiMessageHistoryOffset
	{
		inline constexpr auto CreatedTick = 4;

		inline constexpr auto AllocatedFlag = 0;
	}

	namespace GuiDisplayedMessageOffset
	{
		inline constexpr auto MiniButton = (sizeof(void*) == 8 ? 784 : 0x30C);

		inline constexpr auto UniqueId = 4;

		inline constexpr auto CreatedTick = 0;
	}

	namespace InputSelectedObjectOffset
	{
		inline constexpr auto UniqueId = 4;

		inline constexpr auto ObjectId = 0;
	}

	namespace SelectionHouseWindowOffset
	{
		inline constexpr auto StockHouseTabCollapsed = 127;

		inline constexpr auto ProductionTabEnabled = 0x75;

		inline constexpr auto BarrackTabEnabled = 0x76;

		inline constexpr auto TabRectangles = 0x8C;

		inline constexpr auto DynamicButtons = 0x31EC;

		inline constexpr auto Tooltips = 0x308;

		inline constexpr auto OpponentTradeTabEnabled = 0x7A;

		inline constexpr auto OpponentTabEnabled = 0x79;

		inline constexpr auto InhabitantsTabEnabled = 0x78;

		inline constexpr auto HomeTabEnabled = 0x73;

		inline constexpr auto DefenceTabEnabled = 0x71;

		inline constexpr auto ConstructionTabCollapsed = 0x7D;

		inline constexpr auto ConstructionTabEnabled = 0x72;

		inline constexpr auto SelectedStockGood = 0x178;

		inline constexpr auto StockPage = 0x17C;

		inline constexpr auto PreviewElement = 0x174;

		inline constexpr auto ComputedHeight = 0x140;

		inline constexpr auto ComputedWidth = 0x13C;

		inline constexpr auto TabStateChecksum = 0x88;

		inline constexpr auto UncategorizedStockGoods = 0x2D0;

		inline constexpr auto StockGoods = 0x180;

		inline constexpr auto StockHouseTabEnabled = 0x74;

		inline constexpr auto LayoutInvalidFlag = 0x170;

		inline constexpr auto TabHeightOverrides = 0x144;

		inline constexpr auto TabCollapsedFlags = 0x7B;

		inline constexpr auto TabEnabledFlags = 0x70;
	}

	namespace SelectionHouseWindowVarsOffset
	{
		inline constexpr auto Tooltips = 0x298;
	}

	namespace HelpWindowOffset
	{
		inline constexpr auto Goods = 0x3B4;

		inline constexpr auto HouseTypes = 0x88;

		inline constexpr auto DynamicButtons = 0x55C;

		inline constexpr auto HyperTextFrame = 0x74;

		inline constexpr auto HyperTextElement = 0x558;

		inline constexpr auto Page = 0x70;

		inline constexpr auto MiscCount = 0x54C;

		inline constexpr auto MiscHelpStrings = 0x550;

		inline constexpr auto SelectedMiscIndex = 0x554;

		inline constexpr auto GoodHelpStrings = 0x544;

		inline constexpr auto GoodsUnsigned = 0x3B4u;

		inline constexpr auto GoodCount = 0x3B0;

		inline constexpr auto SelectedGoodIndex = 0x548;

		inline constexpr auto HouseHelpStrings = 0x3A8;

		inline constexpr auto HouseDevelopmentStagesUnsigned = 0x8Cu;

		inline constexpr auto HouseTypesUnsigned = 0x88u;

		inline constexpr auto HouseCount = 0x84;

		inline constexpr auto SelectedHouseIndex = 0x3AC;
	}

	namespace HelpHouseEntryOffset
	{
		inline constexpr auto EngineType = 0;

		inline constexpr auto DevelopmentStage = 4;
	}

	namespace HelpGoodEntryOffset
	{
		inline constexpr auto GoodType = 0;
	}

	namespace EngineHouseTypeOffset
	{
		inline constexpr auto ArrowSlotsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlots);

		inline constexpr auto ConstructionGoodsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods);

		inline constexpr auto WorkAreas = offsetof(NC2E2::SEngineHouseTypeStorage, workAreas);

		inline constexpr auto WalkAreas = offsetof(NC2E2::SEngineHouseTypeStorage, walkAreas);

		inline constexpr auto BuildArea = offsetof(NC2E2::SEngineHouseTypeStorage, buildArea);

		inline constexpr auto ClosedDoorBobIdsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays) + offsetof(NC2E2::SHouseDoorOverlayStorage, closedBob);

		inline constexpr auto OpenDoorBobIdsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays) + offsetof(NC2E2::SHouseDoorOverlayStorage, openBob);

		inline constexpr auto DoorPoints = offsetof(NC2E2::SEngineHouseTypeStorage, doorPoints);

		inline constexpr auto LogicTypes = offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto DoorOverlays = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays);

		inline constexpr auto PaletteIds = offsetof(NC2E2::SEngineHouseTypeStorage, paletteIds);

		inline constexpr auto ArrowSlotCount = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlotCount);

		inline constexpr auto MaximumHitpoints = offsetof(NC2E2::SEngineHouseTypeStorage, maximumHitpoints);

		inline constexpr auto ClosedDoorBobIds = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays) + offsetof(NC2E2::SHouseDoorOverlayStorage, closedBob);

		inline constexpr auto OpenDoorBobIds = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays) + offsetof(NC2E2::SHouseDoorOverlayStorage, openBob);

		inline constexpr auto StagePoints = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints);

		inline constexpr auto ConstructionLayersUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, constructionLayers);

		inline constexpr auto LandscapeOverlaysUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, landscapeOverlays);

		inline constexpr auto StagePointsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints);

		inline constexpr auto MaximumHitpointsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, maximumHitpoints);

		inline constexpr auto DoorOverlaysUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, doorOverlays);

		inline constexpr auto ActionOverlays = offsetof(NC2E2::SEngineHouseTypeStorage, actionOverlays);

		inline constexpr auto ActionOverlayCount = offsetof(NC2E2::SEngineHouseTypeStorage, actionOverlayCount);

		inline constexpr auto ShadingFactor256 = offsetof(NC2E2::SEngineHouseTypeStorage, shadingFactor256);

		inline constexpr auto StageBobIdsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stageBobIds);

		inline constexpr auto ShadowBobManager = offsetof(NC2E2::SEngineHouseTypeStorage, shadowBobManager);

		inline constexpr auto BobManager = offsetof(NC2E2::SEngineHouseTypeStorage, bobManager);

		inline constexpr auto PaletteIdsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, paletteIds);

		inline constexpr auto PaletteCount = offsetof(NC2E2::SEngineHouseTypeStorage, paletteCount);

		inline constexpr auto LogicTypesUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto DevelopmentStageCount = offsetof(NC2E2::SEngineHouseTypeStorage, developmentStageCount);

		inline constexpr auto TribeType = offsetof(NC2E2::SEngineHouseTypeStorage, tribeType);
	}

	namespace ListBoxControllerOffset
	{
		inline constexpr auto VerticalEnabledFlag = 0x35;

		inline constexpr auto HorizontalEnabledFlag = 0x34;

		inline constexpr auto Inset = 0x38;

		inline constexpr auto List = 0x30;
	}

	namespace ListBoxOffset
	{
		inline constexpr auto InputChildren = 0x60;

		inline constexpr auto DrawChildren = 0x4C;

		inline constexpr auto ScrollY = 0x34;

		inline constexpr auto ChildBoundsY = 0x3C;

		inline constexpr auto ChildBoundsHeight = 0x44;

		inline constexpr auto ScrollX = 0x30;

		inline constexpr auto ChildBounds = 0x38;

		inline constexpr auto ChildBoundsWidth = 0x40;

		inline constexpr auto ActiveChild = 0x48;
	}

	namespace ListBoxObjectVarsOffset
	{
		inline constexpr auto InputChildren = 0x14;
	}

	namespace TechTreeWindowStateOffset
	{
		inline constexpr auto SelectedHouseTypeUnsigned = 4u;

		inline constexpr auto Page = 0;

		inline constexpr auto SelectedJobType = 12;

		inline constexpr auto SelectedGoodType = 8;

		inline constexpr auto SelectedHouseType = 4;
	}

	namespace TechTreeWindowOffset
	{
		inline constexpr auto DynamicButtons = 0x70;
	}

	namespace TechTreeTypeElementOffset
	{
		inline constexpr auto Window = 0x64;

		inline constexpr auto TypeId = 0x60;
	}

	namespace TechTreeLineElementOffset
	{
		inline constexpr auto SecondPointY = 0x38;

		inline constexpr auto SecondPointX = 0x34;

		inline constexpr auto FirstPointY = 0x30;

		inline constexpr auto FirstPointX = 0x2C;
	}

	namespace DiplomacyWindowOffset
	{
		inline constexpr auto LastButtonsUpdateTick = 0xDC;

		inline constexpr auto OverviewRectangle = 0xE4;

		inline constexpr auto VisiblePlayerCount = 0xD8;

		inline constexpr auto HiddenPlayerFlagsUnsigned = 0xC4u;

		inline constexpr auto PlayerIdsUnsigned = 0x74u;

		inline constexpr auto PlayerCount = 0x70;

		inline constexpr auto OverviewElement = 0xF4;

		inline constexpr auto MissionStrings = 0xE0;

		inline constexpr auto PlayerPageSlotsUnsigned = 0x70u;
	}

	namespace DiplomacyWindowStateOffset
	{
		inline constexpr auto Page = 0;
	}

	namespace DiplomacyCommandOffset
	{
		inline constexpr auto State = 12;

		inline constexpr auto Parameter = 12;

		inline constexpr auto SecondPlayer = 8;

		inline constexpr auto FirstPlayer = 4;

		inline constexpr auto Opcode = 0;

		inline constexpr auto PlayerId = 1;
	}

	namespace HumanListWindowStateOffset
	{
		inline constexpr auto PossibleJobFilter = 0x14;

		inline constexpr auto JobFilter = 0x10;
	}

	namespace HumanListJobSelectionWindowOffset
	{
		inline constexpr auto CurrentJobFlag = 0x74;

		inline constexpr auto OwnerWindow = 0x70;
	}

	namespace HumanListWindowOffset
	{
		inline constexpr auto FirstHumanUniqueIdUnsigned = 0x78u;

		inline constexpr auto HumanReferencesUnsigned = 0x74u;

		inline constexpr auto HumanCount = 0x70;

		inline constexpr auto JobSelectionWindow = 0x1014;
	}

	namespace HumanListButtonOffset
	{
		inline constexpr auto DebugFlag = 0x44;

		inline constexpr auto Handler = 0x40;

		inline constexpr auto DoubleClickCommand = 0x3C;

		inline constexpr auto PressedCommand = 0x38;

		inline constexpr auto HumanUniqueId = 0x34;

		inline constexpr auto HumanId = 0x30;
	}

	namespace StatisticsWindowStateOffset
	{
		inline constexpr auto TimeScaleHours = 8;

		inline constexpr auto GoodGroup = 4;

		inline constexpr auto Page = 0;
	}

	namespace StatisticsWindowOffset
	{
		inline constexpr auto DiagramColors = 0xAC;

		inline constexpr auto DiagramBitmap = 0x8C;

		inline constexpr auto DiagramState = 0x88;

		inline constexpr auto DiagramPlayerId = 0x98;

		inline constexpr auto DiagramWidth = 0x90;

		inline constexpr auto DiagramHeight = 0x94;

		inline constexpr auto DisplayMaximum = 0xA8;

		inline constexpr auto DisplayMinimum = 0xA4;

		inline constexpr auto ObservedMaximum = 0xA0;

		inline constexpr auto ObservedMinimum = 0x9C;

		inline constexpr auto GraveyardList = 0x78;

		inline constexpr auto HousesList = 0x74;

		inline constexpr auto JobsList = 0x70;

		inline constexpr auto GraveyardListUpdateTick = 0x84;

		inline constexpr auto HousesListUpdateTick = 0x80;

		inline constexpr auto JobsListUpdateTick = 0x7C;
	}

	namespace StatisticsDiagramStateOffset
	{
		inline constexpr auto Colors = 0x24;
	}

	namespace MissionManagerOffset
	{
		inline constexpr auto FirstMissionVars = 0x40;

		inline constexpr auto Missions = 0x1C;

		inline constexpr auto FailedFlag = 5;

		inline constexpr auto WonFlag = 4;

		inline constexpr auto CurrentBriefingId = 0;

		inline constexpr auto SubMapId = 0x10;

		inline constexpr auto SubMapCampaignId = 0x0C;

		inline constexpr auto ReturnToParentFlag = 0x14;

		inline constexpr auto MissionCount = 0x18;

		inline constexpr auto EvaluationInterruptedFlag = 6;

		inline constexpr auto StartSubMapFlag = 8;
	}

	namespace MissionDataOffset
	{
		inline constexpr auto DescriptionId = 0x30;

		inline constexpr auto InitiallyActiveFlag = 0x2C;

		inline constexpr auto VisibleFlag = 0x24;

		inline constexpr auto Results = 0x4F0;

		inline constexpr auto Goals = 0x3C;

		inline constexpr auto FirstGoalFulfilledFlagUnsigned = 0x60u;

		inline constexpr auto FulfilledFlag = 0x25;

		inline constexpr auto ResultCount = 0x4EC;

		inline constexpr auto GoalMatchMode = 0x34;

		inline constexpr auto ActivationTick = 0x28;

		inline constexpr auto GoalCount = 0x38;

		inline constexpr auto ActiveFlag = 0x26;
	}

	namespace MissionResultOffset
	{
		inline constexpr auto ParametersUnsigned = 4u;
	}

	namespace MissionGoalOffset
	{
		inline constexpr auto FulfilledFlag = 0x24;
	}

	namespace MissionDataVarsOffset
	{
		inline constexpr auto ResultsUnsigned = 0x4CCu;

		inline constexpr auto GoalsUnsigned = 0x18u;
	}

	namespace MapMacroPointOffset
	{
		inline constexpr auto PatternB = 1;

		inline constexpr auto PatternA = 0;

		inline constexpr auto Height = 2;
	}

	namespace MapContinentOffset
	{
		inline constexpr auto RetainedFlag = 12;

		inline constexpr auto PointCount = 8;

		inline constexpr auto SeedPoint = 4;

		inline constexpr auto Type = 0;
	}

	namespace MapMigPointOffset
	{
		inline constexpr auto MapYUnsigned = 2u;

		inline constexpr auto MapX = 0;

		inline constexpr auto MapY = 2;

		inline constexpr auto Coordinates = 0;
	}

	namespace EngineHouseStageViewOffset
	{
		inline constexpr auto SoldierFlagXUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, soldierFlagX);

		inline constexpr auto HolyFirePointCountUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, holyFire) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto BackSmokePointCountUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, backSmoke) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto ForwardSmokePointCountUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, forwardSmoke) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto FirePointCountUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, fire) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto FirePoints = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, fire) + offsetof(NC2E2::SHousePointArrayStorage, points);

		inline constexpr auto HolyFirePointCount = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, holyFire) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto HolyFirePoints = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, holyFire) + offsetof(NC2E2::SHousePointArrayStorage, points);

		inline constexpr auto BackSmokePointCount = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, backSmoke) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto BackSmokePoints = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, backSmoke) + offsetof(NC2E2::SHousePointArrayStorage, points);

		inline constexpr auto ForwardSmokePointCount = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, forwardSmoke) + offsetof(NC2E2::SHousePointArrayStorage, count);

		inline constexpr auto ForwardSmokePoints = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, forwardSmoke) + offsetof(NC2E2::SHousePointArrayStorage, points);

		inline constexpr auto SoldierFlagY = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, soldierFlagY);

		inline constexpr auto SoldierFlagX = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, soldierFlagX);

		inline constexpr auto SoldierFlagEnabled = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, soldierFlagEnabled);

		inline constexpr auto FlagY = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, flagY);

		inline constexpr auto FlagX = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, flagX);

		inline constexpr auto FlagYUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, flagY);

		inline constexpr auto FlagXUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, stagePoints) + offsetof(NC2E2::SHouseStagePointsStorage, flagX);
	}

	namespace EngineHouseActionOverlayOffset
	{
		inline constexpr auto BobIds = 28;

		inline constexpr auto OffsetY = 16;

		inline constexpr auto OffsetX = 12;

		inline constexpr auto BobIdsUnsigned = 0x1Cu;

		inline constexpr auto FrameTicks = 20;

		inline constexpr auto RequiredActiveFlag = 8;

		inline constexpr auto FrameCount = 24;

		inline constexpr auto DevelopmentStage = 0;

		inline constexpr auto Action = 4;
	}

	namespace EngineHouseLandscapeOverlayOffset
	{
		inline constexpr auto OffsetY = 8;

		inline constexpr auto OffsetX = 4;

		inline constexpr auto LandscapeId = 0xC;

		inline constexpr auto EnabledFlag = 0;
	}

	namespace EngineHouseConstructionLayerOffset
	{
		inline constexpr auto BobId = 4;

		inline constexpr auto ShadowBobId = 8;

		inline constexpr auto UpgradeFlag = 0;

		inline constexpr auto LastPercentage = 0x10;

		inline constexpr auto FirstPercentage = 0xC;
	}

	namespace EngineHouseConstructionLayersOffset
	{
		inline constexpr auto EntriesUnsigned = 4u;

		inline constexpr auto Count = 0;
	}

	namespace WorldDisplayOffset
	{
		inline constexpr auto DrawFlagsByte1 = EngineDisplayOffset::DrawFlagsByte1;
		inline constexpr auto ActiveDisplayFlag = EngineDisplayOffset::ActiveDisplayFlag;

		inline constexpr auto Font = EngineDisplayOffset::Font;

		inline constexpr auto DrawFlagsByte2 = EngineDisplayOffset::DrawFlags + 2;

		inline constexpr auto HighlightingEnabled = EngineDisplayOffset::HighlightingEnabled;
	}

	namespace EngineHouseLogicViewOffset
	{
		inline constexpr auto MaximumHitpoints = offsetof(NC2E2::SEngineHouseTypeStorage, maximumHitpoints) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);
		inline constexpr auto ConstructionGoodCounts = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);
		inline constexpr auto ArrowSlotYUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlots) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes) + 4;
		inline constexpr auto ArrowSlotXUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlots) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);
		inline constexpr auto ArrowSlotParameterUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlots) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes) + 8;
		inline constexpr auto ArrowSlotCount = offsetof(NC2E2::SEngineHouseTypeStorage, arrowSlotCount) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);


		inline constexpr auto FirstConstructionGood = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes) + 4;

		inline constexpr auto FirstDoorPointYUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, doorPoints) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes) + 2;

		inline constexpr auto DoorPointsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, doorPoints) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto BuildAreaUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, buildArea) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto BuildArea = offsetof(NC2E2::SEngineHouseTypeStorage, buildArea) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto WorkAreasUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, workAreas) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto FirstConstructionGoodUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes) + 4;

		inline constexpr auto ConstructionGoodsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto MaximumHitpointsUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, maximumHitpoints) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto WalkAreasUnsigned = offsetof(NC2E2::SEngineHouseTypeStorage, walkAreas) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto ConstructionGoods = offsetof(NC2E2::SEngineHouseTypeStorage, constructionGoods) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto DevelopmentStageCount = offsetof(NC2E2::SEngineHouseTypeStorage, developmentStageCount) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto TribeType = offsetof(NC2E2::SEngineHouseTypeStorage, tribeType) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);

		inline constexpr auto DoorPoints = offsetof(NC2E2::SEngineHouseTypeStorage, doorPoints) - offsetof(NC2E2::SEngineHouseTypeStorage, logicTypes);
	}

	namespace EnginePixelPointOffset
	{
		inline constexpr auto YUnsigned = 4u;
	}

	namespace AIConditionOffset
	{
		inline constexpr auto PayloadWord2 = 32;

		inline constexpr auto PayloadWord1 = 28;

		inline constexpr auto PayloadWord0 = 24;

		inline constexpr auto Payload = 24;

		inline constexpr auto DeactivatedTickUnsigned = 8u;

		inline constexpr auto ActivatedTickUnsigned = 4u;

		inline constexpr auto Active = 0;

		inline constexpr auto CreatedTick = 0x14;

		inline constexpr auto DeactivatedTick = 8;

		inline constexpr auto ActivatedTick = 4;

		inline constexpr auto KeepActive = 0x10;

		inline constexpr auto Type = 0x0C;
	}

	namespace AIConditionCheckOffset
	{
		inline constexpr auto OperandIds = 8;

		inline constexpr auto OperandCount = 4;

		inline constexpr auto Operation = 0;
	}

	namespace AIConditionGameTimeOffset
	{
		inline constexpr auto RequiredTick = 24;
	}

	namespace AIConditionLogicalCombinationOffset
	{
		inline constexpr auto OperandIdsUnsigned = 0x20u;

		inline constexpr auto OperandCount = 28;

		inline constexpr auto SecondOperandId = 0x24;

		inline constexpr auto OperandIds = 32;

		inline constexpr auto Operation = 0x18;
	}

	namespace AIConditionTimeSinceConditionChangeOffset
	{
		inline constexpr auto DelayTicks = 32;

		inline constexpr auto WatchActive = 28;

		inline constexpr auto ConditionId = 24;
	}

	namespace AIConditionDiplomacyOffset
	{
		inline constexpr auto State = 0x20;

		inline constexpr auto SecondPlayerId = 0x1C;

		inline constexpr auto FirstPlayerId = 0x18;
	}

	namespace AIConditionMapMoveableNearPositionOffset
	{
		inline constexpr auto Radius = 0x1C;

		inline constexpr auto MilitaryOnly = 37;

		inline constexpr auto OpponentsOnly = 0x24;

		inline constexpr auto PlayerId = 0x20;

		inline constexpr auto MapY = 0x1A;

		inline constexpr auto MapX = 0x18;
	}

	namespace AIConditionHouseNearPositionOffset
	{
		inline constexpr auto Radius = 0x1C;

		inline constexpr auto OpponentsOnly = 0x24;

		inline constexpr auto HouseType = 40;

		inline constexpr auto CompletedOnly = 44;

		inline constexpr auto PlayerId = 0x20;

		inline constexpr auto MapY = 0x1A;

		inline constexpr auto MapX = 0x18;
	}

	namespace AIConditionExternalFlagOffset
	{
		inline constexpr auto SeenPlayerId = 28;

		inline constexpr auto PlayerId = 24;
	}

	namespace AIConditionPlayerHasNoHumansOffset
	{
		inline constexpr auto PlayerId = 24;
	}

	namespace AIConditionSoldierCountOffset
	{
		inline constexpr auto MinimumCount = 24;

		inline constexpr auto PlayerId = 28;
	}

	namespace AIConditionActivatedFlagOffset
	{
		inline constexpr auto Flag = 24;
	}

	namespace AIConditionPeriodicOffset
	{
		inline constexpr auto InitialDelayTicks = 0x18;

		inline constexpr auto InactiveTicks = 0x20;

		inline constexpr auto ActiveTicks = 0x1C;
	}

	namespace AIConditionNearPositionOffset
	{
		inline constexpr auto OpponentsOnly = 36;

		inline constexpr auto Radius = 28;

		inline constexpr auto MapY = 26;

		inline constexpr auto MapX = 24;

		inline constexpr auto PlayerId = 32;
	}

	namespace PlayerStatisticSampleOffset
	{
		inline constexpr auto Houses = 20;
		inline constexpr auto Goods = 144;
		inline constexpr auto AdultFemaleCount = 8;

		inline constexpr auto AdultMaleCount = 2;

		inline constexpr auto SoldierCount = 0x12;

		inline constexpr auto HumanCount = 0;
	}

	namespace MerchantOffset
	{
		inline constexpr auto GoodsReceived = 16;

		inline constexpr auto GoodsDelivered = 12;

		inline constexpr auto SecondHouseId = 0x70;

		inline constexpr auto Houses = 0x1C;

		inline constexpr auto Allocated = 0;

		inline constexpr auto HousesUnsigned = 0x1Cu;

		inline constexpr auto CurrentHouseIndex = 0x18;

		inline constexpr auto TradeAgreementId = 4;

		inline constexpr auto ForeignHouseCount = 8;

		inline constexpr auto HouseCount = 0x14;
	}

	namespace MerchantHouseOffset
	{
		inline constexpr auto ImportFlags = 0x10;

		inline constexpr auto ImportFlagsUnsigned = 0x10u;

		inline constexpr auto ImportCount = 12;

		inline constexpr auto Foreign = 8;

		inline constexpr auto HouseRevision = 4;

		inline constexpr auto HouseId = 0;
	}

	namespace TradeAgreementOffset
	{
		inline constexpr auto IncomingAmount = 20;

		inline constexpr auto IncomingGood = 16;

		inline constexpr auto OutgoingAmount = 12;

		inline constexpr auto OutgoingGood = 8;

		inline constexpr auto HouseRevision = 4;

		inline constexpr auto HouseId = 0;
	}

	namespace MerchantManagerOffset
	{
		inline constexpr auto Entries = sizeof(void*) == 8 ? 72 : 48;

		inline constexpr auto TradeAgreementsUnsigned = sizeof(void*) == 8 ? 84u : 56u;

		inline constexpr auto TradeAgreementCount = sizeof(void*) == 8 ? 80 : 52;
	}

	namespace ArrayHandlerOffset
	{
		inline constexpr auto Initialized = 0;

		inline constexpr auto Capacity = 4;
	}

	namespace MarkerOffset
	{
		inline constexpr auto MapMoveableId = 0x0C;

		inline constexpr auto Type = 0x10;

		inline constexpr auto UniqueId = 8;

		inline constexpr auto MarkerId = 4;

		inline constexpr auto Allocated = 0;

		inline constexpr auto HumanId = 0x14;
	}

	namespace GameStaticVarsOffset
	{
		inline constexpr auto SuppressDestructionLandscapeFlag = sizeof(void*) == 8 ? 445 : 433;

		inline constexpr auto SuppressDeathStatisticsFlag = sizeof(void*) == 8 ? 444 : 432;
	}

	namespace StaticArmorTypeOffset
	{
		inline constexpr auto GoodType = 36;

		inline constexpr auto MaterialType = 40;

		inline constexpr auto GoodTypeUnsigned = 0x24u;
	}

	namespace ByteEnableSelectionOffset
	{
		inline constexpr auto Entries = 1;

		inline constexpr auto Mode = 0;

		inline constexpr auto FirstEnabled = 1;
	}

	namespace EngineMapMicroPointOffset
	{
		inline constexpr auto Flags = 2;

		inline constexpr auto LandscapeId = 0;

		inline constexpr auto HighlightFlags = 2;
	}

	namespace InGameGuiManagerOffset
	{
		inline constexpr auto MissionOnscreenInfoWindow = 8;
		inline constexpr auto NetworkInfoButton = 56;
		inline constexpr auto MeasuredFrameRate = 0x190;
		inline constexpr auto ActionButtonsOriginY = 0xD4;
		inline constexpr auto ActionButtonsOriginX = 0xD0;
		inline constexpr auto GuiMarkers = 0xE8;

		inline constexpr auto ActionButtonsChecksum = 0xCC;

		inline constexpr auto WallBuildWindow = 0xB4;

		inline constexpr auto RoadBuildWindow = 0xB0;

		inline constexpr auto GuideWindow = 0xA8;

		inline constexpr auto HouseTypeWindow = 0xA4;

		inline constexpr auto SingleAnimalWindow = 0x94;

		inline constexpr auto MultipleVehiclesWindow = 0x9C;

		inline constexpr auto MultipleHumansWindow = 0x90;

		inline constexpr auto StatisticsWindow = 0x48;

		inline constexpr auto SingleHumanWindow = 0x8C;

		inline constexpr auto LandscapeWindow = 0xAC;

		inline constexpr auto SingleVehicleWindow = 0x98;

		inline constexpr auto HouseWindow = 0xA0;

		inline constexpr auto SelectionChangeCount = 0x88;

		inline constexpr auto MessagePriorityButton = 0x40;

		inline constexpr auto SpeedButton = 0x34;

		inline constexpr auto OverviewWindow = 0x44;

		inline constexpr auto ExpertMode = 0xE0;

		inline constexpr auto MainMode = 0xDC;

		inline constexpr auto ScrollSpeed = 0xD8;

		inline constexpr auto SavedGameSpeed = 0x1A0;

		inline constexpr auto PausedByLargeWindow = 0x198;

		inline constexpr auto SoftwareMouse = 0xE4;

		inline constexpr auto TooltipsDisabled = 0xE1;

		inline constexpr auto BorderScroll = 0xE3;

		inline constexpr auto ThirdButtonScroll = 0xE2;

		inline constexpr auto LocalPlayerId = 0;

		inline constexpr auto WorldDisplayElement = 4;
	}

	namespace ActorIdentityOffset
	{
		inline constexpr auto Id = 4;

		inline constexpr auto UniqueId = 8;

		inline constexpr auto Allocated = 0;
	}

	namespace HouseBuildCommandOffset
	{
		inline constexpr auto SpecialItem = 16;

		inline constexpr auto Position = 12;

		inline constexpr auto DevelopmentStage = 8;

		inline constexpr auto HouseType = 4;
	}

	namespace ConstructionLineCommandOffset
	{
		inline constexpr auto SecondPoint = 8;

		inline constexpr auto FirstPoint = 4;
	}

	namespace AIEquipmentGoodViewOffset
	{
		inline constexpr auto PreferredType = 4;
	}

	namespace MapTargetDataOffset
	{
		inline constexpr auto MapY = 6;

		inline constexpr auto MapX = 4;
	}

	namespace AIVillagePhysicalSlotOffset
	{
		inline constexpr auto Radius = 20;

		inline constexpr auto Center = 16;

		inline constexpr auto Active = 0;
	}

	namespace AIHighLevelVillageViewOffset
	{
		inline constexpr auto DefendPoints = 0x108;

		inline constexpr auto DefendPointCount = 0x104;

		inline constexpr auto Active = 0x24;
	}

	namespace AttachedVehicleSelectionOffset
	{
		inline constexpr auto Tooltip = 0x220;

		inline constexpr auto Slots = 0x38;

		inline constexpr auto Count = 0x34;

		inline constexpr auto BaselineY = 0x218;

		inline constexpr auto Receiver = 0x2F0;

		inline constexpr auto ActionCommand = 0x2EC;

		inline constexpr auto SelectCommand = 0x2E8;

		inline constexpr auto HitIndex = 0x21C;

		inline constexpr auto VehicleRevision = 0x30;

		inline constexpr auto VehicleId = 0x2C;
	}

	namespace AttachedVehicleSelectionSlotOffset
	{
		inline constexpr auto X = 8;

		inline constexpr auto SortPriority = 0xC;

		inline constexpr auto Commander = 4;

		inline constexpr auto ObjectId = 0;
	}

	namespace MapAreaCommandOffset
	{
		inline constexpr auto Radius = 8;

		inline constexpr auto CenterPoint = 4;
	}

	namespace MapPointCommandOffset
	{
		inline constexpr auto Position = 4;
	}

	namespace WallGateStateCommandOffset
	{
		inline constexpr auto Open = 8;

		inline constexpr auto Position = 4;
	}

	namespace TributeCommandOffset
	{
		inline constexpr auto TributeId = 12;
	}

	namespace GameAssistantCommandOffset
	{
		inline constexpr auto Parameter = 8;

		inline constexpr auto Action = 4;
	}

	namespace GlobalDefendCommandOffset
	{
		inline constexpr auto Enabled = 4;
	}

	namespace GameSpeedCommandOffset
	{
		inline constexpr auto Speed = 4;
	}

	namespace PlayerDepartureCommandOffset
	{
		inline constexpr auto RemoveObjects = 12;

		inline constexpr auto OriginalFlag = 8;

		inline constexpr auto PlayerId = 4;
	}

	namespace GameChecksumCommandOffset
	{
		inline constexpr auto Checksum = 4;

		inline constexpr auto Tick = 8;

		inline constexpr auto NetworkPlayerId = 12;
	}

	namespace NetworkFrequencyCommandOffset
	{
		inline constexpr auto Frequency = 4;
	}

	namespace UserNetworkCommandManagerOffset
	{
		inline constexpr auto LastVerifiedTick = 0x960;

		inline constexpr auto OrdinaryQueueUnsigned = 0x98Cu;

		inline constexpr auto OrdinaryQueue = 0x98C;

		inline constexpr auto DeferredQueueUnsigned = 0x978u;

		inline constexpr auto DeferredQueue = 0x978;

		inline constexpr auto SystemQueueUnsigned = 0x964u;

		inline constexpr auto SystemQueue = 0x964;
	}

	namespace NetworkChecksumSampleOffset
	{
		inline constexpr auto NetworkPlayerId = 8;

		inline constexpr auto Tick = 4;
	}

	namespace NetworkCommandPacketOffset
	{
		inline constexpr auto Commands = 4;

		inline constexpr auto Count = 0;
	}

	namespace UserCommandQueuesOffset
	{
		inline constexpr auto Deferred = 0x14;

		inline constexpr auto Ordinary = 0x28;
	}

	namespace LogicActorHeaderOffset
	{
		inline constexpr auto UniqueId = 8;

		inline constexpr auto AllocatedFlag = 0;
	}

	namespace ObjectGroupOffset
	{
		inline constexpr auto SecondObjectTypeUnsigned = 16u;

		inline constexpr auto FirstObjectUniqueId = 12;

		inline constexpr auto FirstObjectId = 8;

		inline constexpr auto FirstObjectUniqueIdUnsigned = 12u;

		inline constexpr auto FirstObjectType = 4;

		inline constexpr auto FirstObjectTypeUnsigned = 4u;

		inline constexpr auto FirstObjectIdUnsigned = 8u;

		inline constexpr auto Count = 0;
	}

	namespace ObjectGroupObjectOffset
	{
		inline constexpr auto UniqueId = 8;

		inline constexpr auto Type = 0;

		inline constexpr auto ObjectId = 4;
	}

	namespace ObjectGroupManagerOffset
	{
		inline constexpr auto Groups = 4;

		inline constexpr auto GroupsUnsigned = 4u;
	}

	namespace DeadHumanManagerOffset
	{
		inline constexpr auto NextRecordIndex = 4;

		inline constexpr auto Records = 8;

		inline constexpr auto Count = 0;
	}

	namespace DeadHumanRecordOffset
	{
		inline constexpr auto Name = 4;

		inline constexpr auto MapX = 0x38;

		inline constexpr auto MapY = 0x3A;

		inline constexpr auto MapPosition = 0x38;

		inline constexpr auto JobType = 0x34;

		inline constexpr auto DeathTick = 0x30;

		inline constexpr auto HumanUniqueId = 0;
	}

	namespace MainMenuManagerOffset
	{
		inline constexpr auto SuppressPreviewDarkening = 0x5C;
		inline constexpr auto PlayerButtons = 48;
		inline constexpr auto WorldDisplayElement = 4;
		inline constexpr auto LobbyMode = 0xF10;

		inline constexpr auto PlayerChangeCount = 0xEE0;

		inline constexpr auto SessionName = 0xA3C;

		inline constexpr auto FirstSessionGuid = 0x5F4;

		inline constexpr auto SessionRows = 0x58C;

		inline constexpr auto SessionCount = 0x588;

		inline constexpr auto FirstProviderGuidUnsigned = 0xD4u;

		inline constexpr auto ProviderRows = 0x6C;

		inline constexpr auto LobbyDifficulty = 0x106C;

		inline constexpr auto FixedPlayerColors = 0x1068;

		inline constexpr auto FirstLobbyPlayerColor = 0xF34;

		inline constexpr auto FirstLobbyPlayerOptions = 0xF30;

		inline constexpr auto FirstLobbyPlayerType = 0xF2C;

		inline constexpr auto LobbyPlayerRows = 0xF28;

		inline constexpr auto LobbySavegameId = 0xF24;

		inline constexpr auto SelectedPlayerId = 0xEE4;

		inline constexpr auto FirstPlayerId = 0xB10;

		inline constexpr auto FirstPlayerHostFlag = 0xB0E;

		inline constexpr auto PlayerRows = 0xAA8;

		inline constexpr auto PlayerCount = 0xAA4;

		inline constexpr auto SelectedProviderIndex = 0x51C;

		inline constexpr auto FirstProviderGuid = 0xD4;

		inline constexpr auto ProviderRowsUnsigned = 0x6Cu;

		inline constexpr auto ProviderCount = 0x68;
	}

	namespace MenuLobbyPlayerTypeViewOffset
	{
		inline constexpr auto Color = 8;

		inline constexpr auto OptionsHighByte = 7;
	}

	namespace MenuLobbyPlayerOffset
	{
		inline constexpr auto NetworkPlayerId = 0;

		inline constexpr auto Color = 0x0C;

		inline constexpr auto Options = 8;

		inline constexpr auto Type = 4;
	}

	namespace MenuLobbyOffset
	{
		inline constexpr auto Difficulty = 0x15C;

		inline constexpr auto FixedPlayerColors = 0x158;

		inline constexpr auto Players = 0x18;

		inline constexpr auto SavegameId = 0x14;

		inline constexpr auto MapGuid = 4;

		inline constexpr auto Mode = 0;
	}

	namespace MenuLobbyScaledOffset
	{
		inline constexpr auto OptionsIndexBiasUnsigned = 0xF3u;
	}

	namespace MenuLobbyOptionsViewOffset
	{
		inline constexpr auto HighByteUnsigned = 3u;
	}

	namespace GameControlStartupOffset
	{
		inline constexpr auto MultiplayerMapGuidUnsigned = 0x18u;

		inline constexpr auto SingleMapGuidUnsigned = 4u;

		inline constexpr auto LocalNetworkPlayerId = 0x28;

		inline constexpr auto FirstPlayerNetworkId = 0x34;

		inline constexpr auto FirstPlayerColor = 0x30;

		inline constexpr auto FirstPlayerType = 0x2C;

		inline constexpr auto DifficultyLevel = 0x11C;

		inline constexpr auto FirstPlayerNetworkIdUnsigned = 0x34u;

		inline constexpr auto FirstPlayerColorUnsigned = 0x30u;

		inline constexpr auto FirstPlayerTypeUnsigned = 0x2Cu;

		inline constexpr auto SingleMapGuid = 4;

		inline constexpr auto SavegameId = 0x14;

		inline constexpr auto Mode = 0;
	}

	namespace MasterControlProgramOffset
	{
		inline constexpr auto StartupDifficulty = 0x148;

		inline constexpr auto StartupData = 0x2C;
	}

	namespace MenuNetworkMessageOffset
	{
		inline constexpr auto Payload = 8;

		inline constexpr auto Type = 0;
	}

	namespace KeyManagerOffset
	{
		inline constexpr auto QueuedMessageCount = 12;
		inline constexpr auto ModifierFlags = 0;
	}

	namespace SessionSummaryOffset
	{
		inline constexpr auto InstanceGuid = 4;

		inline constexpr auto Name = 0;
	}

	namespace MenuNetworkListOffset
	{
		inline constexpr auto Rows = 4;
	}

	namespace MenuNamedGuidRowOffset
	{
		inline constexpr auto Guid = 0x68;
	}

	namespace MenuNetworkPlayerOffset
	{
		inline constexpr auto HostFlag = 0x66;
	}

	namespace HouseInhabitantsSelectionOffset
	{
		inline constexpr auto Tooltip = 0x810;

		inline constexpr auto Rows = 0x38;

		inline constexpr auto Count = 0x34;

		inline constexpr auto BaselineY = 0x808;

		inline constexpr auto Receiver = 0x8E0;

		inline constexpr auto ActionCommand = 0x8DC;

		inline constexpr auto SelectCommand = 0x8D8;

		inline constexpr auto HitIndex = 0x80C;

		inline constexpr auto HouseRevision = 0x30;

		inline constexpr auto HouseId = 0x2C;
	}

	namespace HouseInhabitantsSelectionSlotOffset
	{
		inline constexpr auto X = 8;

		inline constexpr auto TraderFlag = 16;

		inline constexpr auto SortPriority = 12;

		inline constexpr auto RoleMask = 4;

		inline constexpr auto HumanId = 0;
	}

	namespace GuiTextElementOffset
	{
		inline constexpr auto Caption = 0x30;

		inline constexpr auto DarkFontFlag = 0xFD;

		inline constexpr auto SmallFontFlag = 0xFB;

		inline constexpr auto DrawFrameFlag = 0xFA;

		inline constexpr auto CenteredFlag = 0xF8;

		inline constexpr auto CaptionTerminator = 0xF7;

		inline constexpr auto WrapTextFlag = 0xFC;

		inline constexpr auto FillBackgroundFlag = 0xF9;
	}

	namespace GuiTextButtonOffset
	{
		inline constexpr auto ImmediateTooltipFlag = 0x48;

		inline constexpr auto MessageParameter2 = 0x50;

		inline constexpr auto MessageParameter1 = 0x4C;

		inline constexpr auto LineCount = 0x38;

		inline constexpr auto SkipOverwideCaptionFlag = 0x58;

		inline constexpr auto MultilineFlag = 0x34;

		inline constexpr auto Caption = 0x30;

		inline constexpr auto SmallFontFlag = 0x57;

		inline constexpr auto SolidColor = 0x5C;

		inline constexpr auto FillSolidColorFlag = 0x59;

		inline constexpr auto DisabledFlag = 0x56;

		inline constexpr auto Tooltip = 0x44;

		inline constexpr auto CenteredFlag = 0x54;

		inline constexpr auto SelectedFlag = 0x55;

		inline constexpr auto Receiver = 0x40;

		inline constexpr auto Command = 0x3C;
	}

	namespace AIGuideManagerOffset
	{
		inline constexpr auto DestinationY = 0x12;

		inline constexpr auto GuidePairSearchExhaustedFlag = 0x14;

		inline constexpr auto ScoutHumanRevision = 4;

		inline constexpr auto ScoutHumanId = 0;

		inline constexpr auto ScoutRequestedFlag = 8;

		inline constexpr auto MilitaryPointIndex = 0x0C;

		inline constexpr auto GridHeight = 28;

		inline constexpr auto CalculationGrid = 24;

		inline constexpr auto Destination = 0x10;

		inline constexpr auto GridCells = 0x20;
	}

	namespace AIGuideGridCellOffset
	{
		inline constexpr auto HouseId = 8;

		inline constexpr auto HasHouseFlag = 0;

		inline constexpr auto GuideId = 4;

		inline constexpr auto HasGuideFlag = 1;
	}

	namespace AIGuideGridOffset
	{
		inline constexpr auto Cells = 8;

		inline constexpr auto Height = 4;

		inline constexpr auto Width = 0;
	}

	namespace AIScoutOffset
	{
		inline constexpr auto HumanRevision = 4;

		inline constexpr auto HumanRequestedFlag = 8;

		inline constexpr auto HumanId = 0;
	}

	// Shared two-WORD map-coordinate storage; grid units belong to the owning API.
	namespace MapCoordinatePairOffset
	{
		inline constexpr auto YUnsigned = 2u;
		inline constexpr auto Coordinates = 0;

		inline constexpr auto X = 0;

		inline constexpr auto Y = 2;
	}

	namespace FishSwarmOffset
	{
		inline constexpr auto MapPositionY = 2;
		inline constexpr auto MapPosition = 0;
		inline constexpr auto FishCount = 4;

		inline constexpr auto MapY = 2;

		inline constexpr auto MapX = 0;
	}

	namespace MacroMapSectionOffset
	{
		inline constexpr auto Entries = 4;

		inline constexpr auto EntriesUnsigned = 4u;
	}

	namespace MacroMapCaptureUsageOffset
	{
		inline constexpr auto NameIndexUnsigned = 4u;
	}

	namespace EngineMapMacroPointOffset
	{
		inline constexpr auto PatternBUnsigned = 2u;
		inline constexpr auto PatternAUnsigned = 0u;
		inline constexpr auto PatternB = 2;

		inline constexpr auto PatternA = 0;

		inline constexpr auto Flags = 6;

		inline constexpr auto Brightness = 4;

		inline constexpr auto TransitionB = 8;

		inline constexpr auto Transitions = 7;

		inline constexpr auto VertexColorIndex = 5;

		inline constexpr auto TransitionsUnsigned = 7u;
	}

	namespace EngineMapStaticOffset
	{
		inline constexpr auto TransitionDirtyBounds = 16;

		inline constexpr auto HeightDirtyBounds = 8;

		inline constexpr auto MissingResourceFlag = 0x18;

		inline constexpr auto MacroPoints = 0;

		inline constexpr auto MicroPoints = 4;
	}

	namespace EngineDirtyBoundsOffset
	{
		inline constexpr auto FirstY = 2;

		inline constexpr auto LastPoint = 4;

		inline constexpr auto LastY = 6;
	}

	namespace AIGlobalGoodOffset
	{
		inline constexpr auto ImportVillageIds = 8;

		inline constexpr auto CandidateVillageIds = 0x30;

		inline constexpr auto CandidateVillageCount = 0x2C;

		inline constexpr auto HasProducer = 0x28;

		inline constexpr auto ImportVillageCount = 4;

		inline constexpr auto SelectedVillageId = 0;
	}

	namespace AIGoodOffset
	{
		inline constexpr auto ProductionConfirmationCount = 4;

		inline constexpr auto ForceProducingFlag = 8;

		inline constexpr auto TraderBringFlag = 10;

		inline constexpr auto ProducingFlag = 0;

		inline constexpr auto ProducerCount = 16;

		inline constexpr auto ProducerHouseId = 28;

		inline constexpr auto Producer = 20;

		inline constexpr auto NeedCount = 12;

		inline constexpr auto ImportingFlag = 9;
	}

	namespace AIVillagesGoodsViewOffset
	{
		inline constexpr auto FirstForceProducingFlag = 0x288;
	}

	namespace AIGoodProducerOffset
	{
		inline constexpr auto HumanRevision = 4;

		inline constexpr auto HouseRevision = 12;

		inline constexpr auto HouseId = 8;

		inline constexpr auto HumanId = 0;
	}

	namespace AIAnimalCountsOffset
	{
		inline constexpr auto FirstRequestedFlag = 12;

		inline constexpr auto FirstTribeType = 8;

		inline constexpr auto FirstQuantity = 4;

		inline constexpr auto Count = 0;
	}

	namespace AIAnimalCatcherOffset
	{
		inline constexpr auto TypeCount = 12;

		inline constexpr auto Counts = 12;

		inline constexpr auto HumanRevision = 4;

		inline constexpr auto HumanId = 0;

		inline constexpr auto HumanRequestedFlag = 8;

		inline constexpr auto Target = 40;

		inline constexpr auto MilitaryPointIndex = 0x34;
	}

	namespace AIDiplomacyOffset
	{
		inline constexpr auto IntrusionCounts = 4;

		inline constexpr auto PlayerCursor = 0;
	}

	namespace HumanSoundManagerOffset
	{
		inline constexpr auto LastGenericSoundTick = 0;
	}

	namespace HumanSoundManagerViewOffset
	{
		inline constexpr auto FirstResponsesUnsigned = 0x10u;

		inline constexpr auto FirstGenericSoundIdUnsigned = 12u;

		inline constexpr auto FirstSeenCountUnsigned = 4u;

		inline constexpr auto FirstScreamSoundIdUnsigned = 8u;
	}

	namespace HumanSoundScreamViewOffset
	{
		inline constexpr auto ResponseSoundIdsUnsigned = 0x0Cu;

		inline constexpr auto ResponseCountUnsigned = 8u;
	}

	namespace StaticSoundFxOffset
	{
		inline constexpr auto FirstWaveVolume = 48;
		inline constexpr auto Waves = 44;
		inline constexpr auto FirstWaveVolumeUnsigned = 0x30u;

		inline constexpr auto WavesUnsigned = 0x2Cu;

		inline constexpr auto WaveCount = 0x28;
	}

	namespace HumanSoundResponseGroupOffset
	{
		inline constexpr auto SoundIdsUnsigned = 4u;
	}

	namespace StaticBobCacheEntryOffset
	{
		inline constexpr auto BobManager = 0x50;
	}

	namespace StaticBobCacheManagerOffset
	{
		inline constexpr auto GuiBubblesBobManager = sizeof(void*) == 8 ? 48 : 24;

		inline constexpr auto FishBobManager = sizeof(void*) == 8 ? 40 : 20;
	}

	namespace AmbientSoundManagerOffset
	{
		inline constexpr auto ActiveLoopCount = (sizeof(void*) == 8 ? 16 : 12);

		inline constexpr auto ActiveLoops = (sizeof(void*) == 8 ? 16 : 12);

		inline constexpr auto LastLandscapeSoundTick = (sizeof(void*) == 8 ? 100 : 96);

		inline constexpr auto SectorHeight = 4;

		inline constexpr auto SectorWidth = 0;

		inline constexpr auto Sectors = 8;
	}

	namespace AmbientSectorOffset
	{
		inline constexpr auto FirstLandscapeVolumeUnsigned = 0x24u;

		inline constexpr auto FirstPatternVolumeUnsigned = 8u;

		inline constexpr auto LandscapeEntriesUnsigned = 0x20u;

		inline constexpr auto PatternEntriesUnsigned = 4u;

		inline constexpr auto LandscapeCount = 0x1C;

		inline constexpr auto PatternCount = 0;
	}

	namespace TerrainAmbientGroupsOffset
	{
		inline constexpr auto IdsUnsigned = 0x2Cu;

		inline constexpr auto Count = 0x28;
	}

	namespace AmbientLoopEntryOffset
	{
		inline constexpr auto VolumeUnsigned = 4u;
	}

	namespace StaticAmbientSoundOffset
	{
		inline constexpr auto Sounds = 0x30;

		inline constexpr auto SoundsUnsigned = 0x30u;

		inline constexpr auto FirstSelectionWeightUnsigned = 0xB0u;

		inline constexpr auto SoundCount = 0x2C;

		inline constexpr auto TotalSelectionWeight = 0x28;

		inline constexpr auto FirstWaveId = 0xA8;
	}

	namespace StaticAmbientWaveOffset
	{
		inline constexpr auto SelectionWeight = 0x80;

		inline constexpr auto WaveId = 0x78;

		inline constexpr auto Volume = 0x7C;

		inline constexpr auto ChanceMultiplier = 0x84;
	}

	namespace EngineMicroPointOffset
	{
		inline constexpr auto LandscapeId = 0;
	}

	namespace AmbientChangeBoundsOffset
	{
		inline constexpr auto FirstX = 0;

		inline constexpr auto FirstY = 2;

		inline constexpr auto LastX = 4;

		inline constexpr auto LastY = 6;
	}

	namespace DirectSoundManagerOffset
	{
		inline constexpr auto MainVolume = (sizeof(void*) == 8 ? 28 : 16);

		inline constexpr auto Quality = (sizeof(void*) == 8 ? 24 : 12);

		inline constexpr auto Initialized = (sizeof(void*) == 8 ? 8 : 4);
	}

	namespace MissionInfoWindowOffset
	{
		inline constexpr auto BriefingFilename = 0x288;

		inline constexpr auto BriefingDirectory = 0x184;

		inline constexpr auto MapRoot = 0x75;

		inline constexpr auto CallbackArgument2 = 0x398;

		inline constexpr auto HasCenteredMainDisplay = 0x179;

		inline constexpr auto CallbackArgument1 = 0x394;

		inline constexpr auto CallbackCommand = 0x390;

		inline constexpr auto HistoryFrame = 0x3A4;

		inline constexpr auto CallbackCalled = 0x38C;

		inline constexpr auto HypertextElement = 0x3B4;

		inline constexpr auto HasBriefingFile = 0x17C;

		inline constexpr auto HasMapRoot = 0x74;

		inline constexpr auto GoalStrings = 0x3A0;

		inline constexpr auto OwnsGoalStrings = 0x39C;

		inline constexpr auto BriefingId = 0x180;

		inline constexpr auto Page = 0x70;
	}

	namespace BaseToolWindowOffset
	{
		inline constexpr auto BackgroundStyle = 0x6C;
	}

	namespace HypertextGuiElementOffset
	{
		inline constexpr auto LastJumpSucceeded = 0x39;

		inline constexpr auto AutoScrollStartTime = 0x40;

		inline constexpr auto AutoScrollDelay = 0x44;

		inline constexpr auto AutoScrollSpeed = 0x3C;

		inline constexpr auto Resources = 0x2C;

		inline constexpr auto Entities = 0x30;

		inline constexpr auto WheelScrollingEnabled = 0x38;

		inline constexpr auto FillBackground = 0x3A;

		inline constexpr auto Display = 0x34;
	}

	namespace BitmapViewOffset
	{
		inline constexpr auto Rectangle = 8;
	}

	namespace AIManagerOffset
	{
		inline constexpr auto HighLevelHandlers = offsetof(NC2Logic::SAIManagerStorage, highLevelHandlers);

		inline constexpr auto HighLevelHandlersUnsigned = offsetof(NC2Logic::SAIManagerStorage, highLevelHandlers);
	}

	namespace TraderConfigWindowOffset
	{
		inline constexpr auto RightPreviewElement = 0x2B0;

		inline constexpr auto LeftPreviewElement = 0x29C;

		inline constexpr auto RightPreviewRectangleUnsigned = 0x2A0u;

		inline constexpr auto RightPreviewRectangle = 0x2A0;

		inline constexpr auto LeftPreviewRectangleUnsigned = 0x28Cu;

		inline constexpr auto LeftPreviewRectangle = 0x28C;

		inline constexpr auto RightImportControlsUnsigned = 0x180u;

		inline constexpr auto LeftImportControlsUnsigned = 0x78u;

		inline constexpr auto ClearImportsButton = 0x288;

		inline constexpr auto CloseCommand = 0x74;

		inline constexpr auto Owner = 0x70;
	}

	namespace GuiGfxButtonOffset
	{
		inline constexpr auto ImmediateTooltip = 0x44;

		inline constexpr auto Disabled = 0x50;

		inline constexpr auto Tooltip = 0x40;

		inline constexpr auto Receiver = 0x3C;

		inline constexpr auto Command = 0x38;

		inline constexpr auto Palette = 0x30;

		inline constexpr auto Centered = 0x34;

		inline constexpr auto GraphicsId = 0x2C;

		inline constexpr auto Selected = 0x36;

		inline constexpr auto RectangularStyle = 0x35;

		inline constexpr auto MessageParameter2 = 0x4C;

		inline constexpr auto MessageParameter1 = 0x48;
	}

	namespace RectangleStorageOffset
	{
		inline constexpr auto Height = 12;

		inline constexpr auto Width = 8;

		inline constexpr auto Y = 4;
	}

	namespace MouseManagerPositionOffset
	{
		inline constexpr auto Y = 4;

		inline constexpr auto X = 0;
	}

	namespace TitleScreenManagerOffset
	{
		inline constexpr auto FmvStem = 1;

		inline constexpr auto PictureStartedAtMilliseconds = 0xD4;

		inline constexpr auto PictureIndex = 0xD0;

		inline constexpr auto PostPictureSequence = 0xCD;

		inline constexpr auto PrePictureSequence = 0xCC;

		inline constexpr auto NextIntroPending = 0xCB;

		inline constexpr auto CursorHideRequested = 0xCA;

		inline constexpr auto FmvActive = 0xC9;

		inline constexpr auto FmvMode = 0;

		inline constexpr auto GrayscalePalette = 0xE4;

		inline constexpr auto Font = 0xE0;

		inline constexpr auto ScreenBitmap = 0xDC;

		inline constexpr auto PictureBitmap = 0xD8;
	}

	namespace ApplicationOffset
	{
		inline constexpr auto Window = 0x4C;
	}

	namespace FontOffset
	{
		inline constexpr auto Height = sizeof(void*) + 4;
		inline constexpr auto CharacterSpacing = sizeof(void*) + 8;
	}

	namespace MasterControlOffset
	{
		inline constexpr auto DifficultyLevel = 328;
		inline constexpr auto ProgramState = 0;
		inline constexpr auto HardwareMouseCursorEnabled = 0x14C;

		inline constexpr auto ExitRequested = 9;
	}

	namespace OptionsWindowOffset
	{
		inline constexpr auto SpeedComparisonValue = 0x74;

		inline constexpr auto Page = 0x70;
	}

	namespace GameSafetyBoxOffset
	{
		inline constexpr auto Mode = 0x70;
	}

	namespace ClockElementOffset
	{
		inline constexpr auto TimeText = (sizeof(void*) == 8 ? 73 : 0x31);

		inline constexpr auto AnalogMode = (sizeof(void*) == 8 ? 72 : 0x30);
	}

	namespace EnemyRangeResultOffset
	{
		inline constexpr auto DistanceUnsigned = 4u;
	}

	namespace EnemyObjectResultOffset
	{
		inline constexpr auto Distance = 4;

		inline constexpr auto ObjectId = 0;
	}

	namespace EnemyLandscapeResultOffset
	{
		inline constexpr auto Distance = 4;

		inline constexpr auto MapPosition = 0;
	}

	namespace EnemyLandscapeTargetListOffset
	{
		inline constexpr auto Count = 0;
	}

	namespace AIBuildHouseOffset
	{
		inline constexpr auto MapPosition = 4;

		inline constexpr auto MissionCheckId = 12;

		inline constexpr auto DevelopmentStage = 8;

		inline constexpr auto EngineType = 0;
	}

	namespace StaticHouseTypeOffset
	{
		inline constexpr auto StockUnsigned = 100u;
		inline constexpr auto ProductionIndicesUnsigned = 1240u;
		inline constexpr auto ProductionGoodsUnsigned = 1160u;
		inline constexpr auto FirstWorkerMaximumUnsigned = 20u;
		inline constexpr auto WorkersUnsigned = 16u;
		inline constexpr auto FirstStockMaximumAmountUnsigned = 0x68u;

		inline constexpr auto MainType = 0;

		inline constexpr auto StockIndicesUnsigned = 0x37Cu;

		inline constexpr auto FirstStockOutputFlagUnsigned = 0x6Cu;

		inline constexpr auto Stocks = 0x64;
	}

	namespace AISingleWallOffset
	{
		inline constexpr auto MapPosition = 2;
	}

	namespace AISingleWallArrayOffset
	{
		inline constexpr auto FirstPosition = 10;
	}

	namespace AISingleGateArrayOffset
	{
		inline constexpr auto FirstPosition = 10;
	}

	namespace AIBuildHouseArrayOffset
	{
		inline constexpr auto FirstPosition = 8;
	}

	namespace EngineLandscapeTypeOffset
	{
		inline constexpr auto LogicTypeUnsigned = 0x38u;
	}

	namespace GuiMousePointerOffset
	{
		inline constexpr auto MethodTable = 0;

		inline constexpr auto TooltipDeadlineTick = 4;

		inline constexpr auto TooltipMouseY = 0x10;

		inline constexpr auto TooltipMouseX = 0xC;

		inline constexpr auto TooltipText = 8;
	}

	namespace MouseManagerOffset
	{
		inline constexpr auto StateChanged = 32;
		inline constexpr auto LeftButtonDown = 8;

		inline constexpr auto PositionOverrideCallback = sizeof(void*) == 8 ? 0x50 : 0x4C;
	}

	namespace MapAreaOffset
	{
		inline constexpr auto RowCount = 0;

		inline constexpr auto Rows = sizeof(void*) == 8 ? 8 : 4;
	}

	namespace MapAreaRowOffset
	{
		inline constexpr auto X = 0;

		inline constexpr auto Width = 4;

		inline constexpr auto Y = 2;
	}

	namespace SelectedHumanActionWindowOffset
	{
		inline constexpr auto NameInputHandler = 0x78;

		inline constexpr auto LastRefreshTime = 0x78;

		inline constexpr auto JobList = 0x7C;

		inline constexpr auto ValidSelection = 0x70;

		inline constexpr auto SelectionChangeCount = 0x74;
	}

	namespace MultiPlayerGoalManagerOffset
	{
		inline constexpr auto PendingRemovalFlags = 0x14;

		inline constexpr auto LossOrders = 0x10;

		inline constexpr auto LostFlags = 0xC;

		inline constexpr auto WinOrders = 8;

		inline constexpr auto WonFlags = 4;

		inline constexpr auto Goals = 0x1A0;

		inline constexpr auto NextLossOrder = 0x198;

		inline constexpr auto NextWinOrder = 0x194;

		inline constexpr auto FirstGoalType = 0x1A0;

		inline constexpr auto GoalCount = 0x19C;

		inline constexpr auto Enabled = 0;
	}

	namespace MultiPlayerGoodsGoalOffset
	{
		inline constexpr auto RequirementCount = 4;

		inline constexpr auto FirstRequiredAmount = 0xC;

		inline constexpr auto FirstGoodId = 8;
	}

	namespace PlayerGoodsCountOffset
	{
		inline constexpr auto AmountsUnsigned = 4u;

		inline constexpr auto Total = 0;

		inline constexpr auto Amounts = 4;
	}

	namespace MultiPlayerPopulationGoalOffset
	{
		inline constexpr auto SoldiersOnly = 8;

		inline constexpr auto RequiredCount = 4;
	}

	namespace MultiPlayerGoalOffset
	{
		inline constexpr auto Type = 0;
	}

	namespace MultiPlayerMissionGoalOffset
	{
		inline constexpr auto PlayerFlagsUnsigned = 4u;

		inline constexpr auto PlayerFlags = 4;
	}


	namespace LandscapeTimerOffset
	{
		inline constexpr auto MapPositionY = 0x12;

		inline constexpr auto MapPosition = 0x10;

		inline constexpr auto EngineLandscapeId = 0x0C;

		inline constexpr auto DurationTicks = 8;

		inline constexpr auto ElapsedTicks = 4;

		inline constexpr auto Allocated = 0;
	}

	namespace TrianglePatternTypeOffset
	{
		inline constexpr auto HouseCanBeBuiltOn = 1;
		inline constexpr auto IsBorder = 6;
		inline constexpr auto HumanCanWalkOn = 0;
		inline constexpr auto MoveResistance = 8;
		inline constexpr auto IsWater = 5;
		inline constexpr auto IsLand = 4;
		inline constexpr auto BioCanPlantOn = 3;

		inline constexpr auto BioCanGrowOn = 2;
	}

	namespace EngineLandscapeLogicViewOffset
	{
		inline constexpr auto LogicType = 0;
	}

	namespace ConstructionSelectionWindowOffset
	{
		inline constexpr auto CachedHouseDevelopmentStagesUnsigned = 0x8Cu;

		inline constexpr auto CachedHouseEngineTypesUnsigned = 0x88u;

		inline constexpr auto CachedHouseDevelopmentStages = 0x8C;

		inline constexpr auto CachedHouseEngineTypes = 0x88;

		inline constexpr auto CachedHouseCount = 0x84;

		inline constexpr auto LastRefreshTime = 0x78;

		inline constexpr auto Page = 0x70;

		inline constexpr auto HouseList = 0x74;

		inline constexpr auto SpecialItem = 0x80;

		inline constexpr auto SpecialItemType = 0x7C;
	}

	namespace BitmapStorageOffset
	{
		inline constexpr auto RawMemory = sizeof(void*) == 8 ? 48 : 36;
		inline constexpr auto Palette = sizeof(void*) == 8 ? 104 : 76;
		inline constexpr auto TrueColorMemory = sizeof(void*) == 8 ? 64 : 44;

		inline constexpr auto HighColorMemory = sizeof(void*) == 8 ? 56 : 40;

		inline constexpr auto RowPixelCount = sizeof(void*) == 8 ? 72 : 48;

		inline constexpr auto Height = sizeof(void*) == 8 ? 24 : 20;

		inline constexpr auto Width = sizeof(void*) == 8 ? 20 : 16;

		inline constexpr auto Depth = sizeof(void*) == 8 ? 8 : 4;

		inline constexpr auto BytesPerPixel = sizeof(void*) == 8 ? 76 : 52;
	}

	namespace ObservationWindowOffset
	{
		inline constexpr auto HouseRevision = 0x88;

		inline constexpr auto VehicleRevision = 0x80;

		inline constexpr auto HumanRevision = 0x78;

		inline constexpr auto HouseId = 0x84;

		inline constexpr auto VehicleId = 0x7C;

		inline constexpr auto HumanId = 0x74;

		inline constexpr auto WorldElement = 0x70;
	}

	namespace ObjectReferenceOffset
	{
		inline constexpr auto Revision = 4;
	}

	namespace MenuWorldElementOffset
	{
		inline constexpr auto FollowVehicleRevision = 0x50;

		inline constexpr auto FollowVehicleId = 0x4C;

		inline constexpr auto FollowVehicle = 0x48;

		inline constexpr auto FollowHumanRevision = 0x44;

		inline constexpr auto FollowHumanId = 0x40;

		inline constexpr auto FollowHuman = 0x3D;

		inline constexpr auto FollowActive = 0x3C;

		inline constexpr auto Bitmap = 0x2C;

		inline constexpr auto Display = 0x30;

		inline constexpr auto DarkenPalette = 0x34;
	}

	namespace GroupSelectionElementOffset
	{
		inline constexpr auto ActionCommand = 0xFC;

		inline constexpr auto PressedCommand = 0xF8;

		inline constexpr auto Receiver = 0x100;

		inline constexpr auto HoveredObjectId = 44;
	}

	namespace HouseGoodElementOffset
	{
		inline constexpr auto Receiver = 0x108;

		inline constexpr auto HoverCommand = 0x104;

		inline constexpr auto PressedCommand = 0x100;

		inline constexpr auto GoodType = 0x34;

		inline constexpr auto HouseRevision = 0x30;

		inline constexpr auto HouseId = 0x2C;
	}

	namespace TributeDataOffset
	{
		inline constexpr auto DescriptionStringId = 0x10;

		inline constexpr auto TargetPlayer = 12;

		inline constexpr auto Allocated = 0;

		inline constexpr auto FirstAmountUnsigned = 0x18u;

		inline constexpr auto Paid = 1;

		inline constexpr auto FirstGoodUnsigned = 0x14u;

		inline constexpr auto GoodCount = 4;

		inline constexpr auto SourcePlayer = 8;
	}

	namespace AIHumanRequestOffset
	{
		inline constexpr auto RequiredJob = 20;

		inline constexpr auto HumanRevision = 16;

		inline constexpr auto TargetRevision = 8;

		inline constexpr auto HumanId = 12;

		inline constexpr auto TargetHouseIdOrPoint = 4;

		inline constexpr auto Type = 0;
	}

	namespace EmbeddedSavegameOffset
	{
		inline constexpr auto Data = offsetof(NC2Logic::SEmbeddedSavegameManagerStorage, data);

		inline constexpr auto Size = 0;
	}

	namespace LandscapeSelectionWindowOffset
	{
		inline constexpr auto StateChecksum = 0x84;

		inline constexpr auto Valency = 0x80;

		inline constexpr auto EngineType = 0x7C;

		inline constexpr auto PlayerId = 0x78;

		inline constexpr auto ValidSelection = 0x70;

		inline constexpr auto MapPosition = 0x72;
	}

	namespace HumanEquipmentWindowOffset
	{
		inline constexpr auto Slot = 0x74;

		inline constexpr auto Category = 0x70;

		inline constexpr auto ValidSelection = 0x78;

		inline constexpr auto SelectionChangeCount = 0x7C;
	}

	namespace PreIngameFmvOffset
	{
		inline constexpr auto CursorHideRequested = 0xCA;

		inline constexpr auto FmvActive = 0xC9;

		inline constexpr auto FmvRequested = 0;

		inline constexpr auto ScreenBitmap = 0xCC;
	}

	namespace GameAssistantPlayerOffset
	{
		inline constexpr auto SpearmanRequestBudgetUnsigned = 0x2Cu;

		inline constexpr auto SpearmanRequestBudget = 44;

		inline constexpr auto ArcherRequestBudget = 48;

		inline constexpr auto SwordsmanRequestBudget = 52;

		inline constexpr auto EquipSoldierMead = 29;

		inline constexpr auto EquipSoldierShoes = 28;

		inline constexpr auto EquipCivilianMead = 27;

		inline constexpr auto EquipCivilianIronTools = 26;

		inline constexpr auto EquipCivilianWoodenTools = 25;

		inline constexpr auto EquipCivilianShoes = 24;

		inline constexpr auto GirlRequestBudget = 36;

		inline constexpr auto RequestedGirls = 4;

		inline constexpr auto RecruitRequestBudget = 40;

		inline constexpr auto BoyRequestBudget = 32;

		inline constexpr auto RequestedSwordsmen = 20;

		inline constexpr auto RequestedArchers = 16;

		inline constexpr auto RequestedSpearmen = 12;

		inline constexpr auto RequestedUnarmedSoldiers = 8;

		inline constexpr auto RequestedBoys = 0;
	}

	namespace HappinessPlayerOffset
	{
		inline constexpr auto Value = 0;

		inline constexpr auto UnderAttack = 0x7C;

		inline constexpr auto AttackExpiresAtTick = 0x84;

		inline constexpr auto AttackExpiresAtTickUnsigned = 0x84u;

		inline constexpr auto AttackerPlayerIdUnsigned = 0x80u;

		inline constexpr auto UnderAttackUnsigned = 0x7Cu;

		inline constexpr auto EventsUnsigned = 4u;
	}

	namespace HappinessEventOffset
	{
		inline constexpr auto InitialValue = 8;

		inline constexpr auto ExpiresAtTick = 4;

		inline constexpr auto InitialValueUnsigned = 8u;

		inline constexpr auto ExpiresAtTickUnsigned = 4u;
	}

	namespace PlayerStatisticManagerOffset
	{
		inline constexpr auto CurrentByPlayer = offsetof(NC2Logic::SPlayerStatisticManagerStorage, currentByPlayer);

		inline constexpr auto HistoryByPlayer = 8;

		inline constexpr auto CurrentSampleIndex = 0;

		inline constexpr auto LastSampleTime = 4;

		inline constexpr auto CurrentByPlayerUnsigned = offsetof(NC2Logic::SPlayerStatisticManagerStorage, currentByPlayer);

		inline constexpr auto HistoryByPlayerUnsigned = 8u;
	}

	namespace MiscLandscapeLinkOffset
	{
		inline constexpr auto EngineLandscapeId = 8;

		inline constexpr auto LogicLandscapeType = 4;

		inline constexpr auto TribeType = 0;
	}

	namespace MiscHouseLandscapeLinkOffset
	{
		inline constexpr auto TotalWeight = 12;

		inline constexpr auto ChoiceCount = 8;

		inline constexpr auto LogicHouseType = 4;

		inline constexpr auto TribeType = 0;

		inline constexpr auto Choices = 16;
	}

	namespace WeightedLandscapeChoiceOffset
	{
		inline constexpr auto Weight = 4;
	}

	namespace LandscapeTransitionOffset
	{
		inline constexpr auto TargetLandscapeId = 4;
	}

	namespace LandscapeLogicViewOffset
	{
		inline constexpr auto MaximumValency = 4;

		inline constexpr auto Transitions = 0x30;

		inline constexpr auto TransitionCount = 0x2C;
	}

	namespace StaticLandscapeOffset
	{
		inline constexpr auto FrameGroups = sizeof(void*) == 8 ? 192 : 136;

		inline constexpr auto FrameGroupCount = sizeof(void*) == 8 ? 184 : 132;

		inline constexpr auto IsStaticUnsigned = sizeof(void*) == 8 ? 173u : 121u;

		inline constexpr auto LogicTypeUnsigned = 0x38u;

		inline constexpr auto PileableOnMap = 0x41;

		inline constexpr auto LogicType = 0x38;
	}

	namespace OwnedArrayHandlerOffset
	{
		inline constexpr auto OwnedEntries = sizeof(void*) == 8 ? 72 : 48;
	}

	namespace AllocatedRecordOffset
	{
		inline constexpr auto Allocated = 0;
	}

	namespace EngineDisplayRectangleOffset
	{
		inline constexpr auto Height = 20;

		inline constexpr auto Width = 16;

		inline constexpr auto BitmapY = 12;

		inline constexpr auto BitmapX = 8;

		inline constexpr auto WorldY = 4;

		inline constexpr auto WorldX = 0;
	}

	namespace MapDirtyBoundsOffset
	{
		inline constexpr auto MaximumPoint = 4;

		inline constexpr auto MaximumY = 6;

		inline constexpr auto MaximumX = 4;

		inline constexpr auto MinimumY = 2;

		inline constexpr auto MinimumX = 0;
	}

	namespace BitmapInfoHeaderOffset
	{
		inline constexpr auto ImageSize = 20;

		inline constexpr auto BitCount = 14;

		inline constexpr auto Planes = 12;

		inline constexpr auto Height = 8;

		inline constexpr auto Width = 4;

		inline constexpr auto Size = 0;
	}

	namespace FrameRateCalculatorOffset
	{
		inline constexpr auto FrameRate = 0x80;

		inline constexpr auto FrameIntervalsUnsigned = 4u;

		inline constexpr auto NextIntervalIndex = 0x7C;

		inline constexpr auto PreviousTick = 0;
	}

	namespace MessagePresentationWindowOffset
	{
		inline constexpr auto UniqueId = 112;
		inline constexpr auto MessageId = 0x70;
	}

	namespace CountedDwordListOffset
	{
		inline constexpr auto ItemsUnsigned = 4u;
	}

	namespace SectorPlayerOffset
	{
		inline constexpr auto CivilianHouseHeadUnsigned = 16u;
		inline constexpr auto MilitaryHouseHeadUnsigned = 40u;
		inline constexpr auto CivilianVehicleHeadUnsigned = 8u;
		inline constexpr auto MilitaryVehicleHeadUnsigned = 32u;
		inline constexpr auto CivilianHumanHeadUnsigned = 0u;
		inline constexpr auto MilitaryHumanHeadUnsigned = 24u;
		inline constexpr auto MilitaryVehicleHead = 0x20;

		inline constexpr auto MilitaryHumanHead = 0x18;

		inline constexpr auto AggressiveAnimalCount = 0x3C;

		inline constexpr auto CivilianHouseCount = 0x14;

		inline constexpr auto MilitaryHumanCount = 0x1C;

		inline constexpr auto CivilianCount = 4;

		inline constexpr auto MilitaryHouseCount = 0x2C;

		inline constexpr auto MilitaryVehicleCount = 0x24;
	}

	namespace SectorAttachmentOffset
	{
		inline constexpr auto NeutralAggressiveAnimalCount = 0x53C;
	}

	namespace SectorObjectListOffset
	{
		inline constexpr auto Count = 4;
	}

	namespace CountedArrayOffset
	{
		inline constexpr auto EntriesUnsigned = 4u;
		inline constexpr auto Entries = 4;
	}

	namespace HouseArrowSlotOffset
	{
		inline constexpr auto Parameter = 8;

		inline constexpr auto Y = 4;
	}

	namespace EngineHouseConstructionListViewOffset
	{
		inline constexpr auto LastPercentage = 20;

		inline constexpr auto FirstPercentage = 16;

		inline constexpr auto FirstShadowBobId = 12;

		inline constexpr auto FirstBobId = 8;

		inline constexpr auto FirstUpgradeFlag = 4;
	}

	namespace PixelPointOffset
	{
		inline constexpr auto X = 0;
		inline constexpr auto Y = 4;
	}

	namespace StaticLandscapeAreaListOffset
	{
		inline constexpr auto Entries = sizeof(void*) == 8 ? 8 : 4;
	}

	namespace StaticLandscapeAreaOffset
	{
		inline constexpr auto Area = sizeof(void*) == 8 ? 8 : 4;
	}

	namespace MissionOnscreenInfoOffset
	{
		inline constexpr auto StringId = 4;

		inline constexpr auto Type = 0;
	}

	namespace MissionTypedAreaInfoOffset
	{
		inline constexpr auto TargetCount = 0x18;

		inline constexpr auto Radius = 0x14;

		inline constexpr auto CenterY = 0x10;

		inline constexpr auto CenterX = 0xC;

		inline constexpr auto ObjectType = 8;
	}

	namespace MissionHumanAreaInfoOffset
	{
		inline constexpr auto TargetCount = 0x14;

		inline constexpr auto Radius = 0x10;

		inline constexpr auto CenterY = 0xC;

		inline constexpr auto CenterX = 8;
	}

	namespace TechTreeFontPaletteOffset
	{
		inline constexpr auto ForbiddenColor = sizeof(void*) + 400;
		inline constexpr auto UnavailableColor = sizeof(void*) + 720;
		inline constexpr auto NormalColor = sizeof(void*) + 1020;
	}

	namespace MessageMiniButtonOffset
	{
		inline constexpr auto MessageId = (sizeof(void*) == 8 ? 72 : 0x30);
	}

	namespace MissionOnscreenWindowOffset
	{
		inline constexpr auto FirstCompactFontFlag = 0x214;
		inline constexpr auto TextRows = 0x84;
		inline constexpr auto LastRefreshTime = 0x80;
		inline constexpr auto MissionStrings = 0x7C;
	}

	namespace OverviewElementOffset
	{
		inline constexpr auto DrawWalls = 32884;
		inline constexpr auto MouseWheelZoomEnabled = 0x807B;
		inline constexpr auto UsePatternDebugColors = 0x8075;
		inline constexpr auto FullRedrawRequested = 0x8044;
		inline constexpr auto DrawGuides = 0x8072;
		inline constexpr auto DrawHouses = 0x8070;
		inline constexpr auto DrawVehicles = 0x806F;
		inline constexpr auto DrawAnimals = 0x806E;
		inline constexpr auto DrawMilitaryHumans = 0x806D;
		inline constexpr auto DrawCivilians = 0x806C;
	}

	namespace WorldClickHandlerOffset
	{
		inline constexpr auto Command = 4;
	}

	namespace TrueColorConverterOffset
	{
		inline constexpr auto RedLeftShift = 0x1C;
		inline constexpr auto RedMask = 4;
	}

	namespace StaticLandscapeFramesOffset
	{
		inline constexpr auto Count = 4;
		inline constexpr auto Frames = 8;
	}

	namespace AIRoadManagerOffset
	{
		inline constexpr auto PreviousHouseId = 0;
	}

	namespace GlobalGameSettingsWindowOffset
	{
		inline constexpr auto LastRefreshTime = 0x70;
	}

	namespace NetworkInfoWindowOffset
	{
		inline constexpr auto Page = 0x70;
	}

	namespace KeyAssignmentViewOffset
	{
		inline constexpr auto FirstControlFlagUnsigned = 10u;
		inline constexpr auto FirstShiftFlagUnsigned = 9u;
		inline constexpr auto FirstAuxiliaryCodeUnsigned = 8u;
		inline constexpr auto FirstVirtualKeyUnsigned = 4u;
	}

	namespace AnimalIteratorOffset
	{
		inline constexpr auto CenterX = 8;
		inline constexpr auto CenterY = 10;
	}

	namespace GoodGraphicsOffset
	{
		inline constexpr auto PalettePatchId = 0;
	}

	namespace MunitionCallbackOffset
	{
		inline constexpr auto FlightTicks = 32;
		inline constexpr auto MunitionType = 28;
		inline constexpr auto SourceHeight = 8;
	}

	namespace DirectionalParticleCallbackOffset
	{
		inline constexpr auto DirectionPoint = 8;
		inline constexpr auto MapPoint = 4;
		inline constexpr auto Human = 24;
		inline constexpr auto PlayerId = 20;
	}

	namespace HumanReferenceOffset
	{
		inline constexpr auto UniqueId = 4;
		inline constexpr auto HumanId = 0;
	}

	namespace DeadHumanDataManagerOffset
	{
		inline constexpr auto Count = 0;
		inline constexpr auto Records = 8;
	}

	namespace ColorRGBOffset
	{
		inline constexpr auto PackedColor = 0;
	}

	namespace GuiVirtualTableOffset
	{
		inline constexpr auto DeletingDestructor = 0;
		inline constexpr auto MessageHandle = 2 * sizeof(void*);
	}

	namespace BitmapFileHeaderOffset
	{
		inline constexpr auto PixelDataOffset = 10;
		inline constexpr auto FileSize = 2;
		inline constexpr auto Signature = 0;
	}

	namespace HouseWindowLimits
	{
		inline constexpr auto StockGoodsPerPage = 14;
	}

	namespace VehicleWindowLimits
	{
		inline constexpr auto StockGoodsPerPage = 14;
	}

	namespace HouseConstructionGoodsOffset
	{
		inline constexpr auto GoodsUnsigned = 4u;
		inline constexpr auto Count = 0;
	}

	namespace StaticHouseStockOffset
	{
		inline constexpr auto IsOutputStockUnsigned = 8u;
		inline constexpr auto MaximumAmountUnsigned = 4u;
	}

	namespace StaticTribeOffset
	{
		inline constexpr auto FirstJobDefaultGoodUnsigned = 27736u;
		inline constexpr auto FirstGoodProductionHouseUnsigned = 84268u;
		inline constexpr auto GoodProductionUnsigned = 84264u;
	}

	namespace TribeExperienceRequirementOffset
	{
		inline constexpr auto Jobs = 8;
		inline constexpr auto JobCount = 4;
	}

	namespace StaticVehicleTypeOffset
	{
		inline constexpr auto PassengerVectorY = 452;
		inline constexpr auto AllowedPassengerTypesUnsigned = 388u;
	}

	namespace HyperlinkResourceOffset
	{
		inline constexpr auto Palette = 264;
		inline constexpr auto Resource = 260;
	}

	namespace AIMainTaskHouseDataOffset
	{
		inline constexpr auto EngineHouseType = 16;
	}

	namespace AIMainTaskDiplomacyDataOffset
	{
		inline constexpr auto OtherPlayerId = 16;
	}

	namespace HouseRemoveWindowOffset
	{
		inline constexpr auto SelectionChangeCount = 116;
		inline constexpr auto SelectionValidFlag = 112;
	}

	namespace SessionSummaryArrayCookieOffset
	{
		inline constexpr auto ElementCount = 0;
	}

	namespace GameControlCommandOffset
	{
		inline constexpr auto Value = 4;
	}

	namespace ActorPlayerOffset
	{
		inline constexpr auto HumanOrAnimalPlayerIdUnsigned = 16u;
		inline constexpr auto HumanOrAnimalPlayerId = 16;
	}

	namespace AnimalSoundManagerOffset
	{
		inline constexpr auto LastUpdate = 0;
	}

	namespace DirectMusicManagerOffset
	{
		inline constexpr auto CurrentEventId = (sizeof(void*) == 8 ? 24 : 20);
	}

	namespace EngineTrianglePatternOffset
	{
		inline constexpr auto LogicType = 56;
	}

	namespace PlayerAllowanceStride
	{
		inline constexpr auto JobsAndEquipmentUnsigned = 56u;
		inline constexpr auto GoodsUnsigned = 66u;
	}

	namespace SectorLimits
	{
		inline constexpr auto PlayerSlotsUnsigned = 21u;
	}

	namespace MenuPlayerSlotOffset
	{
		inline constexpr auto ColorId = 12;
		inline constexpr auto Type = 4;
		inline constexpr auto Flags = 8;
	}

	namespace ObjectArrayCookieOffset
	{
		inline constexpr auto ElementCount = 0;
	}

	namespace TextureCacheEntryOffset
	{
		inline constexpr auto Palette = 120 + sizeof(void*);
		inline constexpr auto Bitmap = 120;
	}

	namespace LogicParticleOffset
	{
		inline constexpr auto MapPositionX = 88;
		inline constexpr auto MapPositionY = 90;
	}

	namespace AITradeManagerOffset
	{
		inline constexpr auto FirstHumanUniqueId = 8;
		inline constexpr auto FirstHumanId = 4;
		inline constexpr auto Count = 0;
	}

	namespace StringOutputWindowOffset
	{
		inline constexpr auto Strings = 112;
	}

	namespace HomeQualityEnhancerOffset
	{
		inline constexpr auto Value = 4;
	}

	namespace SectorAttachmentManagerOffset
	{
		inline constexpr auto SectorAuxiliaryData = 8 + sizeof(void*);
		inline constexpr auto SectorData = 8;
		inline constexpr auto GridWidth = 4;
		inline constexpr auto GridHeight = 0;
	}

	namespace SectorEnemySearchCacheOffset
	{
		inline constexpr auto LastLandscapeSearchTick = 4;
		inline constexpr auto FoundTarget = 8;
		inline constexpr auto LastSearchTick = 0;
	}

	namespace EmbeddedMapPointPairOffset
	{
		inline constexpr auto SecondX = 8;
		inline constexpr auto SecondY = 10;
		inline constexpr auto FirstX = 4;
		inline constexpr auto FirstY = 6;
	}

	namespace HumanSearchPilePointOffset
	{
		inline constexpr auto PointsUnsigned = 8u;
	}

	namespace StorableOffset
	{
		inline constexpr auto VirtualTable = 0;
	}

	namespace StorableVirtualTableOffset
	{
		inline constexpr auto DeletingDestructor = 3 * sizeof(void*);
	}

	namespace GuiWindowClipSizeOffset
	{
		inline constexpr auto Height = offsetof(NXBaseGui::SBaseWindowStorage, clippingRectangle) + 12;
		inline constexpr auto Width = offsetof(NXBaseGui::SBaseWindowStorage, clippingRectangle) + 8;
	}

	namespace GouraudShaderOffset
	{
		inline constexpr auto Flags = 3 * sizeof(void*);
	}

	namespace WallRoadPathfinderOffset
	{
		inline constexpr auto Target = 36;
		inline constexpr auto Source = 32;
	}

	namespace AIHighLevelVillageActiveViewOffset
	{
		inline constexpr auto FirstGoodImportingFlag = 641;
	}

	namespace GuiDesktopOffset
	{
		inline constexpr auto LeftButtonDownFlag = offsetof(NXBaseGui::SDesktopStorage, mLeftButtonDown);
	}

	namespace PlayerSpecialItemOffset
	{
		inline constexpr auto ItemUnsigned = 4u;
	}

	namespace LandscapeLogicTypeOffset
	{
		inline constexpr auto TransitionMask = 44;
	}

	namespace AIMainTaskCalcVarsOffset
	{
		inline constexpr auto TargetPoint = 12;
		inline constexpr auto SourcePoint = 8;
	}

	namespace AISingleSoldiersOffset
	{
		inline constexpr auto DefaultPositionX = 16838;
		inline constexpr auto DefaultPositionY = 16840;
	}

	namespace DynamicSelectionStateOffset
	{
		inline constexpr auto HighlightedDisplacement = 1;
	}

	namespace GameMapContinentArrayViewOffset
	{
		inline constexpr auto FirstSeedPointYUnsigned = 10u;
		inline constexpr auto FirstSeedPointXUnsigned = 8u;
	}

}
