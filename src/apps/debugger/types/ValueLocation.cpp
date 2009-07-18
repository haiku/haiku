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
