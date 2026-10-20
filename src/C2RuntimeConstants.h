#pragma once

// Windows game random seeds, state sentinels and caller-defined sound instance tags.
// Values and integer literal types are retained from the Windows reconstruction.
namespace NC2Runtime
{
	namespace GuideRoutingLimits
	{
		inline constexpr auto DistanceBuckets = 40;
		inline constexpr auto ConnectionsPerGuide = 8;
	}

	namespace GoodSearchLimits
	{
		inline constexpr auto WorkCenterPileRadius = 20;

		inline constexpr auto QuarryPileRadius = 10;

		inline constexpr auto SearchRadiusUnsigned = 40u;

		inline constexpr auto SearchRadius = 40;

		inline constexpr auto LandscapeCandidates = 10;
		inline constexpr auto HouseCandidates = 2;
		inline constexpr auto GuideCandidates = 3;
		inline constexpr auto DistanceSlackUnsigned = 10u;
		inline constexpr auto Goods = 66;
	}

	namespace ProductionCounter
	{
		inline constexpr auto ImpossibleUnsigned = 14u;
		inline constexpr auto Impossible = 14;
		inline constexpr auto Incapable = 13;
		inline constexpr auto Forbidden = 12;
		inline constexpr auto Infinite = 11;
	}


	namespace HumanAssistantCommandHistory
	{
		inline constexpr auto Capacity = 10;
		inline constexpr auto TicksPerDurationUnit = 0x2D0u;
	}

	namespace MoveableAnimationMode
	{
		inline constexpr auto Unspecified = -1;
		inline constexpr auto ProgressFrames = 0;
		inline constexpr auto CyclicFrames = 1;
		inline constexpr auto Compound = 2;
	}

	namespace MoveableAnimationPartType
	{
		inline constexpr auto Walk = 1;
		inline constexpr auto WalkUnsigned = 1u;
		inline constexpr auto AtomicAnimation = 2;
		inline constexpr auto AtomicAnimationUnsigned = 2u;
		inline constexpr auto HouseBobOverlay = 3;
		inline constexpr auto HouseBobOverlayUnsigned = 3u;
		inline constexpr auto LandscapeOverlay = 4;
		inline constexpr auto LandscapeOverlayUnsigned = 4u;
	}

	namespace MoveableAnimationLimits
	{
		inline constexpr auto FramesPerList = 500;
		inline constexpr auto CompoundParts = 50;
		inline constexpr auto OverlaysPerLayer = 5;
	}

	namespace HumanAppearanceLimits
	{
		inline constexpr auto Goods = 66;
		inline constexpr auto GraphicsChoices = 5;
		inline constexpr auto Tribes = 8;
		inline constexpr auto PlayerColors = 10;
		inline constexpr auto InhabitantSigns = 10;
		inline constexpr auto SoldierSigns = 5;
		inline constexpr auto DestinationRanges = 32;
	}

	namespace HumanPaletteRange
	{
		inline constexpr auto FirstHeadRange = 16;
		inline constexpr auto LocalRangeMask = 15u;
		inline constexpr auto Bytes = 64;
		inline constexpr auto BytesUnsigned = 64u;
	}

	namespace FastMoveableAccessLimits
	{
		inline constexpr auto TribeSlots = 42;
		inline constexpr auto JobSlots = 56;
		inline constexpr auto JobSlotsHex = 0x38;
		inline constexpr auto RandomAnimationChoices = 10;
		inline constexpr auto FramesPerWalkList = 200;
	}

	namespace WeatherGrid
	{
		inline constexpr auto SectorWidthInMapPoints = 10;
		inline constexpr auto SectorCenterInMapPoints = 5;
		inline constexpr auto SectorRoundUp = 9;
		inline constexpr auto FullPercentage = 100;
		inline constexpr auto DensityMaximum = 10000;
		inline constexpr auto DensityUnitsPerPercent = 100u;
	}

	namespace WeatherType
	{
		inline constexpr auto None = 0;
		inline constexpr auto Rain = 1;
		inline constexpr auto Snow = 2;
		inline constexpr auto Sand = 3;
	}

	namespace InputTargetPointLimits
	{
		inline constexpr auto Points = 300;
	}

	namespace InputSelectionLimits
	{
		inline constexpr auto Humans = 300;
		inline constexpr auto Vehicles = 30;
	}

