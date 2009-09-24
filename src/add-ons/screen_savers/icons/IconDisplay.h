/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/
#ifndef ICON_DISPLAY_H
#define ICON_DISPLAY_H

#include <Rect.h>
#include <View.h>

struct VectorIcon {
        uint8* data;
        size_t size;
};


class BBitmap;


class IconDisplay {
public:
								IconDisplay();
								~IconDisplay();
	
			void				Run(VectorIcon* icon, BRect frame);
	inline	bool				IsRunning() { return fIsRunning; };
	inline	BRect				GetFrame() { return fFrame; };

			void				ClearOn(BView* view);
			void				DrawOn(BView* view, uint32 delta);

private:
	bool						fIsRunning;
	uint8						fState;
	
	int32						fTicks;
	int32						fDelay;

	BBitmap*					fBitmap;
	BRect						fFrame;
};

#endif
