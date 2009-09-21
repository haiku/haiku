/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ValueLocation.h"


ValueLocation::ValueLocation()
{
}


ValueLocation::ValueLocation(const ValuePieceLocation& piece)
{
	AddPiece(piece);
}


ValueLocation::ValueLocation(const ValueLocation& other)
	:
	fPieces(other.fPieces)
{
}


bool
ValueLocation::SetTo(const ValueLocation& other, uint64 bitOffset,
	uint64 bitSize)
{
	Clear();

	// skip pieces before the offset
	int32 count = other.CountPieces();
	int32 i;
	ValuePieceLocation piece;
	for (i = 0; i < count; i++) {
		piece = other.PieceAt(i);
		if (piece.size * 8 + piece.bitSize > bitOffset)
			break;
		bitOffset -= piece.size * 8 + piece.bitSize;
	}

	if (i >= count)
		return true;

	// handle partial piece
	if (bitOffset > 0) {
		uint64 remainingBits = piece.size * 8 + piece.bitSize - bitOffset;
		piece.size = remainingBits / 8;
		piece.bitSize = remainingBits % 8;

		switch (piece.type) {
			case VALUE_PIECE_LOCATION_MEMORY:
				piece.address += (bitOffset + piece.bitOffset) / 8;
				piece.bitOffset = (bitOffset + piece.bitOffset) % 8;
				break;
			case VALUE_PIECE_LOCATION_UNKNOWN:
				piece.bitOffset = 0;
				break;
			case VALUE_PIECE_LOCATION_REGISTER:
				piece.bitOffset += bitOffset;
				break;
			default:
				break;
		}
	}

	// handle remaining pieces
	while (bitSize > 0) {
		target_addr_t pieceSize = piece.size * 8 + piece.bitSize;
		if (pieceSize > bitSize) {
			// the piece is bigger than the remaining size -- cut it
			piece.size = bitSize / 8;
			piece.bitSize = bitSize % 8;
			bitSize = 0;
		} else
			bitSize -= pieceSize;

		if (!AddPiece(piece))
			return false;

		if (++i >= count)
			break;

		piece = other.PieceAt(i);
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
	return *this;
}


void
ValueLocation::Dump() const
{
	int32 count = fPieces.Size();
	printf("ValueLocation: %ld pieces:\n", count);

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
				printf("  address %#llx", piece.address);
				break;
			case VALUE_PIECE_LOCATION_REGISTER:
				printf("  register %lu", piece.reg);
				break;
		}

		printf(" size: %llu+%u, offset: %u\n", piece.size, piece.bitSize,
			piece.bitOffset);
	}
}
