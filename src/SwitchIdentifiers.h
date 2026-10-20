#pragma once

// Named selectors retain the original Windows values and literal types.
// These constants do not change stored object fields, packet layouts or switch behavior.
namespace SwitchIds
{
	namespace AICondition
	{
		inline constexpr auto PeriodicUnsigned = 12u;

		inline constexpr auto ActivatedFlagUnsigned = 11u;

		inline constexpr auto SoldierCountUnsigned = 10u;

		inline constexpr auto PlayerHasNoHumansUnsigned = 9u;

		inline constexpr auto ExternalFlagUnsigned = 8u;

		inline constexpr auto HouseNearPositionUnsigned = 7u;

		inline constexpr auto MapMoveableNearPositionUnsigned = 6u;

		inline constexpr auto DiplomacyUnsigned = 5u;

		inline constexpr auto TimeSinceConditionChangeUnsigned = 4u;

		inline constexpr auto LogicalCombinationUnsigned = 3u;

		inline constexpr auto GameTimeUnsigned = 2u;

		inline constexpr auto TrueUnsigned = 1u;

		inline constexpr auto Disabled = 13;

		inline constexpr auto True = 1;
		inline constexpr auto GameTime = 2;
		inline constexpr auto LogicalCombination = 3;
		inline constexpr auto TimeSinceConditionChange = 4;
		inline constexpr auto Diplomacy = 5;
		inline constexpr auto MapMoveableNearPosition = 6;
		inline constexpr auto HouseNearPosition = 7;
		inline constexpr auto ExternalFlag = 8;
		inline constexpr auto PlayerHasNoHumans = 9;
		inline constexpr auto SoldierCount = 10;
		inline constexpr auto ActivatedFlag = 11;
		inline constexpr auto Periodic = 12;
	}

	namespace AIGoodAction
	{
		inline constexpr auto ImportAndBring = 1;
		inline constexpr auto Bring = 2;
		inline constexpr auto StopProduction = 3;
		inline constexpr auto StartProduction = 4;
		inline constexpr auto StopBringing = 5;
	}

	namespace AIToken
	{
		inline constexpr auto AIMainTaskAttack = 0x61696174;
		inline constexpr auto AIMainTaskBuildHouse = 0x61696268;
		inline constexpr auto AISetConditionOnConditions = 0x61696363;
		inline constexpr auto AISetConditionOnDiplomacyChange = 0x61696364;
		inline constexpr auto AISetConditionOnExternal = 0x61696365;
		inline constexpr auto AISetConditionOnHouseInRange = 0x61696368;
		inline constexpr auto AISetConditionTrue = 0x6169636F;
		inline constexpr auto AISetConditionOnPlayerDead = 0x61696370;
		inline constexpr auto AISetConditionOnCreatureInRange = 0x61696372;
		inline constexpr auto AISetConditionOnPlayerSeen = 0x61696373;
		inline constexpr auto AISetConditionOnTime = 0x61696374;
		inline constexpr auto AIMainTaskCreateCreatures = 0x61696378;
		inline constexpr auto AIMainTaskChangeDiplomacy = 0x61696379;
		inline constexpr auto AISetConditionOnConditionChangeDelayed = 0x6169637A;
		inline constexpr auto AIMainTaskDefend = 0x61696465;
		inline constexpr auto AIMainTaskSelfDestroyPlayer = 0x61696470;
		inline constexpr auto AIDisable = 0x61696473;
		inline constexpr auto AIMaxUnitLimit = 0x61696D75;
		inline constexpr auto AISetConditionOnNumberOfSoldiers = 0x61696E73;
		inline constexpr auto AISoldiersDefaultPosition = 0x61697364;
		inline constexpr auto AIUnitLimit = 0x6169756C;
		inline constexpr auto AISetConditionOnTimer = 0x61736374;
		inline constexpr auto HAIDisable = 0x68616964;
		inline constexpr auto HAIDisableCollectResources = 0x68646372;
		inline constexpr auto HAIDisableGuideBuild = 0x68646762;
		inline constexpr auto HAIDisableHouseBuild = 0x68646862;
		inline constexpr auto HAIDisableHomeExpansion = 0x68646865;
		inline constexpr auto HAIDisableHouseUpgrade = 0x68646875;
		inline constexpr auto HAIDisableMilitary = 0x68646D69;
		inline constexpr auto HAIDisableRoadBuild = 0x68647262;
	}

	namespace ActionButtonArc
	{
		inline constexpr auto Bottom = 0;
		inline constexpr auto Top = 1;
		inline constexpr auto Right = 2;
		inline constexpr auto Left = 3;
		inline constexpr auto InnerLeft = 4;
	}

	namespace AmbientSoundToken
	{
		inline constexpr auto LandscapeGroup = 0x6C6E6467u;
		inline constexpr auto Name = 0x6E616D65u;
		inline constexpr auto PatternGroup = 0x70677270u;
		inline constexpr auto SFX = 0x73667864u;
	}

	namespace AnimalSoundToken
	{
		inline constexpr auto EngineSoundGroup = 0x656E6773u;
		inline constexpr auto LogicTribeType = 0x6C747274u;
		inline constexpr auto MinCount = 0x6D696E63u;
		inline constexpr auto Probability = 0x70726F62u;
	}

	namespace AnimalTask
	{
		inline constexpr auto Idle = 1;
		inline constexpr auto IdleAlternate = 2;
		inline constexpr auto FollowLeader = 3;
		inline constexpr auto RunAway = 4;
		inline constexpr auto Stay = 5;
		inline constexpr auto GotoNextPoint = 6;
		inline constexpr auto Eat = 7;
		inline constexpr auto Attack = 8;
		inline constexpr auto InteractWithVehicle = 9;
		inline constexpr auto InteractWithHouse = 10;
	}

	namespace AnimalTypeToken
	{
		inline constexpr auto Aggressive = 0x61676772u;
		inline constexpr auto AngryGameTime = 0x6174696Du;
		inline constexpr auto Catchable = 0x63616266u;
		inline constexpr auto Cannotbeattacked = 0x636E6261u;
		inline constexpr auto GetAngry = 0x67616E67u;
		inline constexpr auto HitpointsAdult = 0x68697461u;
		inline constexpr auto HitpointsBABY = 0x68697462u;
		inline constexpr auto IgnoreHouses = 0x6967686Fu;
		inline constexpr auto MoveSpeed = 0x6D737064u;
		inline constexpr auto Maximumcadaversize = 0x6D786373u;
		inline constexpr auto MaximumDistanceToBirthPoint = 0x6D786462u;
		inline constexpr auto MaximumDistanceToStayPoint = 0x6D786473u;
		inline constexpr auto MaximumGroupSize = 0x6D786773u;
		inline constexpr auto MaximumLeaderDistance = 0x6D786C64u;
		inline constexpr auto Runspeed = 0x72756E73u;
		inline constexpr auto SearchForLeader = 0x736C6561u;
		inline constexpr auto Warrantable = 0x77617272u;
	}

	namespace AnimalVehicleJobToken
	{
		inline constexpr auto Gfxbobmanagerbody = 0x67666262;
		inline constexpr auto GfxPaletteBody = 0x67667062;
		inline constexpr auto LogicJob = 0x6C6F6A6F;
		inline constexpr auto LogicTribe = 0x6C6F7472;
		inline constexpr auto LogicVehicle = 0x6C6F7665;
	}

	namespace AnimationSequenceToken
	{
		inline constexpr auto Sequence = 0x736571;
		inline constexpr auto Imagelib = 0x696C6962;
		inline constexpr auto Shadowlib = 0x736C6962;
	}

	namespace ArmorTypeToken
	{
		inline constexpr auto BlockingValue = 0x626C766Cu;
		inline constexpr auto GoodType = 0x676F6F64u;
		inline constexpr auto Maintype = 0x6D61696Eu;
		inline constexpr auto MaterialType = 0x6D617479u;
		inline constexpr auto Name = 0x6E616D65u;
		inline constexpr auto Weight = 0x77656967u;
	}

	namespace AtomicAnimationEvent
	{
		inline constexpr auto ModifyStamina = 1;
		inline constexpr auto ModifyFood = 2;
		inline constexpr auto ModifySocial = 3;
		inline constexpr auto ModifyReligion = 4;
		inline constexpr auto ModifyHitpoints = 5;
		inline constexpr auto StopGoodCirculationUnsupported = 6;
		inline constexpr auto StopGoodCirculationUnsupportedAlias = 7;
		inline constexpr auto SendSocial = 8;
		inline constexpr auto SendReligion = 9;
		inline constexpr auto SendHitpoints = 10;
		inline constexpr auto Pickup = 11;
		inline constexpr auto Pileup = 12;
		inline constexpr auto SplitUp = 13;
		inline constexpr auto Merge = 14;
		inline constexpr auto Transform = 15;
		inline constexpr auto Plant = 16;
		inline constexpr auto Shrink = 17;
		inline constexpr auto Grow = 18;
		inline constexpr auto CatchFish = 19;
		inline constexpr auto Build = 20;
		inline constexpr auto Produce = 21;
		inline constexpr auto LeaveHouse = 22;
		inline constexpr auto EnterHouse = 23;
		inline constexpr auto GiveBirth = 24;
		inline constexpr auto Attack = 25;
		inline constexpr auto ModifyWorkHouseStock = 27;
		inline constexpr auto ChangeGold = 30;
		inline constexpr auto ChangeWealth = 31;
		inline constexpr auto DidProduceOneGood = 32;
		inline constexpr auto OpenChest = 33;
		inline constexpr auto PlaySoundFX = 34;
		inline constexpr auto ChangeMiscPalette = 35;
		inline constexpr auto HouseParticleEffect = 36;
	}

	namespace AtomicAnimationToken
	{
		inline constexpr auto EndDirection = 0x656E6464;
		inline constexpr auto Event = 0x6576656E;
		inline constexpr auto EventX = 0x65766578;
		inline constexpr auto Interruptable = 0x696E7465;
		inline constexpr auto Length = 0x6C656E67;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto StartDirection = 0x73746164;
	}

	namespace BitmapDepth
	{
		inline constexpr auto Indexed8 = 0;
		inline constexpr auto HighColor16 = 1;
		inline constexpr auto TrueColor32 = 2;
	}

	namespace BmpRleEscape
	{
		inline constexpr auto EndOfLine = 0;
		inline constexpr auto EndOfBitmap = 1;
		inline constexpr auto Delta = 2;
	}

	namespace BobShadowMode
	{
		inline constexpr auto Darken = 0;
		inline constexpr auto Grayscale = 1;
		inline constexpr auto HalfColorBrighten = 2;
		inline constexpr auto SaturatingBrighten = 3;
	}

	namespace CallbackType
	{
		inline constexpr auto MapPointChangedUnsigned = 0x33u;

		inline constexpr auto EndMapChangesUnsigned = 0x32u;

		inline constexpr auto BeginMapChangesUnsigned = 0x31u;

		inline constexpr auto GameTickUnsigned = 1u;

		inline constexpr auto PlayerWonOrLost = 0x30;

		inline constexpr auto PlayerDied = 0x2A;

		inline constexpr auto GameTick = 1;
		inline constexpr auto MapLifetime = 2;
		inline constexpr auto HumanManagerLifetime = 3;
		inline constexpr auto HumanLifetime = 4;
		inline constexpr auto AnimalManagerLifetime = 5;
		inline constexpr auto AnimalLifetime = 6;
		inline constexpr auto VehicleManagerLifetime = 7;
		inline constexpr auto VehicleLifetime = 8;
		inline constexpr auto HouseManagerLifetime = 9;
		inline constexpr auto HouseLifetime = 10;
		inline constexpr auto GuideLifetime = 12;
		inline constexpr auto MapAreaChanged = 0x0D;
		inline constexpr auto HumanDisplayStateChanged = 0xE;
		inline constexpr auto AnimalDisplayStateChanged = 0xF;
		inline constexpr auto VehicleDisplayStateChanged = 0x10;
		inline constexpr auto HouseDisplayStateChanged = 0x11;
		inline constexpr auto SaveCleanmap = 0x12;
		inline constexpr auto LoadCleanmap = 0x13;
		inline constexpr auto SaveSavegame = 0x14;
		inline constexpr auto LoadSavegame = 0x15;
		inline constexpr auto LoadDone = 0x16;
		inline constexpr auto PlayPlayerJingle = 0x18;
		inline constexpr auto UpdateHouseOverlayParticles = 0x1C;
		inline constexpr auto HouseWalkAreaChanged = 0x1D;
		inline constexpr auto ResetHouseDynamicGraphics = 0x1E;
		inline constexpr auto HumanJobChanged = 0x1F;
		inline constexpr auto HumanArmorChanged = 0x20;
		inline constexpr auto HumanPaletteChanged = 0x21;
		inline constexpr auto HumanMiscPaletteChanged = 0x22;
		inline constexpr auto CreateMunitionParticle = 0x23;
		inline constexpr auto CreateFallingSmokeParticle = 0x24;
		inline constexpr auto StartParticleEffect = 0x25;
		inline constexpr auto StartDirectionalParticleEffect = 0x26;
		inline constexpr auto StartDirectionalParticleEffectFromRecord = 0x27;
		inline constexpr auto PlayerAttackedByPlayer = 0x28;
		inline constexpr auto BeginMapChanges = 0x31;
		inline constexpr auto EndMapChanges = 0x32;
		inline constexpr auto MapPointChanged = 0x33;
	}

	namespace CampaignMenuToken
	{
		inline constexpr auto Playerfixcolors = 0x7066636Fu;
		inline constexpr auto Playerhideinmenu = 0x7068696Du;
		inline constexpr auto Playeroption = 0x706C6F70u;
	}

