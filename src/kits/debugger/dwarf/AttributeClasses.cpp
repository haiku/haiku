/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "AttributeClasses.h"
#include "Dwarf.h"


enum {
	AC_ADDRESS		= 1 << (ATTRIBUTE_CLASS_ADDRESS - 1),
	AC_BLOCK		= 1 << (ATTRIBUTE_CLASS_BLOCK - 1),
	AC_CONSTANT		= 1 << (ATTRIBUTE_CLASS_CONSTANT - 1),
	AC_FLAG			= 1 << (ATTRIBUTE_CLASS_FLAG - 1),
	AC_LINEPTR		= 1 << (ATTRIBUTE_CLASS_LINEPTR - 1),
	AC_LOCLISTPTR	= 1 << (ATTRIBUTE_CLASS_LOCLISTPTR - 1),
	AC_MACPTR		= 1 << (ATTRIBUTE_CLASS_MACPTR - 1),
	AC_RANGELISTPTR	= 1 << (ATTRIBUTE_CLASS_RANGELISTPTR - 1),
	AC_REFERENCE	= 1 << (ATTRIBUTE_CLASS_REFERENCE - 1),
	AC_STRING		= 1 << (ATTRIBUTE_CLASS_STRING - 1)
};


struct attribute_info_entry {
	const char*	name;
	uint16		value;
	uint16		classes;
};

struct attribute_name_info_entry {
	const char*				name;
	DebugInfoEntrySetter	setter;
	uint16					value;
	uint16					classes;
};


#undef ENTRY
#define ENTRY(name)	"DW_AT_" #name, &DebugInfoEntry::AddAttribute_##name, \
	DW_AT_##name

