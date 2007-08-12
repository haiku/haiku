/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _TEST_RESULT_ITEM_H
#define _TEST_RESULT_ITEM_H

#include <Bitmap.h>
#include <ListItem.h>
#include <String.h>
#include <View.h>

class TestResultItem : public BListItem {

public:
	TestResultItem(const char* name, BRect bitmapSize);
	virtual ~TestResultItem();
	
	void DrawItem(BView *owner, BRect itemRect, bool drawEverthing);
	void Update(BView *owner, const BFont *font);
	
	void SetOk(bool ok) { fOk = ok; }
	void SetErrorMessage(const char *errorMessage) { fErrorMessage = errorMessage; }
	void SetDirectBitmap(BBitmap *directBitmap) { fDirectBitmap = directBitmap; }
	void SetOriginalBitmap(BBitmap *originalBitmap) { fOriginalBitmap = originalBitmap; }
	void SetArchivedBitmap(BBitmap *archivedBitmap) { fArchivedBitmap = archivedBitmap; }
	
private:
	BString fName;
	BRect fBitmapSize;
	bool fOk;
	BString fErrorMessage;
	BBitmap *fDirectBitmap;
	BBitmap *fOriginalBitmap;
	BBitmap *fArchivedBitmap;
};

#endif
