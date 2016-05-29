/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TypeLookupConstraints.h"


TypeLookupConstraints::TypeLookupConstraints()
	:
	fTypeKindGiven(false),
	fSubtypeKindGiven(false)
{
}


TypeLookupConstraints::TypeLookupConstraints(type_kind typeKind)
	:
	fTypeKind(typeKind),
	fTypeKindGiven(true),
	fSubtypeKindGiven(false)
{
}


TypeLookupConstraints::TypeLookupConstraints(type_kind typeKind,
	int32 subTypeKind)
	:
	fTypeKind(typeKind),
	fSubtypeKind(subTypeKind),
	fTypeKindGiven(true),
	fSubtypeKindGiven(true),
	fBaseTypeName()
{
}


bool
TypeLookupConstraints::HasTypeKind() const
{
	return fTypeKindGiven;
}


bool
TypeLookupConstraints::HasSubtypeKind() const
{
	return fSubtypeKindGiven;
}


bool
TypeLookupConstraints::HasBaseTypeName() const
{
	return fBaseTypeName.Length() > 0;
}


type_kind
TypeLookupConstraints::TypeKind() const
{
	return fTypeKind;
}


int32
TypeLookupConstraints::SubtypeKind() const
{
	return fSubtypeKind;
}


const BString&
TypeLookupConstraints::BaseTypeName() const
{
	return fBaseTypeName;
}


void
TypeLookupConstraints::SetTypeKind(type_kind typeKind)
{
	fTypeKind = typeKind;
	fTypeKindGiven = true;
}


void
TypeLookupConstraints::SetSubtypeKind(int32 subtypeKind)
{
	fSubtypeKind = subtypeKind;
	fSubtypeKindGiven = true;
}


void
TypeLookupConstraints::SetBaseTypeName(const BString& name)
{
	fBaseTypeName = name;
}
