#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2InGameGuiKeyAssignmentManager
{
	class CKeyAssignmentManager
	{
	  public:
		CKeyAssignmentManager();
		~CKeyAssignmentManager();
		bool AddKey(std::int32_t assignment, std::uint32_t virtualKey, std::uint8_t auxiliaryCode, std::uint8_t shift, std::uint8_t control);
		bool CheckKey(std::int32_t assignment, std::uint32_t virtualKey, std::uint8_t auxiliaryCode) const;
		bool CheckDirectKeyVirtualOnly(std::int32_t assignment) const;
		inline static CKeyAssignmentManager* sTheObjectPtr = nullptr;

	  private:
		alignas(4) std::byte mStorage[2568];
	};

	static_assert(sizeof(CKeyAssignmentManager) == 2568);
}