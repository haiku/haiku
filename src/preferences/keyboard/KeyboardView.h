/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#ifndef KEYBOARD_VIEW_H
#define KEYBOARD_VIEW_H


#include <Box.h>
#include <Slider.h>
#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <Application.h>

class KeyboardView : public BView
{
public:
			KeyboardView(BRect frame);
	void	Draw(BRect frame);
	void	AttachedToWindow(void);
	
private:
	BBitmap		*fIconBitmap;
	BBitmap		*fClockBitmap;
	BBox		*fBox;
	BSlider		*fDelaySlider;
	BSlider		*fRepeatSlider;
};

#endif
