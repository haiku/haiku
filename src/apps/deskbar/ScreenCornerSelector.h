/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_CORNER_SELECTOR_H
#define SCREEN_CORNER_SELECTOR_H


#include <Control.h>
#include <Deskbar.h>


static const int32 kExpandBit = 1 << 3;


class ScreenCornerSelector : public BControl {
public:
								ScreenCornerSelector(BRect frame,
									const char *name, BMessage* message,
									uint32 resizingMode);

	virtual	void				Draw(BRect updateRect);
	virtual status_t			Invoke(BMessage* message = NULL);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint point);

	virtual	int32				Value();
	virtual	void				SetValue(int32 value);

			deskbar_location	Corner() const;
			void				SetCorner(deskbar_location corner);

private:
			BRect				_MonitorFrame() const;
			BRect				_InnerFrame(BRect monitorFrame) const;
			void				_DrawArrow(BRect innerFrame);
			int32				_ScreenCorner(BPoint point) const;

			int32				fCurrentCorner;
			bool				fDragging;
};


#endif	// SCREEN_CORNER_SELECTOR_H
