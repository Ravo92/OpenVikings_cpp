#pragma once

// Integer ranges, bit masks and pixel/floating-point representations.
// Values and integer literal types are retained from the Windows reconstruction.
namespace NXBasics
{
	namespace Binary64
	{
		inline constexpr auto ExponentMaximum = 0x7FFu;
		inline constexpr auto ExponentHighWordMask = 0x7FF00000u;
		inline constexpr auto NegativeInfinityHighWord = 0xFFF00000u;
		inline constexpr auto FractionMask = 0x000FFFFFFFFFFFFFULL;
		inline constexpr auto ImplicitLeadingBit = 0x0010000000000000ULL;
		inline constexpr auto LargestFiniteBits = 0x7FEFFFFFFFFFFFFFULL;
		inline constexpr auto ExponentMask = 0x7FF0000000000000ULL;
		inline constexpr auto SignMask = 0x8000000000000000ULL;
	}

	namespace BitMask
	{
		inline constexpr auto Bit1 = 0x02u;
		inline constexpr auto Bit1Signed = 0x02;
		inline constexpr auto Bit2 = 0x04u;
		inline constexpr auto Bit2Signed = 0x04;
		inline constexpr auto Bit3 = 0x08u;
		inline constexpr auto Bit1AndBit3 = 0x0Au;
		inline constexpr auto Bits2Through3 = 0x0Cu;
		inline constexpr auto Low4Bits = 0x0Fu;
		inline constexpr auto Bit4 = 0x10u;
		inline constexpr auto Bit4Signed = 0x10;
		inline constexpr auto Bits3Through4 = 0x18u;
		inline constexpr auto Low5Bits = 0x1Fu;
		inline constexpr auto Bit5 = 0x20u;
		inline constexpr auto Bit5Signed = 0x20;
		inline constexpr auto Bits2Through5 = 0x3C;
		inline constexpr auto Low6Bits = 0x3Fu;
		inline constexpr auto Bit6 = 0x40u;
		inline constexpr auto Bit6Signed = 0x40;
		inline constexpr auto Bit0AndBit6 = 0x41u;
		inline constexpr auto Bits0Through1AndBit6 = 0x43u;
		inline constexpr auto Bit3AndBit6 = 0x48u;
		inline constexpr auto Bit3AndBit6Signed = 0x48;
		inline constexpr auto Bit4AndBit6 = 0x50u;
		inline constexpr auto Bit7 = 0x80u;
		inline constexpr auto Bit7Signed = 0x80;
		inline constexpr auto Bit1AndBit7 = 0x82u;
		inline constexpr auto ByteExceptBits2Through6 = 0x83u;
		inline constexpr auto ByteExceptBits3Through6 = 0x87u;
		inline constexpr auto ByteExceptBits4Through6 = 0x8Fu;
		inline constexpr auto Bit4AndBit7 = 0x90u;
		inline constexpr auto ByteExceptBits0Through5 = 0xC0u;
		inline constexpr auto ByteExceptBits4Through5 = 0xCFu;
		inline constexpr auto ByteExceptBit5 = 0xDFu;
		inline constexpr auto ByteExceptBit4 = 0xEFu;
		inline constexpr auto ByteExceptBits2Through3 = 0xF3;
		inline constexpr auto ByteExceptBit3 = 0xF7u;
		inline constexpr auto ByteExceptBits0Through2 = 0xF8u;
		inline constexpr auto ByteExceptBit2 = 0xFB;
		inline constexpr auto ByteExceptBits0Through1 = 0xFCu;
		inline constexpr auto ByteExceptBits0Through1Signed = 0xFC;
		inline constexpr auto ByteExceptBit1 = 0xFDu;
		inline constexpr auto ByteExceptBit1Signed = 0xFD;
		inline constexpr auto ByteExceptBit0 = 0xFEu;
		inline constexpr auto Bit8 = 0x100u;
		inline constexpr auto Bit8Signed = 0x100;
		inline constexpr auto Bits0Through1AndBit8 = 0x103u;
		inline constexpr auto Bit2AndBit8 = 0x104u;
		inline constexpr auto Bits0Through2AndBit8 = 0x107u;
		inline constexpr auto Bit3AndBit8 = 0x108u;
		inline constexpr auto Bits2Through3AndBit8 = 0x10Cu;
		inline constexpr auto Bits1Through8 = 0x1FEu;
		inline constexpr auto Low9Bits = 0x1FFu;
		inline constexpr auto Low9BitsSigned = 0x1FF;
		inline constexpr auto Bit9 = 0x200;
		inline constexpr auto Bit9Unsigned = 0x200u;
		inline constexpr auto Bits8Through9 = 0x300u;
		inline constexpr auto Bit10 = 0x400u;
		inline constexpr auto Bits8Through10 = 0x700u;
		inline constexpr auto Bit11 = 0x800u;
		inline constexpr auto Bit4AndBit11 = 0x810u;
		inline constexpr auto Bits4Through5AndBit11 = 0x830u;
		inline constexpr auto Bit8AndBit11 = 0x900u;
		inline constexpr auto Bits10Through11 = 0xC00u;
		inline constexpr auto Low12Bits = 0x0FFF;
		inline constexpr auto Low12BitsUnsigned = 0xFFFu;
		inline constexpr auto Bit12 = 0x1000u;
		inline constexpr auto Bit0AndBit5AndBit12 = 0x1021u;
		inline constexpr auto Bit1AndBit8AndBit12 = 0x1102u;
		inline constexpr auto Low13Bits = 0x1FFFu;
		inline constexpr auto Bit13 = 0x2000u;
		inline constexpr auto Bit13Signed = 0x2000;
		inline constexpr auto Low14Bits = 0x3FFFu;
		inline constexpr auto Bit14 = 0x4000u;
		inline constexpr auto Bit8AndBit14 = 0x4100u;
		inline constexpr auto Bits8Through10AndBit14 = 0x4700u;
		inline constexpr auto Bits3Through14 = 0x7FF8u;
		inline constexpr auto Bit15 = 0x8000u;
		inline constexpr auto Bit15Signed = 0x8000;
		inline constexpr auto WordExceptBits4Through14 = 0x800Fu;
		inline constexpr auto WordExceptBits0Through13 = 0xC000u;
		inline constexpr auto WordExceptBits0Through10 = 0xF800u;
		inline constexpr auto WordExceptBits0Through9 = 0xFC00u;
		inline constexpr auto WordExceptBits0Through7 = 0xFF00u;
		inline constexpr auto WordExceptBits0Through1 = 0xFFFCu;
		inline constexpr auto WordExceptBit0 = 0xFFFEu;
		inline constexpr auto Bit16 = 0x10000u;
		inline constexpr auto Bit16Signed = 0x10000;
		inline constexpr auto Bits1Through16 = 0x1FFFEu;
		inline constexpr auto Low17Bits = 0x1FFFFu;
		inline constexpr auto Bit17 = 0x00020000u;
		inline constexpr auto Bit17Signed = 0x20000;
		inline constexpr auto Bits16Through17 = 0x30000u;
		inline constexpr auto Bit18 = 0x40000u;
		inline constexpr auto Low19Bits = 0x7FFFFu;
		inline constexpr auto Bit19 = 0x00080000u;
		inline constexpr auto Low20Bits = 0xFFFFFu;
		inline constexpr auto Low22Bits = 0x003FFFFFu;
		inline constexpr auto Bit23 = 0x00800000u;
		inline constexpr auto Bit19AndBits22Through23 = 0x00C80000;
		inline constexpr auto Bits16Through23 = 0xFF0000u;
		inline constexpr auto Low24Bits = 0x00FFFFFFu;
		inline constexpr auto Bit30 = 0x40000000u;
		inline constexpr auto Bits0Through18AndBits20Through21AndBits24Through30 = 0x7F37FFFF;
		inline constexpr auto DwordExceptBits0Through23 = 0xFF000000u;
		inline constexpr auto DwordExceptBits5Through16 = 0xFFFE001Fu;
		inline constexpr auto DwordExceptBits0Through15 = 0xFFFF0000u;
		inline constexpr auto DwordExceptBits0Through11 = 0xFFFFF000u;
		inline constexpr auto DwordExceptBits10Through11 = 0xFFFFF3FFu;
		inline constexpr auto DwordExceptBit11 = 0xFFFFF7FFu;
		inline constexpr auto DwordExceptBit10 = 0xFFFFFBFFu;
		inline constexpr auto DwordExceptBits0Through7 = 0xFFFFFF00u;
		inline constexpr auto DwordExceptBit5 = 0xFFFFFFDFu;
		inline constexpr auto DwordExceptBits2Through4 = 0xFFFFFFE3u;
		inline constexpr auto DwordExceptBits3Through4 = 0xFFFFFFE7u;
		inline constexpr auto Bits0Through1AndBit3AndBits5Through31 = 0xFFFFFFEBu;
		inline constexpr auto DwordExceptBit4 = 0xFFFFFFEFu;
		inline constexpr auto DwordExceptBits0Through3 = 0xFFFFFFF0u;
		inline constexpr auto Bit2AndBits4Through31 = 0xFFFFFFF4u;
		inline constexpr auto DwordExceptBit3 = 0xFFFFFFF7u;
		inline constexpr auto DwordExceptBit2 = 0xFFFFFFFBu;
		inline constexpr auto DwordExceptBits0Through1 = 0xFFFFFFFCu;
		inline constexpr auto DwordExceptBit0 = 0xFFFFFFFEu;
		inline constexpr auto Low48Bits = 0x0000FFFFFFFFFFFFULL;
		inline constexpr auto Bit63 = 0x8000000000000000ULL;
	}

