/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 */
#ifndef SCREEN_CORNER_SELECTOR_H
#define SCREEN_CORNER_SELECTOR_H


#include "ScreenSaverSettings.h"

#include <View.h>


class ScreenCornerSelector : public BView {
	public:
		ScreenCornerSelector(BRect frame, const char *name, uint32 resizingMode);

		virtual void Draw(BRect update); 
		virtual void MouseUp(BPoint point);

		void DrawArrow();
		inline screen_corner Corner() { return fCurrentCorner; }
		void SetCorner(screen_corner direction);

	private:
		BRect _ArrowSize(BRect monitorRect, bool isCentered);

		BRect fMonitorFrame;
		screen_corner fCurrentCorner;
};

#endif	// SCREEN_CORNER_SELECTOR_H
