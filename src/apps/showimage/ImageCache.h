/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_CACHE_H
#define IMAGE_CACHE_H


#include <deque>
#include <map>
#include <set>

#include <Entry.h>
#include <Locker.h>
#include <String.h>

#include <kernel/util/DoublyLinkedList.h>
#include <Referenceable.h>


class BBitmap;
class BMessage;
class BMessenger;
struct QueueEntry;


enum {
	kMsgImageCacheImageLoaded		= 'icIL',
	kMsgImageCacheProgressUpdate	= 'icPU'
};


class BitmapOwner : public BReferenceable {
public:
								BitmapOwner(BBitmap* bitmap);
	virtual						~BitmapOwner();

private:
			BBitmap*			fBitmap;
};


struct CacheEntry : DoublyLinkedListLinkImpl<CacheEntry> {
	entry_ref				ref;
	int32					page;
	int32					pageCount;
	BBitmap*				bitmap;
	BitmapOwner*			bitmapOwner;
	BString					type;
	BString					mimeType;
};


class ImageCache {
public:
	static	ImageCache&			Default() { return sCache; }

			status_t			RetrieveImage(const entry_ref& ref,
									int32 page = 1,
									const BMessenger* target = NULL);

private:
								ImageCache();
	virtual						~ImageCache();

	static	status_t			_QueueWorkerThread(void* self);

			status_t			_RetrieveImage(QueueEntry* entry,
									CacheEntry** _entry);
			void				_NotifyListeners(CacheEntry* entry,
									QueueEntry* queueEntry);
			void				_NotifyTarget(CacheEntry* entry,
									const BMessenger* target);
			void				_BuildNotification(CacheEntry* entry,
									BMessage& message);

private:
			typedef std::pair<entry_ref, int32> ImageSelector;
			typedef std::map<ImageSelector, CacheEntry*> CacheMap;
			typedef std::map<ImageSelector, QueueEntry*> QueueMap;
			typedef std::deque<QueueEntry*> QueueDeque;
			typedef DoublyLinkedList<CacheEntry> CacheList;

			BLocker				fLocker;
			CacheMap			fCacheMap;
			CacheList			fCacheEntriesByAge;
			QueueMap			fQueueMap;
			QueueDeque			fQueue;
			vint32				fThreadCount;
			int32				fMaxThreadCount;
			uint64				fBytes;
			uint64				fMaxBytes;
			size_t				fMaxEntries;

	static	ImageCache			sCache;
};


#endif	// IMAGE_CACHE_H
