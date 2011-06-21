/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfStackFrameDebugInfo.h"

#include <new>

#include "Architecture.h"
#include "CompilationUnit.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfTargetInterface.h"
#include "DwarfTypeFactory.h"
#include "DwarfUtils.h"
#include "DwarfTypes.h"
#include "FunctionID.h"
#include "FunctionParameterID.h"
#include "GlobalTypeLookup.h"
#include "LocalVariableID.h"
#include "Register.h"
#include "RegisterMap.h"
#include "StringUtils.h"
#include "Tracing.h"
#include "ValueLocation.h"
#include "Variable.h"


// #pragma mark - DwarfFunctionParameterID


struct DwarfStackFrameDebugInfo::DwarfFunctionParameterID
	: public FunctionParameterID {

	DwarfFunctionParameterID(FunctionID* functionID, const BString& name)
		:
		fFunctionID(functionID),
		fName(name)
	{
		fFunctionID->AcquireReference();
	}

	virtual ~DwarfFunctionParameterID()
	{
		fFunctionID->ReleaseReference();
	}

	virtual bool operator==(const ObjectID& other) const
	{
		const DwarfFunctionParameterID* parameterID
			= dynamic_cast<const DwarfFunctionParameterID*>(&other);
		return parameterID != NULL && *fFunctionID == *parameterID->fFunctionID
			&& fName == parameterID->fName;
	}

protected:
	virtual uint32 ComputeHashValue() const
	{
		uint32 hash = fFunctionID->HashValue();
		return hash * 19 + StringUtils::HashValue(fName);
	}

private:
	FunctionID*		fFunctionID;
	const BString	fName;
};


// #pragma mark - DwarfLocalVariableID


struct DwarfStackFrameDebugInfo::DwarfLocalVariableID : public LocalVariableID {

	DwarfLocalVariableID(FunctionID* functionID, const BString& name,
		int32 line, int32 column)
		:
		fFunctionID(functionID),
		fName(name),
		fLine(line),
		fColumn(column)
	{
		fFunctionID->AcquireReference();
	}

	virtual ~DwarfLocalVariableID()
	{
		fFunctionID->ReleaseReference();
	}

	virtual bool operator==(const ObjectID& other) const
	{
		const DwarfLocalVariableID* otherID
			= dynamic_cast<const DwarfLocalVariableID*>(&other);
		return otherID != NULL && *fFunctionID == *otherID->fFunctionID
			&& fName == otherID->fName && fLine == otherID->fLine
			&& fColumn == otherID->fColumn;
	}

protected:
	virtual uint32 ComputeHashValue() const
	{
		uint32 hash = fFunctionID->HashValue();
		hash = hash * 19 + StringUtils::HashValue(fName);
		hash = hash * 19 + fLine;
		hash = hash * 19 + fColumn;
		return hash;
	}

private:
	FunctionID*		fFunctionID;
	const BString	fName;
	int32			fLine;
	int32			fColumn;
};


// #pragma mark - DwarfStackFrameDebugInfo


DwarfStackFrameDebugInfo::DwarfStackFrameDebugInfo(Architecture* architecture,
	image_id imageID, DwarfFile* file, CompilationUnit* compilationUnit,
	DIESubprogram* subprogramEntry, GlobalTypeLookup* typeLookup,
	GlobalTypeCache* typeCache, target_addr_t instructionPointer,
	target_addr_t framePointer, target_addr_t relocationDelta,
	DwarfTargetInterface* targetInterface, RegisterMap* fromDwarfRegisterMap)
	:
	StackFrameDebugInfo(),
	fTypeContext(new(std::nothrow) DwarfTypeContext(architecture, imageID, file,
		compilationUnit, subprogramEntry, instructionPointer, framePointer,
		relocationDelta, targetInterface, fromDwarfRegisterMap)),
	fTypeLookup(typeLookup),
	fTypeCache(typeCache)
{
	fTypeCache->AcquireReference();
}


DwarfStackFrameDebugInfo::~DwarfStackFrameDebugInfo()
{
	fTypeCache->ReleaseReference();

	if (fTypeContext != NULL)
		fTypeContext->ReleaseReference();

	delete fTypeFactory;
}


