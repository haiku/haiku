/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef __NU_ICON_CACHE__
#define __NU_ICON_CACHE__


// Icon cache is used for drawing node icons; it caches icons
// and reuses them for successive draws


#include <Bitmap.h>
#include <Mime.h>
#include <String.h>

#include "AutoLock.h"
#include "ObjectList.h"
#include "OpenHashTable.h"
#include "Utilities.h"


// Icon cache splits icons into two caches - the shared cache, likely to get the
// most hits and the node cache. Every icon that is found in a mime based
// structure goes into the shared cache, only files that have their own private
// icon use the node cache;
// Entries are only deleted from the shared cache if an icon for a mime type changes,
// this makes async icon drawing easier. Node cache deletes it's entries whenever a
// file gets deleted.

// if a view ever uses the cache to draw in async mode, it needs to call
// it when it is being destroyed

namespace BPrivate {

class Model;
class ModelNodeLazyOpener;
class LazyBitmapAllocator;
class SharedIconCache;
class SharedCacheEntry;

enum IconDrawMode {
	// Different states of icon drawing
	kSelected 					= 0x01,
	kNotFocused					= 0x02,		// Tracker window
	kOpen						= 0x04,		// open folder, trash
	kNotEmpty					= 0x08,		// full trash
	kDisabled					= 0x10,		// inactive nav menu entry
	kActive						= 0x20,		// active home dir, boot volume
	kLink						= 0x40,		// symbolic link
	kTrackerSpecialized			= 0x80,

	// some common combinations
	kNormalIcon						= 0,
	kSelectedIcon					= kSelected,
	kSelectedInBackgroundIcon		= kSelected | kNotFocused,
	kOpenIcon						= kOpen,
	kOpenSelectedIcon				= kSelected | kOpen,
	kOpenSelectedInBackgroundIcon	= kSelected | kNotFocused | kOpen,
	kFullIcon						= kNotEmpty,
	kFullSelectedIcon				= kNotEmpty | kOpen,
	kDimmedIcon
};


#define NORMAL_ICON_ONLY kNormalIcon
	// replace use of these defines with mode once the respective getters
	// can get non-plain icons


// Where did an icon come from
enum IconSource {
	kUnknownSource,
	kUnknownNotFromNode,	// icon origin not known but determined not to be from
							// the node itself
	kTrackerDefault,		// file has no type, Tracker provides generic, folder,
							// symlink or app
	kTrackerSupplied,		// home directory, boot volume, trash, etc.
	kMetaMime,				// from BMimeType
	kPreferredAppForType,	// have a preferred application for a type, has an icon
	kPreferredAppForNode,	// have a preferred application for this node,
							// has an icon
	kVolume,
	kNode
};


class IconCacheEntry {
	// aliased entries don't own their icons, just point
	// to some other entry that does
	
	// This is used for icons that are defined by a preferred app for
	// a metamime, types that do not have an icon get to point to
	// generic, etc.
	
public:
	IconCacheEntry();
	~IconCacheEntry();
	
	void SetAliasFor(const SharedIconCache*, const SharedCacheEntry*);
	static IconCacheEntry* ResolveIfAlias(const SharedIconCache*, IconCacheEntry*);
	IconCacheEntry* ResolveIfAlias(const SharedIconCache*);
	
	void SetIcon(BBitmap* bitmap, IconDrawMode mode, icon_size size,
		bool create = false);
	
	bool HaveIconBitmap(IconDrawMode mode, icon_size size) const;
	bool CanConstructBitmap(IconDrawMode mode, icon_size size) const;
	static bool AlternateModeForIconConstructing(IconDrawMode requestedMode,
		IconDrawMode &alternate, icon_size size);
	BBitmap* ConstructBitmap(BBitmap* constructFrom, IconDrawMode requestedMode,
		IconDrawMode constructFromMode, icon_size size,
		LazyBitmapAllocator*);
	BBitmap* ConstructBitmap(IconDrawMode requestedMode, icon_size size,
		LazyBitmapAllocator*);
		// same as above, always uses normal icon as source

