#ifndef _RASTERIZER_H
#define _RASTERIZER_H


#include <Bitmap.h>

#include "Halftone.h"
#include "ValidRect.h"


class Rasterizer {
public:
						Rasterizer(Halftone* halftone);
	virtual				~Rasterizer();

	/**
	 *  Sets the bitmap to be rasterized
	 *  Either the iterator methods HasNextLine() and RasterizeNextLine()
	 *  can be used to rasterize the bitmap line per line or the method
	 *	RasterizeBitamp()
	 *  can be used to rasterize the entire bitmap at once.
	 *  @param x the x position of the image on the page.
	 *  @param y the y position of the image on the page.
	 *  @param bitmap the bitamp to be rasterized.
	 *  @param height the page height.
	 *  @return true if the bitmap is not empty and false if the bitmap is
	 *		empty.
	 */
			bool		SetBitmap(int x, int y, BBitmap* bitmap,
							int pageHeight);

	// Is there a next line?
			bool		HasNextLine();
	// Rasterizes the next line and returns the line.
			const void*	RasterizeNextLine();
	
	// Iterates over all lines.
			void		RasterizeBitmap();

	// Returns the Halftone object specified in the constructor
			Halftone*	GetHalftone()
						{
							return fHalftone;
						}
	// The bounds of the bitmap to be rasterized
			RECT		GetBounds()
						{
							return fBounds;
						}
	// The width (in pixels) of the bounds passed to Rasterized()
			int			GetWidth()
						{
							return fWidth;
						}
	// The height (in pixels) of the bounds passed to Rasterize()
			int			GetHeight()
						{
							return fHeight;
						}
	// Returns the current x position
			int			GetX()
						{
							return fX;
						}
	// Returns the current y position
			int			GetY()
						{
							return fY;
						}
	
	// The method is called for each line in the bitmap.
	virtual	const void*	RasterizeLine(int x, int y,
							const ColorRGB32Little* source) = 0;

	// Returns the number of bytes to store widthInPixels pixels with
	// BPP = bitsPerPixel and padBytes number of pad bytes.
	static	int			RowBufferSize(int widthInPixels, int bitsPerPixel,
							int padBytes = 1)
						{
							int sizeInBytes = (widthInPixels * bitsPerPixel + 7)
								/ 8;
							return padBytes * ((sizeInBytes + padBytes - 1)
								/ padBytes);
						}

private:
			Halftone*	fHalftone;

			RECT		fBounds;
			int			fWidth;
			int			fHeight;
			int			fX;
			int			fY;

			const uchar* fBits;
			int			fBPR;
			int			fIndex;
};

#endif // _RASTERIZER_H
