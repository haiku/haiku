/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_TABLES_H
#define ATTRIBUTE_TABLES_H

#include "DebugInfoEntry.h"


typedef status_t (DebugInfoEntry::* DebugInfoEntrySetter)(uint16,
	const AttributeValue&);


// attribute classes
enum {
	ATTRIBUTE_CLASS_UNKNOWN			= 0,
	ATTRIBUTE_CLASS_ADDRESS			= 1,
	ATTRIBUTE_CLASS_BLOCK			= 2,
	ATTRIBUTE_CLASS_CONSTANT		= 3,
	ATTRIBUTE_CLASS_FLAG			= 4,
	ATTRIBUTE_CLASS_LINEPTR			= 5,
	ATTRIBUTE_CLASS_LOCLISTPTR		= 6,
	ATTRIBUTE_CLASS_MACPTR			= 7,
	ATTRIBUTE_CLASS_RANGELISTPTR	= 8,
	ATTRIBUTE_CLASS_REFERENCE		= 9,
	ATTRIBUTE_CLASS_STRING			= 10
};


uint16	get_attribute_name_classes(uint32 name);
uint16	get_attribute_form_classes(uint32 form);
uint8	get_attribute_class(uint32 name, uint32 form);

const char*	get_attribute_name_name(uint32 name);
const char*	get_attribute_form_name(uint32 form);

DebugInfoEntrySetter	get_attribute_name_setter(uint32 name);


#endif	// ATTRIBUTE_TABLES_H
