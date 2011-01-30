/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_ATTRIBUTE_VALUE_H_
#define _PACKAGE__HPKG__PACKAGE_ATTRIBUTE_VALUE_H_


#include <string.h>

#include <package/hpkg/HPKGDefs.h>


namespace BPackageKit {

namespace BHPKG {


struct BPackageAttributeValue {
			union {
				int64			signedInt;
				uint64			unsignedInt;
				const char*		string;
				struct {
					uint64		size;
					union {
						uint64	offset;
						uint8	raw[B_HPKG_MAX_INLINE_DATA_SIZE];
					};
				} data;
			};
			uint8				type;
			uint8				encoding;

public:
	inline						BPackageAttributeValue();

	inline	void				SetTo(int8 value);
	inline	void				SetTo(uint8 value);
	inline	void				SetTo(int16 value);
	inline	void				SetTo(uint16 value);
	inline	void				SetTo(int32 value);
	inline	void				SetTo(uint32 value);
	inline	void				SetTo(int64 value);
	inline	void				SetTo(uint64 value);
	inline	void				SetTo(const char* value);
	inline	void				SetToData(uint64 size, uint64 offset);
	inline	void				SetToData(uint64 size, const void* rawData);
};


BPackageAttributeValue::BPackageAttributeValue()
	:
	type(B_HPKG_ATTRIBUTE_TYPE_INVALID)
{
}


void
BPackageAttributeValue::SetTo(int8 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
BPackageAttributeValue::SetTo(uint8 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
BPackageAttributeValue::SetTo(int16 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
BPackageAttributeValue::SetTo(uint16 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
BPackageAttributeValue::SetTo(int32 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
BPackageAttributeValue::SetTo(uint32 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
BPackageAttributeValue::SetTo(int64 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
BPackageAttributeValue::SetTo(uint64 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
BPackageAttributeValue::SetTo(const char* value)
{
	string = value;
	type = B_HPKG_ATTRIBUTE_TYPE_STRING;
}


void
BPackageAttributeValue::SetToData(uint64 size, uint64 offset)
{
	data.size = size;
	data.offset = offset;
	type = B_HPKG_ATTRIBUTE_TYPE_RAW;
	encoding = B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP;
}


void
BPackageAttributeValue::SetToData(uint64 size, const void* rawData)
{
	data.size = size;
	if (size > 0)
		memcpy(data.raw, rawData, size);
	type = B_HPKG_ATTRIBUTE_TYPE_RAW;
	encoding = B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE;
}


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_ATTRIBUTE_VALUE_H_
