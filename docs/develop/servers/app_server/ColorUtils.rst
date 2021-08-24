ColorUtils
##########

These functions are used for general purpose color-related tasks.

Global Functions
================

void SetRGBColor32(rgb_color \*col, uint8 r, uint8 g, uint8 b, uint8 a=255)
---------------------------------------------------------------------------

Simply assigns the passed parameters to the internal members of the
passed color

void SetRGBAColor32(rgb_color \*col, uint16 color16)
----------------------------------------------------

Maps a 16-bit color to a 32-bit one.

gggbbbbb arrrrrgg

1. Extract component values using the following calculations:

   .. code-block:: cpp

      red16 = (uint8[1] & 124) >> 2;
      green16 = ((uint8[0] & 224) >> 5) | ((uint8[1] & 3) << 3);
      blue16 = uint8[0] & 31;

2. Use cross-multiplication to map each 16-bit color component from 0-31
   space to 0-255 space, i.e. red32 = (red16 / 31) \* 255
3. Assign mapped values to the rgb_color passed to the function

void SetRGBColor16(uint16 \*col, uint8 r, uint8 g, uint8 b)
-----------------------------------------------------------

Used for easy assignment of opaque (B_RGB16) 16-bit colors.

1. Clip parameters via a bitwise AND with 31 (var &=31)
2. Create a uint8 * to the passed color
3. Assign as follows and return:

   .. code-block:: cpp

      uint8[0] = ( (g & 7) << 5) | (b & 31);
      uint8[1] = ( (r & 31) << 3) | ( (g & 56) >> 3);

void SetRGBAColor15(uint16 \*col, uint8 r, uint8 g, uint8 b, bool opaque=true)
------------------------------------------------------------------------------

Used for easy assignment of alpha-aware (B_RGBA16) 16-bit colors.

1. Clip parameters via a bitwise AND with 31 (var &=31)
2. Create a uint8 * to the passed color
3. Assign as follows and return:

   .. code-block:: cpp

      uint8[0] = ( (g & 7) << 5) | (b & 31);
      uint8[1] = ( (r & 31) << 2) | ( (g & 24) >> 3) | (a) ? 128 : 0;

uint8 FindClosestColor(rgb_color \*palette,rgb_color col)
---------------------------------------------------------

Finds the color which most closely resembles the given one in the
given palette.

1. Set the saved delta value to 765 (maximum difference)
2. Loop through all the colors in the palette. For each color,

   a. calculate the delta value for each color component and add them
      together
   b. compare the new combined delta with the saved one
   c. if the delta is 0, immediately return the current index
   d. if the new one is smaller, save it and also the palette index

uint16 FindClosestColor16(rgb_color col)
----------------------------------------

Returns a 16-bit approximation of the given 32-bit color. Alpha values
are ignored.

1. Create an opaque, 16-bit approximation of col using the following
   calculations:

   .. code-block:: cpp

      r16 = (31 * col.red) / 255;
      g16 = (31 * col.green) / 255;
      b16 = (31 * col.blue) / 255;

2. Assign it to a uint16 using the same code as in SetRGBColor16() and
   return it.

uint16 FindClosestColor15(rgb_color col)
----------------------------------------

This functions almost exactly like the 16-bit version, but this also
takes into account the alpha transparency bit and works in the color
space B_RGBA15. Follow the same algorithm as FindClosestColor16(), but
assign the return value using SetRGBColor15.

rgb_color MakeBlendColor(rgb_color col, rgb_color col2, float position)
-----------------------------------------------------------------------

MakeBlendColor calculates a color that is somewhere between start
color col and end color col2, based on position, where 0<= position <=
1. If position is out of these bounds, a color of {0,0,0,0} is
returned. If position is 0, the start color is returned. If position
is 1, col2 is returned. Otherwise, the color is calculated thus:

1. calculate delta values for each channel, i.e. int16 delta_r=col.red-col2.red
2. Based on these delta values, calculate the blend values for each
   channel, i.e. blend_color.red=uint8(col1.red - (delta_r \* position) )
