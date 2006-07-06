/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#include <stdio.h>
#include <string.h>
#include <Screen.h>
#include <Window.h>
#include "DrawingTidbits.h"
#include "VUView.h"

const rgb_color back_color = {12, 36, 12};
const rgb_color low_color = {40, 120, 40};
const rgb_color high_color = {240, 255, 240};

VUView::VUView(BRect rect, uint32 resizeFlags)
	: BView(rect, "vumeter", resizeFlags, B_WILL_DRAW),
	fThreadId(-1),
	fBitmap(NULL),
	fQuitting(false)
{
	rect.OffsetTo(B_ORIGIN);
	fLevelCount = int(rect.Height()) / 2;
	fChannels = 2;
	fCurrentLevels = new int32[fChannels];
	for (int channel=0; channel < fChannels; channel++)
		fCurrentLevels[channel] = 0;
	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	
	
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
	
	fBitmapView = new BView(rect, "bitmapView", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
	fBitmap->AddChild(fBitmapView);
}


VUView::~VUView()
{
	
}


void
VUView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	Run();
}


void
VUView::DetachedFromWindow()
{
	Quit();
}


void
VUView::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap);
		
	Sync();
}


void
VUView::Run()
{
	fThreadId = spawn_thread(&RenderLaunch, "VU view", B_NORMAL_PRIORITY, this);
	if (fThreadId < 0)
		return;
	resume_thread(fThreadId);
}

void
VUView::Quit()
{
	fQuitting = true;
	snooze(10000);
	kill_thread(fThreadId);
}



int32
VUView::RenderLaunch(void *data)
{
	VUView *vu = (VUView*) data;
	vu->RenderLoop();
	return B_OK;
}


#define SHIFT_UNTIL(value,shift,min) value = (value - shift > min) ? (value - shift) : min

void
VUView::RenderLoop()
{
	rgb_color levels[fLevelCount][2];
	
	for (int32 i=0; i<fLevelCount; i++) {
		levels[i][0] = levels[i][1] = back_color;	
	}
	
	int32 level = 0;
	
	while (!fQuitting) {
		
		/* computing */
		for (int32 channel = 0; channel < 2; channel++) {
			level = fCurrentLevels[channel];
			for (int32 i=0; i<level; i++) {
				if (levels[i][channel].red >= 90) {
					SHIFT_UNTIL(levels[i][channel].red, 15, low_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 15, low_color.blue);
				} else {
					SHIFT_UNTIL(levels[i][channel].red, 7, low_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 7, low_color.blue);
					SHIFT_UNTIL(levels[i][channel].green, 14, low_color.green);
				}
			}
			
			levels[level][channel] = high_color;
			
			for (int32 i=level+1; i<fLevelCount; i++) {
				if (levels[i][channel].red >= 85) {
					SHIFT_UNTIL(levels[i][channel].red, 15, back_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 15, back_color.blue);
				} else {
					SHIFT_UNTIL(levels[i][channel].red, 7, back_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 7, back_color.blue);
					SHIFT_UNTIL(levels[i][channel].green, 14, back_color.green);
				}
			}
		}
		
		/* rendering */
		fBitmap->Lock();
		fBitmapView->BeginLineArray(fLevelCount * 2);
		BPoint start1, end1, start2, end2;
		start1.x = 3;
		start2.x = 22;
		end1.x = 16;
		end2.x = 35;
		start1.y = end1.y = start2.y = end2.y = 2;
		for (int32 i=fLevelCount-1; i>=0; i--) {
			fBitmapView->AddLine(start1, end1, levels[i][0]);
			fBitmapView->AddLine(start2, end2, levels[i][1]);
			start1.y = end1.y = start2.y = end2.y = end2.y + 2;
		}
		fBitmapView->EndLineArray();
		fBitmap->Unlock();
	
		/* ask drawing */
	
		if (Window()->LockWithTimeout(5000) == B_OK) {
			Invalidate();
			Window()->Unlock();
			snooze(50000);
		}
	}	
}


void 
VUView::ComputeNextLevel(void *data, size_t size)
{
	int16* samp = (int16*)data;
	
	for (int32 channel = 0; channel < fChannels; channel++) {
	
		// get the min and max values in the nibbling interval
		// and set max to be the greater of the absolute value
		// of these.
		
		int mi = 0, ma = 0;
		for (uint32 ix=channel; ix<size/sizeof(uint16); ix += fChannels) {
			if (mi > samp[ix]) mi = samp[ix];
			else if (ma < samp[ix]) ma = samp[ix];
		}
		if (-ma > mi) ma = (mi == -32768) ? 32767 : -mi;
		
		uint8 n = ma / (2 << (16-7));
		
		fCurrentLevels[channel] = n;
		if (fCurrentLevels[channel] < 0)
			fCurrentLevels[channel] = 0;
	}

}
