#include "X2DDisplay.h"
#include "X2DApplication.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XMMouseManager.h"
#include "XPixelFormatConverter.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <algorithm>
#include <cstring>
#include <new>

namespace
{
	template<class T> void Release(T*& object)
	{
		if (object != nullptr) object->Release();
		object = nullptr;
	}

	bool CheckGraphics(HRESULT result)
	{
		if (SUCCEEDED(result)) return true;
		char message[100];
		wsprintfA(message, "OpenVikings: presentation failed (HRESULT 0x%08X).\n", static_cast<unsigned>(result));
		OutputDebugStringA(message);
		return false;
	}

	std::uint32_t ExtractChannel(std::uint32_t pixel, std::uint32_t mask)
	{
		if (mask == 0) return 0;
		while ((mask & 1) == 0) { mask >>= 1; pixel >>= 1; }
		return static_cast<std::uint32_t>(static_cast<std::uint64_t>(pixel & mask) * 255 / mask);
	}

	// The software game renderer supplies one opaque image. A fullscreen triangle
	// presents it without changing the monitor mode or the game's pixel formats.
	constexpr char PresentationShader[] = R"(
Texture2D image : register(t0);
SamplerState imageSampler : register(s0);
struct Vertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };
Vertex VS(uint id : SV_VertexID) {
    Vertex v;
    v.uv = float2((id << 1) & 2, id & 2);
    v.position = float4(v.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return v;
}
float4 PS(Vertex v) : SV_Target { return float4(image.Sample(imageSampler, v.uv).rgb, 1); }
float4 HudPS(Vertex v) : SV_Target {
    float4 color = image.Sample(imageSampler, v.uv);
    return float4(color.rgb, 1 - color.a);
}
)";
}

namespace NXSys2DApp
{
	struct CDisplay::SGraphics
	{
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		IDXGISwapChain1* swapChain = nullptr;
		ID3D11RenderTargetView* target = nullptr;
		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* image = nullptr;
		UINT frameWidth = 0;
		UINT frameHeight = 0;
		ID3D11VertexShader* vertexShader = nullptr;
		ID3D11PixelShader* pixelShader = nullptr;
		ID3D11PixelShader* hudShader = nullptr;
		ID3D11BlendState* hudBlend = nullptr;
		ID3D11Texture2D* hudTexture = nullptr;
		ID3D11ShaderResourceView* hudImage = nullptr;
		UINT hudWidth = 0;
		UINT hudHeight = 0;
		ID3D11SamplerState* pointSampler = nullptr;
		ID3D11SamplerState* linearSampler = nullptr;
		UINT targetWidth = 0;
		UINT targetHeight = 0;
	};

	CDisplay::CDisplay(bool fullscreen) : mFullscreen(fullscreen) {}
	CDisplay::~CDisplay() { DisableHud(); ShutDown(); }

