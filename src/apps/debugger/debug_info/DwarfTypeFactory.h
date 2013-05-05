/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_TYPE_FACTORY_H
#define DWARF_TYPE_FACTORY_H


#include <String.h>

#include "Type.h"


class CompilationUnit;
class DIEAddressingType;
class DIEArrayType;
class DIEBaseType;
class DIECompoundType;
class DIEEnumerationType;
class DIEFormalParameter;
class DIEModifiedType;
class DIEPointerToMemberType;
class DIESubprogram;
class DIESubrangeType;
class DIESubroutineType;
class DIEType;
class DIETypedef;
class DIEUnspecifiedType;
class DwarfAddressType;
class DwarfArrayDimension;
class DwarfArrayType;
class DwarfCompoundType;
class DwarfDataMember;
class DwarfEnumerationType;
class DwarfEnumeratorValue;
class DwarfFile;
class DwarfFunctionParameter;
class DwarfFunctionType;
class DwarfInheritance;
class DwarfModifiedType;
class DwarfPointerToMemberType;
class DwarfPrimitiveType;
class DwarfSubrangeType;
class DwarfTargetInterface;
class DwarfType;
class DwarfTypeContext;
class DwarfTypedefType;
class DwarfUnspecifiedType;
class GlobalTypeCache;
class GlobalTypeLookup;
class LocationDescription;
class MemberLocation;
class RegisterMap;


class DwarfTypeFactory {
public:
								DwarfTypeFactory(DwarfTypeContext* typeContext,
									GlobalTypeLookup* typeLookup,
									GlobalTypeCache* typeCache);
								~DwarfTypeFactory();

			status_t			CreateType(DIEType* typeEntry,
									DwarfType*& _type);
									// returns reference

private:
			status_t			_CreateTypeInternal(const BString& name,
									DIEType* typeEntry, DwarfType*& _type);

			status_t			_CreateCompoundType(const BString& name,
									DIECompoundType* typeEntry,
									compound_type_kind compoundKind,
									DwarfType*& _type);
			status_t			_CreatePrimitiveType(const BString& name,
									DIEBaseType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateAddressType(const BString& name,
									DIEAddressingType* typeEntry,
									address_type_kind addressKind,
									DwarfType*& _type);
			status_t			_CreateModifiedType(const BString& name,
									DIEModifiedType* typeEntry,
									uint32 modifiers, DwarfType*& _type);
			status_t			_CreateTypedefType(const BString& name,
									DIETypedef* typeEntry, DwarfType*& _type);
			status_t			_CreateArrayType(const BString& name,
									DIEArrayType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateEnumerationType(const BString& name,
									DIEEnumerationType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateSubrangeType(const BString& name,
									DIESubrangeType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateUnspecifiedType(const BString& name,
									DIEUnspecifiedType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateFunctionType(const BString& name,
									DIESubroutineType* typeEntry,
									DwarfType*& _type);
			status_t			_CreatePointerToMemberType(const BString& name,
									DIEPointerToMemberType* typeEntry,
									DwarfType*& _type);

			status_t			_ResolveTypedef(DIETypedef* entry,
									DIEType*& _baseTypeEntry);
			status_t			_ResolveTypeByteSize(DIEType* typeEntry,
									uint64& _size);

private:
			class ArtificialIntegerType;

private:
			DwarfTypeContext*	fTypeContext;
			GlobalTypeLookup*	fTypeLookup;
			GlobalTypeCache*	fTypeCache;
};


#endif	// DWARF_TYPE_FACTORY_H
