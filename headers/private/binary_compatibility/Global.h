/*
 * Copyright 2008, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2008-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BINARY_COMPATIBILITY_GLOBAL_H_
#define _BINARY_COMPATIBILITY_GLOBAL_H_


#if __GNUC__ == 2
#	define B_IF_GCC_2(ifBlock, elseBlock)	ifBlock
#else
#	define B_IF_GCC_2(ifBlock, elseBlock)	elseBlock
#endif


// method codes
enum {
	// app kit

	// interface kit
	PERFORM_CODE_MIN_SIZE				= 1000,
	PERFORM_CODE_MAX_SIZE				= 1001,
	PERFORM_CODE_PREFERRED_SIZE			= 1002,
	PERFORM_CODE_LAYOUT_ALIGNMENT		= 1003,
	PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH	= 1004,
	PERFORM_CODE_GET_HEIGHT_FOR_WIDTH	= 1005,
	PERFORM_CODE_SET_LAYOUT				= 1006,
	PERFORM_CODE_LAYOUT_INVALIDATED		= 1007,
	PERFORM_CODE_DO_LAYOUT				= 1008,
	PERFORM_CODE_GET_TOOL_TIP_AT		= 1009,

	// support kit
	PERFORM_CODE_ALL_ARCHIVED			= 1010,
	PERFORM_CODE_ALL_UNARCHIVED			= 1011,

	PERFORM_CODE_LAYOUT_CHANGED			= 1012
};


#endif // _BINARY_COMPATIBILITY__GLOBAL_H_
