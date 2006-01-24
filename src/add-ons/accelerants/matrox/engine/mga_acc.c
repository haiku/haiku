/* MGA Acceleration functions */
/* Authors:
   Mark Watson 2/2000,
   Rudolf Cornelissen 10/2002-1/2006.
*/

#define MODULE_BIT 0x00080000

#include "mga_std.h"

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

/* needed by MIL 1/2 because of adress linearisation constraints */
#define ACCW_YDSTLEN(dst, len) do { \
	if (si->engine.y_lin) { \
		ACCW(YDST,((dst)* (si->fbc.bytes_per_row / (si->engine.depth >> 3))) >> 5); \
		ACCW(LEN,len); \
	} else ACCW(YDSTLEN,((dst)<<16)|(len)); \
} while (0)

status_t gx00_acc_wait_idle()
{
	/* wait until engine completely idle */
	while (ACCR(STATUS) & 0x00010000)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100); 
	}

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t gx00_acc_init()
{
	/* used for convenience: MACCESS is a write only register! */
	uint32 maccess = 0x00000000;
	/* if we were unable to read PINS, we have to assume something (keeping bit6 zero) */
	if ((si->ps.card_type >= G450) && (si->ps.pins_status = B_OK))
	{
		/* b7 v5_mem_type = done by Mark Watson. fixme: still confirm! (unknown bits) */
		maccess |= ((((uint32)si->ps.v5_mem_type) & 0x80) >> 1);
	}

	/* preset using hardware adress linearisation */
	si->engine.y_lin = 0x00;
	/* reset depth */
	si->engine.depth = 0;

	/* cleanup bitblt */
	ACCW(OPMODE,0);

	/* Set the Z origin to the start of FB (otherwise lockup on blits) */
	ACCW(ZORG,0);

	/* Set pixel width */
	switch(si->dm.space)
	{
	case B_CMAP8:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x00));
		si->engine.depth = 8;
		break;
	case B_RGB15_LITTLE:case B_RGB16_LITTLE:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x01)); 
		si->engine.depth = 16;
		break;
	case B_RGB32_LITTLE:case B_RGBA32_LITTLE:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x02));
		si->engine.depth = 32;
		break;
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* setup PITCH: very cardtype specific! */
	switch (si->ps.card_type)
	{
	case MIL1:
		switch (si->fbc.bytes_per_row / (si->engine.depth >> 3))
		{
			case 640:
			case 768:
			case 800:
			case 960:
			case 1024:
			case 1152:
			case 1280:
			case 1600:
			case 1920:
			case 2048:
				/* we are using hardware adress linearisation */
				break;
			default:
				/* we are using software adress linearisation */
				si->engine.y_lin = 0x01;
				LOG(8,("ACC: using software adress linearisation\n"));
				break;
		}
		ACCW(PITCH, (si->engine.y_lin << 15) |
					((si->fbc.bytes_per_row / (si->engine.depth >> 3)) & 0x0FFF));
		break;
	case MIL2:
		switch (si->fbc.bytes_per_row / (si->engine.depth >> 3))
		{
			case 512:
			case 640:
			case 768:
			case 800:
			case 832:
			case 960:
			case 1024:
			case 1152:
			case 1280:
			case 1600:
			case 1664:
			case 1920:
			case 2048:
				/* we are using hardware adress linearisation */
				break;
			default:
				/* we are using software adress linearisation */
				si->engine.y_lin = 0x01;
				LOG(8,("ACC: using software adress linearisation\n"));
				break;
		}
		ACCW(PITCH, (si->engine.y_lin << 15) |
					((si->fbc.bytes_per_row / (si->engine.depth >> 3)) & 0x0FFF));
		break;
	case G100:
		/* always using hardware adress linearisation, because 2D/3D
		 * engine works on every pitch multiple of 32 */
		ACCW(PITCH, ((si->fbc.bytes_per_row / (si->engine.depth >> 3)) & 0x0FFF));
		break;
	default:
		/* G200 and up are equal.. */
		/* always using hardware adress linearisation, because 2D/3D
		 * engine works on every pitch multiple of 32 */
		ACCW(PITCH, ((si->fbc.bytes_per_row / (si->engine.depth >> 3)) & 0x1FFF));
		break;
	}

	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);

	if (si->ps.card_type >= G200) {
		/*DSTORG - location of active screen in framebuffer*/
		ACCW(DSTORG,((uint8*)si->fbc.frame_buffer) - ((uint8*)si->framebuffer));

		/*SRCORG - init source address - same as dest*/
		ACCW(SRCORG,((uint8*)si->fbc.frame_buffer) - ((uint8*)si->framebuffer));
	}

	/* init YDSTORG - apsed, if not inited, BitBlts may fails on <= G200 */
	si->engine.src_dst = 0;
	ACCW(YDSTORG, si->engine.src_dst);

	/* <= G100 uses this register as SRCORG/DSTORG replacement, but
	 * MIL 1/2 does not need framebuffer space for the hardcursor! */
	if ((si->ps.card_type == G100) && (si->settings.hardcursor))
	{
		switch (si->dm.space)
		{
			case B_CMAP8:
				si->engine.src_dst = 1024 / 1;
				break;
			case B_RGB15_LITTLE:
			case B_RGB16_LITTLE:
				si->engine.src_dst = 1024 / 2;
				break;
			case B_RGB32_LITTLE:
				si->engine.src_dst =  1024 / 4;
				break;
			default:
				LOG(8,("ACC: G100 hardcursor not supported for current colorspace\n"));
				return B_ERROR;
		}		
	}
	ACCW(YDSTORG, si->engine.src_dst);

	/* clipping */
	/* i.e. highest and lowest X pixel adresses */
	ACCW(CXBNDRY,(((si->fbc.bytes_per_row / (si->engine.depth >> 3)) - 1) << 16) | (0));

	/* Y pixel addresses must be linear */
	/* lowest adress */
	ACCW(YTOP, 0 + si->engine.src_dst);
	/* highest adress */
	ACCW(YBOT,((si->dm.virtual_height - 1) *
		(si->fbc.bytes_per_row / (si->engine.depth >> 3))) + si->engine.src_dst);

	return B_OK;
}


