/* nm Acceleration functions */

/* Author:
   Rudolf Cornelissen 3/2004.
*/

#define MODULE_BIT 0x00080000

#include "nm_std.h"

//static status_t nm_acc_wait_fifo(uint32 n);

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
	/* Note:
	 * because we have no FIFO functionality we have to make sure we return ASAP(!).
	 * snoozing() for just 1 microSecond already more than halfs the engine
	 * performance... */
	while (ACCR(STATUS) & 0x00000001);

	return B_OK;
}

/* wait for enough room in fifo */
//static status_t nm_acc_wait_fifo(uint32 n)
//{
//	while (((ACCR(STATUS) & 0x0000ff00) >> 8) < n)
//	{
		/* snooze a bit so I do not hammer the bus */
//		snooze (10);
//	}
//}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nm_acc_init()
{
	/* Set pixel width */
	switch(si->dm.space)
	{
	case B_CMAP8:
		/* b8-9 determine engine colordepth */
		si->engine.control = (1 << 8);
		si->engine.depth = 1;
		break;
	case B_RGB15_LITTLE:case B_RGB16_LITTLE:
		/* b8-9 determine engine colordepth */
		si->engine.control = (2 << 8);
		si->engine.depth = 2;
		break;
	case B_RGB24_LITTLE:
		/* no acceleration supported on NM2097 and NM2160 */
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* setup memory pitch (b10-12):
	 * this works with a table, there are very few fixed settings.. */
	switch(si->fbc.bytes_per_row / si->engine.depth)
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

	/* enable engine FIFO */
	/* fixme when/if possible:
	 * does not work on most cards.. (tried NM2160)
	 * workaround: always wait until engine completely idle before programming. */
//	si->engine.control |= (1 << 27);

	/* setup buffer startadress */
	/* fixme when/if possible:
	 * not possible on all cards or not enough specs known. (tried NM2160)
	 * workaround: place cursor bitmap at _end_ of cardRAM instead of in _beginning_. */

	/* setup clipping */
	/* note:
	 * on NM2160 the max acc engine width of 1600 pixels can be programmed, but
	 * the max. height is only 1023 pixels (height register holds just 10 bits)! 
	 * note also:
	 * while the vertical clipping feature can do upto and including 1023 pixels,
	 * the engine itself can do upto and including 1024 pixels vertically.
	 * So:
	 * Don't use the engine's clipping feature as we want to get the max out of the
	 * engine. We won't export the acc hooks for modes beyond the acc engine's
	 * capabilities. */
//	si->engine.control |= (1 << 26);
//	ACCW(CLIPLT, 0);
//	ACCW(CLIPRB, ((si->dm.virtual_height << 16) | (si->dm.virtual_width & 0x0000ffff)));

	return B_OK;
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nm_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* make sure the previous command (if any) is completed */
//	does not work yet:
//	nm_acc_wait_fifo(4);
//	so:
	nm_acc_wait_idle();

    if ((yd < ys) || ((yd == ys) && (xd < xs)))
    {
		/* start with upper left corner */
		/* use ROP GXcopy (b16-19), and use XY coord. system (b24-25) */
		ACCW(CONTROL, si->engine.control | 0x830c0000);
		/* send command and exexute */
		ACCW(SRCSTARTOFF, ((ys << 16) | (xs & 0x0000ffff)));
		ACCW(DSTSTARTOFF, ((yd << 16) | (xd & 0x0000ffff)));
		ACCW(XYEXT, (((h + 1) << 16) | ((w + 1) & 0x0000ffff)));
	}
    else
    {
		/* start with lower right corner */
		/* use ROP GXcopy (b16-19), and use XY coord. system (b24-25) */
		ACCW(CONTROL, (si->engine.control | 0x830c0013));
		/* send command and exexute */
		ACCW(SRCSTARTOFF, (((ys + h) << 16) | ((xs + w) & 0x0000ffff)));
		ACCW(DSTSTARTOFF, (((yd + h) << 16) | ((xd + w) & 0x0000ffff)));
		ACCW(XYEXT, (((h + 1) << 16) | ((w + 1) & 0x0000ffff)));
	}

	return B_OK;
}

/* rectangle fill - i.e. workspace and window background color */
/* span fill - i.e. (selected) menuitem background color (Dano) */
status_t nm_acc_setup_rectangle(uint32 color)
{
//	does not work yet:
//	nm_acc_wait_fifo(2);
//	so:
	nm_acc_wait_idle();

	/* use ROP GXcopy (b16-19), use XY coord. system (b24-25), do foreground color (b3) */
	ACCW(CONTROL, (si->engine.control | 0x830c0008));
	/* setup color */
	ACCW(FGCOLOR, color);

	return B_OK;
}

status_t nm_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* The engine does not take kindly if we try to let it fill a rect with
	 * zero width. Dano's app_server occasionally tries to let us do that though!
	 * Testable with BeRoMeter 1.2.6, all Ellipses tests. Effect of zero width fill:
	 * horizontal lines across the entire screen at top and bottom of ellipses. */
	if (xe == xs) return B_OK;

	/* make sure the previous command (if any) is completed */
//	does not work yet:
//	nm_acc_wait_fifo(2);
//	so:
	nm_acc_wait_idle();

	/* send command and exexute */
	ACCW(DSTSTARTOFF, ((ys << 16) | (xs & 0x0000ffff)));
	ACCW(XYEXT, ((yl << 16) | ((xe - xs) & 0x0000ffff)));

	return B_OK;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t nm_acc_setup_rect_invert()
{
	/* make sure the previous command (if any) is completed */
//	does not work yet:
//	nm_acc_wait_fifo(4);
//	so:
	nm_acc_wait_idle();

	/* use ROP GXinvert (b16-19), use XY coord. system (b24-25), do foreground color (b3) */
	ACCW(CONTROL, (si->engine.control | 0x83050008));
	/* reset color */
	ACCW(FGCOLOR, 0);

	return B_OK;
}

status_t nm_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* The engine probably also does not take kindly if we try to let it invert a
	 * rect with zero width... (see nm_acc_rectangle() routine for explanation.) */
	if (xe == xs) return B_OK;

	/* make sure the previous command (if any) is completed */
//	does not work yet:
//	nm_acc_wait_fifo(4);
//	so:
	nm_acc_wait_idle();

	/* send command and exexute */
	ACCW(DSTSTARTOFF, ((ys << 16) | (xs & 0x0000ffff)));
	ACCW(XYEXT, ((yl << 16) | ((xe - xs) & 0x0000ffff)));

	return B_OK;
}

/* screen to screen tranparent blit */
status_t nm_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	//fixme: implement.

	return B_ERROR;
}

/* screen to screen scaled filtered blit - i.e. scale video in memory */
status_t nm_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd)
{
	//fixme: implement.

	return B_OK;
}
