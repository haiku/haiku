#include<unistd.h>
#include<sys/stat.h>
#include<Translator.h>
#include<TranslationUtils.h>
#include<TranslatorRoster.h>
#include<BitmapStream.h>
#include<File.h>
#include<Debug.h>

#include "ImageCache.h"


// Implementation of Image

Image::Image(int imageID, const char* fileName, int width, int height, color_space colorSpace, int mask)
	: fImageID(imageID)
	, fFileName(fileName)
	, fWidth(width)
	, fHeight(height)
	, fColorSpace(colorSpace)
	, fMask(mask)
{
}

bool Image::Equals(BBitmap* bitmap) const {
	BBitmap* bm = BTranslationUtils::GetBitmapFile(FileName());
	bool equals = false;
	if (bm) {
		bm->Lock();
		bitmap->Lock();
		
		equals = bm->BitsLength() == bitmap->BitsLength() &&
			bm->ColorSpace() == bitmap->ColorSpace() &&
			memcmp(bm->Bits(), bitmap->Bits(), bm->BitsLength()) == 0;
		
		bitmap->Unlock();
		bm->Unlock();
		delete bm;
	}
	return equals;
}


// Implementation of ImageReference

ImageReference::ImageReference(Image* image)
	: fImage(image)
{
}


// Implementation of ImageCache

const char* kTemporaryPath =   "/tmp/PDFWriter";
const char* kImageCachePath =  "/tmp/PDFWriter/Cache";
const char* kImagePathPrefix = "/tmp/PDFWriter/Cache/Image";

ImageCache::ImageCache() 
	: fNextID(0)
{
	mkdir(kTemporaryPath, 0777);
	mkdir(kImageCachePath, 0777);
}

ImageCache::~ImageCache() {
	rmdir(kImageCachePath);
}

void ImageCache::Flush(PDF* pdf) {
	for (int32 i = 0; i < fCache.CountItems(); i ++) {
		CachedImage* image = fCache.ItemAt(i);
		if (dynamic_cast<Image*>(image) != NULL) {
			PDF_close_image(pdf, image->ImageID());
			unlink(image->FileName());
		}
	}
}

int ImageCache::GetImage(PDF* pdf, BBitmap* bitmap, int mask) {
	int id = fNextID ++;
	CachedImage* image = fCache.ItemAt(id);
	// In 2. pass for each bitmap an entry exists
	if (image != NULL) {
		return image->ImageID();
	}
	// In 1. pass we create an entry for each bitmap
	image = Find(bitmap, mask);
	// Bitmap not in cache 
	if (image == NULL) {
		image = Store(pdf, id, bitmap, mask);
	} else {
		Image* im = dynamic_cast<Image*>(image);
		ASSERT(im != NULL);
		image = new ImageReference(im);
	}
	
	if (image == NULL) return -1; // error occured
	
	fCache.AddItem(image);
	ASSERT(fCache.CountItems() == fNextID);
	return image->ImageID();
}

CachedImage* ImageCache::Find(BBitmap* bitmap, int mask) {
	int w, h;
	color_space cs;
	bitmap->Lock();
	w = bitmap->Bounds().IntegerWidth()+1;
	h = bitmap->Bounds().IntegerHeight()+1;
	cs = bitmap->ColorSpace();
	bitmap->Unlock();

	for (int32 i = 0; i < fCache.CountItems(); i ++) {
		CachedImage* image = fCache.ItemAt(i);
		if (dynamic_cast<ImageReference*>(image) != NULL) continue;
		if (w != image->Width() || h != image->Height() ||
			cs != image->ColorSpace() ||
			mask != image->Mask()) continue;
		if (image->Equals(bitmap)) {
			return image;
		}
	}
	return NULL;
}

CachedImage* ImageCache::Store(PDF* pdf, int id, BBitmap* bitmap, int mask) {
	BString fileName(kImagePathPrefix);
	fileName << id << ".png";
	int w, h;
	color_space cs;
	bitmap->Lock();
	w = bitmap->Bounds().IntegerWidth()+1;
	h = bitmap->Bounds().IntegerHeight()+1;
	cs = bitmap->ColorSpace();
	bitmap->Unlock();

	if (!StoreBitmap(fileName.String(), bitmap)) return NULL;
	
	int image;
	image = PDF_open_image_file(pdf, "png", fileName.String(), 
		mask == -1 ? "" : "masked", mask == -1 ? 0 : mask);
	if (image < 0) return NULL;

	return new Image(image, fileName.String(), w, h, cs, mask);
}

bool ImageCache::StoreBitmap(const char* fileName, BBitmap* bitmap) {
	bool ok;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	BBitmapStream stream(bitmap); // init with contents of bitmap
	BFile file(fileName, B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
	ok = roster->Translate(&stream, NULL, NULL, &file, B_PNG_FORMAT) == B_OK;
	BBitmap *bm = NULL; stream.DetachBitmap(&bm); // otherwise bitmap destructor crashes here!
	ASSERT(bm == bitmap);
	return ok;
}