/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_INTERFACE_FACTORY_H
#define DWARF_INTERFACE_FACTORY_H


#include <String.h>

#include <util/OpenHashTable.h>

#include "Type.h"


class CompilationUnit;
class DIEAddressingType;
class DIEArrayType;
class DIEBaseType;
class DIECompoundType;
class DIEFormalParameter;
class DIEModifiedType;
class DIESubprogram;
class DIEType;
class DIETypedef;
class DwarfFile;
class DwarfTargetInterface;
class Type;
class Variable;


class DwarfInterfaceFactory {
public:
								DwarfInterfaceFactory(DwarfFile* file,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									DwarfTargetInterface* targetInterface);
								~DwarfInterfaceFactory();

			status_t			Init();

			status_t			CreateType(DIEType* typeEntry, Type*& _type);
									// returns reference
			status_t			CreateParameter(
									DIEFormalParameter* parameterEntry,
									Variable*& _parameter);
									// returns reference

private:
			struct DwarfType;
			struct DwarfDataMember;
			struct DwarfPrimitiveType;
			struct DwarfCompoundType;
			struct DwarfModifiedType;
			struct DwarfTypedefType;
			struct DwarfAddressType;
			struct DwarfArrayType;
			struct DwarfTypeHashDefinition;

			typedef OpenHashTable<DwarfTypeHashDefinition> TypeTable;

private:
			status_t			_CreateType(DIEType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateTypeInternal(DIEType* typeEntry,
									DwarfType*& _type);

			status_t			_CreateCompoundType(const BString& name,
									DIECompoundType* typeEntry,
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

			status_t			_ResolveTypedef(DIETypedef* entry,
									DIEType*& _baseTypeEntry);

private:
			DwarfFile*			fFile;
			CompilationUnit*	fCompilationUnit;
			DIESubprogram*		fSubprogramEntry;
			target_addr_t		fInstructionPointer;
			target_addr_t		fFramePointer;
			DwarfTargetInterface* fTargetInterface;
			TypeTable*			fTypes;
};


#endif	// DWARF_INTERFACE_FACTORY_H
