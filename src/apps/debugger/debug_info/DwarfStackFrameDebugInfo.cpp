/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfStackFrameDebugInfo.h"

#include <algorithm>
#include <new>

#include <Variant.h>

#include "Architecture.h"
#include "CompilationUnit.h"
#include "DebugInfoEntries.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfUtils.h"
#include "FunctionID.h"
#include "FunctionParameterID.h"
#include "LocalVariableID.h"
#include "Register.h"
#include "RegisterMap.h"
#include "SourceLanguageInfo.h"
#include "StringUtils.h"
#include "Tracing.h"
#include "ValueLocation.h"
#include "Variable.h"


namespace {


// #pragma mark - HasTypePredicate


template<typename EntryType>
struct HasTypePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->GetType() != NULL;
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


}	// unnamed namespace


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


// #pragma mark - DwarfType


struct DwarfStackFrameDebugInfo::DwarfType : virtual Type {
public:
	DwarfType(const BString& name)
		:
		fName(name),
		fByteSize(0)
	{
	}

	virtual const char* Name() const
	{
		return fName.Length() > 0 ? fName.String() : NULL;
	}

	virtual target_size_t ByteSize() const
	{
		return fByteSize;
	}

	void SetByteSize(target_size_t size)
	{
		fByteSize = size;
	}

	virtual DIEType* GetDIEType() const = 0;

private:
	BString			fName;
	target_size_t	fByteSize;

public:
	DwarfType*		fNext;
};


// #pragma mark - DwarfInheritance


struct DwarfStackFrameDebugInfo::DwarfInheritance : BaseType {
public:
	DwarfInheritance(DIEInheritance* entry, DwarfType* type)
		:
		fEntry(entry),
		fType(type)
	{
		fType->AcquireReference();
	}

	~DwarfInheritance()
	{
		fType->ReleaseReference();
	}

	virtual Type* GetType() const
	{
		return fType;
	}

	DIEInheritance* Entry() const
	{
		return fEntry;
	}

private:
	DIEInheritance*	fEntry;
	DwarfType*		fType;

};


// #pragma mark - DwarfDataMember


struct DwarfStackFrameDebugInfo::DwarfDataMember : DataMember {
public:
	DwarfDataMember(DIEMember* entry, const BString& name, DwarfType* type)
		:
		fEntry(entry),
		fName(name),
		fType(type)
	{
		fType->AcquireReference();
	}

	~DwarfDataMember()
	{
		fType->ReleaseReference();
	}

	virtual const char* Name() const
	{
		return fName.Length() > 0 ? fName.String() : NULL;
	}

	virtual Type* GetType() const
	{
		return fType;
	}

	DIEMember* Entry() const
	{
		return fEntry;
	}

private:
	DIEMember*	fEntry;
	BString		fName;
	DwarfType*	fType;

};


// #pragma mark - DwarfEnumerationValue


struct DwarfStackFrameDebugInfo::DwarfEnumerationValue : EnumerationValue {
public:
	DwarfEnumerationValue(DIEEnumerator* entry, const BString& name,
		const BVariant& value)
		:
		fEntry(entry),
		fName(name),
		fValue(value)
	{
	}

	~DwarfEnumerationValue()
	{
	}

	virtual const char* Name() const
	{
		return fName.Length() > 0 ? fName.String() : NULL;
	}

	DIEEnumerator* Entry() const
	{
		return fEntry;
	}

	virtual BVariant Value() const
	{
		return fValue;
	}

private:
	DIEEnumerator*	fEntry;
	BString			fName;
	BVariant		fValue;

};


// #pragma mark - DwarfArrayDimension


struct DwarfStackFrameDebugInfo::DwarfArrayDimension : ArrayDimension {
public:
	DwarfArrayDimension(DwarfType* type)
		:
		fType(type)
	{
		fType->AcquireReference();
	}

	~DwarfArrayDimension()
	{
		fType->ReleaseReference();
	}

	virtual Type* GetType() const
	{
		return fType;
	}

private:
	DwarfType*	fType;

};


// #pragma mark - DwarfPrimitiveType


struct DwarfStackFrameDebugInfo::DwarfPrimitiveType : PrimitiveType, DwarfType {
public:
	DwarfPrimitiveType(const BString& name, DIEBaseType* entry,
		uint32 typeConstant)
		:
		DwarfType(name),
		fEntry(entry),
		fTypeConstant(typeConstant)
	{
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIEBaseType* Entry() const
	{
		return fEntry;
	}

	virtual uint32 TypeConstant() const
	{
		return fTypeConstant;
	}

private:
	DIEBaseType*	fEntry;
	uint32			fTypeConstant;
};


// #pragma mark - DwarfCompoundType


struct DwarfStackFrameDebugInfo::DwarfCompoundType : CompoundType, DwarfType {
public:
	DwarfCompoundType(const BString& name, DIECompoundType* entry)
		:
		DwarfType(name),
		fEntry(entry)
	{
	}

