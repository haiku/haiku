/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Rudolf Cornelissen 9/2003-1/2005.
*/

/*
	note:
	attempting DMA on NV40 and higher because without it I can't get it going ATM.
	Later on this can become a nv.settings switch, and maybe later we can even
	forget about non-DMA completely (depends on 3D acceleration attempts).
*/

#define MODULE_BIT 0x40000000

#include "acc_std.h"

void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count)
{
	int i;

	if(si->ps.card_arch < NV40A)
	{
		/* init acc engine for blit function */
		nv_acc_setup_blit();

		/* do each blit */
		i=0;
		while (count--)
		{
			nv_acc_blit
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
	else
	{
		/* init acc engine for blit function */
		nv_acc_setup_blit_dma();

		/* do each blit */
		i=0;
		while (count--)
		{
			nv_acc_blit_dma
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
}

void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT(engine_token *et, scaled_blit_params *list, uint32 count)
{
	int i;

	/* do each blit */
	i=0;
	while (count--)
	{
		nv_acc_video_blit
		(
			list[i].src_left,
			list[i].src_top,
			list[i].src_width,
			list[i].src_height,
			list[i].dest_left,
			list[i].dest_top,
			list[i].dest_width,
			list[i].dest_height
		);
		i++;
	}
}

void SCREEN_TO_SCREEN_TRANSPARENT_BLIT(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count)
{
	int i;

	/* do each blit */
	i=0;
	while (count--)
	{
		nv_acc_transparent_blit
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

void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count)
{
	int i;

	if(si->ps.card_arch < NV40A)
	{
		/* init acc engine for fill function */
		nv_acc_setup_rectangle(colorIndex);

		/* draw each rectangle */
		i=0;
		while (count--)
		{
			nv_acc_rectangle
			(
				list[i].left,
				(list[i].right)+1,
				list[i].top,
				(list[i].bottom-list[i].top)+1
			);
			i++;
		}
	}
	else
	{
		/* init acc engine for fill function */
		nv_acc_setup_rectangle_dma(colorIndex);

		/* draw each rectangle */
		i=0;
		while (count--)
		{
			nv_acc_rectangle_dma
			(
				list[i].left,
				(list[i].right)+1,
				list[i].top,
				(list[i].bottom-list[i].top)+1
			);
			i++;
		}
	}
}

void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count)
{
	int i;

	if(si->ps.card_arch < NV40A)
	{
		/* init acc engine for invert function */
		nv_acc_setup_rect_invert();

		/* invert each rectangle */
		i=0;
		while (count--)
		{
			nv_acc_rectangle_invert
			(
				list[i].left,
				(list[i].right)+1,
				list[i].top,
				(list[i].bottom-list[i].top)+1
			);
			i++;
		}
	}
	else
	{
		/* init acc engine for invert function */
		nv_acc_setup_rect_invert_dma();

		/* invert each rectangle */
		i=0;
		while (count--)
		{
			nv_acc_rectangle_invert_dma
			(
				list[i].left,
				(list[i].right)+1,
				list[i].top,
				(list[i].bottom-list[i].top)+1
			);
			i++;
		}
	}
}

void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count)
{
	int i;

	if(si->ps.card_arch < NV40A)
	{
		/* init acc engine for fill function */
		nv_acc_setup_rectangle(colorIndex);

		/* draw each span */
		i=0;
		while (count--)
		{
			nv_acc_rectangle
			(
				list[i+1],
				list[i+2]+1,
				list[i],
				1
			);
			i+=3;
		}
	}
	else
	{
		/* init acc engine for fill function */
		nv_acc_setup_rectangle_dma(colorIndex);

		/* draw each span */
		i=0;
		while (count--)
		{
			nv_acc_rectangle_dma
			(
				list[i+1],
				list[i+2]+1,
				list[i],
				1
			);
			i+=3;
		}
	}
}
