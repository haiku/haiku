/*
 * ValidRect.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2005 Michael Pfeiffer. All Rights Reserved.
 * - Rewrote get_valid_rect from scratch.
 */

#include <Bitmap.h>
#include "ValidRect.h"

#define INLINE inline

class BoundsCalculator
{
public:
	bool getValidRect(BBitmap *bitmap, RECT *rect);
	
private:
	const uchar *fBits;
	int fBPR;
	int fLeft;
	int fRight;
	int fTop;
	int fBottom;
	int fWidth;
	
	int fLeftBound;
	int fRightBound;

	INLINE bool isEmpty(const rgb_color *pixel);

	INLINE bool isRowEmpty(const rgb_color *row);

	INLINE const uchar *getRow(int x, int y);

	int getTop();
	
	int getBottom();
	
	INLINE void updateLeftBound(const rgb_color *row);
	INLINE void updateRightBound(const rgb_color *row);
};


bool 
BoundsCalculator::isEmpty(const rgb_color *pixel)
{
	return pixel->red == 0xff && pixel->green == 0xff && pixel->blue == 0xff;
}


bool 
BoundsCalculator::isRowEmpty(const rgb_color *row)
{
	for (int x = 0; x < fWidth; x ++) {
		if (!isEmpty(row)) {
			return false;
		}
		row ++;
	}
	return true;
}


const uchar *
BoundsCalculator::getRow(int x, int y)
{
	return fBits + x + fBPR * y;
}


int 
BoundsCalculator::getTop()
{	
	const uchar* row = getRow(fLeft, fTop);

	int top;
	for (top = fTop; top <= fBottom; top ++) {
		if (!isRowEmpty((const rgb_color*)row)) {
			break;
		}
		row += fBPR;
	}
	
	return top;
}


int
BoundsCalculator::getBottom()
{
	const uchar *row = getRow(fLeft, fBottom);

	int bottom;
	for (bottom = fBottom; bottom >= fTop; bottom --) {
		if (!isRowEmpty((const rgb_color*)row)) {
			break;
		}
		row -= fBPR;
	}
	
	return bottom;
}


void
BoundsCalculator::updateLeftBound(const rgb_color *row)
{
	for (int x = fLeft; x < fLeftBound; x ++) {
		if (!isEmpty(row)) {
			fLeftBound = x;
			return;
		}
		row ++;
	}
}


void
BoundsCalculator::updateRightBound(const rgb_color *row)
{
	row += fWidth - 1;
	for (int x = fRight; x > fRightBound; x --) {
		if (!isEmpty(row)) {
			fRightBound = x;
			return;
		}
		row --;
	}
}


// returns false if the bitmap is empty or has wrong color space.
bool
BoundsCalculator::getValidRect(BBitmap *bitmap, RECT *rect)
{
	enum {
		kRectIsInvalid = false,
		kRectIsEmpty = false,
		kRectIsValid = true
	};
	
	switch (bitmap->ColorSpace()) {
		case B_RGB32:
		case B_RGB32_BIG:
			break;
		default:
			return kRectIsInvalid;
			break;
	};

	// initialize member variables
	fBits = (uchar*)bitmap->Bits();
	fBPR  = bitmap->BytesPerRow();
	
	fLeft   = rect->left;	
	fRight  = rect->right;
	fTop    = rect->top;
	fBottom = rect->bottom;
	
	fWidth = fRight - fLeft + 1;

	// get top bound
	fTop = getTop();
	if (fTop > fBottom) {
		return kRectIsEmpty;
	}
	
	// get bottom bound
	fBottom = getBottom();
	
	// calculate left and right bounds
	fLeftBound = fRight + 1;
	fRightBound = fLeft - 1;
	
	const uchar *row = getRow(fLeft, fTop);
	for (int y = fTop; y <= fBottom; y ++) {
		updateLeftBound((const rgb_color*)row);
		updateRightBound((const rgb_color*)row);
		if (fLeft == fLeftBound && fRight == fRightBound) {
			break;
		}
		row += fBPR;
	}
	
	// return bounds in rectangle
	rect->left = fLeftBound;
	rect->right = fRightBound; 
	rect->top = fTop;
	rect->bottom = fBottom;
		
	return kRectIsValid;
}


bool get_valid_rect(BBitmap *a_bitmap, RECT *rc)
{
	BoundsCalculator calculator;
	return calculator.getValidRect(a_bitmap, rc);
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
