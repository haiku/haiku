////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFLoad.cpp
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

#include "GIFLoad.h"
#include <ByteOrder.h>
#include <TranslatorFormats.h>
#include <InterfaceDefs.h>
#include <stdlib.h>
#include <stdio.h>

extern bool debug;

GIFLoad::GIFLoad(BPositionIO *input, BPositionIO *output) {
	this->input = input;
	this->output = output;
	Init();
	
	if (!ReadGIFHeader()) {
		fatalerror = true;
		return;
	}
	
	if (debug) printf("GIFLoad::GIFLoad() - Image dimensions are %d x %d\n", width, height);
	
	unsigned char c;
	if (input->Read(&c, 1) < 1) {
		fatalerror = true;
		return;
	}
	while (c != 0x3b) {
		if (c == 0x2c) {
			if ((!ReadGIFImageHeader()) || (!ReadGIFImageData())) {
				if (debug) printf("GIFLoad::GIFLoad() - A fatal error occurred\n");
				fatalerror = true;
			} else {
				if (debug) printf("GIFLoad::GIFLoad() - Found a single image and leaving\n");
			}
			if (scanline != NULL) free(scanline);
			return;
		} else if (c == 0x21) {
			unsigned char d;
			if (input->Read(&d, 1) < 1) {
				fatalerror = true;
				return;
			}
			if (d == 0xff) {
				if (!ReadGIFLoopBlock()) {
					fatalerror = true;
					return;
				}
			} else if (d == 0xf9) {
				if (!ReadGIFControlBlock()) {
					fatalerror = true;
					return;
				}
			} else if (d == 0xfe) {
				if (!ReadGIFCommentBlock()) {
					fatalerror = true;
					return;
				}
			} else {
				if (!ReadGIFUnknownBlock(d)) {
					fatalerror = true;
					return;
				}
			}
		} else if (c != 0x00) {
			if (!ReadGIFUnknownBlock(c)) {
				fatalerror = true;
				return;
			}
		}
		if (input->Read(&c, 1) < 1) {
			fatalerror = true;
			return;
		}
	}
	if (debug) printf("GIFLoad::GIFLoad() - Done\n");
}

void GIFLoad::Init() {
	fatalerror = false;
	scanline = NULL;
	palette = NULL;
	input->Seek(0, SEEK_SET);
	head_memblock = NULL;
}

bool GIFLoad::ReadGIFHeader() {
	// Standard header
	unsigned char header[13];
	if (input->Read(header, 13) < 13) return false;
	width = header[6] + (header[7] << 8);
	height = header[8] + (header[9] << 8);
	
	palette = new LoadPalette();
	// Global palette
	if (header[10] & 0x80) {
		palette->size_in_bits = (header[10] & 0x07) + 1;
		if (debug) printf("GIFLoad::ReadGIFHeader() - Found %d bit global palette\n", palette->size_in_bits);
		int s = 1 << palette->size_in_bits;
		palette->size = s;

		unsigned char gp[256 * 3];
		if (input->Read(gp, s * 3) < s * 3) return false;
		for (int x = 0; x < s; x++) {
			palette->SetColor(x, gp[x * 3], gp[x * 3 + 1], gp[x * 3 + 2]);
		}
		palette->backgroundindex = header[11];
	} else { // Install BeOS system palette in case local palette isn't present
		color_map *map = (color_map *)system_colors();
		for (int x = 0; x < 256; x++) {
			palette->SetColor(x, map->color_list[x].red, map->color_list[x].green,
				map->color_list[x].blue);
		}
		palette->size = 256;
		palette->size_in_bits = 8;
	}
	return true;
}

bool GIFLoad::ReadGIFLoopBlock() {
	unsigned char length;
	if (input->Read(&length, 1) < 1) return false;
	input->Seek(length, SEEK_CUR);
	
	do {
		if (input->Read(&length, 1) < 1) {
			return false;
		}
		input->Seek(length, SEEK_CUR);
	} while (length != 0);

	return true;
}

bool GIFLoad::ReadGIFControlBlock() {
	unsigned char data[6];
	if (input->Read(data, 6) < 6) return false;
	if (data[1] & 0x01) {
		palette->usetransparent = true;
		palette->transparentindex = data[4];
		if (debug) printf("GIFLoad::ReadGIFControlBlock() - Transparency active, using palette index %d\n", data[4]);
	}
	return true;
}

