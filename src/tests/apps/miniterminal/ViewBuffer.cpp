/*
 * ViewBuffer - Mimicing a frame buffer output - but using a BView.
 * Based on frame_buffer_console.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the Haiku License.
 *
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

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
	:	BView(frame, "ViewBuffer", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	SetFont(be_fixed_font);
	
	fColumns = frame.IntegerWidth() / CHAR_WIDTH;
	fRows = frame.IntegerHeight() / CHAR_HEIGHT;
	fResizeCallback = NULL;
	
	// initially, the cursor is hidden
	fCursorX = -1;
	fCursorY = -1;
	
	// initialize private palette
	for (int i = 0; i < 8; i++) {
		fPalette[i].red = (sPalette32[i] >> 16) & 0xff;
		fPalette[i].green = (sPalette32[i] >> 8) & 0xff;
		fPalette[i].blue = (sPalette32[i] >> 0) & 0xff;
		fPalette[i].alpha = 0xff;
	}
}


ViewBuffer::~ViewBuffer()
{
}


void
ViewBuffer::FrameResized(float width, float height)
{
	fColumns = (int32)width / CHAR_WIDTH;
	fRows = (int32)height / CHAR_HEIGHT;
	
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
	BPoint where(x * CHAR_WIDTH, (y + 1) * CHAR_HEIGHT - 3);
	
	char string[2];
	string[0] = glyph;
	string[1] = 0;
	
	if (LockLooper()) {
		SetHighColor(GetPaletteEntry(ForegroundColor(attr)));
		SetLowColor(GetPaletteEntry(BackgroundColor(attr)));
		FillRect(BRect(x * CHAR_WIDTH, y * CHAR_HEIGHT, (x + 1) * CHAR_WIDTH, (y + 1) * CHAR_HEIGHT), B_SOLID_LOW);
		DrawString(string, where);
		Sync();
		UnlockLooper();
	}
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
}