	namespace ChestReward
	{
		inline constexpr auto Food = 20;
		inline constexpr auto LargeFoodPotions = 21;
		inline constexpr auto LargeStaminaPotions = 22;
		inline constexpr auto LargeHealingPotions = 23;
		inline constexpr auto ChainArmor = 24;
		inline constexpr auto PlateArmor = 25;
		inline constexpr auto Shoes = 26;
		inline constexpr auto LetterOfIndulgence = 50;
		inline constexpr auto PlaceHouse = 51;
		inline constexpr auto PlaceWatchtower = 52;
		inline constexpr auto PlaceTemple = 53;
		inline constexpr auto PlaceBarracks = 54;
		inline constexpr auto PlaceStockhouse = 55;
		inline constexpr auto PlaceSmallHome = 56;
		inline constexpr auto PlaceLargeHome = 57;
		inline constexpr auto PlaceWell = 60;
		inline constexpr auto PlaceFarm = 61;
		inline constexpr auto PlaceMill = 62;
		inline constexpr auto PlaceBakery = 63;
		inline constexpr auto PlaceHeadquarters = 64;
		inline constexpr auto PlaceSchool = 65;
		inline constexpr auto SmithyAndSmith = 70;
		inline constexpr auto AlchemyHutAndDruid = 71;
		inline constexpr auto MintAndCoinMaker = 72;
		inline constexpr auto AdvancedAlchemyHutAndDruid = 73;
		inline constexpr auto WeaponsHutAndArmorer = 74;
		inline constexpr auto Catapult = 91;
		inline constexpr auto Wolves = 90;
		inline constexpr auto Civilians = 92;
		inline constexpr auto Women = 93;
		inline constexpr auto Swordsmen = 94;
		inline constexpr auto Archers = 95;
		inline constexpr auto Lions = 96;
	}

	namespace CleanMapToken
	{
		inline constexpr auto Mapcampaignid = 0x6D616369;
		inline constexpr auto MapType = 0x6D617074;
		inline constexpr auto Mapmultiplayeronly = 0x6D6D6F6E;
	}

	namespace CodePageId
	{
		inline constexpr auto JapaneseShiftJis = 932;
		inline constexpr auto SimplifiedChinese = 936;
		inline constexpr auto Korean = 949;
		inline constexpr auto TraditionalChinese = 950;
	}

	namespace CodePageSelector
	{
		inline constexpr auto Current = -4;
		inline constexpr auto Ansi = -3;
		inline constexpr auto Oem = -2;
	}

	namespace DamageTargetType
	{
		inline constexpr auto Human = 1;
		inline constexpr auto Vehicle = 2;
		inline constexpr auto Animal = 3;
		inline constexpr auto House = 4;
		inline constexpr auto Landscape = 5;
	}

	namespace DecimalParseState
	{
		inline constexpr auto Start = 0;
		inline constexpr auto LeadingIntegerZeros = 1;
		inline constexpr auto AfterSign = 2;
		inline constexpr auto IntegerDigits = 3;
		inline constexpr auto FractionDigits = 4;
		inline constexpr auto LeadingDecimalPoint = 5;
		inline constexpr auto ExponentMarker = 6;
		inline constexpr auto ExponentDigitsStart = 7;
		inline constexpr auto ExponentZeros = 8;
		inline constexpr auto ExponentDigits = 9;
		inline constexpr auto BareExponentSign = 11;
	}

	namespace DirectoryScanMode
	{
		inline constexpr auto FileSystemFirst = 1;
		inline constexpr auto FileSystemNext = 2;
		inline constexpr auto LibraryFirst = 3;
		inline constexpr auto LibraryNext = 4;
	}

	namespace EngineMapChunk
	{
		inline constexpr auto LandscapeNames = 0x65616C64;
		inline constexpr auto PatternNames = 0x65617064;
		inline constexpr auto TransitionNames = 0x65617464;
		inline constexpr auto MacroBrightness = 0x656D6272;
		inline constexpr auto Landscapes = 0x656D6C61;
		inline constexpr auto MacroFlags = 0x656D6D31;
		inline constexpr auto MicroFlags = 0x656D6D69;
		inline constexpr auto PatternA = 0x656D7061;
		inline constexpr auto PatternB = 0x656D7062;
		inline constexpr auto TransitionFirst = 0x656D7431;
		inline constexpr auto TransitionSecond = 0x656D7432;
		inline constexpr auto TransitionThird = 0x656D7433;
		inline constexpr auto TransitionFourth = 0x656D7434;
		inline constexpr auto MacroVertexColor = 0x656D7663;
	}

	namespace EngineSavegameChunk
	{
		inline constexpr auto DynamicAnimals = 0x4444616E;
		inline constexpr auto DynamicHumans = 0x4444686D;
		inline constexpr auto DynamicHouses = 0x4444686F;
		inline constexpr auto DynamicVehicles = 0x44447665;
		inline constexpr auto WorldMusic = 0x4D754D61;
		inline constexpr auto Particles = 0x50614D61;
		inline constexpr auto Weather = 0x57684D61;
		inline constexpr auto EngineMap = 0x58456D61;
	}

	namespace FileAccessMode
	{
		inline constexpr auto ReadOnly = 0;
		inline constexpr auto WriteOnly = 1;
		inline constexpr auto ReadWrite = 2;
	}

	namespace FileCreationFlags
	{
		inline constexpr auto Existing = 0x000;
		inline constexpr auto Create = 0x100;
		inline constexpr auto Truncate = 0x200;
		inline constexpr auto CreateTruncate = 0x300;
		inline constexpr auto ExclusiveWithoutCreate = 0x400;
		inline constexpr auto CreateExclusive = 0x500;
		inline constexpr auto TruncateExclusive = 0x600;
		inline constexpr auto CreateTruncateExclusive = 0x700;
	}

	namespace FileSharingMode
	{
		inline constexpr auto Exclusive = 0x10;
		inline constexpr auto DenyWrite = 0x20;
		inline constexpr auto DenyRead = 0x30;
		inline constexpr auto DenyNone = 0x40;
	}

	namespace FileTransferState
	{
		inline constexpr auto Announce = 0;
		inline constexpr auto Completed = 1;
		inline constexpr auto Aborted = 4;
		inline constexpr auto AwaitingAccept = 5;
		inline constexpr auto OpenSource = 6;
		inline constexpr auto AwaitingHeaderRequest = 7;
		inline constexpr auto RequestHeader = 8;
		inline constexpr auto AwaitingHeader = 10;
		inline constexpr auto ReceivingBlocks = 12;
		inline constexpr auto SendingBlocks = 13;
		inline constexpr auto Invalid = 14;
	}

	namespace GameAssistantCommand
	{
		inline constexpr auto RequestBoys = 10;
		inline constexpr auto RequestGirls = 11;
		inline constexpr auto RequestUnarmedSoldiers = 20;
		inline constexpr auto RequestSpearmen = 21;
		inline constexpr auto RequestArchers = 22;
		inline constexpr auto RequestSwordsmen = 23;
		inline constexpr auto EquipCivilianShoes = 30;
		inline constexpr auto EquipCivilianWoodenTools = 31;
		inline constexpr auto EquipCivilianIronTools = 32;
		inline constexpr auto EquipCivilianMead = 33;
		inline constexpr auto EquipSoldierShoes = 40;
		inline constexpr auto EquipSoldierMead = 41;
	}

	namespace GlobalGameSettingsCommand
	{
		inline constexpr auto DecreaseRequestBoys = 10300;
		inline constexpr auto IncreaseRequestBoys = 10301;
		inline constexpr auto DecreaseRequestGirls = 10310;
		inline constexpr auto IncreaseRequestGirls = 10311;
		inline constexpr auto DecreaseRequestUnarmedSoldiers = 10320;
		inline constexpr auto IncreaseRequestUnarmedSoldiers = 10321;
		inline constexpr auto DecreaseRequestSpearmen = 10322;
		inline constexpr auto IncreaseRequestSpearmen = 10323;
		inline constexpr auto DecreaseRequestArchers = 10324;
		inline constexpr auto IncreaseRequestArchers = 10325;
		inline constexpr auto DecreaseRequestSwordsmen = 10326;
		inline constexpr auto IncreaseRequestSwordsmen = 10327;
		inline constexpr auto DisableEquipCivilianShoes = 10340;
		inline constexpr auto EnableEquipCivilianShoes = 10341;
		inline constexpr auto DisableEquipCivilianWoodenTools = 10350;
		inline constexpr auto EnableEquipCivilianWoodenTools = 10351;
		inline constexpr auto DisableEquipCivilianIronTools = 10360;
		inline constexpr auto EnableEquipCivilianIronTools = 10361;
		inline constexpr auto DisableEquipCivilianMead = 10370;
		inline constexpr auto EnableEquipCivilianMead = 10371;
		inline constexpr auto DisableEquipSoldierShoes = 10440;
		inline constexpr auto EnableEquipSoldierShoes = 10441;
		inline constexpr auto DisableEquipSoldierMead = 10470;
		inline constexpr auto EnableEquipSoldierMead = 10471;
	}

	namespace GoodCirculationMode
	{
		inline constexpr auto None = 0;
		inline constexpr auto FetchForSelf = 1;
		inline constexpr auto FillStockFetch = 2;
		inline constexpr auto FetchForHome = 3;
		inline constexpr auto FillStockBringAway = 4;
		inline constexpr auto FlushStock = 5;
	}

	namespace GoodEquipmentCategory
	{
		inline constexpr auto Weapon = 1;
		inline constexpr auto Armor = 2;
		inline constexpr auto Tool = 3;
		inline constexpr auto Shoes = 4;
		inline constexpr auto Item = 5;
	}

	namespace GoodToken
	{
		inline constexpr auto AtomicForCultivating = 0x6163756C;
		inline constexpr auto AtomicForHarvesting = 0x61686172;
		inline constexpr auto AtomicForPlanting = 0x61706C61;
		inline constexpr auto AtomicForProduction = 0x6170726F;
		inline constexpr auto ArmorType = 0x61726D6F;
		inline constexpr auto IsBioLandscapeFlag = 0x62696F6C;
		inline constexpr auto ProductionInputGoods = 0x696E676F;
		inline constexpr auto LandscapeType = 0x6C616E64;
		inline constexpr auto LandscapeToHarvest = 0x6C686172;
		inline constexpr auto LandscapeToPickup = 0x6C706963;
		inline constexpr auto LandscapeToStore = 0x6C73746F;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto IsProducedInHouseFlag = 0x70686F75;
		inline constexpr auto IsInputGoodFlag = 0x70696E70;
		inline constexpr auto IsProducedOnMapFlag = 0x706D6170;
		inline constexpr auto WeaponType = 0x77656170;
	}

	namespace GoodType
	{
		inline constexpr auto Wheat = 4;
		inline constexpr auto Herb = 13;
		inline constexpr auto Mushroom = 14;
		inline constexpr auto Mead = 43;
		inline constexpr auto PotionFoodSmall = 44;
		inline constexpr auto PotionFoodBig = 45;
		inline constexpr auto PotionStaminaSmall = 46;
		inline constexpr auto PotionStaminaBig = 47;
		inline constexpr auto PotionHealSmall = 48;
		inline constexpr auto PotionHealBig = 49;
		inline constexpr auto AmuletFood = 50;
		inline constexpr auto AmuletStamina = 51;
	}

	namespace GridRowAlignment
	{
		inline constexpr auto AlignDownEven = 0;
		inline constexpr auto AlignUpEven = 1;
		inline constexpr auto AlignDownOdd = 2;
		inline constexpr auto AlignUpOdd = 3;
	}

	namespace GuiSavegameChunk
	{
		inline constexpr auto DeadHumanData = 0x5864686D;
		inline constexpr auto DiplomacyWindow = 0x58646970;
		inline constexpr auto DisplayPosition = 0x58647372;
		inline constexpr auto HumanListWindow = 0x5868756C;
		inline constexpr auto MissionBriefingHistory = 0x586D6268;
		inline constexpr auto MessagePriority = 0x586D6C65;
		inline constexpr auto Messages = 0x586D6D61;
		inline constexpr auto ObjectGroups = 0x586F676D;
		inline constexpr auto StatisticsWindow = 0x58737461;
	}

	namespace HelpWindowCommand
	{
		inline constexpr auto SelectHouse = 10011;
		inline constexpr auto ClearHouseSelection = 10012;
		inline constexpr auto PreviousHouse = 10013;
		inline constexpr auto NextHouse = 10014;
		inline constexpr auto ShowGoodForHouse = 10015;
		inline constexpr auto SelectGood = 10016;
		inline constexpr auto ClearGoodSelection = 10017;
		inline constexpr auto PreviousGood = 10018;
		inline constexpr auto NextGood = 10019;
		inline constexpr auto SelectMisc = 10020;
		inline constexpr auto ClearMiscSelection = 10021;
		inline constexpr auto PreviousMisc = 10022;
		inline constexpr auto NextMisc = 10023;
		inline constexpr auto OpenHouseInfo = 20000;
		inline constexpr auto OpenGoodInfo = 20001;
	}

	namespace HexagonDirection
	{
		inline constexpr auto East = 0;
		inline constexpr auto SouthEast = 1;
		inline constexpr auto SouthWest = 2;
		inline constexpr auto West = 3;
		inline constexpr auto NorthWest = 4;
		inline constexpr auto NorthEast = 5;
		inline constexpr auto North = 6;
		inline constexpr auto South = 7;

		inline constexpr auto EastUnsigned = 0u;
		inline constexpr auto SouthEastUnsigned = 1u;
		inline constexpr auto SouthWestUnsigned = 2u;
		inline constexpr auto WestUnsigned = 3u;
		inline constexpr auto NorthWestUnsigned = 4u;
		inline constexpr auto NorthEastUnsigned = 5u;
	}

	namespace HouseAttachmentMask
	{
		inline constexpr auto BuilderUnsigned = 4u;

		inline constexpr auto WorkerUnsigned = 2u;

		inline constexpr auto HomeUnsigned = 1u;

		inline constexpr auto Home = 1;
		inline constexpr auto Worker = 2;
		inline constexpr auto Builder = 4;
		inline constexpr auto Student = 8;
	}