	bool IconHitTest(BPoint, IconDrawMode, icon_size) const;
		// given a point, returns true if a non-transparent pixel was hit

	void RetireIcons(BObjectList<BBitmap>* retiredBitmapList);
		// can't just delete icons, they may be still drawing
		// async; instead, put them on the retired list and
		// only delete the list if it grows too much, way after
		// the icon finishes drawing
		//
		// This could fail if we retire a lot of icons (10 * 1024)
		// while we are drawing them, shouldn't be a practical problem

protected:

	BBitmap* IconForMode(IconDrawMode mode, icon_size size) const;
	void SetIconForMode(BBitmap* bitmap, IconDrawMode mode, icon_size size);

	// list of most common icons
	BBitmap* fLargeIcon;
	BBitmap* fMiniIcon;
	BBitmap* fHilitedLargeIcon;
	BBitmap* fHilitedMiniIcon;
	int32 fAliasForIndex;
	
	// list of other icon kinds would be added here
	
	friend class SharedIconCache;
	friend class NodeIconCache;
};


class SimpleIconCache {
public:
	SimpleIconCache(const char*);
	virtual ~SimpleIconCache() {}

	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode mode,
		icon_size size, bool async = false) = 0;
	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
		icon_size, void (*)(BView*, BPoint, BBitmap*, void*),
		void* = NULL) = 0;

	bool Lock();
	void Unlock();
	bool IsLocked() const;
	
private:
	Benaphore fLock;
};


class SharedCacheEntry : public IconCacheEntry {
public:
	SharedCacheEntry();
	SharedCacheEntry(const char* fileType, const char* appSignature = 0);

	void Draw(BView*, BPoint, IconDrawMode mode, icon_size size,
		bool async = false);

	void Draw(BView*, BPoint, IconDrawMode, icon_size,
		void (*)(BView*, BPoint, BBitmap*, void*), void* = NULL);

	const char* FileType() const;
	const char* AppSignature() const;
	
	// hash table support
	uint32 Hash() const;
	static uint32 Hash(const char* fileType, const char* appSignature = 0);
	bool operator==(const SharedCacheEntry &) const;
	void SetTo(const char* fileType, const char* appSignature = 0);

	int32 fNext;
private:
	BString fFileType;
	BString fAppSignature;

	friend class SharedIconCache;
};


class SharedCacheEntryArray : public OpenHashElementArray<SharedCacheEntry> {
	// SharedIconCache stores all it's elements in this array
public:
	SharedCacheEntryArray(int32 initialSize);
	SharedCacheEntry* Add();
};


class SharedIconCache : public SimpleIconCache {
	// SharedIconCache is used for icons that come from the mime database
public:
	SharedIconCache();

	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode mode,
		icon_size size, bool async = false);
	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
		icon_size, void (*)(BView*, BPoint, BBitmap*, void*), void* = NULL);

	SharedCacheEntry* FindItem(const char* fileType, const char* appSignature = 0)
		const;
	SharedCacheEntry* AddItem(const char* fileType, const char* appSignature = 0);
	SharedCacheEntry* AddItem(SharedCacheEntry** outstandingEntry, const char* fileType,
		const char* appSignature = 0);
		// same as previous AddItem, updates the pointer to outstandingEntry, because
		// adding to the hash table makes any pending pointer invalid
	void IconChanged(SharedCacheEntry*);

	void SetAliasFor(IconCacheEntry* alias, const SharedCacheEntry* original) const;
	IconCacheEntry* ResolveIfAlias(IconCacheEntry* entry) const;
	int32 EntryIndex(const SharedCacheEntry* entry) const;

	void RemoveAliasesTo(int32 index);

private:
	OpenHashTable<SharedCacheEntry, SharedCacheEntryArray> fHashTable;
	SharedCacheEntryArray fElementArray;
	BObjectList<BBitmap> fRetiredBitmaps;
		// icons are drawn asynchronously, can't just delete them
		// right away, instead have to place them onto the retired bitmap list
		// and wait for the next sync to delete them
};


