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
//						Philippe Saint-Pierre, <stpere@gmail.com>
//						John Scipione, <jscipione@gmail.com>


#include "GIFSave.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <new>

#include "GIFPrivate.h"


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


GIFSave::GIFSave(BBitmap* bitmap, BPositionIO* output,
	TranslatorSettings* settings)
{
	fSettings = settings;
	color_space colorSpace = bitmap->ColorSpace();
	if (colorSpace != B_RGB32 && colorSpace != B_RGBA32
		&& colorSpace != B_RGB32_BIG && colorSpace != B_RGBA32_BIG) {
		if (debug)
			syslog(LOG_ERR, "GIFSave::GIFSave() - Unknown color space\n");

		fatalerror = true;
		return;
	}

	fatalerror = false;
	if (fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE) == OPTIMAL_PALETTE) {
		palette = new(std::nothrow) SavePalette(bitmap,
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE));
	} else {
		palette = new(std::nothrow) SavePalette(
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE));
	}

	if (palette == NULL) {
		fatalerror = true;
		return;
	}

	if (!palette->IsValid()) {
		delete palette;
		fatalerror = true;
		return;
	}

	width = bitmap->Bounds().IntegerWidth() + 1;
	height = bitmap->Bounds().IntegerHeight() + 1;
	if (debug) {
		syslog(LOG_INFO, "GIFSave::GIFSave() - "
			"Image dimensions are %d by %d\n", width, height);
	}

	if (fSettings->SetGetBool(GIF_SETTING_USE_DITHERING)) {
		if (debug)
			syslog(LOG_INFO, "GIFSave::GIFSave() - Using dithering\n");

		red_error = new(std::nothrow) int32[width + 2];
		if (red_error == NULL) {
			delete palette;
			fatalerror = true;
			return;
		}
		red_error = &red_error[1];
			// Allow index of -1 too

		green_error = new(std::nothrow) int32[width + 2];
		if (green_error == NULL) {
			delete palette;
			delete[] &red_error[-1];
			fatalerror = true;
			return;
		}
		green_error = &green_error[1];
			// Allow index of -1 too

		blue_error = new(std::nothrow) int32[width + 2];
		if (blue_error == NULL) {
			delete palette;
			delete[] &red_error[-1];
			delete[] &green_error[-1];
			fatalerror = true;
			return;
		}
		blue_error = &blue_error[1];
			// Allow index of -1 too

		red_side_error = green_side_error = blue_side_error = 0;
		for (int32 x = -1; x < width + 1; x++) {
			red_error[x] = 0;
			green_error[x] = 0;
			blue_error[x] = 0;
		}
	} else if (debug)
		syslog(LOG_INFO, "GIFSave::GIFSave() - Not using dithering\n");

	if (debug) {
		if (fSettings->SetGetBool(GIF_SETTING_INTERLACED))
			syslog(LOG_INFO, "GIFSave::GIFSave() - Interlaced, ");
		else
			syslog(LOG_INFO, "GIFSave::GIFSave() - Not interlaced, ");

		switch (fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE)) {
			case WEB_SAFE_PALETTE:
				syslog(LOG_INFO, "web safe palette\n");
				break;

			case BEOS_SYSTEM_PALETTE:
				syslog(LOG_INFO, "BeOS system palette\n");
				break;

			case GREYSCALE_PALETTE:
				syslog(LOG_INFO, "greyscale palette\n");
				break;

			case OPTIMAL_PALETTE:
			default:
				syslog(LOG_INFO, "optimal palette\n");
		}
	}

	if (fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT)) {
		if (fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO)) {
			palette->PrepareForAutoTransparency();
			if (debug) {
				syslog(LOG_INFO, "GIFSave::GIFSave() - "
					"Using transparent index %d\n",
					palette->TransparentIndex());
			}
		} else {
			palette->SetTransparentColor(
				(uint8)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_RED),
				(uint8)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_GREEN),
				(uint8)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_BLUE));
			if (debug) {
				syslog(LOG_INFO, "GIFSave::GIFSave() - "
					"Found transparent color %d,%d,%d at index %d\n",
					fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_RED),
					fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_GREEN),
					fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_BLUE),
					palette->TransparentIndex());
			}
		}
	} else {
		if (debug)
			syslog(LOG_INFO, "GIFSave::GIFSave() - Not using transparency\n");
	}

	this->output = output;
	this->bitmap = bitmap;

	if (WriteGIFHeader() != B_OK) {
		delete palette;
		delete[] &red_error[-1];
		delete[] &green_error[-1];
		delete[] &blue_error[-1];
		fatalerror = true;
		return;
	}

	if (debug)
		syslog(LOG_INFO, "GIFSave::GIFSave() - Wrote gif header\n");

	hash = new(std::nothrow) SFHash(1 << 16);
	if (hash == NULL) {
		delete palette;
		delete[] &red_error[-1];
		delete[] &green_error[-1];
		delete[] &blue_error[-1];
		fatalerror = true;
		return;
	}

	WriteGIFControlBlock();
	if (debug)
		syslog(LOG_INFO, "GIFSave::GIFSave() - Wrote gif control block\n");

	WriteGIFImageHeader();
	if (debug)
		syslog(LOG_INFO, "GIFSave::GIFSave() - Wrote gif image header\n");

	WriteGIFImageData();
	if (debug)
		syslog(LOG_INFO, "GIFSave::GIFSave() - Wrote gif image data\n");

	if (fSettings->SetGetBool(GIF_SETTING_USE_DITHERING)) {
		delete[] &red_error[-1];
		delete[] &green_error[-1];
		delete[] &blue_error[-1];
	}
	delete hash;

	// Terminating character
	char t = TERMINATOR_INTRODUCER;
	output->Write(&t, 1);
}