/*
	note:
	moved acceleration 'top-level' routines to be integrated in the engine:
	it is costly to call the engine for every single function within a loop!
	(measured with BeRoMeter 1.2.6: upto 15% speed increase on all CPU's.)
*/

/* screen to screen blit - i.e. move windows around.
 * Engine function bitblit, paragraph 4.5.7.2 */
void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;
	int i = 0;

	/* calc offset 'per line' */
	offset = (si->fbc.bytes_per_row / (si->engine.depth >> 3));

	while (count--)
	{
		/* find where the top and bottom are */
		t_end = t_start =
			list[i].src_left + (offset * list[i].src_top) + si->engine.src_dst;
		t_end += list[i].width;

		b_end = b_start =
			list[i].src_left + (offset * (list[i].src_top + list[i].height)) + si->engine.src_dst;
		b_end += list[i].width;

		/* sgnzero bit _must_ be '0' before accessing SGN! */
		ACCW(DWGCTL, 0x00000000);

		/*find which quadrant */
		switch((list[i].dest_top > list[i].src_top) | ((list[i].dest_left > list[i].src_left) << 1))
		{
		case 0: /*L->R,down*/ 
			ACCW(SGN, 0);
			ACCW(AR3, t_start);
			ACCW(AR0, t_end);
			ACCW(AR5, offset);
			ACCW_YDSTLEN(list[i].dest_top, list[i].height + 1);
			break;
		case 1: /*L->R,up*/
			ACCW(SGN, 4);
			ACCW(AR3, b_start);
			ACCW(AR0, b_end);
			ACCW(AR5, -offset);
			ACCW_YDSTLEN(list[i].dest_top + list[i].height, list[i].height + 1);
			break;
		case 2: /*R->L,down*/
			ACCW(SGN, 1);
			ACCW(AR3, t_end);
			ACCW(AR0, t_start);
			ACCW(AR5, offset);
			ACCW_YDSTLEN(list[i].dest_top, list[i].height + 1);
			break;
		case 3: /*R->L,up*/
			ACCW(SGN, 5);
			ACCW(AR3, b_end);
			ACCW(AR0, b_start);
			ACCW(AR5, -offset);
			ACCW_YDSTLEN(list[i].dest_top + list[i].height, list[i].height + 1);
			break;
		}
		ACCW(FXBNDRY,((list[i].dest_left + list[i].width) << 16) | list[i].dest_left);

		/* start the blit */
		ACCGO(DWGCTL, 0x040c4018); // atype RSTR
		i++;
	}
}

/* screen to screen tranparent blit - not sure what uses this.
 * Engine function bitblit, paragraph 4.5.7.2 */
//WARNING:
//yet untested function!!
void SCREEN_TO_SCREEN_TRANSPARENT_BLIT(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;
	int i = 0;

	/* calc offset 'per line' */
	offset = (si->fbc.bytes_per_row / (si->engine.depth >> 3));

	while (count--)
	{
		/* find where the top and bottom are */
		t_end = t_start =
			list[i].src_left + (offset * list[i].src_top) + si->engine.src_dst;
		t_end += list[i].width;

		b_end = b_start =
			list[i].src_left + (offset * (list[i].src_top + list[i].height)) + si->engine.src_dst;
		b_end += list[i].width;

		/* sgnzero bit _must_ be '0' before accessing SGN! */
		ACCW(DWGCTL, 0x00000000);

		/*find which quadrant */
		switch((list[i].dest_top > list[i].src_top) | ((list[i].dest_left > list[i].src_left) << 1))
		{
		case 0: /*L->R,down*/ 
			ACCW(SGN, 0);
			ACCW(AR3, t_start);
			ACCW(AR0, t_end);
			ACCW(AR5, offset);
			ACCW_YDSTLEN(list[i].dest_top, list[i].height + 1);
			break;
		case 1: /*L->R,up*/
			ACCW(SGN, 4);
			ACCW(AR3, b_start);
			ACCW(AR0, b_end);
			ACCW(AR5, -offset);
			ACCW_YDSTLEN(list[i].dest_top + list[i].height, list[i].height + 1);
			break;
		case 2: /*R->L,down*/
			ACCW(SGN, 1);
			ACCW(AR3, t_end);
			ACCW(AR0, t_start);
			ACCW(AR5, offset);
			ACCW_YDSTLEN(list[i].dest_top, list[i].height + 1);
			break;
		case 3: /*R->L,up*/
			ACCW(SGN, 5);
			ACCW(AR3, b_end);
			ACCW(AR0, b_start);
			ACCW(AR5, -offset);
			ACCW_YDSTLEN(list[i].dest_top + list[i].height, list[i].height + 1);
			break;
		}
		ACCW(FXBNDRY,((list[i].dest_left + list[i].width) << 16) | list[i].dest_left);

		/* start the blit */
		ACCW(FCOL, transparent_colour);
		ACCW(BCOL, 0xffffffff);
		ACCGO(DWGCTL, 0x440c4018); // atype RSTR
		i++;
	}
}

