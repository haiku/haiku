#ifndef _IMAGE_CACHE_H
#define _IMAGE_CACHE_H

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <String.h>

#include "pdflib.h"
#include "Utils.h"

/*
	
*/

class CachedImage {
public:
	CachedImage() {};
	virtual ~CachedImage() {};
	virtual int ImageID() const = 0;
	virtual const char* FileName() const = 0;
	virtual int Width() const = 0;
	virtual int Height() const = 0;	
	virtual color_space ColorSpace() const = 0;
	virtual int Mask() const = 0;
	virtual bool Equals(BBitmap* bitmap) const = 0;
};

class Image : public CachedImage {
public:
	Image(int imageID, const char* fileName, int width, int height, color_space colorSpace, int mask);
	
	int ImageID() const { return fImageID; };
	const char* FileName() const  { return fFileName.String(); };
	int Width() const { return fWidth; };
	int Height() const { return fHeight; };	
	color_space ColorSpace() const { return fColorSpace; };
	int Mask() const { return fMask; };
	bool Equals(BBitmap* bitmap) const;

private:
	int         fImageID;
	BString     fFileName;
	int         fWidth, fHeight;
	color_space fColorSpace;
	int         fMask;
};

class ImageReference : public CachedImage {
public:
	ImageReference(Image* image);
		
	int ImageID() const { return fImage->ImageID(); };
	const char* FileName() const  { return fImage->FileName(); };
	int Width() const { return fImage->Width(); };
	int Height() const { return fImage->Height(); };	
	color_space ColorSpace() const { return fImage->ColorSpace(); };
	int Mask() const { return fImage->Mask(); };
	bool Equals(BBitmap* bitmap) const { return fImage->Equals(bitmap); }

private:
	Image* fImage;
};

class ImageCache {
public:
	ImageCache();
	~ImageCache();
	void Flush(PDF* pdf);
	
	void ResetID() { fNextID = 0; }
	int GetImage(PDF* pdf, BBitmap* bitmap, int mask);

private:
	CachedImage* Find(BBitmap* bitmap, int mask);
	CachedImage* Store(PDF* pdf, int id, BBitmap* bitmap, int mask);
	bool StoreBitmap(const char* fileName, BBitmap* bitmap);
	
	int fNextID;
	TList<CachedImage> fCache;
};

#endif

