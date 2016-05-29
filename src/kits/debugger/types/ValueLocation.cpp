/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include "ValueLocation.h"


// #pragma mark - ValuePieceLocation


ValuePieceLocation&
ValuePieceLocation::Normalize(bool bigEndian)
{
	uint64 excessMSBs = bitOffset / 8;
	uint64 excessLSBs = size - (bitOffset + bitSize + 7) / 8;

	if (excessMSBs > 0 || excessLSBs > 0) {
		switch (type) {
			case VALUE_PIECE_LOCATION_MEMORY:
				if (bigEndian)
					address += excessMSBs;
				else
					address += excessLSBs;
				bitOffset -= excessMSBs * 8;
				size -= excessMSBs + excessLSBs;
				break;
			case VALUE_PIECE_LOCATION_UNKNOWN:
				bitOffset -= excessMSBs * 8;
				size -= excessMSBs + excessLSBs;
				break;
			case VALUE_PIECE_LOCATION_REGISTER:
			default:
				break;
		}
	}

	return *this;
}


// #pragma mark - ValueLocation


ValueLocation::ValueLocation()
	:
	fBigEndian(false),
	fWritable(false)
{
}


ValueLocation::ValueLocation(bool bigEndian)
	:
	fBigEndian(bigEndian),
	fWritable(false)
{
}


ValueLocation::ValueLocation(bool bigEndian, const ValuePieceLocation& piece)
	:
	fBigEndian(bigEndian)
{
	AddPiece(piece);
}


ValueLocation::ValueLocation(const ValueLocation& other)
	:
	fPieces(other.fPieces),
	fBigEndian(other.fBigEndian)
{
}


bool
ValueLocation::SetToByteOffset(const ValueLocation& other, uint64 byteOffset,
	uint64 byteSize)
{
	Clear();

	fBigEndian = other.fBigEndian;
	ValuePieceLocation piece = other.PieceAt(0);
	piece.SetToMemory(piece.address + byteOffset);
	piece.SetSize(byteSize);

	return AddPiece(piece);
}


bool
ValueLocation::SetTo(const ValueLocation& other, uint64 bitOffset,
	uint64 bitSize)
{
	Clear();

	fBigEndian = other.fBigEndian;

	// compute the total bit size
	int32 count = other.CountPieces();
	uint64 totalBitSize = 0;
	for (int32 i = 0; i < count; i++) {
		const ValuePieceLocation &piece = other.PieceAt(i);
		totalBitSize += piece.bitSize;
	}

	// adjust requested bit offset/size to something reasonable, if necessary
	if (bitOffset + bitSize > totalBitSize) {
		if (bitOffset >= totalBitSize)
			return true;
		bitSize = totalBitSize - bitOffset;
	}

	if (fBigEndian) {
		// Big endian: Skip the superfluous most significant bits, copy the
		// pieces we need (cutting the first and the last one as needed) and
		// ignore the remaining pieces.

		// skip pieces for the most significant bits we don't need anymore
		uint64 bitsToSkip = bitOffset;
		int32 i;
		ValuePieceLocation piece;
		for (i = 0; i < count; i++) {
			const ValuePieceLocation& tempPiece = other.PieceAt(i);
			if (tempPiece.bitSize > bitsToSkip) {
				if (!piece.Copy(tempPiece))
					return false;
				break;
			}
			bitsToSkip -= tempPiece.bitSize;
		}

		// handle partial piece
		if (bitsToSkip > 0) {
			piece.bitOffset += bitsToSkip;
			piece.bitSize -= bitsToSkip;
			piece.Normalize(fBigEndian);
		}

		// handle remaining pieces
		while (bitSize > 0) {
			if (piece.bitSize > bitSize) {
				// the piece is bigger than the remaining size -- cut it
				piece.bitSize = bitSize;
				piece.Normalize(fBigEndian);
				bitSize = 0;
			} else
				bitSize -= piece.bitSize;

			if (!AddPiece(piece))
				return false;

			if (++i >= count)
				break;

			if (!piece.Copy(other.PieceAt(i)))
				return false;
		}
	} else {
		// Little endian: Skip the superfluous least significant bits, copy the
		// pieces we need (cutting the first and the last one as needed) and
		// ignore the remaining pieces.

		// skip pieces for the least significant bits we don't need anymore
		uint64 bitsToSkip = totalBitSize - bitOffset - bitSize;
		int32 i;
		ValuePieceLocation piece;
		for (i = 0; i < count; i++) {
			const ValuePieceLocation& tempPiece = other.PieceAt(i);
			if (tempPiece.bitSize > bitsToSkip) {
				if (!piece.Copy(tempPiece))
					return false;
				break;
			}
			bitsToSkip -= piece.bitSize;
		}

		// handle partial piece
		if (bitsToSkip > 0) {
			piece.bitSize -= bitsToSkip;
			piece.Normalize(fBigEndian);
		}

		// handle remaining pieces
		while (bitSize > 0) {
			if (piece.bitSize > bitSize) {
				// the piece is bigger than the remaining size -- cut it
				piece.bitOffset += piece.bitSize - bitSize;
				piece.bitSize = bitSize;
				piece.Normalize(fBigEndian);
				bitSize = 0;
			} else
				bitSize -= piece.bitSize;

			if (!AddPiece(piece))
				return false;

			if (++i >= count)
				break;

			if (!piece.Copy(other.PieceAt(i)))
				return false;
		}
	}

	return true;
}