	namespace DisplaySelectionLimits
	{
		inline constexpr auto SecondaryMarks = 30;
		inline constexpr auto BobsPerHit = 20;
	}

	namespace DisplayObjectMarkType
	{
		inline constexpr auto LocalOrUnowned = 1u;
		inline constexpr auto OtherPlayer = 3u;
	}

	namespace DisplayProjection
	{
		inline constexpr auto DefaultVerticalSpacing = 19.0f;
		inline constexpr auto DefaultHorizontalSpacing = 34.0f;
		inline constexpr auto DefaultHeightScale = 1.2f;
		inline constexpr auto MinimumVerticalSpacing = 1.0f;
		inline constexpr auto MinimumHorizontalSpacing = 2.0f;
		inline constexpr auto MinimumHeightScale = 0.0f;
		inline constexpr auto FixedPointScale = 256.0f;
		inline constexpr auto DoubledFixedPointScale = 512.0f;
		inline constexpr auto FractionBits = 8;
		inline constexpr auto RoundingBias = 127u;
	}

	namespace DisplayScrolling
	{
		inline constexpr auto MinimumWorldX = -100;
		inline constexpr auto MinimumWorldY = -200;
		inline constexpr auto MaximumWorldMargin = 100u;
		inline constexpr auto FullRebuildAreaPercent = 75u;
	}

	namespace EarthquakeDisplacement
	{
		inline constexpr auto HorizontalRange = 41u;
		inline constexpr auto HorizontalCenter = 20u;
		inline constexpr auto VerticalRange = 21u;
		inline constexpr auto VerticalCenter = 10u;
	}

	namespace RandomSeed
	{
		inline constexpr auto Game = 0x18273645;
		inline constexpr auto DisplayEffects = 0x5432ACE0u;
	}

	namespace SoundTag
	{
		inline constexpr auto MissionBriefingVoice = 0x65655656;
	}

	namespace AnimalFoodValue
	{
		inline constexpr auto Slaughtered = -100000;
		inline constexpr auto SlaughteredBits = 0xFFFE7960u;
	}

	namespace WalkingProgress
	{
		inline constexpr auto RelocationThreshold = 5000u;
		inline constexpr auto FullStep = 10000u;
		inline constexpr auto FullStepSigned = 10000;
	}

	namespace PathfinderDirectionResult
	{
		inline constexpr auto Deferred = -4;
		inline constexpr auto Stopped = -3;
		inline constexpr auto Arrived = -2;
		inline constexpr auto Failed = -1;
	}

	namespace PathfinderCalculationValue
	{
		inline constexpr auto BlockedStartAreaFlag = 0x200;
		inline constexpr auto DirectionMaskBase = 0x400u;
	}

	namespace PathfinderSearchState
	{
		inline constexpr auto Stopped = 0;
		inline constexpr auto SearchRequired = 1;
		inline constexpr auto SearchRequiredUnsigned = 1u;
		inline constexpr auto RouteAvailable = 2;
		inline constexpr auto RouteAvailableUnsigned = 2u;
		inline constexpr auto Arrived = 3;
		inline constexpr auto ArrivedUnsigned = 3u;
	}

	namespace PathfinderTargetMode
	{
		inline constexpr auto Position = 1;
		inline constexpr auto PositionUnsigned = 1u;
		inline constexpr auto Moveable = 2;
		inline constexpr auto MoveableUnsigned = 2u;
	}

	namespace BuildLimits
	{
		inline constexpr auto Players = 20;
		inline constexpr auto WorkerSearchRadius = 50;
		inline constexpr auto RoadWorkers = 3;
		inline constexpr auto WallWorkers = 5;
		inline constexpr auto RoadPoints = 200;
		inline constexpr auto WallPoints = 100;
	}

	namespace MapTerrainFlags
	{
		inline constexpr auto BuildBlockedUnsigned = 1u;

		inline constexpr auto RoadBuildPending = 4;
		inline constexpr auto WallBuildPending = 8;
		inline constexpr auto WallBuildPendingUnsigned = 8u;
		inline constexpr auto RoadBuildPendingUnsigned = 4u;
		inline constexpr auto RoadUnsigned = 2u;
	}

	namespace PlayerAllowanceStride
	{
		inline constexpr auto Houses = 55;

		inline constexpr auto Goods = 66;

		inline constexpr auto JobsAndEquipment = 56;

