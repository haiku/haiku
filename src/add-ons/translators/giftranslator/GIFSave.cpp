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

#include "GIFSave.h"
#include <stdio.h>
#include <stdlib.h>

const int gs_pass_starts_at[] = {0, 4, 2, 1, 0};
const int gs_increment_pass_by[] = {8, 8, 4, 2, 0};
const int32 one_sixteenth = (int32)((1.0 / 16.0) * 32768);
const int32 three_sixteenth = (int32)((3.0 / 16.0) * 32768);
const int32 five_sixteenth = (int32)((5.0 / 16.0) * 32768);
const int32 seven_sixteenth = (int32)((7.0 / 16.0) * 32768);

extern bool debug;

GIFSave::GIFSave(BBitmap *bitmap, BPositionIO *output) {
	color_space cs = bitmap->ColorSpace();
    if (cs != B_RGB32 && cs != B_RGBA32 && cs != B_RGB32_BIG && cs != B_RGBA32_BIG) {
    	if (debug) printf("GIFSave::GIFSave() - Unknown color space\n");
    	fatalerror = true;
    	return;
    }
    
	fatalerror = false;
	prefs = new Prefs();
	if (prefs->palettemode == OPTIMAL_PALETTE) palette = new SavePalette(bitmap);
	else palette = new SavePalette(prefs->palettemode);
	if (palette->fatalerror) {
		fatalerror = true;
		return;
	}
	
	width = bitmap->Bounds().IntegerWidth() + 1;
	height = bitmap->Bounds().IntegerHeight() + 1;
	if (debug) printf("GIFSave::GIFSave() - Image dimensions are %d by %d\n", width, height);
	
	if (prefs->usedithering) {
		if (debug) printf("GIFSave::GIFSave() - Using dithering\n");
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
		if (debug) printf("GIFSave::GIFSave() - Not using dithering\n");
	}
	
	if (debug) {
		if (prefs->interlaced) printf("GIFSave::GIFSave() - Interlaced, ");
		else printf("GIFSave::GIFSave() - Not interlaced, ");
		switch (prefs->palettemode) {
			case WEB_SAFE_PALETTE:
				printf("web safe palette\n");
				break;
			case BEOS_SYSTEM_PALETTE:
				printf("BeOS system palette\n");
				break;
			case GREYSCALE_PALETTE:
				printf("greyscale palette\n");
				break;
			case OPTIMAL_PALETTE:
			default:
				printf("optimal palette\n");
				break;
		}
	}
	
	palette->usetransparent = prefs->usetransparent;
	if (prefs->usetransparent) {
		if (prefs->usetransparentindex) {
			palette->transparentindex = prefs->transparentindex;
			if (debug) printf("GIFSave::GIFSave() - Using transparent index %d\n", palette->transparentindex);
		} else {
			palette->transparentindex = -1;
			for (int x = 0; x < palette->size; x++) {
				if (palette->pal[x].red == prefs->transparentred &&
					palette->pal[x].green == prefs->transparentgreen &&
					palette->pal[x].blue == prefs->transparentblue) {
					
					palette->transparentindex = x;
					if (debug) printf("GIFSave::GIFSave() - Found transparent color %d,%d,%d at index %d\n", prefs->transparentred,
						prefs->transparentgreen, prefs->transparentblue, x);
					break;
				}
			}
			if (palette->transparentindex == -1) {
				palette->usetransparent = false;
				palette->transparentindex = 0;
				if (debug) printf("GIFSave::GIFSave() - Did not find color %d,%d,%d - deactivating transparency\n",
					prefs->transparentred, prefs->transparentgreen, prefs->transparentblue);
			}
		}
	} else {
		if (debug) printf("GIFSave::GIFSave() - Not using transparency\n");
	}

	this->output = output;
	this->bitmap = bitmap;
	WriteGIFHeader();
	if (debug) printf("GIFSave::GIFSave() - Wrote gif header\n");
		
	hash = new SFHash(1 << 16);
	WriteGIFControlBlock();
	if (debug) printf("GIFSave::GIFSave() - Wrote gif control block\n");
	WriteGIFImageHeader();
	if (debug) printf("GIFSave::GIFSave() - Wrote gif image header\n");
	WriteGIFImageData();
	if (debug) printf("GIFSave::GIFSave() - Wrote gif image data\n");
	
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

void GIFSave::WriteGIFHeader() {
	// Standard header
	unsigned char header[] = {'G', 'I', 'F', '8', '9', 'a', 0, 0, 0, 0, 0, 0, 0};
	header[6] = width & 0xff;
	header[7] = (width & 0xff00) >> 8;
	header[8] = height & 0xff;
	header[9] = (height & 0xff00) >> 8;
	header[10] = 0xf0 | (palette->size_in_bits - 1);
	header[11] = palette->backgroundindex;
	output->Write(header, 13);
	
	// Global palette
	int size = 1 << palette->size_in_bits;
	char c[256 * 3]; // can't be bigger than this
	for (int x = 0; x < size; x++) {
		c[x * 3] = palette->pal[x].red;
		c[x * 3 + 1] = palette->pal[x].green;
		c[x * 3 + 2] = palette->pal[x].blue;
	}
	output->Write(c, size * 3);
}

void GIFSave::WriteGIFControlBlock() {
	unsigned char b[8] = {0x21, 0xf9, 0x04, 0, 0, 0, 0, 0x00};
	if (palette->usetransparent) {
		b[3] = b[3] | 1;
		b[6] = palette->transparentindex;
	}
	output->Write(b, 8);
}

void GIFSave::WriteGIFImageHeader() {
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

void GIFSave::WriteGIFImageData() {
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

void GIFSave::OutputCode(short code, int BITS, bool flush) {
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

void GIFSave::ResetHashtable() {
	for (int q = 0; q < HASHSIZE; q++) {
		code_value[q] = -1;
		prefix_code[q] = 0;
		append_char[q] = 0;
	}
}

int GIFSave::CheckHashtable(int s, unsigned char c) {
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

void GIFSave::AddToHashtable(int s, unsigned char c) {
    int hashindex = HASH(s, c);
    while (code_value[hashindex] != -1)	hashindex = (hashindex + HASHSTEP) % HASHSIZE;
    code_value[hashindex] = next_code;
    prefix_code[next_code] = s;
    append_char[next_code] = c;
}

unsigned char GIFSave::NextPixel(int pixel) {
	int bpr = bitmap->BytesPerRow();
	color_space cs = bitmap->ColorSpace();
	unsigned char r, g, b;

	if (cs == B_RGB32 || cs == B_RGBA32) {
		b = gifbits[0];
		g = gifbits[1];
		r = gifbits[2];
	} else {
		r = gifbits[1];
		g = gifbits[2];
		b = gifbits[3];
	}
	gifbits += 4;
	pos += 4;
	
	if (prefs->usedithering) {
		if (pixel % width == 0) {
			red_side_error = green_side_error = blue_side_error = 0;
		}
		b = min_c(255, max_c(0, b - blue_side_error));
		g = min_c(255, max_c(0, g - green_side_error));
		r = min_c(255, max_c(0, r - red_side_error));
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
	
	return cc->index;
}

void GIFSave::InitFrame() {
	code_size = palette->size_in_bits;
	if (code_size == 1) code_size++;
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

GIFSave::~GIFSave() {
	delete palette;
	delete prefs;
}

