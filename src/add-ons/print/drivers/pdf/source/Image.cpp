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

#include <unistd.h>
#include <sys/stat.h>
#include <File.h>
#include <Translator.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Debug.h>

#include "Report.h"
#include "Image.h"
#include "ImageCache.h"

// Implementation of ImageDescription

ImageDescription::ImageDescription(PDF* pdf, BBitmap* bitmap, int mask)
	: fPDF(pdf)
	, fBitmap(bitmap)
	, fMask(mask)
{
	bitmap->Lock();
	fWidth = bitmap->Bounds().IntegerWidth()+1;
	fHeight = bitmap->Bounds().IntegerHeight()+1;
	fColorSpace = bitmap->ColorSpace();
	bitmap->Unlock();
}

CacheItem* ImageDescription::NewItem(int id) {
	REPORT(kDebug, -1, "ImageDescription::NewItem %d", id);
	Image* image = Store(fPDF, id, fBitmap, fMask);
	if (image == NULL) {
		REPORT(kDebug, -1, "Could not store image in cache!");
	}
}

Image* ImageDescription::Store(PDF* pdf, int id, BBitmap* bitmap, int mask) {
	BString fileName(kImagePathPrefix);
	BString pdfFileName;
	int w, h;
	color_space cs;

	fileName << id;
	pdfFileName = fileName;
	pdfFileName << ".png";
	
	bitmap->Lock();
	w = bitmap->Bounds().IntegerWidth()+1;
	h = bitmap->Bounds().IntegerHeight()+1;
	cs = bitmap->ColorSpace();
	bitmap->Unlock();

#if STORE_AS_BBITMAP
	fileName << ".bitmap";
#else
	fileName << ".png";
#endif
	if (!StorePNG(pdfFileName.String(), bitmap)) {
		REPORT(kError, -1, "Image cache could not store image as PNG file.");
		return NULL;
	}

	if (!StoreBitmap(fileName.String(), bitmap)) {
		REPORT(kError, -1, "Image cache could not store image as flattend BBitmap.");
		unlink(pdfFileName.String());
		return NULL;
	}
	
	int image;
	image = PDF_open_image_file(pdf, "png", pdfFileName.String(), 
		mask == -1 ? "" : "masked", mask == -1 ? 0 : mask);

#if STORE_AS_BBITMAP
	unlink(pdfFileName.String());
#endif

	if (image < 0) {
		REPORT(kError, -1, "Image cache could not embed image.");
		return NULL;
	}

	return new Image(pdf, image, fileName.String(), w, h, cs, mask);
}

bool ImageDescription::StorePNG(const char* fileName, BBitmap* bitmap) {
	bool ok = true;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BBitmapStream stream(bitmap); // init with contents of bitmap
	BFile file(fileName, B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
	ok = roster->Translate(&stream, NULL, NULL, &file, B_PNG_FORMAT) == B_OK;
	BBitmap *bm = NULL; stream.DetachBitmap(&bm); // otherwise bitmap destructor crashes here!
	ASSERT(bm == bitmap);
	return ok;
}

bool ImageDescription::StoreBitmap(const char* fileName, BBitmap* bitmap) {
	bool ok = true;
#if STORE_AS_BBITMAP
	BMessage msg;
	BFile file(fileName, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	ok = file.InitCheck() == B_OK && bitmap->Archive(&msg) == B_OK && msg.Flatten(&file) == B_OK;
#endif
	return ok;
}

// Implementation of Image

Image::Image(PDF* pdf, int imageID, const char* fileName, int width, int height, color_space colorSpace, int mask)
	: fPDF(pdf)
	, fImageID(imageID)
	, fFileName(fileName)
	, fWidth(width)
	, fHeight(height)
	, fColorSpace(colorSpace)
	, fMask(mask)
{
}

Image::~Image() {
	PDF_close_image(fPDF, ImageID());
	unlink(FileName());
}

bool Image::Equals(CIDescription* description) const {
	REPORT(kDebug, -1, "Image::Equals called.");
	ImageDescription* desc = dynamic_cast<ImageDescription*>(description);
	ASSERT(desc != NULL);
	if (desc->Width() != Width() || desc->Height() != Height() ||
		desc->ColorSpace() != ColorSpace() ||
		desc->Mask() != Mask()) return false;
	return Equals(desc->Bitmap());
}

bool Image::Equals(BBitmap* bitmap) const {
	REPORT(kDebug, -1, "Image::Equals called.");
	bool equals = false;
	BBitmap* bm = NULL;
#if STORE_AS_BBITMAP
	BMessage msg;
	BFile file(FileName(), B_READ_ONLY);
	if (file.InitCheck() == B_OK && msg.Unflatten(&file) == B_OK) {
		BArchivable* a = BBitmap::Instantiate(&msg);
		bm = dynamic_cast<BBitmap*>(a);
		if (bm == NULL && a != NULL) delete a;
	}
#else
	bm = BTranslationUtils::GetBitmapFile(FileName());
#endif
	if (bm) {
		bm->Lock();
		bitmap->Lock();
		
		equals = bm->BitsLength() == bitmap->BitsLength() &&
			bm->ColorSpace() == bitmap->ColorSpace() &&
			memcmp(bm->Bits(), bitmap->Bits(), bm->BitsLength()) == 0;
		
		bitmap->Unlock();
		bm->Unlock();
		delete bm;
	} else {
		REPORT(kError, -1, "Could not load image from cache!");
	}
	return equals;
}
