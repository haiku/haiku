#ifndef COLORUTILS_H_
#define COLORUTILS_H_

#include <GraphicsDefs.h>

// Quick assignment for rgb_color structs
void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);

// Given a color palette, returns the index of the closest match to
// the color passed to it. Alpha values are not considered
uint8 FindClosestColor(rgb_color *palette, rgb_color color);
uint16 FindClosestColor16(rgb_color color);
#endif