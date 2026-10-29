#include "XUnicodeFont.h"
#include "XBitmap.h"
#include "XFontMetrics.h"
#include "XFont.h"
#include "XPalette.h"
#include "XTextEncoding.h"

#include <Windows.h>
#include <dwrite_1.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <list>
#include <vector>

namespace NXBasics
{
	using Microsoft::WRL::ComPtr;

	bool Font_TryLegacyText(const void* font, const char* text, std::string& legacy)
	{
		legacy.clear();

		if (font == nullptr || text == nullptr)
			return true;

		const auto wide = Text_ToWide(text);

		if (wide.empty())
			return true;

		BOOL substituted = FALSE;
		// The original western sprite atlas is not a Unicode cmap. In particular,
		// Cyrillic code-page bytes must not select visually unrelated western bobs.
		constexpr UINT page = 1252;
		const int count = WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, &substituted);

		if (count == 0 || substituted)
			return false;

		legacy.resize(count);
		WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), legacy.data(), count, nullptr, &substituted);

		if (substituted)
			return false;

		const void* manager;
		std::memcpy(&manager, static_cast<const std::byte*>(font) + FontStorageOffset::BobManager, sizeof(manager));

		for (const auto value : legacy)
		{
			const auto byte = static_cast<unsigned char>(value);

			if (byte > 32 && byte != '_' && (manager == nullptr || BobManager_GetBobAreaRectanglePtr(manager, byte - 32u) == nullptr))
				return false;
		}

		return true;
	}

	namespace
	{
		// The game consumes text on its main thread. Cache only bounded raster data,
		// never borrowed CFont/CBitmap/palette pointers or native object members.
		struct Raster
		{
			std::wstring text;
			int size = 0, wrap = 0, width = 0, height = 0, advance = 0, lineHeight = 0, left = 0, top = 0;
			std::vector<std::uint32_t> pixels;
			std::vector<float> carets;
		};

		class BitmapTextRenderer final : public IDWriteTextRenderer
		{
		  public:
			IDWriteBitmapRenderTarget* target;
			IDWriteRenderingParams* parameters;

			BitmapTextRenderer(IDWriteBitmapRenderTarget* bitmap, IDWriteRenderingParams* rendering) : target(bitmap), parameters(rendering)
			{
			}

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** result) override
			{
				if (result == nullptr)
					return E_POINTER;

				*result = nullptr;

				if (id == __uuidof(IUnknown) || id == __uuidof(IDWritePixelSnapping) || id == __uuidof(IDWriteTextRenderer))
				{
					*result = static_cast<IDWriteTextRenderer*>(this);
					AddRef();
					return S_OK;
				}

				return E_NOINTERFACE;
			}

			ULONG STDMETHODCALLTYPE AddRef() override
			{

				return 1;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{

				return 1;
			}

			HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* disabled) override
			{
				*disabled = FALSE;

				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* matrix) override
			{
				*matrix = { 1, 0, 0, 1, 0, 0 };

				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* scale) override
			{
				*scale = 1;

				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*, FLOAT x, FLOAT y, DWRITE_MEASURING_MODE mode, const DWRITE_GLYPH_RUN* run, const DWRITE_GLYPH_RUN_DESCRIPTION*, IUnknown*) override
			{

				return target->DrawGlyphRun(x, y, mode, run, parameters, RGB(255, 255, 255), nullptr);
			}

			HRESULT STDMETHODCALLTYPE DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override
			{

				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override
			{

				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) override
			{

				return E_NOTIMPL;
			}
		};

		struct TextRuntime
		{
			ComPtr<IDWriteFactory> factory;
			ComPtr<IDWriteGdiInterop> interop;
			ComPtr<IDWriteRenderingParams> parameters;
			std::list<Raster> cache;
			std::size_t cacheBytes = 0;

			TextRuntime()
			{
				if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf()))))
				{

					return;
				}

				factory->GetGdiInterop(&interop);
				factory->CreateCustomRenderingParams(1.0f, 0.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_FLAT, DWRITE_RENDERING_MODE_NATURAL, &parameters);
			}
		};

		int FontSize(const void* font)
		{
			int height = 10;

			if (font != nullptr)
				std::memcpy(&height, static_cast<const std::byte*>(font) + FontStorageOffset::Height, sizeof(height));

			return (std::clamp)(height, 6, 72);
		}

		const Raster* GetRaster(const void* font, const char* text, int wrap)
		{
			static TextRuntime runtime;

			if (!runtime.factory || !runtime.interop || !runtime.parameters)
				return nullptr;

			auto wide = Text_ToWide(text);

			if (wide.empty() || wide.size() > 32768)
				return nullptr;

			const int size = FontSize(font);
			wrap = (std::clamp)(wrap, 0, 8192);

			for (auto entry = runtime.cache.begin(); entry != runtime.cache.end(); ++entry)
			{
				if (entry->size == size && entry->wrap == wrap && entry->text == wide)
				{
					runtime.cache.splice(runtime.cache.begin(), runtime.cache, entry);
					return &runtime.cache.front();
				}
			}

			ComPtr<IDWriteTextFormat> format;

			if (FAILED(runtime.factory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<float>(size), L"", &format)))
				return nullptr;

			format->SetWordWrapping(wrap > 0 ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
			format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, static_cast<float>(size + 2), static_cast<float>(size));
			// DirectWrite shapes runs and selects installed fallback fonts. Choose
			// paragraph direction from the first strong character, including RTL text.

			for (wchar_t character : wide)
			{
				WORD type = 0;
				GetStringTypeW(CT_CTYPE2, &character, 1, &type);

				if (type == C2_RIGHTTOLEFT)
				{
					format->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
					format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
					break;
				}

				if (type == C2_LEFTTORIGHT)
					break;
			}

			ComPtr<IDWriteTextLayout> layout;

			if (FAILED(runtime.factory->CreateTextLayout(wide.data(), static_cast<UINT32>(wide.size()), format.Get(), static_cast<float>(wrap > 0 ? wrap : 32768), 65536.0f, &layout)))
				return nullptr;

			DWRITE_TEXT_METRICS metrics{ };
			DWRITE_OVERHANG_METRICS overhang{ };

			if (FAILED(layout->GetMetrics(&metrics)) || FAILED(layout->GetOverhangMetrics(&overhang)))
				return nullptr;
			// Align an unwrapped RTL paragraph to its measured width, not 32768 px.

			if (wrap == 0)
			{
				layout->SetMaxWidth((std::max)(1.0f, metrics.widthIncludingTrailingWhitespace));
				layout->GetMetrics(&metrics);
			}

			Raster raster;
			raster.text = std::move(wide);
			raster.size = size;
			raster.wrap = wrap;
			raster.advance = static_cast<int>(std::ceil(metrics.widthIncludingTrailingWhitespace));
			raster.lineHeight = static_cast<int>(std::ceil(metrics.height));
			raster.carets.resize(raster.text.size() + 1);

			for (UINT32 position = 0; position <= raster.text.size(); ++position)
			{
				FLOAT caretX = 0, caretY = 0;
				DWRITE_HIT_TEST_METRICS hit{ };

				if (SUCCEEDED(layout->HitTestTextPosition(position, FALSE, &caretX, &caretY, &hit)))
					raster.carets[position] = caretX;
			}

			const int left = static_cast<int>(std::ceil((std::max)(0.0f, overhang.left)));
			const int top = static_cast<int>(std::ceil((std::max)(0.0f, overhang.top)));
			raster.left = left;
			raster.top = top;
			raster.width = raster.advance + left + static_cast<int>(std::ceil((std::max)(0.0f, overhang.right))) + 2;
			raster.height = raster.lineHeight + top + static_cast<int>(std::ceil((std::max)(0.0f, overhang.bottom))) + 2;

			if (raster.width <= 0 || raster.width > 8192 || raster.height <= 0 || raster.height > 4096 || static_cast<std::uint64_t>(raster.width) * raster.height > 4u * 1024u * 1024u)
				return nullptr;

			ComPtr<IDWriteBitmapRenderTarget> target;

			if (FAILED(runtime.interop->CreateBitmapRenderTarget(nullptr, raster.width, raster.height, &target)))
				return nullptr;

			target->SetPixelsPerDip(1);
			ComPtr<IDWriteBitmapRenderTarget1> grayscale;

			if (SUCCEEDED(target.As(&grayscale)))
				grayscale->SetTextAntialiasMode(DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE);

			const HDC dc = target->GetMemoryDC();
			PatBlt(dc, 0, 0, raster.width, raster.height, BLACKNESS);
			BitmapTextRenderer renderer(target.Get(), runtime.parameters.Get());

			if (FAILED(layout->Draw(nullptr, &renderer, static_cast<float>(left), static_cast<float>(top))))
				return nullptr;

			GdiFlush();
			DIBSECTION dib{ };

			if (GetObjectW(GetCurrentObject(dc, OBJ_BITMAP), sizeof(dib), &dib) == 0 || dib.dsBm.bmBits == nullptr || dib.dsBm.bmBitsPixel != 32)
				return nullptr;

			raster.pixels.resize(static_cast<std::size_t>(raster.width) * raster.height);
			// GetMemoryDC guarantees a 32-bit top-down DIB. Preserve its scanline
			// order instead of inferring orientation from GetObject's height sign.

			for (int row = 0; row < raster.height; ++row)
			{
				std::memcpy(raster.pixels.data() + static_cast<std::size_t>(row) * raster.width, static_cast<const std::byte*>(dib.dsBm.bmBits) + static_cast<std::size_t>(row) * dib.dsBm.bmWidthBytes, static_cast<std::size_t>(raster.width) * 4);
			}

			const auto bytes = raster.pixels.size() * 4;

			while (!runtime.cache.empty() && (runtime.cache.size() >= 128 || runtime.cacheBytes + bytes > 16u * 1024u * 1024u))
			{
				runtime.cacheBytes -= runtime.cache.back().pixels.size() * 4;
				runtime.cache.pop_back();
			}

			runtime.cacheBytes += bytes;
			runtime.cache.push_front(std::move(raster));

			return &runtime.cache.front();
		}
	}

	std::int32_t UnicodeFont_Measure(const void* font, const char* text, bool height, std::int32_t wrapWidth)
	{
		const auto* raster = GetRaster(font, text, wrapWidth);

		return raster == nullptr ? 0 : height ? raster->lineHeight
		                                      : raster->advance;
	}

	std::int32_t UnicodeFont_CursorPosition(const void* font, const char* text, std::uint32_t bytePosition)
	{
		const auto* raster = GetRaster(font, text, 0);

		if (raster == nullptr)
			return 0;

		const std::string prefix(text, Utf8_PrefixBytes(text, bytePosition));
		const auto position = Text_ToWide(prefix.c_str()).size();

		return position < raster->carets.size() ? static_cast<std::int32_t>(std::lround(raster->carets[position])) : raster->advance;
	}

	bool UnicodeFont_Print(const void* font, CBitmap* bitmap, const SColorRGB* color, std::int32_t x, std::int32_t y, const char* text, std::int32_t wrapWidth)
	{
		if (bitmap == nullptr || font == nullptr || text == nullptr)
			return false;

		const auto* raster = GetRaster(font, text, wrapWidth);

		if (raster == nullptr)
			return false;

		SColorRGB ink(255, 255, 255);

		if (color != nullptr)
		{
			ink = *color;
		}
		else
		{
			const void* palette;
			std::memcpy(&palette, static_cast<const std::byte*>(font) + FontStorageOffset::Palette, sizeof(palette));

			if (palette != nullptr)
				std::memcpy(&ink, static_cast<const std::byte*>(palette) + PaletteOffset::Colors + 255 * 4, sizeof(ink));
		}

		const auto& rectangle = bitmap->GetRectangle();

		for (int row = 0; row < raster->height; ++row)
		{
			const auto destinationY = static_cast<std::int64_t>(y) + row - raster->top;

			if (destinationY < 0 || destinationY >= rectangle.height)
				continue;

			for (int column = 0; column < raster->width; ++column)
			{
				const auto destinationX = static_cast<std::int64_t>(x) + column - raster->left;

				if (destinationX < 0 || destinationX >= rectangle.width)
					continue;

				const unsigned alpha = raster->pixels[static_cast<std::size_t>(row) * raster->width + column] & 255u;

				if (alpha == 0)
					continue;

				const int px = static_cast<int>(destinationX), py = static_cast<int>(destinationY);
				// Draw_SetPixel performs the bitmap's existing depth/palette dispatch.
				// Normal remaster GUI bitmaps are BGRX; blend their background in place.
				int depth;
				std::memcpy(&depth, reinterpret_cast<const std::byte*>(bitmap) + BitmapOffset::Depth, sizeof(depth));

				if (depth == 2)
				{
					auto* pixel = static_cast<std::uint32_t*>(bitmap->GetUnclippedPixelPtr(px, py));

					if (pixel == nullptr)
						continue;

					const auto old = *pixel;
					const unsigned red = (ink.red * alpha + ((old >> 16) & 255u) * (255u - alpha) + 127u) / 255u;
					const unsigned green = (ink.green * alpha + ((old >> 8) & 255u) * (255u - alpha) + 127u) / 255u;
					const unsigned blue = (ink.blue * alpha + (old & 255u) * (255u - alpha) + 127u) / 255u;
					*pixel = (old & 0xFF000000u) | (red << 16) | (green << 8) | blue;
				}
				else if (alpha >= 128)
				{
					bitmap->Draw_SetPixel(px, py, ink);
				}
			}
		}

		return true;
	}
}