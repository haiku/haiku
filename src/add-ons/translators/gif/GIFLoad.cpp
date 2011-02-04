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
#include <syslog.h>

extern bool debug;


GIFLoad::GIFLoad(BPositionIO *input, BPositionIO *output)
	:
	fatalerror(false),
	fInput(input),
	fOutput(output),
	fPalette(NULL),
 	fHeadMemblock(NULL),
	fScanLine(NULL)
{
	fInput->Seek(0, SEEK_SET);

	if (!ReadGIFHeader()) {
		fatalerror = true;
		return;
	}
	
	if (debug)
		syslog(LOG_ERR, "GIFLoad::GIFLoad() - Image dimensions are %d x %d\n",
				fWidth, fHeight);
	
	unsigned char c;
	if (fInput->Read(&c, 1) < 1) {
		fatalerror = true;
		return;
	}
	while (c != 0x3b) {
		if (c == 0x2c) {
			if ((!ReadGIFImageHeader()) || (!ReadGIFImageData())) {
				if (debug) 
					syslog(LOG_ERR, "GIFLoad::GIFLoad() - A fatal error occurred\n");
				fatalerror = true;
			} else {
				if (debug) 
					syslog(LOG_ERR, "GIFLoad::GIFLoad() - Found a single image and "
						"leaving\n");
			}
			free(fScanLine);
			fScanLine = NULL;
			return;
		} else if (c == 0x21) {
			unsigned char d;
			if (fInput->Read(&d, 1) < 1) {
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
		if (fInput->Read(&c, 1) < 1) {
			fatalerror = true;
			return;
		}
	}
	if (debug)
		syslog(LOG_ERR, "GIFLoad::GIFLoad() - Done\n");
}


bool
GIFLoad::ReadGIFHeader()
{
	// Standard header
	unsigned char header[13];
	if (fInput->Read(header, 13) < 13) return false;
	fWidth = header[6] + (header[7] << 8);
	fHeight = header[8] + (header[9] << 8);
	
	fPalette = new LoadPalette();
	// Global palette
	if (header[10] & GIF_LOCALCOLORMAP) {
		fPalette->size_in_bits = (header[10] & 0x07) + 1;
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFHeader() - Found %d bit global palette\n", 
				fPalette->size_in_bits);
		int s = 1 << fPalette->size_in_bits;
		fPalette->size = s;

		unsigned char gp[256 * 3];
		if (fInput->Read(gp, s * 3) < s * 3)
			return false;
		for (int x = 0; x < s; x++) {
			fPalette->SetColor(x, gp[x * 3], gp[x * 3 + 1], gp[x * 3 + 2]);
		}
		fPalette->backgroundindex = header[11];
	} else { // Install BeOS system palette in case local palette isn't present
		color_map *map = (color_map *)system_colors();
		for (int x = 0; x < 256; x++) {
			fPalette->SetColor(x, map->color_list[x].red, 
				map->color_list[x].green, map->color_list[x].blue);
		}
		fPalette->size = 256;
		fPalette->size_in_bits = 8;
	}
	return true;
}


bool
GIFLoad::ReadGIFLoopBlock()
{
	unsigned char length;
	if (fInput->Read(&length, 1) < 1) return false;
	fInput->Seek(length, SEEK_CUR);
	
	do {
		if (fInput->Read(&length, 1) < 1) {
			return false;
		}
		fInput->Seek(length, SEEK_CUR);
	} while (length != 0);

	return true;
}


bool
GIFLoad::ReadGIFControlBlock()
{
	unsigned char data[6];
	if (fInput->Read(data, 6) < 6)
		return false;
	if (data[1] & 0x01) {
		fPalette->usetransparent = true;
		fPalette->transparentindex = data[4];
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFControlBlock() - Transparency active, "
				"using palette index %d\n", data[4]);
	}
	return true;
}


bool 
GIFLoad::ReadGIFCommentBlock() 
{
	if (debug) syslog(LOG_ERR, "GIFLoad::ReadGIFCommentBlock() - Found:\n");
	unsigned char length;
	char comment_data[256];
	do {
		if (fInput->Read(&length, 1) < 1)
			return false;
		if (fInput->Read(comment_data, length) < length)
			return false;
		comment_data[length] = 0x00;
		if (debug)
			syslog(LOG_ERR, "%s", comment_data);
	} while (length != 0x00);
	if (debug)
		syslog(LOG_ERR, "\n");
	return true;
}


