/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed.
*/

#define MODULE_BIT 0x40000000

// apsed, TODO ?? change interface of gx00_acc_* and use MGA pseudo DMA

#include "acc_std.h"

void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count) {
	int i;

	/*do each blit*/
	i=0;
	while (count--)
	{
		gx00_acc_blit
		(
			list[i].src_left,
			list[i].src_top,
			list[i].dest_left,
			list[i].dest_top,
			list[i].width,
			list[i].height
		);
		i++;
	}
}

//Not possible with G400 AFAIK (if anyone knows otherwise then please contact me)
//void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT(engine_token *et, scaled_blit_params *list, uint32 count) {
//typedef struct {
//	uint16	src_left;	/* guaranteed constrained to virtual width and height */
//	uint16	src_top;
//	uint16	src_width;	/* 0 to N, where zero means one pixel, one means two pixels, etc. */
//	uint16	src_height;	/* 0 to M, where zero means one line, one means two lines, etc. */
//	uint16	dest_left;
//	uint16	dest_top;
//	uint16	dest_width;	/* 0 to N, where zero means one pixel, one means two pixels, etc. */
//	uint16	dest_height;	/* 0 to M, where zero means one line, one means two lines, etc. */
//} scaled_blit_params;
//}

void SCREEN_TO_SCREEN_TRANSPARENT_BLIT(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count) {
	int i;

	/*do each blit*/
	i=0;
	while (count--)
	{
		gx00_acc_transparent_blit
		(
			list[i].src_left,
			list[i].src_top,
			list[i].dest_left,
			list[i].dest_top,
			list[i].width,
			list[i].height,
			transparent_colour
		);
		i++;
	}
}

void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count) {
	int i;

	/*draw each rectangle*/
	i=0;
	while (count--)
	{
		gx00_acc_rectangle
		(
			list[i].left,
			(list[i].right)+1,
			list[i].top,
			(list[i].bottom-list[i].top)+1,
			colorIndex
		);
		i++;
	}
}

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
