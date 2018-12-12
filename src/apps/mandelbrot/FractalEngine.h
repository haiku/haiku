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


#define MAX_RENDER_THREADS 16


class FractalEngine : public BLooper {
public:
	enum {
		MSG_CHANGE_SET = 'Frct',
		MSG_SET_PALETTE,
		MSG_SET_ITERATIONS,
		MSG_SET_SUBSAMPLING,
		MSG_RESIZE,
		MSG_BUFFER_CREATED,
		MSG_RENDER,
		MSG_RENDER_COMPLETE,
		MSG_THREAD_RENDER_COMPLETE,
	};

	FractalEngine(BHandler* parent, BLooper* looper);
	~FractalEngine();

	virtual void MessageReceived(BMessage* msg);
	void WriteToBitmap(BBitmap*);

private:
	uint16 fWidth;
	uint16 fHeight;

	uint8* fRenderBuffer;
	uint32 fRenderBufferLen;

	uint8 fSubsampling;
		// 1 disables subsampling.

	BMessenger fMessenger;

	uint8 fThreadCount;
	thread_id fRenderThreads[MAX_RENDER_THREADS];
	sem_id fRenderSem;
	sem_id fRenderStoppedSem;

	bool fStopRender;
	bool fRenderStopped;

	double fLocationX;
	double fLocationY;
	double fSize;

	uint16 fIterations;

	const uint8* fColorset;

	int32 (FractalEngine::*fDoSet)(double real, double imaginary);

	void Render(double locationX, double locationY, double size);
	void StopRender();
	static status_t RenderThread(void* data);
	void RenderPixel(uint32 x, uint32 y);

	int32 DoSet_Mandelbrot(double real, double imaginary);
	int32 DoSet_BurningShip(double real, double imaginary);
	int32 DoSet_Tricorn(double real, double imaginary);
	int32 DoSet_Julia(double real, double imaginary);
	int32 DoSet_OrbitTrap(double real, double imaginary);
	int32 DoSet_Multibrot(double real, double imaginary);
};


#endif	/* FRACTALENGINE_H */
