/* 
** DeltaRowCompression.cpp
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "DeltaRowCompression.h"

#include <memory.h>

#include <SupportDefs.h>


AbstractDeltaRowCompressor::AbstractDeltaRowCompressor(int rowSize,
	uchar initialSeed)
	:
	fSeedRow(new uchar[rowSize]),
	fSize(rowSize),
	fInitialSeed(initialSeed)
{
	Reset();
}


AbstractDeltaRowCompressor::~AbstractDeltaRowCompressor()
{
	delete[] fSeedRow;
	fSeedRow = NULL;
}


status_t
AbstractDeltaRowCompressor::InitCheck()
{
	if (fSeedRow != NULL)
		return B_OK;
	else
		return B_NO_MEMORY;
}


void
AbstractDeltaRowCompressor::Reset()
{
	if (fSeedRow != NULL)
		memset(fSeedRow, fInitialSeed, fSize);
}


int
AbstractDeltaRowCompressor::CompressRaw(const uchar* row, bool updateSeedRow,
	bool updateDeltaRow)
{
	int index = DiffersIndex(row, 0);
	if (index == -1) {
		// no differences
		return 0;
	}

	fUpdateDeltaRow = updateDeltaRow;
	fDeltaRowIndex = 0;
	
	int seedRowIndex = 0;
	do {
		int length = DiffersLength(row, index);
		
		// delta starts at index and contains length bytes
		do {
			// control byte limits data bytes to 8 bytes
			int deltaBytes = length;
			if (length > 8)
				deltaBytes = 8;

			// calculate offset
			int totalOffset = index - seedRowIndex;
			bool needsOffsetBytes = totalOffset > 30;
			int offset = totalOffset;
			// control byte limits offset value to 31
			if (needsOffsetBytes)
				offset = 31;
			
			// write control byte (delta bytes bits 5-7; offset bits 0-4)
			Put(((deltaBytes - 1) << 5) | offset);
			
			if (needsOffsetBytes) {
				// write additional offset bytes after control byte	
				// the last offset byte must be less than 255
				totalOffset -= offset;
				while (totalOffset >= 255) {
					Put(255);
					totalOffset -= 255;
				}
				
				Put(totalOffset);
			}
			
			// write data bytes
			for (int i = 0; i < deltaBytes; i ++) {
				// copy row to seed row and delta row
				uchar byte = row[index];
				if (updateSeedRow) {
					ASSERT (index < fSize);
					fSeedRow[index] = byte;
				}
				Put(byte);
				index ++;
			}
			
			seedRowIndex = index;
			
			length -= deltaBytes;
			
		} while (length > 0);
		
		index = DiffersIndex(row, index);
		
	} while (index != -1);
	
	return fDeltaRowIndex;
}


int
AbstractDeltaRowCompressor::CalculateSize(const uchar* row, bool updateSeedRow)
{
	return CompressRaw(row, updateSeedRow, false);
}


void
AbstractDeltaRowCompressor::Compress(const uchar* row)
{
	CompressRaw(row, true, true);
}


#ifdef TEST_DELTA_ROW_COMPRESSION

void
test(AbstractDeltaRowCompressor* compressor, uchar* row) {
	int size = compressor->CalculateSize(row);
	printf("size %d\n", size);

	if (size > 0) {
		uchar* buffer = new uchar[size];
		compressor->Compress(row, buffer, size);
		for (int i = 0; i < size; i ++) {
			printf("%2.2x ", (int)buffer[i]);
		}		
		printf("\n");
		delete buffer;
	}	
	printf("\n");
}


int
main(int argc, char* argv[])
{
	int n = 5;
	uchar row1[] = {0, 0, 0, 0, 0};
	uchar row2[] = {0, 1, 0, 0, 0};
	uchar row3[] = {1, 1, 0, 2, 2};

	DeltaRowCompressor compressor(n, 0);
	test(&compressor, row1);
	test(&compressor, row2);
	test(&compressor, row3);
}

#endif // TEST_DELTA_ROW_COMPRESSION
