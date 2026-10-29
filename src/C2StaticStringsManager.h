#pragma once

namespace NC2Logic
{
	class CStaticStringsManager
	{
	  public:
		CStaticStringsManager();
		~CStaticStringsManager();
		CStaticStringsManager(const CStaticStringsManager&) = delete;
		CStaticStringsManager& operator=(const CStaticStringsManager&) = delete;

	  private:
		unsigned char mState;
	};

	static_assert(sizeof(CStaticStringsManager) == 1);
}