/* MGA Acceleration functions */
/* Authors:
   Mark Watson 2/2000,
   Rudolf Cornelissen 10/2002
*/

#define MODULE_BIT 0x00080000

#include "mga_std.h"

/*acceleration notes*/

/*functions Be needs:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

/* G100 pre SRCORG/DSTORG registers */
static uint32 src_dst;

// needed by MIL2 in 800x600 8bpp
#define ACCW_YDSTLEN(dst, len) do { \
/*	if (si->ylin) { */ \
	if ((si->ps.card_type==MIL2) && (si->dm.space==B_CMAP8)) { \
		ACCW(YDST,((dst)*si->dm.virtual_width) >> 5); \
		ACCW(LEN,len); \
	} else ACCW(YDSTLEN,((dst)<<16)|(len)); \
} while (0)

status_t gx00_acc_wait_idle()
{
	volatile int i;
	while (ACCR(STATUS)&(1<<16))
	{
		for (i=0;i<10000;i++); /*spin in place so I do not hammer the bus*/
	};
	return B_OK;
}

/*AFAIK this must be done for every new screenmode*/
status_t gx00_acc_init()
{
	ACCW(OPMODE,0); // cleanup bitblt

	/*Set the Z origin to the start of FB (otherwise lockup on blits)*/
	if (si->ps.card_type>=G100)
		ACCW(ZORG,0);

	/*MACCESS - for 2D, only pixel width is important > all others can be 0*/
	switch(si->dm.space)
	{
	case B_CMAP8:
		ACCW(MACCESS,0);
		break;
	case B_RGB15_LITTLE:case B_RGB16_LITTLE:
		ACCW(MACCESS,1); 
		break;
	case B_RGB32_LITTLE:case B_RGBA32_LITTLE:
		ACCW(MACCESS,2);
		break;
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/*PITCH*/
	// TODO apsed 3-129 32 or 64 following depth (or 128 if MIl2)
	if (si->dm.virtual_width&0x1F)
	{
		LOG(8,("ACC: can not accelerate, pitch is not multiple of 32 pixels\n"));
		return B_ERROR;
	}
	if (si->ps.card_type>=G200)
		ACCW(PITCH,(si->dm.virtual_width)&0x1FFF); /* use hardware Y decoding */
	else
		ACCW(PITCH,(si->dm.virtual_width)&0x0FFF); /* use hardware Y decoding */

if ((si->ps.card_type==MIL2) && (si->dm.space==B_CMAP8)) {
	// ylin shall be 1 if 800x600 
	ACCW(PITCH, (1<<15) | (si->dm.virtual_width&0x0FFF));
}
	/*PLNWT - plane write mask*/
//	if (si->ps.card_type>=G200) // apsed: PLNWT exists also in MIL2, G100
	ACCW(PLNWT,0xFFFFFFFF); /*all planes are written*/

	if (si->ps.card_type>=G200) {
		/*DSTORG - location of active screen in framebuffer*/
		ACCW(DSTORG,(si->fbc.frame_buffer)-(si->framebuffer));

		/*SRCORG - init source address - same as dest*/
		ACCW(SRCORG,(si->fbc.frame_buffer)-(si->framebuffer));
	}

	/*YDSTORG - apsed, if not inited, BitBlts may fails on g200, see YTOP/BOT after */
	/* Used for G100, included in acceleration routines also: */
	src_dst = 0;
	ACCW(YDSTORG, src_dst);

	/* G100 uses this register as SRCORG/DSTORG replacement */
	if ((si->ps.card_type == G100) && (si->settings.hardcursor))
	{
		switch (si->dm.space)
		{
			case B_CMAP8:
				src_dst = 1024 / 1;
				break;
			case B_RGB15_LITTLE:
			case B_RGB16_LITTLE:
				src_dst = 1024 / 2;
				break;
			case B_RGB32_LITTLE:
				src_dst =  1024 / 4;
				break;
			default:
				LOG(8,("ACC: G100 hardcursor not supported for current colorspace\n"));
				return B_ERROR;
		}		
	}
	ACCW(YDSTORG,src_dst);

	/*clipping*/
	ACCW(CXBNDRY,((si->dm.virtual_width -1)<<16)|(0)); /*i.e. highest and lowest right pixel value*/

	// apsed TODO g200 shall be YDSTORG + value
	// apsed TODO why -1, must be a multiple of 32 since MIl2
	ACCW(YTOP,0);
	ACCW(YBOT,(si->dm.virtual_height*si->dm.virtual_width)-1); /*y address must be linear*/

	return B_OK;
}

/*screen to screen blit - i.e. move windows around*/
status_t gx00_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;

	/*find where the top,bottom and offset are*/
	offset = si->dm.virtual_width;

	t_end = t_start = xs + (offset*ys) + src_dst;
	t_end += w;

	b_end = b_start = xs + (offset*(ys+h)) + src_dst;
	b_end +=w;

	/*find which quadrant */
	switch((yd>ys)|((xd>xs)<<1))
	{
	case 0: /*L->R,down*/ 
		ACCW(SGN,0);

		ACCW(AR3,t_start);
		ACCW(AR0,t_end);
		ACCW(AR5,offset);

		ACCW_YDSTLEN(yd,h+1);
		break;
	case 1: /*L->R,up*/
		ACCW(SGN,4);

		ACCW(AR3,b_start);
		ACCW(AR0,b_end);
		ACCW(AR5,-offset);

		ACCW_YDSTLEN(yd+h,h+1);
		break;
	case 2: /*R->L,down*/
		ACCW(SGN,1);

		ACCW(AR3,t_end);
		ACCW(AR0,t_start);
		ACCW(AR5,offset);

		ACCW_YDSTLEN(yd,h+1);
		break;
	case 3: /*R->L,up*/
		ACCW(SGN,5);

		ACCW(AR3,b_end);
		ACCW(AR0,b_start);
		ACCW(AR5,-offset);

		ACCW_YDSTLEN(yd+h,h+1);
		break;
	}
	ACCW(FXBNDRY,((xd+w)<<16)|xd);

	/*do the blit*/
	ACCGO(DWGCTL,0x040C4018);

	return B_OK;
}