/* screen to screen scaled filtered blit - i.e. scale video in memory.
 * Engine function texture mapping for video, paragraphs 4.5.5.5 - 4.5.5.9 */
//fixme: implement...
void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT(engine_token *et, scaled_blit_params *list, uint32 count)
{
	int i = 0;

	while (count--)
	{
/*
			list[i].src_left,
			list[i].src_top,
			list[i].src_width,
			list[i].src_height,
			list[i].dest_left,
			list[i].dest_top,
			list[i].dest_width,
			list[i].dest_height
*/
		i++;
	}
}

/* rectangle fill.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/
	int i = 0;

	while (count--)
	{
		ACCW(FXBNDRY, (((list[i].right + 1) << 16) | list[i].left));
		ACCW_YDSTLEN(list[i].top, ((list[i].bottom - list[i].top) + 1));
		ACCW(FCOL, colorIndex);

		/* start the fill */
//acc fixme: checkout blockmode constraints for G100+ (mil: nc?): also add blockmode
//	         for other functions, and use fastblt on MIL1/2 if possible...
//or is CMAP8 contraint a non-blockmode contraint? (linearisation problem maybe?)
		if ((si->dm.space == B_CMAP8) || si->ps.sdram)
		{
			ACCGO(DWGCTL, 0x400c7814); // atype RSTR
		}
		else
		{
			ACCGO(DWGCTL, 0x400c7844); // atype BLK 
		}
		i++;
	}
}

/* horizontal span fill.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
//(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/
	int i = 0;

	while (count--)
	{
		ACCW(FXBNDRY, ((list[i + 2] + 1) << 16)| list[i + 1]);
		ACCW_YDSTLEN(list[i], 1);
		ACCW(FCOL, colorIndex);

		/* start the fill */
//acc fixme: checkout blockmode constraints for G100+ (mil: nc?): also add blockmode
//	         for other functions, and use fastblt on MIL1/2 if possible...
//or is CMAP8 contraint a non-blockmode contraint? (linearisation problem maybe?)
		if ((si->dm.space == B_CMAP8) || si->ps.sdram)
		{
			ACCGO(DWGCTL, 0x400c7814); // atype RSTR
		}
		else
		{
			ACCGO(DWGCTL, 0x400c7844); // atype BLK
		}
		i += 3;
	}
}

/* rectangle invert.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/
	int i = 0;
//	uint32 * dma;
//	uint32 pci;

	while (count--)
	{
		ACCW(FXBNDRY, (((list[i].right) + 1) << 16) | list[i].left);
		ACCW_YDSTLEN(list[i].top, ((list[i].bottom - list[i].top) + 1));
		ACCW(FCOL, 0); /* color */

		/* start the invert (top nibble is c is clipping enabled) */
		ACCGO(DWGCTL, 0x40057814); // atype RSTR

		/* pseudo_dma version! */
//		MGAACC_DWGCTL      =0x1c00,
//		MGAACC_FCOL        =0x1c24,
//		MGAACC_FXBNDRY     =0x1c84,
//		MGAACC_YDSTLEN     =0x1c88,
//
//		40,09,21,22 (ordered as registers)

//		dma = (uint32 *)si->pseudo_dma;
//		*dma++= 0x40092221;
//		*dma++= (((list[i].right) + 1) << 16) | list[i].left;
//		*dma++= (list[i].top << 16) | ((list[i].bottom - list[i].top) + 1);
//		*dma++= 0; /* color */
//		*dma++= 0x40057814;

		/* real dma version! */
//		dma = (vuint32 *)si->dma_buffer;
//		*dma++= 0x40092221; /* indices */
//		*dma++= (((list[i].right) + 1) << 16) | list[i].left;
//		*dma++= (list[i].top << 16) | ((list[i].bottom - list[i].top) + 1);
//		*dma++= 0; /* color */
//		*dma++= 0x40057814;

//		pci = si->dma_buffer_pci;
//		ACCW(PRIMADDRESS, (pci));
//		ACCW(PRIMEND, (20 + pci));

//		delay(100);

		i++;
	}
}
