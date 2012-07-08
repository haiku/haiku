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

#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <NaturalCompare.h>
#include <ObjectList.h>
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

			bool				IsValid();

private:
			BMessenger			fTrackerMessenger;
				// of the window that this was launched from
};


class FolderNavigator : public Navigator {
public:
								FolderNavigator(entry_ref& ref);
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


// This class handles the case of the user closing the Tracker window after
// opening ShowImage from that window.
class AutoAdjustingNavigator : public Navigator {
public:
								AutoAdjustingNavigator(entry_ref& ref,
									const BMessenger& trackerMessenger);
	virtual						~AutoAdjustingNavigator();

	virtual	bool				FindNextImage(const entry_ref& currentRef,
									entry_ref& ref, bool next, bool rewind);
	virtual void				UpdateSelection(const entry_ref& ref);

private:
			bool				_CheckForTracker(const entry_ref& ref);

			TrackerNavigator*	fTrackerNavigator;
			FolderNavigator*	fFolderNavigator;
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
		if (rewind) {
			// if rewinding, ask for the ref to the
			// first item in the directory
			specifier.AddInt32("data", 0);
		} else
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


bool
TrackerNavigator::IsValid()
{
	return fTrackerMessenger.IsValid();
}


// #pragma mark -


FolderNavigator::FolderNavigator(entry_ref& ref)
	:
	fEntries(true)
{
	BEntry entry(&ref);
	if (entry.IsDirectory())
		fFolder.SetTo(&ref);
	else {
		node_ref nodeRef;
		nodeRef.device = ref.device;
		nodeRef.node = ref.directory;

		fFolder.SetTo(&nodeRef);
	}

	_BuildEntryList();

	// TODO: monitor the directory for changes, sort it naturally

	if (entry.IsDirectory())
		FindNextImage(ref, ref, false, true);
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
		if (status != B_OK) {
			delete ref;
			break;
		}

		fEntries.AddItem(ref);
	}

	fEntries.SortItems(&FolderNavigator::_CompareRefs);
}


/*static*/ int
FolderNavigator::_CompareRefs(const entry_ref* refA, const entry_ref* refB)
{
	return BPrivate::NaturalCompare(refA->name, refB->name);
}


// #pragma mark -


AutoAdjustingNavigator::AutoAdjustingNavigator(entry_ref& ref,
	const BMessenger& trackerMessenger)
	:
	fTrackerNavigator(NULL),
	fFolderNavigator(NULL)
{
	// TODO: allow selecting a folder from Tracker as well!
	if (trackerMessenger.IsValid())
		fTrackerNavigator = new TrackerNavigator(trackerMessenger);
	else
		fFolderNavigator = new FolderNavigator(ref);
}


AutoAdjustingNavigator::~AutoAdjustingNavigator()
{
	delete fTrackerNavigator;
	delete fFolderNavigator;
}


bool
AutoAdjustingNavigator::FindNextImage(const entry_ref& currentRef,
	entry_ref& nextRef,	bool next, bool rewind)
{
	if (_CheckForTracker(currentRef))
		return fTrackerNavigator->FindNextImage(currentRef, nextRef, next,
			rewind);

	if (fFolderNavigator != NULL)
		return fFolderNavigator->FindNextImage(currentRef, nextRef, next,
			rewind);

	return false;
}


void
AutoAdjustingNavigator::UpdateSelection(const entry_ref& ref)
{
	if (_CheckForTracker(ref)) {
		fTrackerNavigator->UpdateSelection(ref);
		return;
	}

	if (fFolderNavigator != NULL)
		fFolderNavigator->UpdateSelection(ref);
}


bool
AutoAdjustingNavigator::_CheckForTracker(const entry_ref& ref)
{
	if (fTrackerNavigator != NULL) {
		if (fTrackerNavigator->IsValid())
			return true;
		else {
			delete fTrackerNavigator;
			fTrackerNavigator = NULL;

			// If for some reason we already have one
			delete fFolderNavigator;
			entry_ref currentRef = ref;
			fFolderNavigator = new FolderNavigator(currentRef);
		}
	}

	return false;
}


// #pragma mark -


ImageFileNavigator::ImageFileNavigator(const entry_ref& ref,
	const BMessenger& trackerMessenger)
	:
	fCurrentRef(ref),
	fDocumentIndex(1),
	fDocumentCount(1)
{
	fNavigator = new AutoAdjustingNavigator(fCurrentRef, trackerMessenger);
}


ImageFileNavigator::~ImageFileNavigator()
{
	delete fNavigator;
}


void
ImageFileNavigator::SetTo(const entry_ref& ref, int32 page, int32 pageCount)
{
	fCurrentRef = ref;
	fDocumentIndex = page;
	fDocumentCount = pageCount;
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
		fDocumentIndex = 1;
		return true;
	}
	return false;
}


