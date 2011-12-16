/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_LOCATION_H
#define VALUE_LOCATION_H


#include <Array.h>
#include <Referenceable.h>

#include "Types.h"


enum value_piece_location_type {
	VALUE_PIECE_LOCATION_INVALID,	// structure is invalid
	VALUE_PIECE_LOCATION_UNKNOWN,	// location unknown, but size is valid
	VALUE_PIECE_LOCATION_MEMORY,	// piece is in memory
	VALUE_PIECE_LOCATION_REGISTER	// piece is in a register
};


struct ValuePieceLocation {
	union {
		target_addr_t			address;	// memory address
		uint32					reg;		// register number
	};
	target_size_t				size;		// size in bytes (including
											// incomplete ones)
	uint64						bitSize;	// total size in bits
	uint64						bitOffset;	// bit offset (to the most
											// significant bit)
	value_piece_location_type	type;

	ValuePieceLocation()
		:
		type(VALUE_PIECE_LOCATION_INVALID)
	{
	}

	bool IsValid() const
	{
		return type != VALUE_PIECE_LOCATION_INVALID;
	}

	void SetToUnknown()
	{
		type = VALUE_PIECE_LOCATION_UNKNOWN;
	}

	void SetToMemory(target_addr_t address)
	{
		type = VALUE_PIECE_LOCATION_MEMORY;
		this->address = address;
	}

	void SetToRegister(uint32 reg)
	{
		type = VALUE_PIECE_LOCATION_REGISTER;
		this->reg = reg;
	}

	void SetSize(target_size_t size)
	{
		this->size = size;
		this->bitSize = size * 8;
		this->bitOffset = 0;
	}

	void SetSize(uint64 bitSize, uint64 bitOffset)
	{
		this->size = (bitOffset + bitSize + 7) / 8;
		this->bitSize = bitSize;
		this->bitOffset = bitOffset;
	}

	ValuePieceLocation& Normalize(bool bigEndian);
};


class ValueLocation : public BReferenceable {
public:
								ValueLocation();
								ValueLocation(bool bigEndian);
								ValueLocation(bool bigEndian,
									const ValuePieceLocation& piece);
								ValueLocation(const ValueLocation& other);

			bool				SetTo(const ValueLocation& other,
									uint64 bitOffset, uint64 bitSize);

			void				Clear();

			bool				IsBigEndian() const	{ return fBigEndian; }

			bool				AddPiece(const ValuePieceLocation& piece);

			int32				CountPieces() const;
			ValuePieceLocation	PieceAt(int32 index) const;
			void				SetPieceAt(int32 index,
									const ValuePieceLocation& piece);

			ValueLocation&		operator=(const ValueLocation& other);

			void				Dump() const;

private:
	typedef Array<ValuePieceLocation> PieceArray;

private:
			PieceArray			fPieces;
			bool				fBigEndian;
};


#endif	// VALUE_LOCATION_H
