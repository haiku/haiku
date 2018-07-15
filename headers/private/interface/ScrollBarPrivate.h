/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _SCROLL_BAR_PRIVATE_H
#define _SCROLL_BAR_PRIVATE_H

// Constants for determining which arrow is down and are defined with respect
// to double arrow mode. ARROW_1 and ARROW_4 refer to the outer pair of arrows
// while ARROW_2 and ARROW_3 refer to the inner ones. ARROW_1 point left/up
// and ARROW_4 points right/down. THUMB is the scroll bar thumb.
enum {
	SCROLL_NO_ARROW = -1,
	SCROLL_ARROW_1,
	SCROLL_ARROW_2,
	SCROLL_ARROW_3,
	SCROLL_ARROW_4,
	SCROLL_THUMB
};

// constants for determining which knob style to draw on scroll bar
typedef enum {
	KNOB_NONE = 0,
	KNOB_DOTS,
	KNOB_LINES
} knob_style;


#endif // _SCROLL_BAR_PRIVATE_H
