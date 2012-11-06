/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "CppLanguage.h"

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
	parsedName.RemoveAll(" ");

	int32 modifierIndex = -1;
	for (int32 i = parsedName.Length() - 1; i >= 0; i--) {
		if (parsedName[i] == '*' || parsedName[i] == '&')
			modifierIndex = i;
	}

	if (modifierIndex >= 0) {
		parsedName.CopyInto(baseTypeName, 0, modifierIndex);
		parsedName.Remove(0, modifierIndex);
	} else
		baseTypeName = parsedName;

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

	typeRef.Detach();

	return result;
}
