/*
 * ViewBuffer - Mimicing a frame buffer output - but using a BView.
 * Based on frame_buffer_console.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the MIT License.
 *
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>

#include "ViewBuffer.h"

#define CHAR_WIDTH	7
#define CHAR_HEIGHT	13

// Palette is (black and white are swapt like in normal BeOS Terminal)
//	0 - black,
//	1 - blue,
//	2 - green,
//	3 - cyan,
//	4 - red,
//	5 - magenta,
//	6 - yellow,
//	7 - white
//  8-15 - same but bright (we're ignoring those)

static uint32 sPalette32[] = {
	0xffffff,
	0x0000ff,
	0x00ff00,
	0x00ffff,
	0xff0000,
	0xff00ff,
	0xffff00,
	0x000000,
};


ViewBuffer::ViewBuffer(BRect frame)
	:	BView(frame, "ViewBuffer", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		fColumns(frame.IntegerWidth() / CHAR_WIDTH),
		fRows(frame.IntegerHeight() / CHAR_HEIGHT),
		fGlyphGrid(NULL),
		fResizeCallback(NULL),
		// initially, the cursor is hidden
		fCursorX(-1),
		fCursorY(-1)
{
	SetFont(be_fixed_font);
	
	// initialize private palette
	for (int i = 0; i < 8; i++) {
		fPalette[i].red = (sPalette32[i] >> 16) & 0xff;
		fPalette[i].green = (sPalette32[i] >> 8) & 0xff;
		fPalette[i].blue = (sPalette32[i] >> 0) & 0xff;
		fPalette[i].alpha = 0xff;
	}

	// initialize glyph grid
	uint32 size = fRows * fColumns;
	if (size > 0) {
		fGlyphGrid = new uint16[size];
		memset(fGlyphGrid, 0, size * sizeof(uint16));
	}
}


ViewBuffer::~ViewBuffer()
{
	delete[] fGlyphGrid;
}


void
ViewBuffer::FrameResized(float width, float height)
{
	int32 oldColumns = fColumns;
	int32 oldRows = fRows;

	fColumns = (int32)width / CHAR_WIDTH;
	fRows = (int32)height / CHAR_HEIGHT;

	// resize glyph grid
	uint16* oldGlyphGrid = fGlyphGrid;
	uint32 size = fRows * fColumns;
	if (size > 0) {
		fGlyphGrid = new uint16[size];
		memset(fGlyphGrid, 0, size * sizeof(uint16));
	} else
		fGlyphGrid = NULL;
	// transfer old glyph grid into new one
	if (oldGlyphGrid && fGlyphGrid) {
		int32 columns	= min_c(oldColumns, fColumns);
		int32 rows		= min_c(oldRows, fRows);
		for (int32 y = 0; y < rows; y++) {
			for (int32 x = 0; x < columns; x++) {
				fGlyphGrid[y * fColumns + x] = oldGlyphGrid[y * oldColumns + x];
			}
		}
	}
	delete[] oldGlyphGrid;

	if (fResizeCallback)
		fResizeCallback(fColumns, fRows, fResizeCallbackData);
}


status_t
ViewBuffer::GetSize(int32 *width, int32 *height)
{
	*width = fColumns;
	*height = fRows;
	return B_OK;
}


void
ViewBuffer::SetResizeCallback(resize_callback callback, void *data)
{
	fResizeCallback = callback;
	fResizeCallbackData = data;
}


uint8
ViewBuffer::ForegroundColor(uint8 attr)
{
	return attr & 0x7;
}


uint8
ViewBuffer::BackgroundColor(uint8 attr)
{
	return (attr >> 4) & 0x7;
}


rgb_color
ViewBuffer::GetPaletteEntry(uint8 index)
{
	return fPalette[index];
}


void
ViewBuffer::PutGlyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	if (x >= fColumns || y >= fRows)
		return;
	
	RenderGlyph(x, y, glyph, attr);
}


void
ViewBuffer::FillGlyph(int32 x, int32 y, int32 width, int32 height, uint8 glyph, uint8 attr)
{
	if (x >= fColumns || y >= fRows)
		return;
	
	int32 left = x + width;
	if (left > fColumns)
		left = fColumns;
	
	int32 bottom = y + height;
	if (bottom > fRows)
		bottom = fRows;
	
	for (; y < bottom; y++) {
		for (int32 x2 = x; x2 < left; x2++) {
			RenderGlyph(x2, y, glyph, attr);
		}
	}
}


void
ViewBuffer::RenderGlyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	char string[2];
	string[0] = glyph;
	string[1] = 0;
	
	if (LockLooper()) {
		_RenderGlyph(x, y, string, attr);
		Sync();
		UnlockLooper();
	}
	// remember the glyph in the grid
	if (fGlyphGrid) {
		fGlyphGrid[y * fColumns + x] = (glyph << 8) | attr;
	}
}


void
ViewBuffer::Draw(BRect updateRect)
{
	if (fGlyphGrid) {
		int32 startX	= max_c(0, (int32)(updateRect.left / CHAR_WIDTH));
		int32 endX		= min_c(fColumns - 1, (int32)(updateRect.right / CHAR_WIDTH) + 1);
		int32 startY	= max_c(0, (int32)(updateRect.top / CHAR_HEIGHT));
		int32 endY		= min_c(fRows - 1, (int32)(updateRect.bottom / CHAR_HEIGHT) + 1);

		char string[2];
		string[1] = 0;

		for (int32 y = startY; y <= endY; y++) {
			for (int32 x = startX; x <= endX; x++) {
				uint16 grid = fGlyphGrid[y * fColumns + x];
				uint8 glyph = grid >> 8;
				uint8 attr = grid & 0x00ff;
				string[0] = glyph;
				_RenderGlyph(x, y, string, attr, false);
			}
		}
	}

	DrawCursor(fCursorX, fCursorY);
}


void
ViewBuffer::DrawCursor(int32 x, int32 y)
{
	if (x < 0 || y < 0)
		return;
	
	x *= CHAR_WIDTH;
	y *= CHAR_HEIGHT;
	
	if (LockLooper()) {
		InvertRect(BRect(x, y, x + CHAR_WIDTH, y + CHAR_HEIGHT));
		Sync();
		UnlockLooper();
	}
}


void
ViewBuffer::MoveCursor(int32 x, int32 y)
{
	DrawCursor(fCursorX, fCursorY);
	DrawCursor(x, y);
	
	fCursorX = x;
	fCursorY = y;
}


void
ViewBuffer::Blit(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx, int32 desty)
{
	// blit inside the glyph grid
	if (fGlyphGrid) {
		int32 xOffset = destx - srcx;
		int32 yOffset = desty - srcy;

		int32 xIncrement;
		int32 yIncrement;

		uint16* src = fGlyphGrid + srcy * fColumns + srcx;

		if (xOffset > 0) {
			// copy from right to left
			xIncrement = -1;
			src += width - 1;
		} else {
			// copy from left to right
			xIncrement = 1;
		}
	
		if (yOffset > 0) {
			// copy from bottom to top
			yIncrement = -fColumns;
			src += (height - 1) * fColumns;
		} else {
			// copy from top to bottom
			yIncrement = fColumns;
		}
	
		uint16* dst = src + yOffset * fColumns + xOffset;
	
		for (int32 y = 0; y < height; y++) {
			uint16* srcHandle = src;
			uint16* dstHandle = dst;
			for (int32 x = 0; x < width; x++) {
				*dstHandle = *srcHandle;
				srcHandle += xIncrement;
				dstHandle += xIncrement;
			}
			src += yIncrement;
			dst += yIncrement;
		}
	}

	height *= CHAR_HEIGHT;
	width *= CHAR_WIDTH;
	
	srcx *= CHAR_WIDTH;
	srcy *= CHAR_HEIGHT;
	BRect source(srcx, srcy, srcx + width, srcy + height);
	
	destx *= CHAR_WIDTH;
	desty *= CHAR_HEIGHT;
	BRect dest(destx, desty, destx + width, desty + height);
	
	if (LockLooper()) {
		CopyBits(source, dest);
		Sync();
		UnlockLooper();
	}
}


void
ViewBuffer::Clear(uint8 attr)
{
	if (LockLooper()) {
		SetLowColor(GetPaletteEntry(BackgroundColor(attr)));
		SetViewColor(LowColor());
		FillRect(Frame(), B_SOLID_LOW);
		Sync();
		UnlockLooper();
	}
	
	fCursorX = -1;
	fCursorY = -1;

	if (fGlyphGrid)
		memset(fGlyphGrid, 0, fRows * fColumns * sizeof(uint16));
}


void
ViewBuffer::_RenderGlyph(int32 x, int32 y, const char* string, uint8 attr, bool fill)
{
	BPoint where(x * CHAR_WIDTH, (y + 1) * CHAR_HEIGHT - 3);
	
	SetHighColor(GetPaletteEntry(ForegroundColor(attr)));
	if (fill) {
		SetLowColor(GetPaletteEntry(BackgroundColor(attr)));
		FillRect(BRect(x * CHAR_WIDTH, y * CHAR_HEIGHT,
					   (x + 1) * CHAR_WIDTH, (y + 1) * CHAR_HEIGHT),
				 B_SOLID_LOW);
	}
	DrawString(string, where);
}

