/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebugInfoEntries.h"

#include <new>

#include "AttributeValue.h"
#include "Dwarf.h"


#define DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(name)				\
	status_t													\
	DebugInfoEntry::AddAttribute_##name(uint16 attributeName,	\
		const AttributeValue& value)							\
	{															\
		return ATTRIBUTE_NOT_HANDLED;											\
	}


DebugInfoEntry::DebugInfoEntry()
	:
	fParent(NULL)
{
}


DebugInfoEntry::~DebugInfoEntry()
{
}


status_t
DebugInfoEntry::InitAfterHierarchy(DebugInfoEntryInitInfo& info)
{
	return B_OK;
}


status_t
DebugInfoEntry::InitAfterAttributes(DebugInfoEntryInitInfo& info)
{
	return B_OK;
}


void
DebugInfoEntry::SetParent(DebugInfoEntry* parent)
{
	fParent = parent;
}


bool
DebugInfoEntry::IsType() const
{
	return false;
}


bool
DebugInfoEntry::IsNamespace() const
{
	return false;
}


const char*
DebugInfoEntry::Name() const
{
	return NULL;
}

const char*
DebugInfoEntry::Description() const
{
	return NULL;
}


DebugInfoEntry*
DebugInfoEntry::Specification() const
{
	return NULL;
}


DebugInfoEntry*
DebugInfoEntry::AbstractOrigin() const
{
	return NULL;
}


LocationDescription*
DebugInfoEntry::GetLocationDescription()
{
	return NULL;
}


bool
DebugInfoEntry::GetDeclarationFile(uint32& _file) const
{
	DeclarationLocation* location = const_cast<DebugInfoEntry*>(this)
		->GetDeclarationLocation();
	if (location == NULL || !location->IsFileSet())
		return false;

	_file = location->file;
	return true;
}


bool
DebugInfoEntry::GetDeclarationLine(uint32& _line) const
{
	DeclarationLocation* location = const_cast<DebugInfoEntry*>(this)
		->GetDeclarationLocation();
	if (location == NULL || !location->IsLineSet())
		return false;

	_line = location->line;
	return true;
}


bool
DebugInfoEntry::GetDeclarationColumn(uint32& _column) const
{
	DeclarationLocation* location = const_cast<DebugInfoEntry*>(this)
		->GetDeclarationLocation();
	if (location == NULL || !location->IsColumnSet())
		return false;

	_column = location->column;
	return true;
}


status_t
DebugInfoEntry::AddChild(DebugInfoEntry* child)
{
	// ignore children where we don't expect them
	return ENTRY_NOT_HANDLED;
}


status_t
DebugInfoEntry::AddAttribute_decl_file(uint16 attributeName,
	const AttributeValue& value)
{
	if (DeclarationLocation* location = GetDeclarationLocation()) {
		location->SetFile(value.constant);
		return B_OK;
	}

	return ATTRIBUTE_NOT_HANDLED;
}


status_t
DebugInfoEntry::AddAttribute_decl_line(uint16 attributeName,
	const AttributeValue& value)
{
	if (DeclarationLocation* location = GetDeclarationLocation()) {
		location->SetLine(value.constant);
		return B_OK;
	}

	return ATTRIBUTE_NOT_HANDLED;
}


status_t
DebugInfoEntry::AddAttribute_decl_column(uint16 attributeName,
	const AttributeValue& value)
{
	if (DeclarationLocation* location = GetDeclarationLocation()) {
		location->SetColumn(value.constant);
		return B_OK;
	}

	return ATTRIBUTE_NOT_HANDLED;
}


status_t
DebugInfoEntry::AddAttribute_location(uint16 attributeName,
	const AttributeValue& value)
{
	if (LocationDescription* location = GetLocationDescription()) {
		if (value.attributeClass == ATTRIBUTE_CLASS_LOCLISTPTR) {
			location->SetToLocationList(value.pointer);
			return B_OK;
		}

		if (value.attributeClass == ATTRIBUTE_CLASS_BLOCK) {
			location->SetToExpression(value.block.data, value.block.length);
			return B_OK;
		}
		return B_BAD_DATA;
	}

	return ATTRIBUTE_NOT_HANDLED;
}


status_t
DebugInfoEntry::AddAttribute_sibling(uint16 attributeName,
	const AttributeValue& value)
{
	// This attribute is only intended to help the debug info consumer. We don't
	// need it.
	return B_OK;
}


DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(name)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(ordering)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(byte_size)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_offset)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_size)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(stmt_list)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(low_pc)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(high_pc)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(language)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr_value)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(visibility)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(import)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(string_length)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(common_reference)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(comp_dir)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(const_value)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(containing_type)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(default_value)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(inline)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(is_optional)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(lower_bound)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(producer)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(prototyped)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(return_addr)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(start_scope)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(bit_stride)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(upper_bound)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(abstract_origin)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(accessibility)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(address_class)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(artificial)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(base_types)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(calling_convention)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(count)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(data_member_location)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(declaration)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(discr_list)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(encoding)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(external)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(frame_base)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(friend)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(identifier_case)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(macro_info)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(namelist_item)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(priority)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(segment)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(specification)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(static_link)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(type)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(use_location)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(variable_parameter)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(virtuality)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(vtable_elem_location)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(allocated)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(associated)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(data_location)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(byte_stride)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(entry_pc)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(use_UTF8)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(extension)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(ranges)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(trampoline)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_column)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_file)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(call_line)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(description)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(binary_scale)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(decimal_scale)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(small)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(decimal_sign)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(digit_count)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(picture_string)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(mutable)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(threads_scaled)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(explicit)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(object_pointer)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(endianity)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(elemental)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(pure)
DEFINE_DEBUG_INFO_ENTRY_ATTR_SETTER(recursive)


DeclarationLocation*
DebugInfoEntry::GetDeclarationLocation()
{
	return NULL;
}


status_t
DebugInfoEntry::SetDynamicAttributeValue(DynamicAttributeValue& toSet,
	const AttributeValue& value)
{
	switch (value.attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
			toSet.SetTo(value.constant);
			return B_OK;
		case ATTRIBUTE_CLASS_REFERENCE:
			toSet.SetTo(value.reference);
			return B_OK;
		case ATTRIBUTE_CLASS_BLOCK:
			toSet.SetTo(value.block.data, value.block.length);
			return B_OK;
		default:
			return B_BAD_DATA;
	}
}


status_t
DebugInfoEntry::SetConstantAttributeValue(ConstantAttributeValue& toSet,
	const AttributeValue& value)
{
	switch (value.attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
			toSet.SetTo(value.constant);
			return B_OK;
		case ATTRIBUTE_CLASS_STRING:
			toSet.SetTo(value.string);
			return B_OK;
		case ATTRIBUTE_CLASS_BLOCK:
			toSet.SetTo(value.block.data, value.block.length);
			return B_OK;
		default:
			return B_BAD_DATA;
	}
}


status_t
DebugInfoEntry::SetMemberLocation(MemberLocation& toSet,
	const AttributeValue& value)
{
	switch (value.attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
			toSet.SetToConstant(value.constant);
			return B_OK;
		case ATTRIBUTE_CLASS_BLOCK:
			toSet.SetToExpression(value.block.data, value.block.length);
			return B_OK;
		case ATTRIBUTE_CLASS_LOCLISTPTR:
			toSet.SetToLocationList(value.pointer);
			return B_OK;
		default:
			return B_BAD_DATA;
	}
}
