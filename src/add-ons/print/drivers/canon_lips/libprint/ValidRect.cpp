/*
 * ValidRect.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <Bitmap.h>
#include "ValidRect.h"

bool get_valid_line_RGB32(
	const uchar *bits,
	const rgb_color *,
	int width,
	int *left,
	int *right)
{
	int i = 0;
	rgb_color c;
	const rgb_color *ptr = (const rgb_color *)bits;

	while (i < *left) {
		c = *ptr++;
		if (c.red != 0xff || c.green != 0xff || c.blue != 0xff) {
			*left = i;
			break;
		}
		i++;
	}

	if (i == width) {
		return false;
	}

	i = width - 1;
	ptr = (const rgb_color *)bits + width - 1;

	while (i > *right) {
		c = *ptr--;
		if (c.red != 0xff || c.green != 0xff || c.blue != 0xff) {
			*right = i;
			break;
		}
		i--;
	}
	return true;
}

bool get_valid_line_CMAP8(
	const uchar *bits,
	const rgb_color *palette,
	int width,
	int *left,
	int *right)
{
	int i = 0;
	rgb_color c;
	const uchar *ptr = bits;

	while (i < *left) {
		c = palette[*ptr++];
		if (c.red != 0xff || c.green != 0xff || c.blue != 0xff) {
			*left = i;
			break;
		}
		i++;
	}

	if (i == width) {
		return false;
	}

	i = width - 1;
	ptr = bits + width - 1;

	while (i > *right) {
		c = palette[*ptr--];
		if (c.red != 0xff || c.green != 0xff || c.blue != 0xff) {
			*right = i;
			break;
		}
		i--;
	}
	return true;
}

bool get_valid_line_GRAY8(
	const uchar *bits,
	const rgb_color *,
	int width,
	int *left,
	int *right)
{
	int i = 0;
	const uchar *ptr = bits;

	while (i < *left) {
		if (*ptr++) {
			*left = i;
			break;
		}
		i++;
	}

	if (i == width) {
		return false;
	}

	i = width - 1;
	ptr = bits + width - 1;

	while (i > *right) {
		if (*ptr--) {
			*right = i;
			break;
		}
		i--;
	}
	return true;
}

bool get_valid_line_GRAY1(
	const uchar *bits,
	const rgb_color *,
	int width,
	int *left,
	int *right)
{
	int i = 0;
	const uchar *ptr = bits;

	while (i < *left) {
		if (*ptr++) {
			*left = i;
			break;
		}
		i += 8;
	}

	if (i == width) {
		return false;
	}

	i = width - 1;
	ptr = bits + ((width - 1) + 7) / 8;

	while (i > *right) {
		if (*ptr--) {
			*right = i;
			break;
		}
		i -= 8;
	}
	return true;
}

typedef bool (*PFN_GET_VALID_LINE)(const uchar *, const rgb_color *, int, int *, int *);

bool get_valid_rect(BBitmap *a_bitmap, const rgb_color *palette, RECT *rc)
{
	int width  = rc->right  - rc->left + 1;
	int height = rc->bottom - rc->top  + 1;
	int delta  = a_bitmap->BytesPerRow();

	int left   = width;
	int right  = 0;
	int top    = 0;
	int bottom = 0;

	PFN_GET_VALID_LINE get_valid_line;

	switch (a_bitmap->ColorSpace()) {
	case B_RGB32:
	case B_RGB32_BIG:
		get_valid_line = get_valid_line_RGB32;
		break;
	case B_CMAP8:
		get_valid_line = get_valid_line_CMAP8;
		break;
	case B_GRAY8:
		get_valid_line = get_valid_line_GRAY8;
		break;
	case B_GRAY1:
		get_valid_line = get_valid_line_GRAY1;
		break;
	default:
		get_valid_line = get_valid_line_GRAY1;
		break;
	};

	int i = 0;
	uchar *ptr = (uchar *)a_bitmap->Bits();

	while (i < height) {
		if (get_valid_line(ptr, palette, width, &left, &right)) {
			top = i;
			break;
		}
		ptr += delta;
		i++;
	}

	if (i == height) {
		return false;
	}

	int j = height - 1;
	ptr = (uchar *)a_bitmap->Bits() + (height - 1) * delta;
	bool found_boundary = false;

	while (j >= i) {
		if (get_valid_line(ptr, palette, width, &left, &right)) {
			if (!found_boundary) {
				bottom = j;
				found_boundary = true;
			}
		}
		ptr -= delta;
		j--;
	}

	rc->left   = left;
	rc->top    = top;
	rc->right  = right;
	rc->bottom = bottom;

	return true;
}

int color_space2pixel_depth(color_space cs)
{
	int pixel_depth;

	switch (cs) {
	case B_GRAY1:		/* Y0[0],Y1[0],Y2[0],Y3[0],Y4[0],Y5[0],Y6[0],Y7[0]	*/
		pixel_depth = 1;
		break;
	case B_GRAY8:		/* Y[7:0]											*/
	case B_CMAP8:		/* D[7:0]  											*/
		pixel_depth = 8;
		break;
	case B_RGB15:		/* G[2:0],B[4:0]  	   -[0],R[4:0],G[4:3]			*/
	case B_RGB15_BIG:	/* -[0],R[4:0],G[4:3]  G[2:0],B[4:0]				*/
		pixel_depth = 16;
		break;
	case B_RGB32:		/* B[7:0]  G[7:0]  R[7:0]  -[7:0]					*/
	case B_RGB32_BIG:	/* -[7:0]  R[7:0]  G[7:0]  B[7:0]					*/
		pixel_depth = 32;
		break;
	default:
		pixel_depth = 0;
		break;
	}
	return pixel_depth;
}
