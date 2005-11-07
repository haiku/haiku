/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FONT_PRIVATE_H
#define FONT_PRIVATE_H


// extra flags (shares B_IS_FIXED and B_HAS_TUNED_FONT)
enum {
	B_PRIVATE_FONT_IS_FULL_AND_HALF_FIXED	= 0x04,
	B_PRIVATE_FONT_HAS_KERNING				= 0x08,
	B_PRIVATE_FONT_DIRECTION_MASK			= 0x30,
};

#define B_PRIVATE_FONT_DIRECTION_SHIFT 8

#endif	/* FONT_PRIVATE_H */