	bool CDisplay::ConfigureHud(std::int32_t& width, std::int32_t& height)
	{
		DisableHud();
		const int logicalWidth = MulDiv(width, 768, height);
		if (!IsSupportedMode(logicalWidth, 768, 32)) return false;
		void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap));
		if (memory == nullptr) return false;
		mWorldComposition = ::new(memory) NXBasics::CBitmap(width, height, 2);
		mHudHeight = 768;
		mHudWidth = logicalWidth;
		width = mHudWidth;
		height = mHudHeight;
		return true;
	}

	void CDisplay::DisableHud()
	{
		mPresentHud = false;
		mHudBitmap = nullptr;
		mHudScratchBitmap = nullptr;
		mMainWorldElement = nullptr;
		mMainWorldBitmap = nullptr;
		mHudWidth = mHudHeight = 0;
		delete mWorldComposition;
		mWorldComposition = nullptr;
	}

	bool CDisplay::IsHudBitmap(const NXBasics::CBitmap* bitmap) const
	{
		if (mHudBitmap == nullptr || bitmap == nullptr) return false;
		while (bitmap->GetSourceBitmap() != nullptr) bitmap = bitmap->GetSourceBitmap();
		return bitmap == mHudBitmap || bitmap == mHudScratchBitmap;
	}

	bool IsRemasterHudBitmap(const NXBasics::CBitmap* bitmap)
	{
		const auto* application = CApplication::sTheObjectPtr;
		const auto* display = application != nullptr ? application->GetDisplay() : nullptr;
		return display != nullptr && display->IsHudBitmap(bitmap);
	}

	bool CDisplay::IsMainWorldBitmap(const NXBasics::CBitmap* bitmap) const
	{
		while (bitmap != nullptr)
		{
			if (bitmap == mMainWorldBitmap || bitmap == mWorldComposition) return true;
			bitmap = bitmap->GetSourceBitmap();
		}
		return false;
	}

	void CDisplay::HudToWorld(std::int32_t* point) const
	{
		if (mHudWidth <= 0 || mHudHeight <= 0) return;
		point[0] = MulDiv(point[0], mWidth, mHudWidth);
		point[1] = MulDiv(point[1], mHeight, mHudHeight);
	}

	void CDisplay::WorldToHud(std::int32_t* point) const
	{
		if (mHudWidth <= 0 || mHudHeight <= 0) return;
		point[0] = MulDiv(point[0], mHudWidth, mWidth);
		point[1] = MulDiv(point[1], mHudHeight, mHeight);
	}

	void CDisplay::CompositeHudImage(const NXBasics::CBitmap* source, NXBasics::CBitmap* destination, std::int32_t x, std::int32_t y)
	{
		if (source == nullptr || destination == nullptr || source->mDepth != 2 || destination->mDepth != 2) return;
		for (int row = 0; row < source->mRectangle.height; ++row) for (int column = 0; column < source->mRectangle.width; ++column)
		{
			const int dx = x + column, dy = y + row;
			if (dx < 0 || dy < 0 || dx >= destination->mRectangle.width || dy >= destination->mRectangle.height) continue;
			const auto foreground = source->mTrueColorMemory[static_cast<std::size_t>(row) * source->mRowPixelCount + column];
			const auto inverse = foreground >> 24;
			if (inverse == 255u) continue;
			auto& background = destination->mTrueColorMemory[static_cast<std::size_t>(dy) * destination->mRowPixelCount + dx];
			std::uint32_t result = (((background >> 24) * inverse) / 255u) << 24;
			for (int shift = 0; shift < 24; shift += 8) result |= (std::min)(255u, ((foreground >> shift) & 255u) + (((background >> shift) & 255u) * inverse) / 255u) << shift;
			background = result;
		}
	}

	void CDisplay::CopyCompositedFrame(NXBasics::CBitmap* bitmap) const
	{
		if (bitmap == nullptr || bitmap->mDepth != 2 || bitmap->mRectangle.width != mWidth || bitmap->mRectangle.height != mHeight) return;
		const bool pointFilter = mHudBitmap != nullptr && mWidth % mHudWidth == 0 && mHeight % mHudHeight == 0 && mWidth / mHudWidth == mHeight / mHudHeight;
		for (int y = 0; y < mHeight; ++y) for (int x = 0; x < mWidth; ++x)
		{
			const auto background = mPixels[static_cast<std::size_t>(y) * mWidth + x];
			auto result = background & 0x00FFFFFF;
			if (mHudBitmap != nullptr)
			{
				const double u = (x + 0.5) * mHudWidth / mWidth - 0.5;
				const double v = (y + 0.5) * mHudHeight / mHeight - 0.5;
				const int ix = u < 0 ? -1 : static_cast<int>(u), iy = v < 0 ? -1 : static_cast<int>(v);
				const auto sample = [this](int sx, int sy) { return mHudBitmap->mTrueColorMemory[static_cast<std::size_t>((std::clamp)(sy, 0, mHudHeight - 1)) * mHudBitmap->mRowPixelCount + (std::clamp)(sx, 0, mHudWidth - 1)]; };
				const auto p00 = sample(ix, iy), p10 = sample(ix + 1, iy), p01 = sample(ix, iy + 1), p11 = sample(ix + 1, iy + 1);
				const auto channel = [&](int shift)
				{
					if (pointFilter) return static_cast<double>((sample(static_cast<int>(u + 0.5), static_cast<int>(v + 0.5)) >> shift) & 255u);
					const double fx = u - ix, fy = v - iy;
					return ((p00 >> shift) & 255u) * (1 - fx) * (1 - fy) + ((p10 >> shift) & 255u) * fx * (1 - fy) + ((p01 >> shift) & 255u) * (1 - fx) * fy + ((p11 >> shift) & 255u) * fx * fy;
				};
				const double inverse = channel(24) / 255.0;
				result = 0;
				for (int shift = 0; shift < 24; shift += 8) result |= static_cast<std::uint32_t>((std::min)(255.0, channel(shift) + ((background >> shift) & 255u) * inverse) + 0.5) << shift;
			}
			bitmap->mTrueColorMemory[static_cast<std::size_t>(y) * bitmap->mRowPixelCount + x] = result;
		}
	}

	bool CDisplay::IsSupportedMode(std::uint32_t width, std::uint32_t height, std::uint32_t depth)
	{
		return width > 0 && height > 0 && width <= 8192 && height <= 8192 && static_cast<std::uint64_t>(width) * height <= 16777216 && depth == 32;
	}

	// Replaces the exclusive DirectDraw startup at Windows 0047ABDC..0047B056.
	bool CDisplay::StartUp(HWND windowHandle, std::int32_t width, std::int32_t height, std::int32_t depth)
	{
		mWindowHandle = windowHandle;
		// Legacy bitmap inputs still need RGB565 conversion; output is always 32-bit.
		NXBasics::g16BitPixelConverter->SetColorMasks(0xF800, 0x07E0, 0x001F);
		NXBasics::g32BitPixelConverter->SetColorMasks(0x00FF0000, 0x0000FF00, 0x000000FF);
		if (!SetDisplayMode(width, height, depth)) return false;
		if (CreateGraphics()) return true;
		ShutDown();
		return false;
	}

	bool CDisplay::CreateGraphics()
	{
		void* memory = NXBasics::Runtime_Allocate(sizeof(SGraphics));
		if (memory == nullptr) return false;
		mGraphics = ::new(memory) SGraphics();
		auto& graphics = *mGraphics;
		constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &graphics.device, nullptr, &graphics.context);
		if (FAILED(result))
		{
			Release(graphics.context);
			Release(graphics.device);
			result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &graphics.device, nullptr, &graphics.context);
		}
		if (!CheckGraphics(result)) return false;
		IDXGIDevice* dxgiDevice = nullptr;
		IDXGIAdapter* adapter = nullptr;
		IDXGIFactory2* factory = nullptr;
		result = graphics.device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
		if (SUCCEEDED(result)) result = dxgiDevice->GetAdapter(&adapter);
		if (SUCCEEDED(result)) result = adapter->GetParent(IID_PPV_ARGS(&factory));
		if (SUCCEEDED(result))
		{
			DXGI_SWAP_CHAIN_DESC1 description{ };
			description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			description.BufferCount = 2;
			description.Scaling = DXGI_SCALING_STRETCH;
			description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			result = factory->CreateSwapChainForHwnd(graphics.device, mWindowHandle, &description, nullptr, nullptr, &graphics.swapChain);
			if (SUCCEEDED(result)) result = factory->MakeWindowAssociation(mWindowHandle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
		}
		Release(factory);
		Release(adapter);
		Release(dxgiDevice);
		if (!CheckGraphics(result)) return false;
		ID3DBlob* shader = nullptr;
		ID3DBlob* errors = nullptr;
		result = D3DCompile(PresentationShader, sizeof(PresentationShader) - 1, nullptr, nullptr, nullptr, "VS", "vs_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &shader, &errors);
		if (SUCCEEDED(result)) result = graphics.device->CreateVertexShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &graphics.vertexShader);
		if (errors != nullptr) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
		Release(errors);
		Release(shader);
		if (!CheckGraphics(result)) return false;
		result = D3DCompile(PresentationShader, sizeof(PresentationShader) - 1, nullptr, nullptr, nullptr, "PS", "ps_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &shader, &errors);
		if (SUCCEEDED(result)) result = graphics.device->CreatePixelShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &graphics.pixelShader);
		if (errors != nullptr) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
		Release(errors);
		Release(shader);
		if (!CheckGraphics(result)) return false;
		result = D3DCompile(PresentationShader, sizeof(PresentationShader) - 1, nullptr, nullptr, nullptr, "HudPS", "ps_4_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &shader, &errors);
		if (SUCCEEDED(result)) result = graphics.device->CreatePixelShader(shader->GetBufferPointer(), shader->GetBufferSize(), nullptr, &graphics.hudShader);
		if (errors != nullptr) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
		Release(errors);
		Release(shader);
		if (!CheckGraphics(result)) return false;
		D3D11_BLEND_DESC blend{ };
		blend.RenderTarget[0].BlendEnable = TRUE;
		blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		if (!CheckGraphics(graphics.device->CreateBlendState(&blend, &graphics.hudBlend))) return false;
		D3D11_SAMPLER_DESC sampler{ };
		sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler.MaxAnisotropy = 1;
		sampler.MaxLOD = D3D11_FLOAT32_MAX;
		if (!CheckGraphics(graphics.device->CreateSamplerState(&sampler, &graphics.pointSampler))) return false;
		sampler.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		return CheckGraphics(graphics.device->CreateSamplerState(&sampler, &graphics.linearSampler)) && CreateFrameTexture() && ResizeTarget();
	}

	void CDisplay::ReleaseGraphics()
	{
		if (mGraphics == nullptr) return;
		auto& graphics = *mGraphics;
		if (graphics.context != nullptr) graphics.context->ClearState();
		Release(graphics.linearSampler);
		Release(graphics.pointSampler);
		Release(graphics.hudImage);
		Release(graphics.hudTexture);
		Release(graphics.hudBlend);
		Release(graphics.hudShader);
		Release(graphics.pixelShader);
		Release(graphics.vertexShader);
		Release(graphics.image);
		Release(graphics.texture);
		Release(graphics.target);
		Release(graphics.swapChain);
		// Flush deferred destruction before a replacement flip swapchain uses this HWND.
		if (graphics.context != nullptr) graphics.context->Flush();
		Release(graphics.context);
		Release(graphics.device);
		graphics.~SGraphics();
		NXBasics::Runtime_Free(mGraphics);
		mGraphics = nullptr;
	}

	void CDisplay::ShutDown()
	{
		ReleaseGraphics();
		delete mVideoFrame;
		mVideoFrame = nullptr;
		mVideoActive = mPresentHud = false;
		NXBasics::Runtime_Free(mPixels);
		mPixels = nullptr;
	}

	bool CDisplay::CreateFrameTexture()
	{
		auto& graphics = *mGraphics;
		ID3D11ShaderResourceView* empty = nullptr;
		graphics.context->PSSetShaderResources(0, 1, &empty);
		Release(graphics.image);
		Release(graphics.texture);
		D3D11_TEXTURE2D_DESC description{ };
		description.Width = mVideoActive ? mVideoFrame->GetRectangle().width : mWidth;
		description.Height = mVideoActive ? mVideoFrame->GetRectangle().height : mHeight;
		description.MipLevels = description.ArraySize = 1;
		description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		description.SampleDesc.Count = 1;
		description.Usage = D3D11_USAGE_DYNAMIC;
		description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (!CheckGraphics(graphics.device->CreateTexture2D(&description, nullptr, &graphics.texture)) || !CheckGraphics(graphics.device->CreateShaderResourceView(graphics.texture, nullptr, &graphics.image))) return false;
		graphics.frameWidth = description.Width;
		graphics.frameHeight = description.Height;
		return true;
	}

	bool CDisplay::ResizeTarget()
	{
		RECT client{ };
		if (!GetClientRect(mWindowHandle, &client) || client.right == 0 || client.bottom == 0) return false;
		auto& graphics = *mGraphics;
		if (graphics.target != nullptr && graphics.targetWidth == static_cast<UINT>(client.right) && graphics.targetHeight == static_cast<UINT>(client.bottom)) return true;
		graphics.context->OMSetRenderTargets(0, nullptr, nullptr);
		Release(graphics.target);
		if (!CheckGraphics(graphics.swapChain->ResizeBuffers(0, client.right, client.bottom, DXGI_FORMAT_UNKNOWN, 0))) return false;
		ID3D11Texture2D* buffer = nullptr;
		HRESULT result = graphics.swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer));
		if (SUCCEEDED(result)) result = graphics.device->CreateRenderTargetView(buffer, nullptr, &graphics.target);
		Release(buffer);
		graphics.targetWidth = client.right;
		graphics.targetHeight = client.bottom;
		return CheckGraphics(result);
	}

	// Replaces Windows 0047B38E. Only the logical frame is resized here.
	bool CDisplay::SetDisplayMode(std::int32_t width, std::int32_t height, std::int32_t depth)
	{
		if (!IsSupportedMode(width, height, depth)) return false;
		mVideoActive = false;
		if (mPixels != nullptr && mWidth == width && mHeight == height && mDepth == depth) return true;
		const std::size_t bytes = static_cast<std::size_t>(width) * height * sizeof(std::uint32_t);
		auto* pixels = static_cast<std::uint32_t*>(NXBasics::Runtime_Allocate(bytes));
		if (pixels == nullptr) return false;
		std::memset(pixels, 0, bytes);
		NXBasics::Runtime_Free(mPixels);
		mPixels = pixels;
		mWidth = width;
		mHeight = height;
		mDepth = depth;
		// Keep the HWND and swapchain intact across image/video/loading/menu changes.
		return mGraphics == nullptr || CreateFrameTexture();
	}

	void CDisplay::UpdateWindowPlacement()
	{
		MONITORINFO monitor{ sizeof(monitor) };
		if (!GetMonitorInfoW(MonitorFromWindow(mWindowHandle, MONITOR_DEFAULTTONEAREST), &monitor)) return;
		// Initial visibility belongs to ShowWindow, so the shell sees the first show.
		const LONG_PTR visibleStyle = GetWindowLongPtrW(mWindowHandle, GWL_STYLE) & WS_VISIBLE;
		if (mFullscreen)
		{
			SetWindowLongPtrW(mWindowHandle, GWL_STYLE, WS_POPUP | visibleStyle);
			SetWindowPos(mWindowHandle, HWND_NOTOPMOST, monitor.rcMonitor.left, monitor.rcMonitor.top, monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
		}
		else
		{
			RECT bounds = mWindowedRectangle;
			// Topology changes should preserve the live decorated-window rectangle.
			if ((GetWindowLongPtrW(mWindowHandle, GWL_STYLE) & WS_CAPTION) != 0 && GetWindowRect(mWindowHandle, &bounds)) mHasWindowedPlacement = true;
			SetWindowLongPtrW(mWindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW | visibleStyle);
			if (!mHasWindowedPlacement)
			{
				bounds = { 0, 0, mWidth, mHeight };
				AdjustWindowRectExForDpi(&bounds, WS_OVERLAPPEDWINDOW, FALSE, 0, GetDpiForWindow(mWindowHandle));
				const LONG width = (std::min)(bounds.right - bounds.left, monitor.rcWork.right - monitor.rcWork.left);
				const LONG height = (std::min)(bounds.bottom - bounds.top, monitor.rcWork.bottom - monitor.rcWork.top);
				bounds.left = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
				bounds.top = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;
				bounds.right = bounds.left + width;
				bounds.bottom = bounds.top + height;
			}
			const LONG width = (std::min)(bounds.right - bounds.left, monitor.rcWork.right - monitor.rcWork.left);
			const LONG height = (std::min)(bounds.bottom - bounds.top, monitor.rcWork.bottom - monitor.rcWork.top);
			bounds.left = (std::clamp)(bounds.left, monitor.rcWork.left, monitor.rcWork.right - width);
			bounds.top = (std::clamp)(bounds.top, monitor.rcWork.top, monitor.rcWork.bottom - height);
			SetWindowPos(mWindowHandle, HWND_NOTOPMOST, bounds.left, bounds.top, width, height, SWP_FRAMECHANGED | SWP_NOACTIVATE);
		}
	}

	bool CDisplay::ToggleFullscreen()
	{
		if (!mFullscreen) mHasWindowedPlacement = GetWindowRect(mWindowHandle, &mWindowedRectangle) != FALSE;
		mFullscreen = !mFullscreen;
		UpdateWindowPlacement();
		auto* application = CApplication::sTheObjectPtr;
		if (application != nullptr && application->fullscreenToggledCallback != nullptr) application->fullscreenToggledCallback();
		if (auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr) mouse->Callback_FullscreenToggled(mFullscreen);
		return true;
	}

	RECT CDisplay::GetPresentationRectangle() const
	{
		RECT client{ };
		GetClientRect(mWindowHandle, &client);
		if (mWidth <= 0 || mHeight <= 0 || client.right <= 0 || client.bottom <= 0) return { };
		// Requested FMV stretching fills the client area independently of game aspect.
		if (mVideoActive) return client;
		LONG width = client.right;
		LONG height = static_cast<LONG>(static_cast<std::int64_t>(width) * mHeight / mWidth);
		if (height > client.bottom)
		{
			height = client.bottom;
			width = static_cast<LONG>(static_cast<std::int64_t>(height) * mWidth / mHeight);
		}
		width = (std::max)(1L, width);
		height = (std::max)(1L, height);
		const LONG x = (client.right - width) / 2;
		const LONG y = (client.bottom - height) / 2;
		return { x, y, x + width, y + height };
	}

	POINT CDisplay::ScreenToGame(POINT point) const
	{
		ScreenToClient(mWindowHandle, &point);
		const RECT area = GetPresentationRectangle();
		if (area.right <= area.left || area.bottom <= area.top) return { };
		const int width = mHudWidth > 0 ? mHudWidth : mWidth;
		const int height = mHudHeight > 0 ? mHudHeight : mHeight;
		point.x = (std::clamp)(MulDiv(point.x - area.left, width, area.right - area.left), 0, width - 1);
		point.y = (std::clamp)(MulDiv(point.y - area.top, height, area.bottom - area.top), 0, height - 1);
		return point;
	}

	POINT CDisplay::GameToScreen(POINT point) const
	{
		const RECT area = GetPresentationRectangle();
		point.x = area.left + MulDiv(point.x, area.right - area.left, mHudWidth > 0 ? mHudWidth : mWidth);
		point.y = area.top + MulDiv(point.y, area.bottom - area.top, mHudHeight > 0 ? mHudHeight : mHeight);
		ClientToScreen(mWindowHandle, &point);
		return point;
	}

	std::int32_t CDisplay::GetWindowOffsetX() const { POINT point{ }; ClientToScreen(mWindowHandle, &point); return point.x; }
	std::int32_t CDisplay::GetWindowOffsetY() const { POINT point{ }; ClientToScreen(mWindowHandle, &point); return point.y; }

	void CDisplay::Update()
	{
		if (mPixels == nullptr || IsIconic(mWindowHandle) || !IsWindowVisible(mWindowHandle)) return;
		if (mGraphics == nullptr)
		{
			if (!CreateGraphics()) { ReleaseGraphics(); return; }
		}
		auto& graphics = *mGraphics;
		const HRESULT deviceStatus = graphics.device->GetDeviceRemovedReason();
		if (FAILED(deviceStatus)) { CheckGraphics(deviceStatus); ReleaseGraphics(); return; }
		if (!ResizeTarget()) return;
		const int frameWidth = mVideoActive ? mVideoFrame->GetRectangle().width : mWidth;
		const int frameHeight = mVideoActive ? mVideoFrame->GetRectangle().height : mHeight;
		const auto* framePixels = mVideoActive ? static_cast<const std::uint32_t*>(mVideoFrame->GetUnclippedPixelPtr(0, 0)) : mPixels;
		if (graphics.frameWidth != static_cast<UINT>(frameWidth) || graphics.frameHeight != static_cast<UINT>(frameHeight) || graphics.image == nullptr)
		{
			if (!CreateFrameTexture()) return;
		}
		ID3D11ShaderResourceView* empty = nullptr;
		graphics.context->PSSetShaderResources(0, 1, &empty);
		D3D11_MAPPED_SUBRESOURCE mapped{ };
		if (!CheckGraphics(graphics.context->Map(graphics.texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
		for (int row = 0; row < frameHeight; ++row) std::memcpy(static_cast<std::uint8_t*>(mapped.pData) + row * mapped.RowPitch, framePixels + static_cast<std::size_t>(row) * frameWidth, static_cast<std::size_t>(frameWidth) * 4);
		graphics.context->Unmap(graphics.texture, 0);
		const RECT area = GetPresentationRectangle();
		const D3D11_VIEWPORT viewport{ static_cast<float>(area.left), static_cast<float>(area.top), static_cast<float>(area.right - area.left), static_cast<float>(area.bottom - area.top), 0, 1 };
		constexpr float black[4] = { 0, 0, 0, 1 };
		graphics.context->ClearRenderTargetView(graphics.target, black);
		graphics.context->OMSetRenderTargets(1, &graphics.target, nullptr);
		graphics.context->RSSetViewports(1, &viewport);
		graphics.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphics.context->VSSetShader(graphics.vertexShader, nullptr, 0);
		graphics.context->PSSetShader(graphics.pixelShader, nullptr, 0);
		graphics.context->PSSetShaderResources(0, 1, &graphics.image);
		const LONG outputWidth = area.right - area.left;
		const LONG outputHeight = area.bottom - area.top;
		// Fractional nearest-neighbor scaling gives glyphs uneven pixel widths.
		// Keep exact integer enlargement sharp; interpolate all other scale factors.
		const bool integerScale = outputWidth % mWidth == 0 && outputHeight % mHeight == 0 && outputWidth / mWidth == outputHeight / mHeight;
		ID3D11SamplerState* sampler = !mVideoActive && integerScale ? graphics.pointSampler : graphics.linearSampler;
		graphics.context->PSSetSamplers(0, 1, &sampler);
		graphics.context->Draw(3, 0);
		if (!mVideoActive && mPresentHud && mHudBitmap != nullptr && !DrawHud()) return;
		const HRESULT result = graphics.swapChain->Present(1, 0);
		if (FAILED(result)) { CheckGraphics(result); ReleaseGraphics(); }
	}

	bool CDisplay::DrawHud()
	{
		auto& graphics = *mGraphics;
		ID3D11ShaderResourceView* empty = nullptr;
		graphics.context->PSSetShaderResources(0, 1, &empty);
		if (graphics.hudWidth != static_cast<UINT>(mHudWidth) || graphics.hudHeight != static_cast<UINT>(mHudHeight) || graphics.hudImage == nullptr)
		{
			Release(graphics.hudImage);
			Release(graphics.hudTexture);
			D3D11_TEXTURE2D_DESC description{ };
			description.Width = mHudWidth;
			description.Height = mHudHeight;
			description.MipLevels = description.ArraySize = 1;
			description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.Usage = D3D11_USAGE_DYNAMIC;
			description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			if (!CheckGraphics(graphics.device->CreateTexture2D(&description, nullptr, &graphics.hudTexture))) return false;
			if (!CheckGraphics(graphics.device->CreateShaderResourceView(graphics.hudTexture, nullptr, &graphics.hudImage))) return false;
			graphics.hudWidth = mHudWidth;
			graphics.hudHeight = mHudHeight;
		}
		D3D11_MAPPED_SUBRESOURCE mapped{ };
		if (!CheckGraphics(graphics.context->Map(graphics.hudTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
		for (int row = 0; row < mHudHeight; ++row) std::memcpy(static_cast<std::uint8_t*>(mapped.pData) + row * mapped.RowPitch, mHudBitmap->mTrueColorMemory + static_cast<std::size_t>(row) * mHudBitmap->mRowPixelCount, static_cast<std::size_t>(mHudWidth) * 4);
		graphics.context->Unmap(graphics.hudTexture, 0);
		const RECT area = GetPresentationRectangle();
		const LONG width = area.right - area.left;
		const LONG height = area.bottom - area.top;
		const bool integerScale = width % mHudWidth == 0 && height % mHudHeight == 0 && width / mHudWidth == height / mHudHeight;
		ID3D11SamplerState* sampler = integerScale ? graphics.pointSampler : graphics.linearSampler;
		graphics.context->PSSetSamplers(0, 1, &sampler);
		graphics.context->PSSetShader(graphics.hudShader, nullptr, 0);
		graphics.context->PSSetShaderResources(0, 1, &graphics.hudImage);
		graphics.context->OMSetBlendState(graphics.hudBlend, nullptr, 0xFFFFFFFF);
		graphics.context->Draw(3, 0);
		graphics.context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
		return true;
	}

	// Replaces Windows 0047B9CE..0047C21E; bitmap origin does not affect placement.
	void CDisplay::BitmapToScreen(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y)
	{
		if (bitmap == nullptr || mPixels == nullptr) return;
		if (IsHudBitmap(bitmap)) return;
		mVideoActive = false;
		// Loading screens can still update after the next desktop has been created.
		mPresentHud = bitmap == mWorldComposition;
		const int left = (std::max)(0, x);
		const int top = (std::max)(0, y);
		const int right = static_cast<int>((std::min)(static_cast<std::int64_t>(mWidth), static_cast<std::int64_t>(x) + bitmap->mRectangle.width));
		const int bottom = static_cast<int>((std::min)(static_cast<std::int64_t>(mHeight), static_cast<std::int64_t>(y) + bitmap->mRectangle.height));
		if (right <= left || bottom <= top) return;
		const auto* palette = bitmap->mBytesPerPixel == 1 ? bitmap->GetTrueColorTablePtr() : nullptr;
		const auto* conversion = bitmap->mBytesPerPixel == 2 ? NXBasics::g16BitPixelConverter->GetTrueColorConvertTablePtr() : nullptr;
		if ((bitmap->mBytesPerPixel == 1 && palette == nullptr) || (bitmap->mBytesPerPixel == 2 && conversion == nullptr)) return;
		for (int row = top; row < bottom; ++row)
		{
			const std::size_t source = static_cast<std::size_t>(row - y) * bitmap->mRowPixelCount + left - x;
			auto* destination = mPixels + static_cast<std::size_t>(row) * mWidth + left;
			if (bitmap->mBytesPerPixel == 4) std::memcpy(destination, bitmap->mTrueColorMemory + source, (right - left) * sizeof(std::uint32_t));
			else for (int column = 0; column < right - left; ++column)
			{
				if (palette != nullptr) destination[column] = palette[bitmap->mRawMemory[source + column]];
				else if (conversion != nullptr) destination[column] = conversion[bitmap->mHighColorMemory[source + column]];
			}
		}
	}

	// Legacy MPEG decoding is isolated from window presentation and monitor modes.
	bool CDisplay::VideoFrameToScreen(const void* pixels, LONG pitch, std::int32_t width, std::int32_t height, std::uint32_t bits, std::uint32_t redMask, std::uint32_t greenMask, std::uint32_t blueMask)
	{
		if (mPixels == nullptr || pixels == nullptr || !IsSupportedMode(width, height, 32) || (bits != 16 && bits != 24 && bits != 32) || redMask == 0 || greenMask == 0 || blueMask == 0) return false;
		if (mVideoFrame == nullptr || mVideoFrame->GetRectangle().width != width || mVideoFrame->GetRectangle().height != height)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap));
			if (memory == nullptr) return false;
			auto* frame = ::new(memory) NXBasics::CBitmap(width, height, 2);
			delete mVideoFrame;
			mVideoFrame = frame;
		}
		// Keep decoded pixels at their source size; the GPU performs one filtered
		// enlargement instead of baking a nearest-neighbor copy into the game frame.
		for (int row = 0; row < height; ++row)
		{
			const auto* source = static_cast<const std::uint8_t*>(pixels) + static_cast<std::ptrdiff_t>(row) * pitch;
			auto* destination = static_cast<std::uint32_t*>(mVideoFrame->GetUnclippedPixelPtr(0, row));
			for (int column = 0; column < width; ++column)
			{
				std::uint32_t pixel = 0;
				std::memcpy(&pixel, source + column * (bits / 8), bits / 8);
				destination[column] = (ExtractChannel(pixel, redMask) << 16) | (ExtractChannel(pixel, greenMask) << 8) | ExtractChannel(pixel, blueMask);
			}
		}
		mVideoActive = true;
		return true;
	}
}
