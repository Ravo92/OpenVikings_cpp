#pragma once

#include <cstdint>

namespace NXBasics
{
	// Original function: FUN_00410715
	void* XB_Storable_LoadObject(void* file);

	// Original function: FUN_0041083d
	void* XB_Storable_LoadObject(const char* fileName);

	// Original function: FUN_00410876
	void XB_Storable_GetObjectIdAndVersion(void* file, std::uint32_t* objectId, std::uint32_t* version);

	// Original function: FUN_004108c5
	void XB_Storable_GetObjectIdAndVersion(const char* fileName, std::uint32_t* objectId, std::uint32_t* version);
}