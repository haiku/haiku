////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFSave.cpp
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

#include "GIFSave.h"
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

const int gs_pass_starts_at[] = {0, 4, 2, 1, 0};
const int gs_increment_pass_by[] = {8, 8, 4, 2, 0};
const int32 one_sixteenth = (int32)((1.0 / 16.0) * 32768);
const int32 three_sixteenth = (int32)((3.0 / 16.0) * 32768);
const int32 five_sixteenth = (int32)((5.0 / 16.0) * 32768);
const int32 seven_sixteenth = (int32)((7.0 / 16.0) * 32768);

extern bool debug;

class ColorCache : public HashItem {
	public:
		unsigned char index;
};

// constructor
GIFSave::GIFSave(BBitmap *bitmap, BPositionIO *output)
{
	color_space cs = bitmap->ColorSpace();
    if (cs != B_RGB32 && cs != B_RGBA32 && cs != B_RGB32_BIG && cs != B_RGBA32_BIG) {
    	if (debug) syslog(LOG_ERR, "GIFSave::GIFSave() - Unknown color space\n");
    	fatalerror = true;
    	return;
    }
    
	fatalerror = false;
	prefs = new Prefs();
	if (prefs->palettemode == OPTIMAL_PALETTE)
		palette = new SavePalette(bitmap, prefs->palette_size_in_bits);
	else
		palette = new SavePalette(prefs->palettemode);
	if (!palette->IsValid()) {
		fatalerror = true;
		return;
	}
	
	width = bitmap->Bounds().IntegerWidth() + 1;
	height = bitmap->Bounds().IntegerHeight() + 1;
	if (debug)
	   syslog(LOG_ERR, "GIFSave::GIFSave() - Image dimensions are %d by %d\n",
					width, height);
	
	if (prefs->usedithering) {
		if (debug)
		   syslog(LOG_ERR, "GIFSave::GIFSave() - Using dithering\n");
		red_error = new int32[width + 2];
		red_error = &red_error[1]; // Allow index of -1 too
		green_error = new int32[width + 2];
		green_error = &green_error[1]; // Allow index of -1 too
		blue_error = new int32[width + 2];
		blue_error = &blue_error[1]; // Allow index of -1 too
		
		red_side_error = green_side_error = blue_side_error = 0;
		for (int32 x = -1; x < width + 1; x++) {
			red_error[x] = 0;
			green_error[x] = 0;
			blue_error[x] = 0;
		}
	} else {
		if (debug)
		   syslog(LOG_ERR, "GIFSave::GIFSave() - Not using dithering\n");
	}
	
	if (debug) {
		if (prefs->interlaced)
		   syslog(LOG_ERR, "GIFSave::GIFSave() - Interlaced, ");
		else syslog(LOG_ERR, "GIFSave::GIFSave() - Not interlaced, ");
		switch (prefs->palettemode) {
			case WEB_SAFE_PALETTE:
				syslog(LOG_ERR, "web safe palette\n");
				break;
			case BEOS_SYSTEM_PALETTE:
				syslog(LOG_ERR, "BeOS system palette\n");
				break;
			case GREYSCALE_PALETTE:
				syslog(LOG_ERR, "greyscale palette\n");
				break;
			case OPTIMAL_PALETTE:
			default:
				syslog(LOG_ERR, "optimal palette\n");
				break;
		}
	}
	
	if (prefs->usetransparent) {
		if (prefs->usetransparentauto) {
			palette->PrepareForAutoTransparency();
			if (debug)
				syslog(LOG_ERR, "GIFSave::GIFSave() - Using transparent index %d\n", 
					palette->TransparentIndex());
		} else {
			palette->SetTransparentColor((uint8)prefs->transparentred,
									     (uint8)prefs->transparentgreen,
									     (uint8)prefs->transparentblue);
			if (debug) {
				syslog(LOG_ERR, "GIFSave::GIFSave() - Found transparent color %d,%d,%d "
					"at index %d\n", prefs->transparentred, 
					prefs->transparentgreen, prefs->transparentblue,
					palette->TransparentIndex());
			}
		}
	} else {
		if (debug)
			syslog(LOG_ERR, "GIFSave::GIFSave() - Not using transparency\n");
	}

	this->output = output;
	this->bitmap = bitmap;
	WriteGIFHeader();
	if (debug) syslog(LOG_ERR, "GIFSave::GIFSave() - Wrote gif header\n");
		
	hash = new SFHash(1 << 16);
	WriteGIFControlBlock();
	if (debug) syslog(LOG_ERR, "GIFSave::GIFSave() - Wrote gif control block\n");
	WriteGIFImageHeader();
	if (debug) syslog(LOG_ERR, "GIFSave::GIFSave() - Wrote gif image header\n");
	WriteGIFImageData();
	if (debug) syslog(LOG_ERR, "GIFSave::GIFSave() - Wrote gif image data\n");
	
	if (prefs->usedithering) {
		delete [] &red_error[-1];
		delete [] &green_error[-1];
		delete [] &blue_error[-1];
	}
	delete hash;

	// Terminating character
	char t = 0x3b;
	output->Write(&t, 1);
}

