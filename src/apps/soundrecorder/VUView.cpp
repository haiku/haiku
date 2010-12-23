/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: 
 * 	Consumers and Producers)
 */

#include <stdio.h>
#include <string.h>

#include <MediaDefs.h>
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
	for (int channel = 0; channel < fChannels; channel++)
		fCurrentLevels[channel] = 0;
	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	
	
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
	
	fBitmapView = new BView(rect, "bitmapView", B_FOLLOW_LEFT|B_FOLLOW_TOP, 
		B_WILL_DRAW);
	fBitmap->AddChild(fBitmapView);
}


VUView::~VUView()
{
	delete fBitmap;
}


void
VUView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	_Run();
}


void
VUView::DetachedFromWindow()
{
	_Quit();
}


void
VUView::Draw(BRect updateRect)
{
	DrawBitmap(fBitmap);
		
	Sync();
}


void
VUView::_Run()
{
	fThreadId = spawn_thread(_RenderLaunch, "VU view", B_NORMAL_PRIORITY, 
		this);
	if (fThreadId < 0)
		return;
	resume_thread(fThreadId);
}

void
VUView::_Quit()
{
	fQuitting = true;
	snooze(10000);
	kill_thread(fThreadId);
}



int32
VUView::_RenderLaunch(void *data)
{
	VUView *vu = (VUView*) data;
	vu->_RenderLoop();
	return B_OK;
}


#define SHIFT_UNTIL(value,shift,min) \
	value = (value - shift > min) ? (value - shift) : min

void
VUView::_RenderLoop()
{
	rgb_color levels[fLevelCount][2];
	
	for (int32 i = 0; i < fLevelCount; i++) {
		levels[i][0] = levels[i][1] = back_color;	
	}
	
	while (!fQuitting) {
		
		/* computing */
		for (int32 channel = 0; channel < 2; channel++) {
			int32 level = fCurrentLevels[channel];
			for (int32 i = 0; i < level; i++) {
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
			
			for (int32 i = level + 1; i < fLevelCount; i++) {
				if (levels[i][channel].red >= 85) {
					SHIFT_UNTIL(levels[i][channel].red, 15, back_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 15, back_color.blue);
				} else {
					SHIFT_UNTIL(levels[i][channel].red, 7, back_color.red);
					SHIFT_UNTIL(levels[i][channel].blue, 7, back_color.blue);
					SHIFT_UNTIL(levels[i][channel].green, 14, 
						back_color.green);
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
		for (int32 i = fLevelCount - 1; i >= 0; i--) {
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


template<typename T>
T 
VUView::_ComputeNextLevel(const void *data, size_t size, uint32 format, 
	int32 channel)
{
	const T* samp = (const T*)data;
	
	// get the min and max values in the nibbling interval
	// and set max to be the greater of the absolute value
	// of these.
	
	T min = 0, max = 0;
	for (uint32 i = channel; i < size/sizeof(T); i += fChannels) {
		if (min > samp[i])
			min = samp[i];
		else if (max < samp[i])
			max = samp[i];
	}
	if (-max > (min + 1))
		max = -min;
		
	return max;
}


void
VUView::ComputeLevels(const void* data, size_t size, uint32 format)
{
	for (int32 channel = 0; channel < fChannels; channel++) {
		switch (format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
			{
				float max = _ComputeNextLevel<float>(data, size, format, 
					channel);
				fCurrentLevels[channel] = (uint8)(max * 127);
				break;
			}
			case media_raw_audio_format::B_AUDIO_INT:
			{
				int32 max = _ComputeNextLevel<int32>(data, size, format, 
					channel);
				fCurrentLevels[channel] = max / (2 << (32-7));
				break;
			}
			case media_raw_audio_format::B_AUDIO_SHORT:
			{
				int16 max = _ComputeNextLevel<int16>(data, size, format, 
					channel);
				fCurrentLevels[channel] = max / (2 << (16-7));
				break;
			}
			case media_raw_audio_format::B_AUDIO_UCHAR:
			{
				uchar max = _ComputeNextLevel<uchar>(data, size, format, 
					channel);
				fCurrentLevels[channel] = max / 2 - 127;
				break;
			}
			case media_raw_audio_format::B_AUDIO_CHAR:
			{
				char max = _ComputeNextLevel<char>(data, size, format, 
					channel);
				fCurrentLevels[channel] = max / 2;
				break;
			}
		}
		if (fCurrentLevels[channel] < 0)
			fCurrentLevels[channel] = 0;
	}
}
