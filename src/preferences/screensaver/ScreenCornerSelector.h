/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_CORNER_SELECTOR_H
#define SCREEN_CORNER_SELECTOR_H


#include "ScreenSaverSettings.h"

#include <Control.h>


class ScreenCornerSelector : public BControl {
	public:
		ScreenCornerSelector(BRect frame, const char *name, BMessage* message,
			uint32 resizingMode);

		virtual void Draw(BRect update); 
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage);
		virtual void KeyDown(const char* bytes, int32 numBytes);

		virtual void SetValue(int32 value);
		virtual int32 Value();

		void SetCorner(screen_corner corner);
		screen_corner Corner() const;

	private:
		BRect _MonitorFrame() const;
		BRect _InnerFrame(BRect monitorFrame) const;
		BRect _CenterFrame(BRect innerFrame) const;
		void _DrawStop(BRect innerFrame);
		void _DrawArrow(BRect innerFrame);
		screen_corner _ScreenCorner(BPoint point, screen_corner previous) const;

		screen_corner fCurrentCorner;
		int32 fPreviousCorner;
};

#endif	// SCREEN_CORNER_SELECTOR_H
