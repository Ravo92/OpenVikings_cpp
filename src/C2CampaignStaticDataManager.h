#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SCampaignSubMission
	{
		std::uint32_t campaignId;
		std::uint32_t subMissionId;
	};

	struct SCampaignVariable
	{
		std::uint32_t campaignId;
		std::uint32_t variableId;
		std::uint32_t value;
	};

	class CCampaignStaticDataManager
	{
	  public:
		CCampaignStaticDataManager();
		~CCampaignStaticDataManager();
		CCampaignStaticDataManager(const CCampaignStaticDataManager&) = delete;
		CCampaignStaticDataManager& operator=(const CCampaignStaticDataManager&) = delete;

		void CampaignSubMission_DoAllow(std::uint32_t campaignId, std::uint32_t subMissionId, bool save);
		bool CampaignSubMission_IsAllowed(std::uint32_t campaignId, std::uint32_t subMissionId) const;

	  private:
		bool l_IO_Load();
		void l_IO_Save() const;

		char mDataDirectory[260];
		char mFileName[260];
		std::uint32_t mSubMissionCount;
		SCampaignSubMission* mSubMissions;
		std::uint32_t mVariableCount;
		SCampaignVariable* mVariables;
	};

	static_assert(sizeof(SCampaignSubMission) == 8);
	static_assert(sizeof(SCampaignVariable) == 12);
	static_assert(sizeof(CCampaignStaticDataManager) == 536);
}