/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#ifndef VUVIEW_H
#define VUVIEW_H

#include <Bitmap.h>
#include <View.h>


class VUView : public BView
{
public:
	VUView(BRect rect, uint32 resizeFlags);
	~VUView();
	void AttachedToWindow();
	void DetachedFromWindow();
	void Draw(BRect updateRect);
	void ComputeNextLevel(void *data, size_t size);
		
private:
	void Run();
	void Quit();
	static int32 RenderLaunch(void *data);
	void RenderLoop();
	
	thread_id fThreadId;
	BBitmap *fBitmap;
	BView *fBitmapView;
	bool fQuitting;
	int32 fLevelCount;
	int32 *fCurrentLevels;
	int32 fChannels;
};

#endif	/* VUVIEW_H */
