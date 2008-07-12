/*
	HID parser
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved

	platform independent
*/

#include <stdlib.h>
#include "hidparse.h"

/*
	decompose report descriptor into array of uniform structure
*/

int decompose_report_descriptor
	(const unsigned char *desc,
	size_t desc_len,
	decomp_item *items,
	size_t *num_items)
{
	int desc_idx, item_idx;
	static int item_size [4] = { 0, 1, 2, 4 };

	if (desc == NULL || items == NULL || num_items == NULL || 
		desc_len <= 0 || *num_items <= 0)
		return FAILURE;

	/* process one item per loop */
	for (desc_idx = item_idx = 0; 
		desc_idx < desc_len && item_idx < *num_items; 
		item_idx++)
	{
		item_prefix pfx;
		int size;
		unsigned short value16;
		unsigned char value8;
		
		decomp_item *item = &items [item_idx];
		item->offset = desc_idx;
		item->prefix = pfx.i = desc [desc_idx++];
		item->type = pfx.b.bType;

		if (pfx.i == LONG_ITEM_PREFIX)
		{
			/* XXX no long item support */
			item->size = desc [desc_idx++];
			item->tag  = desc [desc_idx++];
			item->value = item->signed_value = 0;
			desc_idx += item->size;
			continue;
		}

		item->size = size = item_size [pfx.b.bSize];
		item->tag = pfx.b.bTag;

		switch (size)
		{
		case 4:
			item->signed_value = 
			item->value = 
				((unsigned long) desc [desc_idx + 3] << 24) | 
				((unsigned long) desc [desc_idx + 2] << 16) |
				(                desc [desc_idx + 1] <<  8) |
				                 desc [desc_idx    ];
			break;
		case 2:
			value16 = (desc [desc_idx + 1] << 8) | desc [desc_idx];
			item->value = value16;
			item->signed_value = (signed short) value16;
			break;
		case 1:
			value8 = desc [desc_idx];
			item->value = value8;
			item->signed_value = (signed char) value8;
			break;
		case 0:
			item->value = item->signed_value = 0;
			break;
		}

		desc_idx += size;
	}

	*num_items = item_idx;
	return SUCCESS;
}

/*
	parse decomposed items and generate "instructions" to interpret a report

	limitations:
	only frequently used items are supported
	only first collection is used
	report ID is abandoned
	output/feature reports are not implemented
*/

#ifndef	min
#define	min(a,b)	((a)<(b)?(a):(b))
#endif

