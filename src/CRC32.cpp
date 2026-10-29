#include "CRC32.h"
#include "XNumericConstants.h"

namespace
{
	// Original data: DAT_00569f7c
	std::uint32_t g_crcTable[256] = { };

	// Original function: FUN_004f294c
	void InitializeCRCTable()
	{
		for (std::uint32_t index = 0; index < 256; ++index)
		{
			std::uint32_t value = index << 24;

			for (unsigned int bit = 0; bit < 8; ++bit)
			{
				if ((value & NXBasics::IntegerRange::Bit31) != 0)
				{
					value = (value << 1) ^ 0x04C11DB7u;
				}
				else
				{
					value <<= 1;
				}
			}

			g_crcTable[index] = value;
		}
	}
}

namespace NCRC32
{
	// Original function: FUN_004f2981
	std::uint32_t Calculate(const void* data, std::int32_t byteCount)
	{
		if (g_crcTable[1] == 0)
			InitializeCRCTable();

		const auto* bytes = static_cast<const std::uint8_t*>(data);
		std::uint32_t checksum = NXBasics::IntegerRange::UInt32Maximum;

		for (std::int32_t index = 0; index < byteCount; ++index)
		{
			const std::uint32_t tableIndex = (checksum >> 24) ^ bytes[index];
			checksum = (checksum << 8) ^ g_crcTable[tableIndex];
		}

		return ~checksum;
	}

	// Original function: FUN_004f29c1
	void UpdateByte(std::uint32_t* checksum, std::uint8_t value)
	{
		// DAT_00569f80 is table entry 1, not a separate flag.

		if (g_crcTable[1] == 0)
		{
			InitializeCRCTable();
		}

		const std::uint32_t current = *checksum;
		const std::uint32_t index = (current >> 24) ^ value;

		*checksum = (current << 8) ^ g_crcTable[index];
	}
}