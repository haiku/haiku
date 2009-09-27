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


// #pragma mark - EnumerationValue


EnumerationValue::~EnumerationValue()
{
}


// #pragma mark - ArrayDimension


ArrayDimension::~ArrayDimension()
{
}


uint64
ArrayDimension::CountElements() const
{
	Type* type = GetType();

	if (type->Kind() == TYPE_ENUMERATION)
		return dynamic_cast<EnumerationType*>(type)->CountValues();

	if (type->Kind() == TYPE_SUBRANGE) {
		SubrangeType* subrangeType = dynamic_cast<SubrangeType*>(type);
		BVariant lower = subrangeType->LowerBound();
		BVariant upper = subrangeType->LowerBound();
		bool isSigned;
		if (!lower.IsInteger(&isSigned) || !upper.IsInteger())
			return 0;

		return isSigned
			? upper.ToInt64() - lower.ToInt64()
			: upper.ToUInt64() - lower.ToUInt64();
	}

	return 0;
}


// #pragma mark - Type


Type::~Type()
{
}


Type*
Type::ResolveRawType() const
{
	return const_cast<Type*>(this);
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


Type*
ModifiedType::ResolveRawType() const
{
	return BaseType();
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


Type*
TypedefType::ResolveRawType() const
{
	return BaseType();
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


// #pragma mark - EnumerationType


EnumerationType::~EnumerationType()
{
}


type_kind
EnumerationType::Kind() const
{
	return TYPE_ENUMERATION;
}


EnumerationValue*
EnumerationType::ValueFor(const BVariant& value) const
{
	// TODO: Optimize?
	for (int32 i = 0; EnumerationValue* enumValue = ValueAt(i); i++) {
		if (enumValue->Value() == value)
			return enumValue;
	}

	return NULL;
}


// #pragma mark - SubrangeType


SubrangeType::~SubrangeType()
{
}


type_kind
SubrangeType::Kind() const
{
	return TYPE_SUBRANGE;
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
