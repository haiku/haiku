/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		kerwizzy
 */
#include "FractalEngine.h"

#include <SupportDefs.h>
#include <math.h>

#include "Colorsets.h"
#define colors Colorset_Royal

double zReal_end = 0;
double zImaginary_end = 0;

double juliaC_a = 0;
double juliaC_b = 1;

uint8 r = 0;
uint8 g = 0;
uint8 b = 0;

uint8 logChecked = 0;
uint8 smoothChecked = 0;

uint8 gEscapeHorizon = 4; // set to 64 when doing smooth colors

int32 gIterations = 1024;

double gPower = 0;

void renderPixel_smooth(double real, double imaginary);
void renderPixel_default(double real, double imaginary);
int32 (*inSet)(double real, double imaginary);
int32 inSet_mandelbrot(double real, double imaginary);
int32 inSet_burningShip(double real, double imaginary);
int32 inSet_tricornMandelbrot(double real, double imaginary);

void (*renderPixel)(double real, double imaginary);

void renderPixel_default(double real, double imaginary)
{
	int32 iterToEscape = (*inSet)(real, imaginary);
	uint16 loc = 0;
	if (iterToEscape == -1) {
		// Didn't escape.
		loc = 999;
	} else {
		loc = 998 - (iterToEscape % 999);
	}

	r = colors[loc * 3 + 0];
	g = colors[loc * 3 + 1];
	b = colors[loc * 3 + 2];
}

void renderPixel_smooth(double real, double imaginary)
{
	int32 outColor = (*inSet)(real, imaginary);
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
		r = colors[999 * 3];
		g = colors[999 * 3 + 1];
		b = colors[999 * 3 + 2];
		return;
	}
	outColor = 998 - (outColor % 999);

	mapperLoc = outColor * 3;

	mapperDiff_r = colors[mapperLoc + 0] - colors[mapperLoc + 0 + 3];
	mapperDiff_g = colors[mapperLoc + 1] - colors[mapperLoc + 1 + 3];
	mapperDiff_b = colors[mapperLoc + 2] - colors[mapperLoc + 2 + 3];

	r = mapperDiff_r * ratio + colors[mapperLoc + 0];
	g = mapperDiff_g * ratio + colors[mapperLoc + 1];
	b = mapperDiff_b * ratio + colors[mapperLoc + 2];
}

int32 inSet_mandelbrot(double real, double imaginary)
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

int32 inSet_burningShip(double real, double imaginary)
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

int32 inSet_tricornMandelbrot(double real, double imaginary)
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

int32 inSet_julia(double real, double imaginary)
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

int32 inSet_mandelbrot_orbitTrap(double real, double imaginary)
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

int32 inSet_multibrot_3(double real, double imaginary)
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

int32 inSet_fractional(double real, double imaginary)
{
	double zReal = 0;
	double zImaginary = 0;

	double r = 0;
	double t = 0;

	double power = gPower;

	int32 iterations = gIterations;
	uint8 escapeHorizon = gEscapeHorizon;

	int32 i = 0;
	for (i = 0; i < iterations; i++) {
		r = sqrt(zReal * zReal + zImaginary * zImaginary);
		r = pow(r, power);
		t = atan2(zImaginary, zReal) * power;

		double nzReal = r * cos(t);
		zImaginary = r * sin(t);

		zReal = nzReal;
		zReal += real;
		zImaginary += imaginary;

		// If it is outside the 2 unit circle...
		if ((zReal * zReal) + (zImaginary * zImaginary) > escapeHorizon) {
			zReal_end = zReal;
			zImaginary_end = zImaginary;

			return i; // stop it from running longer
		}
	}
	return -1;
}

#define buf_SetPixel(x, y, r, g, b) \
	buf[width * y * 3 + x * 3 + 0] = r; \
	buf[width * y * 3 + x * 3 + 1] = g; \
	buf[width * y * 3 + x * 3 + 2] = b;
BBitmap* FractalEngine(uint32 width, uint32 height, double locationX,
	double locationY, double size)
{
	uint16 halfcWidth =	width / 2;
	uint16 halfcHeight = height / 2;

	uint32 bufLen = width * height * 3;
	uint8* buf = new uint8[bufLen];

	renderPixel = renderPixel_default;
	inSet = inSet_mandelbrot;

	for (uint32 x = 0; x < width; x++) {
		for (uint32 y = 0; y < height; y++) {
			(*renderPixel)((x * size + locationX) - (halfcWidth * size),
						   (y * -size + locationY) - (halfcHeight * -size));
			buf_SetPixel(x, y, r, g, b);
		}
	}

	BBitmap* ret = new BBitmap(BRect(0, 0, width - 1 , height - 1), 0, B_RGB24);
	ret->ImportBits(buf, bufLen, width * 3, 0, B_RGB24_BIG);
	return ret;
}
