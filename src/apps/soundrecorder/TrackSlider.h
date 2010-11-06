/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */
#ifndef TRACKSLIDER_H
#define TRACKSLIDER_H

#include <Control.h>
#include <Bitmap.h>
#include <Font.h>

class SliderOffscreenView : public BView {

public:
	SliderOffscreenView(BRect frame, const char *name); 
	virtual		~SliderOffscreenView();
	virtual	void	DrawX();
	
	BBitmap leftBitmap, rightBitmap;
	BBitmap leftThumbBitmap, rightThumbBitmap;
	float fRight;
	
	float fLeftX;
	float fRightX;
	float fPositionX;
	float fLastX;
};

class TrackSlider : public BControl
{
public:
	TrackSlider(BRect rect, const char* title, BMessage *msg, uint32 resizeFlags);
	~TrackSlider();
	void AttachedToWindow();
	virtual void Draw(BRect);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual void MouseUp(BPoint point);
	virtual void MouseDown(BPoint point);
	void SetMainTime(bigtime_t timestamp, bool reset);
	void SetTotalTime(bigtime_t timestamp, bool reset);
	bigtime_t * MainTime() { return &fMainTime; };
	bigtime_t RightTime() { return fRightTime; };
	bigtime_t LeftTime() { return fLeftTime; };
	void ResetMainTime();
	virtual void FrameResized(float width, float height);
private:
	void _DrawCounter(bigtime_t timestamp, float position, bool isTracking);
	void _DrawMarker(float position);
	void _TimeToString(bigtime_t timestamp, char *string);
	void _UpdatePosition(BPoint point);
	void _InitBitmap();
	void _RenderBitmap();
	
	
	bigtime_t fLeftTime;
	bigtime_t fRightTime;
	bigtime_t fMainTime;
	bigtime_t fTotalTime;
	
	bool fLeftTracking;
	bool fRightTracking;
	bool fMainTracking;
	
	BFont fFont;
	BBitmap *fBitmap;
	SliderOffscreenView *fBitmapView;
	
};


#endif	/* TRACKSLIDER_H */
