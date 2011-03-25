/*****************************************************************************/
// SlideShowSaver
// Written by Michael Wilber
// Slide show code derived from ShowImage code, written by Michael Pfeiffer
//
// SlideShowSaver.h
//
//
// Copyright (C) Haiku
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


#ifndef SLIDE_SHOW_SAVER_H
#define SLIDE_SHOW_SAVER_H

#include <Locker.h>
#include <ScreenSaver.h>
#include <Bitmap.h>
#include <String.h>
#include <Entry.h>
#include "LiveSettings.h"

class SlideShowSaver :
	public BScreenSaver,
	public LiveSettingsObserver {
public:
	SlideShowSaver(BMessage *archive, image_id image);
	virtual ~SlideShowSaver(void);
	
	virtual void SettingChanged(uint32 setting);

	virtual void StartConfig(BView *view);
	virtual status_t StartSaver(BView *view, bool preview);
	virtual void Draw(BView *view, int32 frame);
	
protected:
	// reload ticksize setting and apply it
	status_t UpdateTickSize();
	
	status_t UpdateShowCaption();
	status_t UpdateShowBorder();
	// reload directory setting and apply it
	status_t UpdateDirectory();
	
	// Function taken from Haiku ShowImage,
	// function originally written by Michael Pfeiffer
	status_t SetImage(const entry_ref *pref);

	// Function originally from Haiku ShowImage
	bool ShowNextImage(bool next, bool rewind);

	// Function taken from Haiku ShowImage,
	// function originally written by Michael Pfeiffer
	bool IsImage(const entry_ref *pref);

	// Function taken from Haiku ShowImage,
	// function originally written by Michael Pfeiffer
	bool FindNextImage(entry_ref *in_current, entry_ref *out_image, bool next, bool rewind);

	// Function taken from Haiku ShowImage,
	// function originally written by Michael Pfeiffer
	void FreeEntries(BList *entries);
	
	void LayoutCaption(BView *view, BFont &font, BPoint &pos, BRect &rect);	
	void DrawCaption(BView *view);
	// void UpdateCaption(BView *view);

private:
	BLocker fLock;
	bool fNewDirectory;
	LiveSettings *fSettings;
	BBitmap *fBitmap;
	BString fCaption;
	entry_ref fCurrentRef;
	bool fShowCaption;
	bool fShowBorder;
};

#endif // #ifndef SLIDE_SHOW_SAVER_H