	namespace HouseGraphicsToken
	{
		inline constexpr auto ArrowSlotData = 0x61736461u;
		inline constexpr auto EditGroups = 0x65646772u;
		inline constexpr auto EditName = 0x65646E61u;
		inline constexpr auto Gfxboblibs = 0x67667862u;
		inline constexpr auto Gfxbobconstructionlayer = 0x67667863u;
		inline constexpr auto Gfxdoorbobid = 0x67667864u;
		inline constexpr auto Gfxbobid = 0x67667866u;
		inline constexpr auto GfxOverlayLandscape = 0x6766786Cu;
		inline constexpr auto GfxOverlay = 0x6766786Fu;
		inline constexpr auto GfxPalette = 0x67667870u;
		inline constexpr auto GfxShadingFactor = 0x67667875u;
		inline constexpr auto GfxFirePoint = 0x67786669u;
		inline constexpr auto GfxFlagPoint = 0x6778666Cu;
		inline constexpr auto Gfxsoldierflagpoint = 0x67786673u;
		inline constexpr auto GfxHolyFirePoint = 0x67786866u;
		inline constexpr auto GfxSmokePoint = 0x6778736Du;
		inline constexpr auto LogicHitpoints = 0x6C686974u;
		inline constexpr auto LogicAdditionalAttachPointArea = 0x6C6F6161u;
		inline constexpr auto LogicBuildBlockArea = 0x6C6F6262u;
		inline constexpr auto Logicconstructiongoods = 0x6C6F6367u;
		inline constexpr auto Logicdoorpoint = 0x6C6F6470u;
		inline constexpr auto LogicMaximumSize = 0x6C6F6D73u;
		inline constexpr auto LogicTribeType = 0x6C6F7474u;
		inline constexpr auto Logictype = 0x6C6F7479u;
		inline constexpr auto Logicconstructionworkarea = 0x6C6F7761u;
		inline constexpr auto LogicWalkBlockArea = 0x6C6F7762u;
	}

	namespace HouseTypeToken
	{
		inline constexpr auto DebugColor = 0x64626763;
		inline constexpr auto Debugname = 0x6462676E;
		inline constexpr auto Logicbuildonbiopattern = 0x6C626970;
		inline constexpr auto Logiccanenabledefencemode = 0x6C65646D;
		inline constexpr auto LogicHomeSize = 0x6C687369;
		inline constexpr auto Logicignorecontinentsflag = 0x6C696366;
		inline constexpr auto Logicmaintype = 0x6C6F6D74;
		inline constexpr auto LogicProduction = 0x6C70726F;
		inline constexpr auto LogicSchoolSize = 0x6C737369;
		inline constexpr auto LogicStock = 0x6C73746F;
		inline constexpr auto LogicVehicleType = 0x6C767479;
		inline constexpr auto LogicWorker = 0x6C776F72;
	}

	namespace HouseWindowCommand
	{
		inline constexpr auto Update = 2030;
		inline constexpr auto ToggleSubTab = 10000;
		inline constexpr auto StartUpgrade = 10001;
		inline constexpr auto StopUpgrade = 10002;
		inline constexpr auto ConfirmRemove = 10003;
		inline constexpr auto CenterHouse = 10004;
		inline constexpr auto OpenWorkerList = 10005;
		inline constexpr auto OpenHouseHelp = 10006;
		inline constexpr auto SelectHuman = 10007;
		inline constexpr auto SelectHumanAndShowActions = 10008;
		inline constexpr auto DecreaseProductionCounter = 10009;
		inline constexpr auto IncreaseProductionCounter = 10010;
		inline constexpr auto StopProduction = 10011;
		inline constexpr auto ResumeProduction = 10012;
		inline constexpr auto SelectStockView = 10013;
		inline constexpr auto SelectStockGood = 10014;
		inline constexpr auto DecreaseStockMinimum = 10015;
		inline constexpr auto IncreaseStockMinimum = 10016;
		inline constexpr auto DisableFoodEnhancer = 10017;
		inline constexpr auto EnableFoodEnhancer = 10018;
		inline constexpr auto DisableSocialEnhancer = 10019;
		inline constexpr auto EnableSocialEnhancer = 10020;
		inline constexpr auto DisableReligionEnhancer = 10021;
		inline constexpr auto EnableReligionEnhancer = 10022;
		inline constexpr auto CenterHouseAndStopFollowing = 10023;
		inline constexpr auto ShowActionButtons = 10024;
		inline constexpr auto EnableDefence = 10025;
		inline constexpr auto DisableDefence = 10026;
		inline constexpr auto SelectSoldiers = 10027;
	}

	namespace HumanAnimationToken
	{
		inline constexpr auto Gfxinhouseanim = 0x67666861;
		inline constexpr auto GfxInHouseOverlayLandscape = 0x6766686C;
		inline constexpr auto Gfxinhouseoverlaybob = 0x6766686F;
		inline constexpr auto GfxInHouseWalk = 0x67666877;
		inline constexpr auto Gfxbobseqbody = 0x67667862;
		inline constexpr auto Gfxanimframelistdir = 0x67667864;
		inline constexpr auto GfxAnimFrameList = 0x67667866;
		inline constexpr auto Gfxbobseqhead = 0x67667868;
		inline constexpr auto Gfxanimmode = 0x6766786D;
		inline constexpr auto LogicAtomicAction = 0x6C6F6174;
		inline constexpr auto LogicInHouseAtomicSubId = 0x6C6F6973;
		inline constexpr auto LogicJob = 0x6C6F6A6F;
		inline constexpr auto LogicTribe = 0x6C6F7472;
	}

	namespace HumanGuiAction
	{
		inline constexpr auto GotoAndWait = 2;
		inline constexpr auto Eat = 3;
		inline constexpr auto Sleep = 4;
		inline constexpr auto Talk = 5;
		inline constexpr auto Pray = 6;
		inline constexpr auto Marry = 7;
		inline constexpr auto GetBoy = 8;
		inline constexpr auto GetGirl = 9;
		inline constexpr auto AttachHomeHouse = 10;
		inline constexpr auto DetachHomeHouse = 11;
		inline constexpr auto AttachVehicle = 12;
		inline constexpr auto DetachVehicle = 13;
		inline constexpr auto AttachWorkHouse = 16;
		inline constexpr auto DetachWorkHouse = 17;
		inline constexpr auto AttachLearnHouse = 18;
		inline constexpr auto DetachLearnHouse = 19;
		inline constexpr auto AttachBuildHouse = 20;
		inline constexpr auto DetachBuildHouse = 21;
		inline constexpr auto AttachTradeHouse = 22;
		inline constexpr auto DetachTradeHouse = 23;
		inline constexpr auto ChangeJob = 24;
		inline constexpr auto ChangeProducedGood = 25;
		inline constexpr auto ChangeEquippedGoods = 26;
		inline constexpr auto LearnJob = 27;
		inline constexpr auto SetWorkCenter = 28;
		inline constexpr auto SetWorkArea = 29;
		inline constexpr auto ExploreArea = 31;
		inline constexpr auto ExploreAreaAndReset = 32;
		inline constexpr auto MilitaryAggressive = 35;
		inline constexpr auto MilitaryDefensive = 36;
		inline constexpr auto MilitaryIgnore = 37;
		inline constexpr auto MilitaryFlee = 38;
		inline constexpr auto AggressionOn = 39;
		inline constexpr auto AggressionOff = 40;
		inline constexpr auto AttackHuman = 41;
		inline constexpr auto AttackHouse = 42;
		inline constexpr auto AttackAnimal = 43;
		inline constexpr auto AttackVehicle = 44;
		inline constexpr auto AttackMapPosition = 45;
		inline constexpr auto GoAway = 48;
		inline constexpr auto GoHome = 49;
		inline constexpr auto Cheer = 54;
		inline constexpr auto Goodie = 55;
	}

	namespace HumanJob
	{
		inline constexpr auto BabyFemale = 1;
		inline constexpr auto BabyMale = 2;
		inline constexpr auto ChildFemale = 3;
		inline constexpr auto ChildMale = 4;
		inline constexpr auto Woman = 5;
		inline constexpr auto Civilist = 6;
		inline constexpr auto Builder = 7;
		inline constexpr auto Collector = 8;
		inline constexpr auto Joiner = 9;
		inline constexpr auto Armorer = 10;
		inline constexpr auto Potter = 11;
		inline constexpr auto Mason = 12;
		inline constexpr auto Smith = 13;
		inline constexpr auto CoinMaker = 14;
		inline constexpr auto Hunter = 15;
		inline constexpr auto Breeder = 16;
		inline constexpr auto Sewer = 17;
		inline constexpr auto Farmer = 18;
		inline constexpr auto Miller = 19;
		inline constexpr auto Baker = 20;
		inline constexpr auto Brewer = 21;
		inline constexpr auto Fisher = 22;
		inline constexpr auto FisherSea = 23;
		inline constexpr auto Carrier = 24;
		inline constexpr auto Trader = 25;
		inline constexpr auto TraderSea = 26;
		inline constexpr auto Scout = 27;
		inline constexpr auto Jester = 28;
		inline constexpr auto HerbMushGuy = 29;
		inline constexpr auto Druid = 30;
		inline constexpr auto SoldierUnarmed = 31;
		inline constexpr auto SoldierSpearWooden = 32;
		inline constexpr auto SoldierSpearIron = 33;
		inline constexpr auto SoldierSwordShort = 34;
		inline constexpr auto SoldierSwordLong = 35;
		inline constexpr auto SoldierSaberShort = 36;
		inline constexpr auto SoldierSaberLong = 37;
		inline constexpr auto SoldierAxeSmall = 38;
		inline constexpr auto SoldierAxeBig = 39;
		inline constexpr auto SoldierBowShort = 40;
		inline constexpr auto SoldierBowLong = 41;
		inline constexpr auto HeroUnarmed = 42;
		inline constexpr auto HeroSpearSIEGFRIED = 43;
		inline constexpr auto HeroSwordBJARNI = 44;
		inline constexpr auto HeroSaberHATSCHI = 45;
		inline constexpr auto HeroAxe = 46;
		inline constexpr auto HeroineBowXENA = 47;
	}

	namespace HumanJobChangeToken
	{
		inline constexpr auto Gfxbobmanagerbody = 0x67666262;
		inline constexpr auto Gfxbobmanagerhead = 0x67666268;
		inline constexpr auto GfxPaletteRandom = 0x67667072;
		inline constexpr auto LogicJob = 0x6C6F6A6F;
		inline constexpr auto LogicTribe = 0x6C6F7472;
	}

	namespace HumanJobGraphicsToken
	{
		inline constexpr auto Gfxbobmanagerbody = 0x67666262;
		inline constexpr auto Gfxbobmanagerhead = 0x67666268;
		inline constexpr auto Gfxpalettebasebody = 0x67667062;
		inline constexpr auto Gfxpalettebasehead = 0x67667068;
		inline constexpr auto GfxPaletteRandom = 0x67667072;
		inline constexpr auto LogicJob = 0x6C6F6A6F;
		inline constexpr auto LogicTribe = 0x6C6F7472;
	}

	namespace HumanMilitaryMode
	{
		inline constexpr auto SearchAroundPosition = 1;
		inline constexpr auto SearchAroundWorkCenter = 2;
		inline constexpr auto SearchWithinWeaponRange = 3;
	}

	namespace HumanMiscSoundToken
	{
		inline constexpr auto MusicType = 0x6D747970u;
		inline constexpr auto SFX = 0x73667864u;
	}

	namespace HumanNameToken
	{
		inline constexpr auto LogicTribe = 0x6C6F7472;
		inline constexpr auto Name = 0x6E616D65;
	}

	namespace HumanNeed
	{
		inline constexpr auto Food = 1;
		inline constexpr auto Stamina = 2;
		inline constexpr auto Social = 3;
		inline constexpr auto Religion = 4;
	}

	namespace HumanTask
	{
		inline constexpr auto AttackUnsigned = 14u;

		inline constexpr auto ExecuteJobUnsigned = 8u;

		inline constexpr auto IdleUnsigned = 1u;

		inline constexpr auto Idle = 1;

		inline constexpr auto Goto = 2;
		inline constexpr auto GoHome = 3;
		inline constexpr auto GotoAndWait = 4;
		inline constexpr auto Marry = 5;
		inline constexpr auto FulfillNeeds = 7;
		inline constexpr auto ExecuteJob = 8;
		inline constexpr auto GetBoy = 11;
		inline constexpr auto GetGirl = 12;
		inline constexpr auto ReceiveLoveOrEnterHome = 13;
		inline constexpr auto Attack = 14;
		inline constexpr auto ApproachAttackTarget = 15;
		inline constexpr auto Equip = 16;
		inline constexpr auto DropItem = 17;
		inline constexpr auto OpenChest = 18;
		inline constexpr auto ExploreArea = 20;
		inline constexpr auto GotoAndResetTarget = 23;
		inline constexpr auto ChooseGoodie = 24;
		inline constexpr auto GoodieIdle = 25;
		inline constexpr auto GoodieWalkAround = 26;
		inline constexpr auto GoodieSleep = 27;
		inline constexpr auto GoodieTalkToAny = 28;
		inline constexpr auto GoodiePlayWithAnimal = 29;
	}

	namespace HumanTaskFailure
	{
		inline constexpr auto NoWork = 0;
		inline constexpr auto NoWay = 1;
		inline constexpr auto WorkHouseUnavailable = 2;
		inline constexpr auto CannotFindGood = 3;
		inline constexpr auto CannotFindBuildGood = 4;
		inline constexpr auto CannotFindHomeGood = 5;
		inline constexpr auto NoTargetHuman = 6;
		inline constexpr auto NoWorkPoint = 7;
		inline constexpr auto NoVehicleBuildPlace = 8;
		inline constexpr auto VehicleBuildPlaceOccupied = 9;
		inline constexpr auto Silent = 10;
		inline constexpr auto MustWaitForGood = 11;
		inline constexpr auto CannotAffordTraining = 13;
		inline constexpr auto VehicleUnavailable = 14;
		inline constexpr auto TraderHouseUnavailable = 15;
	}

	namespace HumanWalkAnimationToken
	{
		inline constexpr auto Gfxbobseqbody = 0x67667862;
		inline constexpr auto GfxWalkFrameList = 0x67667866;
		inline constexpr auto Gfxbobseqhead = 0x67667868;
		inline constexpr auto Gfxturnframelist = 0x67667874;
		inline constexpr auto LogicGoodType = 0x6C6F676F;
		inline constexpr auto LogicJob = 0x6C6F6A6F;
		inline constexpr auto LogicTribe = 0x6C6F7472;
		inline constexpr auto LogicWalkSpeed = 0x6C6F7773;
	}

	namespace HyperlinkController
	{
		inline constexpr auto NewLine = 0;
		inline constexpr auto Font = 1;
		inline constexpr auto Palette = 2;
		inline constexpr auto Alignment = 3;
		inline constexpr auto XStart = 4;
		inline constexpr auto XEnd = 5;
		inline constexpr auto Anchor = 6;
		inline constexpr auto LocalJump = 7;
		inline constexpr auto GlobalJump = 8;
		inline constexpr auto OnscreenCallback = 9;
	}

