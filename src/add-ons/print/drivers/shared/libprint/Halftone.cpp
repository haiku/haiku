/*
 * HalftoneEngine.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <Debug.h>
#include <InterfaceDefs.h>
#include <math.h>
#include <memory>
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

#include "Pattern.h"

static uint gray(ColorRGB32 c)
{
	if (c.little.red == c.little.green && c.little.red == c.little.blue) {
		return 255 - c.little.red;
	} else {
		return 255 - (c.little.red * 3 + c.little.green * 6 + c.little.blue) / 10;
	}
}

static uint channel_red(ColorRGB32 c)
{
	return c.little.red;	
}

static uint channel_green(ColorRGB32 c)
{
	return c.little.green;	
}

static uint channel_blue(ColorRGB32 c)
{
	return c.little.blue;	
}

Halftone::Halftone(color_space cs, double gamma, DitherType dither_type)
{
	fPixelDepth = color_space2pixel_depth(cs);
	fGray       = gray;
	setPlanes(kPlaneMonochrome1);
	
	initFloydSteinberg();
	
	createGammaTable(gamma);
	
	if (dither_type == kTypeFloydSteinberg) {
		fDither = &Halftone::ditherFloydSteinberg;
		return;
	}

	switch (dither_type) {
	case kType2:
		fPattern = pattern16x16_type2;
		break;
	case kType3:
		fPattern = pattern16x16_type3;
		break;
	default:
		fPattern = pattern16x16_type1;
		break;
	}

	switch (cs) {
	case B_RGB32:
	case B_RGB32_BIG:
		fDither = &Halftone::ditherRGB32;
		break;
	default:
		fDither = NULL;
		break;
	}
}

Halftone::~Halftone()
{
	uninitFloydSteinberg();
}

void Halftone::setPlanes(Planes planes)
{
	fPlanes = planes;
	if (planes == kPlaneMonochrome1) {
		fNumberOfPlanes = 1;
		fGray = gray;
	} else {
		ASSERT(planes == kPlaneRGB1);
		fNumberOfPlanes = 3;
	}
	fCurrentPlane = 0;
}

void Halftone::createGammaTable(double gamma)
{
//	gamma = 1.0f / gamma;
	uint *g = fGammaTable;
	for (int i = 0; i < kGammaTableSize; i++) {
		*g++ = (uint)(pow((double)i / 255.0, gamma) * 256.0);
	}
}

void Halftone::initElements(int x, int y, uchar *elements)
{
	x &= 0x0F;
	y &= 0x0F;

	const uchar *left  = &fPattern[y * 16];
	const uchar *pos   = left + x;
	const uchar *right = left + 0x0F;

	for (int i = 0; i < 16; i++) {
		*elements++ = *pos;
		if (pos >= right) {
			pos = left;
		} else {
			pos++;
		}
	}
}

int Halftone::dither(
	uchar *dst,
	const uchar *src,
	int x,
	int y,
	int width)
{
	int result;
	
	if (fPlanes == kPlaneRGB1) {
		switch (fCurrentPlane) {
			case 0: 
				setGrayFunction(kRedChannel);
				break;
			case 1:
				setGrayFunction(kGreenChannel);
				break;
			case 2:
				setGrayFunction(kBlueChannel);
				break;
		}
	} else {
		ASSERT(fGray == &gray);
	}

	result = (this->*fDither)(dst, src, x, y, width);

	// next plane
	fCurrentPlane ++;
	if (fCurrentPlane >= fNumberOfPlanes) {
		fCurrentPlane = 0;
	}
	return result;
}

void Halftone::setGrayFunction(GrayFunction grayFunction)
{
	PFN_gray function = NULL;
	switch (grayFunction) {
		case kMixToGray: function = gray;
			break;
		case kRedChannel: function = channel_red;
			break;
		case kGreenChannel: function = channel_green;
			break;
		case kBlueChannel: function = channel_blue;
			break;
	};
	setGrayFunction(function);
}

int Halftone::ditherRGB32(
	uchar *dst,
	const uchar *a_src,
	int x,
	int y,
	int width)
{
	uchar elements[16];
	initElements(x, y, elements);

	const ColorRGB32 *src = (const ColorRGB32 *)a_src;

	int widthByte = (width + 7) / 8;
	int remainder = width % 8;
	if (remainder == 0)
		remainder = 8;

	ColorRGB32 c;
	uchar cur;
	uint  density;
	int i, j;
	uchar *e = elements;
	uchar *last_e = elements + 16;

	c = *src;
	density = getDensity(c);

	if (width >= 8) {
		for (i = 0; i < widthByte - 1; i++) {
			cur = 0;
			if (e == last_e) {
				e = elements;
			}
			for (j = 0; j < 8; j++) {
				if (c.little.red != src->little.red || c.little.green != src->little.green || c.little.blue != src->little.blue) {
					c = *src;
					density = getDensity(c);
				}
				src++;
				if (density > *e++) {
					cur |= (0x80 >> j);
				}
			}
			*dst++ = cur;
		}
	}
	if (remainder > 0) {
		cur = 0;
		if (e == last_e) {
			e = elements;
		}
		for (j = 0; j < remainder; j++) {
			if (c.little.red != src->little.red || c.little.green != src->little.green || c.little.blue != src->little.blue) {
				c = *src;
				density = getDensity(c);
			}
			src++;
			if (density > *e++) {
				cur |= (0x80 >> j);
			}
		}
		*dst++ = cur;
	}

	return widthByte;
}

// Floyd-Steinberg dithering
void Halftone::initFloydSteinberg()
{
	for (int i = 0; i < kMaxNumberOfPlanes; i ++) {
		fErrorTables[i] = NULL;
	}
}

void Halftone::deleteErrorTables()
{
	for (int i = 0; i < kMaxNumberOfPlanes; i ++) {
		delete fErrorTables[i];
		fErrorTables[i] = NULL;
	}
}

void Halftone::uninitFloydSteinberg()
{
	deleteErrorTables();
}

void Halftone::setupErrorBuffer(int x, int y, int width)
{
	deleteErrorTables();
	fX = x; 
	fY = y; 
	fWidth = width;
	for (int i = 0; i < fNumberOfPlanes; i ++) {
		// reserve also space for sentinals at both ends of error table
		const int size = width + 2;
		fErrorTables[i] = new int[size];
		memset(fErrorTables[i], 0, sizeof(int) * size);
	}	
}

int Halftone::ditherFloydSteinberg(uchar *dst, const uchar* a_src, int x, int y, int width)
{
	if (fErrorTables[fCurrentPlane] == NULL || fX != x || fCurrentPlane == 0 && fY != y - 1 || fCurrentPlane > 0 && fY != y || fWidth != width) {
		setupErrorBuffer(x, y, width);
	} else {
		fY = y;
	}

	int* error_table = &fErrorTables[fCurrentPlane][1];
	int current_error = 0, error;
	const ColorRGB32 *src = (const ColorRGB32 *)a_src;
	for (int x = 0; x < width; x ++, src ++) {
		const int bit = 7 - x % 8;
		if (bit == 7) {
			// clear byte if at first pixel in byte
			*dst = 0;
		}
		
		int density = getDensity(*src) + current_error / 16;
		
		if (density < 128) {
			// white pixel
			error = density;
		} else {
			// black pixel
			error = density - 255;
			*dst |= (1 << bit);
		}
		
		// distribute error
		int* right = &error_table[x+1];
		current_error = *right + 7 * error;
		*right = error;
		
		int* middle = right - 1;
		*middle += 5 * error;
		
		int* left = middle - 1;
		*left += 3 * error;
		
		if (bit == 0) {
			// advance to next byte
			dst ++;
		}
	}
}

