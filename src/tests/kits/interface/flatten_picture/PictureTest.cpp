/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include <InterfaceKit.h>
#include <String.h>

#include <stdio.h>

#include "PictureTest.h"

#define TEST_AND_RETURN(condition, message, result) \
	{ \
		if (condition) { \
			fErrorMessage = message; \
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
	
	TEST_AND_RETURN(!IsSame(fDirectBitmap, fBitmapFromPicture), "Bitmap from picture differs from direct drawing bitmap", false);
	TEST_AND_RETURN(!IsSame(fDirectBitmap, fBitmapFromRestoredPicture), "Bitmap from picture differs from direct drawing bitmap", false);
	return true;
}


BBitmap *
PictureTest::CreateBitmap(draw_func* func, BRect frame)
{
	OffscreenBitmap bitmap(frame, fColorSpace);
	if (bitmap.InitCheck() != B_OK)
		return NULL;
	func(bitmap.View(), frame);
	return bitmap.Copy();
}

BPicture *
PictureTest::RecordPicture(draw_func* func, BRect frame)
{
	OffscreenBitmap bitmap(frame, fColorSpace);
	if (bitmap.InitCheck() != B_OK)
		return NULL;
		
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
	if (bitmap.InitCheck() != B_OK)
		return NULL;

	BView *view = bitmap.View();		
	view->DrawPicture(picture);
	view->Sync();
	
	return bitmap.Copy();
}


bool
PictureTest::IsSame(BBitmap *bitmap1, BBitmap *bitmap2)
{
	if (bitmap1->ColorSpace() != bitmap2->ColorSpace())
		return false;
	
	if (bitmap1->BitsLength() != bitmap2->BitsLength())
		return false;
	
	return memcmp(bitmap1->Bits(), bitmap2->Bits(), bitmap1->BitsLength()) == 0;
}


FlattenPictureTest::FlattenPictureTest()
{
}

BPicture *
FlattenPictureTest::SaveAndRestore(BPicture *picture)
{
	BMallocIO *data = new BMallocIO();
	AutoDelete<BMallocIO> _data(data);
	if (data == NULL)
		return NULL;
	
	picture->Flatten(data);
	
	data->Seek(0, SEEK_SET);
	BPicture *archivedPicture = new BPicture();
	if (archivedPicture == NULL)
		return NULL;
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
	if (picture->Archive(&archive) != B_OK)
		return NULL;

	BPicture *archivedPicture = new BPicture(&archive);
	if (archivedPicture == NULL)
		return NULL;

	return archivedPicture;
}