bool 
GIFLoad::ReadGIFUnknownBlock(unsigned char c) 
{
	if (debug)
		syslog(LOG_ERR, "GIFLoad::ReadGIFUnknownBlock() - Found: %d\n", c);
	unsigned char length;
	do {
		if (fInput->Read(&length, 1) < 1)
			return false;
		fInput->Seek(length, SEEK_CUR);
	} while (length != 0x00);
	return true;
}


bool 
GIFLoad::ReadGIFImageHeader() 
{
	unsigned char data[9];
	if (fInput->Read(data, 9) < 9)
		return false;
	
	int localWidth = data[4] + (data[5] << 8);
	int localHeight = data[6] + (data[7] << 8);
	if (fWidth != localWidth || fHeight != localHeight) {
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - Local dimensions do not "
				"match global, setting to %d x %d\n", localWidth, localHeight);
		fWidth = localWidth;
		fHeight = localHeight;
	}
	
	fScanLine = (uint32 *)malloc(fWidth * 4);
	if (fScanLine == NULL) {
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - Could not allocate "
				"scanline\n");
		return false;
	}
		
	BRect rect(0, 0, fWidth - 1, fHeight - 1);
	TranslatorBitmap header;
	header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(rect.left);
	header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(rect.top);
	header.bounds.right = B_HOST_TO_BENDIAN_FLOAT(rect.right);
	header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(rect.bottom);
	header.rowBytes = B_HOST_TO_BENDIAN_INT32(fWidth * 4);
	header.colors = (color_space)B_HOST_TO_BENDIAN_INT32(B_RGBA32);
	header.dataSize = B_HOST_TO_BENDIAN_INT32(fWidth * 4 * fHeight);
	if (fOutput->Write(&header, 32) < 32)
		return false;

	// Has local palette
	if (data[8] & GIF_LOCALCOLORMAP) {
		fPalette->size_in_bits = (data[8] & 0x07) + 1;
		int s = 1 << fPalette->size_in_bits;
		fPalette->size = s;
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - Found %d bit local "
				"palette\n", fPalette->size_in_bits);
		
		unsigned char lp[256 * 3];
		if (fInput->Read(lp, s * 3) < s * 3)
			return false;
		for (int x = 0; x < s; x++) {
			fPalette->SetColor(x, lp[x * 3], lp[x * 3 + 1], lp[x * 3 + 2]);
		}
	}
	
	fInterlaced = data[8] & GIF_INTERLACED;
	if (debug) {
		if (fInterlaced)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - Image is interlaced\n");
		else
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - Image is not "
				"interlaced\n");
	}
	return true;
}


bool
GIFLoad::ReadGIFImageData() 
{
	unsigned char newEntry[4096];
	
	unsigned char cs;
	fInput->Read(&cs, 1);
	if (cs == fPalette->size_in_bits) {
		if (!InitFrame(fPalette->size_in_bits))
			return false;
	} else if (cs > fPalette->size_in_bits) {
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Code_size should be %d, not "
				"%d, allowing it\n", fCodeSize, cs);
		if (!InitFrame(cs))
			return false;
	} else if (cs < fPalette->size_in_bits) {
		if (debug)
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Code_size should be %d, not "
				"%d\n", fCodeSize, cs);
		return false;
	}
	
	if (debug)
		syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Starting LZW\n");
	
	while ((fNewCode = NextCode()) != -1 && fNewCode != fEndCode) {
		if (fNewCode == fClearCode) {
			ResetTable();
			fNewCode = NextCode();
			fOldCode[0] = fNewCode;
			fOldCodeLength = 1;
			if (!OutputColor(fOldCode, 1)) goto bad_end;
			if (fNewCode == -1 || fNewCode == fEndCode) {
				if (debug)
					syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Premature fEndCode "
						"or error\n");
				goto bad_end;
			}
			continue;
		}
		
		// Explicitly check for lack of clear code at start of file
		if (fOldCodeLength == 0) {
			fOldCode[0] = fNewCode;
			fOldCodeLength = 1;
			if (!OutputColor(fOldCode, 1))
				goto bad_end;
			continue;
		}
		
		if (fTable[fNewCode] != NULL) { // Does exist in table
			if (!OutputColor(fTable[fNewCode], fEntrySize[fNewCode]))
				goto bad_end;
			
			//memcpy(newEntry, fOldCode, fOldCodeLength);
			for (int x = 0; x < fOldCodeLength; x++) {
				newEntry[x] = fOldCode[x];
			}
			
			//memcpy(newEntry + fOldCodeLength, fTable[fNewCode], 1);
			newEntry[fOldCodeLength] = *fTable[fNewCode];
		} else { // Does not exist in table
			//memcpy(newEntry, fOldCode, fOldCodeLength);
			for (int x = 0; x < fOldCodeLength; x++) {
				newEntry[x] = fOldCode[x];
			}
			
			//memcpy(newEntry + fOldCodeLength, fOldCode, 1);
			newEntry[fOldCodeLength] = *fOldCode;
			
			if (!OutputColor(newEntry, fOldCodeLength + 1))
				goto bad_end;
		}
		fTable[fNextCode] = MemblockAllocate(fOldCodeLength + 1);

		//memcpy(fTable[fNextCode], newEntry, fOldCodeLength + 1);
		for (int x = 0; x < fOldCodeLength + 1; x++) {
			fTable[fNextCode][x] = newEntry[x];
		}
		
		fEntrySize[fNextCode] = fOldCodeLength + 1;
		
		//memcpy(fOldCode, fTable[fNewCode], fEntrySize[fNewCode]);
		for (int x = 0; x < fEntrySize[fNewCode]; x++) {
			fOldCode[x] = fTable[fNewCode][x];
		}
		
		fOldCodeLength = fEntrySize[fNewCode];
		fNextCode++;
		
		if (fNextCode > fMaxCode && fBits != 12) {
			fBits++;
			fMaxCode = (1 << fBits) - 1;
		}
	}

	MemblockDeleteAll();
	if (fNewCode == -1)
		return false;
	if (debug)
		syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Done\n");
	return true;
	
