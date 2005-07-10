/* Acceleration functions */
/* Author:
   Rudolf Cornelissen 8/2003-11/2004.
*/

#define MODULE_BIT 0x00080000

#include "std.h"

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

status_t eng_acc_wait_idle()
{
	/* wait until engine completely idle */
//	while (ACCR(STATUS))
	while (0)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100); 
	}

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t eng_acc_init()
{
	/*** Set pixel width and format ***/
	switch(si->dm.space)
	{
	case B_CMAP8:
		break;
	case B_RGB15_LITTLE:
		break;
	case B_RGB16_LITTLE:
		break;
	case B_RGB32_LITTLE:case B_RGBA32_LITTLE:
		break;
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/*** setup screen location and pitch ***/
	switch (si->ps.card_arch)
	{
	default:
		/* location of active screen in framebuffer */
//		ACCW(OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));

		/* setup buffer pitch */
//		ACCW(PITCH0, (si->fbc.bytes_per_row & 0x0000ffff));
		break;
	}

	/* do first actual acceleration engine command:
	 * setup clipping region (workspace size) to 32768 x 32768 pixels:
	 * wait for room in fifo for clipping cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
//	while (((ENG_REG16(RG16_CLP_FIFOFREE)) >> 2) < 2)
	while (0)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup clipping (writing 2 32bit words) */
//	ACCW(CLP_TOPLEFT, 0x00000000);
//	ACCW(CLP_WIDHEIGHT, 0x80008000);

	return B_OK;
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t eng_acc_setup_blit()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_PAT_FIFOFREE)) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	ACCW(PAT_SHAPE, 0); /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	ACCW(PAT_COLOR0, 0xffffffff);
	ACCW(PAT_COLOR1, 0xffffffff);
	ACCW(PAT_MONO1, 0xffffffff);
	ACCW(PAT_MONO2, 0xffffffff);

	/* ROP3 registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_ROP_FIFOFREE)) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup ROP (writing 1 32bit word) */
	ACCW(ROP_ROP3, 0xcc);

	return B_OK;
}

status_t eng_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* Note: blit-copy direction is determined inside riva hardware: no setup needed */

	/* instruct engine what to blit:
	 * wait for room in fifo for blit cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_BLT_FIFOFREE)) >> 2) < 3)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup blit (writing 3 32bit words) */
	ACCW(BLT_TOPLFTSRC, ((ys << 16) | xs));
	ACCW(BLT_TOPLFTDST, ((yd << 16) | xd));
	ACCW(BLT_SIZE, (((h + 1) << 16) | (w + 1)));

	return B_OK;
}

/* rectangle fill - i.e. workspace and window background color */
/* span fill - i.e. (selected) menuitem background color (Dano) */
status_t eng_acc_setup_rectangle(uint32 color)
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_PAT_FIFOFREE)) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	ACCW(PAT_SHAPE, 0); /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	ACCW(PAT_COLOR0, 0xffffffff);
	ACCW(PAT_COLOR1, 0xffffffff);
	ACCW(PAT_MONO1, 0xffffffff);
	ACCW(PAT_MONO2, 0xffffffff);

	/* ROP3 registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_ROP_FIFOFREE)) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup ROP (writing 1 32bit word) for GXcopy */
	ACCW(ROP_ROP3, 0xcc);

	/* setup fill color:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_BMP_FIFOFREE)) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup color (writing 1 32bit word) */
	ACCW(BMP_COLOR1A, color);

	return B_OK;
}

status_t eng_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to fill:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_BMP_FIFOFREE)) >> 2) < 2)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup fill (writing 2 32bit words) */
	ACCW(BMP_UCRECTL_0, ((xs << 16) | (ys & 0x0000ffff)));
	ACCW(BMP_UCRECSZ_0, (((xe - xs) << 16) | (yl & 0x0000ffff)));

	return B_OK;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t eng_acc_setup_rect_invert()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_PAT_FIFOFREE)) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	ACCW(PAT_SHAPE, 0); /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	ACCW(PAT_COLOR0, 0xffffffff);
	ACCW(PAT_COLOR1, 0xffffffff);
	ACCW(PAT_MONO1, 0xffffffff);
	ACCW(PAT_MONO2, 0xffffffff);

	/* ROP3 registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_ROP_FIFOFREE)) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup ROP (writing 1 32bit word) for GXinvert */
	ACCW(ROP_ROP3, 0x55);

	/* reset fill color:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_BMP_FIFOFREE)) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now reset color (writing 1 32bit word) */
	ACCW(BMP_COLOR1A, 0);

	return B_OK;
}

status_t eng_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to invert:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((ENG_REG16(RG16_BMP_FIFOFREE)) >> 2) < 2)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup invert (writing 2 32bit words) */
	ACCW(BMP_UCRECTL_0, ((xs << 16) | (ys & 0x0000ffff)));
	ACCW(BMP_UCRECSZ_0, (((xe - xs) << 16) | (yl & 0x0000ffff)));

	return B_OK;
}

/* screen to screen tranparent blit */
status_t eng_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	//fixme: implement.

	return B_ERROR;
}

/* screen to screen scaled filtered blit - i.e. scale video in memory */
status_t eng_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd)
{
	//fixme: implement.

	return B_ERROR;
}
