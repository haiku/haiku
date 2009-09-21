/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_INFO_ENTRY_H
#define DEBUG_INFO_ENTRY_H

#include <String.h>

#include <util/DoublyLinkedList.h>

#include "Types.h"


#define DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(name)	\
	virtual	status_t			AddAttribute_##name(uint16 attributeName, \
									const AttributeValue& value);

enum {
	ATTRIBUTE_NOT_HANDLED	= 1,
	ENTRY_NOT_HANDLED		= 2
};


struct AttributeValue;
struct ConstantAttributeValue;
struct DeclarationLocation;
struct DynamicAttributeValue;
struct LocationDescription;
struct MemberLocation;
struct SourceLanguageInfo;


struct DebugInfoEntryInitInfo {
	const SourceLanguageInfo*	languageInfo;
};


class DebugInfoEntry : public DoublyLinkedListLinkImpl<DebugInfoEntry> {
public:
								DebugInfoEntry();
	virtual						~DebugInfoEntry();

	virtual	status_t			InitAfterHierarchy(
									DebugInfoEntryInitInfo& info);
	virtual	status_t			InitAfterAttributes(
									DebugInfoEntryInitInfo& info);

	virtual	uint16				Tag() const = 0;

			DebugInfoEntry*		Parent() const	{ return fParent; }
			void				SetParent(DebugInfoEntry* parent);

	virtual	bool				IsType() const;
	virtual	bool				IsNamespace() const;
									// a namespace-like thingy (namespace,
									// class, ...)

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;
	virtual	DebugInfoEntry*		Specification() const;
	virtual	DebugInfoEntry*		AbstractOrigin() const;
	virtual	LocationDescription* GetLocationDescription();

			bool				GetDeclarationFile(uint32& _file) const;
			bool				GetDeclarationLine(uint32& _line) const;
			bool				GetDeclarationColumn(uint32& _column) const;

	virtual	status_t			AddChild(DebugInfoEntry* child);

	virtual	status_t			AddAttribute_decl_file(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_line(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_decl_column(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_location(uint16 attributeName,
									const AttributeValue& value);
	virtual	status_t			AddAttribute_sibling(uint16 attributeName,
									const AttributeValue& value);

// TODO: Handle (ignore?) DW_AT_description here?

	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(name)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(ordering)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(byte_size)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_offset)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_size)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(stmt_list)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(low_pc)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(high_pc)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(language)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr_value)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(visibility)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(import)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(string_length)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(common_reference)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(comp_dir)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(const_value)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(containing_type)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(default_value)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(inline)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(is_optional)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(lower_bound)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(producer)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(prototyped)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(return_addr)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(start_scope)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_stride)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(upper_bound)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(abstract_origin)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(accessibility)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(address_class)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(artificial)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(base_types)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(calling_convention)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(count)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(data_member_location)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(declaration)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr_list)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(encoding)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(external)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(frame_base)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(friend)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(identifier_case)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(macro_info)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(namelist_item)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(priority)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(segment)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(specification)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(static_link)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(type)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(use_location)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(variable_parameter)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(virtuality)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(vtable_elem_location)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(allocated)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(associated)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(data_location)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(byte_stride)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(entry_pc)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(use_UTF8)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(extension)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(ranges)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(trampoline)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_column)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_file)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_line)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(description)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(binary_scale)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(decimal_scale)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(small)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(decimal_sign)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(digit_count)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(picture_string)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(mutable)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(threads_scaled)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(explicit)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(object_pointer)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(endianity)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(elemental)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(pure)
	DECLARE_DEBUG_INFO_ENTRY_ATTR_SETTER(recursive)

protected:
	virtual	DeclarationLocation* GetDeclarationLocation();

			status_t	 		SetDynamicAttributeValue(
									DynamicAttributeValue& toSet,
									const AttributeValue& value);
			status_t	 		SetConstantAttributeValue(
									ConstantAttributeValue& toSet,
									const AttributeValue& value);
			status_t	 		SetMemberLocation(MemberLocation& toSet,
									const AttributeValue& value);

protected:
			DebugInfoEntry*		fParent;
};


typedef DoublyLinkedList<DebugInfoEntry> DebugInfoEntryList;


#endif	// DEBUG_INFO_ENTRY_H