	namespace HyperlinkToken
	{
		inline constexpr auto Anchor = 0x616E6368;
		inline constexpr auto BlockAlignment = 0x626C636B;
		inline constexpr auto Color = 0x636F6C6F;
		inline constexpr auto Font = 0x666F6E74;
		inline constexpr auto GlobalJump = 0x676A6D70;
		inline constexpr auto Icon = 0x69636F6E;
		inline constexpr auto Include = 0x696E636C;
		inline constexpr auto LocalJump = 0x6C6A6D70;
		inline constexpr auto OnscreenCallback = 0x6F736362;
		inline constexpr auto Picture = 0x70696374;
		inline constexpr auto UserIcon = 0x7569636F;
		inline constexpr auto UserPicture = 0x75706963;
		inline constexpr auto XEnd = 0x78656E64;
		inline constexpr auto XStart = 0x78737461;
	}

	namespace InputMode
	{
		inline constexpr auto GateStart = 18;

		inline constexpr auto WallStart = 16;

		inline constexpr auto HousePlacement = 12;

		inline constexpr auto RoadStart = 14;

		inline constexpr auto VehicleAttackHuman = 24;

		inline constexpr auto AttachHomeHouse = 4;
		inline constexpr auto AttachWorkHouse = 5;
		inline constexpr auto AttachLearnHouse = 6;
		inline constexpr auto AttachBuildHouse = 7;
		inline constexpr auto AttachTradeHouse = 8;
		inline constexpr auto DetachTradeHouse = 9;
	}

	namespace InteractionTargetType
	{
		inline constexpr auto Position = 1;
		inline constexpr auto Landscape = 2;
		inline constexpr auto RoadBuildPlace = 3;
		inline constexpr auto WallBuildPlace = 4;
		inline constexpr auto House = 5;
		inline constexpr auto Vehicle = 6;
		inline constexpr auto Human = 7;
		inline constexpr auto Animal = 8;
	}

	namespace JobExperienceToken
	{
		inline constexpr auto Job = 0x6A6F62u;
		inline constexpr auto Baserepeatcounter = 0x6272636Eu;
		inline constexpr auto ExperienceFactor = 0x65787066u;
		inline constexpr auto Good = 0x676F6F64u;
		inline constexpr auto Name = 0x6E616D65u;
	}

	namespace JobTypeToken
	{
		inline constexpr auto Allowatomic = 0x616C6C61u;
		inline constexpr auto Baseatomics = 0x62617361u;
		inline constexpr auto CanBeTrainedFlag = 0x63627472u;
		inline constexpr auto CanHaveWorkHouseFlag = 0x63687768u;
		inline constexpr auto Forbidatomic = 0x666F6261u;
		inline constexpr auto IgnoresHomeHouseFlag = 0x6967686Fu;
		inline constexpr auto MustHaveFinishedWorkHouseFlag = 0x6D686677u;
		inline constexpr auto MustHaveWorkHouseFlag = 0x6D687768u;
		inline constexpr auto Name = 0x6E616D65u;
		inline constexpr auto NeedsReligionFlag = 0x6E72656Cu;
		inline constexpr auto UserShouldAttachWorkPlaceAfterJobChangeFlag = 0x75636177u;
		inline constexpr auto UserCanChangeProductionFlag = 0x75636370u;
	}

	namespace LandscapeToken
	{
		inline constexpr auto EditGroups = 0x65646772u;
		inline constexpr auto EditName = 0x65646E61u;
		inline constexpr auto GfxLoopAnimation = 0x67667861u;
		inline constexpr auto Gfxboblibs = 0x67667862u;
		inline constexpr auto GfxDynamicBackground = 0x67667864u;
		inline constexpr auto GfxFrames = 0x67667866u;
		inline constexpr auto GfxPalette = 0x67667870u;
		inline constexpr auto GfxStatic = 0x67667873u;
		inline constexpr auto GfxTransition = 0x67667874u;
		inline constexpr auto GfxShadingFactor = 0x67667875u;
		inline constexpr auto GfxDrawVoidEver = 0x67667876u;
		inline constexpr auto GfxUserFxMatrix = 0x67667878u;
		inline constexpr auto LogicAdditionalAttachPointArea = 0x6C6F6161u;
		inline constexpr auto LogicBuildBlockArea = 0x6C6F6262u;
		inline constexpr auto LogicIsPileableOnMap = 0x6C6F7075u;
		inline constexpr auto Logictype = 0x6C6F7479u;
		inline constexpr auto LogicMaximumValency = 0x6C6F7661u;
		inline constexpr auto Logicworkarea = 0x6C6F7761u;
		inline constexpr auto LogicWalkBlockArea = 0x6C6F7762u;
		inline constexpr auto LogicIsWorkable = 0x6C6F776Fu;
	}

	namespace Latin1Character
	{
		inline constexpr auto UppercaseAUmlaut = 0xC4;
		inline constexpr auto UppercaseOUmlaut = 0xD6;
		inline constexpr auto UppercaseUUmlaut = 0xDC;
		inline constexpr auto LowercaseAUmlaut = 0xE4;
		inline constexpr auto LowercaseOUmlaut = 0xF6;
		inline constexpr auto LowercaseUUmlaut = 0xFC;
	}

	namespace LogicLandscapeToken
	{
		inline constexpr auto Allowedoneverything = 0x616C6C65;
		inline constexpr auto Allowedonland = 0x616C6C6C;
		inline constexpr auto Allowedonwater = 0x616C6C77;
		inline constexpr auto DebugColor = 0x64626763;
		inline constexpr auto MaximumValency = 0x6D617876;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto Playeridallowed = 0x706C6964;
		inline constexpr auto Transition = 0x7472616E;
	}

	namespace LogicMapChunk
	{
		inline constexpr auto Continents = 0x6C61636Fu;
		inline constexpr auto CleanFishManager = 0x6C61666Du;
		inline constexpr auto CleanWalkSectors = 0x6C617377u;
		inline constexpr auto AttachedObjects = 0x6C6D616Fu;
		inline constexpr auto MicroBuildingBlocked = 0x6C6D6262u;
		inline constexpr auto MicroContinent = 0x6C6D636Fu;
		inline constexpr auto MacroHeight = 0x6C6D6865u;
		inline constexpr auto MicroHouseBuildForbidden = 0x6C6D6866u;
		inline constexpr auto LandscapePlayer = 0x6C6D6C70u;
		inline constexpr auto LandscapeType = 0x6C6D6C74u;
		inline constexpr auto LandscapeValency = 0x6C6D6C76u;
		inline constexpr auto MicroMaximumMoveableSize = 0x6C6D6D73u;
		inline constexpr auto MacroPatternA = 0x6C6D7061u;
		inline constexpr auto MacroPatternB = 0x6C6D7062u;
		inline constexpr auto MicroPathResistance = 0x6C6D7072u;
		inline constexpr auto MicroRoad = 0x6C6D726Fu;
		inline constexpr auto MicroSectorBasePoint = 0x6C6D7362u;
		inline constexpr auto MicroWalkConnections = 0x6C6D7477u;
		inline constexpr auto MicroWalkingBlocked = 0x6C6D7762u;
		inline constexpr auto SectorAttachments = 0x7361616Du;
		inline constexpr auto BioRegrowth = 0x73616772u;
		inline constexpr auto WalkSectors = 0x73617377u;
		inline constexpr auto AutoTransitions = 0x7361746Du;
		inline constexpr auto FishManager = 0x7366696Du;
		inline constexpr auto Exploration = 0x736D6578u;
		inline constexpr auto FirstMapMoveableId = 0x736D666Du;
		inline constexpr auto Guides = 0x736D6775u;
		inline constexpr auto Houses = 0x736D686Fu;
		inline constexpr auto RoadBuildManager = 0x73726F64u;
		inline constexpr auto WallBuildManager = 0x73726F77u;
	}

	namespace LogicObjectType
	{
		inline constexpr auto Human = 1;
		inline constexpr auto Animal = 2;
		inline constexpr auto Vehicle = 3;
		inline constexpr auto House = 4;
		inline constexpr auto Guide = 5;
	}

	namespace MainMenuCommand
	{
		inline constexpr auto ShowExitConfirmation = 0x1388;
		inline constexpr auto ExitGame = 0x1389;
		inline constexpr auto ShowMainMenu = 0x138A;
		inline constexpr auto ShowSinglePlayerMenu = 0x138B;
		inline constexpr auto ShowMultiplayerCallsign = 0x138C;
		inline constexpr auto PlayIntro = 0x138D;
		inline constexpr auto InternetLobby = 0x138E;
		inline constexpr auto ShowCredits = 0x138F;
		inline constexpr auto MainMenuAfterCdCheck = 0x1390;
		inline constexpr auto DisableSinglePlayer = 0x1391;
		inline constexpr auto ShowMapList = 0x1392;
		inline constexpr auto ShowCampaignMap = 0x1393;
		// Die Saga reaches its three campaigns and the user campaign list through four
		// consecutive commands. 0x1393 already means the first campaign screen; the rest
		// live past the engine's existing map/load commands.
		inline constexpr auto ShowCampaignMap2 = 0x13C0;
		inline constexpr auto ShowCampaignMap3 = 0x13C1;
		inline constexpr auto ShowUserCampaignMenu = 0x13C2;
		inline constexpr auto ShowUserCampaign00 = 0x13C3;
		inline constexpr auto ShowUserCampaign01 = 0x13C4;
		inline constexpr auto ShowTutorialMapList = 0x1394;
		inline constexpr auto ShowLoadGame = 0x1395;
		inline constexpr auto StartSelectedMap = 0x1396;
		inline constexpr auto StartSelectedCampaignMap = 0x1397;
		inline constexpr auto LoadSelectedSavegame = 0x1398;
		inline constexpr auto SortSavegamesByDate = 0x1399;
		inline constexpr auto SortSavegamesByMapName = 0x139A;
		inline constexpr auto SortSavegamesBySlot = 0x139B;
		inline constexpr auto DifficultyNormal = 0x139C;
		inline constexpr auto DifficultyEasy = 0x139D;
		inline constexpr auto DifficultyEasiest = 0x139E;
		inline constexpr auto StartPreGameVideo = 0x139F;
		inline constexpr auto ReturnMainMenuAndShutDownNetwork = 0x13A0;
		inline constexpr auto ConfirmCallsign = 0x13A1;
		inline constexpr auto SelectServiceProvider = 0x13A2;
		inline constexpr auto HostSession = 0x13A3;
		inline constexpr auto PrepareJoinSession = 0x13A4;
		inline constexpr auto SearchSessions = 0x13A5;
		inline constexpr auto JoinSelectedSession = 0x13A6;
		inline constexpr auto LobbySelectMap = 0x13A7;
		inline constexpr auto LobbySelectSavegame = 0x13A8;
		inline constexpr auto LobbyShowPlayers = 0x13A9;
		inline constexpr auto LobbyChooseMap = 0x13AA;
		inline constexpr auto LobbyDifficultyNormal = 0x13AB;
		inline constexpr auto LobbyDifficultyEasy = 0x13AC;
		inline constexpr auto LobbyDifficultyEasiest = 0x13AD;
		inline constexpr auto LobbyChooseSavegame = 0x13AE;
		inline constexpr auto ToggleConnections = 0x13AF;
		inline constexpr auto StartNetworkGame = 0x13B0;
		inline constexpr auto SelectNetworkPlayer = 0x13B1;
		inline constexpr auto KickPlayer = 0x13B2;
		inline constexpr auto AssignPlayerSlot = 0x13B3;
		inline constexpr auto ChangePlayerType = 0x13B4;
		inline constexpr auto ChangePlayerColor = 0x13B5;
	}

	namespace MainMenuScreen
	{
		inline constexpr auto MainMenu = 1;
		inline constexpr auto ExitConfirmation = 2;
		inline constexpr auto SinglePlayerMenu = 3;
		inline constexpr auto MapList = 4;
		inline constexpr auto CampaignMap = 5;
		inline constexpr auto LoadGame = 6;
		inline constexpr auto DifficultyOptions = 7;
		inline constexpr auto TutorialMapList = 8;
		inline constexpr auto EnterCallsign = 9;
		inline constexpr auto SelectServiceProvider = 10;
		inline constexpr auto HostOrJoin = 11;
		inline constexpr auto EnterSessionName = 12;
		inline constexpr auto SessionList = 13;
		inline constexpr auto Lobby = 14;
		inline constexpr auto PlayerConfiguration = 15;
		inline constexpr auto FileUpload = 16;
		inline constexpr auto FileDownload = 17;
		inline constexpr auto NetworkError = 18;
		inline constexpr auto AutoHost = 19;
		inline constexpr auto AutoJoin = 20;
		inline constexpr auto Disconnected = 21;
		inline constexpr auto Credits = 22;
		inline constexpr auto InsertGameCd = 23;
		// Die Saga offers three campaign screens and a user campaign list. The original puts
		// them at 5..10 and shifts every later screen by five; this engine keeps its own
		// numbering and appends them instead, so the earlier variants stay untouched.
		inline constexpr auto CampaignMapSecond = 24;
		inline constexpr auto CampaignMapThird = 25;
		inline constexpr auto UserCampaignMenu = 26;
		inline constexpr auto UserCampaignList00 = 27;
		inline constexpr auto UserCampaignList01 = 28;
		inline constexpr auto Skirmish = 29;
	}

	namespace MapMoveableType
	{
		inline constexpr auto Human = 1;
		inline constexpr auto Animal = 2;
		inline constexpr auto Vehicle = 3;
		inline constexpr auto Marker = 4;
	}

	namespace MapObjectUnderCursor
	{
		inline constexpr auto None = 0;
		inline constexpr auto RoadBuildPlaceUnsigned = 7u;
		inline constexpr auto WallBuildPlaceUnsigned = 8u;
		inline constexpr auto Human = 1;
		inline constexpr auto Animal = 2;
		inline constexpr auto Vehicle = 3;
		inline constexpr auto House = 4;
		inline constexpr auto Guide = 5;
		inline constexpr auto Landscape = 6;
		inline constexpr auto RoadBuildPlace = 7;
		inline constexpr auto WallBuildPlace = 8;
		inline constexpr auto Position = 9;
	}

