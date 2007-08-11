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

	BBitmap *GetOriginalBitmap(bool detach = false);
	BBitmap *GetArchivedBitmap(bool detach = false);

private:
	
	void CleanUp();
	
	BPicture *RecordPicture(draw_func* func, BRect frame);
	
	BPicture *SaveAndRestore(BPicture *picture);
	
	BBitmap *CreateBitmap(BPicture *picture, BRect frame);

	bool IsSame(BBitmap *bitmap1, BBitmap *bitmap2);

	color_space fColorSpace;

	BBitmap *fOriginalBitmap;
	BBitmap *fArchivedBitmap;
	
	BString fErrorMessage;
};



#endif
