/*
 * Copyright 2009-2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_DATA_TYPES_H
#define HID_DATA_TYPES_H

#include <lendian_bitfield.h>

#define ITEM_TYPE_MAIN						0x0
#define ITEM_TYPE_GLOBAL					0x1
#define ITEM_TYPE_LOCAL						0x2
#define ITEM_TYPE_LONG						0x3

#define ITEM_TAG_MAIN_INPUT					0x8
#define ITEM_TAG_MAIN_OUTPUT				0x9
#define ITEM_TAG_MAIN_FEATURE				0xb
#define ITEM_TAG_MAIN_COLLECTION			0xa
#define ITEM_TAG_MAIN_END_COLLECTION		0xc

#define ITEM_TAG_GLOBAL_USAGE_PAGE			0x0
#define ITEM_TAG_GLOBAL_LOGICAL_MINIMUM		0x1
#define ITEM_TAG_GLOBAL_LOGICAL_MAXIMUM		0x2
#define ITEM_TAG_GLOBAL_PHYSICAL_MINIMUM	0x3
#define ITEM_TAG_GLOBAL_PHYSICAL_MAXIMUM	0x4
#define ITEM_TAG_GLOBAL_UNIT_EXPONENT		0x5
#define ITEM_TAG_GLOBAL_UNIT				0x6
#define ITEM_TAG_GLOBAL_REPORT_SIZE			0x7
#define ITEM_TAG_GLOBAL_REPORT_ID			0x8
#define ITEM_TAG_GLOBAL_REPORT_COUNT		0x9
#define ITEM_TAG_GLOBAL_PUSH				0xa
#define ITEM_TAG_GLOBAL_POP					0xb

#define ITEM_TAG_LOCAL_USAGE				0x0
#define ITEM_TAG_LOCAL_USAGE_MINIMUM		0x1
#define ITEM_TAG_LOCAL_USAGE_MAXIMUM		0x2
#define ITEM_TAG_LOCAL_DESIGNATOR_INDEX		0x3
#define ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM	0x4
#define ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM	0x5
#define ITEM_TAG_LOCAL_STRING_INDEX			0x7
#define ITEM_TAG_LOCAL_STRING_MINIMUM		0x8
#define ITEM_TAG_LOCAL_STRING_MAXIMUM		0x9
#define ITEM_TAG_LOCAL_DELIMITER			0xa

#define ITEM_TAG_LONG						0xf

#define COLLECTION_PHYSICAL					0x00
#define COLLECTION_APPLICATION				0x01
#define COLLECTION_LOGICAL					0x02
#define COLLECTION_REPORT					0x03
#define COLLECTION_NAMED_ARRAY				0x04
#define COLLECTION_USAGE_SWITCH				0x05
#define COLLECTION_USAGE_MODIFIER			0x06
#define COLLECTION_ALL						0xff

#define UNIT_SYSTEM							0x0
#define UNIT_LENGTH							0x1
#define UNIT_MASS							0x2
#define UNIT_TIME							0x3
#define UNIT_TEMPERATURE					0x4
#define UNIT_CURRENT						0x5
#define UNIT_LUMINOUS_INTENSITY				0x6

#define USAGE_PAGE_SHIFT					16
#define USAGE_PAGE_MASK						0xffff
#define USAGE_ID_SHIFT						0
#define USAGE_ID_MASK						0xffff


typedef struct item_prefix {
	LBITFIELD8_3(
		size	: 2,
		type	: 2,
		tag		: 4
	);
} _PACKED item_prefix;


typedef struct short_item {
	item_prefix	prefix;

	union {
		uint8	as_uint8[4];
		int8	as_int8[4];
		uint16	as_uint16[2];
		int16	as_int16[2];
		uint32	as_uint32;
		int32	as_int32;
	} data;
} _PACKED short_item;


typedef struct long_item {
	item_prefix	prefix;
	uint8		data_size;
	uint8		long_item_tag;
	uint8		data[0];
} _PACKED long_item;


typedef struct main_item_data {
	LBITFIELD9(
		data_constant	: 1,
		array_variable	: 1,
		relative		: 1,
		wrap			: 1,
		non_linear		: 1,
		no_preferred	: 1,
		null_state		: 1,
		is_volatile		: 1,
		bits_bytes		: 1
	);

	//uint8			reserved[2];
} _PACKED main_item_data;


typedef struct usage_value {
	union {
		struct {
			uint16	usage_id;
			uint16	usage_page;
		} _PACKED s;
		uint32		extended;
	} u;

	bool			is_extended;

					usage_value()
					{
						u.extended = 0;
						is_extended = false;
					}
} usage_value;


typedef struct global_item_state {
	uint16			usage_page;
	uint32			logical_minimum;
	uint32			logical_maximum;
	uint32			physical_minimum;
	uint32			physical_maximum;
	uint8			unit_exponent;
	uint8			unit;
	uint32			report_size;
	uint32			report_count;
	uint8			report_id;
	global_item_state *link;
} global_item_state;


typedef struct local_item_state {
	usage_value *	usage_stack;
	uint32			usage_stack_used;
	usage_value		usage_minimum;
	usage_value		usage_maximum;
	bool			usage_minimum_set;
	bool			usage_maximum_set;
	uint32			designator_index;
	bool			designator_index_set;
	uint32			designator_minimum;
	uint32			designator_maximum;
	uint8			string_index;
	bool			string_index_set;
	uint8			string_minimum;
	uint8			string_maximum;
} local_item_state;

#endif // HID_DATA_TYPES_H
