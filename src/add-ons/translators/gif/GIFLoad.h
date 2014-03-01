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

// Additional authors:	John Scipione, <jscipione@gmail.com>

#ifndef GIF_LOAD_H
#define GIF_LOAD_H


#include <DataIO.h>
#include "LoadPalette.h"


#define GIF_INTERLACED		0x40
#define GIF_LOCALCOLORMAP	0x80


struct Memblock {
	uchar		data[4096];
	int			offset;
	Memblock*	next;
};


const int gl_pass_starts_at[]		= { 0, 4, 2, 1, 0 };
const int gl_increment_pass_by[]	= { 8, 8, 4, 2, 0 };


class GIFLoad {
public:
								GIFLoad(BPositionIO* input,
									BPositionIO* output);
	virtual						~GIFLoad();

			bool				fatalerror;

private:
			bool				ReadGIFHeader();
			bool				ReadGIFLoopBlock();
			bool				ReadGIFControlBlock();
			bool				ReadGIFImageHeader();
			bool				ReadGIFImageData();
			bool				ReadGIFCommentBlock();
			bool				ReadGIFUnknownBlock(unsigned char c);

			bool				InitFrame(int size);
			short				NextCode();
			void				ResetTable();

			uchar*				MemblockAllocate(int size);
			void				MemblockDeleteAll();

			inline	bool		OutputColor(unsigned char* string, int size);

			BPositionIO*		fInput;
			BPositionIO*		fOutput;
			LoadPalette*		fPalette;

			bool				fInterlaced;

			int					fPass;
			int					fRow;

			int					fWidth;
			int					fHeight;

			unsigned char		fOldCode[4096];
			unsigned int		fOldCodeLength;

			short				fNewCode;
			int					fBits;
			int					fMaxCode;
			int					fCodeSize;

			short				fClearCode;
			short				fEndCode;
			short				fNextCode;

			unsigned char*		fTable[4096];
			short				fEntrySize[4096];
			Memblock*			fHeadMemblock;

			int					fBitCount;
			unsigned int		fBitBuffer;
			unsigned char		fByteCount;
			unsigned char		fByteBuffer[255];

			uint32*				fScanLine;
			int					fScanlinePosition;
};


#endif	// GIF_LOAD_H
