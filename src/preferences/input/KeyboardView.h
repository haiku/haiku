/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/


#ifndef KEYBOARD_VIEW_H
#define KEYBOARD_VIEW_H


#include <GroupView.h>
#include <Slider.h>
#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <Application.h>


class KeyboardView : public BGroupView
{
public:
	KeyboardView();
	virtual ~KeyboardView();
	void	Draw(BRect frame);

private:
	BBitmap		*fIconBitmap;
	BBitmap		*fClockBitmap;
	BSlider		*fDelaySlider;
	BSlider		*fRepeatSlider;
};

#endif