bad_end:
	if (debug)
		syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - Reached a bad end\n");
	MemblockDeleteAll();
	return false;
}


short 
GIFLoad::NextCode() 
{
	while (fBitCount < fBits) {
		if (fByteCount == 0) {
			if (fInput->Read(&fByteCount, 1) < 1) return -1;
			if (fByteCount == 0) return fEndCode;
			if (fInput->Read(fByteBuffer + (255 - fByteCount), 
				fByteCount) < fByteCount) return -1;
		}
		fBitBuffer |= (unsigned int)fByteBuffer[255 - fByteCount] << fBitCount;
		fByteCount--;
		fBitCount += 8;
	}

	short s = fBitBuffer & ((1 << fBits) - 1);
	fBitBuffer >>= fBits;
	fBitCount -= fBits;
	return s;
}


void 
GIFLoad::ResetTable() 
{
	fBits = fCodeSize + 1;
	fNextCode = fClearCode + 2;
	fMaxCode = (1 << fBits) - 1;
	
	MemblockDeleteAll();
	for (int x = 0; x < 4096; x++) {
		fTable[x] = NULL;
		if (x < (1 << fCodeSize)) {
			fTable[x] = MemblockAllocate(1);
			fTable[x][0] = x;
			fEntrySize[x] = 1;
		}
	}
}


bool 
GIFLoad::InitFrame(int size) 
{
	fCodeSize = size;
	if (fCodeSize == 1)
		fCodeSize++;
	fBits = fCodeSize + 1;
	fClearCode = 1 << fCodeSize;
	fEndCode = fClearCode + 1;
	fNextCode = fClearCode + 2;
	fMaxCode = (1 << fBits) - 1;
	fPass = 0;
	if (fInterlaced)
		fRow = gl_pass_starts_at[0];
	else 
		fRow = 0;
	
	fBitCount = 0;
	fBitBuffer = 0;
	fByteCount = 0;
	fOldCodeLength = 0;
	fNewCode = 0;
	fScanlinePosition = 0;
	
	ResetTable();
	return true;
}


// Do 4k mallocs, keep them in a linked list, do a first fit across them
// when a new request comes along
uchar *
GIFLoad::MemblockAllocate(int size) 
{
	if (fHeadMemblock == NULL) {
		fHeadMemblock = new Memblock();
		uchar *value = fHeadMemblock->data;
		fHeadMemblock->offset = size;
		fHeadMemblock->next = NULL;
		return value;
	} else {
		Memblock *block = fHeadMemblock;
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
void 
GIFLoad::MemblockDeleteAll() 
{
	Memblock *block = NULL;
	while (fHeadMemblock != NULL) {
		block = fHeadMemblock->next;
		delete fHeadMemblock;
		fHeadMemblock = block;
	}
}


GIFLoad::~GIFLoad() 
{
	delete fPalette;
}

