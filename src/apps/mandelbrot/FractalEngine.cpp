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
	fIterations(1024),
	fWidth(0), fHeight(0),
	fRenderBuffer(NULL),
	fRenderBufferLen(0),
	fColorset(Colorset_Royal)
{
	fDoSet = &FractalEngine::DoSet_Mandelbrot;
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


// Magic numbers & other general constants
const double gJuliaA = 0;
const double gJuliaB = 1;

const uint8 gEscapeHorizon = 4;


void FractalEngine::RenderPixel(uint32 x, uint32 y, double real,
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

	uint32 offsetBase = fWidth * y * 3 + x * 3;
	loc *= 3;
	fRenderBuffer[offsetBase + 0] = fColorset[loc + 0];
	fRenderBuffer[offsetBase + 1] = fColorset[loc + 1];
	fRenderBuffer[offsetBase + 2] = fColorset[loc + 2];
}


int32 FractalEngine::DoSet_Mandelbrot(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	for (int32 i = 0; i < fIterations; i++) {
		double zRealSq = zReal * zReal;
		double zImaginarySq = zImaginary * zImaginary;
		double nzReal = (zRealSq + (-1 * (zImaginarySq)));

		zImaginary = 2 * (zReal * zImaginary);
		zReal = nzReal;

		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > gEscapeHorizon) {
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

	for (int32 i = 0; i < fIterations; i++) {
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
		if ((zRealSq) + (zImaginarySq) > gEscapeHorizon) {
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

	for (int32 i = 0; i < fIterations; i++) {
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
		if ((zRealSq) + (zImaginarySq) > gEscapeHorizon) {
			return i; // stop it from running longer
		}
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
		if ((zRealSq) + (zImaginarySq) > gEscapeHorizon) {
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

	for (int32 i = 0; i < fIterations; i++) {
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

		if (distance > gEscapeHorizon) {
			return static_cast<int32>(floor(4 * log(4 / closest)));
		}
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
		double nzReal = (zRealSq * zReal - 3 * zReal * (zImaginarySq));

		zImaginary = 3 * ((zRealSq)*zImaginary) - (zImaginarySq * zImaginary);

		zReal = nzReal;
		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zRealSq) + (zImaginarySq) > gEscapeHorizon) {
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
			RenderPixel(x, y, (x * size + locationX) - (halfWidth * size),
				(y * -size + locationY) - (halfHeight * -size));
		}
	}

	fBitmapStandby->ImportBits(fRenderBuffer, fRenderBufferLen, fWidth * 3,
		0, B_RGB24_BIG);
}
