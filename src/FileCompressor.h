#pragma once

#include <cstdint>

namespace NXBasics
{
	struct RuntimeFormatStream;
}

namespace NNetwork
{
	// Descriptive reconstruction name; the original class name is not established.
	class CFileCompressor
	{
	  public:
		// Original function: FUN_004e0b3a
		CFileCompressor();

		// Original function: FUN_004e0b4e
		std::int32_t CompressFile(const char* fileName);

		// Original function: FUN_004e0d5c
		std::int32_t DecompressFile(const char* fileName);

	  private:
		NXBasics::RuntimeFormatStream* mInput;     // +0x00
		NXBasics::RuntimeFormatStream* mOutput;    // +0x04
		NXBasics::RuntimeFormatStream* mTemporary; // +0x08
	};

	static_assert(sizeof(CFileCompressor) == 3 * sizeof(void*), "Unexpected native compressor layout");
}