bool
ImageFileNavigator::LastPage()
{
	if (fDocumentIndex != fDocumentCount) {
		fDocumentIndex = fDocumentCount;
		return true;
	}
	return false;
}


bool
ImageFileNavigator::NextPage()
{
	if (fDocumentIndex < fDocumentCount) {
		fDocumentIndex++;
		return true;
	}
	return false;
}


bool
ImageFileNavigator::PreviousPage()
{
	if (fDocumentIndex > 1) {
		fDocumentIndex--;
		return true;
	}
	return false;
}


bool
ImageFileNavigator::GoToPage(int32 page)
{
	if (page > 0 && page <= fDocumentCount && page != fDocumentIndex) {
		fDocumentIndex = page;
		return true;
	}
	return false;
}


bool
ImageFileNavigator::FirstFile()
{
	entry_ref ref;
	if (fNavigator->FindNextImage(fCurrentRef, ref, false, true)) {
		SetTo(ref, 1, 1);
		fNavigator->UpdateSelection(fCurrentRef);
		return true;
	}

	return false;
}


bool
ImageFileNavigator::NextFile()
{
	entry_ref ref;
	if (fNavigator->FindNextImage(fCurrentRef, ref, true, false)) {
		SetTo(ref, 1, 1);
		fNavigator->UpdateSelection(fCurrentRef);
		return true;
	}

	return false;
}


bool
ImageFileNavigator::PreviousFile()
{
	entry_ref ref;
	if (fNavigator->FindNextImage(fCurrentRef, ref, false, false)) {
		SetTo(ref, 1, 1);
		fNavigator->UpdateSelection(fCurrentRef);
		return true;
	}

	return false;
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


bool
ImageFileNavigator::GetNextFile(const entry_ref& ref, entry_ref& nextRef)
{
	return fNavigator->FindNextImage(ref, nextRef, true, false);
}


bool
ImageFileNavigator::GetPreviousFile(const entry_ref& ref,
	entry_ref& previousRef)
{
	return fNavigator->FindNextImage(ref, previousRef, false, false);
}


/*!	Moves the current file into the trash.
	Returns true if a new file should be loaded, false if not.
*/
bool
ImageFileNavigator::MoveFileToTrash()
{
	entry_ref nextRef;
	if (!fNavigator->FindNextImage(fCurrentRef, nextRef, true, false)
		&& !fNavigator->FindNextImage(fCurrentRef, nextRef, false, false))
		nextRef.device = -1;

	// Move image to Trash
	BMessage trash(BPrivate::kMoveToTrash);
	trash.AddRef("refs", &fCurrentRef);

	// We create our own messenger because the member fTrackerMessenger
	// could be invalid
	BMessenger tracker(kTrackerSignature);
	if (tracker.SendMessage(&trash) != B_OK)
		return false;

	if (nextRef.device != -1) {
		SetTo(nextRef, 1, 1);
		return true;
	}

	return false;
}
