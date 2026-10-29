#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	class CPalette;
}

namespace NToolGouraudShader
{
	struct SVertex
	{
		std::int32_t x, y, u, v, brightness, red, green, blue;
	};

	class CGouraudShader
	{
	  public:
		NXBasics::CBitmap* bitmap;
		const std::uint8_t* texture;
		const std::uint8_t* alphaTexture;
		std::uint32_t flags;
		SVertex vertices[3];
		NXBasics::CPalette* palette;
		std::uint32_t unresolved74;
		std::int32_t firstY, height, skipRows, clipLeft, clipRight, clipTop, clipBottom;

		CGouraudShader();
		void XShade();
		void XShade_VertexColor();
		void XShade_Alpha();
		void XShade_VertexColor_Alpha();
		void l_SetupAndClip();
		void XShade_32Bit();
		void XShade_VertexColor_32Bit();
		void XShade_Alpha_32Bit();
		void XShade_VertexColor_Alpha_32Bit();
		void XShade_SSE2();
		void XShade_SSE2_32Bit();
		void XShade_Alpha_SSE2();
		void XShade_Alpha_SSE2_32Bit();
	};

	static_assert(sizeof(SVertex) == 32);
	static_assert(sizeof(CGouraudShader) == (sizeof(void*) == 8 ? 168 : 148));
	static_assert(offsetof(CGouraudShader, vertices) == (sizeof(void*) == 8 ? 28 : 16));
	static_assert(offsetof(CGouraudShader, palette) == (sizeof(void*) == 8 ? 128 : 112));
	static_assert(offsetof(CGouraudShader, skipRows) == (sizeof(void*) == 8 ? 148 : 128));
}