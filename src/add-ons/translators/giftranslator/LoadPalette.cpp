////////////////////////////////////////////////////////////////////////////////
//
//	File: LoadPalette.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

#include "LoadPalette.h"
#include <GraphicsDefs.h>
#include <ByteOrder.h>

LoadPalette::LoadPalette() {
	backgroundindex = 0;
	usetransparent = false;
	transparentindex = 0;
	size = size_in_bits = 0;
}

// Never index into pal directly - this function is safe
uint32 LoadPalette::ColorForIndex(int index) {
	if (index >= 0 && index <= size) {
		if (usetransparent && index == transparentindex) return B_TRANSPARENT_MAGIC_RGBA32;
		else return data[index];
	} else {
		return B_BENDIAN_TO_HOST_INT32(0x000000ff);
	}
}

void LoadPalette::SetColor(int index, uint8 red, uint8 green, uint8 blue) {
	if (index < 0 || index > 255) return;
	data[index] = (blue << 24) + (green << 16) + (red << 8) + 0xff;
	data[index] = B_BENDIAN_TO_HOST_INT32(data[index]);
}

