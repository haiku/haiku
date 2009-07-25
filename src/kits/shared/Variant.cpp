/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <Variant.h>

#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>


template<typename NumberType>
inline NumberType
BVariant::_ToNumber() const
{
	switch (fType) {
		case B_BOOL_TYPE:
			return fBool ? 1 : 0;
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


BVariant::~BVariant()
{
	Unset();
}


status_t
BVariant::SetToTypedData(const void* data, type_code type)
{
	Unset();

	switch (type) {
		case B_BOOL_TYPE:
			fBool = *(bool*)data;
			break;
		case B_INT8_TYPE:
			fInt8 = *(int8*)data;
			break;
		case B_UINT8_TYPE:
			fUInt8 = *(uint8*)data;
			break;
		case B_INT16_TYPE:
			fInt16 = *(int16*)data;
			break;
		case B_UINT16_TYPE:
			fUInt16 = *(uint16*)data;
			break;
		case B_INT32_TYPE:
			fInt32 = *(int32*)data;
			break;
		case B_UINT32_TYPE:
			fUInt32 = *(uint32*)data;
			break;
		case B_INT64_TYPE:
			fInt64 = *(int64*)data;
			break;
		case B_UINT64_TYPE:
			fUInt64 = *(uint64*)data;
			break;
		case B_FLOAT_TYPE:
			fFloat = *(float*)data;
			break;
		case B_DOUBLE_TYPE:
			fDouble = *(double*)data;
			break;
		case B_POINTER_TYPE:
			fPointer = *(void**)data;
			break;
		case B_STRING_TYPE:
			return _SetTo((const char*)data, 0) ? B_OK : B_NO_MEMORY;
		default:
			return B_BAD_TYPE;
	}

	fType = type;
	return B_OK;
}


void
BVariant::Unset()
{
	if ((fFlags & B_VARIANT_OWNS_DATA) != 0) {
		switch (fType) {
			case B_STRING_TYPE:
				free(fString);
				break;
			default:
				break;
		}
	} else if ((fFlags & B_VARIANT_REFERENCEABLE_DATA) != 0) {
		if (fReferenceable != NULL)
			fReferenceable->ReleaseReference();
	}

	fType = 0;
	fFlags = 0;
}


size_t
BVariant::Size() const
{
	if (fType == B_STRING_TYPE)
		return fString != NULL ? strlen(fString) + 1 : 0;
	if ((fFlags & B_VARIANT_REFERENCEABLE_DATA) != 0)
		return sizeof(this->fReferenceable);
	return SizeOfType(fType);
}


const uint8*
BVariant::Bytes() const
{
	if (fType == B_STRING_TYPE)
		return (const uint8*)fString;
	return fBytes;
}


bool
BVariant::IsNumber() const
{
	switch (fType) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			return true;
		default:
			return false;
	}
}


bool
BVariant::IsInteger() const
{
	switch (fType) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
			return true;
		default:
			return false;
	}
}


bool
BVariant::IsFloat() const
{
	switch (fType) {
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			return true;
		default:
			return false;
	}
}



bool
BVariant::ToBool() const
{
	switch (fType) {
		case B_BOOL_TYPE:
			return fBool;
		case B_INT8_TYPE:
			return fInt8 != 0;
		case B_UINT8_TYPE:
			return fUInt8 != 0;
		case B_INT16_TYPE:
			return fInt16 != 0;
		case B_UINT16_TYPE:
			return fUInt16 != 0;
		case B_INT32_TYPE:
			return fInt32 != 0;
		case B_UINT32_TYPE:
			return fUInt32 != 0;
		case B_INT64_TYPE:
			return fInt64 != 0;
		case B_UINT64_TYPE:
			return fUInt64 != 0;
		case B_FLOAT_TYPE:
			return fFloat != 0;
		case B_DOUBLE_TYPE:
			return fDouble != 0;
		case B_POINTER_TYPE:
			return fPointer != NULL;
		case B_STRING_TYPE:
			return fString != NULL;
				// TODO: We should probably check for actual values like "true",
				// "false", "on", "off", etc.
		default:
			return false;
	}
}


int8
BVariant::ToInt8() const
{
	return _ToNumber<int8>();
}


uint8
BVariant::ToUInt8() const
{
	return _ToNumber<uint8>();
}


int16
BVariant::ToInt16() const
{
	return _ToNumber<int16>();
}


uint16
BVariant::ToUInt16() const
{
	return _ToNumber<uint16>();
}


int32
BVariant::ToInt32() const
{
	return _ToNumber<int32>();
}


uint32
BVariant::ToUInt32() const
{
	return _ToNumber<uint32>();
}


int64
BVariant::ToInt64() const
{
	return _ToNumber<int64>();
}


