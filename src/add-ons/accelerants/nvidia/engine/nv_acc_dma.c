/* NV Acceleration functions */

/* Author:
   Rudolf Cornelissen 8/2003-1/2005.

   This code was possible thanks to:
    - the Linux XFree86 NV driver,
    - the Linux UtahGLX 3D driver.
*/

/*
	note:
	attempting DMA on NV40 and higher because without it I can't get it going ATM.
	Later on this can become a nv.settings switch, and maybe later we can even
	forget about non-DMA completely (depends on 3D acceleration attempts).
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

status_t nv_acc_wait_idle_dma()
{
	/* wait until engine completely idle */
	//fixme: implement.

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nv_acc_init_dma()
{
	//fixme: implement.

	return B_ERROR;
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nv_acc_setup_blit_dma()
{
	//fixme: implement.

	return B_ERROR;
}

status_t nv_acc_blit_dma(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	//fixme: implement.

	return B_ERROR;
}

/* rectangle fill - i.e. workspace and window background color */
/* span fill - i.e. (selected) menuitem background color (Dano) */
status_t nv_acc_setup_rectangle_dma(uint32 color)
{
	//fixme: implement.

	return B_ERROR;
}

status_t nv_acc_rectangle_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	//fixme: implement.

	return B_ERROR;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t nv_acc_setup_rect_invert_dma()
{
	//fixme: implement.

	return B_ERROR;
}

status_t nv_acc_rectangle_invert_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	//fixme: implement.

	return B_ERROR;
}
