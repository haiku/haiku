/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_INTERFACE_FACTORY_H
#define DWARF_INTERFACE_FACTORY_H


#include <String.h>

#include <util/OpenHashTable.h>

#include "StackFrameDebugInfo.h"
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
class DIEVariable;
class DwarfFile;
class DwarfTargetInterface;
class FunctionID;
class LocationDescription;
class MemberLocation;
class ObjectID;
class RegisterMap;
class Variable;


class DwarfStackFrameDebugInfo : public StackFrameDebugInfo {
public:
								DwarfStackFrameDebugInfo(DwarfFile* file,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									DwarfTargetInterface* targetInterface,
									RegisterMap* fromDwarfRegisterMap);
								~DwarfStackFrameDebugInfo();

			status_t			Init();

	virtual	status_t			ResolveObjectDataLocation(
									StackFrame* stackFrame, Type* type,
									target_addr_t objectAddress,
									ValueLocation*& _location);
	virtual	status_t			ResolveBaseTypeLocation(
									StackFrame* stackFrame, Type* type,
									BaseType* baseType,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
	virtual	status_t			ResolveDataMemberLocation(
									StackFrame* stackFrame, Type* type,
									DataMember* member,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);

			status_t			CreateType(DIEType* typeEntry, Type*& _type);
									// returns reference
			status_t			CreateParameter(FunctionID* functionID,
									DIEFormalParameter* parameterEntry,
									Variable*& _parameter);
									// returns reference
			status_t			CreateLocalVariable(FunctionID* functionID,
									DIEVariable* variableEntry,
									Variable*& _variable);
									// returns reference

private:
			struct DwarfFunctionParameterID;
			struct DwarfLocalVariableID;
			struct DwarfType;
			struct DwarfInheritance;
			struct DwarfDataMember;
			struct DwarfPrimitiveType;
			struct DwarfCompoundType;
			struct DwarfModifiedType;
			struct DwarfTypedefType;
			struct DwarfAddressType;
			struct DwarfArrayType;
			struct DwarfTypeHashDefinition;

			typedef BOpenHashTable<DwarfTypeHashDefinition> TypeTable;

private:
			status_t			_ResolveDataMemberLocation(
									StackFrame* stackFrame,
									DwarfCompoundType* type,
									Type* memberType,
									const MemberLocation* memberLocation,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
									// returns a new location

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

			status_t			_CreateVariable(ObjectID* id,
									const BString& name, DIEType* typeEntry,
									LocationDescription* locationDescription,
									Variable*& _variable);

			status_t			_ResolveTypedef(DIETypedef* entry,
									DIEType*& _baseTypeEntry);
			status_t			_ResolveTypeByteSize(DIEType* typeEntry,
									uint64& _size);

			void				_FixLocation(ValueLocation* location,
									DwarfType* type);

	template<typename EntryType>
	static	DIEType*			_GetDIEType(EntryType* entry);

private:
			DwarfFile*			fFile;
			CompilationUnit*	fCompilationUnit;
			DIESubprogram*		fSubprogramEntry;
			target_addr_t		fInstructionPointer;
			target_addr_t		fFramePointer;
			DwarfTargetInterface* fTargetInterface;
			RegisterMap*		fFromDwarfRegisterMap;
			TypeTable*			fTypes;
};


#endif	// DWARF_INTERFACE_FACTORY_H