GIFSave::~GIFSave()
{
	delete palette;
	fSettings->Release();
}


status_t
GIFSave::WriteGIFHeader()
{
	// Standard header
	unsigned char header[]
		= { 'G', 'I', 'F', '8', '9', 'a', 0, 0, 0, 0, 0, 0, 0 };
	header[6] = width & 0xff;
	header[7] = (width & 0xff00) >> 8;
	header[8] = height & 0xff;
	header[9] = (height & 0xff00) >> 8;
	header[10] = 0xf0 | (palette->SizeInBits() - 1);
	header[11] = palette->BackgroundIndex();
	if (output->Write(header, 13) < 13)
		return B_IO_ERROR;

	// global palette
	int size = (1 << palette->SizeInBits()) * 3;
		// can't be bigger than this
	uint8* buffer = new(std::nothrow) uint8[size];
	if (buffer == NULL)
		return B_NO_MEMORY;

	palette->GetColors(buffer, size);
	if (output->Write(buffer, size) < size) {
		delete[] buffer;
		return B_IO_ERROR;
	}
	delete[] buffer;

	return B_OK;
}


status_t
GIFSave::WriteGIFControlBlock()
{
	unsigned char b[8] = {
		EXTENSION_INTRODUCER, GRAPHIC_CONTROL_LABEL, 0x04, 0x00, 0x00, 0x00,
		0x00, BLOCK_TERMINATOR
	};
	if (palette->UseTransparent()) {
		b[3] = b[3] | 1;
		b[6] = palette->TransparentIndex();
	}
	return output->Write(b, 8) < 8 ? B_IO_ERROR : B_OK;
}


status_t
GIFSave::WriteGIFImageHeader()
{
	unsigned char header[10];
	header[0] = DESCRIPTOR_INTRODUCER;
	header[1] = header[2] = 0;
	header[3] = header[4] = 0;

	header[5] = width & 0xff;
	header[6] = (width & 0xff00) >> 8;
	header[7] = height & 0xff;
	header[8] = (height & 0xff00) >> 8;

	if (fSettings->SetGetBool(GIF_SETTING_INTERLACED))
		header[9] = 0x40;
	else
		header[9] = BLOCK_TERMINATOR;

	return output->Write(header, 10) < 10 ? B_IO_ERROR : B_OK;
}


