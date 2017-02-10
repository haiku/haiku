/* 
** DeltaRowCompression.h
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _DELTA_ROW_COMPRESSION_H
#define _DELTA_ROW_COMPRESSION_H


#include <Debug.h>


class AbstractDeltaRowCompressor {
public:
						AbstractDeltaRowCompressor(int rowSize,
							uchar initialSeed);
	virtual				~AbstractDeltaRowCompressor();
	
	// InitCheck returns B_OK on successful construction of this object or 
	// B_NO_MEMORY if the buffer for the seed row could not be allocated.
			status_t	InitCheck();
	
	// Clears the seed row to the initial seed specified in the constructor
			void		Reset();
	
	// Returns the size of the delta row. 
	// The size is 0 if the row is equal to the seed row (previous row).
	// The seed row is updated only if updateSeedRow is true.
			int			CalculateSize(const uchar* row,
							bool updateSeedRow = false);
	
	// Compresses the row using the delta row compression algorithm.
	// The seed row is updated.
			void		Compress(const uchar* row);

protected:
	// append byte to delta row
	virtual	void		AppendByteToDeltaRow(uchar byte) = 0;
	
	// returns the current size of the delta row
	inline	int			CurrentDeltaRowSize()
						{
							return fDeltaRowIndex;
						}

private:
	// Returns the index where seed row and row differ 
	// or -1 if both arrays are equal.
	inline	int			DiffersIndex(const uchar* row, int index)
						{
							while (index < fSize) {
								if (fSeedRow[index] != row[index])
									return index;
								index ++;
							}
							return -1;
						}

	// Returns the number of bytes that row differs from seed row 
	// starting at the specified index.
	inline	int			DiffersLength(const uchar* row, int index)
						{
							int startIndex = index;
							while (index < fSize) {
								if (fSeedRow[index] == row[index])
									break;
								index ++;
							}
							return index - startIndex;
						}

	// Compresses row with delta row compression algorithm.
	// The seed row is updated only if updateSeedRow is true.
	// If updateDeltaRow is true the method AppendByteToDeltaRow is called.
			int 		CompressRaw(const uchar* row, bool updateSeedRow,
							bool updateDeltaRow);

	// write byte to delta row and calculate size of delta row
			void		Put(uchar byte)
						{
							if (fUpdateDeltaRow)
								AppendByteToDeltaRow(byte);
							fDeltaRowIndex ++;
						}

			uchar*		fSeedRow; // the seed row
			int			fSize; // the size of the seed row in bytes
			uchar		fInitialSeed;
							// the value to initialize the seed row with

			int			fDeltaRowIndex;
							// the index of the next byte to be written into
							// the delta row
			bool		fUpdateDeltaRow; // write delta row
};


class DeltaRowCompressor : public AbstractDeltaRowCompressor
{
public:
						DeltaRowCompressor(int rowSize, uchar initialSeed)
						:
						AbstractDeltaRowCompressor(rowSize, initialSeed)
						{}
	
	// The delta row to be written to.
			void		SetDeltaRow(uchar* deltaRow)
						{
							fDeltaRow = deltaRow;
						}
	
protected:
	virtual	void		AppendByteToDeltaRow(uchar byte)
						{
							fDeltaRow[CurrentDeltaRowSize()] = byte;
						}	

private:
			uchar*		fDeltaRow; // the delta row
};

#endif