	namespace MathErrorType
	{
		inline constexpr auto Domain = 1;
		inline constexpr auto Singularity = 2;
		inline constexpr auto Overflow = 3;
		inline constexpr auto Underflow = 4;
		inline constexpr auto TotalLoss = 5;
		inline constexpr auto DomainAlias = 7;
		inline constexpr auto Inexact = 8;
	}

	namespace MathRecordPrecision
	{
		inline constexpr auto Extended64 = 0;
		inline constexpr auto Double53 = 1;
		inline constexpr auto Single24 = 2;
	}

	namespace MathRecordRounding
	{
		inline constexpr auto Nearest = 0;
		inline constexpr auto Down = 1;
		inline constexpr auto Up = 2;
		inline constexpr auto TowardZero = 3;
	}

	namespace MenuNetworkMessage
	{
		inline constexpr auto LobbyData = 1;
		inline constexpr auto RequestLobbyData = 2;
		inline constexpr auto Disconnected = 3;
		inline constexpr auto StartUpload = 4;
		inline constexpr auto StartDownload = 5;
	}

	namespace MessagePriorityMode
	{
		inline constexpr auto Normal = 0;
		inline constexpr auto Elevated = 1;
		inline constexpr auto ScoutOnly = 2;
	}

	namespace MicroGridOffset
	{
		inline constexpr auto Origin = 0;
		inline constexpr auto East = 1;
		inline constexpr auto SouthWest = 2;
		inline constexpr auto SouthEast = 3;
	}

	namespace MiscHouseLandscapeToken
	{
		inline constexpr auto LogicHouseType = 0x6C687479;
		inline constexpr auto LandscapeName = 0x6C6E616D;
		inline constexpr auto LogicTribeType = 0x6C747274;
	}

	namespace MiscLandscapeToken
	{
		inline constexpr auto LogicLandscapeType = 0x6C6C7479;
		inline constexpr auto LandscapeName = 0x6C6E616D;
		inline constexpr auto LogicTribeType = 0x6C747274;
	}

	namespace MissionGoal
	{
		inline constexpr auto True = 0;
		inline constexpr auto BuildVehicles = 1;
		inline constexpr auto BuildHumans = 2;
		inline constexpr auto BuildHouses = 3;
		inline constexpr auto GoodsInVehicles = 4;
		inline constexpr auto GoodsInHouses = 5;
		inline constexpr auto GoodsGlobal = 6;
		inline constexpr auto FindPos = 7;
		inline constexpr auto FindHumans = 8;
		inline constexpr auto FindVehicles = 9;
		inline constexpr auto FindHouses = 10;
		inline constexpr auto FindPosByHumans = 11;
		inline constexpr auto FindPosByVehicles = 12;
		inline constexpr auto FindHumansByHumans = 13;
		inline constexpr auto FindHumansByVehicles = 14;
		inline constexpr auto FindVehiclesByVehicles = 15;
		inline constexpr auto FindHousesByHumans = 16;
		inline constexpr auto FindHousesByVehicles = 17;
		inline constexpr auto HumansDied = 18;
		inline constexpr auto VehiclesDied = 19;
		inline constexpr auto HousesDied = 20;
		inline constexpr auto PlayerDied = 21;
		inline constexpr auto FindPosByPlayersMapMoveable = 22;
		inline constexpr auto BuildHouseOnContinent = 23;
		inline constexpr auto DiplomacyState = 24;
		inline constexpr auto HumansWithHome = 25;
		inline constexpr auto NumberOfSoldiers = 26;
		inline constexpr auto DetectGuide = 27;
		inline constexpr auto TimeGone = 28;
		inline constexpr auto CheckMission = 29;
		inline constexpr auto PayTribute = 30;
		inline constexpr auto Population = 31;
		inline constexpr auto PlayerSeen = 32;
		inline constexpr auto IsHumanInVehicle = 33;
		inline constexpr auto FindHumansByPlayersMM = 34;
		inline constexpr auto SoldiersDied = 35;
		inline constexpr auto AnimalsDied = 36;
		inline constexpr auto PlayerAttackedByPlayer = 37;
		inline constexpr auto JobEnabled = 38;
		inline constexpr auto GoodProduceable = 39;
		inline constexpr auto NumberOfHumansDied = 40;
		inline constexpr auto FindAnimals = 41;
		inline constexpr auto NumberOfGoodsTraded = 42;
		inline constexpr auto HumanAttachedToWorkHouse = 43;
		inline constexpr auto CheckNumberOfWildAnimals = 44;
		inline constexpr auto RandomTimeGone = 45;
		inline constexpr auto IfMissionIsActive = 46;
		inline constexpr auto NumberOfAnimals = 47;
		inline constexpr auto NumberOfHumansKilled = 48;
		inline constexpr auto HumanIsOnContinent = 49;
		inline constexpr auto IsMissionDone = 50;
		inline constexpr auto NumberOfSoldiersNearPos = 51;
		inline constexpr auto NumberOfCivilainsNearPos = 52;
		inline constexpr auto ChestNearPos = 53;
		inline constexpr auto NumberOfGoodsInArea = 54;
		inline constexpr auto NumberOfHousesInArea = 55;
		inline constexpr auto NumberOfGoodsInHousesInArea = 56;
		inline constexpr auto NumberOfVehiclesInArea = 57;
		inline constexpr auto NumberOfAnimalsInArea = 58;
		inline constexpr auto CheckHumanJob = 59;
		inline constexpr auto NumberOfGoodsInVehiclesInArea = 60;
		inline constexpr auto IsAnyLandscapeOnPoint = 61;
		inline constexpr auto IsLandscapePlayer10ConstructionSignOnPoint = 62;
	}

	namespace MissionGoalMatchMode
	{
		inline constexpr auto All = 0;
		inline constexpr auto Any = 1;
		inline constexpr auto MajorityAtLeastHalf = 2;
		inline constexpr auto None = 3;
	}

	namespace MissionParameterType
	{
		inline constexpr auto Tribe = 3;
		inline constexpr auto Job = 4;
		inline constexpr auto Vehicle = 5;
		inline constexpr auto Good = 6;
		inline constexpr auto House = 15;
		inline constexpr auto DiplomacyState = 25;
	}

	namespace MissionResult
	{
		inline constexpr auto None = 0;
		inline constexpr auto SetHuman = 1;
		inline constexpr auto SetVehicle = 2;
		inline constexpr auto SetHouse = 3;
		inline constexpr auto SetLandscape = 4;
		inline constexpr auto RemoveHumans = 5;
		inline constexpr auto RemoveVehicles = 6;
		inline constexpr auto RemoveHouses = 7;
		inline constexpr auto RemoveLandscape = 8;
		inline constexpr auto PlayCutscene = 9;
		inline constexpr auto ActivateMission = 10;
		inline constexpr auto DeactivateMission = 11;
		inline constexpr auto MissionWon = 12;
		inline constexpr auto MissionFailed = 13;
		inline constexpr auto AllowMap = 14;
		inline constexpr auto CloseMap = 15;
		inline constexpr auto ExploreArea = 16;
		inline constexpr auto Exit = 17;
		inline constexpr auto SetExternalFlag = 18;
		inline constexpr auto SetDiplomacy = 19;
		inline constexpr auto SetVisible = 20;
		inline constexpr auto ChangeHumanPlayerId = 21;
		inline constexpr auto ChangePlayerPlayerId = 22;
		inline constexpr auto SendHuman = 23;
		inline constexpr auto SendVehicle = 24;
		inline constexpr auto DockVehicle = 25;
		inline constexpr auto PlaySoundEffect = 26;
		inline constexpr auto CreateTribute = 27;
		inline constexpr auto AddTributeGoods = 28;
		inline constexpr auto AllowJob = 29;
		inline constexpr auto AllowHouse = 30;
		inline constexpr auto AddGoodsToHouses = 31;
		inline constexpr auto StartSubMission = 32;
		inline constexpr auto EndSubMission = 33;
		inline constexpr auto ChangeVehiclesPlayerId = 34;
		inline constexpr auto ChangeHousesPlayerId = 35;
		inline constexpr auto SetImportHumanFlag = 36;
		inline constexpr auto EnableJob = 37;
		inline constexpr auto EnableHouse = 38;
		inline constexpr auto DisableAll = 39;
		inline constexpr auto AddGoodsToVehicle = 40;
		inline constexpr auto AddGoodsToAnyStock = 41;
		inline constexpr auto AllowGood = 42;
		inline constexpr auto EnableGood = 43;
		inline constexpr auto ChangePlayerIdInArea = 44;
		inline constexpr auto SetDiplomacyNotChangeableFlag = 45;
		inline constexpr auto RemoveFXWaveLandscapeInArea = 46;
		inline constexpr auto SetWallGateOpen = 47;
		inline constexpr auto MissionQuitAndPlayVideo = 48;
		inline constexpr auto SetPlayerBehaviourFlag = 49;
		inline constexpr auto SetHumanBehaviourFlag = 50;
		inline constexpr auto SetRandomChestOnRandomPos = 51;
		inline constexpr auto RemoveHumansNearPos = 52;
		inline constexpr auto StopHumanByPlayerId = 53;
		inline constexpr auto SetAnimal = 54;
		inline constexpr auto RemoveAnimals = 55;
		inline constexpr auto ChangeHumanObjectIdInArea = 56;
		inline constexpr auto HealHumansInArea = 57;
		inline constexpr auto ClearTribute = 58;
		inline constexpr auto SetGuiMarker = 59;
		inline constexpr auto SetHumanName = 60;
		inline constexpr auto SetWeather = 61;
		inline constexpr auto StartEarthquake = 62;
		inline constexpr auto SelectHuman = 63;
		inline constexpr auto AddGoodsToMapArea = 64;
		inline constexpr auto RemoveGoodsFromMapArea = 65;
		inline constexpr auto ChangeMissionIdOfHumanInRange = 66;
		inline constexpr auto ChangeMissionIdOfPlayer = 67;
		inline constexpr auto SetVertexColor = 68;
		inline constexpr auto RemoveLandscapesInArea = 69;
		inline constexpr auto MoveUnitsInArea = 70;
		inline constexpr auto SetHouseExtensionLevel = 71;
		inline constexpr auto InfoClear = 72;
		inline constexpr auto InfoShowString = 73;
		inline constexpr auto InfoCountGoodsInArea = 74;
		inline constexpr auto InfoCountHousesInArea = 75;
		inline constexpr auto InfoCountHumenInArea = 76;
		inline constexpr auto InfoCountSoldiersInArea = 77;
		inline constexpr auto InfoCountAnimalsInArea = 78;
		inline constexpr auto RemoveFXSmokeLandscapeInArea = 79;
		inline constexpr auto ChangeAnimalPlayerIdInArea = 80;
		inline constexpr auto RemoveHPsOfHousesInArea = 81;
		inline constexpr auto RemoveBlockerLandscapeInArea = 82;
		inline constexpr auto RemoveFX1LandscapeInArea = 83;
		inline constexpr auto RemoveFX2LandscapeInArea = 84;
		inline constexpr auto SetCameraPosition = 85;
		inline constexpr auto SetHumanX = 86;
		inline constexpr auto RemoveHPsOfHousesInAreaX = 87;
		inline constexpr auto SetHouseOverlayState = 88;
		inline constexpr auto AttachHumanToVehicle = 89;
		inline constexpr auto DetachHumanFromVehicle = 90;
		inline constexpr auto SetHouseBuildForbiddenArea = 91;
		inline constexpr auto MoveHuman = 92;
		inline constexpr auto SetHouseBehaviourFlag = 93;
		inline constexpr auto ChangeMissionIdOfVehiclesInRange = 94;
		inline constexpr auto RemoveVehiclesWithMissionId = 95;
		inline constexpr auto SetImportLandscapeMarker = 96;
		inline constexpr auto SetVertexColorOnLand = 97;
		inline constexpr auto ChangeMissionIdOfPlayersVehiclesOnContinent = 98;
		inline constexpr auto ChangeMissionIdOfVehicles = 99;
		inline constexpr auto SetRandomChestOnPosition = 100;
		inline constexpr auto SetMapAreaMarker = 101;
		inline constexpr auto SetMapAreaMarkerMagic = 102;
	}

	namespace MissionScriptToken
	{
		inline constexpr auto Active = 0x61637469;
		inline constexpr auto DebugInfo = 0x64656275;
		inline constexpr auto Description = 0x64657363;
		inline constexpr auto Goal = 0x676F616C;
		inline constexpr auto Result = 0x72657375;
		inline constexpr auto Successfullif = 0x73756363;
		inline constexpr auto Visible = 0x76697369;
	}

	namespace MissionStartToken
	{
		inline constexpr auto AddGoods = 0x61676F64;
		inline constexpr auto AttachToHouse = 0x6174686F;
		inline constexpr auto AttachToVehicle = 0x61747665;
		inline constexpr auto ChildOfWoman = 0x636F776F;
		inline constexpr auto Marry = 0x6D617272;
		inline constexpr auto MoveInToVehicle = 0x6D697665;
		inline constexpr auto SetAnimal = 0x73616E69;
		inline constexpr auto SetExperience = 0x73657870;
		inline constexpr auto SetGuide = 0x73677569;
		inline constexpr auto SetHouse = 0x73686F75;
		inline constexpr auto SetHuman = 0x7368756D;
		inline constexpr auto SetProducedGood = 0x73706764;
		inline constexpr auto SetVehicle = 0x73766568;
	}

	namespace MultiplayerGoal
	{
		inline constexpr auto Survive = 1;
		inline constexpr auto CollectGoods = 2;
		inline constexpr auto Population = 3;
		inline constexpr auto PlayersWin = 4;
		inline constexpr auto PlayersLose = 5;
	}

	namespace MusicEventVariant
	{
		inline constexpr auto Neutral = 4;
		inline constexpr auto Positive = 5;
		inline constexpr auto Negative = 6;
	}

	namespace NetworkApplicationMessage
	{
		inline constexpr auto ChatText = 1;
		inline constexpr auto DebugHumanCommand = 2;
	}

	namespace NetworkEvent
	{
		inline constexpr auto Error = 1;
		inline constexpr auto PlayerJoined = 2;
		inline constexpr auto PlayerLeft = 3;
		inline constexpr auto PlayerKicked = 4;
		inline constexpr auto ConnectionLost = 5;
		inline constexpr auto HostTerminatedSession = 6;
		inline constexpr auto PlayerLostConnection = 7;
		inline constexpr auto HostMigrated = 8;
		inline constexpr auto PlayerStoppedResponding = 9;
		inline constexpr auto HostUnavailable = 10;
		inline constexpr auto RecoverMissingCommands = 11;
		inline constexpr auto Timeout = 12;
		inline constexpr auto HostHandlingUnresponsivePlayer = 13;
	}

