#ifndef PNGDUMP_H
#define PNGDUMP_H

void SaveToPNG(const char *filename, const BRect &bounds, color_space space, 
	const void *bits, const int32 &bitslength, const int32 bytesperrow);


#endif
