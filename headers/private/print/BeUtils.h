/*****************************************************************************/
// BeUtils.h
//
// Version: 1.0.0d1
//
// Several utilities for writing applications for the BeOS. It are small
// very specific functions, but generally useful (could be here because of a
// lack in the APIs, or just sheer lazyness :))
//
// Authors
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001, 2002 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _BE_UTILS_H
#define _BE_UTILS_H

#include <FindDirectory.h>
#include <Path.h>
#include <SupportDefs.h>
#include <Picture.h>
#include <PictureButton.h>
#include <TranslatorFormats.h>
#include <TranslationUtils.h>

status_t TestForAddonExistence(const char* name, directory_which which,
							   const char* section, BPath& outPath);

// Reference counted object
class Object {
private:
	int32 fRefCount;

public:
	// After construction reference count is 1
	Object() : fRefCount(1) { }
	// dtor should be private, but ie. ObjectList requires a public dtor!
	virtual ~Object() { };

	// thread-safe as long as thread that calls Acquire has already
	// a reference to the object
	void Acquire() {
		atomic_add(&fRefCount, 1);
	}

	bool Release() {
		if (atomic_add(&fRefCount, -1) == 1) {
			delete this; return true;
		} else {
			return false;
		}
	}
};

// Automatically send a reply to sender on destruction of object
// and delete sender
class AutoReply {
	BMessage* fSender;
	BMessage  fReply;
	
public:
	AutoReply(BMessage* sender, uint32 what);
	~AutoReply();
	void SetReply(BMessage* m) { fReply = *m; }
};

// mimetype from sender
bool MimeTypeForSender(BMessage* sender, BString& mime);
// load bitmap from application resources
BBitmap* LoadBitmap(const char* name, uint32 type_code = B_TRANSLATOR_BITMAP);
// convert bitmap to picture; view must be attached to a window!
// returns NULL if bitmap is NULL
BPicture *BitmapToPicture(BView* view, BBitmap *bitmap);
BPicture *BitmapToGrayedPicture(BView* view, BBitmap *bitmap);

#endif
