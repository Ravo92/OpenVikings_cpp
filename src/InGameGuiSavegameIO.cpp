#include "C2DataIds.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameState.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "InGameGuiManager.h"
#include "InGameGuiMissionInfoWindow.h"
#include "InGameGuiSupportAccess.h"
#include "InGameGuiWindowState.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"

#include <cstring>

namespace NC2InGameGuiMiscWindows
{
	// Original function: FUN_004b1e0f.
	static void StatisticsWindow_IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::StatisticsWindow, 1);
		io->IO_Memory_WriteLong(NC2Lifetime::Read<std::uint32_t>(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::Page));
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004b1e38.
	static void StatisticsWindow_IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.marker == SwitchIds::GuiSavegameChunk::StatisticsWindow && io->mReadHeader.version == 1 && !NNetwork::Network_IsInSession())
		{
			NC2Lifetime::Write(gStatisticsWindowStatics, NC2Layout::StatisticsWindowStateOffset::Page, io->IO_File_ReadLong());
		}
		else
		{
			StatisticsWindow_Statics_Init();
			io->IO_File_Chunk_Skip();
		}
	}

	// Original function: FUN_004b00f0.
	static void HumanListWindow_IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::HumanListWindow, 1);
		io->IO_Memory_WriteBinary(gHumanListWindowStatics, NC2Layout::RecordSize::HumanListWindowState);
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004b011a.
	static void HumanListWindow_IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.marker == SwitchIds::GuiSavegameChunk::HumanListWindow && io->mReadHeader.version == 1 && !NNetwork::Network_IsInSession())
		{
			io->IO_File_ReadBinary(gHumanListWindowStatics, NC2Layout::RecordSize::HumanListWindowState);
		}
		else
		{
			HumanListWindow_Statics_Init();
			io->IO_File_Chunk_Skip();
		}
	}

	// Original function: FUN_004af070.
	static void DiplomacyWindow_IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::DiplomacyWindow, 1);
		io->IO_Memory_WriteLong(NC2Lifetime::Read<std::uint32_t>(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page));
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004af099.
	static void DiplomacyWindow_IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.marker == SwitchIds::GuiSavegameChunk::DiplomacyWindow && io->mReadHeader.version == 1 && !NNetwork::Network_IsInSession())
		{
			NC2Lifetime::Write(gDiplomacyWindowStatics, NC2Layout::DiplomacyWindowStateOffset::Page, io->IO_File_ReadLong());
		}
		else
		{
			std::memset(gDiplomacyWindowStatics, 0, 4);
			io->IO_File_Chunk_Skip();
		}
	}
}

namespace NC2InGameGuiGroups
{
	// Original function: FUN_004a3251.
	void CObjectGroupManager::IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::ObjectGroups, 2);
		io->IO_Memory_WriteBinary(this, NC2Layout::ObjectSize::ObjectGroupManager);
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004a3280; network loading deliberately discards saved groups.
	void CObjectGroupManager::IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.marker == SwitchIds::GuiSavegameChunk::ObjectGroups && io->mReadHeader.version == 2)
		{
			if (NC2Logic::gGameStaticVars.networkGame == 0)
			{
				io->IO_File_ReadBinary(this, NC2Layout::ObjectSize::ObjectGroupManager);

				return;
			}

			std::memset(this, 0, NC2Layout::ObjectSize::ObjectGroupManager);
		}

		io->IO_File_Chunk_Skip();
	}
}