	namespace NetworkPacket
	{
		inline constexpr auto AllowConnections = 2;
		inline constexpr auto Chat = 3;
		inline constexpr auto DynamicData = 4;
		inline constexpr auto Commands = 5;
		inline constexpr auto PlayerInfo = 6;
		inline constexpr auto ConnectionProbe = 7;
		inline constexpr auto PrepareCommandBuffers = 8;
		inline constexpr auto RequestCommands = 9;
		inline constexpr auto ConvertCommands = 10;
		inline constexpr auto FileTransferAnnouncement = 11;
		inline constexpr auto FileTransferAccept = 12;
		inline constexpr auto FileTransferRequestHeader = 13;
		inline constexpr auto FileTransferHeader = 14;
		inline constexpr auto FileTransferData = 15;
	}

	namespace ObjectGroupType
	{
		inline constexpr auto Human = 1;
		inline constexpr auto Vehicle = 2;
		inline constexpr auto House = 3;
	}

	namespace OptionsPage
	{
		inline constexpr auto Game = 0;
		inline constexpr auto General = 1;
		inline constexpr auto Graphics = 2;
		inline constexpr auto Sound = 3;
		inline constexpr auto Load = 4;
		inline constexpr auto Save = 5;
	}

	namespace OptionsWindowCommand
	{
		inline constexpr auto UpdateGameSpeed = 0x7EE;
		inline constexpr auto DetailLow = 0x2774;
		inline constexpr auto DetailMedium = 0x2775;
		inline constexpr auto DetailHigh = 0x2776;
		inline constexpr auto SoftwareMouse = 0x277E;
		inline constexpr auto HardwareMouse = 0x277F;
		inline constexpr auto ScrollSlow = 0x27D8;
		inline constexpr auto ScrollNormal = 0x27D9;
		inline constexpr auto ScrollFast = 0x27DA;
		inline constexpr auto ToggleThirdMouseButtonScroll = 0x27EC;
		inline constexpr auto ToggleBorderScroll = 0x27ED;
		inline constexpr auto SoundQualityLow = 0x2968;
		inline constexpr auto SoundQualityMedium = 0x2969;
		inline constexpr auto SoundQualityHigh = 0x296A;
		inline constexpr auto SoundVolumeDown = 0x296B;
		inline constexpr auto SoundVolumeUp = 0x296C;
		inline constexpr auto JinglesOn = 0x296D;
		inline constexpr auto JinglesOff = 0x296E;
		inline constexpr auto MusicOff = 0x2972;
		inline constexpr auto DirectMusic = 0x2973;
		inline constexpr auto CdMusic = 0x2974;
		inline constexpr auto PreviewNeutralMusic = 0x29CC;
		inline constexpr auto PreviewPositiveMusic = 0x29CD;
		inline constexpr auto PreviewNegativeMusic = 0x29CE;
		inline constexpr auto PreviewBirthJingle = 0x29D6;
		inline constexpr auto PreviewMarriageJingle = 0x29D7;
		inline constexpr auto MusicVolumeDown = 0x29E0;
		inline constexpr auto MusicVolumeUp = 0x29E1;
		inline constexpr auto StartCdTrack = 0x2A30;
		inline constexpr auto StopCdTrack = 0x2A31;
		inline constexpr auto CdVolumeDown = 0x2A32;
		inline constexpr auto CdVolumeUp = 0x2A33;
		inline constexpr auto SortSavegamesByDate = 0x2AF8;
		inline constexpr auto SortSavegamesByMapName = 0x2AF9;
		inline constexpr auto SortSavegamesBySlot = 0x2AFA;
		inline constexpr auto LoadSavegame = 0x2B02;
		inline constexpr auto SaveGame = 0x2B03;
		inline constexpr auto ConfirmQuit = 0x32C8;
		inline constexpr auto ConfirmRestart = 0x32C9;
		inline constexpr auto ExpertModeOff = 0x32D2;
		inline constexpr auto ExpertModeOn = 0x32D3;
		inline constexpr auto TooltipsOn = 0x32D4;
		inline constexpr auto TooltipsOff = 0x32D5;
		inline constexpr auto GuiDetailedHoverHints = 0x32DC;
		inline constexpr auto GuiCompactHoverHints = 0x32DD;
	}

	namespace OverviewWindowCommand
	{
		inline constexpr auto Close = 10000;
		inline constexpr auto CycleViewMode = 10001;
		inline constexpr auto ToggleLargeOverview = 10005;
		inline constexpr auto ZoomOut = 10010;
		inline constexpr auto ZoomIn = 10011;
	}

	namespace PackingType
	{
		inline constexpr auto None = 0x6E6F6E65u;
		inline constexpr auto NoneSigned = 0x6E6F6E65;
		inline constexpr auto Rle16 = 0x726C6536u;
		inline constexpr auto Rle16Signed = 0x726C6536;
		inline constexpr auto Rle8 = 0x726C6538u;
		inline constexpr auto Rle8Signed = 0x726C6538;
	}

	namespace Palette16Token
	{
		inline constexpr auto EditName = 0x65646E61u;
		inline constexpr auto GfxColorRange = 0x67667863u;
	}

	namespace PaletteToken
	{
		inline constexpr auto EditName = 0x65646E61u;
		inline constexpr auto Gfxfile = 0x67667866u;
		inline constexpr auto GfxPreshade = 0x67667870u;
		inline constexpr auto GfxRemapToPreshaded = 0x67667872u;
	}

	namespace ParticleEffectToken
	{
		inline constexpr auto Drawbottom = 0x64626F74;
		inline constexpr auto Deltaheight = 0x64686569;
		inline constexpr auto LifeTime = 0x6C696665;
		inline constexpr auto MigPointoffset = 0x6D69676F;
		inline constexpr auto Movement = 0x6D6F7665;
		inline constexpr auto PixelOffset = 0x7069786F;
		inline constexpr auto Particelname = 0x706E616D;
		inline constexpr auto Particelspeed = 0x70737064;
		inline constexpr auto StartLifeTime = 0x736C6966;
		inline constexpr auto Usemapheights = 0x75736568;
		inline constexpr auto Usewind = 0x75736577;
	}

	namespace ParticleGeneratorToken
	{
		inline constexpr auto Generate = 0x67656E65;
		inline constexpr auto Groundcollision = 0x67726463;
		inline constexpr auto HeightOffset = 0x6867746F;
		inline constexpr auto MaxGoneLifeTime = 0x6D617867;
		inline constexpr auto MaxLifeTime = 0x6D61786C;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto Particelmove = 0x706D6F76;
		inline constexpr auto Particelspeed = 0x70737064;
		inline constexpr auto SetArea = 0x73657461;
		inline constexpr auto Usemapheights = 0x75736568;
		inline constexpr auto Usewind = 0x75736577;
	}

	namespace MapMicroObjectType
	{
		inline constexpr auto Empty = 0;
		inline constexpr auto Landscape = 1;
		inline constexpr auto House = 2;
		inline constexpr auto Guide = 3;
	}

	namespace AISoldierAction
	{
		inline constexpr auto Waiting = 1;
		inline constexpr auto Moving = 2;
		inline constexpr auto Attacking = 14;
		inline constexpr auto ChangingEquipment = 16;
	}

	namespace MacroMapSection
	{
		inline constexpr auto PatternNames = 0;
		inline constexpr auto Patterns = 1;
		inline constexpr auto TransitionNames = 2;
		inline constexpr auto Transitions = 3;
		inline constexpr auto LandscapeNames = 4;
		inline constexpr auto Landscapes = 5;
		inline constexpr auto Heights = 6;
		inline constexpr auto VertexColors = 7;
		inline constexpr auto Roads = 8;
		inline constexpr auto Humans = 9;
		inline constexpr auto Animals = 10;
		inline constexpr auto Houses = 11;
		inline constexpr auto FishSwarms = 12;
	}

	namespace ParticleMotionMode
	{
		inline constexpr auto Stationary = 0;
		inline constexpr auto Linear = 1;
		inline constexpr auto Parabolic = 2;
		inline constexpr auto Rise = 3;
		inline constexpr auto Fall = 4;
		inline constexpr auto RiseAlternate = 5;
		inline constexpr auto FallWithGroundRefresh = 6;
		inline constexpr auto FollowGround = 7;
	}

	namespace ParticleValencyMode
	{
		inline constexpr auto Fixed = 0;
		inline constexpr auto RemainingLifetime = 1;
		inline constexpr auto ElapsedLifetime = 2;
	}

	namespace ParticleBobType
	{
		inline constexpr auto Opaque = 0;
		inline constexpr auto LifetimeFade = 1;
		inline constexpr auto Transparency = 2;
		inline constexpr auto EffectMatrix = 3;
	}

	namespace ParticleToken
	{
		inline constexpr auto Animloop = 0x616E696D;
		inline constexpr auto Bobmanager = 0x626F626D;
		inline constexpr auto Bobtype = 0x626F6274;
		inline constexpr auto GfxFrames = 0x67667866;
		inline constexpr auto MaxValency = 0x6D617876;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto Palette = 0x70616C65;
		inline constexpr auto SpecialType = 0x73706563;
		inline constexpr auto SpawnparticelId = 0x73706964;
		inline constexpr auto ValencyIsDirection = 0x76696469;
		inline constexpr auto MunitionType = 0x776D7479;
	}

	namespace PatternToken
	{
		inline constexpr auto EditGroups = 0x65646772u;
		inline constexpr auto EditName = 0x65646E61u;
		inline constexpr auto GfxCoordsA = 0x67667861u;
		inline constexpr auto GfxCoordsB = 0x67667862u;
		inline constexpr auto GfxTexture = 0x67667874u;
		inline constexpr auto Logictype = 0x6C6F7479u;
	}

	namespace PictureFileType
	{
		inline constexpr auto Pcx = 0;
		inline constexpr auto Bmp = 1;
		inline constexpr auto Unknown = 2;
	}

	namespace PixelBitCount
	{
		inline constexpr auto Indexed8 = 8;
		inline constexpr auto HighColor16 = 16;
		inline constexpr auto Rgb24 = 24;
		inline constexpr auto TrueColor32 = 32;
	}

	namespace PrintFormatState
	{
		inline constexpr auto Literal = 0;
		inline constexpr auto BeginConversion = 1;
		inline constexpr auto Flags = 2;
		inline constexpr auto Width = 3;
		inline constexpr auto BeginPrecision = 4;
		inline constexpr auto Precision = 5;
		inline constexpr auto LengthModifier = 6;
		inline constexpr auto Conversion = 7;
	}

	namespace ProgramState
	{
		inline constexpr auto TitleScreenShutDown = 6;

		inline constexpr auto TitleScreenStartUp = 4;

		inline constexpr auto TitleScreen = 5;
		inline constexpr auto MainMenu = 8;
		inline constexpr auto Game = 11;
		inline constexpr auto GameAlternate = 12;
		inline constexpr auto PreIngameFmv = 15;
	}



	namespace SavegameChunk
	{
		inline constexpr auto GameAssistant = 0x7832616D;
		inline constexpr auto Animals = 0x78646161;
		inline constexpr auto DelayedActions = 0x7864616D;
		inline constexpr auto AI = 0x78646178;
		inline constexpr auto EmbeddedSavegame = 0x78646573;
		inline constexpr auto Guides = 0x78646778;
		inline constexpr auto Houses = 0x78646861;
		inline constexpr auto Happiness = 0x78646870;
		inline constexpr auto Humans = 0x78646875;
		inline constexpr auto Merchants = 0x78646D61;
		inline constexpr auto MapMoveables = 0x78646D6D;
		inline constexpr auto Missions = 0x78646D78;
		inline constexpr auto Markers = 0x78646E61;
		inline constexpr auto Pathfinders = 0x7864706D;
		inline constexpr auto Tributes = 0x78647478;
		inline constexpr auto Vehicles = 0x78647661;
		inline constexpr auto GameState = 0x7867616D;
		inline constexpr auto GameControl = 0x7867636F;
		inline constexpr auto GameMap = 0x78676D61;
		inline constexpr auto MultiplayerGoals = 0x786D7367;
		inline constexpr auto MissionOnscreenInfo = 0x786F6D69;
		inline constexpr auto PlayerStatistics = 0x7870736D;
	}

	namespace SavegameInfoChunk
	{
		inline constexpr auto MapGuid = 0x49436367;
		inline constexpr auto InfoText = 0x49436973;
		inline constexpr auto MapName = 0x49436D6E;
		inline constexpr auto MultiplayerPlayers = 0x49436D70;
		inline constexpr auto SlotId = 0x49437369;
		inline constexpr auto CreationTime = 0x49437469;
		inline constexpr auto EndInfoHeader = 0x49437878;
	}

	namespace SavegameListType
	{
		inline constexpr auto System = 1;
		inline constexpr auto User = 2;
		inline constexpr auto Autosave = 3;
		inline constexpr auto DeveloperUndo = 4;
		inline constexpr auto DeveloperTemporary = 5;
	}

	namespace SelectionAction
	{
		inline constexpr auto Human = 1;
		inline constexpr auto Animal = 3;
		inline constexpr auto Vehicle = 4;
		inline constexpr auto House = 6;
		inline constexpr auto Guide = 8;
		inline constexpr auto Landscape = 9;
		inline constexpr auto RoadBuildPlace = 10;
		inline constexpr auto WallBuildPlace = 11;
	}

	namespace SelectionType
	{
		inline constexpr auto SingleHuman = 1;
		inline constexpr auto MultipleHumans = 2;
		inline constexpr auto SingleAnimal = 3;
		inline constexpr auto SingleVehicle = 4;
		inline constexpr auto MultipleVehicles = 5;
		inline constexpr auto House = 6;
		inline constexpr auto Guide = 8;
		inline constexpr auto Landscape = 9;
		inline constexpr auto RoadBuildPlace = 10;
		inline constexpr auto WallBuildPlace = 11;
	}

	namespace SoundFxToken
	{
		inline constexpr auto LogicSoundType = 0x6C737479u;
		inline constexpr auto Name = 0x6E616D65u;
		inline constexpr auto SFX = 0x73667864u;
	}