	namespace IntegerRange
	{
		inline constexpr auto Int8Maximum = 0x7Fu;
		inline constexpr auto Int8MaximumSigned = 0x7F;
		inline constexpr auto UInt8Maximum = 0xFFu;
		inline constexpr auto UInt8MaximumSigned = 0xFF;
		inline constexpr auto Int16Maximum = 0x7FFFu;
		inline constexpr auto Int16MaximumSigned = 0x7fff;
		inline constexpr auto UInt16Maximum = 0xFFFFu;
		inline constexpr auto UInt16MaximumSigned = 0xFFFF;
		inline constexpr auto Int32Maximum = 0x7FFFFFFFu;
		inline constexpr auto Int32MaximumSigned = 0x7FFFFFFF;
		inline constexpr auto Bit31 = 0x80000000u;
		inline constexpr auto UInt32Maximum = 0xFFFFFFFF;
		inline constexpr auto UInt32MaximumUnsignedLongLong = 0xFFFFFFFFULL;
		inline constexpr auto Int64Maximum = 0x7FFFFFFFFFFFFFFFULL;
	}

	namespace Rgb555
	{
		inline constexpr auto GreenMask = 0x3E0u;
		inline constexpr auto GreenMaskSigned = 0x03E0;
		inline constexpr auto RedMask = 0x7C00;
		inline constexpr auto PairedGreenMask = 0x03E003E0u;
	}

	namespace Rgb565
	{
		inline constexpr auto GreenMask = 0x07E0;
		inline constexpr auto GreenMaskUnsigned = 0x7E0u;
		inline constexpr auto RedMask = 0xF800;
		inline constexpr auto RedMaskUnsigned = 0xF800u;
		inline constexpr auto PairedGreenMask = 0x07E007E0u;
		inline constexpr auto RedProductMask = 0xFFFFF800u;
	}

	namespace Rgb888
	{
		inline constexpr auto GreenMask = 0x0000FF00;
		inline constexpr auto GreenMaskUnsigned = 0xFF00u;
		inline constexpr auto RedMask = 0x00FF0000;
		inline constexpr auto RedMaskUnsigned = 0xFF0000u;
	}

}