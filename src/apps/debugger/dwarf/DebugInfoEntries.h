/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_INFO_ENTRIES_H
#define DEBUG_INFO_ENTRIES_H


#include "DebugInfoEntry.h"

#include "AttributeValue.h"


// common:
// DW_AT_name
// // not supported by all types:
// DW_AT_allocated
// DW_AT_associated
// DW_AT_data_location
// DW_AT_start_scope

// modified:
// DW_AT_type

// declared:
// DECL
// DW_AT_accessibility		// !file, !pointer to member
// DW_AT_declaration		// !file
// DW_AT_abstract_origin	// !interface
// DW_AT_description		// !interface
// DW_AT_visibility			// !interface

// derived: declared
// DW_AT_type

// addressing: modified
// DW_AT_address_class

// compound: declared
// DW_AT_byte_size			// !interface
// DW_AT_specification		// !interface

// class base: compound

// array index: derived
// DW_AT_bit_stride
// DW_AT_byte_stride
// DW_AT_byte_size


// unspecified: common
// DECL
// DW_AT_description



// class/structure: class base

// interface: class base

// union: compound

// string: declared
// DW_AT_byte_size
// DW_AT_string_length

// subroutine: declared
// DW_AT_address_class
// DW_AT_prototyped
// DW_AT_type


// enumeration: array index
// DW_AT_specification

// pointer to member: derived
// DW_AT_address_class
// DW_AT_containing_type
// DW_AT_use_location

// set: derived
// DW_AT_byte_size

// subrange: array index
// DW_AT_count
// DW_AT_lower_bound
// DW_AT_threads_scaled
// DW_AT_upper_bound

// array: derived
// DW_AT_bit_stride
// DW_AT_byte_size
// DW_AT_ordering
// DW_AT_specification

// typedef: derived

// file: derived
// DW_AT_byte_size


// shared: modified
// DECL
// DW_AT_count

// const: modified

// packed: modified

// volatile: modified
// DECL

// restrict: modified

// pointer: addressing
// DW_AT_specification

// reference: addressing


// basetype:
// DW_AT_binary_scale
// DW_AT_bit_offset
// DW_AT_bit_size
// DW_AT_byte_size
// DW_AT_decimal_scale
// DW_AT_decimal_sign
// DW_AT_description
// DW_AT_digit_count
// DW_AT_encoding
// DW_AT_endianity
// DW_AT_picture_string
// DW_AT_small


class DIECompileUnitBase : public DebugInfoEntry {
public:
								DIECompileUnitBase();
								~DIECompileUnitBase();

	virtual	status_t			InitAfterAttributes(
									DebugInfoEntryInitInfo& info);

	virtual	const char*			Name() const;

			const char*			CompilationDir() const
									{ return fCompilationDir; }

			uint16				Language() const	{ return fLanguage; }

			const DebugInfoEntryList& Types() const	{ return fTypes; }
			const DebugInfoEntryList& OtherChildren() const
										{ return fOtherChildren; }
			off_t				AddressRangesOffset() const
										{ return fAddressRangesOffset; }

			target_addr_t		LowPC() const	{ return fLowPC; }
			target_addr_t		HighPC() const	{ return fHighPC; }

