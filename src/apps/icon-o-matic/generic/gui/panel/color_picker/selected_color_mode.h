/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *		
 */

#ifndef	_SELECTED_COLOR_MODE_H
#define _SELECTED_COLOR_MODE_H

enum selected_color_mode {
	R_SELECTED = 0x01,
	G_SELECTED = 0x02,
	B_SELECTED = 0x04,
	H_SELECTED = 0x10,
	S_SELECTED = 0x20,
	V_SELECTED = 0x40
};

#endif // _SELECTED_COLOR_MODE_H
