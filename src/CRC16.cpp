#include "CRC16.h"
#include "XNumericConstants.h"

namespace NCRC16
{
	namespace
	{
		// Original data: DAT_00569d7c.
		std::uint16_t g_crcTable[256] = { };
	}

	// Original function: FUN_004f28cf
	void InitializeCRCTable()
	{
		for (std::uint32_t index = 0; index < 256; ++index)
		{
			std::uint32_t value = index << 8;

			for (std::uint32_t bit = 0; bit < 8; ++bit)
			{
				if ((value & NXBasics::BitMask::Bit15) != 0)
				{
					value = (value << 1) ^ NXBasics::BitMask::Bit0AndBit5AndBit12;
				}
				else
				{
					value <<= 1;
				}
			}

			g_crcTable[index] = static_cast<std::uint16_t>(value);
		}
	}

	// Original function: FUN_004f2902
	std::uint16_t Calculate(const void* data, std::int32_t length)
	{
		// DAT_00569d7e is table entry 1, not a separate initialization flag.

		if (g_crcTable[1] == 0)
			InitializeCRCTable();

		std::uint16_t checksum = NXBasics::IntegerRange::UInt16Maximum;
		const auto* bytes = static_cast<const std::uint8_t*>(data);

		while (length > 0)
		{
			const std::uint32_t index = (static_cast<std::uint32_t>(checksum) >> 8) ^ *bytes;
			const std::uint32_t shifted = static_cast<std::uint32_t>(checksum) << 8;

			checksum = static_cast<std::uint16_t>(g_crcTable[index] ^ shifted);

			++bytes;
			--length;
		}

		return checksum;
	}
}