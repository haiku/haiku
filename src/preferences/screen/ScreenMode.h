/*
 * Copyright 2005-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_MODE_H
#define SCREEN_MODE_H


#include <Screen.h>


typedef enum {
	kCombineDisable,
	kCombineHorizontally,
	kCombineVertically
} combine_mode;

struct screen_mode {
	int32			width;		// these reflect the corrected width/height,
	int32			height;		// taking the combine mode into account
	color_space		space;
	float			refresh;
	combine_mode	combine;
	bool			swap_displays;
	bool			use_laptop_panel;
	uint32			tv_standard;

	void SetTo(display_mode& mode);
	int32 BitsPerPixel() const;

	bool operator==(const screen_mode &otherMode) const;
	bool operator!=(const screen_mode &otherMode) const;
};


class ScreenMode {
public:
								ScreenMode(BWindow* window);
								~ScreenMode();

			status_t			Set(const screen_mode& mode,
									int32 workspace = ~0);
			status_t			Get(screen_mode& mode,
									int32 workspace = ~0) const;
			status_t			GetOriginalMode(screen_mode &mode,
									int32 workspace = ~0) const;

			status_t			Set(const display_mode& mode,
									int32 workspace = ~0);
			status_t			Get(display_mode& mode,
									int32 workspace = ~0) const;

			status_t			Revert();
			void				UpdateOriginalModes();

			bool				SupportsColorSpace(const screen_mode& mode,
									color_space space);
			status_t			GetRefreshLimits(const screen_mode& mode,
									float& min, float& max);
			status_t			GetMonitorInfo(monitor_info& info,
									float* _diagonalInches = NULL);

			status_t			GetDeviceInfo(accelerant_device_info& info);

			screen_mode			ModeAt(int32 index);
			const display_mode&	DisplayModeAt(int32 index);
			int32				CountModes();

private:
			bool				_GetDisplayMode(const screen_mode& mode,
									display_mode& displayMode);

private:
			BWindow*			fWindow;
			display_mode*		fModeList;
			uint32				fModeCount;

			bool				fUpdatedModes;
			display_mode		fOriginalDisplayMode[32];
			screen_mode			fOriginal[32];
};


#endif	/* SCREEN_MODE_H */
