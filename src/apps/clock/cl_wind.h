/*
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 */
#ifndef _CLOCK_WINDOW_H
#define _CLOCK_WINDOW_H


#include <Window.h>


class TOnscreenView;


class TClockWindow : public BWindow {
	public:
						TClockWindow(BRect rect, const char* name);
		virtual			~TClockWindow();

		virtual	bool	QuitRequested();

	private:
		void			_InitWindow();

	private:
		TOnscreenView	*fOnScreenView;
};

#endif	// _CLOCK_WINDOW_H