// destructor
GIFSave::~GIFSave()
{
	delete palette;
	delete prefs;
}

// WriteGIFHeader
void
GIFSave::WriteGIFHeader()
{
	// Standard header
	unsigned char header[] = {'G', 'I', 'F', '8', '9', 'a', 0, 0, 0, 0, 0, 0, 0};
	header[6] = width & 0xff;
	header[7] = (width & 0xff00) >> 8;
	header[8] = height & 0xff;
	header[9] = (height & 0xff00) >> 8;
	header[10] = 0xf0 | (palette->SizeInBits() - 1);
	header[11] = palette->BackgroundIndex();
	output->Write(header, 13);
	
	// Global palette
	int size = (1 << palette->SizeInBits()) * 3;
	uint8* buffer = new uint8[size]; // can't be bigger than this
	palette->GetColors(buffer, size);
	output->Write(buffer, size);
	delete[] buffer;
}

// WriteGIFControlBlock
void
GIFSave::WriteGIFControlBlock()
{
	unsigned char b[8] = {0x21, 0xf9, 0x04, 0, 0, 0, 0, 0x00};
	if (palette->UseTransparent()) {
		b[3] = b[3] | 1;
		b[6] = palette->TransparentIndex();
	}
	output->Write(b, 8);
}

// WriteGIFImageHeader
void GIFSave::WriteGIFImageHeader()
{
	unsigned char header[10];
	header[0] = 0x2c;
	header[1] = header[2] = 0;
	header[3] = header[4] = 0;
	
	header[5] = width & 0xff;
	header[6] = (width & 0xff00) >> 8;
	header[7] = height & 0xff;
	header[8] = (height & 0xff00) >> 8;

	if (prefs->interlaced) header[9] = 0x40;
	else header[9] = 0x00;
	output->Write(header, 10);
}

// WriteGIFImageData
void GIFSave::WriteGIFImageData()
{
	InitFrame();
	code_value = (short *)malloc(HASHSIZE * 2);
	prefix_code = (short *)malloc(HASHSIZE * 2);
	append_char = (unsigned char *)malloc(HASHSIZE);
	ResetHashtable();
	
	output->Write(&code_size, 1);
	OutputCode(clear_code, BITS);
	string_code = NextPixel(0);
	int area = height * width;
	
	for (int x = 1; x < area; x++) {
		character = NextPixel(x);
		int y = 0;
		if ((y = CheckHashtable(string_code, character)) != -1) {
			string_code = y;
		} else {
			AddToHashtable(string_code, character);
			OutputCode(string_code, BITS);
			
			if (next_code > max_code) {
				BITS++;
				if (BITS > 12) {
					OutputCode(clear_code, 12);
					BITS = code_size + 1;
					ResetHashtable();
					next_code = clear_code + 1; // this is different
				}
				max_code = (1 << BITS) - 1;
			}
			string_code = character;
			next_code++;
		}
	}
	OutputCode(string_code, BITS);
	OutputCode(end_code, BITS);
	OutputCode(0, BITS, true);
	char t = 0x00;
	output->Write(&t, 1);
	free(code_value);
	free(prefix_code);
	free(append_char);
}