		inline constexpr auto VehiclesUnsigned = 7u;
		inline constexpr auto HousesUnsigned = 55u;
		inline constexpr auto JobsAndEquipmentUnsigned = 56u;
		inline constexpr auto GoodsUnsigned = 66u;
	}

	namespace PlayerAllowanceSlots
	{
		inline constexpr auto Tribes = 42;
	}

	namespace GuiMessageLimits
	{
		inline constexpr auto Capacity = 200;

		inline constexpr auto LifetimeTicksUnsigned = 3600u;
	}

	namespace MissionCapacity
	{
		inline constexpr auto Missions = 150;

		inline constexpr auto GoalsAndResults = 30;
	}

	namespace AIConditionReference
	{
		inline constexpr auto UnresolvedOperand = 999999;

		inline constexpr auto AlwaysInactive = 100001;

		inline constexpr auto AlwaysActive = 100000;
	}

	namespace AIConditionLimits
	{
		inline constexpr auto TicksPerScriptSecondUnsigned = 12u;

		inline constexpr auto TicksPerScriptMinuteUnsigned = 720u;

		inline constexpr auto Operands = 10;

		inline constexpr auto PlayerDeathGraceTicks = 720;

		inline constexpr auto RecheckPasses = 10;

		inline constexpr auto Slots = 100;
	}

	namespace MerchantLimits
	{
		inline constexpr auto DefaultCapacity = 1000;

		inline constexpr auto Goods = 66;

		inline constexpr auto TradeAgreements = 60;

		inline constexpr auto Houses = 2;
	}

	namespace AIEquipmentLimits
	{
		inline constexpr auto RecruitmentLimit = 190;

		inline constexpr auto Armors = 5;

		inline constexpr auto Tribes = 8;

		inline constexpr auto Weapons = 22;
	}

	namespace AttachedVehicleSelectionLimits
	{
		inline constexpr auto Rows = 30;
	}

	namespace NetworkCommandLimits
	{
		inline constexpr auto MaximumFrequency = 12;

		inline constexpr auto MinimumFrequency = 1;

		inline constexpr auto TicksPerSecondUnsigned = 12u;

		inline constexpr auto ChecksumExpiryTicksUnsigned = 1440u;

		inline constexpr auto ChecksumExpiryTicks = 1440;

		inline constexpr auto Players = 20;

		inline constexpr auto SamplesPerPlayer = 10;

		inline constexpr auto PacketCommandsUnsigned = 9u;
	}

	namespace MissionTiming
	{
		inline constexpr auto TicksPerSecondUnsigned = 12u;

		inline constexpr auto EvaluationIntervalTicksUnsigned = 36u;
	}

	namespace VehicleCommandQueue
	{
		inline constexpr auto Capacity = 22;
	}

	namespace VehicleWeaponSlot
	{
		inline constexpr auto CatapultUnsigned = 21u;
	}

	namespace VehicleAtomicAction
	{
		inline constexpr auto Dock = 0x54;

		inline constexpr auto Attack = 0x51;

		inline constexpr auto Walk = 1;

		inline constexpr auto None = 0;
	}

	namespace VehicleWalkTiming
	{
		inline constexpr auto TurningTicks = 2;

		inline constexpr auto ProgressRoundingBiasUnsigned = 9999u;

		inline constexpr auto MinimumTicks = 3;

		inline constexpr auto CatapultMultiplierUnsigned = 2u;

		inline constexpr auto BaseTicksUnsigned = 4u;

		inline constexpr auto TerrainMultiplierUnsigned = 2u;
	}

	namespace MapWalkingFlags
	{
		inline constexpr auto SpeedMaskUnsigned = 15u;

		inline constexpr auto SpeedShift = 3;

		inline constexpr auto VehicleSizeMaskUnsigned = 7u;
	}

	namespace ObjectGroupLimits
	{
		inline constexpr auto Objects = 100;

		inline constexpr auto Groups = 10;
	}

	namespace DeadHumanLimits
	{
		inline constexpr auto Capacity = 50;

		inline constexpr auto NameBytes = 40;
	}

	namespace GuiCallbackPriority
	{
		inline constexpr auto InputManager = 20;

		inline constexpr auto StateManagers = 10;
	}

	namespace PathfinderSearchLimits
	{
		inline constexpr auto InitialRadius = 5;
		inline constexpr auto Players = 20;
		inline constexpr auto MaximumRadiusUnsigned = 511u;
		inline constexpr auto MaximumBudgetUnsigned = 511u;

