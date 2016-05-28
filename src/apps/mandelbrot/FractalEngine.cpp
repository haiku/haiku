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

#include "Colorsets.h"


FractalEngine::FractalEngine(BHandler* parent, BLooper* looper)
	:
	BLooper("FractalEngine"),
	fMessenger(parent, looper),
	fBitmapStandby(NULL),
	fBitmapDisplay(NULL),
	fWidth(0), fHeight(0),
	fRenderBuffer(NULL),
	fRenderBufferLen(0),
	fColorset(Colorset_Royal)
{
	fRenderPixel = &FractalEngine::RenderPixelDefault;
	fDoSet = &FractalEngine::DoSet_Mandelbrot;
}


FractalEngine::~FractalEngine()
{
}


void FractalEngine::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_RESIZE: {
		delete fBitmapStandby;
		// We don't delete the "display" bitmap; the viewer now owns it
		delete fRenderBuffer;

		fWidth = msg->GetUInt16("width", 320);
		fHeight = msg->GetUInt16("height", 240);
		BRect rect(0, 0, fWidth - 1, fHeight - 1);
		fBitmapStandby = new BBitmap(rect, B_RGB24);
		fBitmapDisplay = new BBitmap(rect, B_RGB24);
		fRenderBufferLen = fWidth * fHeight * 3;
		fRenderBuffer = new uint8[fRenderBufferLen];
		break;
	}
	case MSG_RENDER: {
		// Render to "standby" bitmap
		Render(msg->GetDouble("locationX", 0), msg->GetDouble("locationY", 0),
			msg->GetDouble("size", 0.005));
		BMessage message(MSG_RENDER_COMPLETE);
		message.AddPointer("bitmap", (const void*)fBitmapStandby);
		fMessenger.SendMessage(&message);
		std::swap(fBitmapStandby, fBitmapDisplay);
		break;
	}

	default:
		BLooper::MessageReceived(msg);
		break;
	}
}


double zReal_end = 0;
double zImaginary_end = 0;

double juliaC_a = 0;
double juliaC_b = 1;

uint8 gEscapeHorizon = 4; // set to 64 when doing smooth colors

int32 gIterations = 1024;

double gPower = 0;


#define RenderBuffer_SetPixel(x, y, r, g, b) \
	uint32 _BUF_BASE = fWidth * y * 3 + x * 3; \
	fRenderBuffer[_BUF_BASE + 0] = r; \
	fRenderBuffer[_BUF_BASE + 1] = g; \
	fRenderBuffer[_BUF_BASE + 2] = b
void FractalEngine::RenderPixelDefault(uint32 x, uint32 y, double real,
	double imaginary)
{
	int32 iterToEscape = (this->*fDoSet)(real, imaginary);
	uint16 loc = 0;
	if (iterToEscape == -1) {
		// Didn't escape.
		loc = 999;
	} else {
		loc = 998 - (iterToEscape % 999);
	}

	RenderBuffer_SetPixel(x, y, fColorset[loc * 3 + 0], fColorset[loc * 3 + 1],
		fColorset[loc * 3 + 2]);
}


void FractalEngine::RenderPixelSmooth(uint32 x, uint32 y, double real,
	double imaginary)
{
	int32 outColor = (this->*fDoSet)(real, imaginary);
	int8 mapperDiff_r = 0;
	int8 mapperDiff_g = 0;
	int8 mapperDiff_b = 0;

	int16 mapperLoc = 0;

	double dist = sqrt(zReal_end * zReal_end + zImaginary_end * zImaginary_end);

	double ratio = (1 - (log(log(dist))) / log(2));
	if (sqrt(real * real + imaginary * imaginary) >	8) {
		 // Make the colors >8 be flat.
		ratio = -1.0821509904820257;
		outColor = 1;
	}

	if (outColor == -1) {
		RenderBuffer_SetPixel(x, y, fColorset[999 * 3], fColorset[999 * 3 + 1],
			fColorset[999 * 3 + 2]);
		return;
	}
	outColor = 998 - (outColor % 999);

	mapperLoc = outColor * 3;

	mapperDiff_r = fColorset[mapperLoc + 0] - fColorset[mapperLoc + 0 + 3];
	mapperDiff_g = fColorset[mapperLoc + 1] - fColorset[mapperLoc + 1 + 3];
	mapperDiff_b = fColorset[mapperLoc + 2] - fColorset[mapperLoc + 2 + 3];

	RenderBuffer_SetPixel(x, y, mapperDiff_r * ratio + fColorset[mapperLoc + 0],
		mapperDiff_g * ratio + fColorset[mapperLoc + 1],
		mapperDiff_b * ratio + fColorset[mapperLoc + 2]);
}
#undef RenderBuffer_SetPixel


int32 FractalEngine::DoSet_Mandelbrot(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		if ((zRealSq) + (zImaginarySq) >
			escapeHorizon) { // If it is outside the 2 unit circle...
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
	}
	return -1;
}


int32 FractalEngine::DoSet_BurningShip(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	// It looks "upside down" otherwise.
	imaginary = -imaginary;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		zReal = fabs(zReal);
		zImaginary = fabs(zImaginary);

		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
	}
	return -1;
}


int32 FractalEngine::DoSet_Tricorn(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	real = -real;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		double znRe = zImaginary * -1;
		zImaginary = zReal * -1;
		zReal = znRe; // Swap the real and complex parts each time.

		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
	}
	return -1;
}


int32 FractalEngine::DoSet_Julia(double real, double imaginary)
{
	double zReal = real;
	double zImaginary = imaginary;

	double muRe = juliaC_a;
	double muIm = juliaC_b;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += muRe;
		zImaginary += muIm;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
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

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		distance = sqrt((zRealSq) + (zImaginarySq));
		lineDist = fabs((zReal) + (zImaginary));

		// If it is closer than ever before...
		if (lineDist < closest)
			closest = lineDist;

		if (distance > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;
			return floor(4 * log(4 / closest));
		}
	}
	return floor(4 * log(4 / closest));
}

int32 FractalEngine::DoSet_Multibrot(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq * zReal - 3 * zReal * (zImaginarySq));

		zImaginary = 3 * ((zRealSq)*zImaginary) - (zImaginarySq * zImaginary);

		zReal = nzReal;
		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
	}
	return -1;
}


void FractalEngine::Render(double locationX, double locationY, double size)
{
	uint16 halfWidth = fWidth / 2;
	uint16 halfHeight = fHeight / 2;

	for (uint32 x = 0; x < fWidth; x++) {
		for (uint32 y = 0; y < fHeight; y++) {
			(this->*fRenderPixel)(x, y,
				(x * size + locationX) - (halfWidth * size),
				(y * -size + locationY) - (halfHeight * -size));
		}
	}

	fBitmapStandby->ImportBits(fRenderBuffer, fRenderBufferLen, fWidth * 3,
		0, B_RGB24_BIG);
}