// OutputCode
void
GIFSave::OutputCode(short code, int BITS, bool flush)
{
	if (!flush) {
		bit_buffer |= (unsigned int) code << bit_count;
		bit_count += BITS;
		while (bit_count >= 8) {
		  byte_buffer[byte_count + 1] = (unsigned char)(bit_buffer & 0xff);
		  byte_count++;
		  bit_buffer >>= 8;
		  bit_count -= 8;
		}
		if (byte_count >= 255) {
			byte_buffer[0] = 255;
			output->Write(byte_buffer, 256);
			if (byte_count == 256) {
				byte_buffer[1] = byte_buffer[256];
				byte_count = 1;
			} else byte_count = 0;
		}
	} else {
		bit_buffer |= (unsigned int) code << bit_count;
		bit_count += BITS;
		while (bit_count > 0) {
			byte_buffer[byte_count + 1] = (unsigned char)(bit_buffer & 0xff);
			byte_count++;
			bit_buffer >>= 8;
			bit_count -= 8;
		}
		if (byte_count > 0) {
			byte_buffer[0] = (unsigned char)byte_count;
			output->Write(byte_buffer, byte_count + 1);
		}
	}
}

// ResetHashtable
void
GIFSave::ResetHashtable()
{
	for (int q = 0; q < HASHSIZE; q++) {
		code_value[q] = -1;
		prefix_code[q] = 0;
		append_char[q] = 0;
	}
}

// CheckHashtable
int
GIFSave::CheckHashtable(int s, unsigned char c)
{
	if (s == -1) return c;
	int hashindex = HASH(s, c);
	int nextindex;
	while ((nextindex = code_value[hashindex]) != -1) {
        if (prefix_code[nextindex] == s && append_char[nextindex] == c)
            return nextindex;
        hashindex = (hashindex + HASHSTEP) % HASHSIZE;
    }
	return -1;
}

// AddToHashtable
void
GIFSave::AddToHashtable(int s, unsigned char c)
{
    int hashindex = HASH(s, c);
    while (code_value[hashindex] != -1)	hashindex = (hashindex + HASHSTEP) % HASHSIZE;
    code_value[hashindex] = next_code;
    prefix_code[next_code] = s;
    append_char[next_code] = c;
}