static const attribute_name_info_entry kAttributeNameInfos[] = {
	{ ENTRY(sibling),				AC_REFERENCE },
	{ ENTRY(location),				AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(name),					AC_STRING },
	{ ENTRY(ordering),				AC_CONSTANT },
	{ ENTRY(byte_size),				AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(bit_offset),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(bit_size),				AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(stmt_list),				AC_LINEPTR },
	{ ENTRY(low_pc),				AC_ADDRESS | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(high_pc),				AC_ADDRESS | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(language),				AC_CONSTANT },
	{ ENTRY(discr),					AC_REFERENCE },
	{ ENTRY(discr_value),			AC_CONSTANT },
	{ ENTRY(visibility),			AC_CONSTANT },
	{ ENTRY(import),				AC_REFERENCE },
	{ ENTRY(string_length),			AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(common_reference),		AC_REFERENCE },
	{ ENTRY(comp_dir),				AC_STRING },
	{ ENTRY(const_value),			AC_BLOCK | AC_CONSTANT | AC_STRING },
	{ ENTRY(containing_type),		AC_REFERENCE },
	{ ENTRY(default_value),			AC_REFERENCE | AC_CONSTANT | AC_FLAG },
	{ ENTRY(inline),				AC_CONSTANT },
	{ ENTRY(is_optional),			AC_FLAG },
	{ ENTRY(lower_bound),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(producer),				AC_STRING },
	{ ENTRY(prototyped),			AC_FLAG },
	{ ENTRY(return_addr),			AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(start_scope),			AC_CONSTANT },
	{ ENTRY(bit_stride),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(upper_bound),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(abstract_origin),		AC_REFERENCE },
	{ ENTRY(accessibility),			AC_CONSTANT },
	{ ENTRY(address_class),			AC_CONSTANT },
	{ ENTRY(artificial),			AC_FLAG },
	{ ENTRY(base_types),			AC_REFERENCE },
	{ ENTRY(calling_convention),	AC_CONSTANT },
	{ ENTRY(count),					AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(data_member_location),	AC_BLOCK | AC_CONSTANT | AC_LOCLISTPTR },
	{ ENTRY(decl_column),			AC_CONSTANT },
	{ ENTRY(decl_file),				AC_CONSTANT },
	{ ENTRY(decl_line),				AC_CONSTANT },
	{ ENTRY(declaration),			AC_FLAG },
	{ ENTRY(discr_list),			AC_BLOCK },
	{ ENTRY(encoding),				AC_CONSTANT },
	{ ENTRY(external),				AC_FLAG },
	{ ENTRY(frame_base),			AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(friend),				AC_REFERENCE },
	{ ENTRY(identifier_case),		AC_CONSTANT },
	{ ENTRY(macro_info),			AC_MACPTR },
	{ ENTRY(namelist_item),			AC_BLOCK | AC_REFERENCE },
	{ ENTRY(priority),				AC_REFERENCE },
	{ ENTRY(segment),				AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(specification),			AC_REFERENCE },
	{ ENTRY(static_link),			AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(type),					AC_REFERENCE },
	{ ENTRY(use_location),			AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(variable_parameter),	AC_FLAG },
	{ ENTRY(virtuality),			AC_CONSTANT },
	{ ENTRY(vtable_elem_location),	AC_BLOCK | AC_LOCLISTPTR },
	{ ENTRY(allocated),				AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(associated),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(data_location),			AC_BLOCK },
	{ ENTRY(byte_stride),			AC_BLOCK | AC_CONSTANT | AC_REFERENCE },
	{ ENTRY(entry_pc),				AC_ADDRESS },
	{ ENTRY(use_UTF8),				AC_FLAG },
	{ ENTRY(extension),				AC_REFERENCE },
	{ ENTRY(ranges),				AC_RANGELISTPTR },
	{ ENTRY(trampoline),			AC_ADDRESS | AC_FLAG | AC_REFERENCE
										| AC_STRING },
	{ ENTRY(call_column),			AC_CONSTANT },
	{ ENTRY(call_file),				AC_CONSTANT },
	{ ENTRY(call_line),				AC_CONSTANT },
	{ ENTRY(description),			AC_STRING },
	{ ENTRY(binary_scale),			AC_CONSTANT },
	{ ENTRY(decimal_scale),			AC_CONSTANT },
	{ ENTRY(small),					AC_REFERENCE },
	{ ENTRY(decimal_sign),			AC_CONSTANT },
	{ ENTRY(digit_count),			AC_CONSTANT },
	{ ENTRY(picture_string),		AC_STRING },
	{ ENTRY(mutable),				AC_FLAG },
	{ ENTRY(threads_scaled),		AC_FLAG },
	{ ENTRY(explicit),				AC_FLAG },
	{ ENTRY(object_pointer),		AC_REFERENCE },
	{ ENTRY(endianity),				AC_CONSTANT },
	{ ENTRY(elemental),				AC_FLAG },
	{ ENTRY(pure),					AC_FLAG },
	{ ENTRY(recursive),				AC_FLAG },
	{ ENTRY(signature),				AC_REFERENCE },
	{ ENTRY(main_subprogram),		AC_FLAG },
	{ ENTRY(data_bit_offset),		AC_CONSTANT },
	{ ENTRY(const_expr),			AC_FLAG },
	{ ENTRY(enum_class),			AC_FLAG },
	{ ENTRY(linkage_name),			AC_STRING },
	{ ENTRY(call_site_value),		AC_BLOCK },
	{ ENTRY(call_site_data_value),	AC_BLOCK },
	{ ENTRY(call_site_target),		AC_BLOCK },
	{ ENTRY(call_site_target_clobbered),
									AC_BLOCK },
	{ ENTRY(tail_call),				AC_FLAG },
	{ ENTRY(all_tail_call_sites),	AC_FLAG },
	{ ENTRY(all_call_sites),		AC_FLAG },
	{ ENTRY(all_source_call_sites),	AC_FLAG },

	{}
};

static const uint32 kAttributeNameInfoCount = DW_AT_linkage_name + 9;
static attribute_name_info_entry sAttributeNameInfos[kAttributeNameInfoCount];


#undef ENTRY
#define ENTRY(name)	"DW_FORM_" #name, DW_FORM_##name

