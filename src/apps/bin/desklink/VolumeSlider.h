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
//      Modified by: François Revol, 10/31/2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <Window.h>
#include <Control.h>
#include <Bitmap.h>
#include <ParameterWeb.h>

#define VOLUME_USE_MIXER 0 /* default */
#define VOLUME_USE_PHYS_OUTPUT 1

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
	const char* fTitle;
};

class VolumeSlider : public BWindow
{
public:
	VolumeSlider(BRect frame, bool dontBeep=false, int32 volumeWhich=0);
	~VolumeSlider();

	void MessageReceived(BMessage*);
	void WindowActivated(bool active);
private:
	void UpdateVolume(BContinuousParameter* param);
	media_node *aOutNode;
	BParameterWeb* paramWeb;
	BContinuousParameter* mixerParam;
	float min, max, step;
	bool hasChanged;
	bool dontBeep;
	SliderView *slider;
};

#endif	
