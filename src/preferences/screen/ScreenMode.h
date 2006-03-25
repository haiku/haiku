/*
 * Copyright 2005, Haiku.
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

		status_t Set(const screen_mode& mode);
		status_t Get(screen_mode& mode);
		status_t Revert();

		void UpdateOriginalMode();

		bool SupportsColorSpace(const screen_mode& mode, color_space space);
		status_t GetRefreshLimits(const screen_mode& mode, float& min, float& max);

		screen_mode ModeAt(int32 index);
		int32 CountModes();

	private:
		bool GetDisplayMode(const screen_mode& mode, display_mode& displayMode);

		BWindow*		fWindow;
		display_mode*	fModeList;
		uint32			fModeCount;

		bool			fUpdatedMode;
		display_mode	fOriginalDisplayMode;
		screen_mode		fOriginal;
};

#endif	/* SCREEN_MODE_H */