static const attribute_info_entry kAttributeFormInfos[] = {
	{ ENTRY(addr),			AC_ADDRESS },
	{ ENTRY(block2),		AC_BLOCK },
	{ ENTRY(block4),		AC_BLOCK },
	{ ENTRY(data2),			AC_CONSTANT },
	{ ENTRY(data4),			AC_CONSTANT | AC_LINEPTR | AC_LOCLISTPTR
								| AC_MACPTR | AC_RANGELISTPTR },
	{ ENTRY(data8),			AC_CONSTANT | AC_LINEPTR | AC_LOCLISTPTR
								| AC_MACPTR | AC_RANGELISTPTR },
	{ ENTRY(string),		AC_STRING },
	{ ENTRY(block),			AC_BLOCK },
	{ ENTRY(block1),		AC_BLOCK },
	{ ENTRY(data1),			AC_CONSTANT },
	{ ENTRY(flag),			AC_FLAG },
	{ ENTRY(sdata),			AC_CONSTANT },
	{ ENTRY(strp),			AC_STRING },
	{ ENTRY(udata),			AC_CONSTANT },
	{ ENTRY(ref_addr),		AC_REFERENCE },
	{ ENTRY(ref1),			AC_REFERENCE },
	{ ENTRY(ref2),			AC_REFERENCE },
	{ ENTRY(ref4),			AC_REFERENCE },
	{ ENTRY(ref8),			AC_REFERENCE },
	{ ENTRY(ref_udata),		AC_REFERENCE },
	{ ENTRY(indirect),		AC_REFERENCE },
	{ ENTRY(sec_offset),	AC_LINEPTR | AC_LOCLISTPTR | AC_MACPTR
								| AC_RANGELISTPTR },
	{ ENTRY(exprloc),		AC_BLOCK },
	{ ENTRY(flag_present),	AC_FLAG },
	{ ENTRY(ref_sig8),		AC_REFERENCE },
	{ ENTRY(implicit_const),
							AC_CONSTANT },
	{}
};

static const uint32 kAttributeFormInfoCount = DW_FORM_implicit_const + 1;
static attribute_info_entry sAttributeFormInfos[kAttributeFormInfoCount];

static struct InitAttributeInfos {
	InitAttributeInfos()
	{
		for (uint32 i = 0; kAttributeNameInfos[i].name != NULL; i++) {
			const attribute_name_info_entry& entry = kAttributeNameInfos[i];
			if (entry.value <= DW_AT_linkage_name)
				sAttributeNameInfos[entry.value] = entry;
			else {
				sAttributeNameInfos[DW_AT_linkage_name + 1
					+ (entry.value - DW_AT_call_site_value)] = entry;
			}
		}

		for (uint32 i = 0; kAttributeFormInfos[i].name != NULL; i++) {
			const attribute_info_entry& entry = kAttributeFormInfos[i];
			sAttributeFormInfos[entry.value] = entry;
		}
	}
} sInitAttributeInfos;


uint16
get_attribute_name_classes(uint32 name)
{
	if (name <= DW_AT_linkage_name)
		return sAttributeNameInfos[name].classes;
	else if (name >= DW_AT_call_site_value
		&& name <= DW_AT_all_source_call_sites) {
		return sAttributeNameInfos[DW_AT_linkage_name + 1
			+ (name - DW_AT_call_site_value)].classes;
	}

	return 0;
}


uint16
get_attribute_form_classes(uint32 form)
{
	return form < kAttributeFormInfoCount
		? sAttributeFormInfos[form].classes : 0;
}


uint8
get_attribute_class(uint32 name, uint32 form)
{
	uint16 classes = get_attribute_name_classes(name)
		& get_attribute_form_classes(form);

	int clazz = 0;
	while (classes != 0) {
		classes >>= 1;
		clazz++;
	}

	return clazz;
}


const char*
get_attribute_name_name(uint32 name)
{
	if (name <= DW_AT_linkage_name)
		return sAttributeNameInfos[name].name;
	else if (name >= DW_AT_call_site_value
		&& name <= DW_AT_all_source_call_sites) {
		return sAttributeNameInfos[DW_AT_linkage_name + 1 +
				(name - DW_AT_call_site_value)].name;
	}

	return NULL;
}


const char*
get_attribute_form_name(uint32 form)
{
	return form < kAttributeFormInfoCount
		? sAttributeFormInfos[form].name : NULL;
}


DebugInfoEntrySetter
get_attribute_name_setter(uint32 name)
{
	return (name < kAttributeNameInfoCount)
		? sAttributeNameInfos[name].setter : NULL;
}
