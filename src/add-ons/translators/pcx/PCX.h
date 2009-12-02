/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PCX_H
#define PCX_H


#include <GraphicsDefs.h>
#include <BufferIO.h>
#include <TranslatorFormats.h>

class BMessage;


namespace PCX {

struct pcx_header {
	uint8	manufacturer;
	uint8	version;
	uint8	encoding;
	uint8	bitsPerPixel;
	uint16	xMin, yMin, xMax, yMax;
	uint16	hDpi, vDpi;
	uint8	colormap[48];
	uint8	reserved;
	uint8	numPlanes;
	uint16	bytesPerLine;
	uint16	paletteInfo;
	uint16	hScreenSize, vScreenSize;
	uint8	filler[54];
	bool IsValid() const;
	void SwapToHost();
	void SwapFromHost();
} _PACKED;


struct rgba32_color {
	uint8	blue;
	uint8	green;
	uint8	red;
	uint8	alpha;

	inline bool
	operator==(const rgba32_color& other) const
	{
		return red == other.red && green == other.green && blue == other.blue;
	}
};


extern status_t identify(BMessage *settings, BPositionIO &stream, uint8 &type, int32 &bitsPerPixel);
extern status_t convert_pcx_to_bits(BMessage *settings, BPositionIO &source, BPositionIO &target);

}	// namespace PCX

#endif	/* PCX_H */
