/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_LOCATION_H
#define VALUE_LOCATION_H

#include <vector>

#include <stdlib.h>
#include <string.h>

#include <Referenceable.h>

#include "Types.h"


enum value_piece_location_type {
	VALUE_PIECE_LOCATION_INVALID,	// structure is invalid
	VALUE_PIECE_LOCATION_UNKNOWN,	// location unknown, but size is valid
	VALUE_PIECE_LOCATION_MEMORY,	// piece is in memory
	VALUE_PIECE_LOCATION_REGISTER,	// piece is in a register
	VALUE_PIECE_LOCATION_IMPLICIT	// value isn't stored anywhere in memory but is known
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
	void*						value;		// used for storing implicit values
	bool						writable;	// indicates if the piece is in a
											// location in the target team
											// where it can be modified


	ValuePieceLocation()
		:
		type(VALUE_PIECE_LOCATION_INVALID),
		value(NULL),
		writable(false)
	{
	}

	ValuePieceLocation(const ValuePieceLocation& other)
	{
		if (!Copy(other))
			throw std::bad_alloc();
	}

	~ValuePieceLocation()
	{
		if (value != NULL)
			free(value);
	}

	ValuePieceLocation& operator=(const ValuePieceLocation& other)
	{
		if (!Copy(other))
			throw std::bad_alloc();

		return *this;
	}

	bool Copy(const ValuePieceLocation& other)
	{
		memcpy((void*)this, (void*)&other, sizeof(ValuePieceLocation));
		if (type == VALUE_PIECE_LOCATION_IMPLICIT) {
			void* tempValue = malloc(size);
			if (tempValue == NULL) {
				type = VALUE_PIECE_LOCATION_INVALID;
				return false;
			}

			memcpy(tempValue, value, other.size);
			value = tempValue;
		}

		return true;
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
		this->writable = true;
	}

	void SetToRegister(uint32 reg)
	{
		type = VALUE_PIECE_LOCATION_REGISTER;
		this->reg = reg;
		this->writable = true;
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

	bool SetToValue(const void* data, target_size_t size)
	{
		char* valueData = (char*)malloc(size);
		if (valueData == NULL)
			return false;
		memcpy(valueData, data, size);
		SetSize(size);
		type = VALUE_PIECE_LOCATION_IMPLICIT;
		value = valueData;
		writable = false;
		return true;
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

			bool				SetToByteOffset(const ValueLocation& other,
									uint64 byteffset, uint64 Size);

			bool				SetTo(const ValueLocation& other,
									uint64 bitOffset, uint64 bitSize);

			void				Clear();

			bool				IsBigEndian() const	{ return fBigEndian; }
			bool				IsWritable() const { return fWritable; }

			bool				AddPiece(const ValuePieceLocation& piece);

			int32				CountPieces() const;
			ValuePieceLocation	PieceAt(int32 index) const;
			bool				SetPieceAt(int32 index,
									const ValuePieceLocation& piece);
			ValueLocation&		operator=(const ValueLocation& other);

			void				Dump() const;

private:
	typedef std::vector<ValuePieceLocation> PieceVector;

private:
			PieceVector			fPieces;
			bool				fBigEndian;
			bool				fWritable;
};


#endif	// VALUE_LOCATION_H
