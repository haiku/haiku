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

#include <unistd.h>
#include <Debug.h>
#include <OS.h>

#include "Report.h"
#include "ImageCache.h"
#include "Image.h"
#include "Mask.h"

const char* kTemporaryPath =   "/tmp/PDFWriter";
const char* kCachePath =  NULL;
const char* kImagePathPrefix = NULL;
const char* kMaskPathPrefix = NULL;

// Implementation of ImageCache

ImageCache::ImageCache() 
{
	BString path(kTemporaryPath);
	path << "/Cache" << (int)find_thread(NULL);
	kCachePath = strdup(path.String());
	BString path2(path);
	path += "/Image";
	kImagePathPrefix = strdup(path.String());
	path2 += "/Mask";
	kMaskPathPrefix = strdup(path2.String());
	mkdir(kTemporaryPath, 0777);
	mkdir(kCachePath, 0777);
}

ImageCache::~ImageCache() {
	rmdir(kCachePath);
	free((void*)kCachePath);
	free((void*)kImagePathPrefix);
	free((void*)kMaskPathPrefix);
}

void ImageCache::Flush() {
	fImageCache.MakeEmpty();
	fMaskCache.MakeEmpty();
}

void ImageCache::NextPass() {
	fImageCache.NextPass();
	fMaskCache.NextPass();
}

int ImageCache::GetImage(PDF* pdf, BBitmap* bitmap, int mask) {
	ImageDescription desc(pdf, bitmap, mask);
	CacheItem* item = fImageCache.Find(&desc);
	Image* image = dynamic_cast<Image*>(item);
	if (image) {
		return image->ImageID();
	}
	REPORT(kError, -1, "Image cache could not find image. Please make sure to have enough disk space available.");
	return -1;
}

int ImageCache::GetMask(PDF* pdf, const char* mask, int length, int width, int height, int bpc) {
	MaskDescription desc(pdf, mask, length, width, height, bpc);
	CacheItem* item = fMaskCache.Find(&desc);
	Mask* image = dynamic_cast<Mask*>(item);
	if (image) {
		return image->ImageID();
	}
	REPORT(kError, -1, "Mask cache could not find image. Please make sure to have enough disk space available.");
	return -1;
}


