/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_LOCATION_H
#define VALUE_LOCATION_H


#include <Referenceable.h>

#include "Array.h"
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
	target_size_t				size;		// size in bytes (complete ones)
	uint8						bitSize;	// totalBitSize = size * 8 + bitSize
	uint8						bitOffset;	// offset in bits
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
		this->bitSize = 0;
		this->bitOffset = 0;
	}

	void SetSize(uint64 bitSize, uint8 bitOffset)
	{
		this->size = bitSize / 8;
		this->bitSize = bitSize % 8;
		this->bitOffset = bitOffset;
	}
};


class ValueLocation : public Referenceable {
public:
								ValueLocation();
								ValueLocation(const ValuePieceLocation& piece);
								ValueLocation(const ValueLocation& other);

			void				Clear();
			bool				AddPiece(const ValuePieceLocation& piece);

			int32				CountPieces() const;
			ValuePieceLocation	PieceAt(int32 index) const;

			ValueLocation&		operator=(const ValueLocation& other);

private:
	typedef Array<ValuePieceLocation> PieceArray;

private:
			PieceArray			fPieces;
};


#endif	// VALUE_LOCATION_H