status_t
GIFSave::WriteGIFImageData()
{
	InitFrame();

	status_t result = B_OK;

	code_value = (short*)malloc(HASHSIZE * 2);
	if (code_value == NULL)
		return B_NO_MEMORY;

	prefix_code = (short*)malloc(HASHSIZE * 2);
	if (prefix_code == NULL) {
		free(code_value);

		return B_NO_MEMORY;
	}

	append_char = (unsigned char*)malloc(HASHSIZE);
	if (append_char == NULL) {
		free(code_value);
		free(prefix_code);

		return B_NO_MEMORY;
	}

	ResetHashtable();

	if (output->Write(&code_size, 1) < 1) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	result = OutputCode(clear_code, BITS);
	if (result != B_OK) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	string_code = NextPixel(0);
	int area = height * width;

	for (int x = 1; x < area; x++) {
		character = NextPixel(x);
		int y = 0;
		if ((y = CheckHashtable(string_code, character)) != -1)
			string_code = y;
		else {
			AddToHashtable(string_code, character);
			result = OutputCode(string_code, BITS);
			if (result != B_OK) {
				free(code_value);
				free(prefix_code);
				free(append_char);

				return B_IO_ERROR;
			}

			if (next_code > max_code) {
				BITS++;
				if (BITS > LZ_MAX_BITS) {
					result = OutputCode(clear_code, LZ_MAX_BITS);
					if (result != B_OK) {
						free(code_value);
						free(prefix_code);
						free(append_char);

						return B_IO_ERROR;
					}

					BITS = code_size + 1;
					ResetHashtable();
					next_code = clear_code + 1;
						// this is different
				}
				max_code = (1 << BITS) - 1;
			}
			string_code = character;
			next_code++;
		}
	}

	result = OutputCode(string_code, BITS);
	if (result != B_OK) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	result = OutputCode(end_code, BITS);
	if (result != B_OK) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	result = OutputCode(0, BITS, true);
	if (result != B_OK) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	char t = BLOCK_TERMINATOR;
	if (output->Write(&t, 1) < 1) {
		free(code_value);
		free(prefix_code);
		free(append_char);

		return B_IO_ERROR;
	}

	free(code_value);
	free(prefix_code);
	free(append_char);

	return result;
}


status_t
GIFSave::OutputCode(short code, int BITS, bool flush)
{
	if (!flush) {
		bit_buffer |= (unsigned int)code << bit_count;
		bit_count += BITS;
		while (bit_count >= 8) {
			byte_buffer[byte_count + 1] = (unsigned char)(bit_buffer & 0xff);
			byte_count++;
			bit_buffer >>= 8;
			bit_count -= 8;
		}
		if (byte_count >= 255) {
			byte_buffer[0] = 255;
			if (output->Write(byte_buffer, 256) < 256)
				return B_IO_ERROR;

			if (byte_count == 256) {
				byte_buffer[1] = byte_buffer[256];
				byte_count = 1;
			} else
				byte_count = 0;
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
			if (output->Write(byte_buffer, byte_count + 1) < byte_count + 1)
				return B_IO_ERROR;
		}
	}

	return B_OK;
}


void
GIFSave::ResetHashtable()
{
	for (int q = 0; q < HASHSIZE; q++) {
		code_value[q] = -1;
		prefix_code[q] = 0;
		append_char[q] = 0;
	}
}


int
GIFSave::CheckHashtable(int s, unsigned char c)
{
	if (s == -1)
		return c;

	int hashindex = HASH(s, c);
	int nextindex;
	while ((nextindex = code_value[hashindex]) != -1) {
		if (prefix_code[nextindex] == s && append_char[nextindex] == c)
			return nextindex;

		hashindex = (hashindex + HASHSTEP) % HASHSIZE;
	}

	return -1;
}


