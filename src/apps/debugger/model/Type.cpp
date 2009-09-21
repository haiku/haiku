/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Type.h"


// #pragma mark - BaseType


BaseType::~BaseType()
{
}


// #pragma mark - DataMember


DataMember::~DataMember()
{
}


// #pragma mark - Type


Type::~Type()
{
}


// #pragma mark - PrimitiveType


PrimitiveType::~PrimitiveType()
{
}


type_kind
PrimitiveType::Kind() const
{
	return TYPE_PRIMITIVE;
}


// #pragma mark - CompoundType


CompoundType::~CompoundType()
{
}


type_kind
CompoundType::Kind() const
{
	return TYPE_COMPOUND;
}


// #pragma mark - ModifiedType


ModifiedType::~ModifiedType()
{
}


type_kind
ModifiedType::Kind() const
{
	return TYPE_MODIFIED;
}


// #pragma mark - TypedefType


TypedefType::~TypedefType()
{
}


type_kind
TypedefType::Kind() const
{
	return TYPE_TYPEDEF;
}


// #pragma mark - AddressType


AddressType::~AddressType()
{
}


type_kind
AddressType::Kind() const
{
	return TYPE_ADDRESS;
}


// #pragma mark - ArrayType


ArrayType::~ArrayType()
{
}


type_kind
ArrayType::Kind() const
{
	return TYPE_ARRAY;
}
