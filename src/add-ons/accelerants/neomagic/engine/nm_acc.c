/* nm Acceleration functions */

/* Author:
   Rudolf Cornelissen 3/2004.
*/

#define MODULE_BIT 0x00080000

#include "nm_std.h"

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

//fixme: acc setup for NM2097 and NM2160 only for now...
status_t nm_acc_wait_idle()
{
	/* wait until engine completely idle */
	switch (si->ps.card_type)
	{
	case NM2097:
	case NM2160:
		while (ACCR(STATUS) & 0x00000001)
		{
			/* snooze a bit so I do not hammer the bus */
			snooze (100); 
		}
		break;
	}

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nm_acc_init()
{
	uint32 depth;

	/* Set pixel width */
	switch(si->dm.space)
	{
	case B_CMAP8:
		/* b8-9 determine engine colordepth */
		si->engine.control = (1 << 8);
//	nAcl->ColorShiftAmt = 8;
		depth = 1;
		break;
	case B_RGB15_LITTLE:case B_RGB16_LITTLE:
		/* b8-9 determine engine colordepth */
		si->engine.control = (2 << 8);
//	nAcl->ColorShiftAmt = 0;
		depth = 2;
		break;
	case B_RGB24_LITTLE:
		/* no acceleration supported on NM2097 and NM2160 */
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* setup memory pitch (b10-12):
	 * this works with a table, there are very few fixed settings.. */
	switch(si->fbc.bytes_per_row / depth)
	{
	case 640:
		si->engine.control |= (2 << 10);
		break;
	case 800:
		si->engine.control |= (3 << 10);
		break;
	case 1024:
		si->engine.control |= (4 << 10);
		break;
	case 1152:
		si->engine.control |= (5 << 10);
		break;
	case 1280:
		si->engine.control |= (6 << 10);
		break;
	case 1600:
		si->engine.control |= (7 << 10);
		break;
	default:
		LOG(8,("ACC: init, invalid mode width\n"));
		return B_ERROR;
	}

	/* fixme?: setup clipping */

	return B_OK;
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nm_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* make sure the previous command (if any) is completed */
	nm_acc_wait_idle();

    if ((yd < ys) || ((yd == ys) && (xd < xs)))
    {
		/* start with upper left corner */
		ACCW(BLTCNTL, si->engine.control);
		ACCW(SRCSTARTOFF, ((ys << 16) | (xs & 0x0000ffff)));
		ACCW(DSTSTARTOFF, ((yd << 16) | (xd & 0x0000ffff)));
		ACCW(XYEXT, ((h << 16) | (w & 0x0000ffff)));
	}
    else
    {
		/* start with lower right corner */
		ACCW(BLTCNTL, (si->engine.control | 0x00000013));
		ACCW(SRCSTARTOFF, (((ys + (h - 1)) << 16) | ((xs + (w - 1)) & 0x0000ffff)));
		ACCW(DSTSTARTOFF, (((yd + (h - 1)) << 16) | ((xd + (w - 1)) & 0x0000ffff)));
		ACCW(XYEXT, ((h << 16) | (w & 0x0000ffff)));
	}

	return B_OK;
}

/* screen to screen tranparent blit - not sure what uses this.
 * Engine function bitblit, paragraph 4.5.7.2 */
status_t nm_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	return B_ERROR;

	/*find where the top,bottom and offset are*/
//	offset = (si->fbc.bytes_per_row / (depth >> 3));

//	t_end = t_start = xs + (offset*ys) + src_dst;
//	t_end += w;

//	b_end = b_start = xs + (offset*(ys+h)) + src_dst;
//	b_end +=w;

	/* sgnzero bit _must_ be '0' before accessing SGN! */
//	ACCW(DWGCTL,0x00000000);

	/*find which quadrant */
	switch((yd>ys)|((xd>xs)<<1))
	{
	case 0: /*L->R,down*/ 
//		ACCW(SGN,0);

//		ACCW(AR3,t_start);
//		ACCW(AR0,t_end);
//		ACCW(AR5,offset);

//		ACCW_YDSTLEN(yd,h+1);
		break;
	case 1: /*L->R,up*/
//		ACCW(SGN,4);

//		ACCW(AR3,b_start);
//		ACCW(AR0,b_end);
//		ACCW(AR5,-offset);

//		ACCW_YDSTLEN(yd+h,h+1);
		break;
	case 2: /*R->L,down*/
//		ACCW(SGN,1);

//		ACCW(AR3,t_end);
//		ACCW(AR0,t_start);
//		ACCW(AR5,offset);

//		ACCW_YDSTLEN(yd,h+1);
		break;
	case 3: /*R->L,up*/
//		ACCW(SGN,5);

//		ACCW(AR3,b_end);
//		ACCW(AR0,b_start);
//		ACCW(AR5,-offset);

//		ACCW_YDSTLEN(yd+h,h+1);
		break;
	}
//	ACCW(FXBNDRY,((xd+w)<<16)|xd);
	
	/*do the blit*/
//	ACCW(FCOL,colour);
//	ACCW(BCOL,0xffffffff);
//	ACCGO(DWGCTL,0x440C4018); // atype RSTR
	return B_OK;
}

/* rectangle fill.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
/*colorIndex,fill_rect_params,count*/
status_t nm_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/

//	ACCW(FXBNDRY,(xe<<16)|xs); /*set x start and end*/
//	ACCW_YDSTLEN(ys,yl); /*set y start and length*/
//	ACCW(FCOL,col);            /*set colour*/

//acc fixme: checkout blockmode constraints for G100+ (mil: nc?): also add blockmode
//	         for other functions, and use fastblt on MIL1/2 if possible...
//or is CMAP8 contraint a non-blockmode contraint? (linearisation problem maybe?)
	if (si->dm.space==B_CMAP8)
	{
//		ACCGO(DWGCTL,0x400C7814); // atype RSTR
	}
	else
	{
//		ACCGO(DWGCTL,0x400C7844); // atype BLK 
	}
	return B_OK;
}

/* rectangle invert.
 * Engine function rectangle_fill: paragraph 4.5.5.2 */
/*colorIndex,fill_rect_params,count*/
status_t nm_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col)
{
/*
	FXBNDRY - left and right coordinates    a
	YDSTLEN - y start and no of lines       a
	(or YDST and LEN)                       
	DWGCTL - atype must be RSTR or BLK      a
	FCOL - foreground colour                a
*/

//	ACCW(FXBNDRY,(xe<<16)|xs); /*set x start and end*/
//	ACCW_YDSTLEN(ys,yl); /*set y start and length*/
//	ACCW(FCOL,col);            /*set colour*/
	
	/*draw it! top nibble is c is clipping enabled*/
//	ACCGO(DWGCTL,0x40057814); // atype RSTR

	return B_OK;
}

/* screen to screen scaled filtered blit - i.e. scale video in memory.
 * Engine function texture mapping for video, paragraphs 4.5.5.5 - 4.5.5.9 */
status_t nm_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd)
{
	//fixme: implement.

	return B_OK;
}
