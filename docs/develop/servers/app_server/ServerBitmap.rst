ServerBitmap class
##################

ServerBitmaps are the server side counterpart to BBitmap. Note that they
are not allocated like other objects - the BitmapManager handles all
allocation and deletion tasks.

Member Functions
================

ServerBitmap(BRect r, color_space cspace, int32 flags, int32 bytesperrow=-1, screen_id screen=B_MAIN_SCREEN_ID)
---------------------------------------------------------------------------------------------------------------

1. Call \_HandleSpace()
2. Call \_HandleFlags()
3. Initialize remaining data members to parameters or safe values

~ServerBitmap(void)
-------------------

Empty

uint8 \*Bits(void)
------------------

Returns the bitmap's buffer member

area_id Area(void)
------------------

Returns the bitmap's buffer member.

uint32 BitsLength(void)
-----------------------

Returns bytes_per_row \* height

BRect Bounds(void)
------------------

returns BRect(width-1,height-1)

int32 BytesPerRow(void)
-----------------------

returns the bitmap's bytes_per_row member

void _HandleSpace(color_space cs, int32 bytesperline=-1)
---------------------------------------------------------

Large function which essentially consists of a switch() of the available
color spaces and assigns the bits per pixel and bytes per line values
based on the color space. If bytesperline is -1, the default is used,
otherwise it uses the specified value.

