////////////////////////////////////////////////////////////////////////////////
//
//	File: SavePalette.h
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

#ifndef SAVE_PALETTE_H
#define SAVE_PALETTE_H

#include "SFHash.h"
#include <GraphicsDefs.h>
class BBitmap;

enum {
	WEB_SAFE_PALETTE = 0,
	BEOS_SYSTEM_PALETTE,
	GREYSCALE_PALETTE,
	OPTIMAL_PALETTE
};

class ColorItem : public HashItem {
	public:
		ColorItem(unsigned int k, unsigned int c);
        unsigned int count;
};

class ColorCache : public HashItem {
	public:
		unsigned char index;
};

class SavePalette {
	public:
		SavePalette(int predefined);
		SavePalette(BBitmap *bitmap);
		SavePalette();
		~SavePalette();
		
		unsigned char IndexForColor(unsigned char red, unsigned char green,
			unsigned char blue);
		unsigned char IndexForColor(rgb_color color);

		rgb_color *pal;
		int size, size_in_bits, mode;
		bool usetransparent;
		int backgroundindex, transparentindex;
		bool fatalerror;
};

#endif

