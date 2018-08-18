/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		kerwizzy
 */
#include "FractalEngine.h"

#include <algorithm>
#include <math.h>

#include <Bitmap.h>
#include <String.h>

#include "Colorsets.h"


// #define TRACE_MANDELBROT_ENGINE
#ifdef TRACE_MANDELBROT_ENGINE
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...)
#endif


FractalEngine::FractalEngine(BHandler* parent, BLooper* looper)
	:
	BLooper("FractalEngine"),
	fWidth(0), fHeight(0),
	fRenderBuffer(NULL),
	fRenderBufferLen(0),
	fMessenger(parent, looper),
	fThreadsRendering(0),
	fIterations(1024),
	fColorset(Colorset_Royal)
{
	fDoSet = &FractalEngine::DoSet_Mandelbrot;

	fRenderSem = create_sem(0, "RenderSem");

	system_info info;
	get_system_info(&info);
	fThreadCount = info.cpu_count;

	if (fThreadCount > MAX_RENDER_THREADS)
		fThreadCount = MAX_RENDER_THREADS;

	for (uint8 i = 0; i < fThreadCount; i++) {
		fRestartRenderThread[i] = false;
		fRenderThreadRunning[i] = false;
		fRenderThreads[i] = spawn_thread(&FractalEngine::RenderThread,
			BString().SetToFormat("RenderThread%d", i).String(),
			B_NORMAL_PRIORITY, this);
		resume_thread(fRenderThreads[i]);
	}
}


FractalEngine::~FractalEngine()
{
}


void FractalEngine::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CHANGE_SET:
		switch (msg->GetUInt8("set", 0)) {
		case 0: fDoSet = &FractalEngine::DoSet_Mandelbrot; break;
		case 1: fDoSet = &FractalEngine::DoSet_BurningShip; break;
		case 2: fDoSet = &FractalEngine::DoSet_Tricorn; break;
		case 3: fDoSet = &FractalEngine::DoSet_Julia; break;
		case 4: fDoSet = &FractalEngine::DoSet_OrbitTrap; break;
		case 5: fDoSet = &FractalEngine::DoSet_Multibrot; break;
		}
		break;
	case MSG_SET_PALETTE:
		switch (msg->GetUInt8("palette", 0)) {
		case 0: fColorset = Colorset_Royal; break;
		case 1: fColorset = Colorset_DeepFrost; break;
		case 2: fColorset = Colorset_Frost; break;
		case 3: fColorset = Colorset_Fire; break;
		case 4: fColorset = Colorset_Midnight; break;
		case 5: fColorset = Colorset_Grassland; break;
		case 6: fColorset = Colorset_Lightning; break;
		case 7: fColorset = Colorset_Spring; break;
		case 8: fColorset = Colorset_HighContrast; break;
		}
		break;
	case MSG_SET_ITERATIONS:
		fIterations = msg->GetUInt16("iterations", 0);
		break;

	case MSG_RESIZE: {
		delete fRenderBuffer;
		fRenderBuffer = NULL;

		fWidth = msg->GetUInt16("width", 320);
		fHeight = msg->GetUInt16("height", 240);

		TRACE("Creating new buffer. width %u height %u\n", fWidth, fHeight);

		fRenderBufferLen = fWidth * fHeight * 3;
		fRenderBuffer = new uint8[fRenderBufferLen];
		memset(fRenderBuffer, 0, fRenderBufferLen);

		BMessage message(MSG_BUFFER_CREATED);
		fMessenger.SendMessage(&message);
		break;
	}
	case MSG_RENDER: {
		TRACE("Got MSG_RENDER. fThreadsRendering = %u\n",fThreadsRendering);
		Render(msg->GetDouble("locationX", 0), msg->GetDouble("locationY", 0),
			msg->GetDouble("size", 0.005));
		break;
	}
	case MSG_THREAD_RENDER_COMPLETE: {
		TRACE("Got MSG_THREAD_RENDER_COMPLETE for thread %d\n",
			msg->GetUInt8("thread", 0));

		fThreadsRendering--;
		TRACE("Set fRendering = %d\n", fThreadsRendering);
		if (!fThreadsRendering) {
			TRACE("Done rendering!\n");
			BMessage message(MSG_RENDER_COMPLETE);
			fMessenger.SendMessage(&message);
		}
		break;
	}

	default:
		BLooper::MessageReceived(msg);
		break;
	}
}


void FractalEngine::WriteToBitmap(BBitmap* bitmap)
{
	bitmap->ImportBits(fRenderBuffer,
		fRenderBufferLen, fWidth * 3, 0,
		B_RGB24);
}


void FractalEngine::Render(double locationX, double locationY, double size)
{
	fLocationX = locationX;
	fLocationY = locationY;
	fSize = size;

	TRACE("Location: %g;%g;%g\n", fLocationX, fLocationY, fSize);

	for (uint8 i = 0; i < fThreadCount; i++) {
		if (fRenderThreadRunning[i])
			fRestartRenderThread[i] = true;
		else
			release_sem(fRenderSem);
		fRenderThreadRunning[i] = true;
	}

	fThreadsRendering = fThreadCount;
	TRACE("Set fRendering = %d\n", fThreadsRendering);
}


