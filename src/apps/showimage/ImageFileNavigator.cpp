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

#include <deque>
#include <map>
#include <new>
#include <set>

#include <stdio.h>

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Path.h>
#include <TranslatorRoster.h>

#include <AutoDeleter.h>
#include <tracker_private.h>
#include <kernel/util/DoublyLinkedList.h>

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


struct CacheEntry : DoublyLinkedListLinkImpl<CacheEntry> {
	entry_ref				ref;
	int32					page;
	int32					pageCount;
	BBitmap*				bitmap;
	BString					type;
	BString					mimeType;
};


struct QueueEntry {
	entry_ref				ref;
	int32					page;
	std::set<BMessenger>	listeners;
};


class ImageCache {
public:
								ImageCache();
	virtual						~ImageCache();

			void				RetrieveImage(const entry_ref& ref,
									const BMessenger* target);

private:
	static	status_t			_QueueWorkerThread(void* self);

			status_t			_RetrieveImage(QueueEntry* entry,
									BMessage& message);
			void				_NotifyListeners(QueueEntry* entry,
									BMessage& message);

private:
			typedef std::pair<entry_ref, int32> ImageSelector;
			typedef std::map<ImageSelector, CacheEntry*> CacheMap;
			typedef std::map<ImageSelector, QueueEntry*> QueueMap;
			typedef std::deque<QueueEntry*> QueueDeque;
			typedef DoublyLinkedList<CacheEntry> CacheList;

			BLocker				fCacheLocker;
			CacheMap			fCacheMap;
			CacheList			fCacheEntriesByAge;
			BLocker				fQueueLocker;
			QueueMap			fQueueMap;
			QueueDeque			fQueue;
			vint32				fThreadCount;
			int32				fMaxThreadCount;
			uint64				fBytes;
			uint64				fMaxBytes;
			size_t				fMaxEntries;
};


ImageCache::ImageCache()
	:
	fCacheLocker("image cache"),
	fQueueLocker("image queue"),
	fThreadCount(0),
	fBytes(0)
{
	system_info info;
	get_system_info(&info);

	fMaxThreadCount = (info.cpu_count + 1) / 2;
	fMaxBytes = info.max_pages * B_PAGE_SIZE / 8;
	fMaxEntries = 10;
}


ImageCache::~ImageCache()
{
	// TODO: delete CacheEntries, and QueueEntries
}


void
ImageCache::RetrieveImage(const entry_ref& ref, const BMessenger* target)
{
	// TODO!
}


/*static*/ status_t
ImageCache::_QueueWorkerThread(void* _self)
{
	ImageCache* self = (ImageCache*)_self;

	// get next queue entry
	while (true) {
		self->fQueueLocker.Lock();
		if (self->fQueue.empty()) {
			self->fQueueLocker.Unlock();
			break;
		}

		QueueEntry* entry = *self->fQueue.begin();
		self->fQueue.pop_front();
		self->fQueueLocker.Unlock();

		if (entry == NULL)
			break;

		BMessage notification(kMsgImageLoaded);
		status_t status = self->_RetrieveImage(entry, notification);
		if (status != B_OK)
			notification.AddInt32("error", status);

		self->fQueueLocker.Lock();
		self->fQueueMap.erase(std::make_pair(entry->ref, entry->page));
		self->fQueueLocker.Unlock();

		self->_NotifyListeners(entry, notification);
		delete entry;
	}

	atomic_add(&self->fThreadCount, -1);
	return B_OK;
}


status_t
ImageCache::_RetrieveImage(QueueEntry* queueEntry, BMessage& message)
{
	CacheEntry* entry = new(std::nothrow) CacheEntry();
	if (entry == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<CacheEntry> deleter(entry);

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return B_ERROR;

	if (!entry_ref_is_file(queueEntry->ref))
		return B_IS_A_DIRECTORY;

	BFile file;
	status_t status = file.SetTo(&queueEntry->ref, B_READ_ONLY);
	if (status != B_OK)
		return status;

	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;

	if (queueEntry->page != 0
		&& ioExtension.AddInt32("/documentIndex", queueEntry->page) != B_OK)
		return B_NO_MEMORY;

// TODO: rethink this!
#if 0
	if (fProgressWindow != NULL) {
		BMessage progress(kMsgProgressStatusUpdate);
		if (ioExtension.AddMessenger("/progressMonitor",
				fProgressWindow) == B_OK
			&& ioExtension.AddMessage("/progressMessage", &progress) == B_OK)
			fProgressWindow->Start();
	}
#endif

	// Translate image data and create a new ShowImage window

	BBitmapStream outstream;

	status = roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP);
	if (status == B_OK) {
		status = roster->Translate(&file, &info, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP);
	}

#if 0
	if (fProgressWindow != NULL)
		fProgressWindow->Stop();
#endif

	if (status != B_OK)
		return status;

	BBitmap* bitmap;
	if (outstream.DetachBitmap(&bitmap) != B_OK)
		return B_ERROR;

	entry->ref = queueEntry->ref;
	entry->page = queueEntry->page;
	entry->bitmap = bitmap;
	entry->type = info.name;
	entry->mimeType = info.MIME;

	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK
		&& documentCount > 0)
		entry->pageCount = documentCount;
	else
		entry->pageCount = 1;

	message.AddString("type", info.name);
	message.AddString("mime", info.MIME);
	message.AddRef("ref", &entry->ref);
	message.AddInt32("page", entry->page);
	message.AddPointer("bitmap", (void*)bitmap);

	deleter.Detach();

	fCacheLocker.Lock();

	fCacheMap.insert(std::make_pair(
		std::make_pair(entry->ref, entry->page), entry));
	fCacheEntriesByAge.Add(entry);

	fBytes += bitmap->BitsLength();

	while (fBytes > fMaxBytes || fCacheMap.size() > fMaxEntries) {
		if (fCacheMap.size() <= 2)
			break;

		// Remove the oldest entry
		entry = fCacheEntriesByAge.RemoveHead();
		fBytes -= entry->bitmap->BitsLength();
		fCacheMap.erase(std::make_pair(entry->ref, entry->page));
		delete entry;
	}

	fCacheLocker.Unlock();
	return B_OK;
}


void
ImageCache::_NotifyListeners(QueueEntry* entry, BMessage& message)
{
	std::set<BMessenger>::iterator iterator = entry->listeners.begin();
	for (; iterator != entry->listeners.end(); iterator++) {
		iterator->SendMessage(&message);
	}
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


/*!	Moves the current file into the trash.
	Returns true if a new file is being loaded, false if not.
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
		return true;

	if (nextRef.device != -1 && LoadImage(nextRef) == B_OK) {
		fNavigator->UpdateSelection(nextRef);
		return true;
	}

	return false;
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