namespace NC2InGameGuiDeadHumanDataManager
{
	// Original function: FUN_004b3818; owned buffer is serialized separately from its pointer.
	void CDeadHumanDataManager::IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::DeadHumanData, 1);
		io->IO_Memory_WriteBinary(this, NC2Layout::RecordSize::DeadHumanStatePrefix);
		io->IO_Memory_WriteBinary(NC2Lifetime::Read<void*>(this, NC2Layout::DeadHumanManagerOffset::Records), NC2Layout::RecordSize::DeadHumanPool);
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_004b3853.
	void CDeadHumanDataManager::IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		if (io->mReadHeader.marker == SwitchIds::GuiSavegameChunk::DeadHumanData && io->mReadHeader.version == 1)
		{
			io->IO_File_ReadBinary(this, NC2Layout::RecordSize::DeadHumanStatePrefix);
			io->IO_File_ReadBinary(NC2Lifetime::Read<void*>(this, NC2Layout::DeadHumanManagerOffset::Records), NC2Layout::RecordSize::DeadHumanPool);
		}
		else
		{
			io->IO_File_Chunk_Skip();
			std::memset(this, 0, NC2Layout::RecordSize::DeadHumanStatePrefix);
		}
	}
}

namespace NInGameGuiManager
{
	using namespace NC2Lifetime;
	using namespace NC2InGameGuiMiscWindows;

	// Original function: FUN_004c9c3b; preserve child-chunk order inside XGUI.
	void CInGameGuiManager::l_IO_Savegame_Save(NC2Logic::CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(NC2DataIds::GuiSavegameChunk::GuiHeader, 1);
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::DisplayPosition, 1);
		std::uint32_t mapPoint = 0;
		NC2E2::DE_Misc_GetDisplayedCenterMapPosition(Read<void*>(sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), &mapPoint);
		io->IO_Memory_WriteLong(mapPoint);
		io->IO_Tool_Chunk_FinishAndWrite();
		StatisticsWindow_IO_Savegame_Save(io);
		HumanListWindow_IO_Savegame_Save(io);
		DiplomacyWindow_IO_Savegame_Save(io);
		static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->IO_Savegame_Save(io);
		static_cast<NC2InGameGuiGroups::CObjectGroupManager*>(NC2InGameGuiGroups::gObjectGroupManager)->IO_Savegame_Save(io);
		static_cast<NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager*>(NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager)->IO_Savegame_Save(io);
		MissionInfoWindow_MissionBriefingHistory_IO_Savegame_Save(io);
		io->IO_Tool_Chunk_StartCreation(SwitchIds::GuiSavegameChunk::MessagePriority, 1);
		io->IO_Memory_WriteLong(Read<std::uint32_t>(NC2InGameGuiMessages::gMessageManager, NC2Layout::GuiMessageManagerOffset::MinimumPriority));
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_WriteEndChunk();
	}

	// Original function: FUN_004c9d08; skip unrecognized children without losing following chunks.
	void CInGameGuiManager::l_IO_Savegame_Load(NC2Logic::CIoHelper* io)
	{
		std::uint32_t version = 0;

		if (!io->IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::GuiSavegameChunk::GuiHeader, &version, nullptr))
		{
			io->IO_File_Chunk_Skip();

			return;
		}

		while (io->IO_File_Chunk_ReadHeader())
		{
			switch (io->mReadHeader.marker)
			{
				case SwitchIds::GuiSavegameChunk::DisplayPosition:
				{
					const auto point = io->IO_File_ReadLong();
					NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(sTheObjectPtr->GetDesktop(), NC2Layout::WorldGuiElementOffset::Display), &point);
					break;
				}
				case SwitchIds::GuiSavegameChunk::StatisticsWindow:
					StatisticsWindow_IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::HumanListWindow:
					HumanListWindow_IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::DiplomacyWindow:
					DiplomacyWindow_IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::Messages:
					static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::ObjectGroups:
					static_cast<NC2InGameGuiGroups::CObjectGroupManager*>(NC2InGameGuiGroups::gObjectGroupManager)->IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::DeadHumanData:
					static_cast<NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager*>(NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager)->IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::MissionBriefingHistory:
					MissionInfoWindow_MissionBriefingHistory_IO_Savegame_Load(io);
					break;
				case SwitchIds::GuiSavegameChunk::MessagePriority:
					static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->MessagePriority_Set(io->IO_File_ReadLong());
					break;
				default:
					io->IO_File_Chunk_Skip();
					break;
			}
		}
	}
}