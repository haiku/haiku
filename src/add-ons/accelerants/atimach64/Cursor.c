#include "GlobalData.h"
#include "generic.h"

void set_cursor_colors(void);
uint8 bit_mirror(uint8 bits);
uint16 bit_merger(uint8 andbits, uint8 xorbits);

void set_cursor_colors(void) {
	/* a place-holder for a routine to set the cursor colors */
	/* In our sample driver, it's only called by the INIT_ACCELERANT() */

  /* Cursor color 0 is black */

  outw(CUR_CLR0, 0x00000000);

  /* Cursor color 1 is white */

  outw(CUR_CLR1, 0xffffffff);
	
}

uint8
bit_mirror(uint8 bits)
{
  int i;
  uint8 res = 0;

  for(i=0; i<8; i++)  {
    res |= (1 & (bits>>i)) <<(7-i);

  }

  return res;
}

uint16 
bit_merger(uint8 andbits, uint8 xorbits) 
{
  int i;
  uint16 res = 0;
  uint8 bits = 0;

  for(i=0; i<8; i++)  {
    bits = (((xorbits & (1<<i))>>i)<<1) | ((andbits & (1<<i))>>i);
    switch (bits){
      case 0: /* Transparent is 10 */
	res |= (2 << (i*2));
	break;
      case 2: /* Inversion is 11 */
	res |= (3 << (i*2));
	break;
      case 1: /* White is color 1 */  
	res |= (1 << (i*2));
	break;
      case 3: /* Black is color 0 */
	break;
    }
  }
  
  return res;

}

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) {

  int i, j, k;
  uint8 andM, xorM;
  uint16 *cdata = (uint16 *)si->cursor.data;
  
  /* NOTE: Currently, for BeOS, cursor width and height must be equal to 16. */

  if ((width != 16) || (height != 16))
    {
      return B_ERROR;
    }
  else if ((hot_x >= width) || (hot_y >= height))
    {
      return B_ERROR;
    }
  else
    {

      /* Turn off cursor */
      SHOW_CURSOR(false);
      
      
      /* Fill up data */
      k = 0;
      for(i = 0; i < height; i++) {
	for(j = 0; j < (width / 8); j++) {
	  andM = bit_mirror(~*(andMask++));
	  xorM = bit_mirror(*(xorMask++));
	  
	  /* convert 8 bits to 16 bits of mask */
	  cdata[j + (i*8)] = 
	    bit_merger(andM, xorM);
	  
	}

      }
      /* Update cursor variables appropriately. */
      si->cursor.width = width;
      si->cursor.height = height;
      si->cursor.hot_x = hot_x;
      si->cursor.hot_y = hot_y;
      
      /* Turn on cursor */
      SHOW_CURSOR(true);
    }

  return B_OK;
}

/*
Move the cursor to the specified position on the desktop.  If we're
using some kind of virtual desktop, adjust the display start position
accordingly and position the cursor in the proper "virtual" location.
*/
void 
MOVE_CURSOR(uint16 x, uint16 y) {

	bool move_screen = false;
	uint16 hds = si->dm.h_display_start;	/* the current horizontal starting pixel */
	uint16 vds = si->dm.v_display_start;	/* the current vertical starting line */
	int xoff, yoff; /* offsets of cursor if it goes out of the screen */
	/*
	Most cards can't set the starting horizontal pixel to anything but a multiple
	of eight.  If your card can, then you can change the adjustment factor here.
	Oftentimes the restriction is mode dependant, so you could be fancier and
	perhaps get smoother horizontal scrolling with more work.  It's a sample driver,
	so we're going to be lazy here.
	*/
	uint16 h_adjust = 7;	/* a mask to make horizontal values a multiple of 8 */

	/* clamp cursor to virtual display */
	if (x >= si->dm.virtual_width) x = si->dm.virtual_width - 1;
	if (y >= si->dm.virtual_height) y = si->dm.virtual_height - 1;

	/* adjust h/v_display_start to move cursor onto screen */
	if (x >= (si->dm.timing.h_display + hds)) {
		hds = ((x - si->dm.timing.h_display) + 1 + h_adjust) & ~h_adjust;
		move_screen = true;
	} else if (x < hds) {
		hds = x & ~h_adjust;
		move_screen = true;
	}
	if (y >= (si->dm.timing.v_display + vds)) {
		vds = y - si->dm.timing.v_display + 1;
		move_screen = true;
	} else if (y < vds) {
		vds = y;
		move_screen = true;
	}
	/* reposition the desktop on the display if required */
	if (move_screen) MOVE_DISPLAY(hds,vds);

	/* put cursor in correct physical position */
	x -= hds;
	y -= vds;

	/* Take care of hot_x hot_y */
	xoff = x - si->cursor.hot_x;
	yoff = y - si->cursor.hot_y;

	/* Do we  have to fake the cursor going out of the screen ? */
	if (xoff < 0) {
	  xoff = -xoff;
	} else {
	  xoff = 0;
	  x -= si->cursor.hot_x;
	}

	if (yoff < 0) {
	  yoff = -yoff;
	} else {
	  yoff = 0;
	  y -= si->cursor.hot_y;
	}

	/* Position the offset of the cursor */
	outw(CUR_HORZ_VERT_OFF, (yoff << 16) | xoff);


	/* position the cursor on the display */
	/* this will be card dependant */
	
	/*	printf(" Place %08x %d %d \n", si->regs + CUR_HORZ_VERT_POSN, x, y);*/

	outw(CUR_HORZ_VERT_POSN, (y << 16) | x);

}

void SHOW_CURSOR(bool is_visible) {

  if (is_visible)	{
    /* add cursor showing code here */
    WaitQueue(4);
    while (!(inw(CRTC_VLINE_CRNT_VLINE) & CRTC_CRNT_VLINE));
    outb(GEN_TEST_CNTL, inb(GEN_TEST_CNTL) | HWCURSOR_ENABLE);
  } else {
    /* add cursor hiding code here */
    WaitQueue(4);
    outb(GEN_TEST_CNTL, inb(GEN_TEST_CNTL) & (~HWCURSOR_ENABLE));
  }
  
  WaitIdleEmpty();
  
  /* record for our info */
  si->cursor.is_visible = is_visible;
}