void
GIFSave::AddToHashtable(int s, unsigned char c)
{
	int hashindex = HASH(s, c);
	while (code_value[hashindex] != -1)
		hashindex = (hashindex + HASHSTEP) % HASHSIZE;

	code_value[hashindex] = next_code;
	prefix_code[next_code] = s;
	append_char[next_code] = c;
}


unsigned char
GIFSave::NextPixel(int pixel)
{
	int bpr = bitmap->BytesPerRow();
	color_space colorSpace = bitmap->ColorSpace();
	bool useAlphaForTransparency = colorSpace == B_RGBA32_BIG
		|| (fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO)
			&& colorSpace == B_RGBA32);
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	if (colorSpace == B_RGB32 || colorSpace == B_RGBA32) {
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

	if (!fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT)
		|| fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO)
		|| r != fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_RED)
		|| g != fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_GREEN)
		|| b != fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_BLUE)) {
		if (fSettings->SetGetBool(GIF_SETTING_USE_DITHERING)) {
			if (pixel % width == 0)
				red_side_error = green_side_error = blue_side_error = 0;

			b = min_c(255, max_c(0, b - blue_side_error));
			g = min_c(255, max_c(0, g - green_side_error));
			r = min_c(255, max_c(0, r - red_side_error));
		}
	}

	if (fSettings->SetGetBool(GIF_SETTING_INTERLACED)) {
		if (pos >= bpr) {
			pos = 0;
			row += gs_increment_pass_by[pass];
			while (row >= height) {
				pass++;
				row = gs_pass_starts_at[pass];
			}
			gifbits = (unsigned char*)bitmap->Bits() + (bpr * row);
		}
	}
#if 0
	unsigned int key = (r << 16) + (g << 8) + b;
	ColorCache* cc = (ColorCache*)hash->GetItem(key);
	if (cc == NULL) {
		cc = new ColorCache();
		cc->key = key;
		cc->index = palette->IndexForColor(r, g, b);
		hash->AddItem((HashItem*)cc);
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

		red_side_error = (red_error[x + 1]
			+ (red_total_error * seven_sixteenth)) >> 15;
		blue_side_error = (blue_error[x + 1]
			+ (blue_total_error * seven_sixteenth)) >> 15;
		green_side_error = (green_error[x + 1]
			+ (green_total_error * seven_sixteenth)) >> 15;

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
#endif

	int index = palette->IndexForColor(r, g, b, useAlphaForTransparency
		? a : 255);

	if (index != palette->TransparentIndex()
		&& fSettings->SetGetBool(GIF_SETTING_USE_DITHERING)) {
		int x = pixel % width;
		// don't carry error on to next line when interlaced because
		// that line won't be adjacent, hence error is meaningless
		if (fSettings->SetGetBool(GIF_SETTING_INTERLACED) && x == width - 1) {
			for (int32 y = -1; y < width + 1; y++) {
				red_error[y] = 0;
				green_error[y] = 0;
				blue_error[y] = 0;
			}
		}

		int32 red_total_error = palette->pal[index].red - r;
		int32 green_total_error = palette->pal[index].green - g;
		int32 blue_total_error = palette->pal[index].blue - b;

		red_side_error = (red_error[x + 1]
			+ (red_total_error * seven_sixteenth)) >> 15;
		blue_side_error = (blue_error[x + 1]
			+ (blue_total_error * seven_sixteenth)) >> 15;
		green_side_error = (green_error[x + 1]
			+ (green_total_error * seven_sixteenth)) >> 15;

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
	table_size = 1 << LZ_MAX_BITS;

	bit_count = 0;
	bit_buffer = 0;
	byte_count = 0;

	pass = pos = 0;
	row = gs_pass_starts_at[0];

	gifbits = (unsigned char*)bitmap->Bits();
}
