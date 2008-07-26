/*

Image Cache.

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

#ifndef _IMAGE_CACHE_H
#define _IMAGE_CACHE_H

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <String.h>

#include "pdflib.h"
#include "PrintUtils.h"
#include "Cache.h"

extern const char* kTemporaryPath;
extern const char* kCachePath;
extern const char* kImagePathPrefix;
extern const char* kMaskPathPrefix;

#define STORE_AS_BBITMAP 1

class ImageCache {
public:
	ImageCache();
	~ImageCache();
	void Flush();
	
	void NextPass();
	int GetImage(PDF* pdf, BBitmap* bitmap, int mask);
	int GetMask(PDF* pdf, const char* mask, int length, int width, int height, int bpc);

private:
	Cache fImageCache;
	Cache fMaskCache;
};

#endif

