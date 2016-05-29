/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SyntheticPrimitiveType.h"

#include <Variant.h>

#include "UiUtils.h"


SyntheticPrimitiveType::SyntheticPrimitiveType(uint32 typeConstant)
	:
	PrimitiveType(),
	fTypeConstant(typeConstant),
	fID(),
	fName()
{
	_Init();
}


SyntheticPrimitiveType::~SyntheticPrimitiveType()
{
}


uint32
SyntheticPrimitiveType::TypeConstant() const
{
	return fTypeConstant;
}


image_id
SyntheticPrimitiveType::ImageID() const
{
	return -1;
}


const BString&
SyntheticPrimitiveType::ID() const
{
	return fID;
}


const BString&
SyntheticPrimitiveType::Name() const
{
	return fName;
}


type_kind
SyntheticPrimitiveType::Kind() const
{
	return TYPE_PRIMITIVE;
}


target_size_t
SyntheticPrimitiveType::ByteSize() const
{
	return BVariant::SizeOfType(fTypeConstant);
}


status_t
SyntheticPrimitiveType::ResolveObjectDataLocation(
	const ValueLocation& objectLocation, ValueLocation*& _location)
{
	_location = NULL;
	return B_NOT_SUPPORTED;
}


status_t
SyntheticPrimitiveType::ResolveObjectDataLocation(target_addr_t objectAddress,
	ValueLocation*& _location)
{
	_location = NULL;
	return B_NOT_SUPPORTED;
}


void
SyntheticPrimitiveType::_Init()
{
	fID.SetToFormat("%p", this);
	fName.SetTo(UiUtils::TypeCodeToString(fTypeConstant));
}
