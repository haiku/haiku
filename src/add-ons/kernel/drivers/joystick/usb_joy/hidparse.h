/*
	HID parser
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved
*/

#include <sys/types.h>

#define	SUCCESS	0
#define	FAILURE	(-1)

#define	BOOL	int
#define	TRUE	1
#define	FALSE	0

/* 
	Report Descriptor Items
	see 
	5.3 Generic Item Format
	6.2.2 Report Descriptor
	8. Report Protocol
 */

enum
{
	ITEM_SIZE_0,
	ITEM_SIZE_1,
	ITEM_SIZE_2,	/* long item */
	ITEM_SIZE_4,
};

enum
{
	ITEM_TYPE_MAIN,
	ITEM_TYPE_GLOBAL,
	ITEM_TYPE_LOCAL,
	ITEM_TYPE_RESERVED,	/* long item */
};

enum
{
	/* main items */
	ITEM_TAG_INPUT = 8,
	ITEM_TAG_OUTPUT,
	ITEM_TAG_COLLECTION,
	ITEM_TAG_FEATURE,
	ITEM_TAG_END_COLLECTION,

	/* global items */
	ITEM_TAG_USAGE_PAGE = 0,
	ITEM_TAG_LOGICAL_MINIMUM,
	ITEM_TAG_LOGICAL_MAXIMUM,
	ITEM_TAG_PHYSICAL_MINIMUM,
	ITEM_TAG_PHYSICAL_MAXIMUM,
	ITEM_TAG_UNIT_EXPONENT,
	ITEM_TAG_UNIT,
	ITEM_TAG_REPORT_SIZE,
	ITEM_TAG_REPORT_ID,
	ITEM_TAG_REPORT_COUNT,
	ITEM_TAG_PUSH,
	ITEM_TAG_POP,

	/* local items */
	ITEM_TAG_USAGE = 0,
	ITEM_TAG_USAGE_MINIMUM,
	ITEM_TAG_USAGE_MAXIMUM,
	ITEM_TAG_DESIGNATOR_INDEX,
	ITEM_TAG_DESIGNATOR_MINIMUM,
	ITEM_TAG_DESIGNATOR_MAXIMUM,
	/* 6 is missing */
	ITEM_TAG_STRING_INDEX = 7,
	ITEM_TAG_STRING_MINIMUM,
	ITEM_TAG_STRING_MAXIMUM,
	ITEM_TAG_DELIMITER,
};

typedef struct
{
	unsigned char bSize:2, bType:2, bTag:4;
} item_prefix_bitfields;

typedef union
{
	item_prefix_bitfields b;
	unsigned char i;
} item_prefix;

#define	LONG_ITEM_PREFIX	0xfe

/*
	parsed item
*/

typedef struct
{
	int				offset;		/* byte offset in report descriptor */
	int				prefix, size, type, tag;
	unsigned long	value;
	signed long		signed_value;	/* sign extended: for logical min/max */
} decomp_item;

/*
	report "instruction"
*/

/* control types for game controllers */
enum
{
	TYPE_NONE, 
	TYPE_HAT, 
	TYPE_AXIS, TYPE_AXIS_X, TYPE_AXIS_Y, 
	TYPE_BUTTON
};

typedef struct usage_range
{
	// id_min == id_max for variables
	unsigned int page, id_min, id_max;
} usage_range;

typedef struct
{
	usage_range		usage;
	int				ctrl_type;		/* TYPE_*; set by count_controls() */
	int				attributes;

	BOOL			is_log_signed, is_phy_signed;
	unsigned long	log_min, log_max;
	unsigned long	phy_min, phy_max;
	signed long		signed_log_min, signed_log_max;
	signed long		signed_phy_min, signed_phy_max;

	/* used to extract bit-field from report */
	/* value = (report [byte_idx] >> bit_pos) & ((1 << num_bits) - 1) */
	/* N.B. you need sign extension if signed */
	int				byte_idx, bit_pos, num_bits;

} report_insn;

int decompose_report_descriptor
	(const unsigned char *desc,
	size_t desc_len,
	decomp_item *items,
	size_t *num_items);

int parse_report_descriptor
	(const decomp_item *items,
	size_t num_items,
	report_insn *insns,
	size_t *num_insns,
	size_t *total_report_size,
	int *first_report_id);

/*
	Usage Pages/IDs
*/

enum
{
	USAGE_PAGE_GENERIC_DESKTOP = 1,
	USAGE_PAGE_SIMULATION = 2,
	USAGE_PAGE_GAME = 5,
	USAGE_PAGE_KEYBOARD = 7,
	USAGE_PAGE_LED,
	USAGE_PAGE_BUTTONS,
};

/* Page 1: Generic Desktop */

enum
{
	USAGE_ID_POINTER = 1,
	USAGE_ID_MOUSE = 2,
	USAGE_ID_JOYSTICK = 4,
	USAGE_ID_GAMEPAD,
	USAGE_ID_KEYBOARD,
	USAGE_ID_KEYPAD,

	USAGE_ID_X = 0x30,
	USAGE_ID_Y,
	USAGE_ID_Z,
	USAGE_ID_RX,
	USAGE_ID_RY,
	USAGE_ID_RZ,
	USAGE_ID_SLIDER,
	USAGE_ID_DIAL,
	USAGE_ID_WHEEL,
	USAGE_ID_HAT_SWITCH,
};

/* Page 2: Simulation */

enum
{
	USAGE_ID_RUDDER = 0xBA,
	USAGE_ID_THROTTLE = 0xBB,
};

int count_controls
	(report_insn *insns,
	size_t num_insns,
	int *num_axes,
	int *num_hats,
	int *num_buttons);
