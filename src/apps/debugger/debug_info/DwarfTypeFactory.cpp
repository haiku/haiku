/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfTypeFactory.h"

#include <algorithm>
#include <new>

#include <AutoLocker.h>
#include <Variant.h>

#include "ArrayIndexPath.h"
#include "Architecture.h"
#include "CompilationUnit.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfTargetInterface.h"
#include "DwarfUtils.h"
#include "DwarfTypes.h"
#include "GlobalTypeLookup.h"
#include "Register.h"
#include "RegisterMap.h"
#include "SourceLanguageInfo.h"
#include "StringUtils.h"
#include "Tracing.h"
#include "TypeLookupConstraints.h"
#include "ValueLocation.h"


namespace {


// #pragma mark - HasTypePredicate


template<typename EntryType>
struct HasTypePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->GetType() != NULL;
	}
};


// #pragma mark - HasReturnTypePredicate


template<typename EntryType>
struct HasReturnTypePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->ReturnType() != NULL;
	}
};


// #pragma mark - HasEnumeratorsPredicate


struct HasEnumeratorsPredicate {
	inline bool operator()(DIEEnumerationType* entry) const
	{
		return !entry->Enumerators().IsEmpty();
	}
};


// #pragma mark - HasDimensionsPredicate


struct HasDimensionsPredicate {
	inline bool operator()(DIEArrayType* entry) const
	{
		return !entry->Dimensions().IsEmpty();
	}
};


// #pragma mark - HasMembersPredicate


struct HasMembersPredicate {
	inline bool operator()(DIECompoundType* entry) const
	{
		return !entry->DataMembers().IsEmpty();
	}
};


// #pragma mark - HasBaseTypesPredicate


struct HasBaseTypesPredicate {
	inline bool operator()(DIEClassBaseType* entry) const
	{
		return !entry->BaseTypes().IsEmpty();
	}
};


// #pragma mark - HasParametersPredicate


template<typename EntryType>
struct HasParametersPredicate {
	inline bool operator()(EntryType* entry) const
	{
		return !entry->Parameters().IsEmpty();
	}
};


// #pragma mark - HasLowerBoundPredicate


struct HasLowerBoundPredicate {
	inline bool operator()(DIESubrangeType* entry) const
	{
		return entry->LowerBound()->IsValid();
	}
};


// #pragma mark - HasUpperBoundPredicate


struct HasUpperBoundPredicate {
	inline bool operator()(DIESubrangeType* entry) const
	{
		return entry->UpperBound()->IsValid();
	}
};


// #pragma mark - HasCountPredicate


struct HasCountPredicate {
	inline bool operator()(DIESubrangeType* entry) const
	{
		return entry->Count()->IsValid();
	}
};


// #pragma mark - HasContainingTypePredicate


struct HasContainingTypePredicate {
	inline bool operator()(DIEPointerToMemberType* entry) const
	{
		return entry->ContainingType() != NULL;
	}
};


}	// unnamed namespace


// #pragma mark - ArtificialIntegerType


class DwarfTypeFactory::ArtificialIntegerType : public PrimitiveType {
public:
	ArtificialIntegerType(const BString& id, const BString& name,
		target_size_t byteSize, uint32 typeConstant)
		:
		fID(id),
		fName(name),
		fByteSize(byteSize),
		fTypeConstant(typeConstant)
	{
	}

	static status_t Create(target_size_t byteSize, bool isSigned, Type*& _type)
	{
		// get the matching type constant
		uint32 typeConstant;
		switch (byteSize) {
			case 1:
				typeConstant = isSigned ? B_INT8_TYPE : B_UINT8_TYPE;
				break;
			case 2:
				typeConstant = isSigned ? B_INT16_TYPE : B_UINT16_TYPE;
				break;
			case 4:
				typeConstant = isSigned ? B_INT32_TYPE : B_UINT32_TYPE;
				break;
			case 8:
				typeConstant = isSigned ? B_INT64_TYPE : B_UINT64_TYPE;
				break;
			default:
				return B_BAD_VALUE;
		}

		// name and ID
		char buffer[16];
		snprintf(buffer, sizeof(buffer), isSigned ? "int%d" : "uint%d",
			(int)byteSize * 8);
		BString id(buffer);
		if (id.Length() == 0)
			return B_NO_MEMORY;

		// create the type
		ArtificialIntegerType* type = new(std::nothrow) ArtificialIntegerType(
			id, id, byteSize, typeConstant);
		if (type == NULL)
			return B_NO_MEMORY;

		_type = type;
		return B_OK;
	}

	virtual image_id ImageID() const
	{
		return -1;
	}

	virtual const BString& ID() const
	{
		return fID;
	}

	virtual const BString& Name() const
	{
		return fName;
	}

	virtual target_size_t ByteSize() const
	{
		return fByteSize;
	}

	virtual status_t ResolveObjectDataLocation(
		const ValueLocation& objectLocation, ValueLocation*& _location)
	{
		// TODO: Implement!
		return B_UNSUPPORTED;
	}

	virtual status_t ResolveObjectDataLocation(target_addr_t objectAddress,
		ValueLocation*& _location)
	{
		// TODO: Implement!
		return B_UNSUPPORTED;
	}