class NodeCacheEntry : public IconCacheEntry {
public:
	NodeCacheEntry(bool permanent = false);
	NodeCacheEntry(const node_ref*, bool permanent = false);
	void Draw(BView*, BPoint, IconDrawMode mode, icon_size size,
		bool async = false);
	
	void Draw(BView*, BPoint, IconDrawMode, icon_size,
		void (*)(BView*, BPoint, BBitmap*, void*), void* = NULL);

	const node_ref* Node() const;
	
	uint32 Hash() const;
	static uint32 Hash(const node_ref*);
	bool operator==(const NodeCacheEntry&) const;
	void SetTo(const node_ref*);
	void MakePermanent();
	bool Permanent() const;
	
	int32 fNext;
private:
	node_ref fRef;
	bool fPermanent;
		// special cache entry that has to be deleted explicitly

	friend class NodeIconCache;
};


class NodeCacheEntryArray : public OpenHashElementArray<NodeCacheEntry> {
	// NodeIconCache stores all it's elements in this array
public:
	NodeCacheEntryArray(int32 initialSize);
	NodeCacheEntry* Add();
};


class NodeIconCache : public SimpleIconCache {
	// NodeIconCache is used for nodes that define their own icons
public:
	NodeIconCache();

	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
		icon_size, bool async = false);

	virtual void Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
		icon_size, void (*)(BView*, BPoint, BBitmap*, void*), void* = 0);

	NodeCacheEntry* FindItem(const node_ref*) const;
	NodeCacheEntry* AddItem(const node_ref*, bool permanent = false);
	NodeCacheEntry* AddItem(NodeCacheEntry** outstandingEntry, const node_ref*);
		// same as previous AddItem, updates the pointer to outstandingEntry, because
		// adding to the hash table makes any pending pointer invalid
	void Deleting(const node_ref*);
		// model for this node is getting deleted (not necessarily the node itself)
	void Removing(const node_ref*);
		// used by permanent NodeIconCache entries, when an entry gets deleted
	void Deleting(const BView*);
	void IconChanged(const Model*);


	void RemoveAliasesTo(int32 index);

private:
	OpenHashTable<NodeCacheEntry, NodeCacheEntryArray> fHashTable;
	NodeCacheEntryArray fElementArray;
};


const int32 kColorTransformTableSize = 256;


class IconCache {
public:
	IconCache();
	
	void Draw(Model*, BView*, BPoint where, IconDrawMode mode,
		icon_size size, bool async = false);
		// draw an icon for a model, load the icon from the appropriate
		// location if not cached already

	void SyncDraw(Model*, BView*, BPoint, IconDrawMode,
		icon_size, void (*)(BView*, BPoint, BBitmap*, void*),
		void* passThruState = 0);
		// draw an icon for a model, load the icon from the appropriate
		// location if not cached already; only works for sync draws,
		// once the call returns, the bitmap may be deleted

	// preload calls used to ensure successive cache hit for the respective
	// icon, used for common tracker types, etc; Not calling these should only
	// cause a slowdown
	void Preload(Model*, IconDrawMode mode, icon_size size,
		bool permanent = false);
	status_t Preload(const char* mimeType, IconDrawMode mode, icon_size size);

	void Deleting(const Model*);
		// hook to manage unloading icons for nodes that are going away
	void Removing(const Model* model);
		// used by permanent NodeIconCache entries, when an entry gets
		// deleted
	void Deleting(const BView*);
		// hook to manage deleting draw view caches for views that are
		// going away
	
	// icon changed calls, used when a node or a file type has an icon changed
	// the icons for the node/file type will be flushed and re-cached during
	// the next draw
	void IconChanged(Model*);
	void IconChanged(const char* mimeType, const char* appSignature);
	
	bool IsIconFrom(const Model*, const char* mimeType,
		const char* appSignature) const;
		// called when metamime database changed to figure out which models
		// to redraw

	bool IconHitTest(BPoint, const Model*, IconDrawMode, icon_size);

	// utility calls for building specialized icons
	BBitmap* MakeSelectedIcon(const BBitmap* normal, icon_size,
		LazyBitmapAllocator*);

