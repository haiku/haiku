#ifndef PNGDUMP_H
#define PNGDUMP_H

void Int32ToRGBColor(const uint32 *color32, const rgb_color *rgb);
void SaveToPNG(const char *filename, const BRect &bounds, color_space space, 
	const void *bits, const int32 &bitslength, const int32 bytesperrow);


#endif
