#include <SupportDefs.h>

/*
The first four bytes of cursor data give information about the cursor: 

Byte 1: Size in pixels-per-side. Cursors are always square; currently, only 16-by-16 
cursors are allowed. 
Byte 2: Color depth in bits-per-pixel. Currently, only one-bit monochrome is allowed. 
Bytes 3&4: Hot spot coordinates. Given in "cursor coordinates" where (0,0) is the upper 
left corner of the cursor grid, byte 3 is the hot spot's y coordinate, and byte 4 is its x 
coordinate. The hot spot is a single pixel that's "activated" when the user clicks the mouse. 
To push a button, for example, the hot spot must be within the button's bounds. 

Next comes an array of pixel color data. Pixels are specified from left to right in rows starting at 
the top of the image and working downward. The size of an array element is the depth of the 
image. In one-bit depth... 

the 16x16 array fits in 32 bytes. 
1 is black and 0 is white. 

Then comes the pixel transparency bitmask, given left-to-right and top-to-bottom. 1 is opaque, 0 is 
transparent. Transparency only applies to white pixels—black pixels are always opaque. 
*/

// Impressive ASCII art, eh?

int8 default_cursor_data[] = {
16,1,0,0,
255,224,	// ***********-----
128,16,		// *----------*----
128,16,		// *----------*----
128,96,		// *--------**-----
128,16,		// *----------*----
128,8,		// *-----------*---
128,8,		// *-----------*---
128,16,		// *----------*----
128,32,		// *---------*-----
144,64,		// *--*-----*------
144,128,	// *--*----*-------
105,0,		// -**-*--*--------

0,6,		// -----**---------
0,0,		// ----------------
0,0,		// ----------------
0,0,		// ----------------

// default_cursor mask - black pixels are always opaque
0,0,
255,224,
255,224,
255,128,
255,224,
255,240,
255,240,
255,224,
255,128,
255,0,
255,0,
255,0,

0,0,
0,0,
0,0,
0,0
};

// known good cursor data for testing rest of system
int8 cross_cursor[] = {16,1,5,5,
14,0,4,0,4,0,4,0,128,32,241,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0,
14,0,4,0,4,0,4,0,128,32,245,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0
};
