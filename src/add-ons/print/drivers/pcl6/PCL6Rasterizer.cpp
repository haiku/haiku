/* 
** PCL6Rasterizer.cpp
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "PCL6Rasterizer.h"

#include <stdio.h>

#ifdef _PCL6_RASTERIZER_TEST_

static void dump(uchar* buffer, int size);
static void dump_bits(uchar* buffer, int size);

#define DUMP(text, buffer, size) { fprintf text; dump(buffer, size); }
#define DUMP_BITS(text, buffer, size) { fprintf text; dump_bits(buffer, size); }

#else

#define DUMP(text, buffer, size) {}
#define DUMP_BITS(text, buffer, size) {}

#endif


// #pragma - MonochromeRasterizer


MonochromeRasterizer::MonochromeRasterizer(Halftone* halftone) 
	:
	PCL6Rasterizer(halftone),
	fOutBuffer(NULL)
{}

	
void 
MonochromeRasterizer::InitializeBuffer()
{
	fWidthByte = RowBufferSize(GetWidth(), 1, 1);
	// line length is a multiple of 4 bytes
	fOutRowSize = RowBufferSize(GetWidth(), 1, 4);
	fPadBytes = fOutRowSize - fWidthByte;
	// Total size
	SetOutBufferSize(fOutRowSize * GetHeight());
	PCL6Rasterizer::InitializeBuffer();
	fCurrentLine = GetOutBuffer();
}


const void*
MonochromeRasterizer::RasterizeLine(int x, int y,
	const ColorRGB32Little* source)
{
	GetHalftone()->Dither(fCurrentLine, (const uchar*)source, x, y, GetWidth());
	
	uchar* out = fCurrentLine;

	// invert pixels
	for (int w = fWidthByte; w > 0; w --, out ++)
		*out = ~*out;

	// pad with zeros
	for (int w = fPadBytes; w > 0; w --, out ++)
		*out = 0;
	
	void* result = fCurrentLine;
	fCurrentLine += fOutRowSize;
	return result;
}


// #pragma - ColorRGBRasterizer


ColorRGBRasterizer::ColorRGBRasterizer(Halftone* halftone) 
	:
	PCL6Rasterizer(halftone) 
{}


void 
ColorRGBRasterizer::InitializeBuffer() {
	fWidthByte = RowBufferSize(GetWidth(), 24, 1);
	// line length is a multiple of 4 bytes 
	fOutRowSize = RowBufferSize(GetWidth(), 24, 4);
	fPadBytes = fOutRowSize - fWidthByte;
	// Total size
	SetOutBufferSize(fOutRowSize * GetHeight());
	PCL6Rasterizer::InitializeBuffer();
	fCurrentLine = GetOutBuffer();
}


const void*
ColorRGBRasterizer::RasterizeLine(int x, int y,
	const ColorRGB32Little* source)
{
	uchar* out = fCurrentLine;
	int width = GetWidth();
	for (int w = width; w > 0; w --) {
		*out++ = source->red;
		*out++ = source->green;
		*out++ = source->blue;
		source ++;
	}
	
	// pad with 0s
	for (int w = fPadBytes; w > 0; w --, out ++)
		*out = 0;

	void* result = fCurrentLine;
	fCurrentLine += fOutRowSize;
	return result;
}


// #pragma - ColorRasterizer


ColorRasterizer::ColorRasterizer::ColorRasterizer(Halftone* halftone) 
	:
	PCL6Rasterizer(halftone) 
{
	for (int plane = 0; plane < 3; plane ++)
		fPlaneBuffers[plane] = NULL;

	halftone->SetPlanes(Halftone::kPlaneRGB1);
	halftone->SetBlackValue(Halftone::kLowValueMeansBlack);
}


ColorRasterizer::~ColorRasterizer() {
	for (int plane = 0; plane < 3; plane ++) {
		delete fPlaneBuffers[plane];
		fPlaneBuffers[plane] = NULL;
	}
}


void 
ColorRasterizer::InitializeBuffer() {
	fWidthByte = RowBufferSize(GetWidth(), 3, 1);
	// line length is a multiple of 4 bytes 
	fOutRowSize = RowBufferSize(GetWidth(), 3, 4);
	fPadBytes = fOutRowSize - fWidthByte;
	// Total size
	SetOutBufferSize(fOutRowSize * GetHeight());
	PCL6Rasterizer::InitializeBuffer();
	fCurrentLine = GetOutBuffer();
	
	fPlaneBufferSize = RowBufferSize(GetWidth(), 1, 1);
	for (int plane = 0; plane < 3; plane ++) {
		fPlaneBuffers[plane] = new uchar[fPlaneBufferSize];
	}
}


enum {
	kRed = 1,
	kGreen = 2,
	kBlue = 4,
};


const void*
ColorRasterizer::RasterizeLine(int x, int y, const ColorRGB32Little* source)
{
	DUMP((stderr, "\nRGB32 row at x %d y %d:\n", x, y), (uchar*)source,
		GetWidth() * 4);
	
	// dither each color component
	for (int plane = 0; plane < 3; plane ++)
		GetHalftone()->Dither(fPlaneBuffers[plane], (const uchar*)source, x, y,
			GetWidth());
	
	DUMP_BITS((stderr, "red   "), fPlaneBuffers[0], fPlaneBufferSize);
	DUMP_BITS((stderr, "green "), fPlaneBuffers[1], fPlaneBufferSize);
	DUMP_BITS((stderr, "blue  "), fPlaneBuffers[2], fPlaneBufferSize);
	
	MergePlaneBuffersToCurrentLine();

	DUMP_BITS((stderr, "merged\n"), fCurrentLine, fOutRowSize);
	DUMP((stderr, "\n"), fCurrentLine, fOutRowSize);

	void* result = fCurrentLine;
	fCurrentLine += fOutRowSize;
	return result;
}


void
ColorRasterizer::MergePlaneBuffersToCurrentLine()
{
	// merge the three planes into output buffer
	int remainingPixels = GetWidth();
	uchar* out = fCurrentLine;
	uchar value = 0;
	uchar outMask = 0x80; // current bit mask (1 << (8 - bit)) in output buffer
	
	// iterate over the three plane buffers
	for (int i = 0; i < fPlaneBufferSize; i ++) {
		int pixels = 8;
		if (remainingPixels < 8)
			pixels = remainingPixels;

		remainingPixels -= pixels;

		if (remainingPixels >= 8) {
			register const uchar
				red = fPlaneBuffers[0][i],
				green = fPlaneBuffers[1][i],
				blue = fPlaneBuffers[2][i];

			register uchar value = 0;
			if (red & 0x80) value =  0x80;
			if (red & 0x40) value |=  0x10;
			if (red & 0x20) value |=  0x02;

			if (green & 0x80) value |= 0x40;
			if (green & 0x40) value |= 0x08;
			if (green & 0x20) value |= 0x01;			

			if (blue & 0x80) value |= 0x20;
			if (blue & 0x40) value |= 0x04;

			*out++ = value;
			
			value = 0;
			if (blue & 0x20) value =  0x80;
			if (blue & 0x10) value |=  0x10;
			if (blue & 0x08) value |=  0x02;

			if (red & 0x10) value |= 0x40;
			if (red & 0x08) value |= 0x08;
			if (red & 0x04) value |= 0x01;

			if (green & 0x10) value |= 0x20;
			if (green & 0x08) value |= 0x04;
			*out++ = value;
			
			value = 0;
			if (green & 0x04) value =  0x80;
			if (green & 0x02) value |=  0x10;
			if (green & 0x01) value |=  0x02;

			if (blue & 0x04) value |= 0x40;
			if (blue & 0x02) value |= 0x08;
			if (blue & 0x01) value |= 0x01;

			if (red & 0x02) value |= 0x20;
			if (red & 0x01) value |= 0x04;
			*out++ = value;
	
		} else {
			register const uchar
				red = fPlaneBuffers[0][i],
				green = fPlaneBuffers[1][i],
				blue = fPlaneBuffers[2][i];
			// for each bit in the current byte of each plane	
			uchar mask = 0x80;
			for (; pixels > 0; pixels --) {			
				int rgb = 0;
				
				if (red & mask)
					rgb |= kRed;
				if (green & mask)
					rgb |= kGreen;
				if (blue & mask)
					rgb |= kBlue;
				
				for (int plane = 0; plane < 3; plane ++) {
					// copy pixel value to output value
					if (rgb & (1 << plane))
						value |= outMask;
				
					// increment output mask
					if (outMask == 0x01) {
						outMask = 0x80;
						// write output value to output buffer
						*out = value;
						out ++;
						value = 0;
					} else
						outMask >>= 1;
				}
				mask >>= 1;
			}	
		}
	}
	
	// write last output value
	if (outMask != 0x80) {
		do {
			value |= outMask;
			outMask >>= 1;
		} while (outMask > 0);
	
		*out = value;
		out ++;
	}
	
	if (out - fCurrentLine != fWidthByte)
		fprintf(stderr, "Error buffer overflow: %d != %d\n", fWidthByte,
			static_cast<int>(out - fCurrentLine));
	
	// pad with zeros
	for (int w = fPadBytes; w > 0; w --, out ++)
		*out = 0xff;
	
	if (out - fCurrentLine != fOutRowSize)
		fprintf(stderr, "Error buffer overflow: %d != %d\n", fOutRowSize,
			static_cast<int>(out - fCurrentLine));
}


#ifdef _PCL6_RASTERIZER_TEST_
#include <Application.h>
#include <Bitmap.h>
#include <stdio.h>

#define COLUMNS 40
#define BIT_COLUMNS 6


static void
dump(uchar* buffer, int size)
{
	int x = 0;
	for (int i = 0; i < size; i ++) {
		if ((x % COLUMNS) == COLUMNS - 1) {
			fprintf(stderr, "\n");
		} else if (i > 0) {
			fprintf(stderr, " ");
		}
		
		fprintf(stderr, "%2.2x", (int)*buffer);
		buffer ++;
		
		x ++;
	}
	
	fprintf(stderr, "\n");
}


static void
dump_bits(uchar* buffer, int size)
{
	int x = 0;
	for (int i = 0; i < size; i ++) {
		if ((x % COLUMNS) == COLUMNS - 1) {
			fprintf(stderr, "\n");
		} else if (i > 0) {
			fprintf(stderr, " ");
		}
		
		uchar value = *buffer;
		for (int bit = 0; bit < 8; bit ++) {
			if (value & (1 << bit)) {
				fprintf(stderr, "*");
			} else {
				fprintf(stderr, ".");
			}
		}
		buffer ++;
		
		x ++;
	}
	
	fprintf(stderr, "\n");		
}


static void
fill(uchar* _row, int width, ColorRGB32Little color)
{
	ColorRGB32Little* row = static_cast<ColorRGB32Little*>(_row);
	for (int i = 0; i < width; i ++) {
		*row = color;
		row ++;
	}
}


static void
initializeBitmap(BBitmap* bitmap, int width, int height)
{
	int bpr = bitmap->BytesPerRow();
	uchar* row = (uchar*)bitmap->Bits();
	// BGRA
	ColorRGB32Little black = {0, 0, 0, 0};
	ColorRGB32Little white = {255, 255, 255, 0};
	ColorRGB32Little red = {0, 0, 255, 0};
	ColorRGB32Little green = {0, 255, 0, 0};
	ColorRGB32Little blue = {255, 0, 0, 0};

	fprintf(stderr, "black row\n");
	fill(row, width, black);
	row += bpr;
	
	fprintf(stderr, "white row\n");
	fill(row, width, white);
	row += bpr;
	
	fprintf(stderr, "red row\n");
	fill(row, width, red);
	row += bpr;
	
	fprintf(stderr, "red green blue pattern");
	ColorRGB32Little* color = (ColorRGB32Little*)row;
	for (int i = 0; i < width; i++) {
		switch (i % 3) {
			case 0: 
				*color = red;
				break;
			case 1:
				*color = green;
				break;
			case 2:
				*color = blue;
				break;
		}
		color ++;
	}
}


int
main() 
{
	const int width = 10;
	const int height = 4;

	fprintf(stderr, "width:  %d\nheight: %d\n", width, height);
	BApplication app("application/pcl6-rasterizer-test");
#if 1
	Halftone halftone(B_RGB32, 0.25f, 0.0f, Halftone::kType1); 
#else
	Halftone halftone(B_RGB32, 0.25f, 0.0f, Halftone::kTypeFloydSteinberg);
#endif
	ColorRasterizer rasterizer(&halftone);
	BBitmap bitmap(BRect(0, 0, width - 1, height - 1), B_RGB32);

	initializeBitmap(&bitmap, width, height);

	rasterizer.SetBitmap(0, 0, &bitmap, height);
	rasterizer.InitializeBuffer();
	while (rasterizer.HasNextLine()) {
		rasterizer.RasterizeNextLine();
	}
}

#endif // _PCL6_RASTERIZER_TEST_
