/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CppLanguage.h"

#include <stdlib.h>

#include "TeamTypeInformation.h"
#include "Type.h"
#include "TypeLookupConstraints.h"


CppLanguage::CppLanguage()
{
}


CppLanguage::~CppLanguage()
{
}


const char*
CppLanguage::Name() const
{
	return "C++";
}


status_t
CppLanguage::ParseTypeExpression(const BString &expression,
									TeamTypeInformation* info,
									Type*& _resultType) const
{
	status_t result = B_OK;
	Type* baseType = NULL;

	BString parsedName = expression;
	BString baseTypeName;
	BString arraySpecifier;
	parsedName.RemoveAll(" ");

	int32 modifierIndex = -1;
	modifierIndex = parsedName.FindFirst('*');
	if (modifierIndex == -1)
		modifierIndex = parsedName.FindFirst('&');
	if (modifierIndex == -1)
		modifierIndex = parsedName.FindFirst('[');

	if (modifierIndex >= 0)
		parsedName.MoveInto(baseTypeName, 0, modifierIndex);
	else
		baseTypeName = parsedName;

	modifierIndex = parsedName.FindFirst('[');
	if (modifierIndex >= 0) {
		parsedName.MoveInto(arraySpecifier, modifierIndex,
			parsedName.Length() - modifierIndex);
	}

	result = info->LookupTypeByName(baseTypeName, TypeLookupConstraints(),
		baseType);
	if (result != B_OK)
		return result;

	BReference<Type> typeRef;
	typeRef.SetTo(baseType, true);

	if (!parsedName.IsEmpty()) {
		AddressType* derivedType = NULL;
		// walk the list of modifiers trying to add each.
		for (int32 i = 0; i < parsedName.Length(); i++) {
			address_type_kind typeKind;
			switch (parsedName[i]) {
				case '*':
				{
					typeKind = DERIVED_TYPE_POINTER;
					break;
				}
				case '&':
				{
					typeKind = DERIVED_TYPE_REFERENCE;
					break;
				}
				default:
				{
					return B_BAD_VALUE;
				}

			}

			if (derivedType == NULL) {
				result = baseType->CreateDerivedAddressType(typeKind,
					derivedType);
			} else {
				result = derivedType->CreateDerivedAddressType(typeKind,
					derivedType);
			}

			if (result != B_OK)
				return result;
			typeRef.SetTo(derivedType, true);
		}

		_resultType = derivedType;
	} else
		_resultType = baseType;


	if (!arraySpecifier.IsEmpty()) {
		ArrayType* arrayType = NULL;

		int32 startIndex = 1;
		do {
			int32 size = strtoul(arraySpecifier.String() + startIndex,
				NULL, 10);
			if (size < 0)
				return B_ERROR;

			if (arrayType == NULL) {
				result = _resultType->CreateDerivedArrayType(0, size, true,
					arrayType);
			} else {
				result = arrayType->CreateDerivedArrayType(0, size, true,
					arrayType);
			}

			if (result != B_OK)
				return result;

			typeRef.SetTo(arrayType, true);

			startIndex = arraySpecifier.FindFirst('[', startIndex + 1);

		} while (startIndex >= 0);

		// since a C/C++ array is essentially pointer math,
		// the resulting array has to be wrapped in a pointer to
		// ensure the element addresses wind up being against the
		// correct address.
		AddressType* addressType = NULL;
		result = arrayType->CreateDerivedAddressType(DERIVED_TYPE_POINTER,
			addressType);
		if (result != B_OK)
			return result;

		_resultType = addressType;
	}

	typeRef.Detach();

	return result;
}
