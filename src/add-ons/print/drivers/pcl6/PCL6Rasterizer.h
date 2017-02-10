/* 
** PCL6Rasterizer.h
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _PCL6_RASTERIZER_H
#define _PCL6_RASTERIZER_H


#include "Rasterizer.h"


class PCL6Rasterizer : public Rasterizer
{
public:
						PCL6Rasterizer(Halftone* halftone)
						:
						Rasterizer(halftone),
						fOutBuffer(NULL),
						fOutBufferSize(0)
						{}
	
						~PCL6Rasterizer()
						{
							delete fOutBuffer;
							fOutBuffer = NULL;
						}
	
			void		SetOutBufferSize(int size)
						{
							fOutBufferSize = size;
						}
			int			GetOutBufferSize()
						{
							return fOutBufferSize;
						}
			uchar*		GetOutBuffer()
						{
							return fOutBuffer;
						}

	virtual	void		InitializeBuffer()
						{
							fOutBuffer = new uchar[fOutBufferSize];
						}
	
	virtual	int			GetOutRowSize() = 0;
	
private:
			uchar*		fOutBuffer;
			int			fOutBufferSize;
};


class MonochromeRasterizer : public PCL6Rasterizer 
{
public:
						MonochromeRasterizer(Halftone* halftone); 

			void		InitializeBuffer();
		
			int			GetOutRowSize() 
						{
							return fOutRowSize;
						}
	
			const void*	RasterizeLine(int x, int y,
							const ColorRGB32Little* source);

private:
			int			fWidthByte;
			int			fOutRowSize;
			int			fPadBytes;
			int			fOutSize;
			uchar*		fOutBuffer;
			uchar*		fCurrentLine;
};


// Output format RGB 8bit per channel
class ColorRGBRasterizer : public PCL6Rasterizer
{
public:
						ColorRGBRasterizer(Halftone* halftone);
	
			void		InitializeBuffer();

			int			GetOutRowSize()
						{
							return fOutRowSize;
						}
		
			const void*	RasterizeLine(int x, int y,
							const ColorRGB32Little* source);

private:
			int			fWidthByte;
			int			fOutRowSize;
			int			fPadBytes;
			uchar*		fCurrentLine;
};


typedef uchar* PlaneBuffer;


// Output format: RGB 1bit per channel
// Class Halftone is used for dithering
class ColorRasterizer : public PCL6Rasterizer
{
public:
						ColorRasterizer(Halftone* halftone);
	
						~ColorRasterizer();
	
			void		InitializeBuffer();

			int			GetOutRowSize()
						{
							return fOutRowSize;
						}
		
			const void*	RasterizeLine(int x, int y,
							const ColorRGB32Little* source);

private:
			void		MergePlaneBuffersToCurrentLine();

			int			fWidthByte;
			int			fOutRowSize;
			int			fPadBytes;
			uchar*		fCurrentLine;
	
			int			fPlaneBufferSize;
			PlaneBuffer	fPlaneBuffers[3];
};

#endif // _PCL6_RASTERIZER_H
