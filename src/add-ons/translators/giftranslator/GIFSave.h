////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFSave.h
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

#ifndef GIFSAVE_H
#define GIFSAVE_H

#include <DataIO.h>
#include <Bitmap.h>
#include "SavePalette.h"
#include "SFHash.h"
#include "Prefs.h"

#define HASHSIZE 9973
#define HASHSTEP 2039

#define HASH(index, lastbyte) (((lastbyte << 8) ^ index) % HASHSIZE)

class GIFSave {
	public:
		GIFSave(BBitmap *bitmap, BPositionIO *output);
		~GIFSave();

		bool fatalerror;

	private:
		void WriteGIFHeader();
		void WriteGIFControlBlock();
		void WriteGIFImageHeader();
		void WriteGIFImageData();
		void OutputCode(short code, int BITS, bool flush=false);

		unsigned char NextPixel(int pixel);
		void InitFrame();
		void ResetHashtable();
		int CheckHashtable(int s, unsigned char c);
		void AddToHashtable(int s, unsigned char c);
		
		BPositionIO *output;
		BBitmap *bitmap;
		SavePalette *palette;
		SFHash *hash;
		Prefs *prefs;
		
		short *code_value, *prefix_code;
		unsigned char *append_char;
		int BITS, max_code;
		char code_size;
		short clear_code, end_code, next_code;
		int string_code;
		unsigned char character;
		int table_size;
		
		int bit_count;
		unsigned int bit_buffer;
		int byte_count;
		unsigned char byte_buffer[257];
		int pass, row, pos;

		unsigned char *gifbits;
		
		int width, height;
		
		// For dithering
		int32 *red_error, *green_error, *blue_error;
		int16 red_side_error, green_side_error, blue_side_error;
};

#endif

