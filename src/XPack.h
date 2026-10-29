#pragma once

#include <cstdint>

namespace NXBasics
{
	std::uint32_t XB_Pack_Simple8BitRLEPack(const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes);
	std::uint32_t XB_Pack_Simple16BitRLEPack(const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes);
	std::uint32_t XB_Pack_Pack(std::uint32_t packingType, const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes);
	bool XB_Pack_Simple8BitRLEUnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes);
	bool XB_Pack_Simple16BitRLEUnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes);
	bool XB_Pack_UnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes);
	bool XB_Pack_UnPack(const void* packedBlock, std::uint32_t packedBytes, void* destination, std::uint32_t destinationBytes);
}