/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef MAGNETIC_BORDRER_H
#define MAGNETIC_BORDRER_H


#include <Point.h>
#include <Screen.h>


class Screen;
class Window;


class MagneticBorder {
public:
								MagneticBorder();

			bool				AlterDeltaForSnap(Window* window, BPoint& delta,
									bigtime_t now);
			bool				AlterDeltaForSnap(const Screen* screen,
									BRect& frame, BPoint& delta, bigtime_t now);

private:
			bigtime_t			fLastSnapTime;
};


#endif // MAGNETIC_BORDRER_H