	~DwarfCompoundType()
	{
		for (int32 i = 0;
				DwarfInheritance* inheritance = fInheritances.ItemAt(i); i++) {
			inheritance->ReleaseReference();
		}
		for (int32 i = 0; DwarfDataMember* member = fDataMembers.ItemAt(i); i++)
			member->ReleaseReference();
	}

	virtual int32 CountBaseTypes() const
	{
		return fInheritances.CountItems();
	}

	virtual BaseType* BaseTypeAt(int32 index) const
	{
		return fInheritances.ItemAt(index);
	}

	virtual int32 CountDataMembers() const
	{
		return fDataMembers.CountItems();
	}

	virtual DataMember* DataMemberAt(int32 index) const
	{
		return fDataMembers.ItemAt(index);
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIECompoundType* Entry() const
	{
		return fEntry;
	}

	bool AddInheritance(DwarfInheritance* inheritance)
	{
		if (!fInheritances.AddItem(inheritance))
			return false;

		inheritance->AcquireReference();
		return true;
	}

	bool AddDataMember(DwarfDataMember* member)
	{
		if (!fDataMembers.AddItem(member))
			return false;

		member->AcquireReference();
		return true;
	}

private:
	typedef BObjectList<DwarfDataMember> DataMemberList;
	typedef BObjectList<DwarfInheritance> InheritanceList;

private:
	DIECompoundType*	fEntry;
	InheritanceList		fInheritances;
	DataMemberList		fDataMembers;
};


// #pragma mark - DwarfModifiedType


struct DwarfStackFrameDebugInfo::DwarfModifiedType : ModifiedType, DwarfType {
public:
	DwarfModifiedType(const BString& name, DIEModifiedType* entry,
		uint32 modifiers, DwarfType* baseType)
		:
		DwarfType(name),
		fEntry(entry),
		fModifiers(modifiers),
		fBaseType(baseType)
	{
		fBaseType->AcquireReference();
	}

	~DwarfModifiedType()
	{
		fBaseType->ReleaseReference();
	}

	virtual uint32 Modifiers() const
	{
		return fModifiers;
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIEModifiedType* Entry() const
	{
		return fEntry;
	}

private:
	DIEModifiedType*	fEntry;
	uint32				fModifiers;
	DwarfType*			fBaseType;
};


// #pragma mark - DwarfTypedefType


struct DwarfStackFrameDebugInfo::DwarfTypedefType : TypedefType, DwarfType {
public:
	DwarfTypedefType(const BString& name, DIETypedef* entry,
		DwarfType* baseType)
		:
		DwarfType(name),
		fEntry(entry),
		fBaseType(baseType)
	{
		fBaseType->AcquireReference();
	}

	~DwarfTypedefType()
	{
		fBaseType->ReleaseReference();
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIETypedef* Entry() const
	{
		return fEntry;
	}

private:
	DIETypedef*	fEntry;
	DwarfType*	fBaseType;
};


// #pragma mark - DwarfAddressType


struct DwarfStackFrameDebugInfo::DwarfAddressType : AddressType, DwarfType {
public:
	DwarfAddressType(const BString& name, DIEAddressingType* entry,
		address_type_kind addressKind, DwarfType* baseType)
		:
		DwarfType(name),
		fEntry(entry),
		fAddressKind(addressKind),
		fBaseType(baseType)
	{
		fBaseType->AcquireReference();
	}

	~DwarfAddressType()
	{
		fBaseType->ReleaseReference();
	}

