/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


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
	fBigEndian(false)
{
}


ValueLocation::ValueLocation(bool bigEndian)
	:
	fBigEndian(bigEndian)
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
ValueLocation::SetTo(const ValueLocation& other, uint64 bitOffset,
	uint64 bitSize)
{
	Clear();

	fBigEndian = other.fBigEndian;

	// compute the total bit size
	int32 count = other.CountPieces();
	uint64 totalBitSize = 0;
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = other.PieceAt(i);
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
			piece = other.PieceAt(i);
			if (piece.bitSize > bitsToSkip)
				break;
			bitsToSkip -= piece.bitSize;
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

			piece = other.PieceAt(i);
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
			piece = other.PieceAt(i);
			if (piece.bitSize > bitsToSkip)
				break;
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

			piece = other.PieceAt(i);
		}
	}

	return true;
}


void
ValueLocation::Clear()
{
	fPieces.Clear();
}


bool
ValueLocation::AddPiece(const ValuePieceLocation& piece)
{
	// Just add, don't normalize. This allows for using the class with different
	// semantics (e.g. in the DWARF code).
	return fPieces.Add(piece);
}


int32
ValueLocation::CountPieces() const
{
	return fPieces.Size();
}


ValuePieceLocation
ValueLocation::PieceAt(int32 index) const
{
	if (index < 0 || index >= fPieces.Size())
		return ValuePieceLocation();

	return fPieces.ElementAt(index);
}


void
ValueLocation::SetPieceAt(int32 index, const ValuePieceLocation& piece)
{
	if (index < 0 || index >= fPieces.Size())
		return;

	fPieces.ElementAt(index) = piece;
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
	int32 count = fPieces.Size();
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
		}

		printf(" size: %" B_PRIu64 " (%" B_PRIu64 " bits), offset: %" B_PRIu64
			" bits\n", piece.size, piece.bitSize, piece.bitOffset);
	}
}
