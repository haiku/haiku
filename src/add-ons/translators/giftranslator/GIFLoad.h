////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFLoad.h
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

#ifndef GIFLOAD_H
#define GIFLOAD_H

#include <DataIO.h>
#include "LoadPalette.h"

class Memblock {
	public:
		uchar data[4096];
		int offset;
		Memblock *next;
};

const int gl_pass_starts_at[] = {0, 4, 2, 1, 0};
const int gl_increment_pass_by[] = {8, 8, 4, 2, 0};

class GIFLoad {
	public:
		GIFLoad(BPositionIO *input, BPositionIO *output);
		~GIFLoad();
		bool fatalerror;
		
	private:
		bool ReadGIFHeader();
		bool ReadGIFLoopBlock();
		bool ReadGIFControlBlock();
		bool ReadGIFImageHeader();
		bool ReadGIFImageData();
		bool ReadGIFCommentBlock();
		bool ReadGIFUnknownBlock(unsigned char c);
		
		void Init();
		bool InitFrame(int size);
		short NextCode();
		void ResetTable();
		
		uchar *MemblockAllocate(int size);
		void MemblockDeleteAll();

		inline bool OutputColor(unsigned char *string, int size) {
			int bpr = width << 2;
			
			for (int x = 0; x < size; x++) {
				scanline[scanline_position] = palette->ColorForIndex(string[x]);
				scanline_position++;
				
				if (scanline_position >= width) {
					if (output->WriteAt(32 + (row * bpr), scanline, bpr) < bpr) return false;
					scanline_position = 0;
					if (interlaced) {
						row += gl_increment_pass_by[pass];
						while (row >= height) {
							pass++;
							if (pass > 3) return true;
							row = gl_pass_starts_at[pass];
						}
					} else row++;
				}
			}
			return true;
		}
		
		BPositionIO *input, *output;
		LoadPalette *palette;
		bool interlaced;
		int pass, row, width, height;
		
		unsigned char old_code[4096];
		int old_code_length;
		short new_code;
		int BITS, max_code, code_size;
		short clear_code, end_code, next_code;
		
		unsigned char *table[4096];
		short entry_size[4096];
		Memblock *head_memblock;
		
		int bit_count;
		unsigned int bit_buffer;
		unsigned char byte_count;
		unsigned char byte_buffer[255];

		uint32 *scanline;
		int scanline_position;
};

#endif