bool GIFLoad::ReadGIFCommentBlock() {
	if (debug) printf("GIFLoad::ReadGIFCommentBlock() - Found:\n");
	unsigned char length;
	char comment_data[256];
	do {
		if (input->Read(&length, 1) < 1) return false;
		if (input->Read(comment_data, length) < length) return false;
		comment_data[length] = 0x00;
		if (debug) printf("%s", comment_data);
	} while (length != 0x00);
	if (debug) printf("\n");
	return true;
}

bool GIFLoad::ReadGIFUnknownBlock(unsigned char c) {
	if (debug) printf("GIFLoad::ReadGIFUnknownBlock() - Found: %d\n", c);
	unsigned char length;
	do {
		if (input->Read(&length, 1) < 1) return false;
		input->Seek(length, SEEK_CUR);
	} while (length != 0x00);
	return true;
}

bool GIFLoad::ReadGIFImageHeader() {
	unsigned char data[9];
	if (input->Read(data, 9) < 9) return false;
	
	int local_width = data[4] + (data[5] << 8);
	int local_height = data[6] + (data[7] << 8);
	if (width != local_width || height != local_height) {
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Local dimensions do not match global, setting to %d x %d\n",
			local_width, local_height);
		width = local_width;
		height = local_height;
	}
	
	scanline = (uint32 *)malloc(width * 4);
	if (scanline == NULL) {
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Could not allocate scanline\n");
		return false;
	}
		
	BRect rect(0, 0, width - 1, height - 1);
	TranslatorBitmap header;
	header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(rect.left);
	header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(rect.top);
	header.bounds.right = B_HOST_TO_BENDIAN_FLOAT(rect.right);
	header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(rect.bottom);
	header.rowBytes = B_HOST_TO_BENDIAN_INT32(width * 4);
	header.colors = (color_space)B_HOST_TO_BENDIAN_INT32(B_RGBA32);
	header.dataSize = B_HOST_TO_BENDIAN_INT32(width * 4 * height);
	if (output->Write(&header, 32) < 32) return false;

	// Has local palette
	if (data[8] & 0x80) {
		palette->size_in_bits = (data[8] & 0x07) + 1;
		int s = 1 << palette->size_in_bits;
		palette->size = s;
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Found %d bit local palette\n",
			palette->size_in_bits);
		
		unsigned char lp[256 * 3];
		if (input->Read(lp, s * 3) < s * 3) return false;
		for (int x = 0; x < s; x++) {
			palette->SetColor(x, lp[x * 3], lp[x * 3 + 1], lp[x * 3 + 2]);
		}
	}
	
	if (data[8] & 0x40) {
		interlaced = true;
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Image is interlaced\n");
	} else {
		interlaced = false;
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Image is not interlaced\n");
	}
	return true;
}

bool GIFLoad::ReadGIFImageData() {
	unsigned char new_entry[4096];
	
	unsigned char cs;
	input->Read(&cs, 1);
	if (cs == palette->size_in_bits) {
		if (!InitFrame(palette->size_in_bits)) return false;
	} else if (cs > palette->size_in_bits) {
		if (debug) printf("GIFLoad::ReadGIFImageData() - Code_size should be %d, not %d, allowing it\n", code_size, cs);
		if (!InitFrame(cs)) return false;
	} else if (cs < palette->size_in_bits) {
		if (debug) printf("GIFLoad::ReadGIFImageData() - Code_size should be %d, not %d\n", code_size, cs);
		return false;
	}
	
	if (debug) printf("GIFLoad::ReadGIFImageData() - Starting LZW\n");
	
	while ((new_code = NextCode()) != -1 && new_code != end_code) {
		if (new_code == clear_code) {
			ResetTable();
			new_code = NextCode();
			old_code[0] = new_code;
			old_code_length = 1;
			if (!OutputColor(old_code, 1)) goto bad_end;
			if (new_code == -1 || new_code == end_code) {
				if (debug) printf("GIFLoad::ReadGIFImageData() - Premature end_code or error\n");
				goto bad_end;
			}
			continue;
		}
		
		// Explicitly check for lack of clear code at start of file
		if (old_code_length == 0) {
			old_code[0] = new_code;
			old_code_length = 1;
			if (!OutputColor(old_code, 1)) goto bad_end;
			continue;
		}
		
		if (table[new_code] != NULL) { // Does exist in table
			if (!OutputColor(table[new_code], entry_size[new_code])) goto bad_end;
			
			//memcpy(new_entry, old_code, old_code_length);
			for (int x = 0; x < old_code_length; x++) {
				new_entry[x] = old_code[x];
			}
			
			//memcpy(new_entry + old_code_length, table[new_code], 1);
			new_entry[old_code_length] = *table[new_code];
		} else { // Does not exist in table
			//memcpy(new_entry, old_code, old_code_length);
			for (int x = 0; x < old_code_length; x++) {
				new_entry[x] = old_code[x];
			}
			
			//memcpy(new_entry + old_code_length, old_code, 1);
			new_entry[old_code_length] = *old_code;
			
			if (!OutputColor(new_entry, old_code_length + 1)) goto bad_end;
		}
		table[next_code] = MemblockAllocate(old_code_length + 1);

		//memcpy(table[next_code], new_entry, old_code_length + 1);
		for (int x = 0; x < old_code_length + 1; x++) {
			table[next_code][x] = new_entry[x];
		}
		
		entry_size[next_code] = old_code_length + 1;
		
		//memcpy(old_code, table[new_code], entry_size[new_code]);
		for (int x = 0; x < entry_size[new_code]; x++) {
			old_code[x] = table[new_code][x];
		}
		
		old_code_length = entry_size[new_code];
		next_code++;
		
		if (next_code > max_code && BITS != 12) {
			BITS++;
			max_code = (1 << BITS) - 1;
		}
	}

	MemblockDeleteAll();
	if (new_code == -1) return false;
	if (debug) printf("GIFLoad::ReadGIFImageData() - Done\n");
	return true;
	
bad_end:
	if (debug) printf("GIFLoad::ReadGIFImageData() - Reached a bad end\n");
	MemblockDeleteAll();
	return false;
}

