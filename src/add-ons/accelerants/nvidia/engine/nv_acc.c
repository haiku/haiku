/* NV Acceleration functions */
/* Authors:
   Mark Watson 2/2000,
   Rudolf Cornelissen 10/2002-4/2003.
*/

#define MODULE_BIT 0x00080000

#include "nv_std.h"

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

/* G100 pre SRCORG/DSTORG registers */
static uint32 src_dst;
/* MIL1/2 adress linearisation does not always work */
static uint8 y_lin;
static uint8 depth;

/* needed by MIL 1/2 because of adress linearisation constraints */
#define ACCW_YDSTLEN(dst, len) do { \
	if (y_lin) { \
		ACCW(YDST,((dst)* (si->fbc.bytes_per_row / (depth >> 3))) >> 5); \
		ACCW(LEN,len); \
	} else ACCW(YDSTLEN,((dst)<<16)|(len)); \
} while (0)

status_t nv_acc_wait_idle()
{
//	volatile int i;
//	while (ACCR(STATUS)&(1<<16))
//	{
//		for (i=0;i<10000;i++); /*spin in place so I do not hammer the bus*/
//	};
	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nv_acc_init()
{
	/* used for convenience: MACCESS is a write only register! */
	uint32 maccess = 0x00000000;
	/* if we were unable to read PINS, we have to assume something (keeping bit6 zero) */
//	if ((si->ps.card_type >= G450) && (si->ps.pins_status = B_OK))
//	{
		/* b7 v5_mem_type = done by Mark Watson. fixme: still confirm! (unknown bits) */
//		maccess |= ((((uint32)si->ps.v5_mem_type) & 0x80) >> 1);
//	}

	/* preset using hardware adress linearisation */
	y_lin = 0x00;
	/* reset depth */
	depth = 0;

	/* cleanup bitblt */
	ACCW(OPMODE,0);

	/* Set the Z origin to the start of FB (otherwise lockup on blits) */
	ACCW(ZORG,0);

	/* Set pixel width */
	switch(si->dm.space)
	{
	case B_CMAP8:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x00));
		depth = 8;
		break;
	case B_RGB15_LITTLE:case B_RGB16_LITTLE:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x01)); 
		depth = 16;
		break;
	case B_RGB32_LITTLE:case B_RGBA32_LITTLE:
		ACCW(MACCESS, ((maccess & 0xfffffffc) | 0x02));
		depth = 32;
		break;
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* setup PITCH: very cardtype specific! */
/*	switch (si->ps.card_type)
	{
	case MIL1:
		switch (si->fbc.bytes_per_row / (depth >> 3))
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
*/				/* we are using hardware adress linearisation */
/*				break;
			default:
*/				/* we are using software adress linearisation */
/*				y_lin = 0x01;
				LOG(8,("ACC: using software adress linearisation\n"));
				break;
		}
		ACCW(PITCH, (y_lin << 15) | ((si->fbc.bytes_per_row / (depth >> 3)) & 0x0FFF));
		break;
	case MIL2:
		switch (si->fbc.bytes_per_row / (depth >> 3))
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
*/				/* we are using hardware adress linearisation */
/*				break;
			default:
*/				/* we are using software adress linearisation */
/*				y_lin = 0x01;
				LOG(8,("ACC: using software adress linearisation\n"));
				break;
		}
		ACCW(PITCH, (y_lin << 15) | ((si->fbc.bytes_per_row / (depth >> 3)) & 0x0FFF));
		break;
	case G100:
*/		/* always using hardware adress linearisation, because 2D/3D
		 * engine works on every pitch multiple of 32 */
/*		ACCW(PITCH, ((si->fbc.bytes_per_row / (depth >> 3)) & 0x0FFF));
		break;
	default:
*/		/* G200 and up are equal.. */
		/* always using hardware adress linearisation, because 2D/3D
		 * engine works on every pitch multiple of 32 */
/*		ACCW(PITCH, ((si->fbc.bytes_per_row / (depth >> 3)) & 0x1FFF));
		break;
	}
*/
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);