// NextPixel
unsigned char
GIFSave::NextPixel(int pixel)
{
	int bpr = bitmap->BytesPerRow();
	color_space cs = bitmap->ColorSpace();
	bool useAlphaForTransparency = (prefs->usetransparentauto && cs == B_RGBA32) || cs == B_RGBA32_BIG;
	unsigned char r, g, b, a;

	if (cs == B_RGB32 || cs == B_RGBA32) {
		b = gifbits[0];
		g = gifbits[1];
		r = gifbits[2];
		a = gifbits[3];
	} else {
		a = gifbits[0];
		r = gifbits[1];
		g = gifbits[2];
		b = gifbits[3];
	}
	gifbits += 4;
	pos += 4;

	if (!prefs->usetransparent || prefs->usetransparentauto ||
		r != prefs->transparentred ||
		g != prefs->transparentgreen ||
		b != prefs->transparentblue) {
	
		if (prefs->usedithering) {
			if (pixel % width == 0) {
				red_side_error = green_side_error = blue_side_error = 0;
			}
			b = min_c(255, max_c(0, b - blue_side_error));
			g = min_c(255, max_c(0, g - green_side_error));
			r = min_c(255, max_c(0, r - red_side_error));
		}
	}
	
	if (prefs->interlaced) {
		if (pos >= bpr) {
			pos = 0;
			row += gs_increment_pass_by[pass];
			while (row >= height) {
				pass++;
				row = gs_pass_starts_at[pass];
			}
			gifbits = (unsigned char *)bitmap->Bits() + (bpr * row);
		}
	}
/*
	unsigned int key = (r << 16) + (g << 8) + b;
	ColorCache *cc = (ColorCache *)hash->GetItem(key);
	if (cc == NULL) {
		cc = new ColorCache();
		cc->key = key;
		cc->index = palette->IndexForColor(r, g, b);
		hash->AddItem((HashItem *)cc);
	}

	if (prefs->usedithering) {
		int x = pixel % width;
		// Don't carry error on to next line when interlaced because
		// that line won't be adjacent, hence error is meaningless
		if (prefs->interlaced && x == width - 1) {
			for (int32 y = -1; y < width + 1; y++) {
				red_error[y] = 0;
				green_error[y] = 0;
				blue_error[y] = 0;
			}
		}

		int32 red_total_error = palette->pal[cc->index].red - r;
		int32 green_total_error = palette->pal[cc->index].green - g;
		int32 blue_total_error = palette->pal[cc->index].blue - b;

		red_side_error = (red_error[x + 1] + (red_total_error * seven_sixteenth)) >> 15;
		blue_side_error = (blue_error[x + 1] + (blue_total_error * seven_sixteenth)) >> 15;
		green_side_error = (green_error[x + 1] + (green_total_error * seven_sixteenth)) >> 15;
		
		red_error[x - 1] += (red_total_error * three_sixteenth);
		green_error[x - 1] += (green_total_error * three_sixteenth);
		blue_error[x - 1] += (blue_total_error * three_sixteenth);

		red_error[x] += (red_total_error * five_sixteenth);
		green_error[x] += (green_total_error * five_sixteenth);
		blue_error[x] += (blue_total_error * five_sixteenth);

		red_error[x + 1] = (red_total_error * one_sixteenth);
		green_error[x + 1] = (green_total_error * one_sixteenth);
		blue_error[x + 1] = (blue_total_error * one_sixteenth);
	}
	
	return cc->index;*/

	int index = palette->IndexForColor(r, g, b, useAlphaForTransparency ? a : 255);

	if (index != palette->TransparentIndex() && prefs->usedithering) {
		int x = pixel % width;
		// Don't carry error on to next line when interlaced because
		// that line won't be adjacent, hence error is meaningless
		if (prefs->interlaced && x == width - 1) {
			for (int32 y = -1; y < width + 1; y++) {
				red_error[y] = 0;
				green_error[y] = 0;
				blue_error[y] = 0;
			}
		}

		int32 red_total_error = palette->pal[index].red - r;
		int32 green_total_error = palette->pal[index].green - g;
		int32 blue_total_error = palette->pal[index].blue - b;

		red_side_error = (red_error[x + 1] + (red_total_error * seven_sixteenth)) >> 15;
		blue_side_error = (blue_error[x + 1] + (blue_total_error * seven_sixteenth)) >> 15;
		green_side_error = (green_error[x + 1] + (green_total_error * seven_sixteenth)) >> 15;
		
		red_error[x - 1] += (red_total_error * three_sixteenth);
		green_error[x - 1] += (green_total_error * three_sixteenth);
		blue_error[x - 1] += (blue_total_error * three_sixteenth);

		red_error[x] += (red_total_error * five_sixteenth);
		green_error[x] += (green_total_error * five_sixteenth);
		blue_error[x] += (blue_total_error * five_sixteenth);

		red_error[x + 1] = (red_total_error * one_sixteenth);
		green_error[x + 1] = (green_total_error * one_sixteenth);
		blue_error[x + 1] = (blue_total_error * one_sixteenth);
	}

	return index;
}

// InitFrame
void
GIFSave::InitFrame()
{
	code_size = palette->SizeInBits();
	if (code_size == 1)
		code_size++;
	BITS = code_size + 1;
	clear_code = 1 << code_size;
	end_code = clear_code + 1;
	next_code = clear_code + 2;
	max_code = (1 << BITS) - 1;
	string_code = 0;
	character = 0;
	table_size = 1 << 12;
	
	bit_count = 0;
	bit_buffer = 0;
	byte_count = 0;
	
	pass = pos = 0;
	row = gs_pass_starts_at[0];
	
	gifbits = (unsigned char *)bitmap->Bits();
}