status_t
DwarfStackFrameDebugInfo::Init()
{
	if (fTypeContext == NULL)
		return B_NO_MEMORY;

	// create a type context without dependency to the stack frame
	DwarfTypeContext* typeContext = new(std::nothrow) DwarfTypeContext(
		fTypeContext->GetArchitecture(), fTypeContext->ImageID(),
		fTypeContext->File(), fTypeContext->GetCompilationUnit(), NULL, 0, 0,
		fTypeContext->RelocationDelta(), fTypeContext->TargetInterface(),
		fTypeContext->FromDwarfRegisterMap());
	if (typeContext == NULL)
		return B_NO_MEMORY;
	BReference<DwarfTypeContext> typeContextReference(typeContext, true);

	// create the type factory
	fTypeFactory = new(std::nothrow) DwarfTypeFactory(typeContext, fTypeLookup,
		fTypeCache);
	if (fTypeFactory == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::CreateParameter(FunctionID* functionID,
	DIEFormalParameter* parameterEntry, Variable*& _parameter)
{
	// get the name
	BString name;
	DwarfUtils::GetDIEName(parameterEntry, name);

	TRACE_LOCALS("DwarfStackFrameDebugInfo::CreateParameter(DIE: %p): name: "
		"\"%s\"\n", parameterEntry, name.String());

	// create the ID
	DwarfFunctionParameterID* id = new(std::nothrow) DwarfFunctionParameterID(
		functionID, name);
	if (id == NULL)
		return B_NO_MEMORY;
	BReference<DwarfFunctionParameterID> idReference(id, true);

	// create the variable
	return _CreateVariable(id, name, _GetDIEType(parameterEntry),
		parameterEntry->GetLocationDescription(), _parameter);
}


status_t
DwarfStackFrameDebugInfo::CreateLocalVariable(FunctionID* functionID,
	DIEVariable* variableEntry, Variable*& _variable)
{
	// get the name
	BString name;
	DwarfUtils::GetDIEName(variableEntry, name);

	TRACE_LOCALS("DwarfStackFrameDebugInfo::CreateLocalVariable(DIE: %p): "
		"name: \"%s\"\n", variableEntry, name.String());

	// get the declaration location
	int32 line = -1;
	int32 column = -1;
	const char* file;
	const char* directory;
	DwarfUtils::GetDeclarationLocation(fTypeContext->File(), variableEntry,
		directory, file, line, column);
		// TODO: If the declaration location is unavailable, we should probably
		// add a component to the ID to make it unique nonetheless (the name
		// might not suffice).

	// create the ID
	DwarfLocalVariableID* id = new(std::nothrow) DwarfLocalVariableID(
		functionID, name, line, column);
	if (id == NULL)
		return B_NO_MEMORY;
	BReference<DwarfLocalVariableID> idReference(id, true);

	// create the variable
	return _CreateVariable(id, name, _GetDIEType(variableEntry),
		variableEntry->GetLocationDescription(), _variable);
}


status_t
DwarfStackFrameDebugInfo::_CreateVariable(ObjectID* id, const BString& name,
	DIEType* typeEntry, LocationDescription* locationDescription,
	Variable*& _variable)
{
	if (typeEntry == NULL)
		return B_BAD_VALUE;

	// create the type
	DwarfType* type;
	status_t error = fTypeFactory->CreateType(typeEntry, type);
	if (error != B_OK)
		return error;
	BReference<DwarfType> typeReference(type, true);

	// get the location, if possible
	ValueLocation* location = new(std::nothrow) ValueLocation(
		fTypeContext->GetArchitecture()->IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	BReference<ValueLocation> locationReference(location, true);

	if (locationDescription->IsValid()) {
		status_t error = type->ResolveLocation(fTypeContext,
			locationDescription, 0, false, *location);
		if (error != B_OK)
			return error;

		TRACE_LOCALS_ONLY(location->Dump());
	}

	// create the variable
	Variable* variable = new(std::nothrow) Variable(id, name, type, location);
	if (variable == NULL)
		return B_NO_MEMORY;

	_variable = variable;
	return B_OK;
}


template<typename EntryType>
/*static*/ DIEType*
DwarfStackFrameDebugInfo::_GetDIEType(EntryType* entry)
{
	if (DIEType* typeEntry = entry->GetType())
		return typeEntry;

	if (EntryType* abstractOrigin = dynamic_cast<EntryType*>(
			entry->AbstractOrigin())) {
		entry = abstractOrigin;
		if (DIEType* typeEntry = entry->GetType())
			return typeEntry;
	}

	if (EntryType* specification = dynamic_cast<EntryType*>(
			entry->Specification())) {
		entry = specification;
		if (DIEType* typeEntry = entry->GetType())
			return typeEntry;
	}

	return NULL;
}