//	if (si->ps.card_type >= G200) {
		/*DSTORG - location of active screen in framebuffer*/
//		ACCW(DSTORG,(si->fbc.frame_buffer)-(si->framebuffer));

		/*SRCORG - init source address - same as dest*/
//		ACCW(SRCORG,(si->fbc.frame_buffer)-(si->framebuffer));
//	}

	/* init YDSTORG - apsed, if not inited, BitBlts may fails on <= G200 */
	src_dst = 0;
	ACCW(YDSTORG, src_dst);

	/* <= G100 uses this register as SRCORG/DSTORG replacement, but
	 * MIL 1/2 does not need framebuffer space for the hardcursor! */
/*	if ((si->ps.card_type == G100) && (si->settings.hardcursor))
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
*/	ACCW(YDSTORG,src_dst);

	/* clipping */
	/* i.e. highest and lowest X pixel adresses */
	ACCW(CXBNDRY,(((si->fbc.bytes_per_row / (depth >> 3)) - 1) << 16) | (0));

	/* Y pixel addresses must be linear */
	/* lowest adress */
	ACCW(YTOP, 0 + src_dst);
	/* highest adress */
	ACCW(YBOT,((si->dm.virtual_height - 1) *
		(si->fbc.bytes_per_row / (depth >> 3))) + src_dst);

	return B_OK;
}

/* screen to screen blit - i.e. move windows around.
 * Engine function bitblit, paragraph 4.5.7.2 */
status_t nv_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;

	/*find where the top,bottom and offset are*/
	offset = (si->fbc.bytes_per_row / (depth >> 3));

	t_end = t_start = xs + (offset*ys) + src_dst;
	t_end += w;

	b_end = b_start = xs + (offset*(ys+h)) + src_dst;
	b_end +=w;

	/* sgnzero bit _must_ be '0' before accessing SGN! */
	ACCW(DWGCTL,0x00000000);

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
	ACCGO(DWGCTL,0x040C4018); // atype RSTR

	return B_OK;
}

/* screen to screen tranparent blit - not sure what uses this.
 * Engine function bitblit, paragraph 4.5.7.2 */
status_t nv_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	uint32 t_start,t_end,offset;
	uint32 b_start,b_end;

	return B_ERROR;

	/*find where the top,bottom and offset are*/
	offset = (si->fbc.bytes_per_row / (depth >> 3));

	t_end = t_start = xs + (offset*ys) + src_dst;
	t_end += w;

	b_end = b_start = xs + (offset*(ys+h)) + src_dst;
	b_end +=w;

	/* sgnzero bit _must_ be '0' before accessing SGN! */
	ACCW(DWGCTL,0x00000000);

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
	ACCGO(DWGCTL,0x440C4018); // atype RSTR
	return B_OK;
}

/* rectangle fill.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
/*colorIndex,fill_rect_params,count*/
status_t nv_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
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

//acc fixme: checkout blockmode constraints for G100+ (mil: nc?): also add blockmode
//	         for other functions, and use fastblt on MIL1/2 if possible...
//or is CMAP8 contraint a non-blockmode contraint? (linearisation problem maybe?)
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

/* rectangle invert.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
/*colorIndex,fill_rect_params,count*/
status_t nv_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
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
	
	/*draw it! top nibble is c is clipping enabled*/
	ACCGO(DWGCTL,0x40057814); // atype RSTR

	/*pseudo_dma version!*/
//NVACC_DWGCTL      =0x1C00,
//NVACC_FCOL        =0x1C24,
//NVACC_FXBNDRY     =0x1C84,
//NVACC_YDSTLEN     =0x1C88,
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

/* screen to screen scaled filtered blit - i.e. scale video in memory.
 * Engine function texture mapping for video, paragraphs 4.5.5.5 - 4.5.5.9 */
status_t nv_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd)
{
	//fixme: implement. Used for G450/G550 Desktop TVout...
	//fixme: see if MIL1 - G200 support this function as well...

	return B_OK;
}