int parse_report_descriptor
	(const decomp_item *items,
	size_t num_items,
	report_insn *insns,
	size_t *num_insns,
	size_t *total_report_size,
	int *first_report_id)
{
	#define	ITEM_STACK_SIZE	32
	#define	USAGE_QUEUE_SIZE	256

	/* global items */
	typedef struct global_items
	{
		unsigned int usage_page;
		BOOL is_log_signed, is_phy_signed;
		unsigned long log_max, log_min,
			phy_max, phy_min;
		signed long signed_log_max, signed_log_min, 
			signed_phy_max, signed_phy_min;
		int report_size, report_id, report_count;
	} global_items;

	global_items item_state, item_stack [ITEM_STACK_SIZE];
	usage_range usage_queue [USAGE_QUEUE_SIZE];
	int item_sp = ITEM_STACK_SIZE, usage_qhead = 0, usage_qtail = 0;

	/* local items */
//	unsigned int usage_id, usage_min, usage_max;

	int bit_pos = 0, byte_idx = 0, insn_idx, item_idx, i, collection = 0;

	if (items == NULL || insns == NULL || num_insns == NULL ||
		*num_insns <= 0 || num_items <= 0)
		return FAILURE;

	memset (&item_state, 0, sizeof (item_state));

	/* interpret one item per loop */
	for (insn_idx = item_idx = 0; 
		item_idx < num_items && insn_idx < *num_insns; 
		item_idx++)
	{
		const decomp_item *item = &items [item_idx];
		report_insn *insn = &insns [insn_idx];
		int tag = item->tag;
		switch (item->type)
		{
		case ITEM_TYPE_MAIN:
			switch (tag)
			{
			case ITEM_TAG_INPUT:
				for (i = 0; i < item_state.report_count && insn_idx < *num_insns; i++)
				{
					if ((item->value & 1) == 0)		/* data */
					{
						if (item->value & 2)	/* variable */
						{
							if (usage_qhead < usage_qtail)
							{
								usage_range *u = &usage_queue [usage_qhead];
								insn->usage.page = u->page;
								insn->usage.id_min = 
								insn->usage.id_max = u->id_min;
								if (u->id_min < u->id_max)
									u->id_min++;
								else
									usage_qhead++;
							}
							else
							{
								/*
									XXX
									usages are fewer than report count
									queue exhausted: repeat last values
								*/
								usage_range *u = &usage_queue [usage_qhead - 1];
								insn->usage.page = u->page;
								insn->usage.id_min = 
								insn->usage.id_max = u->id_min;
							}
						}
						else
						{
							if (usage_qhead < usage_qtail)
							{
								usage_range *u = &usage_queue [usage_qhead];
								if (u->id_min < u->id_max)
								{
									/* usage min / max */
									insn->usage = *u;
									usage_qhead++;
								}
								else
								{
									/*
										XXX
										sequence of usage IDs
										assume currently queued IDs are continuous
										and use all of them
									*/
									insn->usage.page = u->page;
									insn->usage.id_min = u->id_min;
									insn->usage.id_max = usage_queue [usage_qtail - 1].id_max;
									usage_qhead = usage_qtail = 0;
								}
							}
							/*
								XXX
								queue exhausted
							*/
						}

						insn->attributes = item->value;

						insn->is_log_signed = item_state.is_log_signed;
						if (item_state.is_log_signed)
						{
							insn->signed_log_min = item_state.signed_log_min;
							insn->signed_log_max = item_state.signed_log_max;
						}
						else
						{
							insn->log_min = item_state.log_min;
							insn->log_max = item_state.log_max;
						}

						insn->is_phy_signed = item_state.is_phy_signed;
						if (item_state.is_phy_signed)
						{
							insn->signed_phy_min = item_state.signed_phy_min;
							insn->signed_phy_max = item_state.signed_phy_max;
						}
						else
						{
							insn->phy_min = item_state.phy_min;
							insn->phy_max = item_state.phy_max;
						}

						insn->byte_idx = byte_idx;
						insn->bit_pos = bit_pos;
						insn->num_bits = item_state.report_size;

						insn++;
						insn_idx++;
					}

					/* to next bit-field */
					bit_pos += item_state.report_size;
					if (bit_pos >= 8)
					{
						byte_idx += bit_pos / 8;
						bit_pos = bit_pos % 8;
					}
				}
				break;

			case ITEM_TAG_OUTPUT:
				/* XXX similar to input */
				break;
			case ITEM_TAG_COLLECTION:
				collection++;
				// XXX
				usage_qhead = usage_qtail = 0;
				break;
			case ITEM_TAG_FEATURE:
				/* XXX similar to input */
				break;
			case ITEM_TAG_END_COLLECTION:
				if (--collection == 0)
					/* exit at end of first collection */
					goto end;
				break;
			default:
				break;
			}
			break;

		case ITEM_TYPE_GLOBAL:
			switch (tag)
			{
			case ITEM_TAG_USAGE_PAGE:
				item_state.usage_page = item->value;
				break;
			case ITEM_TAG_LOGICAL_MINIMUM:
				item_state.log_min = item->value;
				item_state.signed_log_min = item->signed_value;
				/* FALLTHROUGH */
			case ITEM_TAG_PHYSICAL_MINIMUM:
				item_state.phy_min = item->value;
				item_state.signed_phy_min = item->signed_value;
				break;
			case ITEM_TAG_LOGICAL_MAXIMUM:
				item_state.log_max = item->value;
				item_state.signed_log_max = item->signed_value;
				item_state.is_log_signed = item_state.log_min > item_state.log_max;
				/* FALLTHROUGH */
			case ITEM_TAG_PHYSICAL_MAXIMUM:
				item_state.phy_max = item->value;
				item_state.signed_phy_max = item->signed_value;
				item_state.is_phy_signed = item_state.phy_min > item_state.phy_max;
				break;
			case ITEM_TAG_UNIT_EXPONENT:
				break;
			case ITEM_TAG_UNIT:
				break;
			case ITEM_TAG_REPORT_SIZE:
				item_state.report_size = item->value;
				break;
			case ITEM_TAG_REPORT_ID:
				/* FIXME */
				if (item_state.report_id == 0)
				{
					item_state.report_id = item->value;
					byte_idx++;
				}
				else
					goto end;
				break;
			case ITEM_TAG_REPORT_COUNT:
				item_state.report_count = item->value;
				break;
			case ITEM_TAG_PUSH:
				if (item_sp > 0)
					item_stack [--item_sp] = item_state;
				break;
			case ITEM_TAG_POP:
				if (item_sp < ITEM_STACK_SIZE)
					item_state = item_stack [item_sp++];
				break;
			}
			break;

		case ITEM_TYPE_LOCAL:
			switch (tag)
			{
			case ITEM_TAG_USAGE:
				/* can be sequentially used */
				/* XXX should be limited to "practical" usages */
				/* XXX should support extended (32-bit) usage */
				if (usage_qtail < USAGE_QUEUE_SIZE)
				{
					usage_queue [usage_qtail].page = item_state.usage_page;
					usage_queue [usage_qtail].id_min =
					usage_queue [usage_qtail].id_max = item->value;
					usage_qtail++;
				}
				break;
			case ITEM_TAG_USAGE_MINIMUM:
#if 0
				usage_min = item->value;
#endif
				if (usage_qtail < USAGE_QUEUE_SIZE)
				{
					usage_queue [usage_qtail].page = item_state.usage_page;
					usage_queue [usage_qtail].id_min = item->value;
				}
				break;
			case ITEM_TAG_USAGE_MAXIMUM:
#if 0
				usage_max = item->value;
#endif
				if (usage_qtail < USAGE_QUEUE_SIZE)
				{
					usage_queue [usage_qtail].id_max = item->value;
					usage_qtail++;
				}
				break;
			case ITEM_TAG_DESIGNATOR_INDEX:
				break;
			case ITEM_TAG_DESIGNATOR_MINIMUM:
				break;
			case ITEM_TAG_DESIGNATOR_MAXIMUM:
				break;
			case ITEM_TAG_STRING_INDEX:
				break;
			case ITEM_TAG_STRING_MINIMUM:
				break;
			case ITEM_TAG_STRING_MAXIMUM:
				break;
			case ITEM_TAG_DELIMITER:
				break;
			}
			break;
		}
	}

end:
	*num_insns = insn_idx;
	if (total_report_size != NULL)
		*total_report_size = byte_idx;
	if (first_report_id != NULL)
		*first_report_id = item_state.report_id;
	return SUCCESS;
}

