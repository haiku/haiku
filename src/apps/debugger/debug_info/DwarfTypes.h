/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_TYPES_H
#define DWARF_TYPES_H


#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>

#include "Type.h"


class Architecture;
class CompilationUnit;
class DIEAddressingType;
class DIEArrayType;
class DIEBaseType;
class DIECompoundType;
class DIEEnumerationType;
class DIEEnumerator;
class DIEFormalParameter;
class DIEInheritance;
class DIEMember;
class DIEModifiedType;
class DIEPointerToMemberType;
class DIESubprogram;
class DIESubrangeType;
class DIESubroutineType;
class DIEType;
class DIETypedef;
class DIEUnspecifiedType;
class DwarfFile;
class DwarfTargetInterface;
struct LocationDescription;
struct MemberLocation;
class RegisterMap;
class ValueLocation;


// conversion functions between model types and dwarf types
type_kind dwarf_tag_to_type_kind(int32 tag);
int32 dwarf_tag_to_subtype_kind(int32 tag);


class DwarfTypeContext : public BReferenceable {
public:
								DwarfTypeContext(Architecture* architecture,
									image_id imageID, DwarfFile* file,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									target_addr_t relocationDelta,
									DwarfTargetInterface* targetInterface,
									RegisterMap* fromDwarfRegisterMap);
								~DwarfTypeContext();

			Architecture*		GetArchitecture() const
									{ return fArchitecture; }
			image_id			ImageID() const
									{ return fImageID; }
			DwarfFile*			File() const
									{ return fFile; }
			CompilationUnit*	GetCompilationUnit() const
									{ return fCompilationUnit; }
			DIESubprogram*		SubprogramEntry() const
									{ return fSubprogramEntry; }
			target_addr_t		InstructionPointer() const
									{ return fInstructionPointer; }
			target_addr_t		FramePointer() const
									{ return fFramePointer; }
			target_addr_t		RelocationDelta() const
									{ return fRelocationDelta; }
			DwarfTargetInterface* TargetInterface() const
									{ return fTargetInterface; }
			RegisterMap*		FromDwarfRegisterMap() const
									{ return fFromDwarfRegisterMap; }

private:
			Architecture*		fArchitecture;
			image_id			fImageID;
			DwarfFile*			fFile;
			CompilationUnit*	fCompilationUnit;
			DIESubprogram*		fSubprogramEntry;
			target_addr_t		fInstructionPointer;
			target_addr_t		fFramePointer;
			target_addr_t		fRelocationDelta;
			DwarfTargetInterface* fTargetInterface;
			RegisterMap*		fFromDwarfRegisterMap;
};


class DwarfType : public virtual Type {
public:
								DwarfType(DwarfTypeContext* typeContext,
									const BString& name, const DIEType* entry);
								~DwarfType();

	static	bool				GetTypeID(const DIEType* entry, BString& _id);

	virtual	image_id			ImageID() const;
	virtual	const BString&		ID() const;
	virtual	const BString&		Name() const;
	virtual	target_size_t		ByteSize() const;

	virtual	status_t			ResolveObjectDataLocation(
									const ValueLocation& objectLocation,
									ValueLocation*& _location);
	virtual	status_t			ResolveObjectDataLocation(
									target_addr_t objectAddress,
									ValueLocation*& _location);

			DwarfTypeContext*	TypeContext() const
									{ return fTypeContext; }

			void				SetByteSize(target_size_t size)
									{ fByteSize = size; }

	virtual	DIEType*			GetDIEType() const = 0;

			status_t			ResolveLocation(DwarfTypeContext* typeContext,
									const LocationDescription* description,
									target_addr_t objectAddress,
									bool hasObjectAddress,
									ValueLocation& _location);

private:
			DwarfTypeContext*	fTypeContext;
			BString				fName;
			BString				fID;
			target_size_t		fByteSize;
};


class DwarfInheritance : public BaseType {
public:
								DwarfInheritance(DIEInheritance* entry,
									DwarfType* type);
								~DwarfInheritance();

	virtual	Type*				GetType() const;

			DwarfType*			GetDwarfType() const
									{ return fType; }
			DIEInheritance*		Entry() const
									{ return fEntry; }

private:
			DIEInheritance*		fEntry;
			DwarfType*			fType;
};


class DwarfDataMember : public DataMember {
public:
								DwarfDataMember(DIEMember* entry,
									const BString& name, DwarfType* type);
								~DwarfDataMember();

	virtual	const char*			Name() const;
	virtual	Type*				GetType() const;

			DwarfType*			GetDwarfType() const
									{ return fType; }
			DIEMember*			Entry() const
									{ return fEntry; }

private:
			DIEMember*			fEntry;
			BString				fName;
			DwarfType*			fType;
};


class DwarfEnumeratorValue : public EnumeratorValue {
public:
								DwarfEnumeratorValue(DIEEnumerator* entry,
									const BString& name, const BVariant& value);
								~DwarfEnumeratorValue();

