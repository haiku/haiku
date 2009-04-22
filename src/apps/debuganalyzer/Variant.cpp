/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Variant.h"

#include <stdlib.h>
#include <string.h>


template<typename NumberType>
inline NumberType
Variant::_ToNumber() const
{
	switch (fType) {
		case B_INT8_TYPE:
			return (NumberType)fInt8;
		case B_UINT8_TYPE:
			return (NumberType)fUInt8;
		case B_INT16_TYPE:
			return (NumberType)fInt16;
		case B_UINT16_TYPE:
			return (NumberType)fUInt16;
		case B_INT32_TYPE:
			return (NumberType)fInt32;
		case B_UINT32_TYPE:
			return (NumberType)fUInt32;
		case B_INT64_TYPE:
			return (NumberType)fInt64;
		case B_UINT64_TYPE:
			return (NumberType)fUInt64;
		case B_FLOAT_TYPE:
			return (NumberType)fFloat;
		case B_DOUBLE_TYPE:
			return (NumberType)fDouble;
		default:
			return 0;
	}
}


Variant::~Variant()
{
	Unset();
}


void
Variant::Unset()
{
	if ((fFlags & VARIANT_OWNS_DATA) != 0) {
		switch (fType) {
			case B_STRING_TYPE:
				free(fString);
				break;
			default:
				break;
		}
	}

	fType = 0;
	fFlags = 0;
}


int8
Variant::ToInt8() const
{
	return _ToNumber<int8>();
}


uint8
Variant::ToUInt8() const
{
	return _ToNumber<uint8>();
}


int16
Variant::ToInt16() const
{
	return _ToNumber<int16>();
}


uint16
Variant::ToUInt16() const
{
	return _ToNumber<uint16>();
}


int32
Variant::ToInt32() const
{
	return _ToNumber<int32>();
}


uint32
Variant::ToUInt32() const
{
	return _ToNumber<uint32>();
}


int64
Variant::ToInt64() const
{
	return _ToNumber<int64>();
}


uint64
Variant::ToUInt64() const
{
	return _ToNumber<uint64>();
}


float
Variant::ToFloat() const
{
	return _ToNumber<float>();
}


double
Variant::ToDouble() const
{
	return _ToNumber<double>();
}


void*
Variant::ToPointer() const
{
	return fType == B_POINTER_TYPE ? fString : NULL;
}


const char*
Variant::ToString() const
{
	return fType == B_STRING_TYPE ? fString : NULL;
}


void
Variant::_SetTo(const Variant& other)
{
	if ((other.fFlags & VARIANT_OWNS_DATA) != 0) {
		switch (other.fType) {
			case B_STRING_TYPE:
				fType = B_STRING_TYPE;
				fString = strdup(other.fString);
				fFlags = VARIANT_OWNS_DATA;
				return;
			default:
				break;
		}
	}

	memcpy(this, &other, sizeof(Variant));
}


void
Variant::_SetTo(int8 value)
{
	fType = B_INT8_TYPE;
	fFlags = 0;
	fInt8 = value;
}


void
Variant::_SetTo(uint8 value)
{
	fType = B_UINT8_TYPE;
	fFlags = 0;
	fUInt8 = value;
}


void
Variant::_SetTo(int16 value)
{
	fType = B_INT16_TYPE;
	fFlags = 0;
	fInt16 = value;
}


void
Variant::_SetTo(uint16 value)
{
	fType = B_UINT16_TYPE;
	fFlags = 0;
	fUInt16 = value;
}


void
Variant::_SetTo(int32 value)
{
	fType = B_INT32_TYPE;
	fFlags = 0;
	fInt32 = value;
}


void
Variant::_SetTo(uint32 value)
{
	fType = B_UINT32_TYPE;
	fFlags = 0;
	fUInt32 = value;
}


void
Variant::_SetTo(int64 value)
{
	fType = B_INT64_TYPE;
	fFlags = 0;
	fInt64 = value;
}


void
Variant::_SetTo(uint64 value)
{
	fType = B_UINT64_TYPE;
	fFlags = 0;
	fUInt64 = value;
}


void
Variant::_SetTo(float value)
{
	fType = B_FLOAT_TYPE;
	fFlags = 0;
	fFloat = value;
}


void
Variant::_SetTo(double value)
{
	fType = B_DOUBLE_TYPE;
	fFlags = 0;
	fDouble = value;
}


void
Variant::_SetTo(const void* value)
{
	fType = B_POINTER_TYPE;
	fFlags = 0;
	fPointer = (void*)value;
}


void
Variant::_SetTo(const char* value, uint32 flags)
{
	fType = B_STRING_TYPE;

	if (value != NULL) {
		if ((flags & VARIANT_DONT_COPY_DATA) == 0) {
			fString = strdup(value);
			fFlags |= VARIANT_OWNS_DATA;
		} else {
			fString = (char*)value;
			fFlags |= flags & VARIANT_OWNS_DATA;
		}
	} else
		fString = NULL;
}

