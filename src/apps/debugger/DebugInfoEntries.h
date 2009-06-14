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


// enumeration: derived
// DW_AT_bit_stride
// DW_AT_byte_size
// DW_AT_byte_stride
// DW_AT_specification

// pointer to member: derived
// DW_AT_address_class
// DW_AT_containing_type
// DW_AT_use_location

// set: derived
// DW_AT_byte_size

// subrange: derived
// DW_AT_bit_stride
// DW_AT_byte_size
// DW_AT_byte_stride
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

	virtual	status_t			InitAfterAttributes(
									DebugInfoEntryInitInfo& info);

	virtual	const char*			Name() const;

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

//TODO:
//	virtual	status_t			AddAttribute_ranges(uint16 attributeName,
//									const AttributeValue& value);
//	virtual	status_t			AddAttribute_segment(uint16 attributeName,
//									const AttributeValue& value);

// TODO: DW_AT_import

protected:
			const char*			fName;
			const char*			fCompilationDir;
			dwarf_addr_t		fLowPC;
			dwarf_addr_t		fHighPC;
			dwarf_off_t			fStatementListOffset;
			dwarf_off_t			fMacroInfoOffset;
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
			uint64				fAddressClass;
};


class DIEDeclaredType : public DIEType {
public:
								DIEDeclaredType();

// TODO:
// DECL
// DW_AT_accessibility		// !file, !pointer to member
// DW_AT_declaration		// !file
// DW_AT_abstract_origin	// !interface
// DW_AT_description		// !interface
// DW_AT_visibility			// !interface

protected:
};


class DIEDerivedType : public DIEDeclaredType {
public:
								DIEDerivedType();

	virtual	status_t			AddAttribute_type(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_type

protected:
			DIEType*			fType;
};


class DIECompoundType : public DIEDeclaredType {
public:
								DIECompoundType();

// TODO:
// DW_AT_byte_size			// !interface
// DW_AT_specification		// !interface

protected:
};


class DIEClassBaseType : public DIECompoundType {
public:
								DIEClassBaseType();

protected:
};



// #pragma mark -


class DIEArrayType : public DIEDerivedType {
public:
								DIEArrayType();

	virtual	uint16				Tag() const;