		inline constexpr auto DirectionRunsUnsigned = 20u;
		inline constexpr auto MaximumRunLengthUnsigned = 255u;
		inline constexpr auto QueuePointsUnsigned = 4000u;

		inline constexpr auto PlainQueuesUnsigned = 2u;
		inline constexpr auto PlainQueueMaskUnsigned = 1u;
		inline constexpr auto WeightedQueues = 12;
		inline constexpr auto WeightedQueuesUnsigned = 12u;
		inline constexpr auto ResistanceQueues = 16;
		inline constexpr auto ResistanceQueuesUnsigned = 16u;
		inline constexpr auto ResistanceQueueMaskUnsigned = 15u;

		inline constexpr auto WeightedRadiusMultiplierUnsigned = 2u;
		inline constexpr auto WeightedBudgetPaddingUnsigned = 12u;
		inline constexpr auto ResistanceBudgetMultiplierUnsigned = 8u;
		inline constexpr auto NonPreferredDirectionCostUnsigned = 4u;

		inline constexpr auto DirectedStopBudgetUnsigned = 13u;
		inline constexpr auto ResistanceStopBudgetUnsigned = 17u;
	}

	namespace PathfinderDirection
	{
		inline constexpr auto Unassigned = 1000;

		inline constexpr auto UnassignedUnsigned = 1000u;
	}

	namespace PathfinderGeometry
	{
		inline constexpr auto NeighbourDirectionsUnsigned = 6u;

		inline constexpr auto OppositeDirectionOffsetUnsigned = 3u;

		inline constexpr auto NeighbourDirections = 6;
	}

	namespace HouseInhabitantsSelectionLimits
	{
		inline constexpr auto HouseAttachments = 20;

		inline constexpr auto Humans = 100;
	}

	namespace TextElementLimits
	{
		inline constexpr auto CaptionCharacters = 0xC7;
	}

	namespace GuiVirtualSlot
	{
		inline constexpr auto Draw = 3;

		inline constexpr auto DeletingDestructor = 0;

		inline constexpr auto ImmediateTooltip = 14;

		inline constexpr auto TooltipAvailable = 13;

		inline constexpr auto DoPressedAction = 17;

		inline constexpr auto MessageHandle = 2;
	}

	namespace AIGuideLimits
	{
		inline constexpr auto SerializedBytes = 0x18;

		inline constexpr auto CandidateLinkSystems = 10;
	}

	namespace EngineMapEncoding
	{
		inline constexpr auto TrianglesPerLayer = 2;

		inline constexpr auto TrianglesPerLayerUnsigned = 2u;
	}

	namespace MacroMapHeightMode
	{
		inline constexpr auto Subtract = 2;

		inline constexpr auto Add = 1;

		inline constexpr auto Set = 0;
	}

	namespace EngineMapDirtyBounds
	{
		inline constexpr auto EmptyMinimum = 32000;
	}

	namespace AIGoodsLimits
	{
		inline constexpr auto Goods = 66;

		inline constexpr auto Villages = 8;

		inline constexpr auto VillageIds = 8;
	}

	namespace AIGoodsProducerAddressing
	{
		inline constexpr auto WorkerRequirementIndexBias = 48;

		inline constexpr auto GoodIndexScale = 2;
	}

	namespace AIAnimalCatcherLimits
	{
		inline constexpr auto DesiredQuantity = 2;

		inline constexpr auto Types = 2;
	}

	namespace HumanSoundLimits
	{
		inline constexpr auto Records = 24;

		inline constexpr auto ResponseSoundsUnsigned = 15u;

		inline constexpr auto GenericChanceRange = 2000;

		inline constexpr auto SexAgeCategories = 3;

		inline constexpr auto Tribes = 8;

		inline constexpr auto SexAgeCategoriesUnsigned = 3u;
	}

	namespace StaticBobCacheLimits
	{
		inline constexpr auto FilenameCopyLength = 0x4F;
	}

	namespace AmbientSoundLimits
	{
		inline constexpr auto StaticSoundsUnsigned = 40u;

		inline constexpr auto ActiveLoops = 10;

		inline constexpr auto LastSectorEntry = 2;

		inline constexpr auto SectorEntries = 3;

		inline constexpr auto GroupCount = 100;
	}

	namespace AmbientSoundGrid
	{
		inline constexpr auto PatternTrianglesPerSectorUnsigned = 200u;

		inline constexpr auto FullPercentageUnsigned = 100u;

