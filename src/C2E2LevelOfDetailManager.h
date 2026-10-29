#pragma once

#include <cstdint>

namespace NC2E2
{
	class CLevelOfDetailManager
	{
	  public:
		CLevelOfDetailManager();
		~CLevelOfDetailManager();

		inline static CLevelOfDetailManager* sTheObjectPtr = nullptr;
		inline static std::int32_t sLevelOfDetail = 0;

		static void CreateInstance();
		static void DestroyInstance();

		static void SetLevelOfDetail(std::int32_t levelOfDetail);
		static bool GetUseLandscapeAlphaBobsFlag();
		static bool GetUseMatrixFXBobsFlag();
		static bool GetSkipUnimportantVoidLandscapesFlag();

	  private:
		std::uint8_t mState;
	};

	static_assert(sizeof(CLevelOfDetailManager) == 1);
}