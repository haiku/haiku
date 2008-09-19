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

#define GIF_INTERLACED		0x40
#define GIF_LOCALCOLORMAP	0x80


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
		
		bool InitFrame(int size);
		short NextCode();
		void ResetTable();
		
		uchar *MemblockAllocate(int size);
		void MemblockDeleteAll();

		inline bool OutputColor(unsigned char *string, int size) {
			int bpr = fWidth << 2;
			
			for (int x = 0; x < size; x++) {
				fScanLine[fScanlinePosition] = fPalette->ColorForIndex(string[x]);
				fScanlinePosition++;
				
				if (fScanlinePosition >= fWidth) {
					if (fOutput->WriteAt(32 + (fRow * bpr), fScanLine, bpr) < bpr) return false;
					fScanlinePosition = 0;
					if (fInterlaced) {
						fRow += gl_increment_pass_by[fPass];
						while (fRow >= fHeight) {
							fPass++;
							if (fPass > 3) return true;
							fRow = gl_pass_starts_at[fPass];
						}
					} else fRow++;
				}
			}
			return true;
		}
		
		BPositionIO *fInput, *fOutput;
		LoadPalette *fPalette;
		bool fInterlaced;
		int fPass, fRow, fWidth, fHeight;
		
		unsigned char fOldCode[4096];
		int fOldCodeLength;
		short fNewCode;
		int fBits, fMaxCode, fCodeSize;
		short fClearCode, fEndCode, fNextCode;
		
		unsigned char *fTable[4096];
		short fEntrySize[4096];
		Memblock *fHeadMemblock;
		
		int fBitCount;
		unsigned int fBitBuffer;
		unsigned char fByteCount;
		unsigned char fByteBuffer[255];

		uint32 *fScanLine;
		int fScanlinePosition;
};

#endif