	namespace SpecialDoubleClass
	{
		inline constexpr auto PositiveInfinity = 1;
		inline constexpr auto NegativeInfinity = 2;
		inline constexpr auto QuietNaN = 3;
	}

	namespace StandardFileDescriptor
	{
		inline constexpr auto Input = 0;
		inline constexpr auto InputUnsigned = 0u;
		inline constexpr auto Output = 1;
		inline constexpr auto OutputUnsigned = 1u;
		inline constexpr auto Error = 2;
		inline constexpr auto ErrorUnsigned = 2u;
	}

	namespace StorableObjectId
	{
		inline constexpr auto Memory = 0x3E9;
		inline constexpr auto Bitmap = 0x3F3;
		inline constexpr auto BobManager = 0x3F4;
		inline constexpr auto Font = 0x3F5;
		inline constexpr auto Palette = 0x3F6;
		inline constexpr auto RemapTable = 0x3F7;
		inline constexpr auto StringArray = 0x3FD;
	}

	namespace TargetType
	{
		inline constexpr auto None = 0;

		inline constexpr auto MapPosition = 5;
		inline constexpr auto Human = 1;
		inline constexpr auto Vehicle = 2;
		inline constexpr auto Animal = 3;
		inline constexpr auto House = 4;
		inline constexpr auto Landscape = 6;
	}

	namespace TraderGoodsCheck
	{
		inline constexpr auto Unavailable = 0;
		inline constexpr auto FetchFromHouse = 1;
		inline constexpr auto VisitNextHouse = 2;
		inline constexpr auto FetchExcessFromVehicle = 3;
	}

	namespace TrainingHouseType
	{
		inline constexpr auto School = 38;
		inline constexpr auto Barracks = 39;
	}

	namespace TransitionPointToken
	{
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto Patterngroup = 0x70617467;
	}

	namespace TransitionToken
	{
		inline constexpr auto GfxCoordsA = 0x636F6F61;
		inline constexpr auto GfxCoordsB = 0x636F6F62;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto PointType = 0x706F6974;
		inline constexpr auto Gfxtexturealpha = 0x74657861;
		inline constexpr auto GfxTexture = 0x74657874;
	}

	namespace TrianglePatternToken
	{
		inline constexpr auto BioCanGrowOn = 0x62696F63;
		inline constexpr auto Biocanplanton = 0x62696F70;
		inline constexpr auto DebugColor = 0x64626763;
		inline constexpr auto Housecanbebuildon = 0x68636262;
		inline constexpr auto Humancanwalkon = 0x68756D77;
		inline constexpr auto IsBorder = 0x6973626F;
		inline constexpr auto IsLand = 0x69736C61;
		inline constexpr auto IsWater = 0x69737761;
		inline constexpr auto Debugname = 0x6E616D65;
		inline constexpr auto MoveResistance = 0x72657369;
	}

	namespace TribeTypeToken
	{
		inline constexpr auto AllowEquip = 0x616C6571;
		inline constexpr auto AllowGood = 0x616C676F;
		inline constexpr auto AllowHouse = 0x616C686F;
		inline constexpr auto AllowJob = 0x616C6A6F;
		inline constexpr auto AllowVehicle = 0x616C7665;
		inline constexpr auto ToProduceGoodNeedHouse = 0x66676E68;
		inline constexpr auto ToBuildHouseNeedGood = 0x66686E67;
		inline constexpr auto ToBuildHouseNeedJob = 0x66686E6A;
		inline constexpr auto ToBuildVehicleNeedGood = 0x66766E67;
		inline constexpr auto ToBuildVehicleNeedJob = 0x66766E6A;
		inline constexpr auto JobDefaultGood = 0x6A64676F;
		inline constexpr auto JobEnablesEquipment = 0x6A656571;
		inline constexpr auto JobEnablesGood = 0x6A65676F;
		inline constexpr auto JobEnablesHouse = 0x6A65686F;
		inline constexpr auto JobEnablesJob = 0x6A656A6F;
		inline constexpr auto JobEnablesVehicle = 0x6A657665;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto NeedForArmor = 0x6E666172;
		inline constexpr auto NeedForEquipment = 0x6E666571;
		inline constexpr auto NeedForGood = 0x6E66676F;
		inline constexpr auto NeedForHouse = 0x6E66686F;
		inline constexpr auto NeedForJob = 0x6E666A6F;
		inline constexpr auto NeedForVehicle = 0x6E667665;
		inline constexpr auto NeedForWeapon = 0x6E667765;
		inline constexpr auto SetAtomic = 0x7361746F;
		inline constexpr auto TrainForGood = 0x7472676F;
		inline constexpr auto TrainForJob = 0x74726A6F;
	}

	namespace UserCommand
	{
		inline constexpr auto Stop = 3;
		inline constexpr auto GotoAndWait = 4;
		inline constexpr auto GoAway = 5;
		inline constexpr auto GoHome = 6;
		inline constexpr auto Eat = 7;
		inline constexpr auto Sleep = 8;
		inline constexpr auto Talk = 9;
		inline constexpr auto Pray = 10;
		inline constexpr auto Goodie = 11;
		inline constexpr auto Cheer = 12;
		inline constexpr auto Marry = 13;
		inline constexpr auto MarryAlternate = 14;
		inline constexpr auto GetBoy = 15;
		inline constexpr auto GetGirl = 16;
		inline constexpr auto SetWorkCenter = 17;
		inline constexpr auto OpenChest = 18;
		inline constexpr auto ExploreArea = 19;
		inline constexpr auto ExploreAreaAndUpdateCenter = 20;
		inline constexpr auto ChangeJob = 21;
		inline constexpr auto ChangeProducedGood = 22;
		inline constexpr auto ChangeProduction = 23;
		inline constexpr auto AssignJob = 24;
		inline constexpr auto RestartJobExecution = 25;
		inline constexpr auto AttachHomeHouse = 26;
		inline constexpr auto DetachHomeHouse = 27;
		inline constexpr auto AttachWorkHouse = 28;
		inline constexpr auto DetachWorkHouse = 29;
		inline constexpr auto AttachBuildHouse = 30;
		inline constexpr auto DetachBuildHouse = 31;
		inline constexpr auto AttachLearnHouse = 32;
		inline constexpr auto DetachLearnHouse = 33;
		inline constexpr auto LearnJob = 34;
		inline constexpr auto LearnGood = 35;
		inline constexpr auto AttachTradeHouse = 36;
		inline constexpr auto DetachTradeHouse = 37;
		inline constexpr auto AttachVehicle = 38;
		inline constexpr auto DetachVehicle = 39;
		inline constexpr auto MoveInsideVehicle = 40;
		inline constexpr auto SetMilitaryMode = 41;
		inline constexpr auto SetMilitaryWorldRefreshmentFlag = 42;
		inline constexpr auto AttackHuman = 43;
		inline constexpr auto AttackHouse = 44;
		inline constexpr auto AttackAnimal = 45;
		inline constexpr auto AttackVehicle = 46;
		inline constexpr auto AttackMapPosition = 47;
		inline constexpr auto StartAttack = 48;
		inline constexpr auto ResetTraderImportFlags = 49;
		inline constexpr auto ConfigureTradeGood = 50;
		inline constexpr auto SelectTradeAgreement = 51;
		inline constexpr auto DestroyHouse = 0x5A;
		inline constexpr auto StartHouseUpgrade = 0x5B;
		inline constexpr auto StopHouseUpgrade = 0x5C;
		inline constexpr auto SetHomeQualityEnhancerAllowed = 0x5D;
		inline constexpr auto SetHouseProductionStopped = 0x5E;
		inline constexpr auto ChangeHouseProductionCounter = 0x5F;
		inline constexpr auto SetHouseStockMinimum = 0x60;
		inline constexpr auto SetHouseDefenceMode = 0x61;
		inline constexpr auto VehicleGotoAndWait = 0x65;
		inline constexpr auto VehicleUnload = 0x66;
		inline constexpr auto VehicleDock = 0x67;
		inline constexpr auto VehicleAttackHuman = 0x68;
		inline constexpr auto VehicleAttackHouse = 0x69;
		inline constexpr auto VehicleAttackAnimal = 0x6A;
		inline constexpr auto VehicleAttackVehicle = 0x6B;
		inline constexpr auto VehicleAttackMapPosition = 0x6C;
		inline constexpr auto SetVehicleWantedAmount = 0x6D;
		inline constexpr auto ResetVehicleWantedAmounts = 0x6E;
		inline constexpr auto SetVehicleMilitaryMode = 0x6F;
		inline constexpr auto VehicleInterrupt = 0x70;
		inline constexpr auto VehicleAttachToCarrier = 0x71;
		inline constexpr auto VehicleDetachFromCarrier = 0x72;
		inline constexpr auto VehicleMoveIntoCarrier = 0x73;
		inline constexpr auto PlaceBuildItem = 0x75;
		inline constexpr auto BuildRoad = 0x76;
		inline constexpr auto ClearRoadBuildArea = 0x77;
		inline constexpr auto BuildWall = 0x78;
		inline constexpr auto ClearWallBuildArea = 0x79;
		inline constexpr auto RemoveWallGate = 0x7A;
		inline constexpr auto SetWallGate = 0x7B;
		inline constexpr auto SetWallGateState = 0x7C;
		inline constexpr auto InsertGuide = 0x7D;
		inline constexpr auto RemoveGuide = 0x7E;
		inline constexpr auto SetDiplomacy = 0x7F;
		inline constexpr auto PayTribute = 0x80;
		inline constexpr auto EquipShoesForPlayer = 0x81;
		inline constexpr auto EquipWoodenToolsForPlayer = 0x82;
		inline constexpr auto EquipIronToolsForPlayer = 0x83;
		inline constexpr auto EquipMeadForPlayer = 0x84;
		inline constexpr auto GameAssistantInput = 0x85;
		inline constexpr auto SetGlobalDefendMode = 0x86;
		inline constexpr auto SetGameSpeed = 0x87;
		inline constexpr auto PlayerLeavesGame = 0x88;
		inline constexpr auto ModifyObjectsAfterPlayerLeaves = 0x89;
		inline constexpr auto SubmitGameChecksum = 0x8A;
		inline constexpr auto SetNetworkFrequency = 0x8B;
	}

	namespace UserMessage
	{
		inline constexpr auto NoWork = 2;
		inline constexpr auto NoWay = 3;
		inline constexpr auto CannotFindGood = 4;
		inline constexpr auto CannotFindBuildGood = 5;
		inline constexpr auto CannotFindHomeGood = 6;
		inline constexpr auto NoTargetHuman = 7;
		inline constexpr auto NoWorkPoint = 8;
		inline constexpr auto NoVehicleBuildPlace = 9;
		inline constexpr auto VehicleBuildPlaceOccupied = 10;
		inline constexpr auto TaskCannotBeCompleted = 11;
		inline constexpr auto MustWaitForGood = 12;
		inline constexpr auto ProductionStockFull = 13;
		inline constexpr auto CannotAffordTraining = 14;
		inline constexpr auto VehicleRequired = 15;
		inline constexpr auto TradeAgreementRequired = 16;
		inline constexpr auto ExperienceUnlocks = 18;
		inline constexpr auto GoodProductionUnlocked = 19;
		inline constexpr auto JobUnlocked = 20;
		inline constexpr auto HouseUnlocked = 21;
		inline constexpr auto VehicleUnlocked = 22;
		inline constexpr auto EquipmentUnlocked = 23;
		inline constexpr auto ProductionAmountCompleted = 24;
		inline constexpr auto CannotProduceWantedGood = 26;
		inline constexpr auto Hungry = 28;
		inline constexpr auto Tired = 29;
		inline constexpr auto Bored = 30;
		inline constexpr auto WantsToPray = 31;
		inline constexpr auto Starving = 32;
		inline constexpr auto WillDieSoon = 33;
		inline constexpr auto NoMarriagePartner = 39;
		inline constexpr auto NoWayToMarriagePartner = 40;
		inline constexpr auto CannotEnterVehicle = 43;
		inline constexpr auto CannotFindEquipment = 44;
		inline constexpr auto BackpackFull = 45;
		inline constexpr auto HouseFinished = 0x2F;
		inline constexpr auto HouseUpgradeFinished = 0x30;
		inline constexpr auto HouseAttacked = 0x31;
		inline constexpr auto VehicleNoWay = 50;
		inline constexpr auto VehicleNoCommander = 51;
		inline constexpr auto VehicleAttacked = 52;
		inline constexpr auto VehicleNoDraggingAnimal = 53;
		inline constexpr auto VehicleNoPassengerSpace = 54;
		inline constexpr auto VehicleCannotAttach = 55;
		inline constexpr auto VehicleCannotReachShip = 56;
		inline constexpr auto VehicleCannotDetach = 57;
		inline constexpr auto VehicleNeedsCarrier = 58;
		inline constexpr auto HeroDied = 0x3B;
		inline constexpr auto PlayerSeen = 0x3C;
		inline constexpr auto DiplomacyChanged = 0x3D;
		inline constexpr auto PlayerDied = 0x3E;
		inline constexpr auto SpecialItemFound = 0x3F;
	}

	namespace VehicleDoorTurn
	{
		inline constexpr auto Straight = 0;
		inline constexpr auto ClockwiseStep = 1;
		inline constexpr auto ClockwiseTwoSteps = 2;
		inline constexpr auto Opposite = 3;
		inline constexpr auto CounterclockwiseTwoSteps = 4;
		inline constexpr auto CounterclockwiseStep = 5;
	}

	namespace VehicleGuiAction
	{
		inline constexpr auto GotoAndWait = 1;
		inline constexpr auto SetWorkCenter = 2;
		inline constexpr auto Stop = 3;
		inline constexpr auto AttackHuman = 5;
		inline constexpr auto AttackHouse = 6;
		inline constexpr auto AttackVehicle = 7;
		inline constexpr auto AttackMapPosition = 8;
		inline constexpr auto MilitaryAggressive = 9;
		inline constexpr auto MilitaryDefensive = 10;
		inline constexpr auto MilitaryIgnore = 11;
		inline constexpr auto DetachVehicle = 13;
		inline constexpr auto ResetWantedAmounts = 14;
	}

	namespace VehicleMilitaryMode
	{
		inline constexpr auto SearchAroundPosition = 1;
		inline constexpr auto SearchAroundTarget = 2;
		inline constexpr auto SearchWithinWeaponRange = 3;
	}

