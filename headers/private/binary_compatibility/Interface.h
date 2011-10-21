/*
 * Copyright 2008, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BINARY_COMPATIBILITY_INTERFACE_H_
#define _BINARY_COMPATIBILITY_INTERFACE_H_


#include <binary_compatibility/Global.h>


struct perform_data_min_size {
	BSize	return_value;
};

struct perform_data_max_size {
	BSize	return_value;
};

struct perform_data_preferred_size {
	BSize	return_value;
};

struct perform_data_layout_alignment {
	BAlignment	return_value;
};

struct perform_data_has_height_for_width {
	bool	return_value;
};

struct perform_data_get_height_for_width {
	float	width;
	float	min;
	float	max;
	float	preferred;
};

struct perform_data_set_layout {
	BLayout*	layout;
};

struct perform_data_layout_invalidated {
	bool	descendants;
};

struct perform_data_get_tool_tip_at {
	BPoint		point;
	BToolTip**	tool_tip;
	bool		return_value;
};

#endif /* _BINARY_COMPATIBILITY_INTERFACE_H_ */
