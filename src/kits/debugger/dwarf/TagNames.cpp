/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TagNames.h"

#include "Dwarf.h"


struct tag_name_info {
	uint16					tag;
	const char*				name;
};


#undef ENTRY
#define ENTRY(name)	{ DW_TAG_##name, "DW_TAG_" #name }

static const tag_name_info kTagNameInfos[] = {
	ENTRY(array_type),
	ENTRY(class_type),
	ENTRY(entry_point),
	ENTRY(enumeration_type),
	ENTRY(formal_parameter),
	ENTRY(imported_declaration),
	ENTRY(label),
	ENTRY(lexical_block),
	ENTRY(member),
	ENTRY(pointer_type),
	ENTRY(reference_type),
	ENTRY(compile_unit),
	ENTRY(string_type),
	ENTRY(structure_type),
	ENTRY(subroutine_type),
	ENTRY(typedef),
	ENTRY(union_type),
	ENTRY(unspecified_parameters),
	ENTRY(variant),
	ENTRY(common_block),
	ENTRY(common_inclusion),
	ENTRY(inheritance),
	ENTRY(inlined_subroutine),
	ENTRY(module),
	ENTRY(ptr_to_member_type),
	ENTRY(set_type),
	ENTRY(subrange_type),
	ENTRY(with_stmt),
	ENTRY(access_declaration),
	ENTRY(base_type),
	ENTRY(catch_block),
	ENTRY(const_type),
	ENTRY(constant),
	ENTRY(enumerator),
	ENTRY(file_type),
	ENTRY(friend),
	ENTRY(namelist),
	ENTRY(namelist_item),
	ENTRY(packed_type),
	ENTRY(subprogram),
	ENTRY(template_type_parameter),
	ENTRY(template_value_parameter),
	ENTRY(thrown_type),
	ENTRY(try_block),
	ENTRY(variant_part),
	ENTRY(variable),
	ENTRY(volatile_type),
	ENTRY(dwarf_procedure),
	ENTRY(restrict_type),
	ENTRY(interface_type),
	ENTRY(namespace),
	ENTRY(imported_module),
	ENTRY(unspecified_type),
	ENTRY(partial_unit),
	ENTRY(imported_unit),
	ENTRY(condition),
	ENTRY(shared_type),
	ENTRY(type_unit),
	ENTRY(rvalue_reference_type),
	ENTRY(template_alias),
	ENTRY(GNU_template_parameter_pack),
	ENTRY(GNU_formal_parameter_pack),
	ENTRY(GNU_call_site),
	ENTRY(GNU_call_site_parameter),
	{}
};


static const uint32 kTagNameInfoCount = DW_TAG_template_alias + 5;
static const char* sTagNames[kTagNameInfoCount];

static struct InitTagNames {
	InitTagNames()
	{
		for (uint32 i = 0; kTagNameInfos[i].name != NULL; i++) {
			const tag_name_info& info = kTagNameInfos[i];
			if (info.tag <= DW_TAG_template_alias)
				sTagNames[info.tag] = info.name;
			else {
				sTagNames[DW_TAG_template_alias + 1 + (info.tag
						- DW_TAG_GNU_template_parameter_pack)] = info.name;
			}
		}
	}
} sInitTagNames;


const char*
get_entry_tag_name(uint16 tag)
{
	if (tag <= DW_TAG_template_alias)
		return sTagNames[tag];
	else if (tag >= DW_TAG_GNU_template_parameter_pack
		&& tag <= DW_TAG_GNU_call_site_parameter) {
		return sTagNames[DW_TAG_template_alias + 1 + (tag
				- DW_TAG_GNU_template_parameter_pack)];
	}

	return NULL;
}
