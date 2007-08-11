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
	
	void Release() { fObject = NULL; }
	
private:
	T *fObject;
};


PictureTest::PictureTest()
	: fColorSpace(B_RGBA32)
	, fOriginalBitmap(NULL)
	, fArchivedBitmap(NULL)
{
}

BBitmap*
PictureTest::GetOriginalBitmap(bool detach)
{
	BBitmap* bitmap = fOriginalBitmap;
	if (detach)
		fOriginalBitmap = NULL;
	return bitmap;
}


BBitmap*
PictureTest::GetArchivedBitmap(bool detach)
{
	BBitmap* bitmap = fArchivedBitmap;
	if (detach)
		fArchivedBitmap = NULL;
	return bitmap;
}


PictureTest::~PictureTest() 
{
	CleanUp();
}

void
PictureTest::CleanUp()
{
	delete fOriginalBitmap;
	fOriginalBitmap = NULL;
	delete fArchivedBitmap;
	fArchivedBitmap = NULL;
	fErrorMessage = "";
}

bool
PictureTest::Test(draw_func* func, BRect frame)
{
	CleanUp();

	BPicture *picture = RecordPicture(func, frame);
	AutoDelete<BPicture> _picture(picture);
	TEST_AND_RETURN(picture == NULL, "Picture could not be recorded!", false);
		
	BPicture *archivedPicture = SaveAndRestore(picture);
	AutoDelete<BPicture> _archivedPicture(archivedPicture);
	TEST_AND_RETURN(picture == NULL, "Picture could not be flattened and unflattened!", false);
	
	fOriginalBitmap = CreateBitmap(picture, frame);
	TEST_AND_RETURN(fOriginalBitmap == NULL, "Could not create bitmap from original picture!", false);
		
	fArchivedBitmap = CreateBitmap(archivedPicture, frame);
	TEST_AND_RETURN(fArchivedBitmap == NULL, "Could not create bitmap from archived picture!", false);
	
	bool result = IsSame(fOriginalBitmap, fArchivedBitmap);
	TEST_AND_RETURN(result == false, "Bitmaps differ!", false);
	return result;
}


BPicture *
PictureTest::RecordPicture(draw_func* func, BRect frame)
{
	// create view for recording to picture
	BBitmap *bitmap = new BBitmap(frame, fColorSpace, true);
	AutoDelete<BBitmap> _bitmap(bitmap);
	if (bitmap == NULL || bitmap->IsValid() == false || bitmap->InitCheck() != B_OK)
		return NULL;
	
	BView *view = new BView(frame, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
	AutoDelete<BView> _view(view);
	if (view == NULL)
		return NULL;
	
	bitmap->Lock();
	bitmap->AddChild(view);
	
	// R5 clears the bitmap! However Haiku does not, so clear it for now.
	view->PushState();
	view->SetHighColor(255, 255, 255);
	view->FillRect(frame);
	view->PopState();
	
	// record
	BPicture *picture = new BPicture();	
	view->BeginPicture(picture);
	func(view, frame);
	picture = view->EndPicture();
	
	// destroy view
	view->Sync();
	view->RemoveSelf();
	bitmap->Unlock();
	
	return picture;
}

BPicture *
PictureTest::SaveAndRestore(BPicture *picture)
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

BBitmap *
PictureTest::CreateBitmap(BPicture *picture, BRect frame)
{
	// create bitmap that accepts a view
	BBitmap *bitmap = new BBitmap(frame, fColorSpace, true);
	AutoDelete<BBitmap> _bitmap(bitmap);
	if (bitmap == NULL || bitmap->IsValid() == false || bitmap->InitCheck() != B_OK)
		return NULL;
	
	BView *view = new BView(frame, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
	AutoDelete<BView> _view(view);
	if (view == NULL)
		return NULL;

	// the result bitmap that does not accept views	
	// to save resources in the application server
	BBitmap *result = new BBitmap(frame, fColorSpace, false);
	if (result == NULL || result->IsValid() == false || result->InitCheck() != B_OK)
		return NULL;

	bitmap->Lock();
	bitmap->AddChild(view);

	view->DrawPicture(picture);
	view->Sync();

	// destroy view
	view->RemoveSelf();
	bitmap->Unlock();

	memcpy(result->Bits(), bitmap->Bits(), bitmap->BitsLength());
	
	return result;
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
