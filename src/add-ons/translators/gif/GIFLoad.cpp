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

// Additional authors:	John Scipione, <jscipione@gmail.com>


#include "GIFLoad.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <new>

#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <TranslatorFormats.h>

#include "GIFPrivate.h"


extern bool debug;


GIFLoad::GIFLoad(BPositionIO* input, BPositionIO* output)
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

	if (debug) {
		syslog(LOG_INFO, "GIFLoad::GIFLoad() - Image dimensions are %d x %d\n",
			fWidth, fHeight);
	}

	unsigned char c;
	if (fInput->Read(&c, 1) < 1) {
		fatalerror = true;
		return;
	}

	while (c != TERMINATOR_INTRODUCER) {
		if (c == DESCRIPTOR_INTRODUCER) {
			if ((!ReadGIFImageHeader()) || (!ReadGIFImageData())) {
				if (debug) {
					syslog(LOG_ERR, "GIFLoad::GIFLoad() - "
						"A fatal error occurred\n");
				}

				fatalerror = true;
			} else {
				if (debug) {
					syslog(LOG_INFO, "GIFLoad::GIFLoad() - "
						"Found a single image and leaving\n");
				}
			}
			free(fScanLine);
			fScanLine = NULL;
			return;
		} else if (c == EXTENSION_INTRODUCER) {
			unsigned char d;
			if (fInput->Read(&d, 1) < 1) {
				fatalerror = true;
				return;
			}
			if (d == LOOP_BLOCK_LABEL) {
				if (!ReadGIFLoopBlock()) {
					fatalerror = true;
					return;
				}
			} else if (d == GRAPHIC_CONTROL_LABEL) {
				if (!ReadGIFControlBlock()) {
					fatalerror = true;
					return;
				}
			} else if (d == COMMENT_EXTENSION_LABEL) {
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
		} else if (c != BLOCK_TERMINATOR) {
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
		syslog(LOG_INFO, "GIFLoad::GIFLoad() - Done\n");
}


GIFLoad::~GIFLoad()
{
	delete fPalette;
}


bool
GIFLoad::ReadGIFHeader()
{
	// standard header
	unsigned char header[13];
	if (fInput->Read(header, 13) < 13)
		return false;

	fWidth = header[6] + (header[7] << 8);
	fHeight = header[8] + (header[9] << 8);

	fPalette = new(std::nothrow) LoadPalette();
	if (fPalette == NULL)
		return false;

	// Global palette
	if (header[10] & GIF_LOCALCOLORMAP) {
		fPalette->size_in_bits = (header[10] & 0x07) + 1;
		if (debug) {
			syslog(LOG_INFO, "GIFLoad::ReadGIFHeader() - "
				"Found %d bit global palette\n",
				fPalette->size_in_bits);
		}
		int s = 1 << fPalette->size_in_bits;
		fPalette->size = s;

		unsigned char gp[256 * 3];
		if (fInput->Read(gp, s * 3) < s * 3)
			return false;

		for (int x = 0; x < s; x++)
			fPalette->SetColor(x, gp[x * 3], gp[x * 3 + 1], gp[x * 3 + 2]);

		fPalette->backgroundindex = header[11];
	} else {
		// install BeOS system palette in case local palette isn't present
		color_map* map = (color_map*)system_colors();
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
	if (fInput->Read(&length, 1) < 1)
		return false;

	fInput->Seek(length, SEEK_CUR);
	do {
		if (fInput->Read(&length, 1) < 1)
			return false;

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

	if ((data[1] & 0x01) != 0) {
		fPalette->usetransparent = true;
		fPalette->transparentindex = data[4];
		if (debug) {
			syslog(LOG_INFO, "GIFLoad::ReadGIFControlBlock() - "
				"Transparency active, using palette index %d\n", data[4]);
		}
	}

	return true;
}


bool
GIFLoad::ReadGIFCommentBlock()
{
	if (debug)
		syslog(LOG_INFO, "GIFLoad::ReadGIFCommentBlock() - Found:\n");

	unsigned char length;
	char comment_data[256];
	do {
		if (fInput->Read(&length, 1) < 1)
			return false;

		if (fInput->Read(comment_data, length) < length)
			return false;

		comment_data[length] = BLOCK_TERMINATOR;
		if (debug)
			syslog(LOG_INFO, "%s", comment_data);
	} while (length != BLOCK_TERMINATOR);

	if (debug)
		syslog(LOG_INFO, "\n");

	return true;
}


bool
GIFLoad::ReadGIFUnknownBlock(unsigned char c)
{
	if (debug)
		syslog(LOG_INFO, "GIFLoad::ReadGIFUnknownBlock() - Found: %d\n", c);

	unsigned char length;
	do {
		if (fInput->Read(&length, 1) < 1)
			return false;

		fInput->Seek(length, SEEK_CUR);
	} while (length != BLOCK_TERMINATOR);

	return true;
}


bool
GIFLoad::ReadGIFImageHeader()
{
	unsigned char data[9];
	if (fInput->Read(data, 9) < 9)
		return false;

	int left = data[0] + (data[1] << 8);
	int top = data[2] + (data[3] << 8);
	int localWidth = data[4] + (data[5] << 8);
	int localHeight = data[6] + (data[7] << 8);
	if (fWidth != localWidth || fHeight != localHeight) {
		if (debug) {
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - "
				"Local dimensions do not match global, setting to %d x %d\n",
				localWidth, localHeight);
		}
		fWidth = localWidth;
		fHeight = localHeight;
	}

	fScanLine = (uint32*)malloc(fWidth * 4);
	if (fScanLine == NULL) {
		if (debug) {
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageHeader() - "
				"Could not allocate scanline\n");
		}
		return false;
	}

	BRect rect(left, top, left + fWidth - 1, top + fHeight - 1);
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

	if (data[8] & GIF_LOCALCOLORMAP) {
		// has local palette
		fPalette->size_in_bits = (data[8] & 0x07) + 1;
		int s = 1 << fPalette->size_in_bits;
		fPalette->size = s;
		if (debug) {
			syslog(LOG_INFO, "GIFLoad::ReadGIFImageHeader() - "
				"Found %d bit local palette\n", fPalette->size_in_bits);
		}

		unsigned char lp[256 * 3];
		if (fInput->Read(lp, s * 3) < s * 3)
			return false;

		for (int x = 0; x < s; x++)
			fPalette->SetColor(x, lp[x * 3], lp[x * 3 + 1], lp[x * 3 + 2]);
	}

	fInterlaced = data[8] & GIF_INTERLACED;
	if (debug) {
		if (fInterlaced) {
			syslog(LOG_INFO, "GIFLoad::ReadGIFImageHeader() - "
				"Image is interlaced\n");
		} else {
			syslog(LOG_INFO, "GIFLoad::ReadGIFImageHeader() - "
				"Image is not interlaced\n");
		}
	}

	return true;
}


bool
GIFLoad::ReadGIFImageData()
{
	unsigned char newEntry[ENTRY_COUNT];
	unsigned char codeSize;
	if (fInput->Read(&codeSize, 1) < 1)
		return false;

	if (codeSize > fPalette->size_in_bits) {
		if (debug) {
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - "
				"Code_size should be %d, not %d, allowing it\n",
				fCodeSize, codeSize);
		}
		if (!InitFrame(codeSize))
			return false;
	} else if (codeSize < fPalette->size_in_bits) {
		if (debug) {
			syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - "
				"Code_size should be %d, not %d\n", fCodeSize, codeSize);
		}
		return false;
	} else if (!InitFrame(fPalette->size_in_bits))
		return false;

	if (debug)
		syslog(LOG_INFO, "GIFLoad::ReadGIFImageData() - Starting LZW\n");

	while ((fNewCode = NextCode()) != -1 && fNewCode != fEndCode) {
		if (fNewCode == fClearCode) {
			ResetTable();
			fNewCode = NextCode();
			fOldCode[0] = fNewCode;
			fOldCodeLength = 1;
			if (!OutputColor(fOldCode, 1))
				goto bad_end;

			if (fNewCode == -1 || fNewCode == fEndCode) {
				if (debug) {
					syslog(LOG_ERR, "GIFLoad::ReadGIFImageData() - "
						"Premature fEndCode or error reading fNewCode\n");
				}
				goto bad_end;
			}
			continue;
		}

		// explicitly check for lack of clear code at start of file
		if (fOldCodeLength == 0) {
			fOldCode[0] = fNewCode;
			fOldCodeLength = 1;
			if (!OutputColor(fOldCode, 1))
				goto bad_end;

			continue;
		}

		// error out if we're trying to access an out-of-bounds index
		if (fNextCode >= ENTRY_COUNT)
			goto bad_end;

		if (fTable[fNewCode] != NULL) {
			// exists in table

			if (!OutputColor(fTable[fNewCode], fEntrySize[fNewCode]))
				goto bad_end;

			//memcpy(newEntry, fOldCode, fOldCodeLength);
			for (unsigned int x = 0; x < fOldCodeLength; x++)
				newEntry[x] = fOldCode[x];

			//memcpy(newEntry + fOldCodeLength, fTable[fNewCode], 1);
			newEntry[fOldCodeLength] = fTable[fNewCode][0];
		} else {
			// does not exist in table

			//memcpy(newEntry, fOldCode, fOldCodeLength);
			for (unsigned int x = 0; x < fOldCodeLength; x++)
				newEntry[x] = fOldCode[x];

			//memcpy(newEntry + fOldCodeLength, fOldCode, 1);
			newEntry[fOldCodeLength] = fOldCode[0];

			if (!OutputColor(newEntry, fOldCodeLength + 1))
				goto bad_end;
		}
		fTable[fNextCode] = MemblockAllocate(fOldCodeLength + 1);
		if (fTable[fNextCode] == NULL)
			goto bad_end;

		//memcpy(fTable[fNextCode], newEntry, fOldCodeLength + 1);
		for (unsigned int x = 0; x < fOldCodeLength + 1; x++)
			fTable[fNextCode][x] = newEntry[x];

		fEntrySize[fNextCode] = fOldCodeLength + 1;

		//memcpy(fOldCode, fTable[fNewCode], fEntrySize[fNewCode]);
		for (int x = 0; x < fEntrySize[fNewCode]; x++)
			fOldCode[x] = fTable[fNewCode][x];

		fOldCodeLength = fEntrySize[fNewCode];
		fNextCode++;

		if (fNextCode > fMaxCode && fBits < LZ_MAX_BITS) {
			fBits++;
			fMaxCode = (1 << fBits) - 1;
		}
	}

	MemblockDeleteAll();
	if (fNewCode == -1)
		return false;

	if (debug)
		syslog(LOG_INFO, "GIFLoad::ReadGIFImageData() - Done\n");

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
			if (fInput->Read(&fByteCount, 1) < 1)
				return -1;

			if (fByteCount == 0)
				return fEndCode;

			if (fInput->Read(fByteBuffer + (255 - fByteCount), fByteCount)
					< fByteCount) {
				return -1;
			}
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
	for (int x = 0; x < ENTRY_COUNT; x++) {
		fTable[x] = NULL;
		if (x < (1 << fCodeSize)) {
			fTable[x] = MemblockAllocate(1);
			if (fTable[x] != NULL) {
				fTable[x][0] = x;
				fEntrySize[x] = 1;
			}
		}
	}
}


