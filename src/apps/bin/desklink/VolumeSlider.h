// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  Program:	 desklink
//  Author:      Jérôme DUVAL
//  Description: VolumeControl and link items in Deskbar
//  Created :    October 20, 2003
//	Modified by: Jérome Duval
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <Window.h>
#include <Control.h>
#include <Bitmap.h>
#include <ParameterWeb.h>

class SliderView : public BControl
{
public:
	SliderView(BRect rect, BMessage *msg, const char* title, uint32 resizeFlags, int32 value);
	~SliderView();
	virtual void Draw(BRect);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual void MouseUp(BPoint point);
	virtual void Pulse();
private:
	BBitmap leftBitmap, rightBitmap, buttonBitmap;
	float fTrackingX;
	const char* fTitle;
};

class VolumeSlider : public BWindow
{
public:
	VolumeSlider(BRect frame);
	~VolumeSlider();

	void MessageReceived(BMessage*);
	void WindowActivated(bool active);
private:
	media_node *aOutNode;
	BParameterWeb* paramWeb;
	BContinuousParameter* mixerParam;
	float min, max, step;
	SliderView *slider;
};

#endif	
