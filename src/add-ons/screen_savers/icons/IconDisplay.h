/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vincent Duvert, vincent.duvert@free.fr
 *		John Scipione, jscipione@gmail.com
 */
#ifndef ICON_DISPLAY_H
#define ICON_DISPLAY_H


#include <Rect.h>


struct vector_icon;


class BBitmap;
class BView;


class IconDisplay {
public:
								IconDisplay();
								~IconDisplay();

			void				Run(vector_icon* icon, BRect frame);
	inline	bool				IsRunning() const { return fIsRunning; };
	inline	BRect				GetFrame() const { return fFrame; };

			void				ClearOn(BView* view);
			void				DrawOn(BView* view, uint32 delta);

private:
			bool				fIsRunning;
			uint8				fState;

			int32				fTicks;
			int32				fDelay;

			BBitmap*			fBitmap;
			BRect				fFrame;
};


#endif	// ICON_DISPLAY_H