	virtual	status_t			InitAfterHierarchy(
									DebugInfoEntryInitInfo& info);

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_ordering(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_bit_stride(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_stride_size(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_byte_size(uint16 attributeName,
									const AttributeValue& value);

// TODO:
// DW_AT_specification

private:
			uint64				fBitStride;
			dwarf_addr_t		fByteSize;
			uint8				fOrdering;
			DebugInfoEntryList	fDimensions;
};


class DIEClassType : public DIEClassBaseType {
public:
								DIEClassType();

	virtual	uint16				Tag() const;
};


class DIEEntryPoint : public DebugInfoEntry {
public:
								DIEEntryPoint();

	virtual	uint16				Tag() const;
};


class DIEEnumerationType : public DIEDerivedType {
public:
								DIEEnumerationType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_bit_stride
// DW_AT_byte_size
// DW_AT_byte_stride
// DW_AT_specification
};


class DIEFormalParameter : public DebugInfoEntry {
public:
								DIEFormalParameter();

	virtual	uint16				Tag() const;
};


class DIEImportedDeclaration : public DebugInfoEntry {
public:
								DIEImportedDeclaration();

	virtual	uint16				Tag() const;
};


class DIELabel : public DebugInfoEntry {
public:
								DIELabel();

	virtual	uint16				Tag() const;
};


class DIELexicalBlock : public DebugInfoEntry {
public:
								DIELexicalBlock();

	virtual	uint16				Tag() const;
};


class DIEMember : public DebugInfoEntry {
public:
								DIEMember();

	virtual	uint16				Tag() const;
};


class DIEPointerType : public DIEAddressingType {
public:
								DIEPointerType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_specification
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

// TODO:
// DW_AT_byte_size
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

// TODO:
// DW_AT_address_class
// DW_AT_prototyped
// DW_AT_type
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


class DIEUnspecifiedParameters : public DebugInfoEntry {
public:
								DIEUnspecifiedParameters();

	virtual	uint16				Tag() const;
};


class DIEVariant : public DebugInfoEntry {
public:
								DIEVariant();

	virtual	uint16				Tag() const;
};


class DIECommonBlock : public DebugInfoEntry {
public:
								DIECommonBlock();

	virtual	uint16				Tag() const;
};


class DIECommonInclusion : public DebugInfoEntry {
public:
								DIECommonInclusion();

	virtual	uint16				Tag() const;
};


class DIEInheritance : public DebugInfoEntry {
public:
								DIEInheritance();

	virtual	uint16				Tag() const;
};


class DIEInlinedSubroutine : public DebugInfoEntry {
public:
								DIEInlinedSubroutine();

	virtual	uint16				Tag() const;
};


class DIEModule : public DebugInfoEntry {
public:
								DIEModule();

	virtual	uint16				Tag() const;
};


class DIEPointerToMemberType : public DIEDerivedType {
public:
								DIEPointerToMemberType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_address_class
// DW_AT_containing_type
// DW_AT_use_location
};


class DIESetType : public DIEDerivedType {
public:
								DIESetType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_byte_size
};


class DIESubrangeType : public DIEDerivedType {
public:
								DIESubrangeType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_bit_stride
// DW_AT_byte_size
// DW_AT_byte_stride
// DW_AT_count
// DW_AT_lower_bound
// DW_AT_threads_scaled
// DW_AT_upper_bound
};


class DIEWithStatement : public DebugInfoEntry {
public:
								DIEWithStatement();

	virtual	uint16				Tag() const;
};


class DIEAccessDeclaration : public DebugInfoEntry {
public:
								DIEAccessDeclaration();

	virtual	uint16				Tag() const;
};


class DIEBaseType : public DIEType {
public:
								DIEBaseType();

	virtual	uint16				Tag() const;

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
			uint8				fEncoding;
			uint8				fEndianity;
			uint16				fByteSize;
			uint16				fBitSize;
			uint16				fBitOffset;
};


class DIECatchBlock : public DebugInfoEntry {
public:
								DIECatchBlock();

	virtual	uint16				Tag() const;
};


class DIEConstType : public DIEModifiedType {
public:
								DIEConstType();

	virtual	uint16				Tag() const;
};


class DIEConstant : public DebugInfoEntry {
public:
								DIEConstant();

	virtual	uint16				Tag() const;
};


class DIEEnumerator : public DebugInfoEntry {
public:
								DIEEnumerator();

	virtual	uint16				Tag() const;
};


class DIEFileType : public DIEDerivedType {
public:
								DIEFileType();

	virtual	uint16				Tag() const;

// TODO:
// DW_AT_byte_size
};


class DIEFriend : public DebugInfoEntry {
public:
								DIEFriend();

	virtual	uint16				Tag() const;
};


class DIENameList : public DebugInfoEntry {
public:
								DIENameList();

	virtual	uint16				Tag() const;
};


class DIENameListItem : public DebugInfoEntry {
public:
								DIENameListItem();

	virtual	uint16				Tag() const;
};


class DIEPackedType : public DIEModifiedType {
public:
								DIEPackedType();

	virtual	uint16				Tag() const;
};


class DIESubprogram : public DebugInfoEntry {
public:
								DIESubprogram();

	virtual	uint16				Tag() const;
};


class DIETemplateTypeParameter : public DebugInfoEntry {
public:
								DIETemplateTypeParameter();

	virtual	uint16				Tag() const;
};


class DIETemplateValueParameter : public DebugInfoEntry {
public:
								DIETemplateValueParameter();

	virtual	uint16				Tag() const;
};


class DIEThrownType : public DebugInfoEntry {
public:
								DIEThrownType();

	virtual	uint16				Tag() const;
};


class DIETryBlock : public DebugInfoEntry {
public:
								DIETryBlock();

	virtual	uint16				Tag() const;
};


class DIEVariantPart : public DebugInfoEntry {
public:
								DIEVariantPart();

	virtual	uint16				Tag() const;
};


class DIEVariable : public DebugInfoEntry {
public:
								DIEVariable();

	virtual	uint16				Tag() const;
};


class DIEVolatileType : public DIEModifiedType {
public:
								DIEVolatileType();

	virtual	uint16				Tag() const;

// TODO:
// DECL
};


class DIEDwarfProcedure : public DebugInfoEntry {
public:
								DIEDwarfProcedure();

	virtual	uint16				Tag() const;
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


class DIENamespace : public DebugInfoEntry {
public:
								DIENamespace();

	virtual	uint16				Tag() const;
};


class DIEImportedModule : public DebugInfoEntry {
public:
								DIEImportedModule();

	virtual	uint16				Tag() const;
};


class DIEUnspecifiedType : public DIEType {
public:
								DIEUnspecifiedType();

	virtual	uint16				Tag() const;

// TODO:
// DECL
// DW_AT_description
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
};


class DIECondition : public DebugInfoEntry {
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

// TODO:
// DECL

private:
			uint64				fBlockSize;
};


// #pragma mark - DebugInfoEntryFactory


class DebugInfoEntryFactory {
public:
								DebugInfoEntryFactory();

			status_t			CreateDebugInfoEntry(uint16 tag,
									DebugInfoEntry*& entry);
};


#endif	// DEBUG_INFO_ENTRIES_H
