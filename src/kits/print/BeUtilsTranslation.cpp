/*****************************************************************************/
// BeUtilsTranslation.cpp
//
// Version: 1.0.0d1
//
// Several utilities for writing applications for the BeOS. It are small
// very specific functions, but generally useful (could be here because of a
// lack in the APIs, or just sheer lazyness :))
//
//
// Author
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

#include <Application.h>
#include <Bitmap.h>
#include <Messenger.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

#include "BeUtils.h"

BBitmap* LoadBitmap(const char* name, uint32 type_code) {
	if (type_code == B_TRANSLATOR_BITMAP) {
		return BTranslationUtils::GetBitmap(type_code, name);
	} else {
		BResources *res = BApplication::AppResources();
		if (res != NULL) {
			BMessage m;
			size_t length;
			const void *bits = res->LoadResource(type_code, name, &length);
			if (bits && m.Unflatten((char*)bits) == B_OK) {
				return (BBitmap*)BBitmap::Instantiate(&m);
			}
		}
		return NULL;
	}
}

BPicture *BitmapToPicture(BView* view, BBitmap *bitmap) {
	if (bitmap) {
		view->BeginPicture(new BPicture());
		view->DrawBitmap(bitmap);
		return view->EndPicture();
	}
	return NULL;
}

BPicture *BitmapToGrayedPicture(BView* view, BBitmap *bitmap) {
	if (bitmap) {
		BRect rect(bitmap->Bounds());
		view->BeginPicture(new BPicture());
		view->DrawBitmap(bitmap);
		view->SetHighColor(255, 255, 255, 128);
		view->SetDrawingMode(B_OP_ALPHA);
		view->FillRect(rect);
		return view->EndPicture();
	}
	return NULL;
}
