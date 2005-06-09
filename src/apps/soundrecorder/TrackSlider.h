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
	void DrawCounter(bigtime_t timestamp, float position, bool isTracking);
	void TimeToString(bigtime_t timestamp, char *string);
	void UpdatePosition(BPoint point);
	BBitmap leftBitmap, rightBitmap, leftThumbBitmap, rightThumbBitmap;
	float fRight;
	
	bigtime_t fLeftTime;
	bigtime_t fRightTime;
	bigtime_t fMainTime;
	bigtime_t fTotalTime;
	
	float fPositionX;
	float fLeftX;
	float fRightX;
	float fLastX;
	bool fLeftTracking;
	bool fRightTracking;
	bool fMainTracking;
	
	BFont fFont;
};

#endif	/* TRACKSLIDER_H */
