#include "C2CampaignStaticDataManager.h"
#include "C2DataIds.h"
#include "C2Debug.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicIOAccess.h"
#include "OriginalStrings.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XIniFileReader.h"

#include <cstring>

namespace NC2Logic
{
	namespace
	{
		// Complete original token table: 005007E8..005007FF.
		constexpr NXBasics::SToken campaignTokens[] = { { "a", NC2DataIds::CampaignStaticDataManagerToken::A }, { "v", NC2DataIds::CampaignStaticDataManagerToken::V }, { nullptr, 0 } };
	}

	// Original function: FUN_00416c1e
	CCampaignStaticDataManager::CCampaignStaticDataManager()
	{
		static_assert(offsetof(CCampaignStaticDataManager, mFileName) == 0x104);
		static_assert(offsetof(CCampaignStaticDataManager, mSubMissionCount) == 0x208);
		static_assert(offsetof(CCampaignStaticDataManager, mSubMissions) == 0x20C);
		static_assert(offsetof(CCampaignStaticDataManager, mVariableCount) == 0x210);
		static_assert(offsetof(CCampaignStaticDataManager, mVariables) == 0x214);
		gCampaignStaticDataManager = this;
		std::memset(this, 0, sizeof(*this));
		// The Windows calls obtain the player name before the data directory.
		const char* playerName = Player_GetSystemPlayerFileSystemName();
		const char* directory = IO_Tool_GetPlayerDataDirectoryNamePtr();
		NXBasics::Runtime_PrintString(mDataDirectory, OriginalStrings::DirectoryAndFileNameFormat, directory, playerName);
		NXBasics::Runtime_PrintString(mFileName, OriginalStrings::DirectoryAndFileNameFormat, mDataDirectory, OriginalStrings::CampaignFileName);
		l_IO_Load();

		for (std::uint32_t campaign = 1; campaign <= 10; ++campaign)
		{
			CampaignSubMission_DoAllow(campaign, 10, false);
		}
	}

	// Original function: FUN_00416c8a; previously reconstructed in the lifetime module.
	CCampaignStaticDataManager::~CCampaignStaticDataManager()
	{
		NXBasics::Runtime_Free(mSubMissions);
		NXBasics::Runtime_Free(mVariables);
		gCampaignStaticDataManager = nullptr;
	}

	// Original function: FUN_00416cae
	void CCampaignStaticDataManager::CampaignSubMission_DoAllow(std::uint32_t campaignId, std::uint32_t subMissionId, bool save)
	{
		const std::uint32_t count = mSubMissionCount;

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (mSubMissions[index].campaignId == campaignId && mSubMissions[index].subMissionId == subMissionId)
			{

				return;
			}
		}

		if (count != 0)
		{
			auto* replacement = static_cast<SCampaignSubMission*>(NXBasics::Runtime_AllocateWithNewHandler(count * 8u + 8u, 1));
			std::memcpy(replacement, mSubMissions, mSubMissionCount * 8u);
			NXBasics::Runtime_Free(mSubMissions);
			++mSubMissionCount;
			mSubMissions = replacement;
		}
		else
		{
			mSubMissionCount = 1;
			mSubMissions = static_cast<SCampaignSubMission*>(NXBasics::Runtime_AllocateWithNewHandler(8, 1));
		}

		SCampaignSubMission* entry = mSubMissions + mSubMissionCount - 1;
		entry->campaignId = campaignId;
		entry->subMissionId = subMissionId;