uint64
BVariant::ToUInt64() const
{
	return _ToNumber<uint64>();
}


float
BVariant::ToFloat() const
{
	return _ToNumber<float>();
}


double
BVariant::ToDouble() const
{
	return _ToNumber<double>();
}


void*
BVariant::ToPointer() const
{
	return fType == B_POINTER_TYPE ? fString : NULL;
}


const char*
BVariant::ToString() const
{
	return fType == B_STRING_TYPE ? fString : NULL;
}


void
BVariant::_SetTo(const BVariant& other)
{
	if ((other.fFlags & B_VARIANT_OWNS_DATA) != 0) {
		switch (other.fType) {
			case B_STRING_TYPE:
				fType = B_STRING_TYPE;
				fString = strdup(other.fString);
				fFlags = B_VARIANT_OWNS_DATA;
				return;
			default:
				break;
		}
	} else if ((other.fFlags & B_VARIANT_REFERENCEABLE_DATA) != 0) {
		if (other.fReferenceable != NULL)
			fReferenceable->AcquireReference();
	}

	memcpy(this, &other, sizeof(BVariant));
}


void
BVariant::SwapEndianess()
{
	if (!IsNumber() || fType == B_POINTER_TYPE)
		return;

	swap_data(fType, fBytes, Size(), B_SWAP_ALWAYS);
}


/*static*/ size_t
BVariant::SizeOfType(type_code type)
{
	switch (type) {
		case B_BOOL_TYPE:
			return 1;
		case B_INT8_TYPE:
			return 1;
		case B_UINT8_TYPE:
			return 1;
		case B_INT16_TYPE:
			return 2;
		case B_UINT16_TYPE:
			return 2;
		case B_INT32_TYPE:
			return 4;
		case B_UINT32_TYPE:
			return 4;
		case B_INT64_TYPE:
			return 8;
		case B_UINT64_TYPE:
			return 8;
		case B_FLOAT_TYPE:
			return sizeof(float);
		case B_DOUBLE_TYPE:
			return sizeof(double);
		case B_POINTER_TYPE:
			return sizeof(void*);
		default:
			return 0;
	}
}


void
BVariant::_SetTo(bool value)
{
	fType = B_BOOL_TYPE;
	fFlags = 0;
	fBool = value;
}


void
BVariant::_SetTo(int8 value)
{
	fType = B_INT8_TYPE;
	fFlags = 0;
	fInt8 = value;
}


void
BVariant::_SetTo(uint8 value)
{
	fType = B_UINT8_TYPE;
	fFlags = 0;
	fUInt8 = value;
}


void
BVariant::_SetTo(int16 value)
{
	fType = B_INT16_TYPE;
	fFlags = 0;
	fInt16 = value;
}


void
BVariant::_SetTo(uint16 value)
{
	fType = B_UINT16_TYPE;
	fFlags = 0;
	fUInt16 = value;
}


void
BVariant::_SetTo(int32 value)
{
	fType = B_INT32_TYPE;
	fFlags = 0;
	fInt32 = value;
}


void
BVariant::_SetTo(uint32 value)
{
	fType = B_UINT32_TYPE;
	fFlags = 0;
	fUInt32 = value;
}


void
BVariant::_SetTo(int64 value)
{
	fType = B_INT64_TYPE;
	fFlags = 0;
	fInt64 = value;
}


void
BVariant::_SetTo(uint64 value)
{
	fType = B_UINT64_TYPE;
	fFlags = 0;
	fUInt64 = value;
}


void
BVariant::_SetTo(float value)
{
	fType = B_FLOAT_TYPE;
	fFlags = 0;
	fFloat = value;
}


void
BVariant::_SetTo(double value)
{
	fType = B_DOUBLE_TYPE;
	fFlags = 0;
	fDouble = value;
}


void
BVariant::_SetTo(const void* value)
{
	fType = B_POINTER_TYPE;
	fFlags = 0;
	fPointer = (void*)value;
}


bool
BVariant::_SetTo(const char* value, uint32 flags)
{
	fType = B_STRING_TYPE;
	fFlags = 0;

	if (value != NULL) {
		if ((flags & B_VARIANT_DONT_COPY_DATA) == 0) {
			fString = strdup(value);
			fFlags |= B_VARIANT_OWNS_DATA;
			if (fString == NULL)
				return false;
		} else {
			fString = (char*)value;
			fFlags |= flags & B_VARIANT_OWNS_DATA;
		}
	} else
		fString = NULL;

	return true;
}


void
BVariant::_SetTo(BReferenceable* value, type_code type)
{
	fType = type;
	fFlags = B_VARIANT_REFERENCEABLE_DATA;
	fReferenceable = value;

	if (fReferenceable != NULL)
		fReferenceable->AcquireReference();
}
