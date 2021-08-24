RGBColor class
##############

RGBColor objects provide a simplified interface to colors for the
app_server, especially the DisplayDriver class

Member Functions
================

RGBColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
------------------------------------------------

RGBColor(rgb_color col)
-----------------------

RGBColor(uint16 color16)
------------------------

RGBColor(uint8 color8)
----------------------

RGBColor(const RGBColor &color)
-------------------------------

RGBColor(void)
--------------

In all cases, extract the color data by calling SetColor. Void version sets to (0,0,0,0)

void PrintToStream(void)
------------------------

Prints the color values of the color32 member via printf()

uint8 GetColor8(void)
---------------------

uint16 GetColor16(void)
-----------------------

rgb_color GetColor32(void)
--------------------------


These are for obtaining space-specific versions of the color assigned
to the object.


1) In all cases, return the internal color storage members


void SetColor(const RGBColor &color)
------------------------------------

Copy all internal members to the current object.

void SetColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
-----------------------------------------------------

void SetColor(rgb_color col)
----------------------------

1. Assign parameters to internal rgb_color
2. call SetRGBAColor15()
3. call SystemPalette::FindClosestColor()

void SetColor(uint16 color16)
-----------------------------

1. Assign parameter to internal uint16
2. call SetRGBAColor32()
3. call SystemPalette::FindClosestColor()

void SetColor(uint8 color8)
---------------------------

1. Assign parameter to internal uint8
2. Get the 32-bit value from the palette and assign it to the internal rgb_color
3. call SetRGBAColor16()

RGBColor & operator=(const RGBColor &from)
------------------------------------------

Copy all data members over and return the value of this (return \*this;)

bool operator==(const RGBColor &from)
-------------------------------------

Compare rgb_colors and if all members are equal, return true. Otherwise,
return false.

bool operator!=(const RGBColor &from)
-------------------------------------


Compare rgb_colors and if all are equal, return false. Otherwise,
return true.


RGBColor MakeBlendColor(RGBColor c, float position)
---------------------------------------------------

Returns a color which is (position \* 100) of the way from the current
color to the one passed to it. This would be an easy way to generate
color gradients, for example, but with more control.


1) Clip position to the range 0<=position<=1
2) For each color component,

   a) calculate the delta (delta=int16(c.component-thiscolor.component))
   b) calculate the modifier (mod=thiscolor.component+(delta \* position))
   c) clip modifier to the range 0 <= modifier <= 255
   d) assign modifier to the component (thiscolor.component=int8(mod))

3) return a new RGBColor constructed around the new color

