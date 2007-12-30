/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 *      Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "GlobalData.h"


void
SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count)
{
	uint32 i;
	blit_params *b;

	FifoBeginWrite();
	for (i = 0; i < count; i++) {
		b = &list[i];
#if 0
		TRACE("BLIT %dx%d, %dx%d->%dx%d\n", b->width + 1, b->height + 1,
			b->src_left, b->src_top, b->dest_left, b->dest_top);
#endif
		FifoWrite(SVGA_CMD_RECT_COPY);
		FifoWrite(b->src_left);
		FifoWrite(b->src_top);
		FifoWrite(b->dest_left);
		FifoWrite(b->dest_top);
		FifoWrite(b->width + 1);
		FifoWrite(b->height + 1);
	}
	FifoEndWrite();
	FifoSync();
}


void
FILL_RECTANGLE(engine_token *et, uint32 colorIndex,
	fill_rect_params *list, uint32 count)
{
	uint32 i;
	fill_rect_params *f;

	for (i = 0; i < count; i++) {
		f = &list[i];
#if 0
		TRACE("FILL %lX, %dx%d->%dx%d\n", colorIndex,
			f->left, f->top, f->right, f->bottom);
#endif
		/* TODO */
	}
}


void
INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count)
{
	/* TODO */
}


void
FILL_SPAN(engine_token * et, uint32 colorIndex, uint16 * list, uint32 count)
{
	/* TODO */
}