	virtual address_type_kind AddressKind() const
	{
		return fAddressKind;
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIEAddressingType* Entry() const
	{
		return fEntry;
	}

private:
	DIEAddressingType*	fEntry;
	address_type_kind	fAddressKind;
	DwarfType*			fBaseType;
};


// #pragma mark - DwarfEnumerationType


struct DwarfStackFrameDebugInfo::DwarfEnumerationType : EnumerationType,
	DwarfType {
public:
	DwarfEnumerationType(const BString& name, DIEEnumerationType* entry,
		DwarfType* baseType)
		:
		DwarfType(name),
		fEntry(entry),
		fBaseType(baseType)
	{
		if (fBaseType != NULL)
			fBaseType->AcquireReference();
	}

	~DwarfEnumerationType()
	{
		for (int32 i = 0; DwarfEnumerationValue* value = fValues.ItemAt(i); i++)
			value->ReleaseReference();

		if (fBaseType != NULL)
			fBaseType->ReleaseReference();
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual int32 CountValues() const
	{
		return fValues.CountItems();
	}

	virtual EnumerationValue* ValueAt(int32 index) const
	{
		return fValues.ItemAt(index);
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIEEnumerationType* Entry() const
	{
		return fEntry;
	}

	bool AddValue(DwarfEnumerationValue* value)
	{
		if (!fValues.AddItem(value))
			return false;

		value->AcquireReference();
		return true;
	}

private:
	typedef BObjectList<DwarfEnumerationValue> ValueList;

private:
	DIEEnumerationType*	fEntry;
	DwarfType*			fBaseType;
	ValueList			fValues;
};


// #pragma mark - DwarfSubrangeType


struct DwarfStackFrameDebugInfo::DwarfSubrangeType : SubrangeType, DwarfType {
public:
	DwarfSubrangeType(const BString& name, DIESubrangeType* entry,
		DwarfType* baseType, const BVariant& lowerBound,
		const BVariant& upperBound)
		:
		DwarfType(name),
		fEntry(entry),
		fBaseType(baseType),
		fLowerBound(lowerBound),
		fUpperBound(upperBound)
	{
		fBaseType->AcquireReference();
	}

	~DwarfSubrangeType()
	{
		fBaseType->ReleaseReference();
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	virtual BVariant LowerBound() const
	{
		return fLowerBound;
	}

	virtual BVariant UpperBound() const
	{
		return fUpperBound;
	}

	DIESubrangeType* Entry() const
	{
		return fEntry;
	}

private:
	DIESubrangeType*	fEntry;
	DwarfType*			fBaseType;
	BVariant			fLowerBound;
	BVariant			fUpperBound;
};


// #pragma mark - DwarfArrayType


struct DwarfStackFrameDebugInfo::DwarfArrayType : ArrayType, DwarfType {
	DwarfArrayType(const BString& name, DIEArrayType* entry,
		DwarfType* baseType)
		:
		DwarfType(name),
		fEntry(entry),
		fBaseType(baseType)
	{
		fBaseType->AcquireReference();
	}

	~DwarfArrayType()
	{
		for (int32 i = 0;
			DwarfArrayDimension* dimension = fDimensions.ItemAt(i); i++) {
			dimension->ReleaseReference();
		}

		fBaseType->ReleaseReference();
	}

	virtual Type* BaseType() const
	{
		return fBaseType;
	}

	virtual int32 CountDimensions() const
	{
		return fDimensions.CountItems();
	}

	virtual ArrayDimension* DimensionAt(int32 index) const
	{
		return fDimensions.ItemAt(index);
	}

	virtual DIEType* GetDIEType() const
	{
		return fEntry;
	}

	DIEArrayType* Entry() const
	{
		return fEntry;
	}

	bool AddDimension(DwarfArrayDimension* dimension)
	{
		if (!fDimensions.AddItem(dimension))
			return false;

		dimension->AcquireReference();
		return true;
	}

private:
	typedef BObjectList<DwarfArrayDimension> DimensionList;

private:
	DIEArrayType*		fEntry;
	DwarfType*			fBaseType;
	DimensionList		fDimensions;
};


// #pragma mark - DwarfTypeHashDefinition


struct DwarfStackFrameDebugInfo::DwarfTypeHashDefinition {
	typedef const DIEType*	KeyType;
	typedef	DwarfType		ValueType;

	size_t HashKey(const DIEType* key) const
	{
		return (addr_t)key;
	}

	size_t Hash(const DwarfType* value) const
	{
		return HashKey(value->GetDIEType());
	}

	bool Compare(const DIEType* key, const DwarfType* value) const
	{
		return key == value->GetDIEType();
	}

	DwarfType*& GetLink(DwarfType* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - DwarfStackFrameDebugInfo


DwarfStackFrameDebugInfo::DwarfStackFrameDebugInfo(Architecture* architecture,
	DwarfFile* file, CompilationUnit* compilationUnit,
	DIESubprogram* subprogramEntry, target_addr_t instructionPointer,
	target_addr_t framePointer, DwarfTargetInterface* targetInterface,
	RegisterMap* fromDwarfRegisterMap)
	:
	StackFrameDebugInfo(architecture),
	fFile(file),
	fCompilationUnit(compilationUnit),
	fSubprogramEntry(subprogramEntry),
	fInstructionPointer(instructionPointer),
	fFramePointer(framePointer),
	fTargetInterface(targetInterface),
	fFromDwarfRegisterMap(fromDwarfRegisterMap),
	fTypes(NULL)
{
}


DwarfStackFrameDebugInfo::~DwarfStackFrameDebugInfo()
{
	if (fTypes != NULL) {
		DwarfType* type = fTypes->Clear(true);
		while (type != NULL) {
			DwarfType* next = type->fNext;
			type->ReleaseReference();
			type = next;
		}

		delete fTypes;
	}
}


status_t
DwarfStackFrameDebugInfo::Init()
{
	fTypes = new(std::nothrow) TypeTable;
	if (fTypes == NULL)
		return B_NO_MEMORY;

	return fTypes->Init();
}


status_t
DwarfStackFrameDebugInfo::ResolveObjectDataLocation(StackFrame* stackFrame,
	Type* type, const ValueLocation& objectLocation, ValueLocation*& _location)
{
	// TODO: In some source languages the object address might be a pointer to
	// a descriptor, not the actual object data.

	// If the given location looks good already, just clone it.
	int32 count = objectLocation.CountPieces();
	if (count == 0)
		return B_BAD_VALUE;

	ValuePieceLocation piece = objectLocation.PieceAt(0);
	if (count > 1 || piece.type != VALUE_PIECE_LOCATION_MEMORY
		|| piece.size != 0 || piece.bitSize != 0) {
		ValueLocation* location
			= new(std::nothrow) ValueLocation(objectLocation);
		if (location == NULL || location->CountPieces() != count) {
			delete location;
			return B_NO_MEMORY;
		}

		_location = location;
		return B_OK;
	}

	// The location contains just a single address piece with a zero size -- set
	// the type's size.
	piece.SetSize(type->ByteSize());
		// TODO: Use bit size and bit offset, if specified!

	ValueLocation* location = new(std::nothrow) ValueLocation(
		objectLocation.IsBigEndian());
	if (location == NULL || !location->AddPiece(piece)) {
		delete location;
		return B_NO_MEMORY;
	}

	_location = location;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::ResolveBaseTypeLocation(StackFrame* stackFrame,
	Type* _type, BaseType* _baseType, const ValueLocation& parentLocation,
	ValueLocation*& _location)
{
	DwarfCompoundType* type = dynamic_cast<DwarfCompoundType*>(_type);
	DwarfInheritance* baseType = dynamic_cast<DwarfInheritance*>(_baseType);
	if (type == NULL || baseType == NULL)
		return B_BAD_VALUE;

	return _ResolveDataMemberLocation(stackFrame, type, baseType->GetType(),
		baseType->Entry()->Location(), parentLocation, _location);
}


status_t
DwarfStackFrameDebugInfo::ResolveDataMemberLocation(StackFrame* stackFrame,
	Type* _type, DataMember* _member, const ValueLocation& parentLocation,
	ValueLocation*& _location)
{
	DwarfCompoundType* type = dynamic_cast<DwarfCompoundType*>(_type);
	DwarfDataMember* member = dynamic_cast<DwarfDataMember*>(_member);
	if (type == NULL || member == NULL)
		return B_BAD_VALUE;

	ValueLocation* location;
	status_t error = _ResolveDataMemberLocation(stackFrame, type,
		member->GetType(), member->Entry()->Location(), parentLocation,
		location);
	if (error != B_OK)
		return error;

	// If the member isn't a bit field, we're done.
	DIEMember* memberEntry = member->Entry();
	if (!memberEntry->ByteSize()->IsValid()
		&& !memberEntry->BitOffset()->IsValid()
		&& !memberEntry->BitSize()->IsValid()) {
		_location = location;
		return B_OK;
	}

	Reference<ValueLocation> locationReference(location);

	// get the byte size
	target_addr_t byteSize;
	if (memberEntry->ByteSize()->IsValid()) {
		BVariant value;
		error = fFile->EvaluateDynamicValue(fCompilationUnit, fSubprogramEntry,
			memberEntry->ByteSize(), fTargetInterface, fInstructionPointer,
			fFramePointer, value);
		if (error != B_OK)
			return error;
		byteSize = value.ToUInt64();
	} else
		byteSize = type->ByteSize();

	// get the bit offset
	uint64 bitOffset = 0;
	if (memberEntry->BitOffset()->IsValid()) {
		BVariant value;
		error = fFile->EvaluateDynamicValue(fCompilationUnit, fSubprogramEntry,
			memberEntry->BitOffset(), fTargetInterface, fInstructionPointer,
			fFramePointer, value);
		if (error != B_OK)
			return error;
		bitOffset = value.ToUInt64();
	}

	// get the bit size
	uint64 bitSize = byteSize * 8;
	if (memberEntry->BitSize()->IsValid()) {
		BVariant value;
		error = fFile->EvaluateDynamicValue(fCompilationUnit, fSubprogramEntry,
			memberEntry->BitSize(), fTargetInterface, fInstructionPointer,
			fFramePointer, value);
		if (error != B_OK)
			return error;
		bitSize = value.ToUInt64();
	}

	TRACE_LOCALS("bit field: byte size: %llu, bit offset/size: %llu/%llu\n",
		byteSize, bitOffset, bitSize);

	if (bitOffset + bitSize > byteSize * 8)
		return B_BAD_VALUE;

	// create the bit field value location
	ValueLocation* bitFieldLocation = new(std::nothrow) ValueLocation;
	if (bitFieldLocation == NULL)
		return B_NO_MEMORY;
	Reference<ValueLocation> bitFieldLocationReference(bitFieldLocation, true);

	if (!bitFieldLocation->SetTo(*location, bitOffset, bitSize))
		return B_NO_MEMORY;

	_location = bitFieldLocationReference.Detach();
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::CreateType(DIEType* typeEntry, Type*& _type)
{
	DwarfType* type;
	status_t error = _CreateType(typeEntry, type);
	if (error != B_OK)
		return error;

	_type = type;
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
	Reference<DwarfFunctionParameterID> idReference(id, true);

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
	DwarfUtils::GetDeclarationLocation(fFile, variableEntry, directory, file,
		line, column);
		// TODO: If the declaration location is unavailable, we should probably
		// add a component to the ID to make it unique nonetheless (the name
		// might not suffice).

	// create the ID
	DwarfLocalVariableID* id = new(std::nothrow) DwarfLocalVariableID(
		functionID, name, line, column);
	if (id == NULL)
		return B_NO_MEMORY;
	Reference<DwarfLocalVariableID> idReference(id, true);

	// create the variable
	return _CreateVariable(id, name, _GetDIEType(variableEntry),
		variableEntry->GetLocationDescription(), _variable);
}


status_t
DwarfStackFrameDebugInfo::_ResolveDataMemberLocation(StackFrame* stackFrame,
	DwarfCompoundType* type, Type* memberType,
	const MemberLocation* memberLocation, const ValueLocation& parentLocation,
	ValueLocation*& _location)
{
	// create the value location object for the member
	ValueLocation* location = new(std::nothrow) ValueLocation(
		parentLocation.IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	Reference<ValueLocation> locationReference(location, true);

	switch (memberLocation->attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
		{
			if (!location->SetTo(parentLocation, memberLocation->constant * 8,
					memberType->ByteSize() * 8)) {
				return B_NO_MEMORY;
			}

			break;
		}
		case ATTRIBUTE_CLASS_BLOCK:
		case ATTRIBUTE_CLASS_LOCLISTPTR:
		{
			// The attribute is a location description. Since we need to push
			// the parent object value onto the stack, we require the parent
			// location to be a memory location.
			if (parentLocation.CountPieces() != 1)
				return B_BAD_VALUE;
			ValuePieceLocation piece = parentLocation.PieceAt(0);
			if (piece.type != VALUE_PIECE_LOCATION_MEMORY)
				return B_BAD_VALUE;

			// convert member location to location description
			LocationDescription locationDescription;
			if (memberLocation->attributeClass == ATTRIBUTE_CLASS_BLOCK) {
				locationDescription.SetToExpression(
					memberLocation->expression.data,
					memberLocation->expression.length);
			} else {
				locationDescription.SetToLocationList(
					memberLocation->listOffset);
			}

			// evaluate the location description
			status_t error = _ResolveLocation(&locationDescription,
				piece.address, memberType, *location);
			if (error != B_OK)
				return error;

			break;
		}
		default:
		{
			// for unions the member location can be omitted -- all members
			// start at the beginning of the parent object
			if (type->GetDIEType()->Tag() != DW_TAG_union_type)
				return B_BAD_VALUE;

			if (!location->SetTo(parentLocation, 0, memberType->ByteSize() * 8))
				return B_NO_MEMORY;

			break;
		}
	}

	_location = locationReference.Detach();
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateType(DIEType* typeEntry, DwarfType*& _type)
{
	// Try the type cache first. If we don't know the type yet, create it.
	DwarfType* type = fTypes->Lookup(typeEntry);

	if (type == NULL) {
		status_t error = _CreateTypeInternal(typeEntry, type);
		if (error != B_OK)
			return error;

		// Insert the type into the hash table. Recheck, as the type may already
		// have been inserted (e.g. in the compound type case).
		if (fTypes->Lookup(typeEntry) == NULL)
			fTypes->Insert(type);

		// try to get the type's size
		uint64 size;
		if (_ResolveTypeByteSize(typeEntry, size) == B_OK)
			type->SetByteSize(size);
	}

	type->AcquireReference();
	_type = type;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateTypeInternal(DIEType* typeEntry,
	DwarfType*& _type)
{
	BString name;
	DwarfUtils::GetFullyQualifiedDIEName(typeEntry, name);
// TODO: The DIE may not have a name (e.g. pointer and reference types don't).

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
		case DW_TAG_subroutine_type:
		case DW_TAG_ptr_to_member_type:
			// TODO: Implement!
			return B_UNSUPPORTED;

		case DW_TAG_string_type:
		case DW_TAG_file_type:
		case DW_TAG_set_type:
			// TODO: Implement (not relevant for C++)!
			return B_UNSUPPORTED;
	}

	return B_UNSUPPORTED;
}


status_t
DwarfStackFrameDebugInfo::_CreateCompoundType(const BString& name,
	DIECompoundType* typeEntry, DwarfType*& _type)
{
	TRACE_LOCALS("DwarfStackFrameDebugInfo::_CreateCompoundType(\"%s\", %p)\n",
		name.String(), typeEntry);

	// create the type
	DwarfCompoundType* type = new(std::nothrow) DwarfCompoundType(name,
		typeEntry);
	if (type == NULL)
		return B_NO_MEMORY;
	Reference<DwarfCompoundType> typeReference(type, true);

	// Already add the type at this pointer to the hash table, since otherwise
	// we could run into an infinite recursion when trying to create the types
	// for the data members.
	fTypes->Insert(type);

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
			if (_CreateType(memberEntry->GetType(), memberType) != B_OK)
				continue;
			Reference<DwarfType> memberTypeReference(memberType, true);

			// get the name
			BString memberName;
			DwarfUtils::GetDIEName(memberEntry, memberName);

			// create and add the member object
			DwarfDataMember* member = new(std::nothrow) DwarfDataMember(
				memberEntry, memberName, memberType);
			Reference<DwarfDataMember> memberReference(member, true);
			if (member == NULL || !type->AddDataMember(member)) {
				fTypes->Remove(type);
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
				if (_CreateType(inheritanceEntry->GetType(), baseType) != B_OK)
					continue;
				Reference<DwarfType> baseTypeReference(baseType, true);

				// create and add the inheritance object
				DwarfInheritance* inheritance = new(std::nothrow)
					DwarfInheritance(inheritanceEntry, baseType);
				Reference<DwarfInheritance> inheritanceReference(inheritance,
					true);
				if (inheritance == NULL || !type->AddInheritance(inheritance)) {
					fTypes->Remove(type);
					return B_NO_MEMORY;
				}
			}
		}
	}

	_type = typeReference.Detach();
	return B_OK;;
}


status_t
DwarfStackFrameDebugInfo::_CreatePrimitiveType(const BString& name,
	DIEBaseType* typeEntry, DwarfType*& _type)
{
	const DynamicAttributeValue* byteSizeValue = typeEntry->ByteSize();
//	const DynamicAttributeValue* bitOffsetValue = typeEntry->BitOffset();
	const DynamicAttributeValue* bitSizeValue = typeEntry->BitSize();

	uint32 bitSize = 0;
	if (byteSizeValue->IsValid()) {
		BVariant value;
		status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
			fSubprogramEntry, byteSizeValue, fTargetInterface,
			fInstructionPointer, fFramePointer, value);
		if (error == B_OK && value.IsInteger())
			bitSize = value.ToUInt32() * 8;
	} else if (bitSizeValue->IsValid()) {
		BVariant value;
		status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
			fSubprogramEntry, bitSizeValue, fTargetInterface,
			fInstructionPointer, fFramePointer, value);
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
	DwarfPrimitiveType* type = new(std::nothrow) DwarfPrimitiveType(name,
		typeEntry, typeConstant);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateAddressType(const BString& name,
	DIEAddressingType* typeEntry, address_type_kind addressKind,
	DwarfType*& _type)
{
	// get the base type entry
	DIEAddressingType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEAddressingType>());
	if (baseTypeOwnerEntry == NULL)
		return B_BAD_VALUE;

	// create the base type
	DwarfType* baseType;
	status_t error = _CreateType(baseTypeOwnerEntry->GetType(), baseType);
	if (error != B_OK)
		return error;
	Reference<Type> baseTypeReference(baseType, true);

	DwarfAddressType* type = new(std::nothrow) DwarfAddressType(name, typeEntry,
		addressKind, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateModifiedType(const BString& name,
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
	status_t error = _CreateType(baseTypeEntry, baseType);
	if (error != B_OK)
		return error;
	Reference<Type> baseTypeReference(baseType, true);

	DwarfModifiedType* type = new(std::nothrow) DwarfModifiedType(name,
		typeEntry, modifiers, baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateTypedefType(const BString& name,
	DIETypedef* typeEntry, DwarfType*& _type)
{
	// resolve the base type
	DIEType* baseTypeEntry;
	status_t error = _ResolveTypedef(typeEntry, baseTypeEntry);
	if (error != B_OK)
		return error;

	// create the base type
	DwarfType* baseType;
	error = _CreateType(baseTypeEntry, baseType);
	if (error != B_OK)
		return error;
	Reference<Type> baseTypeReference(baseType, true);

	DwarfTypedefType* type = new(std::nothrow) DwarfTypedefType(name, typeEntry,
		baseType);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateArrayType(const BString& name,
	DIEArrayType* typeEntry, DwarfType*& _type)
{
	// create the base type
	DIEArrayType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEArrayType>());
	if (baseTypeOwnerEntry != NULL) {
		WARNING("Failed to get base type for array type \"%s\"\n",
			name.String());
		return B_BAD_VALUE;
	}

	DwarfType* baseType = NULL;
	status_t error = _CreateType(baseTypeOwnerEntry->GetType(), baseType);
	if (error != B_OK)
		return error;
	Reference<Type> baseTypeReference(baseType, true);

	// create the array type
	DwarfArrayType* type = new(std::nothrow) DwarfArrayType(name, typeEntry,
		baseType);
	if (type == NULL)
		return B_NO_MEMORY;
	Reference<DwarfType> typeReference(type, true);

	// add the array dimensions
	DIEArrayType* dimensionOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasDimensionsPredicate());

	if (dimensionOwnerEntry != NULL) {
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
		status_t error = _CreateType(dimensionEntry, dimensionType);
		if (error != B_OK)
			return error;
		Reference<Type> dimensionTypeReference(dimensionType, true);

		// create and add the array dimension object
		DwarfArrayDimension* dimension
			= new(std::nothrow) DwarfArrayDimension(dimensionType);
		Reference<DwarfArrayDimension> dimensionReference(dimension, true);
		if (dimension == NULL || !type->AddDimension(dimension))
			return B_NO_MEMORY;
	}

	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateEnumerationType(const BString& name,
	DIEEnumerationType* typeEntry, DwarfType*& _type)
{
	// create the base type (it's optional)
	DIEEnumerationType* baseTypeOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasTypePredicate<DIEEnumerationType>());

	DwarfType* baseType = NULL;
	if (baseTypeOwnerEntry != NULL) {
		status_t error = _CreateType(baseTypeOwnerEntry->GetType(), baseType);
		if (error != B_OK)
			return error;
	}
	Reference<Type> baseTypeReference(baseType, true);

	// create the enumeration type
	DwarfEnumerationType* type = new(std::nothrow) DwarfEnumerationType(name,
		typeEntry, baseType);
	if (type == NULL)
		return B_NO_MEMORY;
	Reference<DwarfEnumerationType> typeReference(type, true);

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
			status_t error = fFile->EvaluateConstantValue(fCompilationUnit,
				fSubprogramEntry, enumeratorEntry->ConstValue(),
				fTargetInterface, fInstructionPointer, fFramePointer, value);
			if (error != B_OK) {
				// The value is probably not stored -- just ignore the
				// enumerator.
				TRACE_LOCALS("Failed to get value for enum type value %s::%s\n",
					name.String(), enumeratorEntry->Name());
				continue;
			}

			// create and add the enumeration value object
			DwarfEnumerationValue* enumValue
				= new(std::nothrow) DwarfEnumerationValue(enumeratorEntry,
					enumeratorEntry->Name(), value);
			Reference<DwarfEnumerationValue> enumValueReference(enumValue, true);
			if (enumValue == NULL || !type->AddValue(enumValue))
				return B_NO_MEMORY;
		}
	}

	_type = typeReference.Detach();
	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_CreateSubrangeType(const BString& name,
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
		status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
			fSubprogramEntry, lowerBoundOwnerEntry->LowerBound(),
			fTargetInterface, fInstructionPointer, fFramePointer, lowerBound,
			&valueType);
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
		lowerBound.SetTo(
			fCompilationUnit->SourceLanguage()->subrangeLowerBound);
	}

	// get the upper bound
	BVariant upperBound;
	DIESubrangeType* upperBoundOwnerEntry = DwarfUtils::GetDIEByPredicate(
		typeEntry, HasUpperBoundPredicate());
	if (upperBoundOwnerEntry != NULL) {
		// evaluate it
		DIEType* valueType;
		status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
			fSubprogramEntry, upperBoundOwnerEntry->UpperBound(),
			fTargetInterface, fInstructionPointer, fFramePointer, upperBound,
			&valueType);
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
			status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
				fSubprogramEntry, countOwnerEntry->Count(), fTargetInterface,
				fInstructionPointer, fFramePointer, count, &valueType);
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
				upperBound.SetTo(lowerBound.ToInt64() + count.ToInt64());
			else
				upperBound.SetTo(lowerBound.ToUInt64() + count.ToUInt64());
		}
	}

	// If we still don't have a base type yet, the base type is supposed to be
	// the a signed integer type with the same size as an address for that
	// compilation unit.
	if (baseTypeEntry == NULL) {
		// TODO: Implement!
		WARNING("Base type fallback for subrange type not implemented yet!\n");
		return B_UNSUPPORTED;
	}

	// create the base type
	DwarfType* baseType;
	status_t error = _CreateType(baseTypeEntry, baseType);
	if (error != B_OK)
		return error;
	Reference<Type> baseTypeReference(baseType, true);

	// TODO: Support the thread scaling attribute!

	// create the type
	DwarfSubrangeType* type = new(std::nothrow) DwarfSubrangeType(name,
		typeEntry, baseType, lowerBound, upperBound);
	if (type == NULL)
		return B_NO_MEMORY;

	_type = type;
	return B_OK;
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
	status_t error = _CreateType(typeEntry, type);
	if (error != B_OK)
		return error;
	Reference<DwarfType> typeReference(type, true);

	// get the location, if possible
	ValueLocation* location = new(std::nothrow) ValueLocation(
		fArchitecture->IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	Reference<ValueLocation> locationReference(location, true);

	if (locationDescription->IsValid()) {
		status_t error = _ResolveLocation(locationDescription, 0, type,
			*location);
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


status_t
DwarfStackFrameDebugInfo::_ResolveTypedef(DIETypedef* entry,
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
DwarfStackFrameDebugInfo::_ResolveTypeByteSize(DIEType* typeEntry,
	uint64& _size)
{
	TRACE_LOCALS("DwarfStackFrameDebugInfo::_ResolveTypeByteSize(%p)\n",
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
				_size = fCompilationUnit->AddressSize();

				TRACE_LOCALS("  pointer/reference type: size: %llu\n", _size);

				return B_OK;
			default:
				return B_ENTRY_NOT_FOUND;
		}
	}

	TRACE_LOCALS("  found attribute\n");

	// get the actual value
	BVariant size;
	status_t error = fFile->EvaluateDynamicValue(fCompilationUnit,
		fSubprogramEntry, sizeValue, fTargetInterface, fInstructionPointer,
		fFramePointer, size);
	if (error != B_OK) {
		TRACE_LOCALS("  failed to resolve attribute: %s\n", strerror(error));
		return error;
	}

	_size = size.ToUInt64();

	TRACE_LOCALS("  -> size: %llu\n", _size);

	return B_OK;
}


status_t
DwarfStackFrameDebugInfo::_ResolveLocation(
	const LocationDescription* description, target_addr_t objectAddress,
	Type* type, ValueLocation& _location)
{
	status_t error = fFile->ResolveLocation(fCompilationUnit,
		fSubprogramEntry, description, fTargetInterface,
		fInstructionPointer, objectAddress, fFramePointer, _location);
	if (error != B_OK)
		return error;

	// translate the DWARF register indices and the bit offset/size semantics
	const Register* registers = fArchitecture->Registers();
	bool bigEndian = fArchitecture->IsBigEndian();
	int32 count = _location.CountPieces();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = _location.PieceAt(i);
		if (piece.type == VALUE_PIECE_LOCATION_REGISTER) {
			int32 reg = fFromDwarfRegisterMap->MapRegisterIndex(piece.reg);
			if (reg >= 0) {
				piece.reg = reg;
				// The bit offset for registers is to the least
				// significant bit, while we want the offset to the most
				// significant bit.
				if (registers[reg].BitSize() > piece.bitSize) {
					piece.bitOffset = registers[reg].BitSize() - piece.bitSize
						- piece.bitOffset;
				}
			} else
				piece.SetToUnknown();
		} else if (piece.type == VALUE_PIECE_LOCATION_MEMORY) {
			// Whether the bit offset is to the least or most significant bit
			// is target architecture and source language specific.
			// TODO: Check whether this is correct!
			// TODO: Source language!
			if (!bigEndian && piece.size * 8 > piece.bitSize) {
				piece.bitOffset = piece.size * 8 - piece.bitSize
					- piece.bitOffset;
			}
		}

		piece.Normalize(bigEndian);
		_location.SetPieceAt(i, piece);
	}

	// If we only have one piece and that doesn't have a size, try to retrieve
	// the size of the type.
	if (count == 1) {
		ValuePieceLocation piece = _location.PieceAt(0);
		if (piece.IsValid() && piece.size == 0 && piece.bitSize == 0) {
			piece.SetSize(type->ByteSize());
				// TODO: Use bit size and bit offset, if specified!
			_location.SetPieceAt(0, piece);

			TRACE_LOCALS("  set single piece size to %llu\n", type->ByteSize());
		}
	}

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
