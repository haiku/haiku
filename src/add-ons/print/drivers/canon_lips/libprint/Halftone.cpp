/*
 * HalftoneEngine.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

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

uint gray(rgb_color c)
{
	if (c.red == c.green && c.red == c.blue) {
		return 255 - c.red;
	} else {
		return 255 - (c.red * 3 + c.green * 6 + c.blue) / 10;
	}
}

Halftone::Halftone(color_space cs, double gamma, DITHERTYPE dither_type)
{
	__pixel_depth = color_space2pixel_depth(cs);
	__palette     = system_colors()->color_list;
	__gray        = gray;

	createGammaTable(gamma);
	memset(__cache_table, 0, sizeof(__cache_table));

	switch (dither_type) {
	case TYPE2:
		__pattern = pattern16x16_type2;
		break;
	case TYPE3:
		__pattern = pattern16x16_type3;
		break;
	default:
		__pattern = pattern16x16_type1;
		break;
	}

	switch (cs) {
	case B_GRAY1:
		__dither = &Halftone::ditherGRAY1;
		break;
	case B_GRAY8:
		__dither = &Halftone::ditherGRAY8;
		break;
	case B_RGB32:
	case B_RGB32_BIG:
		__dither = &Halftone::ditherRGB32;
		break;
//	case B_CMAP8:
	default:
		__dither = &Halftone::ditherCMAP8;
		break;
	}
}

Halftone::~Halftone()
{
}

void Halftone::createGammaTable(double gamma)
{
//	gamma = 1.0f / gamma;
	uint *g = __gamma_table;
	for (int i = 0; i < 256; i++) {
		*g++ = (uint)(pow((double)i / 255.0, gamma) * 256.0);
	}
}

void Halftone::initElements(int x, int y, uchar *elements)
{
	x &= 0x0F;
	y &= 0x0F;

	const uchar *left  = &__pattern[y * 16];
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
	return (this->*__dither)(dst, src, x, y, width);
}

int Halftone::ditherGRAY1(
	uchar *dst,
	const uchar *src,
	int,
	int,
	int width)
{
	int widthByte = (width + 7) / 8;
	memcpy(dst, src, widthByte);
	return widthByte;
}

int Halftone::ditherGRAY8(
	uchar *dst,
	const uchar *src,
	int x,
	int y,
	int width)
{
	uchar elements[16];
	initElements(x, y, elements);

	int widthByte = (width + 7) / 8;
	int remainder = width % 8;
	if (!remainder)
		remainder = 8;

	uchar cur;
	uint  density;
	int   i, j;
	uchar *e = elements;
	uchar *last_e = elements + 16;

	if (width >= 8) {
		for (i = 0; i < widthByte - 1; i++) {
			cur = 0;
			if (e == last_e) {
				e = elements;
			}
			for (j = 0; j < 8; j++) {
				density = __gamma_table[*src++];
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
			density = __gamma_table[*src++];
			if (density > *e++) {
				cur |= (0x80 >> j);
			}
		}
		*dst++ = cur;
	}

	return widthByte;
}

int Halftone::ditherCMAP8(
	uchar *dst,
	const uchar *src,
	int x,
	int y,
	int width)
{
	uchar elements[16];
	initElements(x, y, elements);

	int widthByte = (width + 7) / 8;
	int remainder = width % 8;
	if (!remainder)
		remainder = 8;

	rgb_color c;
	uchar cur;
	int i, j;

	uchar *e = elements;
	uchar *last_e = elements + 16;
	CACHE_FOR_CMAP8 *cache;

	if (width >= 8) {
		for (i = 0; i < widthByte - 1; i++) {
			cur = 0;
			if (e == last_e) {
				e = elements;
			}
			for (j = 0; j < 8; j++) {
				cache = &__cache_table[*src];
				if (!cache->hit) {
					cache->hit = true;
					c = __palette[*src];
					cache->density = __gamma_table[__gray(c)];
				}
				src++;
				if (cache->density > *e++) {
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
			cache = &__cache_table[*src];
			if (!cache->hit) {
				cache->hit = true;
				c = __palette[*src];
				cache->density = __gamma_table[__gray(c)];
			}
			src++;
			if (cache->density > *e++) {
				cur |= (0x80 >> j);
			}
		}
		*dst++ = cur;
	}

	return widthByte;
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

	const rgb_color *src = (const rgb_color *)a_src;

	int widthByte = (width + 7) / 8;
	int remainder = width % 8;
	if (remainder == 0)
		remainder = 8;

	rgb_color c;
	uchar cur;
	uint  density;
	int i, j;
	uchar *e = elements;
	uchar *last_e = elements + 16;

	c = *src;
	density = __gamma_table[__gray(c)];

	if (width >= 8) {
		for (i = 0; i < widthByte - 1; i++) {
			cur = 0;
			if (e == last_e) {
				e = elements;
			}
			for (j = 0; j < 8; j++) {
				if (c.red != src->red || c.green != src->green || c.blue != src->blue) {
					c = *src;
					density = __gamma_table[__gray(c)];
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
			if (c.red != src->red || c.green != src->green || c.blue != src->blue) {
				c = *src;
				density = __gamma_table[__gray(c)];
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