short GIFLoad::NextCode() {
	while (bit_count < BITS) {
		if (byte_count == 0) {
			if (input->Read(&byte_count, 1) < 1) return -1;
			if (byte_count == 0) return end_code;
			if (input->Read(byte_buffer + (255 - byte_count), byte_count) < byte_count) return -1;
		}
		bit_buffer |= (unsigned int)byte_buffer[255 - byte_count] << bit_count;
		byte_count--;
		bit_count += 8;
	}

	short s = bit_buffer & ((1 << BITS) - 1);
	bit_buffer >>= BITS;
	bit_count -= BITS;
	return s;
}

void GIFLoad::ResetTable() {
	BITS = code_size + 1;
	next_code = clear_code + 2;
	max_code = (1 << BITS) - 1;
	
	MemblockDeleteAll();
	for (int x = 0; x < 4096; x++) {
		table[x] = NULL;
		if (x < (1 << code_size)) {
			table[x] = MemblockAllocate(1);
			table[x][0] = x;
			entry_size[x] = 1;
		}
	}
}

bool GIFLoad::InitFrame(int size) {
	code_size = size;
	if (code_size == 1) code_size++;
	BITS = code_size + 1;
	clear_code = 1 << code_size;
	end_code = clear_code + 1;
	next_code = clear_code + 2;
	max_code = (1 << BITS) - 1;
	pass = 0;
	if (interlaced) row = gl_pass_starts_at[0];
	else row = 0;
	
	bit_count = 0;
	bit_buffer = 0;
	byte_count = 0;
	old_code_length = 0;
	new_code = 0;
	scanline_position = 0;
	
	ResetTable();
	return true;
}

// Do 4k mallocs, keep them in a linked list, do a first fit across them
// when a new request comes along
uchar *GIFLoad::MemblockAllocate(int size) {
	if (head_memblock == NULL) {
		head_memblock = new Memblock();
		uchar *value = head_memblock->data;
		head_memblock->offset = size;
		head_memblock->next = NULL;
		return value;
	} else {
		Memblock *block = head_memblock;
		Memblock *last = NULL;
		while (block != NULL) {
			if (4096 - block->offset > size) {
				uchar *value = block->data + block->offset;
				block->offset += size;
				return value;
			}
			last = block;
			block = block->next;
		}

		block = new Memblock();
		uchar *value = block->data;
		block->offset = size;
		block->next = NULL;
		last->next = block;
		return value;
	}
}

// Delete the linked list
void GIFLoad::MemblockDeleteAll() {
	Memblock *block = NULL;
	while (head_memblock != NULL) {
		block = head_memblock->next;
		delete head_memblock;
		head_memblock = block;
	}
}

GIFLoad::~GIFLoad() {
	delete palette;
}

