/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include <GraphicsDefs.h>
#include <InterfaceKit.h>
#include <String.h>

#include <stdio.h>

#include "PictureTest.h"

#define TEST_AND_RETURN(condition, message, result) \
	{ \
		if (condition) { \
			SetErrorMessage(message); \
			return result; \
		} \
	}

template <class T>
class AutoDelete
{
public:
	AutoDelete(T *object) : fObject(object) { }
	~AutoDelete() { delete fObject; fObject = NULL; }
	
	T* Release() { T* object = fObject; fObject = NULL; return object; }
	
private:
	T *fObject;
};

class OffscreenBitmap {
public:
	OffscreenBitmap(BRect frame, color_space colorSpace);
	virtual ~OffscreenBitmap();
	status_t InitCheck() const { return fStatus; }
	
	BView *View();
	BBitmap *Copy();

private:
	BRect fFrame;
	color_space fColorSpace;
	status_t fStatus;
	
	BBitmap *fBitmap;
	BView *fView;
};

OffscreenBitmap::OffscreenBitmap(BRect frame, color_space colorSpace)
	: fFrame(frame)
	, fColorSpace(colorSpace)
	, fStatus(B_ERROR)
	, fBitmap(NULL)
	, fView(NULL)
{
	BBitmap *bitmap = new BBitmap(frame, fColorSpace, true);
	AutoDelete<BBitmap> _bitmap(bitmap);
	if (bitmap == NULL || bitmap->IsValid() == false || bitmap->InitCheck() != B_OK)
		return;
	
	BView *view = new BView(frame, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
	AutoDelete<BView> _view(view);
	if (view == NULL)
		return;
	
	bitmap->Lock();
	bitmap->AddChild(view);
	// bitmap is locked during the life time of this object
	
	fBitmap = _bitmap.Release();
	fView = _view.Release();
	fStatus = B_OK;
}

OffscreenBitmap::~OffscreenBitmap()
{
	if (fStatus != B_OK)
		return;
	
	fView->RemoveSelf();
	fBitmap->Unlock();
	delete fView;
	fView = NULL;
	
	delete fBitmap;
	fBitmap = NULL;
	
	fStatus = B_ERROR;
}

BView *
OffscreenBitmap::View()
{
	return fView;
}

BBitmap*
OffscreenBitmap::Copy()
{
	// the result bitmap that does not accept views	
	// to save resources in the application server
	BBitmap *copy = new BBitmap(fFrame, fColorSpace, false);
	AutoDelete<BBitmap> _copy(copy);
	if (copy == NULL || copy->IsValid() == false || copy->InitCheck() != B_OK)
		return NULL;

	fView->Sync();
	fBitmap->Unlock();
	
	memcpy(copy->Bits(), fBitmap->Bits(), fBitmap->BitsLength());	
	
	fBitmap->Lock();

	return _copy.Release();
}

PictureTest::PictureTest()
	: fColorSpace(B_RGBA32)
	, fDirectBitmap(NULL)
	, fBitmapFromPicture(NULL)
	, fBitmapFromRestoredPicture(NULL)
{
}


BBitmap*
PictureTest::DirectBitmap(bool detach)
{
	BBitmap* bitmap = fDirectBitmap;
	if (detach)
		fDirectBitmap = NULL;
	return bitmap;
}

BBitmap*
PictureTest::BitmapFromPicture(bool detach)
{
	BBitmap* bitmap = fBitmapFromPicture;
	if (detach)
		fBitmapFromPicture = NULL;
	return bitmap;
}


BBitmap*
PictureTest::BitmapFromRestoredPicture(bool detach)
{
	BBitmap* bitmap = fBitmapFromRestoredPicture;
	if (detach)
		fBitmapFromRestoredPicture = NULL;
	return bitmap;
}


PictureTest::~PictureTest() 
{
	CleanUp();
}

void
PictureTest::CleanUp()
{
	delete fBitmapFromPicture;
	fBitmapFromPicture = NULL;
	delete fBitmapFromRestoredPicture;
	fBitmapFromRestoredPicture = NULL;
	fErrorMessage = "";
}

void
PictureTest::SetErrorMessage(const char *message)
{
	if (fErrorMessage.Length() == 0)
		fErrorMessage = message;
}

bool
PictureTest::Test(draw_func* func, BRect frame)
{
	CleanUp();

	fDirectBitmap = CreateBitmap(func, frame);
	TEST_AND_RETURN(fDirectBitmap == NULL, "Could not create direct draw bitmap!", false);

	BPicture *picture = RecordPicture(func, frame);
	AutoDelete<BPicture> _picture(picture);
	TEST_AND_RETURN(picture == NULL, "Picture could not be recorded!", false);
		
	BPicture *archivedPicture = SaveAndRestore(picture);
	AutoDelete<BPicture> _archivedPicture(archivedPicture);
	TEST_AND_RETURN(picture == NULL, "Picture could not be flattened and unflattened!", false);
	
	fBitmapFromPicture = CreateBitmap(picture, frame);
	TEST_AND_RETURN(fBitmapFromPicture == NULL, "Could not create bitmap from original picture!", false);
		
	fBitmapFromRestoredPicture = CreateBitmap(archivedPicture, frame);
	TEST_AND_RETURN(fBitmapFromRestoredPicture == NULL, "Could not create bitmap from archived picture!", false);
	
	BString reason;
	if (!IsSame(fDirectBitmap, fBitmapFromPicture, reason)) {
		BString message("Bitmap from picture differs from direct drawing bitmap: ");
		message += reason;
		SetErrorMessage(message.String());
		return false;
	}
	if (!IsSame(fDirectBitmap, fBitmapFromRestoredPicture, reason)) {
		BString message("Bitmap from restored picture differs from direct drawing bitmap: ");
		message += reason;
		SetErrorMessage(message.String());
		return false;
	}
	return true;
}


BBitmap *
PictureTest::CreateBitmap(draw_func* func, BRect frame)
{
	OffscreenBitmap bitmap(frame, fColorSpace);
	TEST_AND_RETURN(bitmap.InitCheck() != B_OK, "Offscreen bitmap for direct drawing could not be created!" , NULL);
	func(bitmap.View(), frame);
	return bitmap.Copy();
}

BPicture *
PictureTest::RecordPicture(draw_func* func, BRect frame)
{
	OffscreenBitmap bitmap(frame, fColorSpace);
	TEST_AND_RETURN(bitmap.InitCheck() != B_OK, "Offscreen bitmap for picture recording could not be created!" , NULL);
		
	BView *view = bitmap.View();
	// record
	BPicture *picture = new BPicture();	
	view->BeginPicture(picture);
	func(view, frame);
	picture = view->EndPicture();	
	
	return picture;
}

BBitmap *
PictureTest::CreateBitmap(BPicture *picture, BRect frame)
{
	OffscreenBitmap bitmap(frame, fColorSpace);
	TEST_AND_RETURN(bitmap.InitCheck() != B_OK, "Offscreen bitmap for picture drawing could not be created!" , NULL);

	BView *view = bitmap.View();		
	view->DrawPicture(picture);
	view->Sync();
	
	return bitmap.Copy();
}


static void setMismatchReason(int32 x, int32 y, uint8 *pixel1, uint8 *pixel2, 
	int32 bpp, BString &reason)
{
	char buffer1[32];
	char buffer2[32];
	uint32 color1 = 0;
	uint32 color2 = 0;
	memcpy(&color1, pixel1, bpp);
	memcpy(&color2, pixel2, bpp);
	sprintf(buffer1, "0x%8.8x", (int)color1);
	sprintf(buffer2, "0x%8.8x", (int)color2);
	
	reason = "Pixel at ";
	reason << x << ", " << y << " differs: " << buffer1 << " != " << buffer2;
}


bool
PictureTest::IsSame(BBitmap *bitmap1, BBitmap *bitmap2, BString &reason)
{
	if (bitmap1->ColorSpace() != bitmap2->ColorSpace()) {
		reason = "ColorSpace() differs";
		return false;
	}
	
	if (bitmap1->BitsLength() != bitmap2->BitsLength()) {
		reason = "BitsLength() differs";
		return false;
	}
	
	size_t rowAlignment;
	size_t pixelChunk;
	size_t pixelsPerChunk;
	if (get_pixel_size_for(bitmap1->ColorSpace(), &pixelChunk, &rowAlignment, 
		&pixelsPerChunk) != B_OK) {
		reason = "get_pixel_size_for() not supported for this color space";
		return false;
	}
	if (pixelsPerChunk != 1) {
		reason = "Unsupported color_space; IsSame(...) supports 1 pixels per chunk only";
		return false;
	}
	int32 bpp = (int32)pixelChunk;
	uint8* row1 = (uint8*)bitmap1->Bits();
	uint8* row2 = (uint8*)bitmap2->Bits();
	int32 bpr = bitmap1->BytesPerRow();
	int32 width = bitmap1->Bounds().IntegerWidth() + 1;
	int32 height = bitmap1->Bounds().IntegerHeight() + 1;
	for (int y = 0; y < height; y ++, row1 += bpr, row2 += bpr) {
		uint8* pixel1 = row1;
		uint8* pixel2 = row2;
		for (int x = 0; x < width; x ++, pixel1 += bpp, pixel2 += bpp) {
			if (memcmp(pixel1, pixel2, bpp) != 0) {
				setMismatchReason(x, y, pixel1, pixel2, bpp, reason);
				return false;
			}
		}
	}

	reason = "";
	return true;
}


FlattenPictureTest::FlattenPictureTest()
{
}

BPicture *
FlattenPictureTest::SaveAndRestore(BPicture *picture)
{
	BMallocIO *data = new BMallocIO();
	AutoDelete<BMallocIO> _data(data);
	TEST_AND_RETURN(data == NULL, "BMallocIO could not be allocated for flattening the picture!" , NULL);
	
	picture->Flatten(data);
	
	data->Seek(0, SEEK_SET);
	BPicture *archivedPicture = new BPicture();
	TEST_AND_RETURN(archivedPicture == NULL, "BPicture could not be allocated for unflattening the picture!" , NULL);
	archivedPicture->Unflatten(data);
		
	return archivedPicture;
}

ArchivePictureTest::ArchivePictureTest()
{
}

BPicture *
ArchivePictureTest::SaveAndRestore(BPicture *picture)
{
	BMessage archive;
	TEST_AND_RETURN(picture->Archive(&archive) != B_OK, "Picture could not be archived to BMessage", NULL);

	BArchivable *archivable = BPicture::Instantiate(&archive);
	AutoDelete<BArchivable> _archivable(archivable);
	TEST_AND_RETURN(archivable == NULL, "Picture could not be instantiated from BMessage", NULL);
	
	BPicture *archivedPicture = dynamic_cast<BPicture*>(archivable);
	TEST_AND_RETURN(archivedPicture == NULL, "Picture could not be restored from BMessage", NULL);

	_archivable.Release();
	return archivedPicture;
}

