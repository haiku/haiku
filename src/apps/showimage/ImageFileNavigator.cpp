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
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ImageFileNavigator.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <ObjectList.h>
#include <Path.h>
#include <TranslatorRoster.h>

#include <tracker_private.h>

#include "ProgressWindow.h"
#include "ShowImageConstants.h"


class Navigator {
public:
								Navigator();
	virtual						~Navigator();

	virtual	bool				FindNextImage(const entry_ref& currentRef,
									entry_ref& ref, bool next, bool rewind) = 0;
	virtual void				UpdateSelection(const entry_ref& ref) = 0;

protected:
			bool				IsImage(const entry_ref& ref);
};


// Navigation to the next/previous image file is based on
// communication with Tracker, the folder containing the current
// image needs to be open for this to work. The routine first tries
// to find the next candidate file, then tries to load it as image.
// As long as loading fails, the operation is repeated for the next
// candidate file.

class TrackerNavigator : public Navigator {
public:
								TrackerNavigator(
									const BMessenger& trackerMessenger);
	virtual						~TrackerNavigator();

	virtual	bool				FindNextImage(const entry_ref& currentRef,
									entry_ref& ref, bool next, bool rewind);
	virtual void				UpdateSelection(const entry_ref& ref);

private:
			BMessenger			fTrackerMessenger;
				// of the window that this was launched from
};


class FolderNavigator : public Navigator {
public:
								FolderNavigator(const entry_ref& ref);
	virtual						~FolderNavigator();

	virtual	bool				FindNextImage(const entry_ref& currentRef,
									entry_ref& ref, bool next, bool rewind);
	virtual void				UpdateSelection(const entry_ref& ref);

private:
			void				_BuildEntryList();
	static	int					_CompareRefs(const entry_ref* refA,
									const entry_ref* refB);

private:
			BDirectory			fFolder;
			BObjectList<entry_ref> fEntries;
};


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


Navigator::Navigator()
{
}


Navigator::~Navigator()
{
}


bool
Navigator::IsImage(const entry_ref& ref)
{
	if (!entry_ref_is_file(ref))
		return false;

	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return false;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	return roster->Identify(&file, NULL, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) == B_OK;
}


// #pragma mark -


TrackerNavigator::TrackerNavigator(const BMessenger& trackerMessenger)
	:
	fTrackerMessenger(trackerMessenger)
{
}


TrackerNavigator::~TrackerNavigator()
{
}


bool
TrackerNavigator::FindNextImage(const entry_ref& currentRef, entry_ref& ref,
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
		BMessage specifier;
		if (rewind)
			specifier.what = B_DIRECT_SPECIFIER;
		else if (next)
			specifier.what = 'snxt';
		else
			specifier.what = 'sprv';
		specifier.AddString("property", "Entry");
		if (rewind)
			// if rewinding, ask for the ref to the
			// first item in the directory
			specifier.AddInt32("data", 0);
		else
			specifier.AddRef("data", &nextRef);
		request.AddSpecifier(&specifier);

		BMessage reply;
		if (fTrackerMessenger.SendMessage(&request, &reply) != B_OK)
			return false;
		if (reply.FindRef("result", &nextRef) != B_OK)
			return false;

		if (IsImage(nextRef))
			foundRef = true;

		rewind = false;
			// stop asking for the first ref in the directory
	}

	ref = nextRef;
	return foundRef;
}


void
TrackerNavigator::UpdateSelection(const entry_ref& ref)
{
	BMessage setSelection(B_SET_PROPERTY);
	setSelection.AddSpecifier("Selection");
	setSelection.AddRef("data", &ref);
	fTrackerMessenger.SendMessage(&setSelection);
}


// #pragma mark -


FolderNavigator::FolderNavigator(const entry_ref& ref)
	:
	fEntries(true)
{
	node_ref nodeRef;
	nodeRef.device = ref.device;
	nodeRef.node = ref.directory;

	fFolder.SetTo(&nodeRef);
	_BuildEntryList();

	// TODO: monitor the directory for changes, sort it naturally
}


FolderNavigator::~FolderNavigator()
{
}


bool
FolderNavigator::FindNextImage(const entry_ref& currentRef, entry_ref& nextRef,
	bool next, bool rewind)
{
	int32 index;
	if (rewind) {
		index = next ? fEntries.CountItems() : 0;
		next = !next;
	} else {
		index = fEntries.BinarySearchIndex(currentRef,
			&FolderNavigator::_CompareRefs);
		if (next)
			index++;
		else
			index--;
	}

	while (index < fEntries.CountItems() && index >= 0) {
		const entry_ref& ref = *fEntries.ItemAt(index);
		if (IsImage(ref)) {
			nextRef = ref;
			return true;
		} else {
			// remove non-image entries
			delete fEntries.RemoveItemAt(index);
			if (!next)
				index--;
		}
	}

	return false;
}


void
FolderNavigator::UpdateSelection(const entry_ref& ref)
{
	// nothing to do for us here
}


void
FolderNavigator::_BuildEntryList()
{
	fEntries.MakeEmpty();
	fFolder.Rewind();

	while (true) {
		entry_ref* ref = new entry_ref();
		status_t status = fFolder.GetNextRef(ref);
		if (status != B_OK)
			break;

		fEntries.AddItem(ref);
	}

	fEntries.SortItems(&FolderNavigator::_CompareRefs);
}


/*static*/ int
FolderNavigator::_CompareRefs(const entry_ref* refA, const entry_ref* refB)
{
	// TODO: natural sorting? Collating via current locale?
	return strcasecmp(refA->name, refB->name);
}


// #pragma mark -


ImageFileNavigator::ImageFileNavigator(const BMessenger& target,
	const entry_ref& ref, const BMessenger& trackerMessenger)
	:
	fTarget(target),
	fProgressWindow(NULL),
	fDocumentIndex(1),
	fDocumentCount(1)
{
	if (trackerMessenger.IsValid())
		fNavigator = new TrackerNavigator(trackerMessenger);
	else
		fNavigator = new FolderNavigator(ref);
}


ImageFileNavigator::~ImageFileNavigator()
{
	delete fNavigator;
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

	return B_OK;
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
	return fNavigator->FindNextImage(fCurrentRef, ref, true, false);
}


bool
ImageFileNavigator::HasPreviousFile()
{
	entry_ref ref;
	return fNavigator->FindNextImage(fCurrentRef, ref, false, false);
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


status_t
ImageFileNavigator::_LoadNextImage(bool next, bool rewind)
{
	entry_ref currentRef = fCurrentRef;
	entry_ref ref;
	if (fNavigator->FindNextImage(currentRef, ref, next, rewind)) {
		// Keep trying to load images until:
		// 1. The image loads successfully
		// 2. The last file in the directory is found (for find next or find
		//    first)
		// 3. The first file in the directory is found (for find prev)
		// 4. The call to _FindNextImage fails for any other reason
		while (LoadImage(ref) != B_OK) {
			currentRef = ref;
			if (!fNavigator->FindNextImage(currentRef, ref, next, false))
				return B_ENTRY_NOT_FOUND;
		}
		fNavigator->UpdateSelection(fCurrentRef);
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