void
ValueLocation::Clear()
{
	fWritable = false;
	fPieces.clear();
}


bool
ValueLocation::AddPiece(const ValuePieceLocation& piece)
{
	// Just add, don't normalize. This allows for using the class with different
	// semantics (e.g. in the DWARF code).
	try {
		fPieces.push_back(piece);
	} catch (...) {
		return false;
	}

	if (fPieces.size() == 1)
		fWritable = piece.writable;
	else
		fWritable = fWritable && piece.writable;

	return true;
}


int32
ValueLocation::CountPieces() const
{
	return fPieces.size();
}


ValuePieceLocation
ValueLocation::PieceAt(int32 index) const
{
	if (index < 0 || index >= (int32)fPieces.size())
		return ValuePieceLocation();

	return fPieces[index];
}


bool
ValueLocation::SetPieceAt(int32 index, const ValuePieceLocation& piece)
{
	if (index < 0 || index >= (int32)fPieces.size())
		return false;

	return fPieces[index].Copy(piece);
}


ValueLocation&
ValueLocation::operator=(const ValueLocation& other)
{
	fPieces = other.fPieces;
	fBigEndian = other.fBigEndian;
	return *this;
}


void
ValueLocation::Dump() const
{
	int32 count = fPieces.size();
	printf("ValueLocation: %s endian, %" B_PRId32 " pieces:\n",
		fBigEndian ? "big" : "little", count);

	for (int32 i = 0; i < count; i++) {
		const ValuePieceLocation& piece = fPieces[i];
		switch (piece.type) {
			case VALUE_PIECE_LOCATION_INVALID:
				printf("  invalid\n");
				continue;
			case VALUE_PIECE_LOCATION_UNKNOWN:
				printf("  unknown");
				break;
			case VALUE_PIECE_LOCATION_MEMORY:
				printf("  address %#" B_PRIx64, piece.address);
				break;
			case VALUE_PIECE_LOCATION_REGISTER:
				printf("  register %" B_PRIu32, piece.reg);
				break;
			case VALUE_PIECE_LOCATION_IMPLICIT:
				printf("  implicit value: ");
				for (uint32 j = 0; j < piece.size; j++)
					printf("%x ", ((char *)piece.value)[j]);
				break;
		}

		printf(" size: %" B_PRIu64 " (%" B_PRIu64 " bits), offset: %" B_PRIu64
			" bits\n", piece.size, piece.bitSize, piece.bitOffset);
	}
}