/*
	designed for game controllers
*/

int count_controls
	(report_insn *insns,
	size_t num_insns,
	int *num_axes,
	int *num_hats,
	int *num_buttons)
{
	int insn_idx, hats = 0, buttons = 0,
		axes = 2;		/* X and Y axes are reserved */

	if (insns == NULL || num_insns <= 0)
		return FAILURE;

	for (insn_idx = 0; insn_idx < num_insns; insn_idx++)
	{
		report_insn *insn = &insns [insn_idx];
		insn->ctrl_type = TYPE_NONE;
		switch (insn->usage.page)
		{
		case USAGE_PAGE_GENERIC_DESKTOP:
			switch (insn->usage.id_min)
			{
			case USAGE_ID_X:
				insn->ctrl_type = TYPE_AXIS_X;
				break;

			case USAGE_ID_Y:
				insn->ctrl_type = TYPE_AXIS_Y;
				break;

			case USAGE_ID_Z:
			case USAGE_ID_RX:
			case USAGE_ID_RY:
			case USAGE_ID_RZ:
			case USAGE_ID_SLIDER:
			case USAGE_ID_DIAL:
			case USAGE_ID_WHEEL:
				axes++;
				insn->ctrl_type = TYPE_AXIS;
				break;

			case USAGE_ID_HAT_SWITCH:
				hats++;
				insn->ctrl_type = TYPE_HAT;
				break;
			}
			break;

		case USAGE_PAGE_SIMULATION:
			switch (insn->usage.id_min)
			{
			case USAGE_ID_RUDDER:
			case USAGE_ID_THROTTLE:
				axes++;
				insn->ctrl_type = TYPE_AXIS;
				break;
			}
			break;

		case USAGE_PAGE_BUTTONS:
			if (insn->attributes & 2)
			{
				buttons++;
			}
			else
			{
				buttons += insn->log_max - insn->log_min + 1;
			}
			insn->ctrl_type = TYPE_BUTTON;
			break;
		}
	}

	if (num_axes != NULL)
		*num_axes = axes;
	if (num_hats != NULL)
		*num_hats = hats;
	if (num_buttons != NULL)
		*num_buttons = buttons;
	return SUCCESS;
}

