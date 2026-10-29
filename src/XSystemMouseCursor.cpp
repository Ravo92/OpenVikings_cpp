#include "XSystemMouseCursor.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"

#include <Windows.h>
#include <algorithm>

namespace NXSysMisc
{
	namespace
	{
		// Original global: DAT_00554d60.
		HCURSOR gCustomCursor = nullptr;

		// Original global: DAT_00504e5c.
		bool gSystemMouseCursorShown = true;

		// Remaster-only cache; original cursor handles remain borrowed.
		struct SScaledCursor
		{
			HCURSOR source = nullptr;
			HCURSOR image = nullptr;
			LONG sourceWidth = 0, sourceHeight = 0;
			DWORD hotspotX = 0, hotspotY = 0;
			int width = 0, height = 0;
		};

		SScaledCursor gScaledCursor;

		void ReleaseScaledCursor()
		{
			if (gScaledCursor.image != nullptr)
				DestroyCursor(gScaledCursor.image);

			gScaledCursor = { };
		}

		void SetScaledCustomCursor(HCURSOR source)
		{
			if (gScaledCursor.source != source)
			{
				// Switch away before destroying the previously selected image.
				SetCursor(source);
				ReleaseScaledCursor();
				ICONINFO info{ };

				if (!GetIconInfo(source, &info))
					return;

				BITMAP bitmap{ };
				const bool sized = GetObjectW(info.hbmColor != nullptr ? info.hbmColor : info.hbmMask, sizeof(bitmap), &bitmap) != 0;

				if (info.hbmColor != nullptr)
					DeleteObject(info.hbmColor);

				if (info.hbmMask != nullptr)
					DeleteObject(info.hbmMask);

				if (!sized || bitmap.bmWidth <= 0 || bitmap.bmHeight <= 0)
					return;

				gScaledCursor.source = source;
				gScaledCursor.sourceWidth = bitmap.bmWidth;
				gScaledCursor.sourceHeight = info.hbmColor != nullptr ? bitmap.bmHeight : bitmap.bmHeight / 2;
				gScaledCursor.hotspotX = info.xHotspot;
				gScaledCursor.hotspotY = info.yHotspot;
			}

			if (gScaledCursor.sourceHeight <= 0)
			{

				return;
			}

			LONG outputHeight = 768;

			if (auto* application = NXSys2DApp::CApplication::sTheObjectPtr)
			{
				if (auto* display = application->GetDisplay())
				{
					const RECT area = display->GetPresentationRectangle();

					if (area.bottom > area.top)
						outputHeight = area.bottom - area.top;
				}
			}

			const int width = (std::max)(1, MulDiv(gScaledCursor.sourceWidth, outputHeight, 768));
			const int height = (std::max)(1, MulDiv(gScaledCursor.sourceHeight, outputHeight, 768));

			if (width != gScaledCursor.width || height != gScaledCursor.height)
			{
				HCURSOR scaled = nullptr;

				if (width != gScaledCursor.sourceWidth || height != gScaledCursor.sourceHeight)
				{
					scaled = static_cast<HCURSOR>(CopyImage(source, IMAGE_CURSOR, width, height, 0));

					if (scaled != nullptr)
					{
						ICONINFO info{ };

						if (GetIconInfo(scaled, &info))
						{
							const DWORD hotspotX = static_cast<DWORD>(MulDiv(gScaledCursor.hotspotX, width, gScaledCursor.sourceWidth));
							const DWORD hotspotY = static_cast<DWORD>(MulDiv(gScaledCursor.hotspotY, height, gScaledCursor.sourceHeight));

							if (info.xHotspot != hotspotX || info.yHotspot != hotspotY)
							{
								info.xHotspot = hotspotX;
								info.yHotspot = hotspotY;

								if (HCURSOR corrected = static_cast<HCURSOR>(CreateIconIndirect(&info)))
								{
									DestroyCursor(scaled);
									scaled = corrected;
								}
								else
								{
									DestroyCursor(scaled);
									scaled = nullptr;
								}
							}

							if (info.hbmColor != nullptr)
								DeleteObject(info.hbmColor);

							if (info.hbmMask != nullptr)
								DeleteObject(info.hbmMask);
						}
					}
				}

				SetCursor(scaled != nullptr ? scaled : source);

				if (gScaledCursor.image != nullptr)
					DestroyCursor(gScaledCursor.image);

				gScaledCursor.image = scaled;
				gScaledCursor.width = width;
				gScaledCursor.height = height;
			}
			else
			{
				SetCursor(gScaledCursor.image != nullptr ? gScaledCursor.image : source);
			}
		}
	}

	// Original function: FUN_00478b1b
	void __stdcall XWS_SystemMouseCursor_SetNormal()
	{
		const HCURSOR cursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7F00));
		SetCursor(cursor);
		ReleaseScaledCursor();

		gCustomCursor = nullptr;
	}

	// Original function: FUN_00478b37
	void __stdcall XWS_SystemMouseCursor_SetWaitMode()
	{
		const HCURSOR cursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(0x7F02));
		SetCursor(cursor);
		ReleaseScaledCursor();

		gCustomCursor = reinterpret_cast<HCURSOR>(static_cast<INT_PTR>(-1));
	}

	// Original function: FUN_00478b53
	void __cdecl XWS_SystemMouseCursor_SetCustom(HCURSOR cursor)
	{
		gCustomCursor = cursor;

		if (cursor != nullptr)
		{
			SetScaledCustomCursor(cursor);
		}
		else
		{
			XWS_SystemMouseCursor_SetNormal();
		}

		if (!gSystemMouseCursorShown)
		{
			ShowCursor(TRUE);
			gSystemMouseCursorShown = true;
		}
	}

	// Original function: FUN_00478b94
	void __stdcall XWS_SystemMouseCursor_Reset()
	{
		if (!gSystemMouseCursorShown)
		{
			SetCursor(nullptr);
			return;
		}

		if (gCustomCursor == nullptr)
		{
			XWS_SystemMouseCursor_SetNormal();
			return;
		}

		if (gCustomCursor == reinterpret_cast<HCURSOR>(static_cast<INT_PTR>(-1)))
		{
			XWS_SystemMouseCursor_SetWaitMode();

			return;
		}

		XWS_SystemMouseCursor_SetCustom(gCustomCursor);
	}

	// Original function: FUN_00478bc6
	bool __stdcall XWS_SystemMouseCursor_IsShown()
	{

		return gSystemMouseCursorShown;
	}

	// Original function: FUN_00478bcc
	void __stdcall XWS_SystemMouseCursor_Hide()
	{
		if (!gSystemMouseCursorShown)
		{

			return;
		}

		// Keep decrementing the display counter until it is negative.

		while (ShowCursor(FALSE) >= 0)
		{
		}

		gSystemMouseCursorShown = false;
	}

	// Original function: FUN_00478be9
	void __stdcall XWS_SystemMouseCursor_Show()
	{
		if (gSystemMouseCursorShown)
		{

			return;
		}

		// The original increments the display counter only once.
		ShowCursor(TRUE);
		gSystemMouseCursorShown = true;
	}
}