bool
GIFLoad::InitFrame(int codeSize)
{
	fCodeSize = codeSize;
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


unsigned char*
GIFLoad::MemblockAllocate(int size)
{
	// Do 4k mallocs, keep them in a linked list, do a first fit across
	// them when a new request comes along.

	if (fHeadMemblock == NULL) {
		fHeadMemblock = (Memblock*)malloc(sizeof(Memblock));
		if (fHeadMemblock == NULL)
			return NULL;

		unsigned char* value = fHeadMemblock->data;
		fHeadMemblock->offset = size;
		fHeadMemblock->next = NULL;

		return value;
	} else {
		Memblock* block = fHeadMemblock;
		Memblock* last = NULL;
		while (block != NULL) {
			if (ENTRY_COUNT - block->offset > size) {
				unsigned char* value = block->data + block->offset;
				block->offset += size;

				return value;
			}
			last = block;
			block = block->next;
		}

		block = (Memblock*)malloc(sizeof(Memblock));
		if (block == NULL)
			return NULL;

		unsigned char* value = block->data;
		block->offset = size;
		block->next = NULL;
		if (last != NULL)
			last->next = block;

		return value;
	}
}


void
GIFLoad::MemblockDeleteAll()
{
	Memblock* block = NULL;

	while (fHeadMemblock != NULL) {
		// delete the linked list
		block = fHeadMemblock->next;
		free(fHeadMemblock);
		fHeadMemblock = block;
	}
}


bool
GIFLoad::OutputColor(unsigned char* string, int size)
{
	int bpr = fWidth << 2;

	for (int x = 0; x < size; x++) {
		fScanLine[fScanlinePosition] = fPalette->ColorForIndex(string[x]);
		fScanlinePosition++;

		if (fScanlinePosition >= fWidth) {
			if (fOutput->WriteAt(32 + (fRow * bpr), fScanLine, bpr) < bpr)
				return false;

			fScanlinePosition = 0;
			if (fInterlaced) {
				fRow += gl_increment_pass_by[fPass];
				while (fRow >= fHeight) {
					fPass++;
					if (fPass > 3)
						return true;

					fRow = gl_pass_starts_at[fPass];
				}
			} else
				fRow++;
		}
	}

	return true;
}