	virtual	const char*			Name() const;
	virtual	BVariant			Value() const;

			DIEEnumerator*		Entry() const
									{ return fEntry; }

private:
			DIEEnumerator*		fEntry;
			BString				fName;
			BVariant			fValue;
};


class DwarfArrayDimension : public ArrayDimension {
public:
								DwarfArrayDimension(DwarfType* type);
								~DwarfArrayDimension();

	virtual	Type*				GetType() const;

			DwarfType*			GetDwarfType() const
									{ return fType; }

private:
			DwarfType*			fType;

};


class DwarfFunctionParameter : public FunctionParameter {
public:
								DwarfFunctionParameter(
									DIEFormalParameter* entry,
									const BString& name, DwarfType* type);
								~DwarfFunctionParameter();

	virtual	const char*			Name() const;
	virtual	Type*				GetType() const;

			DIEFormalParameter*	Entry() const
									{ return fEntry; }

private:
			DIEFormalParameter*	fEntry;
			BString				fName;
			DwarfType*			fType;

};


class DwarfPrimitiveType : public PrimitiveType, public DwarfType {
public:
								DwarfPrimitiveType(
									DwarfTypeContext* typeContext,
									const BString& name, DIEBaseType* entry,
									uint32 typeConstant);

	virtual	DIEType*			GetDIEType() const;
	virtual	uint32				TypeConstant() const;

			DIEBaseType*		Entry() const
									{ return fEntry; }

private:
			DIEBaseType*		fEntry;
			uint32				fTypeConstant;
};


class DwarfCompoundType : public CompoundType, public DwarfType {
public:
								DwarfCompoundType(DwarfTypeContext* typeContext,
									const BString& name, DIECompoundType* entry,
									compound_type_kind compoundKind);
								~DwarfCompoundType();

	virtual	compound_type_kind	CompoundKind() const;

	virtual	int32				CountBaseTypes() const;
	virtual	BaseType*			BaseTypeAt(int32 index) const;

	virtual	int32				CountDataMembers() const;
	virtual	DataMember*			DataMemberAt(int32 index) const;

	virtual	status_t			ResolveBaseTypeLocation(BaseType* _baseType,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
	virtual	status_t			ResolveDataMemberLocation(DataMember* _member,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);

	virtual	DIEType*			GetDIEType() const;

			DIECompoundType*	Entry() const
									{ return fEntry; }

			bool				AddInheritance(DwarfInheritance* inheritance);
			bool				AddDataMember(DwarfDataMember* member);

private:
			typedef BObjectList<DwarfDataMember> DataMemberList;
			typedef BObjectList<DwarfInheritance> InheritanceList;

private:
			status_t			_ResolveDataMemberLocation(
									DwarfType* memberType,
									const MemberLocation* memberLocation,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);

private:
			compound_type_kind	fCompoundKind;
			DIECompoundType*	fEntry;
			InheritanceList		fInheritances;
			DataMemberList		fDataMembers;
};


class DwarfArrayType : public ArrayType, public DwarfType {
public:
								DwarfArrayType(DwarfTypeContext* typeContext,
									const BString& name, DIEArrayType* entry,
									DwarfType* baseType);
								~DwarfArrayType();

	virtual	Type*				BaseType() const;

	virtual	int32				CountDimensions() const;
	virtual	ArrayDimension*		DimensionAt(int32 index) const;

	virtual	status_t			ResolveElementLocation(
									const ArrayIndexPath& indexPath,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);

	virtual	DIEType*			GetDIEType() const;

			bool				AddDimension(DwarfArrayDimension* dimension);

			DwarfArrayDimension* DwarfDimensionAt(int32 index) const
									{ return fDimensions.ItemAt(index); }
			DIEArrayType*		Entry() const
									{ return fEntry; }

private:
			typedef BObjectList<DwarfArrayDimension> DimensionList;

private:
			DIEArrayType*		fEntry;
			DwarfType*			fBaseType;
			DimensionList		fDimensions;
};


class DwarfModifiedType : public ModifiedType, public DwarfType {
public:
								DwarfModifiedType(DwarfTypeContext* typeContext,
									const BString& name, DIEModifiedType* entry,
									uint32 modifiers, DwarfType* baseType);
								~DwarfModifiedType();

	virtual	uint32				Modifiers() const;
	virtual	Type*				BaseType() const;

	virtual	DIEType*			GetDIEType() const;

			DIEModifiedType*	Entry() const
									{ return fEntry; }

private:
			DIEModifiedType*	fEntry;
			uint32				fModifiers;
			DwarfType*			fBaseType;
};


class DwarfTypedefType : public TypedefType, public DwarfType {
public:
								DwarfTypedefType(DwarfTypeContext* typeContext,
									const BString& name, DIETypedef* entry,
									DwarfType* baseType);
								~DwarfTypedefType();

