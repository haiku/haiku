/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 */
#ifndef MOUSEAREAVIEW_H
#define MOUSEAREAVIEW_H


#include "ScreenSaverPrefs.h"

#include <View.h>


class MouseAreaView : public BView {
	public:
		MouseAreaView(BRect frame, const char *name, uint32 resizingMode);

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

#endif	// MOUSEAREAVIEW_H
