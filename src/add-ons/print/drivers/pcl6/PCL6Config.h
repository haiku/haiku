/*
 * PCL6Config.cpp
 * Copyright 2005 Michael Pfeiffer.
 */
#ifndef _PCL6_CONFIG
#define _PCL6_CONFIG

// Compression method configuration
// Set to 0 to disable compression and to 1 to enable compression
// Note compression takes place only if it takes less space than uncompressed
// data!

// Run-Length-Encoding Compression
#define ENABLE_RLE_COMPRESSION			1
//#define ENABLE_RLE_COMPRESSION		0

// Delta Row Compression
#define ENABLE_DELTA_ROW_COMPRESSION	1

// Color depth for color printing.
// Use either 1 or 8.
// If 1 bit depth is used, the class Halftone is used for dithering
// otherwise dithering is not performed.
#define COLOR_DEPTH						1
//#define COLOR_DEPTH					8

#define DISPLAY_COMPRESSION_STATISTICS	0

#endif
