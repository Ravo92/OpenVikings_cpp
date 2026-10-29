#pragma once

// Exact NUL-terminated bytes recovered from Game.exe; sorted by original address.
namespace OriginalStrings
{
	// Original data: 004fe164.
	inline constexpr char Dot[4] = ".";

	// Original data: 004fe238.
	inline constexpr char UnknownPlayerName[] = "?";

	// Original data: 004fe2f4 and 004fe300; game-disc identification pair.
	inline constexpr char GameCDVolumeName[] = "weltwunder";
	inline constexpr char GameCDMarkerFile[] = "autorun.exe";

	// Original data: 004fefb0 and 004fefb4; adjacent CD-audio string region.
	inline constexpr char CDAudioDriveNameFormat[] = "%c:";
	inline constexpr char CDAudioErrorPrefix[] = "CCDAudioManager error: ";

	// Original data: 004ff054.
	inline constexpr char WindowsRootPath[4] = "\\";

	// Original data: 004ff058
	inline constexpr char FileOpenForReadingError[] = "NXBasics::CFile::OpenForReading\n\nfile %s - errno %d: %s";

	// Original data: 004ff090
	inline constexpr char FileOpenForWritingError[] = "NXBasics::CFile::OpenForWriting\n\nfile %s - errno %d: %s";

	// Original data: 004ff0c8
	inline constexpr char FileCloseError[] = "NXBasics::CFile::Close\n\nfile %s - errno %d: %s";

	// Original data: 004ff994
	inline constexpr char CannotSaveVirtualBitmap[] = "NXBasics::CBitmap::Storable_SaveData\n\ncannot save virtual bitmap";

	// Original data: 0050071c
	inline constexpr char DirectPlayErrorSeparator[] = " - ";

	// Original data: 00500808.
	inline constexpr char CampaignFileName[] = "campaign.ini";

	// Original data: 00500818.
	inline constexpr char DirectoryAndFileNameFormat[] = "%s\\%s";

	// Original data: 0050089c.
	inline constexpr char CampaignWriteWarning[] = "CCampaignStaticDataManager::l_IO_Save\n\nCould not write campaign ini file!";

	// Original data: 00501044.
	inline constexpr char SystemPlayerFileSystemName[] = "singleplayer";

	// Original data: 0050105c.
	inline constexpr char SaveDirectory[] = "Saves";

	// Original data: 00501064.
	inline constexpr char UserMapDirectory[] = "UserMaps";

	// Original data: 00504f68 and 00504f70; adjacent CD-drive path formats.
	inline constexpr char CDDriveFileNameFormat[] = "%c:\\%s";
	inline constexpr char CDDriveRootNameFormat[] = "%c:\\";

	// Original data: 0050716c.
	inline constexpr char RightArrow[] = ">";

	// Original data: 005087fc
	inline constexpr char NetworkLogTimeFormat[] = "%s ";

	// Original data: 00509630
	inline constexpr char NewLine[] = "\n";

	// Original data: 0050978c
	inline constexpr char NetworkLogIndentation[] = "\t";

	// Original data: 00509910.
	inline constexpr char FileTransferServiceUnavailable[] = "No file transfer service - message ignored\n";

	// Original data: 00509dec
	inline constexpr char NetworkPeerDestroyedLog[] = "DirectPlay peer object destroyed. Cannot re-initialize DirectPlay";

	// Original data: 00509e30
	inline constexpr char NetworkPeerDestroyedError[] = "CDPNet::ReInitialize(): DirectPlay peer object destroyed. Cannot re-initialize DirectPlay.";

	// Original data: 00509e8c
	inline constexpr char NetworkReinitializationFailed[] = "IDirectPlay8Peer-Reinitialization failed";

	// Original data: 00509eb8
	inline constexpr char NetworkInitializeOperation[] = "CDPNet::ReInitialize(): IDirectPlay8Peer::Initialize";

	// Original data: 0050a698
	inline constexpr char NetworkReturnBufferOperation[] = "CDPNet::ReleaseReceivedMessages(): IDirectPlay8Peer::ReturnBuffer";

	// Original data: 0050a9c8
	inline constexpr char TimerTimeoutLabel[] = "\nTimeout [ms]: ";

	// Original data: 0050a9d8
	inline constexpr char TimerThreadFormat[] = "%x";

	// Original data: 0050a9dc
	inline constexpr char TimerThreadLabel[] = "Thread ID: 0x";

	// Original data: 0050a9ec
	inline constexpr char TimerNotRespondingWarning[] = "CDPTimer::~CDPTimer() - Warning: Thread stopped responding.\n";

	// Original data: 0050b9f8.
	inline constexpr char GameOptionsFileNameFormat[] = "%s\\opt_game.ini";

	// Original data: 0050ba08.
	inline constexpr char CallsignOptionsFileNameFormat[] = "%s\\opt_callsign.ini";

	// Original data: 0050ba1c.
	inline constexpr char GlobalOptionsFileNameFormat[] = "%s\\opt_glob.ini";

}