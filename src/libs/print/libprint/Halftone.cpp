/*
 * Halftone.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <Debug.h>
#include <InterfaceDefs.h>
#include <math.h>
#include <memory>
#include <string.h>
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"


using namespace std;


#include "Pattern.h"


static uint
ToGray(ColorRGB32 c)
{
	if (c.little.red == c.little.green && c.little.red == c.little.blue)
		return c.little.red;
	return (c.little.red * 3 + c.little.green * 6 + c.little.blue) / 10;
}


static uint
GetRedValue(ColorRGB32 c)
{
	return c.little.red;
}


static uint
GetGreenValue(ColorRGB32 c)
{
	return c.little.green;
}


static uint
GetBlueValue(ColorRGB32 c)
{
	return c.little.blue;
}


Halftone::Halftone(color_space colorSpace, double gamma, double min,
	DitherType ditherType)
{
	fPixelDepth = color_space2pixel_depth(colorSpace);
	fGray       = ToGray;
	SetPlanes(kPlaneMonochrome1);
	SetBlackValue(kHighValueMeansBlack);

	InitFloydSteinberg();

	CreateGammaTable(gamma, min);

	if (ditherType == kTypeFloydSteinberg) {
		fDither = &Halftone::DitherFloydSteinberg;
		return;
	}

	switch (ditherType) {
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

	switch (colorSpace) {
	case B_RGB32:
	case B_RGB32_BIG:
		fDither = &Halftone::DitherRGB32;
		break;
	default:
		fDither = NULL;
		break;
	}
}


Halftone::~Halftone()
{
	UninitFloydSteinberg();
}


void
Halftone::SetPlanes(Planes planes)
{
	fPlanes = planes;
	if (planes == kPlaneMonochrome1) {
		fNumberOfPlanes = 1;
		fGray = ToGray;
	} else {
		ASSERT(planes == kPlaneRGB1);
		fNumberOfPlanes = 3;
	}
	fCurrentPlane = 0;
}


void
Halftone::SetBlackValue(BlackValue blackValue)
{
	fBlackValue = blackValue;
}


void
Halftone::CreateGammaTable(double gamma, double min)
{
	const double kScalingFactor = 255.0 - min;
	for (int i = 0; i < kGammaTableSize; i++) {
		const double kGammaCorrectedValue = pow((double)i / 255.0, gamma);
		const double kTranslatedValue = min + kGammaCorrectedValue * kScalingFactor;
		fGammaTable[i] = (uint)(kTranslatedValue);
	}
}


void
Halftone::InitElements(int x, int y, uchar* elements)
{
	x &= 0x0F;
	y &= 0x0F;

	const uchar *left  = &fPattern[y * 16];
	const uchar *pos   = left + x;
	const uchar *right = left + 0x0F;

	for (int i = 0; i < 16; i++) {
		elements[i] = *pos;
		if (pos >= right)
			pos = left;
		else
			pos++;
	}
}


void
Halftone::Dither(uchar* destination, const uchar* source, int x, int y,
	int width)
{
	if (fPlanes == kPlaneRGB1) {
		switch (fCurrentPlane) {
			case 0: 
				SetGrayFunction(kRedChannel);
				break;
			case 1:
				SetGrayFunction(kGreenChannel);
				break;
			case 2:
				SetGrayFunction(kBlueChannel);
				break;
		}
	} else {
		ASSERT(fGray == &ToGray);
	}

	(this->*fDither)(destination, source, x, y, width);

	// next plane
	fCurrentPlane ++;
	if (fCurrentPlane >= fNumberOfPlanes)
		fCurrentPlane = 0;
}


void
Halftone::SetGrayFunction(GrayFunction grayFunction)
{
	PFN_gray function = NULL;
	switch (grayFunction) {
		case kMixToGray: function = ToGray;
			break;
		case kRedChannel: function = GetRedValue;
			break;
		case kGreenChannel: function = GetGreenValue;
			break;
		case kBlueChannel: function = GetBlueValue;
			break;
	};
	SetGrayFunction(function);
}


void
Halftone::DitherRGB32(uchar *destination, const uchar *source0, int x, int y,
	int width)
{
	uchar elements[16];
	InitElements(x, y, elements);

	const ColorRGB32* source = reinterpret_cast<const ColorRGB32*>(source0);

	int widthByte = (width + 7) / 8;
	int remainder = width % 8;
	if (remainder == 0)
		remainder = 8;

	ColorRGB32 c;
	uchar cur; // cleared bit means white, set bit means black
	uint  density;
	int i, j;
	uchar *e = elements;
	uchar *last_e = elements + 16;

	c = *source;
	density = GetDensity(c);

	if (width >= 8) {
		for (i = 0; i < widthByte - 1; i++) {
			cur = 0;
			if (e == last_e) {
				e = elements;
			}
			for (j = 0; j < 8; j++) {
				if (c.little.red != source->little.red
					|| c.little.green != source->little.green
					|| c.little.blue != source->little.blue) {
					c = *source;
					density = GetDensity(c);
				}
				source++;
				if (density <= *e++) {
					cur |= (0x80 >> j);
				}
			}
			*destination++ = ConvertUsingBlackValue(cur);
		}
	}

	if (remainder > 0) {
		cur = 0;
		if (e == last_e) {
			e = elements;
		}
		for (j = 0; j < remainder; j++) {
			if (c.little.red != source->little.red
				|| c.little.green != source->little.green
				|| c.little.blue != source->little.blue) {
				c = *source;
				density = GetDensity(c);
			}
			source++;
			if (density <= *e++) {
				cur |= (0x80 >> j);
			}
		}
		*destination++ = ConvertUsingBlackValue(cur);
	}
}


// Floyd-Steinberg dithering
void
Halftone::InitFloydSteinberg()
{
	for (int i = 0; i < kMaxNumberOfPlanes; i ++)
		fErrorTables[i] = NULL;
}


void
Halftone::DeleteErrorTables()
{
	for (int i = 0; i < kMaxNumberOfPlanes; i ++) {
		delete fErrorTables[i];
		fErrorTables[i] = NULL;
	}
}


void
Halftone::UninitFloydSteinberg()
{
	DeleteErrorTables();
}


void
Halftone::SetupErrorBuffer(int x, int y, int width)
{
	DeleteErrorTables();
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


void
Halftone::DitherFloydSteinberg(uchar *destination, const uchar* source0,
	int x, int y, int width)
{
	if (fErrorTables[fCurrentPlane] == NULL || fX != x
		|| (fCurrentPlane == 0 && fY != y - 1)
		|| (fCurrentPlane > 0 && fY != y)
		|| fWidth != width)
		SetupErrorBuffer(x, y, width);
	else
		fY = y;

	int* errorTable = &fErrorTables[fCurrentPlane][1];
	int current_error = errorTable[0];
	int error;
	errorTable[0] = 0;

	const ColorRGB32 *source = reinterpret_cast<const ColorRGB32 *>(source0);
	uchar cur = 0; // cleared bit means white, set bit means black
	for (int x = 0; x < width; x ++, source ++) {
		const int bit = 7 - x % 8;		
		const int density = GetDensity(*source) + current_error / 16;
		
		if (density < 128) {
			error = density;
			cur |= (1 << bit);
		} else {
			error = density - 255;
		}
		
		// distribute error using this pattern:
		//        0 X 7 (current_error)
		// (left) 3 5 1 (right)
		//       (middle)
		int* right = &errorTable[x+1];
		current_error = (*right) + 7 * error;
		*right = 1 * error;
		
		int* middle = right - 1;
		*middle += 5 * error;
		
		int* left = middle - 1;
		*left += 3 * error;
		
		if (bit == 0) {
			*destination = ConvertUsingBlackValue(cur);
			// advance to next byte
			destination ++;
			cur = 0;
		}
	}
	
	const bool hasRest = (width % 8) != 0;
	if (hasRest) {
		*destination = ConvertUsingBlackValue(cur);
	}
}

