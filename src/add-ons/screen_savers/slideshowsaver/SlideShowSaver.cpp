/*****************************************************************************/
// SlideShowSaver
// Written by Michael Wilber
// Slide show code derived from ShowImage code, written by Michael Pfeiffer
//
// SlideShowSaver.cpp
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


#include "SlideShowSaver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BitmapStream.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <List.h>
#include <Path.h>
#include <StringView.h>
#include <TranslatorRoster.h>

#include "SlideShowConfigView.h"


// Called by system to get the screen saver
extern "C" _EXPORT BScreenSaver *
instantiate_screen_saver(BMessage *message, image_id id)
{
	return new SlideShowSaver(message, id);
}

// returns B_ERROR if problems reading ref
// B_OK if ref is not a directory
// B_OK + 1 if ref is a directory
status_t
ent_is_dir(const entry_ref *ref)
{
	BEntry ent(ref);
	if (ent.InitCheck() != B_OK)
		return B_ERROR;

	struct stat st;
	if (ent.GetStat(&st) != B_OK)
		return B_ERROR;

	return S_ISDIR(st.st_mode) ? (B_OK + 1) : B_OK;
}

int CompareEntries(const void* a, const void* b)
{
	entry_ref *r1, *r2;
	r1 = *(entry_ref**)a;
	r2 = *(entry_ref**)b;
	return strcasecmp(r1->name, r2->name);
}

// Default settings for the Translator
LiveSetting gDefaultSettings[] = {
	LiveSetting(CHANGE_CAPTION, SAVER_SETTING_CAPTION, true),
		// Show image caption by default
	LiveSetting(CHANGE_BORDER, SAVER_SETTING_BORDER, true),
		// Show image border by default
	LiveSetting(CHANGE_DIRECTORY, SAVER_SETTING_DIRECTORY, "/boot/home"),
		// Set default image directory to home
	LiveSetting(CHANGE_DELAY, SAVER_SETTING_DELAY, (int32) 3000)
		// Default delay: 3 seconds
};

SlideShowSaver::SlideShowSaver(BMessage *archive, image_id image)
	:
	BScreenSaver(archive, image), fLock("SlideShow Lock")
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("SlideShowSaver");

	fNewDirectory = true;
	fBitmap = NULL;
	fShowBorder = true;
	fShowCaption = true;

	fSettings = new LiveSettings("SlideShowSaver_Settings",
		gDefaultSettings, sizeof(gDefaultSettings) / sizeof(LiveSetting));
	fSettings->LoadSettings();
		// load settings from the settings file

	fSettings->AddObserver(this);
}

SlideShowSaver::~SlideShowSaver()
{
	delete fBitmap;
	fBitmap = NULL;

	fSettings->RemoveObserver(this);
	fSettings->Release();
}

// Called by fSettings to notify that someone has changed
// a setting. For example, if the user changes a setting
// on the config panel, this will be called to notify this
// object.
void
SlideShowSaver::SettingChanged(uint32 setting)
{
	switch (setting) {
		case CHANGE_CAPTION:
			UpdateShowCaption();
			break;
		case CHANGE_BORDER:
			UpdateShowBorder();
			break;
		case CHANGE_DIRECTORY:
			UpdateDirectory();
			break;
		case CHANGE_DELAY:
			UpdateTickSize();
			break;

		default:
			break;
	}
}

status_t
SlideShowSaver::UpdateTickSize()
{
	// Tick size is in microseconds, but is stored in settings as
	// milliseconds
	bigtime_t ticks = static_cast<bigtime_t>
		(fSettings->SetGetInt32(SAVER_SETTING_DELAY)) * 1000;
	SetTickSize(ticks);

	return B_OK;
}

status_t
SlideShowSaver::UpdateShowCaption()
{
	fShowCaption = fSettings->SetGetBool(SAVER_SETTING_CAPTION);
	return B_OK;
}

status_t
SlideShowSaver::UpdateShowBorder()
{
	fShowBorder = fSettings->SetGetBool(SAVER_SETTING_BORDER);
	return B_OK;
}

status_t
SlideShowSaver::UpdateDirectory()
{
	status_t result = B_OK;

	fLock.Lock();

	BString strDirectory;
	fSettings->GetString(SAVER_SETTING_DIRECTORY, strDirectory);
	BDirectory dir(strDirectory.String());
	if (dir.InitCheck() != B_OK || dir.GetNextRef(&fCurrentRef) != B_OK)
		result = B_ERROR;
	// Use ShowNextImage to find which translatable image is
	// alphabetically first in the given directory, and load it
	if (result == B_OK && ShowNextImage(true, true) == false)
		result = B_ERROR;

	fNewDirectory = true;

	fLock.Unlock();

	return result;
}

