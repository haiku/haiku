/*

Image Cache Item.

Copyright (c) 2003 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef _IMAGE_CACHE_ITEM_H
#define _IMAGE_CACHE_ITEM_H

#include "pdflib.h"
#include <Bitmap.h>

#include "pdflib.h"
#include "Utils.h"
#include "Cache.h"

class Image;

class ImageDescription : public CIDescription {
public:
	ImageDescription(PDF* pdf, BBitmap* bitmap, int mask);

	CacheItem* NewItem(int id);

	BBitmap* Bitmap() { return fBitmap; }
	int Width() const { return fWidth; }
	int Height() const { return fHeight; }
	color_space ColorSpace() const { return fColorSpace; }
	int Mask() const { return fMask; }
	
private:
	Image* Store(PDF* pdf, int id, BBitmap* bitmap, int mask);
	bool StoreBitmap(const char* fileName, BBitmap* bitmap);
	bool StorePNG(const char* fileName, BBitmap* bitmap);

	PDF*        fPDF;
	BBitmap*    fBitmap;
	int         fWidth, fHeight;
	color_space fColorSpace;
	int         fMask;
};

class Image : public CacheItem {
public:
	Image(PDF* pdf, int imageID, const char* fileName, int width, int height, color_space colorSpace, int mask);
	~Image();
	
	int ImageID() const { return fImageID; };
	const char* FileName() const  { return fFileName.String(); };
	int Width() const { return fWidth; };
	int Height() const { return fHeight; };	
	color_space ColorSpace() const { return fColorSpace; };
	int Mask() const { return fMask; };
	bool Equals(CIDescription* desc) const;
	bool Equals(BBitmap* bitmap) const;

private:
	PDF*        fPDF;
	int         fImageID;
	BString     fFileName;
	int         fWidth, fHeight;
	color_space fColorSpace;
	int         fMask;
};



#endif