status_t FractalEngine::RenderThread(void* data)
{
	FractalEngine* engine = static_cast<FractalEngine*>(data);
	thread_id self = find_thread(NULL);
	uint8 threadNum = 0;
	for (uint8 i = 0; i < engine->fThreadCount; i++) {
		if (engine->fRenderThreads[i] == self) {
			threadNum = i;
			break;
		}
	}

	bool restarting = false;
	while (true) {
		TRACE("Thread %d starting another render pass\n", threadNum);

		if (!restarting) {
			TRACE("Thread %d awaiting semaphore...\n", threadNum);
			acquire_sem(engine->fRenderSem);
			TRACE("Thread %d got semaphore!\n", threadNum);
		} else {
			TRACE("Thread %d setting restart = false\n", threadNum);
			restarting = false;
		}


		uint16 width = engine->fWidth;
		uint16 height = engine->fHeight;
		uint16 halfHeight = height / 2;
		uint16 threadWidth = width / engine->fThreadCount;

		uint16 startX = threadWidth * threadNum;
		uint16 endX = threadWidth * (threadNum + 1);

		for (uint16 y = 0; y<halfHeight; y++) {
			for (uint16 x = startX; x<endX; x++) {
				engine->RenderPixel(x, halfHeight + y);
					// max y to RenderPixel will be
					// floor(height/2)*2 = height-height%2.
					// Thus there will be a line of blank pixels if height
					// is not even. Is this a problem?
				engine->RenderPixel(x, halfHeight - y - 1);
					// min y to RenderPixel will be
					// halfHeight-(halfHeight-1)-1 = 0
			}

			if (engine->fRestartRenderThread[threadNum]) {
				TRACE("Thread %d Got restart message\n", threadNum);

				engine->fRestartRenderThread[threadNum] = false;
				restarting = true;
				break; // Restart the loop to update width, height, etc.
			}
		}

		if (!restarting) {
			engine->fRenderThreadRunning[threadNum] = false;
			BMessage message(FractalEngine::MSG_THREAD_RENDER_COMPLETE);
			message.AddUInt8("thread", threadNum);
			engine->PostMessage(&message);
		}
	}
	return B_OK;
}


void FractalEngine::RenderPixel(uint32 x, uint32 y)
{
	double real = (x * fSize + fLocationX) - (fWidth / 2 * fSize);
	double imaginary = (y * -(fSize) + fLocationY) - (fHeight / 2 * -(fSize));

	int32 iterToEscape = (this->*fDoSet)(real, imaginary);
	uint16 loc = 0;
	if (iterToEscape == -1)
		// Didn't escape.
		loc = 999;
	else
		loc = 998 - (iterToEscape % 999);

	uint32 offsetBase = fWidth * y * 3 + x * 3;
	loc *= 3;
	fRenderBuffer[offsetBase + 2] = fColorset[loc + 0]; // fRenderBuffer is BGR
	fRenderBuffer[offsetBase + 1] = fColorset[loc + 1];
	fRenderBuffer[offsetBase + 0] = fColorset[loc + 2];
}


// Magic numbers & other general constants
const double gJuliaA = 0;
const double gJuliaB = 1;

const uint8 gEscapeHorizon = 4;


int32 FractalEngine::DoSet_Mandelbrot(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	for (int32 i = 0; i < fIterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * zImaginarySq));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq + zImaginarySq) > gEscapeHorizon)
			return i; // stop it from running longer
	}
	return -1;
}


int32 FractalEngine::DoSet_BurningShip(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	// It looks "upside down" otherwise.
	imaginary = -imaginary;

	for (int32 i = 0; i < fIterations; i++) {
		zReal = fabs(zReal);
		zImaginary = fabs(zImaginary);

		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * zImaginarySq));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq + zImaginarySq) > gEscapeHorizon)
			return i; // stop it from running longer
	}
	return -1;
}


int32 FractalEngine::DoSet_Tricorn(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	real = -real;

	for (int32 i = 0; i < fIterations; i++) {
		double znRe = zImaginary * -1;
		zImaginary = zReal * -1;
		zReal = znRe; // Swap the real and complex parts each time.

		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * zImaginarySq));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq + zImaginarySq) > gEscapeHorizon)
			return i; // stop it from running longer
	}
	return -1;
}


int32 FractalEngine::DoSet_Julia(double real, double imaginary)
{
	double zReal = real;
	double zImaginary = imaginary;

	double muRe = gJuliaA;
	double muIm = gJuliaB;

	for (int32 i = 0; i < fIterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += muRe;
		zImaginary += muIm;

		// If it is outside the 2 unit circle...
		if ((zRealSq + zImaginarySq) > gEscapeHorizon)
			return i; // stop it from running longer
	}
	return -1;
}


int32 FractalEngine::DoSet_OrbitTrap(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	double closest = 10000000;
	double distance = 0;
	double lineDist = 0;

	for (int32 i = 0; i < fIterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * zImaginarySq));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		distance = sqrt(zRealSq + zImaginarySq);
		lineDist = fabs(zReal + zImaginary);

		// If it is closer than ever before...
		if (lineDist < closest)
			closest = lineDist;

		if (distance > gEscapeHorizon)
			return static_cast<int32>(floor(4 * log(4 / closest)));
	}
	return static_cast<int32>(floor(4 * log(4 / closest)));
}


int32 FractalEngine::DoSet_Multibrot(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	for (int32 i = 0; i < fIterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq * zReal - 3 * zReal * zImaginarySq);

		zImaginary = 3 * (zRealSq * zImaginary) - (zImaginarySq * zImaginary);

		zReal = nzReal;
		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq + zImaginarySq) > gEscapeHorizon)
			return i; // stop it from running longer
	}
	return -1;
}
