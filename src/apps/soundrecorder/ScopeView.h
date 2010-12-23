/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#ifndef SCOPEVIEW_H
#define SCOPEVIEW_H

#include <Bitmap.h>
#include <View.h>
#include <MediaTrack.h>


class ScopeView : public BView
{
public:
	ScopeView(BRect rect, uint32 resizeFlags);
	~ScopeView();
	void AttachedToWindow();
	void DetachedFromWindow();
	void Draw(BRect updateRect);
	void SetMainTime(bigtime_t timestamp);
	void SetLeftTime(bigtime_t timestamp);
	void SetRightTime(bigtime_t timestamp);
	void SetTotalTime(bigtime_t timestamp, bool reset);
	void RenderTrack(BMediaTrack *track, const media_format &format);
	void CancelRendering();
	virtual void FrameResized(float width, float height);
	virtual void MouseDown(BPoint position);
private:
	void Run();
	void Quit();
	static int32 RenderLaunch(void *data);
	void RenderLoop();
	template<typename T, typename U> void ComputeRendering();
	void RenderBitmap();
	void InitBitmap();
	
	thread_id fThreadId;
	BBitmap *fBitmap;
	BView *fBitmapView;
	sem_id fRenderSem;
	bool fIsRendering;
	BMediaTrack *fMediaTrack;
	media_format fPlayFormat;
	
	bigtime_t fMainTime;
	bigtime_t fRightTime;
	bigtime_t fLeftTime;
	bigtime_t fTotalTime;
	int32 fPreview[20000];
	float fHeight;
};

#endif	/* SCOPEVIEW_H */
