/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 *		yellowTAB GmbH
 *		Bernd Korz
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ImageFileNavigator.h"

#include <math.h>
#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <SupportDefs.h>
#include <TranslatorRoster.h>

#include <tracker_private.h>

#include "ProgressWindow.h"
#include "ShowImageApp.h"
#include "ShowImageConstants.h"
#include "ShowImageWindow.h"


// TODO: Remove this and use Tracker's Command.h once it is moved into the
// private headers!
namespace BPrivate {
	const uint32 kMoveToTrash = 'Ttrs';
}

using std::nothrow;


#define SHOW_IMAGE_ORIENTATION_ATTRIBUTE "ShowImage:orientation"

enum ImageFileNavigator::image_orientation
ImageFileNavigator::fTransformation[ImageProcessor::kNumberOfAffineTransformations][kNumberOfOrientations] = {
	// rotate 90°
	{k90, k180, k270, k0, k270V, k0V, k90V, k0H},
	// rotate -90°
	{k270, k0, k90, k180, k90V, k0H, k270V, k0V},
	// mirror vertical
	{k0H, k270V, k0V, k90V, k180, k270, k0, k90},
	// mirror horizontal
	{k0V, k90V, k0H, k270V, k0, k90, k180, k270}
};

static bool
entry_ref_is_file(const entry_ref *ref)
{
	BEntry entry(ref, true);
	if (entry.InitCheck() != B_OK)
		return false;

	return entry.IsFile();
}


ImageFileNavigator::ImageFileNavigator(BRect rect, const char *name, uint32 resizingMode,
		uint32 flags)
	:
	fDocumentIndex(1),
	fDocumentCount(1),
{
}


ImageFileNavigator::~ImageFileNavigator()
{
}


void
ImageFileNavigator::SetTrackerMessenger(const BMessenger& trackerMessenger)
{
	fTrackerMessenger = trackerMessenger;
}


status_t
ImageFileNavigator::LoadImage(const entry_ref* ref, BBitmap** bitmap)
{
	if (bitmap == NULL)
		return B_BAD_VALUE;

	// If no ref was specified, load the specified page of the current file.
	if (ref == NULL)
		ref = &fCurrentRef;

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return B_ERROR;

	if (!entry_ref_is_file(ref))
		return B_ERROR;

	BFile file(ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;
	if (ref != &fCurrentRef) {
		// if new image, reset to first document
		fDocumentIndex = 1;
	}

	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return B_ERROR;

	BMessage progress(kMsgProgressStatusUpdate);
	if (ioExtension.AddMessenger("/progressMonitor", fProgressWindow) == B_OK
		&& ioExtension.AddMessage("/progressMessage", &progress) == B_OK)
		fProgressWindow->Start();

	// Translate image data and create a new ShowImage window

	BBitmapStream outstream;

	status_t status = roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP);
	if (status == B_OK) {
		status = roster->Translate(&file, &info, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP);
	}

	fProgressWindow->Stop();

	if (status != B_OK)
		return status;

	if (outstream.DetachBitmap(bitmap) != B_OK)
		return B_ERROR;

	fCurrentRef = *ref;

	// restore orientation
	int32 orientation;
	fImageOrientation = k0;
	fInverted = false;
	if (file.ReadAttr(SHOW_IMAGE_ORIENTATION_ATTRIBUTE, B_INT32_TYPE, 0,
			&orientation, sizeof(orientation)) == sizeof(orientation)) {
		fInverted = (orientation & 256) != 0;
		fImageOrientation = (orientation & 255) != 0;
	}

	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK
		&& documentCount > 0)
		fDocumentCount = documentCount;
	else
		fDocumentCount = 1;

	fImageType = info.name;
	fImageMime = info.MIME;

	be_roster->AddToRecentDocuments(&fCurrentRef, kApplicationSignature);

	return B_OK;
}


void
ImageFileNavigator::GetName(BString* outName)
{
	BEntry entry(&fCurrentRef);
	char name[B_FILE_NAME_LENGTH];
	if (entry.InitCheck() < B_OK || entry.GetName(name) < B_OK)
		outName->SetTo("");
	else
		outName->SetTo(name);
}


void
ImageFileNavigator::GetPath(BString* outPath)
{
	BEntry entry(&fCurrentRef);
	BPath path;
	if (entry.InitCheck() < B_OK || entry.GetPath(&path) < B_OK)
		outPath->SetTo("");
	else
		outPath->SetTo(path.Path());
}


int32
ImageFileNavigator::CurrentPage()
{
	return fDocumentIndex;
}


int32
ImageFileNavigator::PageCount()
{
	return fDocumentCount;
}


status_t
ImageFileNavigator::FirstPage(BBitmap** bitmap)
{
	if (fDocumentIndex != 1) {
		fDocumentIndex = 1;
		return LoadImage(NULL, bitmap);
	}
	return B_BAD_INDEX;
}


