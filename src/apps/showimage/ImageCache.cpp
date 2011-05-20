/*
 * Copyright 2010-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ImageCache.h"

#include <new>

#include <Autolock.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Debug.h>
#include <File.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <AutoDeleter.h>

#include "ShowImageConstants.h"


//#define TRACE_CACHE
#undef TRACE
#ifdef TRACE_CACHE
#	define TRACE(x, ...) printf(x, __VA_ARGS__)
#else
#	define TRACE(x, ...) ;
#endif


struct QueueEntry {
	entry_ref				ref;
	int32					page;
	status_t				status;
	std::set<BMessenger>	listeners;
};


/*static*/ ImageCache ImageCache::sCache;


// #pragma mark -


BitmapOwner::BitmapOwner(BBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


BitmapOwner::~BitmapOwner()
{
	delete fBitmap;
}


// #pragma mark -


ImageCache::ImageCache()
	:
	fLocker("image cache"),
	fThreadCount(0),
	fBytes(0)
{
	system_info info;
	get_system_info(&info);

	fMaxThreadCount = info.cpu_count - 1;
	if (fMaxThreadCount < 1)
		fMaxThreadCount = 1;
	fMaxBytes = info.max_pages * B_PAGE_SIZE / 5;
	fMaxEntries = 10;
	TRACE("max thread count: %" B_PRId32 ", max bytes: %" B_PRIu64
			", max entries: %" B_PRIuSIZE "\n",
		fMaxThreadCount, fMaxBytes, fMaxEntries);
}


ImageCache::~ImageCache()
{
	// TODO: delete CacheEntries, and QueueEntries
}


status_t
ImageCache::RetrieveImage(const entry_ref& ref, int32 page,
	const BMessenger* target)
{
	BAutolock locker(fLocker);

	CacheMap::iterator find = fCacheMap.find(std::make_pair(ref, page));
	if (find != fCacheMap.end()) {
		CacheEntry* entry = find->second;

		// Requeue cache entry to the end of the by-age list
		TRACE("requeue trace entry %s\n", ref.name);
		fCacheEntriesByAge.Remove(entry);
		fCacheEntriesByAge.Add(entry);

		// Notify target, if any
		_NotifyTarget(entry, target);
		return B_OK;
	}

	QueueMap::iterator findQueue = fQueueMap.find(std::make_pair(ref, page));
	QueueEntry* entry;

	if (findQueue == fQueueMap.end()) {
		if (target == NULL
			&& ((fCacheMap.size() < 4 && fCacheMap.size() > 1
					&& fBytes + fBytes / fCacheMap.size() > fMaxBytes)
				|| (fMaxThreadCount == 1 && fQueueMap.size() > 1))) {
			// Don't accept any further precaching if we're low on memory
			// anyway, or if there is already a busy queue.
			TRACE("ignore entry %s\n", ref.name);
			return B_NO_MEMORY;
		}

		TRACE("add to queue %s\n", ref.name);

		// Push new entry to the queue
		entry = new(std::nothrow) QueueEntry();
		if (entry == NULL)
			return B_NO_MEMORY;

		entry->ref = ref;
		entry->page = page;

		if (fThreadCount < fMaxThreadCount) {
			// start a new worker thread to load the image
			thread_id thread = spawn_thread(&ImageCache::_QueueWorkerThread,
				"image loader", B_LOW_PRIORITY, this);
			if (thread >= B_OK) {
				atomic_add(&fThreadCount, 1);
				resume_thread(thread);
			} else if (fThreadCount == 0) {
				delete entry;
				return thread;
			}
		}

		fQueueMap.insert(std::make_pair(
			std::make_pair(entry->ref, entry->page), entry));
		fQueue.push_front(entry);
	} else {
		entry = findQueue->second;
		TRACE("got entry %s from cache\n", entry->ref.name);
	}

	if (target != NULL) {
		// Attach target as listener
		entry->listeners.insert(*target);
	}

	return B_OK;
}


/*static*/ status_t
ImageCache::_QueueWorkerThread(void* _self)
{
	ImageCache* self = (ImageCache*)_self;
	TRACE("%ld: start worker thread\n", find_thread(NULL));

	// get next queue entry
	while (true) {
		self->fLocker.Lock();
		if (self->fQueue.empty()) {
			self->fLocker.Unlock();
			break;
		}

		QueueEntry* entry = *self->fQueue.begin();
		TRACE("%ld: got entry %s from queue.\n", find_thread(NULL),
			entry->ref.name);
		self->fQueue.pop_front();
		self->fLocker.Unlock();

		CacheEntry* cacheEntry = NULL;
		entry->status = self->_RetrieveImage(entry, &cacheEntry);

		self->fLocker.Lock();
		self->fQueueMap.erase(std::make_pair(entry->ref, entry->page));
		self->_NotifyListeners(cacheEntry, entry);
		self->fLocker.Unlock();

		delete entry;
	}

	atomic_add(&self->fThreadCount, -1);
	TRACE("%ld: end worker thread\n", find_thread(NULL));
	return B_OK;
}


status_t
ImageCache::_RetrieveImage(QueueEntry* queueEntry, CacheEntry** _entry)
{
	CacheEntry* entry = new(std::nothrow) CacheEntry();
	if (entry == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<CacheEntry> deleter(entry);

	BTranslatorRoster* roster = BTranslatorRoster::Default();
	if (roster == NULL)
		return B_ERROR;

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

	// TODO: this doesn't work for images that already are in the queue...
	if (!queueEntry->listeners.empty()) {
		BMessage progress(kMsgImageCacheProgressUpdate);
		progress.AddRef("ref", &queueEntry->ref);
		ioExtension.AddMessenger("/progressMonitor",
			*queueEntry->listeners.begin());
		ioExtension.AddMessage("/progressMessage", &progress);
	}

	// Translate image data and create a new ShowImage window

	BBitmapStream outstream;

	status = roster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP);
	if (status == B_OK) {
		status = roster->Translate(&file, &info, &ioExtension, &outstream,
			B_TRANSLATOR_BITMAP);
	}
	if (status != B_OK)
		return status;

	BBitmap* bitmap;
	if (outstream.DetachBitmap(&bitmap) != B_OK)
		return B_ERROR;

	entry->bitmapOwner = new(std::nothrow) BitmapOwner(bitmap);
	if (entry->bitmapOwner == NULL) {
		delete bitmap;
		return B_NO_MEMORY;
	}

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

	deleter.Detach();
	*_entry = entry;

	BAutolock locker(fLocker);

	fCacheMap.insert(std::make_pair(
		std::make_pair(entry->ref, entry->page), entry));
	fCacheEntriesByAge.Add(entry);

	fBytes += bitmap->BitsLength();

	TRACE("%ld: cached entry %s from queue (%" B_PRIu64 " bytes.\n",
		find_thread(NULL), entry->ref.name, fBytes);

	while (fBytes > fMaxBytes || fCacheMap.size() > fMaxEntries) {
		if (fCacheMap.size() <= 2)
			break;

		// Remove the oldest entry
		entry = fCacheEntriesByAge.RemoveHead();
		TRACE("%ld: purge cached entry %s from queue.\n", find_thread(NULL),
			entry->ref.name);
		fBytes -= entry->bitmap->BitsLength();
		fCacheMap.erase(std::make_pair(entry->ref, entry->page));

		entry->bitmapOwner->ReleaseReference();
		delete entry;
	}

	return B_OK;
}


void
ImageCache::_NotifyListeners(CacheEntry* entry, QueueEntry* queueEntry)
{
	ASSERT(fLocker.IsLocked());

	if (queueEntry->listeners.empty())
		return;

	BMessage notification(kMsgImageCacheImageLoaded);
	_BuildNotification(entry, notification);

	if (queueEntry->status != B_OK)
		notification.AddInt32("error", queueEntry->status);

	std::set<BMessenger>::iterator iterator = queueEntry->listeners.begin();
	for (; iterator != queueEntry->listeners.end(); iterator++) {
		if (iterator->SendMessage(&notification) == B_OK && entry != NULL) {
			entry->bitmapOwner->AcquireReference();
				// this is the reference owned by the target
		}
	}
}


void
ImageCache::_NotifyTarget(CacheEntry* entry, const BMessenger* target)
{
	if (target == NULL)
		return;

	BMessage notification(kMsgImageCacheImageLoaded);
	_BuildNotification(entry, notification);

	if (target->SendMessage(&notification) == B_OK && entry != NULL) {
		entry->bitmapOwner->AcquireReference();
			// this is the reference owned by the target
	}
}


void
ImageCache::_BuildNotification(CacheEntry* entry, BMessage& message)
{
	if (entry == NULL)
		return;

	message.AddString("type", entry->type);
	message.AddString("mime", entry->mimeType);
	message.AddRef("ref", &entry->ref);
	message.AddInt32("page", entry->page);
	message.AddInt32("pageCount", entry->pageCount);
	message.AddPointer("bitmap", (void*)entry->bitmap);
	message.AddPointer("bitmapOwner", (void*)entry->bitmapOwner);
}
