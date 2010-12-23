/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: 
 *	Consumers and Producers)
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
	void ComputeLevels(const void* data, size_t size, uint32 format);
			
private:
	void _Run();
	void _Quit();
	static int32 _RenderLaunch(void *data);
	void _RenderLoop();
	template<typename T> T _ComputeNextLevel(const void *data, 
		size_t size, uint32 format, int32 channel);
	
	thread_id fThreadId;
	BBitmap *fBitmap;
	BView *fBitmapView;
	bool fQuitting;
	int32 fLevelCount;
	int32 *fCurrentLevels;
	int32 fChannels;
};

#endif	/* VUVIEW_H */
