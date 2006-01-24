/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed,
	Rudolf Cornelissen 1/2006.
*/

#define MODULE_BIT 0x40000000

#include "acc_std.h"

void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count) {
	int i;

	/*draw each rectangle*/
	i=0;
	while (count--)
	{
		gx00_acc_rectangle_invert
		(
			list[i].left,
			(list[i].right)+1,
			list[i].top,
			(list[i].bottom-list[i].top)+1,
			0
		);
		i++;
	}
}

void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count) {
	int i;

	/*draw each span*/
	i=0;
	while (count--)
	{
		gx00_acc_rectangle
		(
			list[i+1],
			list[i+2]+1,
			list[i],
			1,
			colorIndex
		);
		i+=3;
	}
}