		inline constexpr auto MicroPointsPerSectorUnsigned = 20u;

		inline constexpr auto MicroSectorCenterUnsigned = 10u;

		inline constexpr auto MacroPointsPerSectorUnsigned = 10u;

		inline constexpr auto MacroPointsPerSector = 10;

		inline constexpr auto MacroRoundUp = 9;
	}

	namespace MissionBriefingHistoryLimits
	{
		inline constexpr auto LastEntry = 49;

		inline constexpr auto LastEntryUnsigned = 49u;

		inline constexpr auto Capacity = 50;
	}

	namespace MissionInfoGeometry
	{
		inline constexpr auto MapPreviewHeight = 220;

		inline constexpr auto MapPreviewWidth = 280;

		inline constexpr auto PortraitHeight = 80;

		inline constexpr auto PortraitWidth = 50;

		inline constexpr auto Height = 420;

		inline constexpr auto Width = 500;

		inline constexpr auto HeightUnsigned = 420u;

		inline constexpr auto WidthUnsigned = 500u;
	}

	namespace AIManagerLimits
	{
		inline constexpr auto Players = 20;
	}

	namespace AIManagerTiming
	{
		inline constexpr auto PlayerSlotTicks = 3;

		inline constexpr auto CycleTicks = 60;
	}

	namespace TraderCommandBits
	{
		inline constexpr auto GoodTypeShiftUnsigned = 16u;

		inline constexpr auto ImportEnabledUnsigned = 0x1000000u;
	}

	namespace TraderConfigLimits
	{
		inline constexpr auto VisibleGoods = 55;

		inline constexpr auto RouteHouses = 2;
	}

	namespace TitleScreenLimits
	{
		inline constexpr auto TransitionDisplayUpdates = 5;

		inline constexpr auto PaletteColors = 256;
	}

	namespace TitleScreenTiming
	{
		inline constexpr auto PictureDurationMillisecondsUnsigned = 20000u;
	}

	namespace TitleScreenGeometry
	{
		inline constexpr auto IntroHeight = 480;

		inline constexpr auto IntroWidth = 640;
	}

	namespace DisplayDepthBits
	{
		inline constexpr auto TrueColor = 32;
	}

	namespace AutosaveDisplayTiming
	{
		inline constexpr auto TicksPerSecondUnsigned = 12u;

		inline constexpr auto TicksPerMinuteUnsigned = 720u;
	}

	namespace OptionsWindowLimits
	{
		inline constexpr auto UserSaveSlots = 100;

		inline constexpr auto Pages = 6;
	}

	namespace EnemyRangeLimits
	{
		inline constexpr auto NeutralPlayer = 20;

		inline constexpr auto PlayersIncludingNeutral = 21;

		inline constexpr auto Players = 20;
	}

	namespace EnemyRangeResultComponent
	{
		inline constexpr auto Distance = 1;

		inline constexpr auto MapPosition = 0;
	}

	namespace AISinglePlayerLimits
	{
		inline constexpr auto HouseBuildSearchRadius = 12;

		inline constexpr auto EnemySearchRadius = 40;

		inline constexpr auto DefenceHouseSearchRadius = 10000;

		inline constexpr auto DefaultMainTaskRadius = 40;

		inline constexpr auto DefaultSoldierRadius = 15;

		inline constexpr auto NearbyHumanRadius = 40;

		inline constexpr auto DefaultUnitLimit = 0x50;

		inline constexpr auto BuildHouses = 100;

		inline constexpr auto EnergyRepairAndFamilyInterval = 12;

		inline constexpr auto ConstructionAndInhabitantsInterval = 6;

		inline constexpr auto HouseBuildInterval = 3;

		inline constexpr auto SoldierTaskUpdateInterval = 2;

		inline constexpr auto DefenceTimeoutTicks = 360;

		inline constexpr auto UnfinishedHouses = 3;

		inline constexpr auto WallOrGateWorkPerPass = 10;

		inline constexpr auto Gates = 50;

		inline constexpr auto Walls = 1000;

		inline constexpr auto MainTasks = 50;
	}

	namespace HumanStatisticIndex
	{
		inline constexpr auto Entries = 10;

		inline constexpr auto ChildrenWithoutHome = 7;

		inline constexpr auto Children = 6;

		inline constexpr auto AdultFemaleWithoutHome = 5;

		inline constexpr auto AdultFemale = 4;

		inline constexpr auto Soldiers = 9;