	namespace VehicleTask
	{
		inline constexpr auto Idle = 0;

		inline constexpr auto Dock = 1;
		inline constexpr auto Attack = 2;
		inline constexpr auto WaitForPassengers = 3;
		inline constexpr auto GetDraggingAnimal = 4;
		inline constexpr auto Interrupted = 5;
		inline constexpr auto AttachToVehicle = 6;
	}

	namespace VehicleType
	{
		inline constexpr auto Handcart = 1;
		inline constexpr auto Oxcart = 2;
		inline constexpr auto SmallShip = 3;
		inline constexpr auto LargeShip = 4;
		inline constexpr auto Catapult = 5;
		inline constexpr auto OxcartFrame = 6;
	}

	namespace VehicleTypeToken
	{
		inline constexpr auto DebugColor = 0x64626763;
		inline constexpr auto DebugRadius = 0x64726164;
		inline constexpr auto LogicDraggingAnimalTribe = 0x6C646174;
		inline constexpr auto LogicCommander = 0x6C67636D;
		inline constexpr auto LogicGood = 0x6C67676F;
		inline constexpr auto LogicPassenger = 0x6C677073;
		inline constexpr auto LogicSize = 0x6C73697A;
		inline constexpr auto LogicTransformVehicleType = 0x6C747674;
		inline constexpr auto Name = 0x6E616D65;
		inline constexpr auto PassengerVector = 0x70617376;
		inline constexpr auto PassengerSlots = 0x70736C74;
		inline constexpr auto StockVector = 0x73746F76;
		inline constexpr auto StockSlots = 0x7374736C;
		inline constexpr auto VehicleSlots = 0x76736C74;
	}

	namespace VehicleWindowCommand
	{
		inline constexpr auto Update = 2030;
		inline constexpr auto ToggleSubTab = 10000;
		inline constexpr auto CenterVehicle = 10001;
		inline constexpr auto ShowActionButtons = 10002;
		inline constexpr auto CenterVehicleAndStopFollowing = 10003;
		inline constexpr auto OpenObservation = 10004;
		inline constexpr auto ToggleFollowVehicle = 10005;
		inline constexpr auto StopVehicle = 10006;
		inline constexpr auto ResetWantedAmounts = 10007;
		inline constexpr auto AssignWorkCenter = 10008;
		inline constexpr auto SelectAttachedVehicle = 10009;
		inline constexpr auto AttachVehicle = 10010;
		inline constexpr auto DetachAttachedVehicle = 10011;
		inline constexpr auto SelectHuman = 10012;
		inline constexpr auto SelectHumanAndShowActions = 10013;
		inline constexpr auto SelectVehicle = 10014;
		inline constexpr auto SelectVehicleAndShowActions = 10015;
		inline constexpr auto SelectStockView = 10016;
		inline constexpr auto DecreaseWantedAmount = 10017;
		inline constexpr auto IncreaseWantedAmount = 10018;
		inline constexpr auto SelectTradeHouse = 10019;
		inline constexpr auto AddTradeHouse = 10020;
		inline constexpr auto RemoveTradeHouse = 10021;
		inline constexpr auto OpenTraderConfiguration = 10022;
		inline constexpr auto TraderConfigurationClosed = 10023;
	}

	namespace VirtualKey
	{
		inline constexpr auto Backspace = 0x08;
		inline constexpr auto Return = 0x0D;
		inline constexpr auto Escape = 0x1B;
		inline constexpr auto End = 0x23;
		inline constexpr auto Home = 0x24;
		inline constexpr auto Left = 0x25;
		inline constexpr auto Right = 0x27;
		inline constexpr auto Delete = 0x2E;
	}

	namespace WallLandscapeType
	{
		inline constexpr auto Wall = 82;
		inline constexpr auto GateOpen = 83;
		inline constexpr auto GateClosed = 84;
	}

	namespace WeaponType
	{
		inline constexpr auto Unarmed = 1;
		inline constexpr auto WoodenSpear = 4;
		inline constexpr auto IronSpear = 5;
		inline constexpr auto ShortSword = 7;
		inline constexpr auto LongSword = 8;
		inline constexpr auto ShortSaber = 10;
		inline constexpr auto LongSaber = 11;
		inline constexpr auto SmallAxe = 13;
		inline constexpr auto LargeAxe = 14;
		inline constexpr auto ShortBow = 16;
		inline constexpr auto LongBow = 17;
	}

	namespace WeaponTypeToken
	{
		inline constexpr auto AtomicActionType = 0x61746174u;
		inline constexpr auto Createsmoke = 0x6372736Du;
		inline constexpr auto DamageType = 0x646D6774u;
		inline constexpr auto Damagevalue = 0x646D766Cu;
		inline constexpr auto GoodType = 0x676F6F64u;
		inline constexpr auto Hitself = 0x68697473u;
		inline constexpr auto JobType = 0x6A6F6274u;
		inline constexpr auto Maintype = 0x6D61696Eu;
		inline constexpr auto MaximumRange = 0x6D617872u;
		inline constexpr auto MinimumRange = 0x6D696E72u;
		inline constexpr auto Name = 0x6E616D65u;
		inline constexpr auto MunitionType = 0x7072746Eu;
		inline constexpr auto SoundtypeHit = 0x73686974u;
		inline constexpr auto Smokelifetime = 0x736D6C66u;
		inline constexpr auto SoundtypeNohit = 0x736E6874u;
		inline constexpr auto Speed = 0x73706564u;
		inline constexpr auto Weight = 0x77656967u;
	}

	namespace WindowsException
	{
		inline constexpr auto FloatingPointDenormalOperand = 0xC000008Du;
		inline constexpr auto FloatingPointDivideByZero = 0xC000008Eu;
		inline constexpr auto FloatingPointInexactResult = 0xC000008Fu;
		inline constexpr auto FloatingPointInvalidOperation = 0xC0000090u;
		inline constexpr auto FloatingPointOverflow = 0xC0000091u;
		inline constexpr auto FloatingPointStackCheck = 0xC0000092u;
		inline constexpr auto FloatingPointUnderflow = 0xC0000093u;
	}

	namespace WorldDisplayCommand
	{
		inline constexpr auto AssignDisplayDefinition = 0x7D2;
		inline constexpr auto ToggleDrawFlags = 0x7D3;
		inline constexpr auto ScrollByPixelDelta = 0x7D5;
		inline constexpr auto ToggleZoom = 0x7D7;
	}

	namespace X87PrecisionControl
	{
		inline constexpr auto Single24 = 0;
		inline constexpr auto Single24Unsigned = 0x0000u;
		inline constexpr auto Double53 = 0x200;
		inline constexpr auto Double53Unsigned = 0x0200u;
		inline constexpr auto Extended64 = 0x300;
	}

	namespace X87RoundingControl
	{
		inline constexpr auto Nearest = 0;
		inline constexpr auto Down = 0x400;
		inline constexpr auto Up = 0x800;
		inline constexpr auto TowardZero = 0xC00;
	}

	namespace AIConditionOperation
	{
		inline constexpr auto Xor = 4;

		inline constexpr auto Not = 3;

		inline constexpr auto Or = 2;

		inline constexpr auto And = 1;
	}

	namespace VehicleJob
	{
		inline constexpr auto Catapult = 0x36;
	}

	namespace TextButtonMessage
	{
		inline constexpr auto DoPressedAction = 0x40;

		inline constexpr auto ForwardToParentHandler = 0x100;

		inline constexpr auto Command = 0x800;
	}

	namespace TextButtonCommand
	{
		inline constexpr auto GetPlainCaption = 0x7F1;

		inline constexpr auto GetCommandParameters = 0x7F0;
	}

	namespace MissionInfoCommand
	{
		inline constexpr auto ClearBriefing = 50001;

		inline constexpr auto SetBriefing = 50000;

		inline constexpr auto PageTabLimitUnsigned = 10100u;

		inline constexpr auto FirstPageTabUnsigned = 10000u;

		inline constexpr auto NextBriefing = 0x2775;

		inline constexpr auto PreviousBriefing = 0x2774;

		inline constexpr auto ScrollDown = 0x277F;

		inline constexpr auto ScrollUp = 0x277E;

		inline constexpr auto FindMissionInfoWindow = 0x7E6;

		inline constexpr auto ClearBriefingUnsigned = 50001u;

		inline constexpr auto SetBriefingUnsigned = 50000u;
	}

	namespace MissionInfoMessage
	{
		inline constexpr auto WindowQuery = 0x1000;
	}

	namespace MissionInfoCallbackCommand
	{
		inline constexpr auto CenterOnHouses = 1003;

		inline constexpr auto CenterOnVehicles = 1002;

		inline constexpr auto CenterOnHumans = 1001;

		inline constexpr auto CenterOnCoordinates = 1000;

		inline constexpr auto PlayVoice = 2000;
	}

	namespace MissionInfoPage
	{
		inline constexpr auto History = 2;

		inline constexpr auto Goals = 1;

		inline constexpr auto Briefing = 0;

		inline constexpr auto BriefingUnsigned = 0u;

		inline constexpr auto GoalsUnsigned = 1u;
	}

	namespace ToolWindowBackgroundStyle
	{
		inline constexpr auto Framed = 0;

		inline constexpr auto Papyrus = 1;
	}

	namespace TraderConfigCommand
	{
		inline constexpr auto FrameUpdate = 2030;

		inline constexpr auto CenterOnSecondHouse = 10301;

		inline constexpr auto CenterOnFirstHouse = 10300;

		inline constexpr auto SelectAgreement = 10200;

		inline constexpr auto ToggleGoodImport = 10100;

		inline constexpr auto ClearImports = 10000;
	}

	namespace TitleScreenSkipKey
	{
		inline constexpr auto Escape = 0x1B;

		inline constexpr auto Space = 0x20;
	}

	namespace GuiWindowMessage
	{
		inline constexpr auto Command = 0x800;

		inline constexpr auto Query = 0x1000;
	}

	namespace GuiWindowQuery
	{
		inline constexpr auto ConstructionSelectionWithItem = 0x7DB;

		inline constexpr auto ConstructionSelection = 0x7DA;

		inline constexpr auto HumanLearnJob = 2017;

		inline constexpr auto HumanChangeProducedGood = 2015;

		inline constexpr auto HumanChangeName = 2014;

		inline constexpr auto HumanChangeJob = 2013;

		inline constexpr auto GameSafetyBox = 0x7EB;

		inline constexpr auto Options = 0x7D8;
	}

	namespace GameSafetyBoxCommand
	{
		inline constexpr auto Restart = 0x2712;

		inline constexpr auto Cancel = 0x2711;

		inline constexpr auto Confirm = 0x2710;
	}

	namespace GameSafetyBoxMode
	{
		inline constexpr auto RestartMap = 3;

		inline constexpr auto MapOutcome = 2;

		inline constexpr auto LeaveGame = 1;
	}

	namespace OptionsCommandRange
	{
		inline constexpr auto FirstPageTab = 0x2904;

		inline constexpr auto FirstPageTabUnsigned = 0x2904u;

		inline constexpr auto FirstAutosaveInterval = 0x28A0;

		inline constexpr auto FirstGameSpeed = 0x283C;

		inline constexpr auto FirstResolution = 0x2710;
	}

	namespace DiplomacyState
	{
		inline constexpr auto Enemy = 3;
	}

	namespace SingleHumanWindowCommand
	{
		inline constexpr auto SelectWorkHouse = 10017;

		inline constexpr auto SelectSpouse = 10022;
	}

	namespace SoftwareMouseGraphic
	{
		inline constexpr auto PositionOverrideUnsigned = 183u;

		inline constexpr auto LeftButtonDownUnsigned = 182u;

		inline constexpr auto NormalUnsigned = 181u;
	}

	namespace WorldCursorGraphic
	{
		inline constexpr auto MarkerFirstFrameUnsigned = 0xBAu;
	}

	namespace IniParameterType
	{
		inline constexpr auto Integer = 3;

		inline constexpr auto String = 2;
	}

	namespace MissionStartObjectKind
	{
		inline constexpr auto Animal = 3;

		inline constexpr auto House = 4;

		inline constexpr auto Vehicle = 2;

		inline constexpr auto Human = 1;

		inline constexpr auto None = 0;
	}

	namespace GameStartupMode
	{
		inline constexpr auto SkirmishMap = 5;
		inline constexpr auto Savegame = 2;

		inline constexpr auto SingleMap = 1;

		inline constexpr auto MultiplayerContinue = 4;

		inline constexpr auto MultiplayerMap = 3;

		inline constexpr auto None = 0;
	}

	namespace HumanTaskStatus
	{
		inline constexpr auto Failed = 3;

		inline constexpr auto Running = 1;

		inline constexpr auto PendingStartup = 0;
	}


	namespace HumanJobWindowCommand
	{
		inline constexpr auto ProducedGoodCommandEnd = 11000;

		inline constexpr auto SelectRow = 10000;

		inline constexpr auto FrameUpdate = 2030;

		inline constexpr auto ValidateSelection = 2029;

		inline constexpr auto NameInputKey = 0x8233;
	}

	namespace GameMapType
	{
		inline constexpr auto Multiplayer = 4;
	}

	namespace LandscapeTransition
	{
		inline constexpr auto BioUpdate = 7;

		inline constexpr auto Automatic = 13;
	}

	namespace ContinentType
	{
		inline constexpr auto WaterUnsigned = 2u;

		inline constexpr auto Land = 1;
	}

	namespace ConstructionSelectionCommand
	{
		inline constexpr auto PageLimit = 10100;

		inline constexpr auto NavigateByLetter = 33331;

		inline constexpr auto GameSettings = 10300;

		inline constexpr auto BuildGate = 10202;

		inline constexpr auto BuildRoad = 10200;

		inline constexpr auto GoodHelp = 10102;

		inline constexpr auto HouseHelp = 10101;

		inline constexpr auto SelectHouse = 10100;

		inline constexpr auto FirstPage = 10000;

		inline constexpr auto Refresh = 0x7EE;
	}

	namespace ConstructionSelectionPage
	{
		inline constexpr auto Military = 4;

		inline constexpr auto Home = 3;

		inline constexpr auto Stock = 2;

		inline constexpr auto Work = 1;

		inline constexpr auto All = 0;
	}

}