status_t
ImageFileNavigator::LastPage(BBitmap** bitmap)
{
	if (fDocumentIndex != fDocumentCount) {
		fDocumentIndex = fDocumentCount;
		return LoadImage(NULL, bitmap);
	}
	return B_BAD_INDEX;
}


status_t
ImageFileNavigator::NextPage(BBitmap** bitmap)
{
	if (fDocumentIndex < fDocumentCount) {
		fDocumentIndex++;
		return LoadImage(NULL, bitmap);
	}
	return B_BAD_INDEX;
}


status_t
ImageFileNavigator::PrevPage(BBitmap** bitmap)
{
	if (fDocumentIndex > 1) {
		fDocumentIndex--;
		return LoadImage(NULL, bitmap);
	}
	return B_BAD_INDEX;
}


status_t
ImageFileNavigator::GoToPage(int32 page, BBitmap** bitmap)
{
	if (page > 0 && page <= fDocumentCount && page != fDocumentIndex) {
		fDocumentIndex = page;
		return LoadImage(NULL, bitmap);
	}
	return B_BAD_INDEX;
}


status_t
ImageFileNavigator::FirstFile(BBitmap** bitmap)
{
	return _LoadNextImage(true, true, bitmap);
}


status_t
ImageFileNavigator::NextFile(BBitmap** bitmap)
{
	return _LoadNextImage(true, false, bitmap);
}


status_t
ImageFileNavigator::PrevFile(BBitmap** bitmap)
{
	return _LoadNextImage(false, false, bitmap);
}


bool
ImageFileNavigator::HasNextFile()
{
	entry_ref ref;
	return _FindNextImage(&fCurrentRef, &ref, true, false);
}


bool
ImageFileNavigator::HasPrevFile()
{
	entry_ref ref;
	return _FindNextImage(&fCurrentRef, &ref, false, false);
}


// #pragma mark -


bool
ImageFileNavigator::_IsImage(const entry_ref *ref)
{
	if (ref == NULL || !entry_ref_is_file(ref))
		return false;

	BFile file(ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BTranslatorRoster *roster = BTranslatorRoster::Default();
	if (!roster)
		return false;

	BMessage ioExtension;
	if (ioExtension.AddInt32("/documentIndex", fDocumentIndex) != B_OK)
		return false;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	if (roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return false;

	return true;
}


bool
ImageFileNavigator::_FindNextImage(entry_ref* currentRef, entry_ref* ref,
	bool next, bool rewind)
{
	// Based on GetTrackerWindowFile function from BeMail
	if (!fTrackerMessenger.IsValid())
		return false;

	//
	//	Ask the Tracker what the next/prev file in the window is.
	//	Continue asking for the next reference until a valid
	//	image is found.
	//
	entry_ref nextRef = *currentRef;
	bool foundRef = false;
	while (!foundRef) {
		BMessage request(B_GET_PROPERTY);
		BMessage spc;
		if (rewind)
			spc.what = B_DIRECT_SPECIFIER;
		else if (next)
			spc.what = 'snxt';
		else
			spc.what = 'sprv';
		spc.AddString("property", "Entry");
		if (rewind)
			// if rewinding, ask for the ref to the
			// first item in the directory
			spc.AddInt32("data", 0);
		else
			spc.AddRef("data", &nextRef);
		request.AddSpecifier(&spc);

		BMessage reply;
		if (fTrackerMessenger.SendMessage(&request, &reply) != B_OK)
			return false;
		if (reply.FindRef("result", &nextRef) != B_OK)
			return false;

		if (_IsImage(&nextRef))
			foundRef = true;

		rewind = false;
			// stop asking for the first ref in the directory
	}

	*ref = nextRef;
	return foundRef;
}

status_t
ImageFileNavigator::_LoadNextImage(bool next, bool rewind, BBitmap** bitmap)
{
	if (bitmap == NULL)
		return B_BAD_VALUE;

	entry_ref curRef = fCurrentRef;
	entry_ref imgRef;
	bool found = _FindNextImage(&curRef, &imgRef, next, rewind);
	if (found) {
		// Keep trying to load images until:
		// 1. The image loads successfully
		// 2. The last file in the directory is found (for find next or find first)
		// 3. The first file in the directory is found (for find prev)
		// 4. The call to _FindNextImage fails for any other reason
		while (LoadImage(&imgRef, bitmap) != B_OK) {
			curRef = imgRef;
			found = _FindNextImage(&curRef, &imgRef, next, false);
			if (!found)
				return B_ENTRY_NOT_FOUND;
		}
		_SetTrackerSelectionToCurrent();
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


void
ImageFileNavigator::_SetTrackerSelectionToCurrent()
{
	BMessage setsel(B_SET_PROPERTY);
	setsel.AddSpecifier("Selection");
	setsel.AddRef("data", &fCurrentRef);
	fTrackerMessenger.SendMessage(&setsel);
}