		inline constexpr auto AdultMaleWithoutWorkHouse = 2;

		inline constexpr auto WithoutHome = 8;

		inline constexpr auto AdultMaleWithoutHome = 3;

		inline constexpr auto AdultMale = 1;

		inline constexpr auto All = 0;
	}

	namespace HouseBuildSearchCallbackComponent
	{
		inline constexpr auto Count = 4;

		inline constexpr auto ResultPosition = 3;
	}

	namespace SingleHumanWindowLimits
	{
		inline constexpr auto TraderRoutes = 2;

		inline constexpr auto ExperienceJobsPerPage = 14;

		inline constexpr auto ExperienceJobs = 78;

		inline constexpr auto ExperiencePages = 11;

		inline constexpr auto Tabs = 6;

		inline constexpr auto ExperienceJobsPerPageUnsigned = 14u;
	}

	namespace SelectionTooltipLimits
	{
		inline constexpr auto FormatTerminator = 99;

		inline constexpr auto FormatBufferBytes = 100;

		inline constexpr auto StoredTerminator = 100;

		inline constexpr auto CopiedTextBytes = 100;

		inline constexpr auto Entries = 100;
	}

	namespace GuiMouseTiming
	{
		inline constexpr auto MarkerFrameMillisecondsUnsigned = 150u;

		inline constexpr auto TooltipDelayMillisecondsUnsigned = 500u;
	}

	namespace GuiMouseTooltipLimits
	{
		inline constexpr auto MultilineTerminator = 399;

		inline constexpr auto MultilineBufferBytes = 400;
	}

	namespace GuiMouseWorldCursorLimits
	{
		inline constexpr auto MarkerFramesUnsigned = 6u;

		inline constexpr auto Markers = 10;
	}

	namespace MissionStartLimits
	{
		inline constexpr auto GoodsUnsigned = 66u;

		inline constexpr auto ChildAgeBaseUnsigned = 1440u;

		inline constexpr auto AgeRandomRangeUnsigned = 720u;

		inline constexpr auto Players = 20;
	}

	namespace HumanExperienceLimits
	{
		inline constexpr auto MaximumUnsigned = 10000u;
	}

	namespace GameControlLimits
	{
		inline constexpr auto Vehicles = 7;

		inline constexpr auto Houses = 55;

		inline constexpr auto GoodsAndEquipment = 66;

		inline constexpr auto Jobs = 56;

		inline constexpr auto Players = 20;

		inline constexpr auto RejectedPathfinderStartCountUnsigned = 2u;

		inline constexpr auto NameCopyCharacters = 50;
	}

	namespace GameControlTiming
	{
		inline constexpr auto PlayerDeathCheckIntervalUnsigned = 125u;

		inline constexpr auto PlayerDeathFirstCheckTickUnsigned = 720u;
	}

	namespace HumanAtomicAction
	{
		inline constexpr auto Attack = 81;
	}

	namespace HumanAttackLimits
	{
		inline constexpr auto PreviousDirectionOffset = 5;

		inline constexpr auto NeedsRequiredForRepeatAttack = 8000;

		inline constexpr auto WorkHouseDistanceUnsigned = 20u;

		inline constexpr auto ShortWorkHouseDistanceUnsigned = 10u;

		inline constexpr auto RangedWorkHouseMarginUnsigned = 5u;

		inline constexpr auto WorkCenterDistanceUnsigned = 40u;

		// Remaster: accept the supported attack radius instead of repeatedly rejecting targets beyond 18.
		inline constexpr auto WeaponModeWorkCenterDistanceUnsigned = 30u;

		inline constexpr auto PositionSearchRadius = 5;

		inline constexpr auto PositionCandidates = 6;

		inline constexpr auto TargetRecords = 2;

		inline constexpr auto VehicleDistanceNotFound = 100000;

		inline constexpr auto PointDistanceNotFound = 1000000;
	}

	namespace WeaponDamageSlot
	{
		inline constexpr auto Landscape = 7;
	}

	namespace HumanJobWindowTiming
	{
		inline constexpr auto CursorBlinkMillisecondsUnsigned = 200u;

		inline constexpr auto RefreshMillisecondsUnsigned = 2000u;
	}

	namespace HumanJobWindowLimits
	{
		inline constexpr auto CollectorGoodOptions = 6;

		inline constexpr auto Goods = 66;

		inline constexpr auto Jobs = 56;
	}

