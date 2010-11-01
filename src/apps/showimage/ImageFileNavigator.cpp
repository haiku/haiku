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


static bool
entry_ref_is_file(const entry_ref& ref)
{
	BEntry entry(&ref, true);
	if (entry.InitCheck() != B_OK)
		return false;

	return entry.IsFile();
}


// #pragma mark -


ImageFileNavigator::ImageFileNavigator(const BMessenger& target)
	:
	fTarget(target),
	fProgressWindow(NULL),
	fDocumentIndex(1),
	fDocumentCount(1)
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


void
ImageFileNavigator::SetProgressWindow(ProgressWindow* progressWindow)
{
	fProgressWindow = progressWindow;
}


status_t
ImageFileNavigator::LoadImage(const entry_ref& ref, int32 page)
{
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return B_ERROR;

	if (!entry_ref_is_file(ref))
		return B_ERROR;

	BFile file(&ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;

	if (page != 0 && ioExtension.AddInt32("/documentIndex", page) != B_OK)
		return B_ERROR;

	if (fProgressWindow != NULL) {
		BMessage progress(kMsgProgressStatusUpdate);
		if (ioExtension.AddMessenger("/progressMonitor",
				fProgressWindow) == B_OK
			&& ioExtension.AddMessage("/progressMessage", &progress) == B_OK)
			fProgressWindow->Start();
	}

	// Translate image data and create a new ShowImage window

	BBitmapStream outstream;

	status_t status = roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP);
	if (status == B_OK) {
		status = roster->Translate(&file, &info, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP);
	}

	if (fProgressWindow != NULL)
		fProgressWindow->Stop();

	if (status != B_OK)
		return status;

	BBitmap* bitmap;
	if (outstream.DetachBitmap(&bitmap) != B_OK)
		return B_ERROR;

	fCurrentRef = ref;
	fDocumentIndex = page;

	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK
		&& documentCount > 0)
		fDocumentCount = documentCount;
	else
		fDocumentCount = 1;

	BMessage message(kMsgImageLoaded);
	message.AddString("type", info.name);
	message.AddString("mime", info.MIME);
	message.AddRef("ref", &ref);
	message.AddInt32("page", page);
	message.AddPointer("bitmap", (void*)bitmap);
	status = fTarget.SendMessage(&message);
	if (status != B_OK) {
		delete bitmap;
		return status;
	}

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


bool
ImageFileNavigator::FirstPage()
{
	if (fDocumentIndex != 1) {
		LoadImage(fCurrentRef, 1);
		return true;
	}
	return false;
}


bool
ImageFileNavigator::LastPage()
{
	if (fDocumentIndex != fDocumentCount) {
		LoadImage(fCurrentRef, fDocumentCount);
		return true;
	}
	return false;
}


bool
ImageFileNavigator::NextPage()
{
	if (fDocumentIndex < fDocumentCount) {
		LoadImage(fCurrentRef, ++fDocumentIndex);
		return true;
	}
	return false;
}


bool
ImageFileNavigator::PreviousPage()
{
	if (fDocumentIndex > 1) {
		LoadImage(fCurrentRef, --fDocumentIndex);
		return true;
	}
	return false;
}


bool
ImageFileNavigator::GoToPage(int32 page)
{
	if (page > 0 && page <= fDocumentCount && page != fDocumentIndex) {
		fDocumentIndex = page;
		LoadImage(fCurrentRef, fDocumentIndex);
		return true;
	}
	return false;
}


void
ImageFileNavigator::FirstFile()
{
	_LoadNextImage(true, true);
}


void
ImageFileNavigator::NextFile()
{
	_LoadNextImage(true, false);
}


void
ImageFileNavigator::PreviousFile()
{
	_LoadNextImage(false, false);
}


bool
ImageFileNavigator::HasNextFile()
{
	entry_ref ref;
	return _FindNextImage(fCurrentRef, ref, true, false);
}


bool
ImageFileNavigator::HasPreviousFile()
{
	entry_ref ref;
	return _FindNextImage(fCurrentRef, ref, false, false);
}


void
ImageFileNavigator::DeleteFile()
{
	// Move image to Trash
	BMessage trash(BPrivate::kMoveToTrash);
	trash.AddRef("refs", &fCurrentRef);

// TODO!
#if 0
	// We create our own messenger because the member fTrackerMessenger
	// could be invalid
	BMessenger tracker(kTrackerSignature);
	if (tracker.SendMessage(&trash) == B_OK && !NextFile()) {
		// This is the last (or only file) in this directory,
		// close the window
		_SendMessageToWindow(B_QUIT_REQUESTED);
	}
#endif
}


// #pragma mark -


bool
ImageFileNavigator::_IsImage(const entry_ref& ref)
{
	if (!entry_ref_is_file(ref))
		return false;

	BFile file(&ref, B_READ_ONLY);
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
ImageFileNavigator::_FindNextImage(const entry_ref& currentRef, entry_ref& ref,
	bool next, bool rewind)
{
	// Based on GetTrackerWindowFile function from BeMail
	if (!fTrackerMessenger.IsValid())
		return false;

	// Ask the Tracker what the next/prev file in the window is.
	// Continue asking for the next reference until a valid
	// image is found.
	entry_ref nextRef = currentRef;
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

		if (_IsImage(nextRef))
			foundRef = true;

		rewind = false;
			// stop asking for the first ref in the directory
	}

	ref = nextRef;
	return foundRef;
}


status_t
ImageFileNavigator::_LoadNextImage(bool next, bool rewind)
{
	entry_ref currentRef = fCurrentRef;
	entry_ref ref;
	bool found = _FindNextImage(currentRef, ref, next, rewind);
	if (found) {
		// Keep trying to load images until:
		// 1. The image loads successfully
		// 2. The last file in the directory is found (for find next or find
		//    first)
		// 3. The first file in the directory is found (for find prev)
		// 4. The call to _FindNextImage fails for any other reason
		while (LoadImage(ref) != B_OK) {
			currentRef = ref;
			found = _FindNextImage(currentRef, ref, next, false);
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
	BMessage setSelection(B_SET_PROPERTY);
	setSelection.AddSpecifier("Selection");
	setSelection.AddRef("data", &fCurrentRef);
	fTrackerMessenger.SendMessage(&setSelection);
}