void
SlideShowSaver::StartConfig(BView *view)
{
	view->AddChild(new SlideShowConfigView(
		BRect(10, 10, 250, 300), "SlideShowSaver Config",
		B_FOLLOW_ALL, B_WILL_DRAW, fSettings->Acquire()));
}

status_t
SlideShowSaver::StartSaver(BView *view, bool preview)
{
	UpdateShowCaption();
	UpdateShowBorder();

	if (UpdateDirectory() != B_OK)
		return B_ERROR;

	// Read ticksize setting and set it as the delay
	UpdateTickSize();

	return B_OK;
}

void
SlideShowSaver::Draw(BView *view, int32 frame)
{
	fLock.Lock();

	view->SetLowColor(0, 0, 0);
	view->SetHighColor(192, 192, 192);
	view->SetViewColor(192, 192, 192);

	bool bResult = false;
	if (fNewDirectory == true) {
		// Already have a bitmap on the first frame
		bResult = true;
	} else {
		bResult = ShowNextImage(true, false);
		// try rewinding to beginning
		if (bResult == false)
			bResult = ShowNextImage(true, true);
	}
	fNewDirectory = false;

	if (bResult == true && fBitmap != NULL) {
		BRect destRect(0, 0, fBitmap->Bounds().Width(), fBitmap->Bounds().Height()),
			vwBounds = view->Bounds();

		if (destRect.Width() < vwBounds.Width()) {
			destRect.OffsetBy((vwBounds.Width() - destRect.Width()) / 2, 0);
		}
		if (destRect.Height() < vwBounds.Height()) {
			destRect.OffsetBy(0, (vwBounds.Height() - destRect.Height()) / 2);
		}

		BRect border = destRect, bounds = view->Bounds();
		// top
		view->FillRect(BRect(0, 0, bounds.right, border.top-1), B_SOLID_LOW);
		// left
		view->FillRect(BRect(0, border.top, border.left-1, border.bottom), B_SOLID_LOW);
		// right
		view->FillRect(BRect(border.right+1, border.top, bounds.right, border.bottom), B_SOLID_LOW);
		// bottom
		view->FillRect(BRect(0, border.bottom+1, bounds.right, bounds.bottom), B_SOLID_LOW);

		if (fShowBorder == true) {
			BRect strokeRect = destRect;
			strokeRect.InsetBy(-1, -1);
			view->StrokeRect(strokeRect);
		}

		view->DrawBitmap(fBitmap, fBitmap->Bounds(), destRect);

		if (fShowCaption == true)
			DrawCaption(view);
	}

	fLock.Unlock();
}

