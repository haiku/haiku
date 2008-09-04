/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _PICTURE_TEST_H
#define _PICTURE_TEST_H

#include <InterfaceKit.h>
#include <String.h>

typedef void (draw_func)(BView *view, BRect frame);

class PictureTest {

public:
	PictureTest();
	virtual ~PictureTest();
	
	void SetColorSpace(color_space colorSpace) { fColorSpace = colorSpace; }

	bool Test(draw_func* func, BRect frame);
	
	const char *ErrorMessage() const { return fErrorMessage.String(); }

	BBitmap *DirectBitmap(bool detach = false);
	BBitmap *BitmapFromPicture(bool detach = false);
	BBitmap *BitmapFromRestoredPicture(bool detach = false);

protected:
	virtual BPicture *SaveAndRestore(BPicture *picture) = 0;
	void SetErrorMessage(const char* message);

private:
	
	void CleanUp();
	
	BPicture *RecordPicture(draw_func* func, BRect frame);

	BBitmap *CreateBitmap(draw_func* func, BRect frame);
	BBitmap *CreateBitmap(BPicture *picture, BRect frame);

	bool IsSame(BBitmap *bitmap1, BBitmap *bitmap2, BString &reason);

	color_space fColorSpace;

	BBitmap *fDirectBitmap;
	BBitmap *fBitmapFromPicture;
	BBitmap *fBitmapFromRestoredPicture;
	
	BString fErrorMessage;
};

class FlattenPictureTest : public PictureTest
{
public:
	FlattenPictureTest();

protected:
	BPicture *SaveAndRestore(BPicture *picture);
};

class ArchivePictureTest : public PictureTest
{
public:
	ArchivePictureTest();

protected:
	BPicture *SaveAndRestore(BPicture *picture);
};

#endif