		if (save)
			l_IO_Save();
	}

	// Original function: FUN_00416d6c
	bool CCampaignStaticDataManager::CampaignSubMission_IsAllowed(std::uint32_t campaignId, std::uint32_t subMissionId) const
	{
		const std::uint32_t count = mSubMissionCount;

		for (std::uint32_t index = 0; index < count; ++index)
		{
			if (mSubMissions[index].campaignId == campaignId && mSubMissions[index].subMissionId == subMissionId)
				return true;
		}

		return false;
	}

	// Original function: FUN_00416da2
	bool CCampaignStaticDataManager::l_IO_Load()
	{
		NXBasics::CIniFileReader reader(mFileName, nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return false;

		if (reader.SearchGroup("campaign_sub_missions"))
		{
			mSubMissionCount = reader.GetNumberOfCommandsInGroup("a");

			if (mSubMissionCount != 0)
			{
				mSubMissions = static_cast<SCampaignSubMission*>(NXBasics::Runtime_AllocateWithNewHandler(mSubMissionCount * 8u, 1));
				std::memset(mSubMissions, 0, mSubMissionCount * 8u);
				std::uint32_t offset = 0;

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if (NXBasics::XB_Token_FindTokenInArray(campaignTokens, reader.GetCurrentName()) != NC2DataIds::CampaignStaticDataManagerToken::A)
						continue;

					auto* entry = reinterpret_cast<SCampaignSubMission*>(reinterpret_cast<std::byte*>(mSubMissions) + offset);
					entry->campaignId = static_cast<std::uint32_t>(reader.GetInt());
					entry->subMissionId = static_cast<std::uint32_t>(reader.GetInt());
					offset += 8;
				}
			}
		}

		if (reader.SearchGroup("campaign_variables"))
		{
			mVariableCount = reader.GetNumberOfCommandsInGroup("v");

			if (mVariableCount != 0)
			{
				mVariables = static_cast<SCampaignVariable*>(NXBasics::Runtime_AllocateWithNewHandler(mVariableCount * 12u, 1));
				std::memset(mVariables, 0, mVariableCount * 12u);
				std::uint32_t offset = 0;

				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if (NXBasics::XB_Token_FindTokenInArray(campaignTokens, reader.GetCurrentName()) != NC2DataIds::CampaignStaticDataManagerToken::V)
						continue;

					auto* entry = reinterpret_cast<SCampaignVariable*>(reinterpret_cast<std::byte*>(mVariables) + offset);
					entry->campaignId = static_cast<std::uint32_t>(reader.GetInt());
					entry->variableId = static_cast<std::uint32_t>(reader.GetInt());
					entry->value = static_cast<std::uint32_t>(reader.GetInt());
					offset += 12;
				}
			}
		}

		return true;
	}

	// Original function: FUN_00416f66
	void CCampaignStaticDataManager::l_IO_Save() const
	{
		NXBasics::CFile::FileSystem_Tool_CreateDirectory(mDataDirectory);
		NXBasics::CFile file(mFileName, false);

		if (!file.OpenForWriting(false))
		{
			C2Debug_WarningBoxV(OriginalStrings::CampaignWriteWarning);

			return;
		}

		char line[100];

		if (mSubMissionCount != 0)
		{
			file.WriteString("[campaign_sub_missions]\r\n");

			for (std::uint32_t index = 0; index < mSubMissionCount; ++index)
			{
				const SCampaignSubMission* entry = mSubMissions + index;
				NXBasics::Runtime_PrintString(line, "a %d %d\r\n", entry->campaignId, entry->subMissionId);
				file.WriteString(line);
			}

			file.WriteString("\r\n");
		}

		if (mVariableCount != 0)
		{
			file.WriteString("[campaign_variables]\r\n");
			std::uint32_t offset = 0;

			for (std::uint32_t index = 0; index < mVariableCount; ++index)
			{
				const auto* entry = reinterpret_cast<const SCampaignVariable*>(reinterpret_cast<const std::byte*>(mVariables) + offset);
				NXBasics::Runtime_PrintString(line, "v %d %d %d\r\n", entry->campaignId, entry->variableId, entry->value);
				file.WriteString(line);
				offset += 12;
			}

			file.WriteString("\r\n");
		}
	}

	// Original function: FUN_0043b060
	const char* Player_GetSystemPlayerFileSystemName()
	{

		return OriginalStrings::SystemPlayerFileSystemName;
	}
}