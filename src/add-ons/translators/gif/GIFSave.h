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

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>
//						Philippe Saint-Pierre, <stpere@gmail.com>
//						John Scipione, <jscipione@gmail.com>

#ifndef GIF_SAVE_H
#define GIF_SAVE_H


#include <DataIO.h>
#include <Bitmap.h>
#include "SavePalette.h"
#include "SFHash.h"

#include "GIFTranslator.h"


#define HASHSIZE 9973
#define HASHSTEP 2039

#define HASH(index, lastbyte) (((lastbyte << 8) ^ index) % HASHSIZE)


class GIFSave {
public:
								GIFSave(BBitmap* bitmap, BPositionIO* output,
									TranslatorSettings* settings);
	virtual						~GIFSave();

			bool				fatalerror;

private:
			status_t			WriteGIFHeader();
			status_t			WriteGIFControlBlock();
			status_t			WriteGIFImageHeader();
			status_t			WriteGIFImageData();
			status_t			OutputCode(short code, int BITS,
									bool flush = false);

			unsigned char		NextPixel(int pixel);
			void				InitFrame();
			void				ResetHashtable();
			int					CheckHashtable(int s, unsigned char c);
			void				AddToHashtable(int s, unsigned char c);

			BPositionIO*		output;
			BBitmap*			bitmap;
			SavePalette*		palette;
			SFHash*				hash;
			TranslatorSettings*	fSettings;

			short*				code_value;
			short*				prefix_code;

			unsigned char*		append_char;
			int					BITS;
			int					max_code;
			char				code_size;
			short				clear_code;
			short				end_code;
			short				next_code;
			int					string_code;
			unsigned char		character;
			int					table_size;

			int					bit_count;
			unsigned int		bit_buffer;
			int					byte_count;
			unsigned char		byte_buffer[257];
			int					pass;
			int					row;
			int					pos;

			unsigned char*		gifbits;

			int					width;
			int					height;

		// For dithering
			int32*				red_error;
			int32*				green_error;
			int32*				blue_error;

			int16				red_side_error;
			int16				green_side_error;
			int16				blue_side_error;
};


#endif	// GIF_SAVE_H