	virtual uint32 TypeConstant() const
	{
		return fTypeConstant;
	}

private:
	BString	fID;
	BString	fName;
	uint32	fByteSize;
	uint32	fTypeConstant;
};


// #pragma mark - DwarfTypeFactory


DwarfTypeFactory::DwarfTypeFactory(DwarfTypeContext* typeContext,
	GlobalTypeLookup* typeLookup, GlobalTypeCache* typeCache)
	:
	fTypeContext(typeContext),
	fTypeLookup(typeLookup),
	fTypeCache(typeCache)
{
	fTypeContext->AcquireReference();
	fTypeCache->AcquireReference();
}


DwarfTypeFactory::~DwarfTypeFactory()
{
	fTypeContext->ReleaseReference();
	fTypeCache->ReleaseReference();
}


status_t
DwarfTypeFactory::CreateType(DIEType* typeEntry, DwarfType*& _type)
{
	// try the type cache first
	BString name;
	DwarfUtils::GetFullyQualifiedDIEName(typeEntry, name);
// TODO: The DIE may not have a name (e.g. pointer and reference types don't).

	TypeLookupConstraints constraints(
		dwarf_tag_to_type_kind(typeEntry->Tag()));
	int32 subtypeKind = dwarf_tag_to_subtype_kind(typeEntry->Tag());
	if (subtypeKind >= 0)
		constraints.SetSubtypeKind(subtypeKind);

	AutoLocker<GlobalTypeCache> cacheLocker(fTypeCache);
	Type* globalType = name.Length() > 0
		? fTypeCache->GetType(name, constraints) : NULL;
	if (globalType == NULL) {
		// lookup by name failed -- try lookup by ID
		BString id;
		if (DwarfType::GetTypeID(typeEntry, id))
			globalType = fTypeCache->GetTypeByID(id);
	}

	if (globalType != NULL) {
		DwarfType* globalDwarfType = dynamic_cast<DwarfType*>(globalType);
		if (globalDwarfType != NULL) {
			globalDwarfType->AcquireReference();
			_type = globalDwarfType;
			return B_OK;
		}
	}

	cacheLocker.Unlock();

	// If the type entry indicates a declaration only, we try to look the
	// type up globally first.
	if (typeEntry->IsDeclaration() && name.Length() > 0
		&& fTypeLookup->GetType(fTypeCache, name,
			constraints, globalType)
			== B_OK) {
		DwarfType* globalDwarfType
			= dynamic_cast<DwarfType*>(globalType);
		if (globalDwarfType != NULL) {
			_type = globalDwarfType;
			return B_OK;
		}

		globalType->ReleaseReference();
	}

	// No luck yet -- create the type.
	DwarfType* type;
	status_t error = _CreateTypeInternal(name, typeEntry, type);
	if (error != B_OK)
		return error;
	BReference<DwarfType> typeReference(type, true);

	// Insert the type into the cache. Re-check, as the type may already
	// have been inserted (e.g. in the compound type case).
	cacheLocker.Lock();
	if (name.Length() > 0
			? fTypeCache->GetType(name, constraints) == NULL
			: fTypeCache->GetTypeByID(type->ID()) == NULL) {
		error = fTypeCache->AddType(type);
		if (error != B_OK)
			return error;
	}
	cacheLocker.Unlock();

	// try to get the type's size
	uint64 size;
	if (_ResolveTypeByteSize(typeEntry, size) == B_OK)
		type->SetByteSize(size);

	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateTypeInternal(const BString& name,
	DIEType* typeEntry, DwarfType*& _type)
{
	switch (typeEntry->Tag()) {
		case DW_TAG_class_type:
		case DW_TAG_structure_type:
		case DW_TAG_union_type:
		case DW_TAG_interface_type:
			return _CreateCompoundType(name,
				dynamic_cast<DIECompoundType*>(typeEntry), _type);

		case DW_TAG_base_type:
			return _CreatePrimitiveType(name,
				dynamic_cast<DIEBaseType*>(typeEntry), _type);

		case DW_TAG_pointer_type:
			return _CreateAddressType(name,
				dynamic_cast<DIEAddressingType*>(typeEntry),
				DERIVED_TYPE_POINTER, _type);
		case DW_TAG_reference_type:
			return _CreateAddressType(name,
				dynamic_cast<DIEAddressingType*>(typeEntry),
				DERIVED_TYPE_REFERENCE, _type);

		case DW_TAG_const_type:
			return _CreateModifiedType(name,
				dynamic_cast<DIEModifiedType*>(typeEntry),
				TYPE_MODIFIER_CONST, _type);
		case DW_TAG_packed_type:
			return _CreateModifiedType(name,
				dynamic_cast<DIEModifiedType*>(typeEntry),
				TYPE_MODIFIER_PACKED, _type);
		case DW_TAG_volatile_type:
			return _CreateModifiedType(name,
				dynamic_cast<DIEModifiedType*>(typeEntry),
				TYPE_MODIFIER_VOLATILE, _type);
		case DW_TAG_restrict_type:
			return _CreateModifiedType(name,
				dynamic_cast<DIEModifiedType*>(typeEntry),
				TYPE_MODIFIER_RESTRICT, _type);
		case DW_TAG_shared_type:
			return _CreateModifiedType(name,
				dynamic_cast<DIEModifiedType*>(typeEntry),
				TYPE_MODIFIER_SHARED, _type);

		case DW_TAG_typedef:
			return _CreateTypedefType(name,
				dynamic_cast<DIETypedef*>(typeEntry), _type);

		case DW_TAG_array_type:
			return _CreateArrayType(name,
				dynamic_cast<DIEArrayType*>(typeEntry), _type);

		case DW_TAG_enumeration_type:
			return _CreateEnumerationType(name,
				dynamic_cast<DIEEnumerationType*>(typeEntry), _type);

		case DW_TAG_subrange_type:
			return _CreateSubrangeType(name,
				dynamic_cast<DIESubrangeType*>(typeEntry), _type);

		case DW_TAG_unspecified_type:
			return _CreateUnspecifiedType(name,
				dynamic_cast<DIEUnspecifiedType*>(typeEntry), _type);

		case DW_TAG_subroutine_type:
			return _CreateFunctionType(name,
				dynamic_cast<DIESubroutineType*>(typeEntry), _type);

		case DW_TAG_ptr_to_member_type:
			return _CreatePointerToMemberType(name,
				dynamic_cast<DIEPointerToMemberType*>(typeEntry), _type);

		case DW_TAG_string_type:
		case DW_TAG_file_type:
		case DW_TAG_set_type:
			// TODO: Implement (not relevant for C++)!
			return B_UNSUPPORTED;
	}

	return B_UNSUPPORTED;
}


status_t
DwarfTypeFactory::_CreateCompoundType(const BString& name,
	DIECompoundType* typeEntry, DwarfType*& _type)
{
	TRACE_LOCALS("DwarfTypeFactory::_CreateCompoundType(\"%s\", %p)\n",
		name.String(), typeEntry);

	// create the type
	DwarfCompoundType* type = new(std::nothrow) DwarfCompoundType(fTypeContext,
		name, typeEntry);
	if (type == NULL)
		return B_NO_MEMORY;
	BReference<DwarfCompoundType> typeReference(type, true);

	// Already add the type at this pointer to the cache, since otherwise
	// we could run into an infinite recursion when trying to create the types
	// for the data members.
// TODO: Since access to the type lookup context is multi-threaded, the
// incomplete type could become visible to other threads. Hence we keep the
// context locked, but that essentially kills multi-threading for this context.
	AutoLocker<GlobalTypeCache> cacheLocker(fTypeCache);
	status_t error = fTypeCache->AddType(type);
	if (error != B_OK)
{
printf("  -> failed to add type to cache\n");
		return error;
}
//	cacheLocker.Unlock();

	// find the abstract origin or specification that defines the data members
	DIECompoundType* memberOwnerEntry = DwarfUtils::GetDIEByPredicate(typeEntry,
		HasMembersPredicate());

	// create the data member objects
	if (memberOwnerEntry != NULL) {
		for (DebugInfoEntryList::ConstIterator it
					= memberOwnerEntry->DataMembers().GetIterator();
				DebugInfoEntry* _memberEntry = it.Next();) {
			DIEMember* memberEntry = dynamic_cast<DIEMember*>(_memberEntry);

			TRACE_LOCALS("  member %p\n", memberEntry);

			// get the type
			DwarfType* memberType;
			if (CreateType(memberEntry->GetType(), memberType) != B_OK)
				continue;
			BReference<DwarfType> memberTypeReference(memberType, true);

			// get the name
			BString memberName;
			DwarfUtils::GetDIEName(memberEntry, memberName);

			// create and add the member object
			DwarfDataMember* member = new(std::nothrow) DwarfDataMember(
				memberEntry, memberName, memberType);
			BReference<DwarfDataMember> memberReference(member, true);
			if (member == NULL || !type->AddDataMember(member)) {
				cacheLocker.Lock();
				fTypeCache->RemoveType(type);
				return B_NO_MEMORY;
			}
		}
	}

	// If the type is a class/struct/interface type, we also need to add its
	// base types.
	if (DIEClassBaseType* classTypeEntry
			= dynamic_cast<DIEClassBaseType*>(typeEntry)) {
		// find the abstract origin or specification that defines the base types
		classTypeEntry = DwarfUtils::GetDIEByPredicate(classTypeEntry,
			HasBaseTypesPredicate());

		// create the inheritance objects for the base types
		if (classTypeEntry != NULL) {
			for (DebugInfoEntryList::ConstIterator it
						= classTypeEntry->BaseTypes().GetIterator();
					DebugInfoEntry* _inheritanceEntry = it.Next();) {
				DIEInheritance* inheritanceEntry
					= dynamic_cast<DIEInheritance*>(_inheritanceEntry);

				// get the type
				DwarfType* baseType;
				if (CreateType(inheritanceEntry->GetType(), baseType) != B_OK)
					continue;
				BReference<DwarfType> baseTypeReference(baseType, true);

				// create and add the inheritance object
				DwarfInheritance* inheritance = new(std::nothrow)
					DwarfInheritance(inheritanceEntry, baseType);
				BReference<DwarfInheritance> inheritanceReference(inheritance,
					true);
				if (inheritance == NULL || !type->AddInheritance(inheritance)) {
					cacheLocker.Lock();
					fTypeCache->RemoveType(type);
					return B_NO_MEMORY;
				}
			}
		}
	}

	_type = typeReference.Detach();
	return B_OK;;
}


status_t
DwarfTypeFactory::_CreatePrimitiveType(const BString& name,
	DIEBaseType* typeEntry, DwarfType*& _type)
{
	const DynamicAttributeValue* byteSizeValue = typeEntry->ByteSize();
//	const DynamicAttributeValue* bitOffsetValue = typeEntry->BitOffset();
	const DynamicAttributeValue* bitSizeValue = typeEntry->BitSize();

	uint32 bitSize = 0;
	if (byteSizeValue->IsValid()) {
		BVariant value;
		status_t error = fTypeContext->File()->EvaluateDynamicValue(
			fTypeContext->GetCompilationUnit(), fTypeContext->SubprogramEntry(),
			byteSizeValue, fTypeContext->TargetInterface(),
			fTypeContext->InstructionPointer(), fTypeContext->FramePointer(),
			value);
		if (error == B_OK && value.IsInteger())
			bitSize = value.ToUInt32() * 8;
	} else if (bitSizeValue->IsValid()) {
		BVariant value;
		status_t error = fTypeContext->File()->EvaluateDynamicValue(
			fTypeContext->GetCompilationUnit(), fTypeContext->SubprogramEntry(),
			bitSizeValue, fTypeContext->TargetInterface(),
			fTypeContext->InstructionPointer(), fTypeContext->FramePointer(),
			value);
		if (error == B_OK && value.IsInteger())
			bitSize = value.ToUInt32();
	}

	// determine type constant
	uint32 typeConstant = 0;
	switch (typeEntry->Encoding()) {
		case DW_ATE_boolean:
			typeConstant = B_BOOL_TYPE;
			break;

		case DW_ATE_float:
			switch (bitSize) {
				case 32:
					typeConstant = B_FLOAT_TYPE;
					break;
				case 64:
					typeConstant = B_DOUBLE_TYPE;
					break;
			}
			break;

		case DW_ATE_signed:
		case DW_ATE_signed_char:
			switch (bitSize) {
				case 8:
					typeConstant = B_INT8_TYPE;
					break;
				case 16:
					typeConstant = B_INT16_TYPE;
					break;
				case 32:
					typeConstant = B_INT32_TYPE;
					break;
				case 64:
					typeConstant = B_INT64_TYPE;
					break;
			}
			break;

		case DW_ATE_address:
		case DW_ATE_unsigned:
		case DW_ATE_unsigned_char:
			switch (bitSize) {
				case 8:
					typeConstant = B_UINT8_TYPE;
					break;
				case 16:
					typeConstant = B_UINT16_TYPE;
					break;
				case 32:
					typeConstant = B_UINT32_TYPE;
					break;
				case 64:
					typeConstant = B_UINT64_TYPE;
					break;
			}
			break;

		case DW_ATE_complex_float:
		case DW_ATE_imaginary_float:
		case DW_ATE_packed_decimal:
		case DW_ATE_numeric_string:
		case DW_ATE_edited:
		case DW_ATE_signed_fixed:
		case DW_ATE_unsigned_fixed:
		case DW_ATE_decimal_float:
		default:
			break;
	}

	// create the type
	DwarfPrimitiveType* type = new(std::nothrow) DwarfPrimitiveType(
		fTypeContext, name, typeEntry, typeConstant);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateAddressType(const BString& name,
	DIEAddressingType* typeEntry, address_type_kind addressKind,
	DwarfType*& _type)
{
	// get the base type entry
	DIEAddressingType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEAddressingType>());

	// create the base type
	DwarfType* baseType;
	if (baseTypeOwnerEntry != NULL) {
		status_t error = CreateType(baseTypeOwnerEntry->GetType(), baseType);
		if (error != B_OK)
			return error;
	} else {
		// According to the DWARF 3 specs a modified type *has* a base type.
		// GCC 4 doesn't (always?) bother to add one for "void".
		// TODO: We should probably search for a respective type by name. ATM
		// we just create a DwarfUnspecifiedType without DIE.
		TRACE_LOCALS("no base type for address type entry -- creating "
			"unspecified type\n");
		baseType = new(std::nothrow) DwarfUnspecifiedType(fTypeContext, "void",
			NULL);
		if (baseType == NULL)
			return B_NO_MEMORY;
	}
	BReference<Type> baseTypeReference(baseType, true);

	DwarfAddressType* type = new(std::nothrow) DwarfAddressType(fTypeContext,
		name, typeEntry, addressKind, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateModifiedType(const BString& name,
	DIEModifiedType* typeEntry, uint32 modifiers, DwarfType*& _type)
{
	// Get the base type entry. If it is a modified type too or a typedef,
	// collect all modifiers and iterate until hitting an actual base type.
	DIEType* baseTypeEntry;
	while (true) {
		DIEModifiedType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
			typeEntry, HasTypePredicate<DIEModifiedType>());
		if (baseTypeOwnerEntry == NULL)
			return B_BAD_VALUE;

		baseTypeEntry = baseTypeOwnerEntry->GetType();

		// resolve a typedef
		if (baseTypeEntry->Tag() == DW_TAG_typedef) {
			status_t error = _ResolveTypedef(
				dynamic_cast<DIETypedef*>(baseTypeEntry), baseTypeEntry);
			if (error != B_OK)
				return error;
		}

		if (baseTypeEntry == NULL)
			return B_BAD_VALUE;

		// If the base type is a modified type, too, resolve it.
		switch (baseTypeEntry->Tag()) {
			case DW_TAG_const_type:
				modifiers |= TYPE_MODIFIER_CONST;
				baseTypeOwnerEntry
					= dynamic_cast<DIEModifiedType*>(baseTypeEntry);
				continue;
			case DW_TAG_packed_type:
				modifiers |= TYPE_MODIFIER_PACKED;
				baseTypeOwnerEntry
					= dynamic_cast<DIEModifiedType*>(baseTypeEntry);
				continue;
			case DW_TAG_volatile_type:
				modifiers |= TYPE_MODIFIER_VOLATILE;
				baseTypeOwnerEntry
					= dynamic_cast<DIEModifiedType*>(baseTypeEntry);
				continue;
			case DW_TAG_restrict_type:
				modifiers |= TYPE_MODIFIER_RESTRICT;
				baseTypeOwnerEntry
					= dynamic_cast<DIEModifiedType*>(baseTypeEntry);
				continue;
			case DW_TAG_shared_type:
				modifiers |= TYPE_MODIFIER_SHARED;
				baseTypeOwnerEntry
					= dynamic_cast<DIEModifiedType*>(baseTypeEntry);
				continue;

			default:
				break;
		}

		// If we get here, we've found an actual base type.
		break;
	}

	// create the base type
	DwarfType* baseType;
	status_t error = CreateType(baseTypeEntry, baseType);
	if (error != B_OK)
		return error;
	BReference<Type> baseTypeReference(baseType, true);

	DwarfModifiedType* type = new(std::nothrow) DwarfModifiedType(fTypeContext,
		name, typeEntry, modifiers, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateTypedefType(const BString& name,
	DIETypedef* typeEntry, DwarfType*& _type)
{
	// resolve the base type
	DIEType* baseTypeEntry;
	status_t error = _ResolveTypedef(typeEntry, baseTypeEntry);
	if (error != B_OK)
		return error;

	// create the base type
	DwarfType* baseType;
	error = CreateType(baseTypeEntry, baseType);
	if (error != B_OK)
		return error;
	BReference<Type> baseTypeReference(baseType, true);

	DwarfTypedefType* type = new(std::nothrow) DwarfTypedefType(fTypeContext,
		name, typeEntry, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateArrayType(const BString& name,
	DIEArrayType* typeEntry, DwarfType*& _type)
{
	TRACE_LOCALS("DwarfTypeFactory::_CreateArrayType(\"%s\", %p)\n",
		name.String(), typeEntry);

	// create the base type
	DIEArrayType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEArrayType>());
	if (baseTypeOwnerEntry == NULL) {
		WARNING("Failed to get base type for array type \"%s\"\n",
			name.String());
		return B_BAD_VALUE;
	}

	DwarfType* baseType = NULL;
	status_t error = CreateType(baseTypeOwnerEntry->GetType(), baseType);
	if (error != B_OK) {
		WARNING("Failed to create base type for array type \"%s\": %s\n",
			name.String(), strerror(error));
		return error;
	}
	BReference<Type> baseTypeReference(baseType, true);

	// create the array type
	DwarfArrayType* type = new(std::nothrow) DwarfArrayType(fTypeContext, name,
		typeEntry, baseType);
	if (type == NULL)
		return B_NO_MEMORY;
	BReference<DwarfType> typeReference(type, true);

	// add the array dimensions
	DIEArrayType* dimensionOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasDimensionsPredicate());

	if (dimensionOwnerEntry == NULL) {
		WARNING("Failed to get dimensions for array type \"%s\"\n",
			name.String());
		return B_BAD_VALUE;
	}

	for (DebugInfoEntryList::ConstIterator it
				= dimensionOwnerEntry->Dimensions().GetIterator();
			DebugInfoEntry* _dimensionEntry = it.Next();) {
		DIEType* dimensionEntry = dynamic_cast<DIEType*>(_dimensionEntry);

		// get/create the dimension type
		DwarfType* dimensionType = NULL;
		status_t error = CreateType(dimensionEntry, dimensionType);
		if (error != B_OK) {
			WARNING("Failed to create type for array dimension: %s\n",
				strerror(error));
			return error;
		}
		BReference<Type> dimensionTypeReference(dimensionType, true);

		// create and add the array dimension object
		DwarfArrayDimension* dimension
			= new(std::nothrow) DwarfArrayDimension(dimensionType);
		BReference<DwarfArrayDimension> dimensionReference(dimension, true);
		if (dimension == NULL || !type->AddDimension(dimension))
			return B_NO_MEMORY;
	}

	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateEnumerationType(const BString& name,
	DIEEnumerationType* typeEntry, DwarfType*& _type)
{
	// create the base type (it's optional)
	DIEEnumerationType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEEnumerationType>());

	DwarfType* baseType = NULL;
	if (baseTypeOwnerEntry != NULL) {
		status_t error = CreateType(baseTypeOwnerEntry->GetType(), baseType);
		if (error != B_OK)
			return error;
	}
	BReference<Type> baseTypeReference(baseType, true);

	// create the enumeration type
	DwarfEnumerationType* type = new(std::nothrow) DwarfEnumerationType(
		fTypeContext, name, typeEntry, baseType);
	if (type == NULL)
		return B_NO_MEMORY;
	BReference<DwarfEnumerationType> typeReference(type, true);

	// get the enumeration values
	DIEEnumerationType* enumeratorOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasEnumeratorsPredicate());

	if (enumeratorOwnerEntry != NULL) {
		for (DebugInfoEntryList::ConstIterator it
					= enumeratorOwnerEntry->Enumerators().GetIterator();
				DebugInfoEntry* _enumeratorEntry = it.Next();) {
			DIEEnumerator* enumeratorEntry = dynamic_cast<DIEEnumerator*>(
				_enumeratorEntry);

			// evaluate the value
			BVariant value;
			status_t error = fTypeContext->File()->EvaluateConstantValue(
				fTypeContext->GetCompilationUnit(),
				fTypeContext->SubprogramEntry(), enumeratorEntry->ConstValue(),
				fTypeContext->TargetInterface(),
				fTypeContext->InstructionPointer(),
				fTypeContext->FramePointer(), value);
			if (error != B_OK) {
				// The value is probably not stored -- just ignore the
				// enumerator.
				TRACE_LOCALS("Failed to get value for enum type value %s::%s\n",
					name.String(), enumeratorEntry->Name());
				continue;
			}

			// create and add the enumeration value object
			DwarfEnumeratorValue* enumValue
				= new(std::nothrow) DwarfEnumeratorValue(enumeratorEntry,
					enumeratorEntry->Name(), value);
			BReference<DwarfEnumeratorValue> enumValueReference(enumValue,
				true);
			if (enumValue == NULL || !type->AddValue(enumValue))
				return B_NO_MEMORY;
		}
	}

	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateSubrangeType(const BString& name,
	DIESubrangeType* typeEntry, DwarfType*& _type)
{
	// get the base type
	DIESubrangeType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIESubrangeType>());
	DIEType* baseTypeEntry = baseTypeOwnerEntry != NULL
		? baseTypeOwnerEntry->GetType() : NULL;

	// get the lower bound
	BVariant lowerBound;
	DIESubrangeType* lowerBoundOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasLowerBoundPredicate());
	if (lowerBoundOwnerEntry != NULL) {
		// evaluate it
		DIEType* valueType;
		status_t error = fTypeContext->File()->EvaluateDynamicValue(
			fTypeContext->GetCompilationUnit(), fTypeContext->SubprogramEntry(),
			lowerBoundOwnerEntry->LowerBound(), fTypeContext->TargetInterface(),
			fTypeContext->InstructionPointer(), fTypeContext->FramePointer(),
			lowerBound, &valueType);
		if (error != B_OK) {
			WARNING("  failed to evaluate lower bound: %s\n", strerror(error));
			return error;
		}

		// If we don't have a base type yet, and the lower bound attribute
		// refers to an object, the type of that object is our base type.
		if (baseTypeEntry == NULL)
			baseTypeEntry = valueType;
	} else {
		// that's ok -- use the language default
		lowerBound.SetTo(fTypeContext->GetCompilationUnit()->SourceLanguage()
			->subrangeLowerBound);
	}

	// get the upper bound
	BVariant upperBound;
	DIESubrangeType* upperBoundOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasUpperBoundPredicate());
	if (upperBoundOwnerEntry != NULL) {
		// evaluate it
		DIEType* valueType;
		status_t error = fTypeContext->File()->EvaluateDynamicValue(
			fTypeContext->GetCompilationUnit(), fTypeContext->SubprogramEntry(),
			upperBoundOwnerEntry->UpperBound(), fTypeContext->TargetInterface(),
			fTypeContext->InstructionPointer(), fTypeContext->FramePointer(),
			upperBound, &valueType);
		if (error != B_OK) {
			WARNING("  failed to evaluate upper bound: %s\n", strerror(error));
			return error;
		}

		// If we don't have a base type yet, and the upper bound attribute
		// refers to an object, the type of that object is our base type.
		if (baseTypeEntry == NULL)
			baseTypeEntry = valueType;
	} else {
		// get the count instead
		DIESubrangeType* countOwnerEntry = DwarfUtils::GetDIEByPredicate(
			typeEntry, HasCountPredicate());
		if (countOwnerEntry != NULL) {
			// evaluate it
			BVariant count;
			DIEType* valueType;
			status_t error = fTypeContext->File()->EvaluateDynamicValue(
				fTypeContext->GetCompilationUnit(),
				fTypeContext->SubprogramEntry(), countOwnerEntry->Count(),
				fTypeContext->TargetInterface(),
				fTypeContext->InstructionPointer(), fTypeContext->FramePointer(),
				count, &valueType);
			if (error != B_OK) {
				WARNING("  failed to evaluate count: %s\n", strerror(error));
				return error;
			}

			// If we don't have a base type yet, and the count attribute refers
			// to an object, the type of that object is our base type.
			if (baseTypeEntry == NULL)
				baseTypeEntry = valueType;

			// we only support integers
			bool isSigned;
			if (!lowerBound.IsInteger(&isSigned) || !count.IsInteger()) {
				WARNING("  count given for subrange type, but lower bound or "
					"count is not integer\n");
				return B_BAD_VALUE;
			}

			if (isSigned)
				upperBound.SetTo(lowerBound.ToInt64() + count.ToInt64() - 1);
			else
				upperBound.SetTo(lowerBound.ToUInt64() + count.ToUInt64() - 1);
		}
	}

	// create the base type
	Type* baseType = NULL;
	status_t error;
	if (baseTypeEntry != NULL) {
		DwarfType* dwarfBaseType;
		error = CreateType(baseTypeEntry, dwarfBaseType);
		baseType = dwarfBaseType;
	} else {
		// We still don't have a base type yet. In this case the base type is
		// supposed to be a signed integer type with the same size as an address
		// for that compilation unit.
		error = ArtificialIntegerType::Create(
			fTypeContext->GetCompilationUnit()->AddressSize(), true, baseType);
	}
	if (error != B_OK)
		return error;
	BReference<Type> baseTypeReference(baseType, true);

	// TODO: Support the thread scaling attribute!

	// create the type
	DwarfSubrangeType* type = new(std::nothrow) DwarfSubrangeType(fTypeContext,
		name, typeEntry, baseType, lowerBound, upperBound);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_CreateUnspecifiedType(const BString& name,
	DIEUnspecifiedType* typeEntry, DwarfType*& _type)
{
	DwarfUnspecifiedType* type = new(std::nothrow) DwarfUnspecifiedType(
		fTypeContext, name, typeEntry);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}

status_t
DwarfTypeFactory::_CreateFunctionType(const BString& name,
	DIESubroutineType* typeEntry, DwarfType*& _type)
{
	// get the return type
	DIESubroutineType* returnTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasReturnTypePredicate<DIESubroutineType>());

	// create the base type
	DwarfType* returnType = NULL;
	if (returnTypeOwnerEntry != NULL) {
		status_t error = CreateType(returnTypeOwnerEntry->ReturnType(),
			returnType);
		if (error != B_OK)
			return error;
	}
	BReference<Type> returnTypeReference(returnType, true);

	DwarfFunctionType* type = new(std::nothrow) DwarfFunctionType(fTypeContext,
		name, typeEntry, returnType);
	if (type == NULL)
		return B_NO_MEMORY;
	BReference<DwarfType> typeReference(type, true);

	// get the parameters
	DIESubroutineType* parameterOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasParametersPredicate<DIESubroutineType>());

	if (parameterOwnerEntry != NULL) {
		for (DebugInfoEntryList::ConstIterator it
					= parameterOwnerEntry->Parameters().GetIterator();
				DebugInfoEntry* _parameterEntry = it.Next();) {
			if (_parameterEntry->Tag() == DW_TAG_unspecified_parameters) {
				type->SetHasVariableArguments(true);
				continue;
			}

			DIEFormalParameter* parameterEntry
				= dynamic_cast<DIEFormalParameter*>(_parameterEntry);

			// get the type
			DIEFormalParameter* typeOwnerEntry = DwarfUtils::GetDIEByPredicate(
				parameterEntry, HasTypePredicate<DIEFormalParameter>());
			if (typeOwnerEntry == NULL)
				return B_BAD_VALUE;

			DwarfType* parameterType;
			status_t error = CreateType(typeOwnerEntry->GetType(),
				parameterType);
			if (error != B_OK)
				return error;
			BReference<DwarfType> parameterTypeReference(parameterType, true);

			// get the name
			BString parameterName;
			DwarfUtils::GetDIEName(parameterEntry, parameterName);

			// create and add the parameter object
			DwarfFunctionParameter* parameter
				= new(std::nothrow) DwarfFunctionParameter(parameterEntry,
					parameterName, parameterType);
			BReference<DwarfFunctionParameter> parameterReference(parameter,
				true);
			if (parameter == NULL || !type->AddParameter(parameter))
				return B_NO_MEMORY;
		}
	}


	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfTypeFactory::_CreatePointerToMemberType(const BString& name,
	DIEPointerToMemberType* typeEntry, DwarfType*& _type)
{
	// get the containing and base type entries
	DIEPointerToMemberType* containingTypeOwnerEntry
		= DwarfUtils::GetDIEByPredicate(typeEntry,
			HasContainingTypePredicate());
	DIEPointerToMemberType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEPointerToMemberType>());

	if (containingTypeOwnerEntry == NULL || baseTypeOwnerEntry == NULL) {
		WARNING("Failed to get containing or base type for pointer to member "
			"type \"%s\"\n", name.String());
		return B_BAD_VALUE;
	}

	// create the containing type
	DwarfType* containingType;
	status_t error = CreateType(containingTypeOwnerEntry->ContainingType(),
		containingType);
	if (error != B_OK)
		return error;
	BReference<Type> containingTypeReference(containingType, true);

	DwarfCompoundType* compoundContainingType
		= dynamic_cast<DwarfCompoundType*>(containingType);
	if (compoundContainingType == NULL) {
		WARNING("Containing type for pointer to member type \"%s\" is not a "
			"compound type.\n", name.String());
		return B_BAD_VALUE;
	}

	// create the base type
	DwarfType* baseType;
	error = CreateType(baseTypeOwnerEntry->GetType(), baseType);
	if (error != B_OK)
		return error;
	BReference<Type> baseTypeReference(baseType, true);

	// create the type object
	DwarfPointerToMemberType* type = new(std::nothrow) DwarfPointerToMemberType(
		fTypeContext, name, typeEntry, compoundContainingType, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfTypeFactory::_ResolveTypedef(DIETypedef* entry,
	DIEType*& _baseTypeEntry)
{
	while (true) {
		// resolve the base type, possibly following abstract origin or
		// specification
		DIETypedef* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
			entry, HasTypePredicate<DIETypedef>());
		if (baseTypeOwnerEntry == NULL)
			return B_BAD_VALUE;

		DIEType* baseTypeEntry = baseTypeOwnerEntry->GetType();
		if (baseTypeEntry->Tag() != DW_TAG_typedef) {
			_baseTypeEntry = baseTypeEntry;
			return B_OK;
		}

		entry = dynamic_cast<DIETypedef*>(baseTypeEntry);
	}
}


status_t
DwarfTypeFactory::_ResolveTypeByteSize(DIEType* typeEntry,
	uint64& _size)
{
	TRACE_LOCALS("DwarfTypeFactory::_ResolveTypeByteSize(%p)\n",
		typeEntry);

	// get the size attribute
	const DynamicAttributeValue* sizeValue;

	while (true) {
		// resolve a typedef
		if (typeEntry->Tag() == DW_TAG_typedef) {
			TRACE_LOCALS("  resolving typedef...\n");

			status_t error = _ResolveTypedef(
				dynamic_cast<DIETypedef*>(typeEntry), typeEntry);
			if (error != B_OK)
				return error;
		}

		sizeValue = typeEntry->ByteSize();
		if (sizeValue != NULL && sizeValue->IsValid())
			break;

		// resolve abstract origin
		if (DIEType* abstractOrigin = dynamic_cast<DIEType*>(
				typeEntry->AbstractOrigin())) {
			TRACE_LOCALS("  resolving abstract origin (%p)...\n",
				abstractOrigin);

			typeEntry = abstractOrigin;
			sizeValue = typeEntry->ByteSize();
			if (sizeValue != NULL && sizeValue->IsValid())
				break;
		}

		// resolve specification
		if (DIEType* specification = dynamic_cast<DIEType*>(
				typeEntry->Specification())) {
			TRACE_LOCALS("  resolving specification (%p)...\n", specification);

			typeEntry = specification;
			sizeValue = typeEntry->ByteSize();
			if (sizeValue != NULL && sizeValue->IsValid())
				break;
		}

		// For some types we have a special handling. For modified types we
		// follow the base type, for address types we know the size anyway.
		TRACE_LOCALS("  nothing yet, special type handling\n");

		switch (typeEntry->Tag()) {
			case DW_TAG_const_type:
			case DW_TAG_packed_type:
			case DW_TAG_volatile_type:
			case DW_TAG_restrict_type:
			case DW_TAG_shared_type:
				typeEntry = dynamic_cast<DIEModifiedType*>(typeEntry)
					->GetType();

				TRACE_LOCALS("  following modified type -> %p\n", typeEntry);

				if (typeEntry == NULL)
					return B_ENTRY_NOT_FOUND;
				break;
			case DW_TAG_pointer_type:
			case DW_TAG_reference_type:
			case DW_TAG_ptr_to_member_type:
				_size = fTypeContext->GetCompilationUnit()->AddressSize();

				TRACE_LOCALS("  pointer/reference type: size: %llu\n", _size);

				return B_OK;
			default:
				return B_ENTRY_NOT_FOUND;
		}
	}

	TRACE_LOCALS("  found attribute\n");

	// get the actual value
	BVariant size;
	status_t error = fTypeContext->File()->EvaluateDynamicValue(
		fTypeContext->GetCompilationUnit(), fTypeContext->SubprogramEntry(),
		sizeValue, fTypeContext->TargetInterface(),
		fTypeContext->InstructionPointer(), fTypeContext->FramePointer(), size);
	if (error != B_OK) {
		TRACE_LOCALS("  failed to resolve attribute: %s\n", strerror(error));
		return error;
	}

	_size = size.ToUInt64();

	TRACE_LOCALS("  -> size: %llu\n", _size);

	return B_OK;
}