/*screen to screen tranparent blit - not sure what uses this...*/
status_t gx00_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;

	return B_ERROR;

	/*find where the top,bottom and offset are*/
	offset = si->dm.virtual_width;

	t_end = t_start = xs + (offset*ys) + src_dst;
	t_end += w;

	b_end = b_start = xs + (offset*(ys+h)) + src_dst;
	b_end +=w;

	/*find which quadrant */
	switch((yd>ys)|((xd>xs)<<1))
	{
	case 0: /*L->R,down*/ 
		ACCW(SGN,0);

		ACCW(AR3,t_start);
		ACCW(AR0,t_end);
		ACCW(AR5,offset);

		ACCW_YDSTLEN(yd,h+1);
		break;
	case 1: /*L->R,up*/
		ACCW(SGN,4);

		ACCW(AR3,b_start);
		ACCW(AR0,b_end);
		ACCW(AR5,-offset);

		ACCW_YDSTLEN(yd+h,h+1);
		break;
	case 2: /*R->L,down*/
		ACCW(SGN,1);

		ACCW(AR3,t_end);
		ACCW(AR0,t_start);
		ACCW(AR5,offset);

		ACCW_YDSTLEN(yd,h+1);
		break;
	case 3: /*R->L,up*/
		ACCW(SGN,5);

		ACCW(AR3,b_end);
		ACCW(AR0,b_start);
		ACCW(AR5,-offset);

		ACCW_YDSTLEN(yd+h,h+1);
		break;
	}
	ACCW(FXBNDRY,((xd+w)<<16)|xd);
	
	/*do the blit*/
	ACCW(FCOL,colour);
	ACCW(BCOL,0xffffffff);
	ACCGO(DWGCTL,0x440C4018);
	return B_OK;
}

/*rectangle fill*/
/*colorIndex,fill_rect_params,count*/
status_t gx00_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/

	ACCW(FXBNDRY,(xe<<16)|xs); /*set x start and end*/
	ACCW_YDSTLEN(ys,yl); /*set y start and length*/
	ACCW(FCOL,col);            /*set colour*/

	if (si->dm.space==B_CMAP8 || si->ps.sdram)
	{
		ACCGO(DWGCTL,0x400C7814); // atype RSTR
	}
	else
	{
		ACCGO(DWGCTL,0x400C7844); // atype BLK 
	}
	return B_OK;
}

/*rectangle invert*/
/*colorIndex,fill_rect_params,count*/
status_t gx00_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
{
//	int i;
//	uint32 * dma;
//	uint32 pci;
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/

	ACCW(FXBNDRY,(xe<<16)|xs); /*set x start and end*/
	ACCW_YDSTLEN(ys,yl); /*set y start and length*/
	ACCW(FCOL,col);            /*set colour*/
	
	ACCGO(DWGCTL,0x40057814);   /*draw it! top nibble is c is clipping enabled*/

	/*pseudo_dma version!*/
//MGAACC_DWGCTL      =0x1C00,
//MGAACC_FCOL        =0x1C24,
//MGAACC_FXBNDRY     =0x1C84,
//MGAACC_YDSTLEN     =0x1C88,
//
//40,09,21,22 (ordered as registers)

//	dma = (uint32 *)si->pseudo_dma;
//	*dma++=0x40092221;
//	*dma++=(xe<<16)|xs;
//	*dma++=(ys<<16)|yl;
//	*dma++=col;
//	*dma++=0x40057814;

	/*real dma version!*/
//	dma = (vuint32 *)si->dma_buffer;
//	*dma++=0x40092221;/*indices*/
//	*dma++=(xe<<16)|xs;
//	*dma++=(ys<<16)|yl;
//	*dma++=col;
//	*dma++=0x40057814;

//	pci = si->dma_buffer_pci;
//	ACCW(PRIMADDRESS,(pci));
//	ACCW(PRIMEND,(20+pci));

//	delay(100);

	return B_OK;
}
