#ifndef COLORUTILS_H_
#define COLORUTILS_H_

#include <GraphicsDefs.h>

// Quick assignment for rgb_color structs
void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);

// Given a color palette, returns the index of the closest match to
// the color passed to it. Alpha values are not considered
uint8 FindClosestColor(rgb_color *palette, rgb_color color);
uint16 FindClosestColor16(rgb_color color);

// Function which could be used to calculate gradient colors. Position is
// a floating point number such that 0.0 <= position <= 1.0. Any number outside
// this range will cause the function to fail and return the color (0,0,0,0)
// Alpha components are included in the calculations. 0 yields color #1
// and 1 yields color #2.
rgb_color MakeBlendColor(rgb_color col, rgb_color col2, float position);
#endif