	virtual	Type*				BaseType() const;

	virtual	DIEType*			GetDIEType() const;

			DIETypedef*			Entry() const
									{ return fEntry; }

private:
			DIETypedef*			fEntry;
			DwarfType*			fBaseType;
};


class DwarfAddressType : public AddressType, public DwarfType {
public:
								DwarfAddressType(DwarfTypeContext* typeContext,
									const BString& name,
									DIEAddressingType* entry,
									address_type_kind addressKind,
									DwarfType* baseType);
								~DwarfAddressType();

	virtual	address_type_kind	AddressKind() const;
	virtual	Type*				BaseType() const;

	virtual	DIEType*			GetDIEType() const;

			DIEAddressingType*	Entry() const
									{ return fEntry; }

private:
			DIEAddressingType*	fEntry;
			address_type_kind	fAddressKind;
			DwarfType*			fBaseType;
};


class DwarfEnumerationType : public EnumerationType, public DwarfType {
public:
								DwarfEnumerationType(
									DwarfTypeContext* typeContext,
									const BString& name,
									DIEEnumerationType* entry,
									DwarfType* baseType);
								~DwarfEnumerationType();

	virtual	Type*				BaseType() const;

	virtual	int32				CountValues() const;
	virtual	EnumeratorValue*	ValueAt(int32 index) const;

	virtual	DIEType*			GetDIEType() const;

			bool				AddValue(DwarfEnumeratorValue* value);

			DIEEnumerationType*	Entry() const
									{ return fEntry; }

private:
			typedef BObjectList<DwarfEnumeratorValue> ValueList;

private:
			DIEEnumerationType*	fEntry;
			DwarfType*			fBaseType;
			ValueList			fValues;
};


class DwarfSubrangeType : public SubrangeType, public DwarfType {
public:
								DwarfSubrangeType(DwarfTypeContext* typeContext,
									const BString& name, DIESubrangeType* entry,
									Type* baseType,
									const BVariant& lowerBound,
									const BVariant& upperBound);
								~DwarfSubrangeType();

	virtual	Type*				BaseType() const;

	virtual	BVariant			LowerBound() const;
	virtual	BVariant			UpperBound() const;

	virtual	DIEType*			GetDIEType() const;

			DIESubrangeType*	Entry() const
									{ return fEntry; }

private:
			DIESubrangeType*	fEntry;
			Type*				fBaseType;
			BVariant			fLowerBound;
			BVariant			fUpperBound;
};


struct DwarfUnspecifiedType : public UnspecifiedType, public DwarfType {
public:
								DwarfUnspecifiedType(
									DwarfTypeContext* typeContext,
									const BString& name,
									DIEUnspecifiedType* entry);
									// entry may be NULL
								~DwarfUnspecifiedType();

	virtual	DIEType*			GetDIEType() const;

			DIEUnspecifiedType*	Entry() const
									{ return fEntry; }

private:
			DIEUnspecifiedType*	fEntry;
};


class DwarfFunctionType : public FunctionType, public DwarfType {
public:
								DwarfFunctionType(DwarfTypeContext* typeContext,
									const BString& name,
									DIESubroutineType* entry,
									DwarfType* returnType);
								~DwarfFunctionType();

	virtual	Type*				ReturnType() const;

	virtual	int32				CountParameters() const;
	virtual	FunctionParameter*	ParameterAt(int32 index) const;

	virtual	bool				HasVariableArguments() const;
			void				SetHasVariableArguments(bool hasVarArgs);

	virtual	DIEType*			GetDIEType() const;

			bool				AddParameter(DwarfFunctionParameter* parameter);

			DwarfFunctionParameter* DwarfParameterAt(int32 index) const
									{ return fParameters.ItemAt(index); }

			DIESubroutineType*	Entry() const
									{ return fEntry; }

private:
			typedef BObjectList<DwarfFunctionParameter> ParameterList;

private:
			DIESubroutineType*	fEntry;
			DwarfType*			fReturnType;
			ParameterList		fParameters;
			bool				fHasVariableArguments;
};


class DwarfPointerToMemberType : public PointerToMemberType, public DwarfType {
public:
								DwarfPointerToMemberType(
									DwarfTypeContext* typeContext,
									const BString& name,
									DIEPointerToMemberType* entry,
									DwarfCompoundType* containingType,
									DwarfType* baseType);
								~DwarfPointerToMemberType();

	virtual	CompoundType*		ContainingType() const;
	virtual	Type*				BaseType() const;

	virtual	DIEType*			GetDIEType() const;

			DIEPointerToMemberType* Entry() const
									{ return fEntry; }

private:
			DIEPointerToMemberType* fEntry;
			DwarfCompoundType*	fContainingType;
			DwarfType*			fBaseType;
};


#endif	// DWARF_TYPES_H