	namespace GuiVirtualTable
	{
		inline constexpr auto BaseWindowEntries = 19;
	}

	namespace CollectorGoodOptionField
	{
		inline constexpr auto Fields = 2;

		inline constexpr auto RequiresCurrentAvailability = 1;

		inline constexpr auto GoodId = 0;
	}

	namespace MultiPlayerGoalLimits
	{
		inline constexpr auto Goals = 20;

		inline constexpr auto Players = 20;

		inline constexpr auto GoodsRequirements = 8;
	}

	namespace MultiPlayerGoalTiming
	{
		inline constexpr auto CheckIntervalTicksUnsigned = 120u;
	}

	namespace MultiPlayerGoalCallback
	{
		inline constexpr auto Priority = 50;

		inline constexpr auto RegistrationTag = 0x6F;
	}

	namespace MultiPlayerGoalSavegame
	{
		inline constexpr auto Version = 1;
	}

	namespace FishIteratorMode
	{
		inline constexpr auto WithinRadiusUnsigned = 2u;

		inline constexpr auto PopulatedOnlyUnsigned = 1u;

		inline constexpr auto PopulatedOnly = 1;
	}

	namespace MapResourceLimits
	{
		inline constexpr auto AutoTransitionEntries = 100;

		inline constexpr auto RegrowEntries = 300;
	}

	namespace BioRegrowLimits
	{
		inline constexpr auto LandscapeSearchRadius = 5;

		inline constexpr auto RoadClearanceRadius = 2;

		inline constexpr auto MinimumMatchingLandscapes = 6;

		inline constexpr auto Candidates = 20;

		inline constexpr auto SearchRadius = 12;
	}

	namespace BioRegrowTiming
	{
		inline constexpr auto PartitionPercent = 5;

		inline constexpr auto PartitionPercentUnsigned = 5u;

		inline constexpr auto PartitionsPerAxisUnsigned = 20u;

		inline constexpr auto PartitionCountUnsigned = 400u;

		inline constexpr auto PartitionIntervalTicksUnsigned = 24u;

		inline constexpr auto MinimumDelayTicksUnsigned = 3600u;

		inline constexpr auto RandomDelayTicksUnsigned = 10800u;

		inline constexpr auto SkippedChanceUnsigned = 10u;

		inline constexpr auto ChanceDivisorUnsigned = 100u;
	}

	namespace FishSwarmLimits
	{
		inline constexpr auto ReproductionRadius = 16;

		inline constexpr auto FishPerSwarm = 30;

		inline constexpr auto FishPerSwarmUnsigned = 30u;

		inline constexpr auto CapacityUnsigned = 500u;

		inline constexpr auto Capacity = 500;

		inline constexpr auto DistanceNotFound = 1000;
	}

	namespace FishSwarmTiming
	{
		inline constexpr auto ReproductionIntervalTicksUnsigned = 2160u;
	}

	namespace FishSwarmCallback
	{
		inline constexpr auto RegistrationTag = 0x65;
	}

	namespace HouseStockLimits
	{
		inline constexpr auto BorrowRoundingBiasUnsigned = 9u;

		inline constexpr auto FractionsPerUnit = 10;

		inline constexpr auto FractionsPerUnitUnsigned = 10u;

		inline constexpr auto AmountCap = 1000;
	}

	namespace HouseConstructionLimits
	{
		inline constexpr auto IncomingAmountCapUnsigned = 254u;

		inline constexpr auto MinimumBuilders = 5;

		inline constexpr auto WorkStepsPerGoodUnsigned = 30u;

		inline constexpr auto DistinctGoods = 10;
	}

	namespace HouseOverlayLimits
	{
		inline constexpr auto TicksPerIntervalUnsigned = 12u;

		inline constexpr auto TickRoundingBiasUnsigned = 11u;

		inline constexpr auto ActionCount = 5;

		inline constexpr auto LandscapeCount = 10;
	}

	namespace ConstructionSelectionLimits
	{
		inline constexpr auto Pages = 5;

		inline constexpr auto RefreshIntervalMillisecondsUnsigned = 2000u;

		inline constexpr auto CachedHouses = 100;
	}

	namespace ConstructionSelectionDimensions
	{
		inline constexpr auto Height = 420;

		inline constexpr auto Width = 500;

		inline constexpr auto HeightUnsigned = 420u;

		inline constexpr auto WidthUnsigned = 500u;
	}

}
