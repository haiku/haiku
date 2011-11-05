/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCREEN_H
#define _SCREEN_H


#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>


class BBitmap;
class BWindow;

namespace BPrivate {
	class BPrivateScreen;
}


class BScreen {
public:  
								BScreen(screen_id id = B_MAIN_SCREEN_ID);
								BScreen(BWindow* window);
								~BScreen();

			bool				IsValid();
			status_t			SetToNext();

			color_space			ColorSpace();
			BRect				Frame();
			screen_id			ID();

			status_t			WaitForRetrace();
			status_t			WaitForRetrace(bigtime_t timeout);

			uint8				IndexForColor(rgb_color color);
			uint8				IndexForColor(uint8 red, uint8 green,
									uint8 blue, uint8 alpha = 255);
			rgb_color			ColorForIndex(uint8 index);
			uint8				InvertIndex(uint8 index);

			const color_map*	ColorMap();

			status_t			GetBitmap(BBitmap** _bitmap,
									bool drawCursor = true,
									BRect* frame = NULL);
			status_t			ReadBitmap(BBitmap* bitmap,
									bool drawCursor = true,
									BRect* frame = NULL);

			rgb_color			DesktopColor();
			rgb_color			DesktopColor(uint32 workspace);
			void				SetDesktopColor(rgb_color color,
									bool stick = true);
			void				SetDesktopColor(rgb_color color,
									uint32 workspace, bool stick = true);

			status_t			ProposeMode(display_mode* target,
									const display_mode* low,
									const display_mode* high);
			status_t			GetModeList(display_mode** _modeList,
									uint32* _count);
			status_t			GetMode(display_mode* mode);
			status_t			GetMode(uint32 workspace,
									display_mode* mode);
			status_t			SetMode(display_mode* mode,
									bool makeDefault = false);
			status_t			SetMode(uint32 workspace,
									display_mode* mode,
									bool makeDefault = false);
			status_t			GetDeviceInfo(accelerant_device_info* info);
			status_t			GetMonitorInfo(monitor_info* info);
			status_t			GetPixelClockLimits(display_mode* mode,
									uint32* _low, uint32* _high);
			status_t			GetTimingConstraints(
									display_timing_constraints*
										timingConstraints);
			status_t			SetDPMS(uint32 state);
			uint32				DPMSState();
			uint32				DPMSCapabilites();

private:
	// Forbidden and deprecated methods
								BScreen(const BScreen& other);
			BScreen&			operator=(const BScreen& other);

			BPrivate::BPrivateScreen* private_screen();
			status_t			ProposeDisplayMode(display_mode* target,
									const display_mode* low,
									const display_mode* high);
			void*				BaseAddress();
			uint32				BytesPerRow();

private:
			BPrivate::BPrivateScreen* fScreen;
};


inline uint8
BScreen::IndexForColor(rgb_color color)
{
	return IndexForColor(color.red, color.green, color.blue, color.alpha);
}

#endif // _SCREEN_H
