#pragma once

// File markers, compression kinds and parser token IDs used by the Windows game.
// Values and integer literal types are retained from the Windows reconstruction.
namespace NC2DataIds
{
	namespace AIManagerToken
	{
		inline constexpr auto AIMainTaskBuildMilestone = 0x6169626D;
		inline constexpr auto AIMainTaskClearTributes = 0x6169636C;
		inline constexpr auto AISetTributeHireling = 0x61697468;
		inline constexpr auto AIAddTribute = 0x61697472;
	}

	namespace AnimalTypeToken
	{
		inline constexpr auto GetAngry = 0x67616E67u;
		inline constexpr auto IgnoreHouses = 0x6967686Fu;
		inline constexpr auto TribeType = 0x74797065u;
	}

	namespace ArmorTypeToken
	{
		inline constexpr auto Type = 0x74797065u;
	}

	namespace ArrayChunk
	{
		inline constexpr auto Header = 0x78616861u;
	}

	namespace CampaignStaticDataManagerToken
	{
		inline constexpr auto V = 0x00766172;
		inline constexpr auto A = 0x616C6C6F;
	}

	namespace CleanMapsManagerToken
	{
		inline constexpr auto MapDescriptionStringId = 0x6D617064;
		inline constexpr auto MapNameStringId = 0x6D617073;
	}

	namespace CompressedFileHeader
	{
		inline constexpr auto Signature = 0x2E433250u;
	}

	namespace EngineLifetimeToken
	{
		inline constexpr auto Generic = 0x67656E65u;
		inline constexpr auto Respond = 0x72657370u;
		inline constexpr auto Scream = 0x73637265u;
		inline constexpr auto SetRainRectangle = 0x73727265u;
		inline constexpr auto SetSandRectangle = 0x7373616Eu;
		inline constexpr auto SetSnowRectangle = 0x73737265u;
		inline constexpr auto LogicTribe = 0x74726962u;
	}

	namespace EngineMacroMapsToken
	{
		inline constexpr auto Version = 0x766572;
		inline constexpr auto Animal = 0x616E696D;
		inline constexpr auto Center = 0x63656E74;
		inline constexpr auto FishSwarm = 0x66697368;
		inline constexpr auto Hgt = 0x68656967;
		inline constexpr auto House = 0x686F7573;
		inline constexpr auto Human = 0x68756D61;
		inline constexpr auto Land = 0x6C616E64;
		inline constexpr auto LandName = 0x6C616E6E;
		inline constexpr auto Pat = 0x70617464;
		inline constexpr auto Patname = 0x7061746E;
		inline constexpr auto Road = 0x726F6164;
		inline constexpr auto Size = 0x73697A65;
		inline constexpr auto Trans = 0x74726164;
		inline constexpr auto Transname = 0x7472616E;
		inline constexpr auto Veco = 0x7665636F;
	}

	namespace EngineMapChunk
	{
		inline constexpr auto MapHeader = 0x656D6D6D;
	}

	namespace EngineSavegameChunk
	{
		inline constexpr auto EngineHeader = 0x58453244;
	}

	namespace GameControlToken
	{
		inline constexpr auto Add = 0x61646469;
		inline constexpr auto AllowGood = 0x61676F6F;
		inline constexpr auto AllowHouse = 0x61686F75;
		inline constexpr auto AllowJob = 0x616A6F62;
		inline constexpr auto Diplomacy = 0x6469706C;
		inline constexpr auto ForbidGood = 0x66676F6F;
		inline constexpr auto ForbidHouse = 0x66686F75;
		inline constexpr auto ForbidJob = 0x666A6F62;
		inline constexpr auto NameTribe = 0x6E616D74;
		inline constexpr auto NameTribeShort = 0x6E616D75;
		inline constexpr auto NoSeenFirstMessage = 0x6E73666D;
		inline constexpr auto PlayerFixColors = 0x7066636F;
		inline constexpr auto PlayerHideInMenu = 0x7068696D;
		inline constexpr auto Player = 0x706C6179;
		inline constexpr auto PlayerOption = 0x706C6F70;
		inline constexpr auto PlayerNeverDies = 0x706E6469;
		inline constexpr auto RelationHide = 0x72656C68;
		inline constexpr auto RelationNotChangeable = 0x72656C6E;
		inline constexpr auto RelationHideDetails = 0x72656C73;
	}

	namespace GoodToken
	{
		inline constexpr auto Type = 0x74797065;
	}

	namespace GuiSavegameChunk
	{
		inline constexpr auto GuiHeader = 0x58475549;
	}

	namespace HouseToken
	{
		inline constexpr auto LogicType = 0x6C6F7479;
	}

	namespace HumanJobExperienceTypeToken
	{
		inline constexpr auto Type = 0x74797065u;
	}

	namespace HumanMerchantToken
	{
		inline constexpr auto TradeAgreement = 0x706E616D;
	}

	namespace HumanMiscToken
	{
		inline constexpr auto GraphicsHumanRandomPalette = 0x67726872;
		inline constexpr auto LogicGood = 0x6C6F6764;
		inline constexpr auto Patch = 0x70617463;
	}

	namespace HyperLinkTextSystemToken
	{
		inline constexpr auto Rem = 0x72656D;
		inline constexpr auto Block = 0x626C636B;
		inline constexpr auto GlobalJump = 0x676A6D70;
		inline constexpr auto LocalJump = 0x6C6A6D70;
		inline constexpr auto OnScreenCallback = 0x6F736362;
		inline constexpr auto UserIcon = 0x7569636F;
		inline constexpr auto UserPicture = 0x75706963;
		inline constexpr auto TextXEnd = 0x78656E64;
		inline constexpr auto TextXStart = 0x78737461;
	}

