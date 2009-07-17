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


ValueLocation&
ValueLocation::operator=(const ValueLocation& other)
{
	fPieces = other.fPieces;
	return *this;
}