status_t
SlideShowSaver::SetImage(const entry_ref *pref)
{
	entry_ref ref;
	if (!pref)
		ref = fCurrentRef;
	else
		ref = *pref;

	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return B_ERROR;

	if (ent_is_dir(pref) != B_OK)
		// if ref is erroneous or a directory, return error
		return B_ERROR;

	BFile file(&ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;
	//if (ref != fCurrentRef)
		// if new image, reset to first document
	//	fDocumentIndex = 1;
	if (ioExtension.AddInt32("/documentIndex", 1 /*fDocumentIndex*/) != B_OK)
		return B_ERROR;
	if (proster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;

	// Translate image data and create a new ShowImage window
	BBitmapStream outstream;
	if (proster->Translate(&file, &info, &ioExtension, &outstream,
		B_TRANSLATOR_BITMAP) != B_OK)
		return B_ERROR;
	BBitmap *newBitmap = NULL;
	if (outstream.DetachBitmap(&newBitmap) != B_OK)
		return B_ERROR;

	// Now that I've successfully loaded the new bitmap,
	// I can be sure it is safe to delete the old one,
	// and clear everything
	delete fBitmap;
	fBitmap = newBitmap;
	newBitmap = NULL;
	fCurrentRef = ref;

	// Get path to use in caption
	fCaption = "<< Unable to read the path >>";
	BEntry entry(&fCurrentRef);
	if (entry.InitCheck() == B_OK) {
		BPath path(&entry);
		if (path.InitCheck() == B_OK) {
			fCaption = path.Path();
		}
	}

	return B_OK;
}

// Function originally from Haiku ShowImage
bool
SlideShowSaver::ShowNextImage(bool next, bool rewind)
{
	bool found;
	entry_ref curRef, imgRef;

	curRef = fCurrentRef;
	found = FindNextImage(&curRef, &imgRef, next, rewind);
	if (found) {
		// Keep trying to load images until:
		// 1. The image loads successfully
		// 2. The last file in the directory is found (for find next or find first)
		// 3. The first file in the directory is found (for find prev)
		// 4. The call to FindNextImage fails for any other reason
		while (SetImage(&imgRef) != B_OK) {
			curRef = imgRef;
			found = FindNextImage(&curRef, &imgRef, next, false);
			if (!found)
				return false;
		}
		return true;
	}
	return false;
}

// Function taken from Haiku ShowImage,
// function originally written by Michael Pfeiffer
bool
SlideShowSaver::IsImage(const entry_ref *pref)
{
	if (!pref)
		return false;

	if (ent_is_dir(pref) != B_OK)
		// if ref is erroneous or a directory, return false
		return false;

	BFile file(pref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return false;

	BMessage ioExtension;
	if (ioExtension.AddInt32("/documentIndex", 1) != B_OK)
		return false;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	if (proster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return false;

	return true;
}

// Function taken from Haiku ShowImage,
// function originally written by Michael Pfeiffer
bool
SlideShowSaver::FindNextImage(entry_ref *in_current, entry_ref *out_image, bool next, bool rewind)
{
//	ASSERT(next || !rewind);
	BEntry curImage(in_current);
	entry_ref entry, *ref;
	BDirectory parent;
	BList entries;
	bool found = false;
	int32 cur;

	if (curImage.GetParent(&parent) != B_OK)
		return false;

	while (parent.GetNextRef(&entry) == B_OK) {
		if (entry != *in_current) {
			entries.AddItem(new entry_ref(entry));
		} else {
			// insert current ref, so we can find it easily after sorting
			entries.AddItem(in_current);
		}
	}

	entries.SortItems(CompareEntries);

	cur = entries.IndexOf(in_current);
//	ASSERT(cur >= 0);

	// remove it so FreeEntries() does not delete it
	entries.RemoveItem(in_current);

	if (next) {
		// find the next image in the list
		if (rewind) cur = 0; // start with first
		for (; (ref = (entry_ref*)entries.ItemAt(cur)) != NULL; cur ++) {
			if (IsImage(ref)) {
				found = true;
				*out_image = (const entry_ref)*ref;
				break;
			}
		}
	} else {
		// find the previous image in the list
		cur --;
		for (; cur >= 0; cur --) {
			ref = (entry_ref*)entries.ItemAt(cur);
			if (IsImage(ref)) {
				found = true;
				*out_image = (const entry_ref)*ref;
				break;
			}
		}
	}

	FreeEntries(&entries);
	return found;
}

// Function taken from Haiku ShowImage,
// function originally written by Michael Pfeiffer
void
SlideShowSaver::FreeEntries(BList *entries)
{
	const int32 n = entries->CountItems();
	for (int32 i = 0; i < n; i ++) {
		entry_ref *ref = (entry_ref *)entries->ItemAt(i);
		delete ref;
	}
	entries->MakeEmpty();
}

void
SlideShowSaver::LayoutCaption(BView *view, BFont &font, BPoint &pos, BRect &rect)
{
	font_height fontHeight;
	float width, height;
	BRect bounds(view->Bounds());
	font = be_plain_font;
	width = font.StringWidth(fCaption.String()) + 1; // 1 for text shadow
	font.GetHeight(&fontHeight);
	height = fontHeight.ascent + fontHeight.descent;
	// center text horizontally
	pos.x = (bounds.left + bounds.right - width)/2;
	// flush bottom
	pos.y = bounds.bottom - fontHeight.descent - 5;

	// background rectangle
	rect.Set(0, 0, (width-1)+2, (height-1)+2+1); // 2 for border and 1 for text shadow
	rect.OffsetTo(pos);
	rect.OffsetBy(-1, -1-fontHeight.ascent); // -1 for border
}

void
SlideShowSaver::DrawCaption(BView *view)
{
	BFont font;
	BPoint pos;
	BRect rect;
	LayoutCaption(view, font, pos, rect);

	view->PushState();
	// draw background
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 255, 128);
	view->FillRect(rect);
	// draw text
	view->SetDrawingMode(B_OP_OVER);
	view->SetFont(&font);
	view->SetLowColor(B_TRANSPARENT_COLOR);
	// text shadow
	pos += BPoint(1, 1);
	view->SetHighColor(0, 0, 0);
	view->SetPenSize(1);
	view->DrawString(fCaption.String(), pos);
	// text
	pos -= BPoint(1, 1);
	view->SetHighColor(255, 255, 0);
	view->DrawString(fCaption.String(), pos);
	view->PopState();
}


