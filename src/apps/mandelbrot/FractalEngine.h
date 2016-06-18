/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		kerwizzy
 */
#ifndef FRACTALENGINE_H
#define FRACTALENGINE_H

#include <SupportDefs.h>
#include <Looper.h>
#include <Messenger.h>

class BBitmap;


class FractalEngine : public BLooper {
public:
	enum {
		MSG_CHANGE_SET = 'Frct',
		MSG_SET_PALETTE,
		MSG_SET_ITERATIONS,
		MSG_RESIZE,
		MSG_RENDER,
		MSG_RENDER_COMPLETE,
	};

	FractalEngine(BHandler* parent, BLooper* looper);
	~FractalEngine();

	virtual void MessageReceived(BMessage* msg);

private:
	BMessenger fMessenger;
	BBitmap* fBitmapStandby;
	BBitmap* fBitmapDisplay;

	uint8 fThreadCount;
	thread_id fRenderThreads[4];
	sem_id fRenderSem;
	sem_id fRenderFinishedSem;

	double fLocationX;
	double fLocationY;
	double fSize;

	uint16 fIterations;
	uint16 fWidth;
	uint16 fHeight;

	uint8* fRenderBuffer;
	uint32 fRenderBufferLen;

	const uint8* fColorset;

	int32 (FractalEngine::*fDoSet)(double real, double imaginary);

	void Render(double locationX, double locationY, double size);
	static status_t RenderThread(void* data);
	void RenderPixel(uint32 x, uint32 y, double real, double imaginary);

	int32 DoSet_Mandelbrot(double real, double imaginary);
	int32 DoSet_BurningShip(double real, double imaginary);
	int32 DoSet_Tricorn(double real, double imaginary);
	int32 DoSet_Julia(double real, double imaginary);
	int32 DoSet_OrbitTrap(double real, double imaginary);
	int32 DoSet_Multibrot(double real, double imaginary);
};


#endif	/* FRACTALENGINE_H */
