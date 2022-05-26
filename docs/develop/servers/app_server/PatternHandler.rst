PatternHandler class
####################

PatternHandler provides an easy way to integrate pattern support into
classes which require it, such as the DisplayDriver class.


Enumerated Types
================

pattern_enum
------------

- uint64 type64
- uint8 type8 [8]

Member Functions
================

PatternHandler()
----------------


1. Initialize internal RGBColor variables to black and white, respectively.
2. Set internal pattern to B_SOLID_HIGH (all 1's)

~PatternHandler()
-----------------

Empty


void SetTarget(int8 \*pattern)
------------------------------

Updates the pattern handler's pattern. It copies the pattern passed to
it, so it does NOT take responsibility for freeing any memory.


1. cast the passed pointer in such a way to copy it as a uint64 to the
   pattern_enum.type64 member

void SetColors(RGBColor c1, RGBColor c2)
----------------------------------------

Sets the internal high and low colors for the pattern handler. These
will be the colors returned when GetColor() is called.


1. Assign c1 to high color and c2 to low color


RGBColor GetColor(BPoint pt) RGBColor GetColor (float x, float y)
-----------------------------------------------------------------

bool GetValue(BPoint pt) bool GetValue (float x, float y)
---------------------------------------------------------

GetColor returns the color in the pattern at the specified location.
GetValue returns true for the high color and false for the low color.

1. xpos = x % 8, ypos = y % 8 2) value = pointer [ ypos ] & ( 1 << (7 - xpos) )
2. GetValue: return (value==0)?false:true
3. GetColor: return (value==0)?lowcolor:highcolor