	static bool NeedsDeletionNotification(IconSource);

	static IconCache* sIconCache;

private:
	// shared calls
	IconCacheEntry* Preload(AutoLock<SimpleIconCache>* nodeCache,
		AutoLock<SimpleIconCache>* sharedCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconDrawMode mode, icon_size size, bool permanent);
		// preload uses lazy locking, returning the cache we decided
		// to use to get the icon
		// <resultingLockedCache> may be null if we don't care

	// shared mime-based icon retrieval calls
	IconCacheEntry* GetIconForPreferredApp(const char* mimeTypeSignature,
		const char* preferredApp, IconDrawMode mode, icon_size size,
		 LazyBitmapAllocator*, IconCacheEntry*);
	IconCacheEntry* GetIconFromFileTypes(ModelNodeLazyOpener*, IconSource &source,
		IconDrawMode mode, icon_size size, LazyBitmapAllocator*,
		IconCacheEntry*);
	IconCacheEntry* GetIconFromMetaMime(const char* fileType, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*,
		IconCacheEntry*);
	IconCacheEntry* GetVolumeIcon(AutoLock<SimpleIconCache>* nodeCache,
		AutoLock<SimpleIconCache>* sharedCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconSource&, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*);
	IconCacheEntry* GetRootIcon(AutoLock<SimpleIconCache>* nodeCache,
		AutoLock<SimpleIconCache>* sharedCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconSource&, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*);
	IconCacheEntry* GetWellKnownIcon(AutoLock<SimpleIconCache> *nodeCache,
		AutoLock<SimpleIconCache>* sharedCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconSource&, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*);
	IconCacheEntry* GetNodeIcon(ModelNodeLazyOpener *,
		AutoLock<SimpleIconCache>* nodeCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconSource&, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*, IconCacheEntry*, bool permanent);
	IconCacheEntry* GetGenericIcon(AutoLock<SimpleIconCache>* sharedCache,
		AutoLock<SimpleIconCache>** resultingLockedCache,
		Model*, IconSource&, IconDrawMode mode,
		icon_size size, LazyBitmapAllocator*, IconCacheEntry*);
	IconCacheEntry* GetFallbackIcon(AutoLock<SimpleIconCache>* sharedCacheLocker,
		AutoLock<SimpleIconCache>** resultingOpenCache,
		Model* model, IconDrawMode mode, icon_size size,
		LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry);

	BBitmap* MakeTransformedIcon(const BBitmap*, icon_size,
		int32 colorTransformTable [], LazyBitmapAllocator*);

	NodeIconCache fNodeCache;
	SharedIconCache fSharedCache;

	void InitHiliteTable();

	int32 fHiliteTable[kColorTransformTableSize];
	bool fInitHiliteTable;
		// on if we still need to initialize the hilite table
};


class LazyBitmapAllocator {
// Utility class used when we aren't sure that we will keep a bitmap,
// need a bitmap or be able to construct it properly
public:
	LazyBitmapAllocator(icon_size size,
						color_space colorSpace = kDefaultIconDepth,
		bool preallocate = false);
	~LazyBitmapAllocator();

	BBitmap* Get();
	BBitmap* Adopt();

private:
	BBitmap* fBitmap;
	icon_size fSize;
	color_space fColorSpace;
};


// inlines follow

inline const char*
SharedCacheEntry::FileType() const
{
	return fFileType.String();
}


inline const char*
SharedCacheEntry::AppSignature() const
{
	return fAppSignature.String();
}


inline bool
IconCache::NeedsDeletionNotification(IconSource from)
{
	return from == kNode;
}


inline IconCacheEntry*
SharedIconCache::ResolveIfAlias(IconCacheEntry* entry) const
{
	if (entry->fAliasForIndex < 0)
		return entry;
	
	return fHashTable.ElementAt(entry->fAliasForIndex);
}


inline int32
SharedIconCache::EntryIndex(const SharedCacheEntry* entry) const
{
	return fHashTable.ElementIndex(entry);
}

} // namespace BPrivate

using namespace BPrivate;

#endif