	namespace IoChunk
	{
		inline constexpr auto End = 0x78656E64u;
		inline constexpr auto EndSigned = 0x78656E64;
		inline constexpr auto HeaderSignature = 0x78696F68u;
		inline constexpr auto HeaderSignatureSigned = 0x78696F68;
	}

	namespace JobTypeToken
	{
		inline constexpr auto Type = 0x74797065u;
	}

	namespace LandscapeToken
	{
		inline constexpr auto Type = 0x74797065;
	}

	namespace LogicMapChunk
	{
		inline constexpr auto Continents = 0x6C61636F;
		inline constexpr auto MapData = 0x6C676D6D;
		inline constexpr auto AttachedObjects = 0x6C6D616F;
		inline constexpr auto MicroBuildingBlocked = 0x6C6D6262;
		inline constexpr auto MicroContinent = 0x6C6D636F;
		inline constexpr auto MacroHeight = 0x6C6D6865;
		inline constexpr auto MicroHouseBuildForbidden = 0x6C6D6866;
		inline constexpr auto LandscapePlayer = 0x6C6D6C70;
		inline constexpr auto LandscapeType = 0x6C6D6C74;
		inline constexpr auto LandscapeValency = 0x6C6D6C76;
		inline constexpr auto MicroMaximumMoveableSize = 0x6C6D6D73;
		inline constexpr auto MacroPatternA = 0x6C6D7061;
		inline constexpr auto MacroPatternB = 0x6C6D7062;
		inline constexpr auto MicroPathResistance = 0x6C6D7072;
		inline constexpr auto MicroRoad = 0x6C6D726F;
		inline constexpr auto MicroSectorBasePoint = 0x6C6D7362;
		inline constexpr auto MicroWalkConnections = 0x6C6D7477;
		inline constexpr auto MicroWalkingBlocked = 0x6C6D7762;
		inline constexpr auto LogicFileStart = 0x6C6F6769;
		inline constexpr auto MapSize = 0x6C73697A;
		inline constexpr auto SectorAttachments = 0x7361616D;
		inline constexpr auto BioRegrowth = 0x73616772;
		inline constexpr auto AutoTransitions = 0x7361746D;
		inline constexpr auto FishManager = 0x7366696D;
		inline constexpr auto Exploration = 0x736D6578;
		inline constexpr auto FirstMapMoveableId = 0x736D666D;
		inline constexpr auto Guides = 0x736D6775;
		inline constexpr auto Houses = 0x736D686F;
		inline constexpr auto RoadBuildManager = 0x73726F64;
		inline constexpr auto WallBuildManager = 0x73726F77;
		inline constexpr auto LogicFileEnd = 0x74656E64;
	}

	namespace MapMoveablesToken
	{
		inline constexpr auto Seq = 0x736571;
		inline constexpr auto GraphicsAnimationFrameList = 0x67667866;
		inline constexpr auto GraphicsWalkFrameList = 0x67667866;
	}

	namespace MissionStartObjectsToken
	{
		inline constexpr auto AllowJob = 0x616A6F62;
		inline constexpr auto ForbidJob = 0x666A6F62;
		inline constexpr auto SetName = 0x7365746E;
		inline constexpr auto SetPalette = 0x73657470;
	}

	namespace MultiPlayerGoalToken
	{
		inline constexpr auto GoalWonWhenGoods = 0x6777676F;
		inline constexpr auto GoalWonWhenInhabitants = 0x6777696E;
		inline constexpr auto GoalLostByMission = 0x67776C6D;
		inline constexpr auto GoalWonByMission = 0x6777776D;
	}

	namespace PackingHeader
	{
		inline constexpr auto Signature = 0x5870636Bu;
		inline constexpr auto SignatureSigned = 0x5870636B;
	}

	namespace PackingType
	{
		inline constexpr auto None = 0x6E6F6E65;
		inline constexpr auto Rle16 = 0x726C6536;
		inline constexpr auto Rle16Unsigned = 0x726C6536u;
		inline constexpr auto Rle8 = 0x726C6538;
		inline constexpr auto Rle8Unsigned = 0x726C6538u;
	}

	namespace ParticleGeneratorToken
	{
		inline constexpr auto Generate = 0x67656E65u;
	}

	namespace ParticlesToken
	{
		inline constexpr auto LogicType = 0x6C747970;
	}

	namespace PlayerCenterPositionToken
	{
		inline constexpr auto StartPosition = 0x73746170;
	}

	namespace SavegameChunk
	{
		inline constexpr auto Info = 0x78736769;
		inline constexpr auto Header = 0x78736776;
	}

	namespace SavegameInfoChunk
	{
		inline constexpr auto EndMetadata = 0x4943656E;
	}

	namespace TrianglePatternToken
	{
		inline constexpr auto BioCanGrowOn = 0x62696F63;
		inline constexpr auto Type = 0x74797065;
	}

	namespace TribeToken
	{
		inline constexpr auto Type = 0x74797065;
	}

	namespace VehicleToken
	{
		inline constexpr auto LogicTransformVehicleType = 0x6C747674;
		inline constexpr auto Type = 0x74797065;
	}

	namespace WaveTag
	{
		inline constexpr auto Format = 0x20746D66u;
		inline constexpr auto Wave = 0x45564157u;
		inline constexpr auto Riff = 0x46464952u;
		inline constexpr auto Data = 0x61746164u;
	}

	namespace WeaponTypeToken
	{
		inline constexpr auto TribeType = 0x74727479u;
		inline constexpr auto Type = 0x74797065u;
	}

	namespace ZeroByteDetection
	{
		inline constexpr auto AdditionBias = 0x7EFEFEFFu;
		inline constexpr auto CandidateMask = 0x81010100u;
	}

}