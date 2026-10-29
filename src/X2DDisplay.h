#pragma once

#include <Windows.h>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
}

namespace NXSys2DApp
{
	// Remaster presentation backend; no native object-layout contract.
	class CDisplay
	{
	  public:
		explicit CDisplay(bool fullscreen);
		~CDisplay();
		bool StartUp(HWND windowHandle, std::int32_t width, std::int32_t height, std::int32_t depth);
		void ShutDown();
		bool SetDisplayMode(std::int32_t width, std::int32_t height, std::int32_t depth);
		bool ToggleFullscreen();
		void Update();
		void BitmapToScreen(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y);
		bool VideoFrameToScreen(const void* pixels, LONG pitch, std::int32_t width, std::int32_t height, std::uint32_t bits, std::uint32_t redMask, std::uint32_t greenMask, std::uint32_t blueMask);
		void UpdateWindowPlacement();
		RECT GetPresentationRectangle() const;
		POINT ScreenToGame(POINT point) const;
		POINT GameToScreen(POINT point) const;

		std::int32_t GetWidth() const
		{

			return mWidth;
		}

		std::int32_t GetHeight() const
		{

			return mHeight;
		}

		std::int32_t GetDepth() const
		{

			return mDepth;
		}

		bool IsFullscreen() const
		{

			return mFullscreen;
		}

		std::int32_t GetWindowOffsetX() const;
		std::int32_t GetWindowOffsetY() const;
		static bool IsSupportedMode(std::uint32_t width, std::uint32_t height, std::uint32_t depth);
		bool ConfigureHud(std::int32_t& width, std::int32_t& height);
		void DisableHud();

		void SetHudBitmap(NXBasics::CBitmap* bitmap)
		{
			mHudBitmap = bitmap;
		}

		void SetHudScratchBitmap(NXBasics::CBitmap* bitmap)
		{
			mHudScratchBitmap = bitmap;
		}

		bool IsHudBitmap(const NXBasics::CBitmap* bitmap) const;

		bool RegisterMainWorld(void* element)
		{
			if (mWorldComposition == nullptr || mMainWorldElement != nullptr)
				return false;

			mMainWorldElement = element;

			return true;
		}

		bool IsMainWorld(const void* element) const
		{

			return element != nullptr && element == mMainWorldElement;
		}

		NXBasics::CBitmap* GetWorldComposition() const
		{

			return mWorldComposition;
		}

		void HudToWorld(std::int32_t* point) const;
		void WorldToHud(std::int32_t* point) const;

		void SetMainWorldBitmap(NXBasics::CBitmap* bitmap)
		{
			mMainWorldBitmap = bitmap;
		}

		bool IsMainWorldBitmap(const NXBasics::CBitmap* bitmap) const;

		NXBasics::CBitmap* GetHudBitmap() const
		{

			return mHudBitmap;
		}

		void CopyCompositedFrame(NXBasics::CBitmap* bitmap) const;
		static void CompositeHudImage(const NXBasics::CBitmap* source, NXBasics::CBitmap* destination, std::int32_t x, std::int32_t y);

	  private:
		struct SGraphics;
		bool CreateGraphics();
		void ReleaseGraphics();
		bool CreateFrameTexture();
		bool ResizeTarget();
		bool DrawHud();
		HWND mWindowHandle = nullptr;
		std::int32_t mWidth = 0;
		std::int32_t mHeight = 0;
		std::int32_t mDepth = 0;
		bool mFullscreen;
		bool mHasWindowedPlacement = false;
		RECT mWindowedRectangle{ };
		std::uint32_t* mPixels = nullptr;
		SGraphics* mGraphics = nullptr;
		NXBasics::CBitmap* mHudBitmap = nullptr;
		NXBasics::CBitmap* mHudScratchBitmap = nullptr;
		NXBasics::CBitmap* mWorldComposition = nullptr;
		NXBasics::CBitmap* mMainWorldBitmap = nullptr;
		NXBasics::CBitmap* mVideoFrame = nullptr;
		bool mVideoActive = false;
		bool mPresentHud = false;
		void* mMainWorldElement = nullptr;
		std::int32_t mHudWidth = 0;
		std::int32_t mHudHeight = 0;
	};

	// Only the dedicated GUI plane uses its high byte as inverse coverage.
	bool IsRemasterHudBitmap(const NXBasics::CBitmap* bitmap);
}