////////////////////////////////////////////////////////////////////////////////
//
//	File: LoadPalette.h
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

#ifndef LOAD_PALETTE_H
#define LOAD_PALETTE_H

#include <SupportDefs.h>

class LoadPalette {
	public:
		LoadPalette();
		uint32 ColorForIndex(int index);
		void SetColor(int index, uint8 red, uint8 green, uint8 blue);
		
		int size, size_in_bits;
		int backgroundindex, transparentindex;
		bool usetransparent;
		
	private:
		uint32 data[256];
};

#endif