			off_t				StatementListOffset() const
									{ return fStatementListOffset; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_name(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_comp_dir(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_low_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_high_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_producer(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_stmt_list(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_macro_info(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_base_types(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_language(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_identifier_case(
									uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_use_UTF8(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_ranges(uint16 attributeName,
									const AttributeValue& value);

//TODO:
//	virtual	status_t			AddAttribute_segment(uint16 attributeName,
//									const AttributeValue& value);

// TODO: DW_AT_import

protected:
			const char*			fName;
			const char*			fCompilationDir;
			target_addr_t		fLowPC;
			target_addr_t		fHighPC;
			off_t				fStatementListOffset;
			off_t				fMacroInfoOffset;
			off_t				fAddressRangesOffset;
			DIECompileUnitBase*	fBaseTypesUnit;
			DebugInfoEntryList	fTypes;
			DebugInfoEntryList	fOtherChildren;
			uint16				fLanguage;
			uint8				fIdentifierCase;
			bool				fUseUTF8;
};


class DIEType : public DebugInfoEntry {
public:
								DIEType();

	virtual	bool				IsType() const;

	virtual	const char*			Name() const;

	virtual	bool				IsDeclaration() const;
	virtual	const DynamicAttributeValue* ByteSize() const;

	virtual	status_t			AddAttribute_name(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_allocated(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_associated(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_data_location
// DW_AT_start_scope

protected:
			const char*			fName;
			DynamicAttributeValue fAllocated;
			DynamicAttributeValue fAssociated;
};


class DIEModifiedType : public DIEType {
public:
								DIEModifiedType();

			DIEType*			GetType() const	{ return fType; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

protected:
			DIEType*			fType;
};


class DIEAddressingType : public DIEModifiedType {
public:
								DIEAddressingType();

	virtual	status_t			AddAttribute_address_class(uint16 attributeName,
									const AttributeValue& value);

protected:
			uint8				fAddressClass;
};


class DIEDeclaredType : public DIEType {
public:
								DIEDeclaredType();

	virtual	const char*			Description() const;
	virtual	DebugInfoEntry*		AbstractOrigin() const;

	virtual	bool				IsDeclaration() const;

	virtual	status_t			AddAttribute_accessibility(uint16 attributeName,
									const AttributeValue& value);
										// TODO: !file, !pointer to member
	virtual	status_t			AddAttribute_declaration(uint16 attributeName,
									const AttributeValue& value);
										// TODO: !file
	virtual	status_t			AddAttribute_description(uint16 attributeName,
									const AttributeValue& value);
										// TODO: !interface
	virtual	status_t			AddAttribute_abstract_origin(
									uint16 attributeName,
									const AttributeValue& value);
										// TODO: !interface

// TODO:
// DW_AT_visibility			// !interface

protected:
	virtual	DeclarationLocation* GetDeclarationLocation();

protected:
			const char*			fDescription;
			DeclarationLocation	fDeclarationLocation;
			DebugInfoEntry*		fAbstractOrigin;
			uint8				fAccessibility;
			bool				fDeclaration;
};


class DIEDerivedType : public DIEDeclaredType {
public:
								DIEDerivedType();

			DIEType*			GetType() const	{ return fType; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

protected:
			DIEType*			fType;
};


class DIECompoundType : public DIEDeclaredType {
public:
								DIECompoundType();

	virtual	bool				IsNamespace() const;

	virtual	DebugInfoEntry*		Specification() const;

	virtual	const DynamicAttributeValue* ByteSize() const;

			const DebugInfoEntryList& DataMembers() const
									{ return fDataMembers; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);
										// TODO: !interface
	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);
										// TODO: !interface

protected:
			DynamicAttributeValue fByteSize;
			DIECompoundType*	fSpecification;
			DebugInfoEntryList	fDataMembers;
};


class DIEClassBaseType : public DIECompoundType {
public:
								DIEClassBaseType();

			const DebugInfoEntryList& BaseTypes() const
									{ return fBaseTypes; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

protected:
			DebugInfoEntryList	fBaseTypes;
			DebugInfoEntryList	fFriends;
			DebugInfoEntryList	fAccessDeclarations;
			DebugInfoEntryList	fMemberFunctions;
			DebugInfoEntryList	fInnerTypes;
};


class DIENamedBase : public DebugInfoEntry {
public:
								DIENamedBase();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

	virtual	status_t			AddAttribute_name(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_description(uint16 attributeName,
									const AttributeValue& value);

protected:
			const char*			fName;
			const char*			fDescription;
};


class DIEDeclaredBase : public DebugInfoEntry {
public:
								DIEDeclaredBase();

protected:
	virtual	DeclarationLocation* GetDeclarationLocation();

protected:
			DeclarationLocation	fDeclarationLocation;
};


class DIEDeclaredNamedBase : public DIEDeclaredBase {
public:
								DIEDeclaredNamedBase();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

			uint8				Accessibility() const { return fAccessibility; }
			uint8				Visibility() const	{ return fVisibility; }
	virtual	bool				IsDeclaration() const;

	virtual	status_t			AddAttribute_name(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_description(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_accessibility(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_declaration(uint16 attributeName,
									const AttributeValue& value);

protected:
			const char*			fName;
			const char*			fDescription;
			uint8				fAccessibility;
			uint8				fVisibility;
			bool				fDeclaration;
};


class DIEArrayIndexType : public DIEDerivedType {
public:
								DIEArrayIndexType();

	virtual	const DynamicAttributeValue* ByteSize() const;

			const DynamicAttributeValue* BitStride() const
									{ return &fBitStride; }
			const DynamicAttributeValue* ByteStride() const
									{ return &fByteStride; }

	virtual	status_t			AddAttribute_bit_stride(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_stride(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fBitStride;
			DynamicAttributeValue fByteSize;
			DynamicAttributeValue fByteStride;
};


// #pragma mark -


class DIEArrayType : public DIEDerivedType {
public:
								DIEArrayType();

	virtual	uint16				Tag() const;

	virtual	status_t			InitAfterHierarchy(
									DebugInfoEntryInitInfo& info);

	virtual	DebugInfoEntry*		Specification() const;

	virtual	const DynamicAttributeValue* ByteSize() const;

			const DynamicAttributeValue* BitStride() const
									{ return &fBitStride; }

			const DebugInfoEntryList& Dimensions() const
									{ return fDimensions; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_ordering(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_stride(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_stride_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fBitStride;
			DynamicAttributeValue fByteSize;
			DebugInfoEntryList	fDimensions;
			DIEArrayType*		fSpecification;
			uint8				fOrdering;
};


class DIEClassType : public DIEClassBaseType {
public:
								DIEClassType();

	virtual	uint16				Tag() const;
};


class DIEEntryPoint : public DebugInfoEntry {
public:
// TODO: Maybe introduce a common base class for DIEEntryPoint and
// DIESubprogram.
								DIEEntryPoint();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_address_class
// DW_AT_description
// DW_AT_frame_base
// DW_AT_low_pc
// DW_AT_name
// DW_AT_return_addr
// DW_AT_segment
// DW_AT_static_link
// DW_AT_type
};


class DIEEnumerationType : public DIEArrayIndexType {
public:
								DIEEnumerationType();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		Specification() const;

			const DebugInfoEntryList& Enumerators() const
									{ return fEnumerators; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);

private:
			DIEEnumerationType*	fSpecification;
			DebugInfoEntryList	fEnumerators;
};


class DIEFormalParameter : public DIEDeclaredNamedBase {
public:
								DIEFormalParameter();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		AbstractOrigin() const;
	virtual	LocationDescription* GetLocationDescription();

			DIEType*			GetType() const	{ return fType; }

			const ConstantAttributeValue* ConstValue() const
									{ return &fValue; }

	virtual	status_t			AddAttribute_abstract_origin(
									uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_const_value(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_artificial
// DW_AT_default_value
// DW_AT_endianity
// DW_AT_is_optional
// DW_AT_segment
// DW_AT_variable_parameter

private:
			LocationDescription	fLocationDescription;
			DebugInfoEntry*		fAbstractOrigin;
			DIEType*			fType;
			ConstantAttributeValue fValue;
};


class DIEImportedDeclaration : public DIEDeclaredNamedBase {
public:
								DIEImportedDeclaration();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_import
// DW_AT_start_scope
};


class DIELabel : public DIEDeclaredNamedBase {
public:
								DIELabel();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_low_pc
// DW_AT_segment
// DW_AT_start_scope
};


class DIELexicalBlock : public DIENamedBase {
public:
								DIELexicalBlock();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		AbstractOrigin() const;

			off_t				AddressRangesOffset() const
										{ return fAddressRangesOffset; }

			target_addr_t		LowPC() const	{ return fLowPC; }
			target_addr_t		HighPC() const	{ return fHighPC; }

			const DebugInfoEntryList& Variables() const	{ return fVariables; }
			const DebugInfoEntryList& Blocks() const	{ return fBlocks; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_low_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_high_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_ranges(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_abstract_origin(
									uint16 attributeName,
									const AttributeValue& value);

protected:
			DebugInfoEntryList	fVariables;
			DebugInfoEntryList	fBlocks;
			target_addr_t		fLowPC;
			target_addr_t		fHighPC;
			off_t				fAddressRangesOffset;
			DIELexicalBlock*	fAbstractOrigin;

// TODO:
// DW_AT_segment
};


class DIEMember : public DIEDeclaredNamedBase {
public:
								DIEMember();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }
			const DynamicAttributeValue* ByteSize() const
									{ return &fByteSize; }
			const DynamicAttributeValue* BitOffset() const
									{ return &fBitOffset; }
			const DynamicAttributeValue* BitSize() const
									{ return &fBitSize; }
			const MemberLocation* Location() const
									{ return &fLocation; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_offset(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_data_member_location(
									uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_mutable

private:
			DIEType*			fType;
			DynamicAttributeValue fByteSize;
			DynamicAttributeValue fBitOffset;
			DynamicAttributeValue fBitSize;
			MemberLocation		fLocation;
};


class DIEPointerType : public DIEAddressingType {
public:
								DIEPointerType();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		Specification() const;

	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);

private:
			DIEPointerType*		fSpecification;
};


class DIEReferenceType : public DIEAddressingType {
public:
								DIEReferenceType();

	virtual	uint16				Tag() const;
};


class DIECompileUnit : public DIECompileUnitBase {
public:
								DIECompileUnit();

	virtual	uint16				Tag() const;
};


class DIEStringType : public DIEDeclaredType {
public:
								DIEStringType();

	virtual	uint16				Tag() const;

	virtual	const DynamicAttributeValue* ByteSize() const;

	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fByteSize;
// TODO:
// DW_AT_string_length
};


class DIEStructureType : public DIEClassBaseType {
public:
								DIEStructureType();

	virtual	uint16				Tag() const;
};


class DIESubroutineType : public DIEDeclaredType {
public:
								DIESubroutineType();

	virtual	uint16				Tag() const;

			DIEType*			ReturnType() const	{ return fReturnType; }

			const DebugInfoEntryList& Parameters() const { return fParameters; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_address_class(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_prototyped(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

protected:
			DebugInfoEntryList	fParameters;
			DIEType*			fReturnType;
			uint8				fAddressClass;
			bool				fPrototyped;
};


class DIETypedef : public DIEDerivedType {
public:
								DIETypedef();

	virtual	uint16				Tag() const;
};


class DIEUnionType : public DIECompoundType {
public:
								DIEUnionType();

	virtual	uint16				Tag() const;
};


class DIEUnspecifiedParameters : public DIEDeclaredBase {
public:
								DIEUnspecifiedParameters();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_artificial
};


class DIEVariant : public DIEDeclaredBase {
public:
								DIEVariant();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_accessibility
// DW_AT_abstract_origin
// DW_AT_declaration
// DW_AT_discr_list
// DW_AT_discr_value
};


class DIECommonBlock : public DIEDeclaredNamedBase {
public:
								DIECommonBlock();

	virtual	uint16				Tag() const;

	virtual	LocationDescription* GetLocationDescription();

// TODO:
// DW_AT_segment

private:
			LocationDescription	fLocationDescription;
};


class DIECommonInclusion : public DIEDeclaredBase {
public:
								DIECommonInclusion();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_common_reference
// DW_AT_declaration
// DW_AT_visibility

};


class DIEInheritance : public DIEDeclaredBase {
public:
								DIEInheritance();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }
			const MemberLocation* Location() const
									{ return &fLocation; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_data_member_location(
									uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_accessibility
// DW_AT_virtuality

private:
			DIEType*			fType;
			MemberLocation		fLocation;
};


class DIEInlinedSubroutine : public DebugInfoEntry {
public:
								DIEInlinedSubroutine();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_call_column
// DW_AT_call_file
// DW_AT_call_line
// DW_AT_entry_pc
// DW_AT_high_pc
// DW_AT_low_pc
// DW_AT_ranges
// DW_AT_return_addr
// DW_AT_segment
// DW_AT_start_scope
// DW_AT_trampoline
};


class DIEModule : public DIEDeclaredNamedBase {
public:
								DIEModule();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_entry_pc
// DW_AT_high_pc
// DW_AT_low_pc
// DW_AT_priority
// DW_AT_ranges
// DW_AT_segment
// DW_AT_specification
};


class DIEPointerToMemberType : public DIEDerivedType {
public:
								DIEPointerToMemberType();

	virtual	uint16				Tag() const;

			DIECompoundType*	ContainingType() const
									{ return fContainingType; }

			const LocationDescription& UseLocation() const
									{ return fUseLocation; }

	virtual	status_t			AddAttribute_address_class(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_containing_type(
									uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_use_location(uint16 attributeName,
									const AttributeValue& value);

protected:
			DIECompoundType*	fContainingType;
			LocationDescription	fUseLocation;
			uint8				fAddressClass;
};


class DIESetType : public DIEDerivedType {
public:
								DIESetType();

	virtual	uint16				Tag() const;

	virtual	const DynamicAttributeValue* ByteSize() const;

	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fByteSize;
};


class DIESubrangeType : public DIEArrayIndexType {
public:
								DIESubrangeType();

	virtual	uint16				Tag() const;

			const DynamicAttributeValue* LowerBound() const
									{ return &fLowerBound; }
			const DynamicAttributeValue* UpperBound() const
									{ return &fUpperBound; }
			const DynamicAttributeValue* Count() const
									{ return &fCount; }

	virtual	status_t			AddAttribute_count(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_lower_bound(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_upper_bound(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_threads_scaled(
									uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fCount;
			DynamicAttributeValue fLowerBound;
			DynamicAttributeValue fUpperBound;
			bool				fThreadsScaled;
};


class DIEWithStatement : public DebugInfoEntry {
public:
								DIEWithStatement();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

	virtual	LocationDescription* GetLocationDescription();

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_accessibility
// DW_AT_address_class
// DW_AT_declaration
// DW_AT_high_pc
// DW_AT_low_pc
// DW_AT_ranges
// DW_AT_segment
// DW_AT_visibility

private:
			DIEType*			fType;
			LocationDescription	fLocationDescription;
};


class DIEAccessDeclaration : public DIEDeclaredNamedBase {
public:
								DIEAccessDeclaration();

	virtual	uint16				Tag() const;
};


class DIEBaseType : public DIEType {
public:
								DIEBaseType();

	virtual	uint16				Tag() const;

	virtual	const DynamicAttributeValue* ByteSize() const;
			const DynamicAttributeValue* BitOffset() const
									{ return &fBitOffset; }
			const DynamicAttributeValue* BitSize() const
									{ return &fBitSize; }
			uint8				Encoding() const	{ return fEncoding; }
			uint8				Endianity() const	{ return fEndianity; }

	virtual	status_t			AddAttribute_encoding(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_offset(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_endianity(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_binary_scale
// DW_AT_decimal_scale
// DW_AT_decimal_sign
// DW_AT_description
// DW_AT_digit_count
// DW_AT_picture_string
// DW_AT_small

private:
			DynamicAttributeValue fByteSize;
			DynamicAttributeValue fBitOffset;
			DynamicAttributeValue fBitSize;
			uint8				fEncoding;
			uint8				fEndianity;
};


class DIECatchBlock : public DebugInfoEntry {
public:
								DIECatchBlock();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_high_pc
// DW_AT_low_pc
// DW_AT_ranges
// DW_AT_segment
};


class DIEConstType : public DIEModifiedType {
public:
								DIEConstType();

	virtual	uint16				Tag() const;
};


class DIEConstant : public DIEDeclaredNamedBase {
public:
								DIEConstant();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

			const ConstantAttributeValue* ConstValue() const
									{ return &fValue; }

	virtual	status_t			AddAttribute_const_value(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_endianity
// DW_AT_external
// DW_AT_start_scope

private:
			DIEType*			fType;
			ConstantAttributeValue fValue;
};


class DIEEnumerator : public DIEDeclaredNamedBase {
public:
								DIEEnumerator();

	virtual	uint16				Tag() const;

			const ConstantAttributeValue* ConstValue() const
									{ return &fValue; }

	virtual	status_t			AddAttribute_const_value(uint16 attributeName,
									const AttributeValue& value);

private:
			ConstantAttributeValue fValue;
};


class DIEFileType : public DIEDerivedType {
public:
								DIEFileType();

	virtual	uint16				Tag() const;

	virtual	const DynamicAttributeValue* ByteSize() const;

	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fByteSize;
};


class DIEFriend : public DIEDeclaredBase {
public:
								DIEFriend();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_friend
};


class DIENameList : public DIEDeclaredNamedBase {
public:
								DIENameList();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
};


class DIENameListItem : public DIEDeclaredBase {
public:
								DIENameListItem();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_namelist_item
};


class DIEPackedType : public DIEModifiedType {
public:
								DIEPackedType();

	virtual	uint16				Tag() const;
};


class DIESubprogram : public DIEDeclaredNamedBase {
public:
								DIESubprogram();
								~DIESubprogram();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		Specification() const;
	virtual	DebugInfoEntry*		AbstractOrigin() const;

			off_t				AddressRangesOffset() const
										{ return fAddressRangesOffset; }

			target_addr_t		LowPC() const	{ return fLowPC; }
			target_addr_t		HighPC() const	{ return fHighPC; }

			const LocationDescription* FrameBase() const { return &fFrameBase; }

			const DebugInfoEntryList Parameters() const	{ return fParameters; }
			const DebugInfoEntryList Variables() const	{ return fVariables; }
			const DebugInfoEntryList Blocks() const		{ return fBlocks; }

			bool				IsPrototyped() const	{ return fPrototyped; }
			uint8				Inline() const			{ return fInline; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_low_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_high_pc(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_ranges(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_address_class(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_prototyped(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_inline(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_abstract_origin(
									uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_frame_base(
									uint16 attributeName,
									const AttributeValue& value);

protected:
			DebugInfoEntryList	fParameters;
			DebugInfoEntryList	fVariables;
			DebugInfoEntryList	fBlocks;
			target_addr_t		fLowPC;
			target_addr_t		fHighPC;
			off_t				fAddressRangesOffset;
			DIESubprogram*		fSpecification;
			DIESubprogram*		fAbstractOrigin;
			DIEType*			fReturnType;
			LocationDescription	fFrameBase;
			uint8				fAddressClass;
			bool				fPrototyped;
			uint8				fInline;

// TODO:
// DW_AT_artificial
// DW_AT_calling_convention
// DW_AT_elemental
// DW_AT_entry_pc
// DW_AT_explicit
// DW_AT_external
// DW_AT_object_pointer
// DW_AT_pure
// DW_AT_recursive
// DW_AT_return_addr
// DW_AT_segment
// DW_AT_start_scope
// DW_AT_static_link
// DW_AT_trampoline
// DW_AT_virtuality
// DW_AT_vtable_elem_location
};


class DIETemplateTypeParameter : public DIEDeclaredNamedBase {
public:
								DIETemplateTypeParameter();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

private:
			DIEType*			fType;
};


class DIETemplateValueParameter : public DIEDeclaredNamedBase {
public:
								DIETemplateValueParameter();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

			const ConstantAttributeValue* ConstValue() const
									{ return &fValue; }

	virtual	status_t			AddAttribute_const_value(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

private:
			DIEType*			fType;
			ConstantAttributeValue fValue;
};


class DIEThrownType : public DIEDeclaredBase {
public:
								DIEThrownType();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_allocated
// DW_AT_associated
// DW_AT_data_location

private:
			DIEType*			fType;
};


class DIETryBlock : public DebugInfoEntry {
public:
								DIETryBlock();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_abstract_origin
// DW_AT_high_pc
// DW_AT_low_pc
// DW_AT_ranges
// DW_AT_segment
};


class DIEVariantPart : public DIEDeclaredBase {
public:
								DIEVariantPart();

	virtual	uint16				Tag() const;

			DIEType*			GetType() const	{ return fType; }

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_abstract_origin
// DW_AT_accessibility
// DW_AT_declaration
// DW_AT_discr

private:
			DIEType*			fType;
};


class DIEVariable : public DIEDeclaredNamedBase {
public:
								DIEVariable();

	virtual	uint16				Tag() const;

	virtual	DebugInfoEntry*		Specification() const;
	virtual	DebugInfoEntry*		AbstractOrigin() const;

	virtual	LocationDescription* GetLocationDescription();

			DIEType*			GetType() const	{ return fType; }

			const ConstantAttributeValue* ConstValue() const
									{ return &fValue; }

			uint64				StartScope() const	{ return fStartScope; }

	virtual	status_t			AddAttribute_const_value(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_specification(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_abstract_origin(
									uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_start_scope(
									uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_endianity
// DW_AT_external
// DW_AT_segment

private:
			LocationDescription	fLocationDescription;
			ConstantAttributeValue fValue;
			DIEType*			fType;
			DebugInfoEntry*		fSpecification;
			DIEVariable*		fAbstractOrigin;
			uint64				fStartScope;
};


class DIEVolatileType : public DIEModifiedType {
public:
								DIEVolatileType();

	virtual	uint16				Tag() const;

	virtual	status_t			AddAttribute_decl_file(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_line(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_column(uint16 attributeName,
									const AttributeValue& value);

private:
			DeclarationLocation	fDeclarationLocation;
};


class DIEDwarfProcedure : public DebugInfoEntry {
public:
								DIEDwarfProcedure();

	virtual	uint16				Tag() const;

	virtual	LocationDescription* GetLocationDescription();

private:
			LocationDescription	fLocationDescription;
};


class DIERestrictType : public DIEModifiedType {
public:
								DIERestrictType();

	virtual	uint16				Tag() const;
};


class DIEInterfaceType : public DIEClassBaseType {
public:
								DIEInterfaceType();

	virtual	uint16				Tag() const;
};


class DIENamespace : public DIEDeclaredNamedBase {
public:
								DIENamespace();

	virtual	uint16				Tag() const;

	virtual	bool				IsNamespace() const;

			const DebugInfoEntryList& Children() const
										{ return fChildren; }

	virtual	status_t			AddChild(DebugInfoEntry* child);

private:
			DebugInfoEntryList	fChildren;

// TODO:
// DW_AT_extension
// DW_AT_start_scope
};


class DIEImportedModule : public DIEDeclaredBase {
public:
								DIEImportedModule();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_import
// DW_AT_start_scope
};


class DIEUnspecifiedType : public DIEType {
public:
								DIEUnspecifiedType();

	virtual	uint16				Tag() const;

	virtual	status_t			AddAttribute_decl_file(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_line(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_column(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_description

private:
			DeclarationLocation	fDeclarationLocation;
};


class DIEPartialUnit : public DIECompileUnitBase {
public:
								DIEPartialUnit();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_description
};


class DIEImportedUnit : public DebugInfoEntry {
public:
								DIEImportedUnit();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_import
};


class DIECondition : public DIEDeclaredNamedBase {
public:
								DIECondition();

	virtual	uint16				Tag() const;
};


class DIESharedType : public DIEModifiedType {
public:
								DIESharedType();

	virtual	uint16				Tag() const;

	virtual	status_t			AddAttribute_count(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_file(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_line(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_column(uint16 attributeName,
									const AttributeValue& value);

private:
			DynamicAttributeValue fBlockSize;
			DeclarationLocation	fDeclarationLocation;
};


// #pragma mark - DebugInfoEntryFactory


class DebugInfoEntryFactory {
public:
								DebugInfoEntryFactory();

			status_t			CreateDebugInfoEntry(uint16 tag,
									DebugInfoEntry*& entry);
};


#endif	// DEBUG_INFO_ENTRIES_H
