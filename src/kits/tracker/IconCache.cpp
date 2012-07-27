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

//	Icon cache is used for drawing node icons; it caches icons
//	and reuses them for successive draws

//
// Possible performance improvements:
//	- Mime API requires BBitmaps to retrieve bits and successive
//	SetBits that cause app server contention
//	Consider having special purpose "give me just the bits" calls
//	to deal with that.
//	- Related to this, node cache entries would only store the raw bits
//	to cut down on number of BBitmaps and related overhead
//	- Make the cache miss and fill case for the shared cache reuse the
//	already calculated hash value
//
//	Other ToDo items:
//	Use lazily allocated bitmap arrays for every view for node icon cache
//		drawing
//	Have an overflow list for deleting shared icons, delete from the list
//	every now and then


// Actual icon lookup sequence:
//			icon from node
//			preferred app for node -> icon for type
//			preferred app for type -> icon for type
//			metamime -> icon for type
//			preferred app for supertype -> icon for type
//			supertype metamime -> icon for type
//			generic icon


#include <Debug.h>
#include <Screen.h>
#include <Volume.h>
#include <fs_info.h>

#include "Bitmaps.h"
#include "FSUtils.h"
#include "IconCache.h"
#include "MimeTypes.h"
#include "Model.h"

#if DEBUG
// #define LOG_DISK_HITS
//	the LOG_DISK_HITS define is used to check that the disk is not hit more
//	than needed - enable it, open a window with a bunch of poses, force
//	it to redraw, shouldn't recache
// #define LOG_ADD_ITEM
#endif

// set up a few printing macros to get rid of a ton of debugging ifdefs in the code
#ifdef LOG_DISK_HITS
 #define PRINT_DISK_HITS(ARGS) _debugPrintf ARGS
#else
 #define PRINT_DISK_HITS(ARGS) (void)0
#endif

#ifdef LOG_ADD_ITEM
 #define PRINT_ADD_ITEM(ARGS) _debugPrintf ARGS
#else
 #define PRINT_ADD_ITEM(ARGS) (void)0
#endif

#undef NODE_CACHE_ASYNC_DRAWS


IconCacheEntry::IconCacheEntry()
	:	fLargeIcon(NULL),
		fMiniIcon(NULL),
		fHilitedLargeIcon(NULL),
		fHilitedMiniIcon(NULL),
		fAliasForIndex(-1)
{
}


IconCacheEntry::~IconCacheEntry()
{
	if (fAliasForIndex < 0) {
		delete fLargeIcon;
		delete fMiniIcon;
		delete fHilitedLargeIcon;
		delete fHilitedMiniIcon;

		// clean up a bit to leave the hash table entry in an initialized state
		fLargeIcon = NULL;
		fMiniIcon = NULL;
		fHilitedLargeIcon = NULL;
		fHilitedMiniIcon = NULL;

	}
	fAliasForIndex = -1;
}


void
IconCacheEntry::SetAliasFor(const SharedIconCache* sharedCache,
	const SharedCacheEntry* entry)
{
	sharedCache->SetAliasFor(this, entry);
	ASSERT(fAliasForIndex >= 0);
}


IconCacheEntry*
IconCacheEntry::ResolveIfAlias(const SharedIconCache* sharedCache)
{
	return sharedCache->ResolveIfAlias(this);
}


IconCacheEntry*
IconCacheEntry::ResolveIfAlias(const SharedIconCache* sharedCache,
	IconCacheEntry* entry)
{
	if (!entry)
		return NULL;

	return sharedCache->ResolveIfAlias(entry);
}


bool
IconCacheEntry::CanConstructBitmap(IconDrawMode mode, icon_size) const
{
	if (mode == kSelected)
		// for now only
		return true;

	return false;
}


bool
IconCacheEntry::HaveIconBitmap(IconDrawMode mode, icon_size size) const
{
	ASSERT(mode == kSelected || mode == kNormalIcon);
		// for now only

	if (mode == kNormalIcon) {
		if (size == B_MINI_ICON)
			return fMiniIcon != NULL;
		else
			return fLargeIcon != NULL
				&& fLargeIcon->Bounds().IntegerWidth() + 1 == size;
	} else if (mode == kSelected) {
		if (size == B_MINI_ICON)
			return fHilitedMiniIcon != NULL;
		else
			return fHilitedLargeIcon != NULL
				&& fHilitedLargeIcon->Bounds().IntegerWidth() + 1 == size;
	}
	return false;
}


BBitmap*
IconCacheEntry::IconForMode(IconDrawMode mode, icon_size size) const
{
	ASSERT(mode == kSelected || mode == kNormalIcon);
		// for now only

	if (mode == kNormalIcon) {
		if (size == B_MINI_ICON)
			return fMiniIcon;
		else
			return fLargeIcon;
	} else if (mode == kSelected) {
		if (size == B_MINI_ICON)
			return fHilitedMiniIcon;
		else
			return fHilitedLargeIcon;
	}
	return NULL;
}


bool
IconCacheEntry::IconHitTest(BPoint where, IconDrawMode mode, icon_size size) const
{
	ASSERT(where.x < size && where.y < size);
	BBitmap* bitmap = IconForMode(mode, size);
	if (!bitmap)
		return false;

	uchar* bits = (uchar*)bitmap->Bits();
	ASSERT(bits);

	BRect bounds(bitmap->Bounds());
	bounds.InsetBy((bounds.Width() + 1.0) / 8.0, (bounds.Height() + 1.0) / 8.0);
	if (bounds.Contains(where))
		return true;

	switch (bitmap->ColorSpace()) {
		case B_RGBA32:
			// test alpha channel
			return *(bits + (int32)(floorf(where.y) * bitmap->BytesPerRow()
				+ floorf(where.x) * 4 + 3)) > 20;

		case B_CMAP8:
			return *(bits + (int32)(floorf(where.y) * size + where.x))
				!= B_TRANSPARENT_8_BIT;

		default:
			return true;
	}
}


BBitmap*
IconCacheEntry::ConstructBitmap(BBitmap* constructFrom, IconDrawMode requestedMode,
	IconDrawMode constructFromMode, icon_size size, LazyBitmapAllocator* lazyBitmap)
{
	ASSERT(requestedMode == kSelected && constructFromMode == kNormalIcon);
		// for now
	if (requestedMode == kSelected && constructFromMode == kNormalIcon)
		return IconCache::sIconCache->MakeSelectedIcon(constructFrom, size, lazyBitmap);

	return NULL;
}


BBitmap*
IconCacheEntry::ConstructBitmap(IconDrawMode requestedMode, icon_size size,
	LazyBitmapAllocator* lazyBitmap)
{
	BBitmap* source = (size == B_MINI_ICON) ? fMiniIcon : fLargeIcon;
	ASSERT(source);
	return ConstructBitmap(source, requestedMode, kNormalIcon, size, lazyBitmap);
}


bool
IconCacheEntry::AlternateModeForIconConstructing(IconDrawMode requestedMode,
	IconDrawMode &alternate, icon_size)
{
	if (requestedMode & kSelected) {
		// for now
		alternate = kNormalIcon;
		return true;
	}
	return false;
}


void
IconCacheEntry::SetIcon(BBitmap* bitmap, IconDrawMode mode, icon_size size,
	bool /*create*/)
{
	if (mode == kNormalIcon) {
		if (size == B_MINI_ICON)
			fMiniIcon = bitmap;
		else
			fLargeIcon = bitmap;
	} else if (mode == kSelectedIcon) {
		if (size == B_MINI_ICON)
			fHilitedMiniIcon = bitmap;
		else
			fHilitedLargeIcon = bitmap;
	} else
		TRESPASS();
}


IconCache::IconCache()
	:	fInitHiliteTable(true)
{
	InitHiliteTable();
}


// The following calls use the icon lookup sequence node-prefered app for node-
// metamime-preferred app for metamime to find an icon;
// if we are trying to get a specialized icon, we will first look for a normal
// icon in each of the locations, if we get a hit, we look for the specialized,
// if we don't find one, we try to auto-construct one, if we can't we assume the
// icon is not available
// for now the code only looks for normal icons, selected icons are auto-generated

IconCacheEntry*
IconCache::GetIconForPreferredApp(const char* fileTypeSignature,
	const char* preferredApp, IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	ASSERT(fSharedCache.IsLocked());

	if (!preferredApp[0])
		return NULL;

	if (!entry) {
		entry = fSharedCache.FindItem(fileTypeSignature, preferredApp);
		if (entry) {
			entry = entry->ResolveIfAlias(&fSharedCache, entry);
#if xDEBUG
			PRINT(("File %s; Line %d # looking for %s, type %s, found %x\n",
				__FILE__, __LINE__, preferredApp, fileTypeSignature, entry));
#endif
			if (entry->HaveIconBitmap(mode, size))
				return entry;
		}
	}

	if (!entry || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {

		PRINT_DISK_HITS(("File %s; Line %d # hitting disk for preferredApp %s, type %s\n",
			__FILE__, __LINE__, preferredApp, fileTypeSignature));

		BMimeType preferredAppType(preferredApp);
		BString signature(fileTypeSignature);
		signature.ToLower();
		if (preferredAppType.GetIconForType(signature.String(), lazyBitmap->Get(),
			size) != B_OK)
			return NULL;

		BBitmap* bitmap = lazyBitmap->Adopt();
		if (!entry) {
			PRINT_ADD_ITEM(("File %s; Line %d # adding entry for preferredApp %s, type %s\n",
				__FILE__, __LINE__, preferredApp, fileTypeSignature));
			entry = fSharedCache.AddItem(fileTypeSignature, preferredApp);
		}
		entry->SetIcon(bitmap, kNormalIcon, size);
	}

	if (mode != kNormalIcon
		&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
	}

	return entry;
}


IconCacheEntry*
IconCache::GetIconFromMetaMime(const char* fileType, IconDrawMode mode,
	icon_size size, LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	ASSERT(fSharedCache.IsLocked());

	if (!entry)
		entry = fSharedCache.FindItem(fileType);

	if (entry) {
		entry = entry->ResolveIfAlias(&fSharedCache, entry);
		// metamime defines an icon and we have it cached
		if (entry->HaveIconBitmap(mode, size))
			return entry;
	}

	if (!entry || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		PRINT_DISK_HITS(("File %s; Line %d # hitting disk for metamime %s\n",
			__FILE__, __LINE__, fileType));

		BMimeType mime(fileType);
		// try getting the icon directly from the metamime
		if (mime.GetIcon(lazyBitmap->Get(), size) != B_OK) {
			// try getting it from the preferred app of this type
			char preferredAppSig[B_MIME_TYPE_LENGTH];
			if (mime.GetPreferredApp(preferredAppSig) != B_OK)
				return NULL;

			SharedCacheEntry* aliasTo = NULL;
			if (entry)
				aliasTo = (SharedCacheEntry*)entry->ResolveIfAlias(&fSharedCache);

			// look for icon defined by preferred app from metamime
			aliasTo = (SharedCacheEntry*)GetIconForPreferredApp(fileType,
				preferredAppSig, mode, size, lazyBitmap, aliasTo);

			if (aliasTo == NULL)
				return NULL;

			// make an aliased entry so that the next time we get a
			// hit on the first FindItem in here
			if (!entry) {
				PRINT_ADD_ITEM(("File %s; Line %d # adding entry as alias for type %s\n",
					__FILE__, __LINE__, fileType));
				entry = fSharedCache.AddItem(&aliasTo, fileType);
				entry->SetAliasFor(&fSharedCache, aliasTo);
			}
			ASSERT(aliasTo->HaveIconBitmap(mode, size));
			return aliasTo;
		}

		// at this point, we've found an icon for the MIME type
		BBitmap* bitmap = lazyBitmap->Adopt();
		if (!entry) {
			PRINT_ADD_ITEM(("File %s; Line %d # adding entry for type %s\n",
				__FILE__, __LINE__, fileType));
			entry = fSharedCache.AddItem(fileType);
		}
		entry->SetIcon(bitmap, kNormalIcon, size);
	}

	ASSERT(entry);
	if (mode != kNormalIcon
		&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
	}

#if xDEBUG
	if (!entry->HaveIconBitmap(mode, size))
		PRINT(("failing on %s, mode %ld, size %ld\n", fileType, mode, size));
#endif

	ASSERT(entry->HaveIconBitmap(mode, size));
	return entry;
}


IconCacheEntry*
IconCache::GetIconFromFileTypes(ModelNodeLazyOpener* modelOpener,
	IconSource &source, IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	ASSERT(fSharedCache.IsLocked());
	// use file types to get the icon
	Model* model = modelOpener->TargetModel();

	const char* fileType = model->MimeType();
	const char* nodePreferredApp = model->PreferredAppSignature();
	if (source == kUnknownSource || source == kUnknownNotFromNode
		|| source == kPreferredAppForNode) {

		if (nodePreferredApp[0]) {
			// file has a locally set preferred app, try getting an icon from
			// there
			entry = GetIconForPreferredApp(fileType, nodePreferredApp, mode,
				size, lazyBitmap, entry);
#if xDEBUG
			PRINT(("File %s; Line %d # looking for %s, type %s, found %x\n",
				__FILE__, __LINE__, nodePreferredApp, fileType, entry));
#endif
			if (entry) {
				source = kPreferredAppForNode;
				ASSERT(entry->HaveIconBitmap(mode, size));
				return entry;
			}
		}
		if (source == kPreferredAppForNode)
			source = kUnknownSource;
	}

	entry = GetIconFromMetaMime(fileType, mode, size, lazyBitmap, entry);
	if (!entry) {
		// Try getting a supertype handler icon
		BMimeType mime(fileType);
		if (!mime.IsSupertypeOnly()) {
			BMimeType superType;
			mime.GetSupertype(&superType);
			const char* superTypeFileType = superType.Type();
			if (superTypeFileType)
				entry = GetIconFromMetaMime(superTypeFileType, mode, size,
					lazyBitmap, entry);
#if DEBUG
			else
				PRINT(("File %s; Line %d # failed to get supertype for type %s\n",
					__FILE__, __LINE__, fileType));
#endif
		}
	}
	ASSERT(!entry || entry->HaveIconBitmap(mode, size));
	if (entry) {
		if (nodePreferredApp[0]) {
			// we got a miss using GetIconForPreferredApp before, cache this
			// fileType/preferredApp combo with an aliased entry

			// make an aliased entry so that the next time we get a
			// hit and substitute a generic icon right away

			PRINT_ADD_ITEM(("File %s; Line %d # adding entry as alias for preferredApp %s, type %s\n",
				__FILE__, __LINE__, nodePreferredApp, fileType));
			IconCacheEntry* aliasedEntry = fSharedCache.AddItem((SharedCacheEntry**)&entry,
				fileType, nodePreferredApp);
			aliasedEntry->SetAliasFor(&fSharedCache, (SharedCacheEntry*)entry);
				// OK to cast here, have a runtime check
			source = kPreferredAppForNode;
				// set source as preferred for node, so that next time we get a hit in
				// the initial find that uses GetIconForPreferredApp
		} else
			source = kMetaMime;
#if DEBUG
		if (!entry->HaveIconBitmap(mode, size))
			model->PrintToStream();
#endif
		ASSERT(entry->HaveIconBitmap(mode, size));
	}
	return entry;
}

IconCacheEntry*
IconCache::GetVolumeIcon(AutoLock<SimpleIconCache>*nodeCacheLocker,
	AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model* model, IconSource &source,
	IconDrawMode mode, icon_size size, LazyBitmapAllocator* lazyBitmap)
{
	*resultingOpenCache = nodeCacheLocker;
	nodeCacheLocker->Lock();

	IconCacheEntry* entry = 0;
	if (source != kUnknownSource) {
		// cached in the node cache
		entry = fNodeCache.FindItem(model->NodeRef());
		if (entry) {
			entry = IconCacheEntry::ResolveIfAlias(&fSharedCache, entry);

			if (source == kTrackerDefault) {
				// if tracker default, resolved entry is from shared cache
				// this could be done a little cleaner if entry had a way to reach
				// the cache it is in
				*resultingOpenCache = sharedCacheLocker;
				sharedCacheLocker->Lock();
			}

			if (entry->HaveIconBitmap(mode, size))
				return entry;
		}
	}

	// try getting using the BVolume::GetIcon call; if miss,
	// go for the default mime based icon
	if (!entry || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		BVolume volume(model->NodeRef()->device);

		if (volume.IsShared()) {
			// Check if it's a network share and give it a special icon
			BBitmap* bitmap = lazyBitmap->Get();
			GetTrackerResources()->GetIconResource(R_ShareIcon, size, bitmap);
			if (!entry) {
				PRINT_ADD_ITEM(("File %s; Line %d # adding entry for model %s\n",
					__FILE__, __LINE__, model->Name()));
				entry = fNodeCache.AddItem(model->NodeRef());
			}
			entry->SetIcon(lazyBitmap->Adopt(), kNormalIcon, size);
		} else if (volume.GetIcon(lazyBitmap->Get(), size) == B_OK) {
			// Ask the device for an icon
			BBitmap* bitmap = lazyBitmap->Adopt();
			ASSERT(bitmap);
			if (!entry) {
				PRINT_ADD_ITEM(("File %s; Line %d # adding entry for model %s\n",
					__FILE__, __LINE__, model->Name()));
				entry = fNodeCache.AddItem(model->NodeRef());
			}
			ASSERT(entry);
			entry->SetIcon(bitmap, kNormalIcon, size);
			source = kVolume;
		} else {
			*resultingOpenCache = sharedCacheLocker;
			sharedCacheLocker->Lock();

			// If the volume doesnt have a device it should have the generic icon
			entry = GetIconFromMetaMime(B_VOLUME_MIMETYPE, mode,
				size, lazyBitmap, entry);
		}
	}

	if (mode != kNormalIcon
		&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
	}
	return entry;
}


IconCacheEntry*
IconCache::GetRootIcon(AutoLock<SimpleIconCache>*,
	AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model*, IconSource &source, IconDrawMode mode,
	icon_size size, LazyBitmapAllocator* lazyBitmap)
{
	*resultingOpenCache = sharedCacheLocker;
	(*resultingOpenCache)->Lock();

	source = kTrackerSupplied;
	return GetIconFromMetaMime(B_ROOT_MIMETYPE, mode, size, lazyBitmap, 0);
}


IconCacheEntry*
IconCache::GetWellKnownIcon(AutoLock<SimpleIconCache>*,
	AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model* model, IconSource &source, IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap)
{
	const WellKnowEntryList::WellKnownEntry* wellKnownEntry
		= WellKnowEntryList::MatchEntry(model->NodeRef());
	if (!wellKnownEntry)
		return NULL;

	IconCacheEntry* entry = NULL;

	BString type("tracker/active_");
	type += wellKnownEntry->name;

	*resultingOpenCache = sharedCacheLocker;
	(*resultingOpenCache)->Lock();

	source = kTrackerSupplied;

	entry = fSharedCache.FindItem(type.String());
	if (entry) {
		entry = entry->ResolveIfAlias(&fSharedCache, entry);
		if (entry->HaveIconBitmap(mode, size))
			return entry;
	}

	if (!entry || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		// match up well known entries in the file system with specialized
		// icons stored in Tracker's resources
		int32 resid = -1;
		switch ((uint32)wellKnownEntry->which) {
			case B_BOOT_DISK:
				resid = R_BootVolumeIcon;
				break;

			case B_BEOS_DIRECTORY:
				resid = R_BeosFolderIcon;
				break;

			case B_USER_DIRECTORY:
				resid = R_HomeDirIcon;
				break;

			case B_BEOS_FONTS_DIRECTORY:
			case B_COMMON_FONTS_DIRECTORY:
			case B_USER_FONTS_DIRECTORY:
				resid = R_FontDirIcon;
				break;

			case B_BEOS_APPS_DIRECTORY:
			case B_APPS_DIRECTORY:
			case B_USER_DESKBAR_APPS_DIRECTORY:
				resid = R_AppsDirIcon;
				break;

			case B_BEOS_PREFERENCES_DIRECTORY:
			case B_PREFERENCES_DIRECTORY:
			case B_USER_DESKBAR_PREFERENCES_DIRECTORY:
				resid = R_PrefsDirIcon;
				break;

			case B_USER_MAIL_DIRECTORY:
				resid = R_MailDirIcon;
				break;

			case B_USER_QUERIES_DIRECTORY:
				resid = R_QueryDirIcon;
				break;

			case B_COMMON_DEVELOP_DIRECTORY:
			case B_USER_DESKBAR_DEVELOP_DIRECTORY:
				resid = R_DevelopDirIcon;
				break;

			case B_USER_CONFIG_DIRECTORY:
				resid = R_ConfigDirIcon;
				break;

			case B_USER_PEOPLE_DIRECTORY:
				resid = R_PersonDirIcon;
				break;

			case B_USER_DOWNLOADS_DIRECTORY:
				resid = R_DownloadDirIcon;
				break;

			default:
				return NULL;
		}

		entry = fSharedCache.AddItem(type.String());

		BBitmap* bitmap = lazyBitmap->Get();
		GetTrackerResources()->GetIconResource(resid, size, bitmap);
		entry->SetIcon(lazyBitmap->Adopt(), kNormalIcon, size);
	}

	if (mode != kNormalIcon
		&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
	}

	ASSERT(entry->HaveIconBitmap(mode, size));
	return entry;
}


IconCacheEntry*
IconCache::GetNodeIcon(ModelNodeLazyOpener* modelOpener,
	AutoLock<SimpleIconCache>* nodeCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model* model, IconSource &source,
	IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry, bool permanent)
{
	*resultingOpenCache = nodeCacheLocker;
	(*resultingOpenCache)->Lock();

	entry = fNodeCache.FindItem(model->NodeRef());
	if (!entry || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		modelOpener->OpenNode();

		BFile* file = NULL;

		// if we are dealing with an application, use the BAppFileInfo
		// superset of node; this makes GetIcon grab the proper icon for
		// an app
		if (model->IsExecutable())
			file = dynamic_cast<BFile*>(model->Node());

		PRINT_DISK_HITS(("File %s; Line %d # hitting disk for node %s\n",
			__FILE__, __LINE__, model->Name()));

		status_t result;
		if (file)
			result = GetAppIconFromAttr(file, lazyBitmap->Get(), size);
		else
			result = GetFileIconFromAttr(model->Node(), lazyBitmap->Get(), size);

		if (result == B_OK) {
			// node has it's own icon, use it

			BBitmap* bitmap = lazyBitmap->Adopt();
			PRINT_ADD_ITEM(("File %s; Line %d # adding entry for model %s\n",
				__FILE__, __LINE__, model->Name()));
			entry = fNodeCache.AddItem(model->NodeRef(), permanent);
			ASSERT(entry);
			entry->SetIcon(bitmap, kNormalIcon, size);
			if (mode != kNormalIcon) {
				entry->ConstructBitmap(mode, size, lazyBitmap);
				entry->SetIcon(lazyBitmap->Adopt(), mode, size);
			}
			source = kNode;
		}
	}

	if (!entry) {
		(*resultingOpenCache)->Unlock();
		*resultingOpenCache = NULL;
	} else if (!entry->HaveIconBitmap(mode, size)
		&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
		ASSERT(entry->HaveIconBitmap(mode, size));
	}

	return entry;
}


IconCacheEntry*
IconCache::GetGenericIcon(AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model* model, IconSource &source,
	IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	*resultingOpenCache = sharedCacheLocker;
	(*resultingOpenCache)->Lock();

	entry = GetIconFromMetaMime(B_FILE_MIMETYPE, mode,
		size, lazyBitmap, 0);

	if (!entry)
		return NULL;

	// make an aliased entry so that the next time we get a
	// hit and substitute a generic icon right away
	PRINT_ADD_ITEM(("File %s; Line %d # adding entry for preferredApp %s, type %s\n",
		__FILE__, __LINE__, model->PreferredAppSignature(),
		model->MimeType()));
	IconCacheEntry* aliasedEntry = fSharedCache.AddItem(
		(SharedCacheEntry**)&entry, model->MimeType(),
		model->PreferredAppSignature());
	aliasedEntry->SetAliasFor(&fSharedCache, (SharedCacheEntry*)entry);

	source = kMetaMime;

	ASSERT(entry->HaveIconBitmap(mode, size));
	return entry;
}


IconCacheEntry*
IconCache::GetFallbackIcon(AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingOpenCache,
	Model* model, IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	*resultingOpenCache = sharedCacheLocker;
	(*resultingOpenCache)->Lock();

	entry = fSharedCache.AddItem(model->MimeType(),
		model->PreferredAppSignature());

	BBitmap* bitmap = lazyBitmap->Get();
	GetTrackerResources()->GetIconResource(R_FileIcon, size, bitmap);
	entry->SetIcon(lazyBitmap->Adopt(), kNormalIcon, size);

	if (mode != kNormalIcon) {
		entry->ConstructBitmap(mode, size, lazyBitmap);
		entry->SetIcon(lazyBitmap->Adopt(), mode, size);
	}

	ASSERT(entry->HaveIconBitmap(mode, size));
	return entry;
}


IconCacheEntry*
IconCache::Preload(AutoLock<SimpleIconCache>* nodeCacheLocker,
	AutoLock<SimpleIconCache>* sharedCacheLocker,
	AutoLock<SimpleIconCache>** resultingCache,
	Model* model, IconDrawMode mode, icon_size size,
	bool permanent)
{
	IconCacheEntry* entry = NULL;

	AutoLock<SimpleIconCache>* resultingOpenCache = NULL;
		// resultingOpenCache is the locker that points to the cache that
		// ended with a hit and will be used for the drawing

	{	// scope for modelOpener

		ModelNodeLazyOpener modelOpener(model);
			// this opener takes care of opening the model and possibly
			// closing it when we are done
		LazyBitmapAllocator lazyBitmap(size);
			// lazyBitmap manages bitmap allocation and freeing if needed

		IconSource source = model->IconFrom();
		if (source == kUnknownSource || source == kUnknownNotFromNode) {

			// fish for special first models and handle them appropriately
			if (model->IsVolume()) {
				// volume may use specialized icon in the volume node
				entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
					&resultingOpenCache, model, source, mode, size,
					&lazyBitmap, entry, permanent);

				if (!entry || !entry->HaveIconBitmap(mode, size))
					// look for volume defined icon
					entry = GetVolumeIcon(nodeCacheLocker, sharedCacheLocker,
						&resultingOpenCache, model, source, mode,
						size, &lazyBitmap);

			} else if (model->IsRoot()) {

				entry = GetRootIcon(nodeCacheLocker, sharedCacheLocker,
					&resultingOpenCache, model, source, mode, size, &lazyBitmap);
				ASSERT(entry);

			} else {
				if (source == kUnknownSource)
					// look for node icons first
					entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
						&resultingOpenCache, model, source,
						mode, size, &lazyBitmap, entry, permanent);


				if (!entry) {
					// no node icon, look for file type based one
					modelOpener.OpenNode();
					// use file types to get the icon
					resultingOpenCache = sharedCacheLocker;
					resultingOpenCache->Lock();

					entry = GetIconFromFileTypes(&modelOpener, source, mode, size,
						&lazyBitmap, 0);

					if (!entry) // we don't have an icon, go with the generic
						entry = GetGenericIcon(sharedCacheLocker, &resultingOpenCache,
							model, source, mode, size, &lazyBitmap, entry);
				}
			}
			// update the icon source
			model->SetIconFrom(source);

		} else {
			// we already know where the icon should come from,
			// use shortcuts to get it
			switch (source) {
				case kNode:
					resultingOpenCache = nodeCacheLocker;
					resultingOpenCache->Lock();

					entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
						&resultingOpenCache, model, source, mode,
						size, &lazyBitmap, entry, permanent);

					if (entry) {
						entry = IconCacheEntry::ResolveIfAlias(&fSharedCache, entry);
						if (!entry->HaveIconBitmap(mode, size)
							&& entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
							entry->ConstructBitmap(mode, size, &lazyBitmap);
							entry->SetIcon(lazyBitmap.Adopt(), mode, size);
						}
						ASSERT(entry->HaveIconBitmap(mode, size));
					}
					break;
				case kTrackerSupplied:
					if (model->IsRoot()) {
						entry = GetRootIcon(nodeCacheLocker, sharedCacheLocker,
							&resultingOpenCache, model, source, mode, size,
							&lazyBitmap);
						break;
					} else {
						entry = GetWellKnownIcon(nodeCacheLocker, sharedCacheLocker,
							&resultingOpenCache, model, source, mode, size,
							&lazyBitmap);

						if (entry)
							break;
					}
					// fall through
				case kTrackerDefault:
				case kVolume:
					if (model->IsVolume()) {
						entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
							&resultingOpenCache, model, source,
							mode, size, &lazyBitmap, entry, permanent);
						if (!entry || !entry->HaveIconBitmap(mode, size))
							entry = GetVolumeIcon(nodeCacheLocker, sharedCacheLocker,
								&resultingOpenCache, model, source, mode, size,
								&lazyBitmap);
						break;
					}
					// fall through
				case kMetaMime:
				case kPreferredAppForType:
				case kPreferredAppForNode:
					resultingOpenCache = sharedCacheLocker;
					resultingOpenCache->Lock();

					entry = GetIconFromFileTypes(&modelOpener, source, mode, size,
						&lazyBitmap, 0);
					ASSERT(!entry || entry->HaveIconBitmap(mode, size));

					if (!entry || !entry->HaveIconBitmap(mode, size))
						// we don't have an icon, go with the generic
						entry = GetGenericIcon(sharedCacheLocker, &resultingOpenCache,
							model, source, mode, size, &lazyBitmap, entry);

					model->SetIconFrom(source);
						// the source shouldn't change in this case; if it does though we
						// might never be hitting the correct icon and instead keep leaking
						// entries after each miss
						// this now happens if an app defines an icon but a GetIconForType
						// fails and we fall back to generic icon
						// ToDo:
						// fix this and add an assert to the effect

					ASSERT(entry);
					ASSERT(entry->HaveIconBitmap(mode, size));
					break;

				default:
					TRESPASS();
			}
		}

		if (!entry || !entry->HaveIconBitmap(mode, size)) {
			// we don't have an icon, go with the generic
			PRINT(("icon cache complete miss, falling back on generic icon for %s\n",
				model->Name()));
			entry = GetGenericIcon(sharedCacheLocker, &resultingOpenCache,
				model, source, mode, size, &lazyBitmap, entry);

			// we don't even have generic, something is really broken,
			// go with hardcoded generic icon
			if (!entry || !entry->HaveIconBitmap(mode, size)) {
				PRINT(("icon cache complete miss, falling back on generic icon for %s\n",
					model->Name()));
				entry = GetFallbackIcon(sharedCacheLocker, &resultingOpenCache,
					model, mode, size, &lazyBitmap, entry);
			}

			// force icon pick up next time around because we probably just
			// hit a node in transition
			model->SetIconFrom(kUnknownSource);
		}
	}

	ASSERT(entry && entry->HaveIconBitmap(mode, size));

	if (resultingCache)
		*resultingCache = resultingOpenCache;

	return entry;
}


void
IconCache::Draw(Model* model, BView* view, BPoint where, IconDrawMode mode,
	icon_size size, bool async)
{
	// the following does not actually lock the caches, we are using the
	// lockLater mode; we will decide which of the two to lock down depending
	// on where we get the icon from
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	AutoLock<SimpleIconCache>* resultingCacheLocker;
	IconCacheEntry* entry = Preload(&nodeCacheLocker, &sharedCacheLocker,
		&resultingCacheLocker, model, mode, size, false);
		// Preload finds/creates the appropriate entry, locking down the
		// cache it is in and returns the whole state back to here

	if (!entry)
		return;

	ASSERT(entry);
	ASSERT(entry->HaveIconBitmap(mode, size));
	// got the entry, now draw it
	resultingCacheLocker->LockedItem()->Draw(entry, view, where, mode,
		size, async);

	// either of the two cache lockers that got locked down by this call get
	// unlocked at this point
}


void
IconCache::SyncDraw(Model* model, BView* view, BPoint where, IconDrawMode mode,
	icon_size size, void (*blitFunc)(BView*, BPoint, BBitmap*, void*),
	void* passThruState)
{
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	AutoLock<SimpleIconCache>* resultingCacheLocker;
	IconCacheEntry* entry = Preload(&nodeCacheLocker, &sharedCacheLocker,
		&resultingCacheLocker, model, mode, size, false);

	if (!entry)
		return;

	ASSERT(entry);
	ASSERT(entry->HaveIconBitmap(mode, size));
	resultingCacheLocker->LockedItem()->Draw(entry, view, where,
		mode, size, blitFunc, passThruState);
}


void
IconCache::Preload(Model* model, IconDrawMode mode, icon_size size, bool permanent)
{
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	Preload(&nodeCacheLocker, &sharedCacheLocker, 0, model, mode, size, permanent);
}


status_t
IconCache::Preload(const char* fileType, IconDrawMode mode, icon_size size)
{
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache);
	LazyBitmapAllocator lazyBitmap(size);

	BMimeType mime(fileType);
	char preferredAppSig[B_MIME_TYPE_LENGTH];
	status_t result = mime.GetPreferredApp(preferredAppSig);
	if (result != B_OK)
		return result;

	// try getting the icon from the preferred app for the signature
	IconCacheEntry* entry = GetIconForPreferredApp(fileType, preferredAppSig,
		mode, size, &lazyBitmap, 0);
	if (entry)
		return B_OK;

	// try getting the icon directly from the metamime
	result = mime.GetIcon(lazyBitmap.Get(), size);

	if (result != B_OK)
		return result;

	entry = fSharedCache.AddItem(fileType);
	BBitmap* bitmap = lazyBitmap.Adopt();
	entry->SetIcon(bitmap, kNormalIcon, size);
	if (mode != kNormalIcon) {
		entry->ConstructBitmap(mode, size, &lazyBitmap);
		entry->SetIcon(lazyBitmap.Adopt(), mode, size);
	}

	return B_OK;
}


void
IconCache::Deleting(const Model* model)
{
	AutoLock<SimpleIconCache> lock(&fNodeCache);

	if (model->IconFrom() == kNode)
		fNodeCache.Deleting(model->NodeRef());

	// don't care if the node uses the shared cache
}


void
IconCache::Removing(const Model* model)
{
	AutoLock<SimpleIconCache> lock(&fNodeCache);

	if (model->IconFrom() == kNode)
		fNodeCache.Removing(model->NodeRef());
}


void
IconCache::Deleting(const BView* view)
{
	AutoLock<SimpleIconCache> lock(&fNodeCache);
	fNodeCache.Deleting(view);
}


void
IconCache::IconChanged(Model* model)
{
	AutoLock<SimpleIconCache> lock(&fNodeCache);

	if (model->IconFrom() == kNode || model->IconFrom() == kVolume)
		fNodeCache.Deleting(model->NodeRef());

	model->ResetIconFrom();
}


void
IconCache::IconChanged(const char* mimeType, const char* appSignature)
{
	AutoLock<SimpleIconCache> sharedLock(&fSharedCache);
	SharedCacheEntry* entry = fSharedCache.FindItem(mimeType, appSignature);
	if (!entry)
		return;

	AutoLock<SimpleIconCache> nodeLock(&fNodeCache);

	entry = (SharedCacheEntry*)fSharedCache.ResolveIfAlias(entry);
	ASSERT(entry);
	int32 index = fSharedCache.EntryIndex(entry);

	fNodeCache.RemoveAliasesTo(index);
	fSharedCache.RemoveAliasesTo(index);

	fSharedCache.IconChanged(entry);
}


BBitmap*
IconCache::MakeSelectedIcon(const BBitmap* normal, icon_size size,
	LazyBitmapAllocator* lazyBitmap)
{
	return MakeTransformedIcon(normal, size, fHiliteTable, lazyBitmap);
}

#if xDEBUG

static void
DumpBitmap(const BBitmap* bitmap)
{
	if (!bitmap){
		printf("NULL bitmap passed to DumpBitmap\n");
		return;
	}
	int32 length = bitmap->BitsLength();

	printf("data length %ld \n", length);

	int32 columns = (int32)bitmap->Bounds().Width() + 1;
	const unsigned char* bitPtr = (const unsigned char*)bitmap->Bits();
	for (; length >= 0; length--) {
		for (int32 columnIndex = 0; columnIndex < columns;
			columnIndex++, length--)
			printf("%c%c", "0123456789ABCDEF"[(*bitPtr)/0x10],
				"0123456789ABCDEF"[(*bitPtr++)%0x10]);

		printf("\n");
	}
	printf("\n");
}

#endif

void
IconCache::InitHiliteTable()
{
	// build the color transform tables for different icon modes
	BScreen screen(B_MAIN_SCREEN_ID);
	rgb_color color;
	for (int32 index = 0; index < kColorTransformTableSize; index++) {
		color = screen.ColorForIndex((uchar)index);
		fHiliteTable[index] = screen.IndexForColor(tint_color(color, 1.3f));
	}

	fHiliteTable[B_TRANSPARENT_8_BIT] = B_TRANSPARENT_8_BIT;
	fInitHiliteTable = false;
}


BBitmap*
IconCache::MakeTransformedIcon(const BBitmap* source, icon_size /*size*/,
	int32 colorTransformTable[], LazyBitmapAllocator* lazyBitmap)
{
	if (fInitHiliteTable)
		InitHiliteTable();

	BBitmap* result = lazyBitmap->Get();
	uint8* src = (uint8*)source->Bits();
	uint8* dst = (uint8*)result->Bits();

//	ASSERT(result->ColorSpace() == source->ColorSpace()
//		&& result->Bounds() == source->Bounds());
	if (result->ColorSpace() != source->ColorSpace()
		|| result->Bounds() != source->Bounds()) {
		printf("IconCache::MakeTransformedIcon() - "
					"bitmap format mismatch!\n");
		return NULL;
	}

	switch (result->ColorSpace()) {
		case B_RGB32:
		case B_RGBA32: {
			uint32 width = source->Bounds().IntegerWidth() + 1;
			uint32 height = source->Bounds().IntegerHeight() + 1;
			uint32 srcBPR = source->BytesPerRow();
			uint32 dstBPR = result->BytesPerRow();
			for (uint32 y = 0; y < height; y++) {
				uint8* d = dst;
				uint8* s = src;
				for (uint32 x = 0; x < width; x++) {
					// 66% brightness
					d[0] = (int)s[0] * 168 >> 8;
					d[1] = (int)s[1] * 168 >> 8;
					d[2] = (int)s[2] * 168 >> 8;
					d[3] = s[3];
					d += 4;
					s += 4;
				}
				dst += dstBPR;
				src += srcBPR;
			}
			break;
		}

		case B_CMAP8: {
			int32 bitsLength = result->BitsLength();
			for (int32 i = 0; i < bitsLength; i++)
				*dst++ = (uint8)colorTransformTable[*src++];
			break;
		}

		default:
			memset(dst, 0, result->BitsLength());
			// unkown colorspace, no tinting for you
			// "black" should make the problem stand out
			break;
	}

	return result;
}


bool
IconCache::IconHitTest(BPoint where, const Model* model, IconDrawMode mode,
	icon_size size)
{
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	AutoLock<SimpleIconCache>* resultingCacheLocker;
	IconCacheEntry* entry = Preload(&nodeCacheLocker, &sharedCacheLocker,
		&resultingCacheLocker, const_cast<Model*>(model), mode, size, false);
		// Preload finds/creates the appropriate entry, locking down the
		// cache it is in and returns the whole state back to here

	if (entry)
		return entry->IconHitTest(where, mode, size);

	return false;
}


void
IconCacheEntry::RetireIcons(BObjectList<BBitmap>* retiredBitmapList)
{
	if (fLargeIcon) {
		retiredBitmapList->AddItem(fLargeIcon);
		fLargeIcon = NULL;
	}
	if (fMiniIcon) {
		retiredBitmapList->AddItem(fMiniIcon);
		fMiniIcon = NULL;
	}
	if (fHilitedLargeIcon) {
		retiredBitmapList->AddItem(fHilitedLargeIcon);
		fHilitedLargeIcon = NULL;
	}
	if (fHilitedMiniIcon) {
		retiredBitmapList->AddItem(fHilitedMiniIcon);
		fHilitedMiniIcon = NULL;
	}

	int32 count = retiredBitmapList->CountItems();
	if (count > 10 * 1024) {
		PRINT(("nuking old icons from the retired bitmap list\n"));
		for (count = 512; count > 0; count--)
			delete retiredBitmapList->RemoveItemAt(0);
	}
}


//	#pragma mark -


// In debug mode keep the hash table sizes small so that they grow a lot and
// execercise the resizing code a lot. In release mode allocate them large up-front
// for better performance

SharedIconCache::SharedIconCache()
#if DEBUG
	:	SimpleIconCache("Shared Icon cache aka \"The Dead-Locker\""),
		fHashTable(20),
		fElementArray(20),
		fRetiredBitmaps(20, true)
#else
	:	SimpleIconCache("Tracker shared icon cache"),
		fHashTable(1000),
		fElementArray(1024),
		fRetiredBitmaps(256, true)
#endif
{
	fHashTable.SetElementVector(&fElementArray);
}


void
SharedIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size, bool async)
{
	((SharedCacheEntry*)entry)->Draw(view, where, mode, size, async);
}


void
SharedIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size, void (*blitFunc)(BView*, BPoint,
	BBitmap*, void*), void* passThruState)
{
	((SharedCacheEntry*)entry)->Draw(view, where, mode, size,
		blitFunc, passThruState);
}


SharedCacheEntry*
SharedIconCache::FindItem(const char* fileType, const char* appSignature) const
{
	ASSERT(fileType);
	if (!fileType)
		fileType = B_FILE_MIMETYPE;

	SharedCacheEntry* result = fHashTable.FindFirst(SharedCacheEntry::Hash(fileType,
		appSignature));

	if (!result)
		return NULL;

	for(;;) {
		if (result->fFileType == fileType && result->fAppSignature == appSignature)
			return result;

		if (result->fNext < 0)
			break;

		result = const_cast<SharedCacheEntry*>(&fElementArray.At(result->fNext));
	}

	return NULL;
}


SharedCacheEntry*
SharedIconCache::AddItem(const char* fileType, const char* appSignature)
{
	ASSERT(fileType);
	if (!fileType)
		fileType = B_FILE_MIMETYPE;

	SharedCacheEntry* result = fHashTable.Add(SharedCacheEntry::Hash(fileType,
		appSignature));
	result->SetTo(fileType, appSignature);
	return result;
}


SharedCacheEntry*
SharedIconCache::AddItem(SharedCacheEntry** outstandingEntry, const char* fileType,
	const char* appSignature)
{
	int32 entryToken = fHashTable.ElementIndex(*outstandingEntry);
	ASSERT(entryToken >= 0);

	ASSERT(fileType);
	if (!fileType)
		fileType = B_FILE_MIMETYPE;

	SharedCacheEntry* result = fHashTable.Add(SharedCacheEntry::Hash(fileType,
		appSignature));
	result->SetTo(fileType, appSignature);
	*outstandingEntry = fHashTable.ElementAt(entryToken);

	return result;
}


void
SharedIconCache::IconChanged(SharedCacheEntry* entry)
{
	// by now there should be no aliases to entry, just remove entry
	// itself
	ASSERT(entry->fAliasForIndex == -1);
	entry->RetireIcons(&fRetiredBitmaps);
	fHashTable.Remove(entry);
}


void
SharedIconCache::RemoveAliasesTo(int32 aliasIndex)
{
	int32 count = fHashTable.VectorSize();
	for (int32 index = 0; index < count; index++) {
		SharedCacheEntry* entry = fHashTable.ElementAt(index);
		if (entry->fAliasForIndex == aliasIndex)
			fHashTable.Remove(entry);
	}
}


void
SharedIconCache::SetAliasFor(IconCacheEntry* alias, const SharedCacheEntry* original) const
{
	alias->fAliasForIndex = fHashTable.ElementIndex(original);
}


SharedCacheEntry::SharedCacheEntry()
	:	fNext(-1)
{
}


SharedCacheEntry::SharedCacheEntry(const char* fileType, const char* appSignature)
	:	fNext(-1),
		fFileType(fileType),
		fAppSignature(appSignature)
{
}


void
SharedCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode, icon_size size,
	bool async)
{
	BBitmap* bitmap = IconForMode(mode, size);
	ASSERT(bitmap);

	drawing_mode oldMode = view->DrawingMode();

	if (bitmap->ColorSpace() == B_RGBA32) {
		if (oldMode != B_OP_ALPHA) {
			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
	} else {
		view->SetDrawingMode(B_OP_OVER);
	}

	if (async)
		view->DrawBitmapAsync(bitmap, where);
	else
		view->DrawBitmap(bitmap, where);

	view->SetDrawingMode(oldMode);
}


void
SharedCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode, icon_size size,
	void (*blitFunc)(BView*, BPoint, BBitmap*, void*), void* passThruState)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (!bitmap)
		return;

//	if (bitmap->ColorSpace() == B_RGBA32) {
//		view->SetDrawingMode(B_OP_ALPHA);
//		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
//	} else {
//		view->SetDrawingMode(B_OP_OVER);
//	}
//
	(blitFunc)(view, where, bitmap, passThruState);
}


uint32
SharedCacheEntry::Hash(const char* fileType, const char* appSignature)
{
	uint32 hash = HashString(fileType, 0);
	if (appSignature && appSignature[0])
		hash = HashString(appSignature, hash);

	return hash;
}


uint32
SharedCacheEntry::Hash() const
{
	uint32 hash = HashString(fFileType.String(), 0);
	if (fAppSignature.Length())
		hash = HashString(fAppSignature.String(), hash);

	return hash;
}


bool
SharedCacheEntry::operator==(const SharedCacheEntry &entry) const
{
	return fFileType == entry.FileType() && fAppSignature == entry.AppSignature();
}


void
SharedCacheEntry::SetTo(const char* fileType, const char* appSignature)
{
	fFileType = fileType;
	fAppSignature = appSignature;
}


SharedCacheEntryArray::SharedCacheEntryArray(int32 initialSize)
	:	OpenHashElementArray<SharedCacheEntry>(initialSize)
{
}


SharedCacheEntry*
SharedCacheEntryArray::Add()
{
	return OpenHashElementArray<SharedCacheEntry>::Add();
}


//	#pragma mark -


NodeCacheEntry::NodeCacheEntry(bool permanent)
	:	fNext(-1),
		fPermanent(permanent)
{
}


NodeCacheEntry::NodeCacheEntry(const node_ref* node, bool permanent)
	:	fNext(-1),
		fRef(*node),
		fPermanent(permanent)
{
}


void
NodeCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode, icon_size size,
	bool async)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (!bitmap)
		return;

	drawing_mode oldMode = view->DrawingMode();

	if (bitmap->ColorSpace() == B_RGBA32) {
		if (oldMode != B_OP_ALPHA) {
			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
	} else {
		view->SetDrawingMode(B_OP_OVER);
	}

	if (false && async) {
		TRESPASS();
		// need to copy the bits first in here
		view->DrawBitmapAsync(bitmap, where);
	} else
		view->DrawBitmap(bitmap, where);

	view->SetDrawingMode(oldMode);
}


void
NodeCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode, icon_size size,
	void (*blitFunc)(BView*, BPoint, BBitmap*, void*), void* passThruState)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (!bitmap)
		return;

//	if (bitmap->ColorSpace() == B_RGBA32) {
//		view->SetDrawingMode(B_OP_ALPHA);
//		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
//	} else {
//		view->SetDrawingMode(B_OP_OVER);
//	}

	(blitFunc)(view, where, bitmap, passThruState);
}


const node_ref*
NodeCacheEntry::Node() const
{
	return &fRef;
}


uint32
NodeCacheEntry::Hash() const
{
	return Hash(&fRef);
}


uint32
NodeCacheEntry::Hash(const node_ref* node)
{
	return node->device ^ ((uint32*)&node->node)[0]
		^ ((uint32*)&node->node)[1];
}


bool
NodeCacheEntry::operator==(const NodeCacheEntry &entry) const
{
	return fRef == entry.fRef;
}


void
NodeCacheEntry::SetTo(const node_ref* node)
{
	fRef = *node;
}


bool
NodeCacheEntry::Permanent() const
{
	return fPermanent;
}


void
NodeCacheEntry::MakePermanent()
{
	fPermanent = true;
}


//	#pragma mark -


NodeIconCache::NodeIconCache()
#if DEBUG
	:	SimpleIconCache("Node Icon cache aka \"The Dead-Locker\""),
		fHashTable(20),
		fElementArray(20)
#else
	:	SimpleIconCache("Tracker node icon cache"),
		fHashTable(100),
		fElementArray(100)
#endif
{
	fHashTable.SetElementVector(&fElementArray);
}


void
NodeIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size, bool async)
{

	((NodeCacheEntry*)entry)->Draw(view, where, mode, size, async);
}


void
NodeIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size, void (*blitFunc)(BView*, BPoint,
	BBitmap*, void*), void* passThruState)
{
	((NodeCacheEntry*)entry)->Draw(view, where, mode, size,
		blitFunc, passThruState);
}


NodeCacheEntry*
NodeIconCache::FindItem(const node_ref* node) const
{
	NodeCacheEntry* result = fHashTable.FindFirst(NodeCacheEntry::Hash(node));

	if (!result)
		return NULL;

	for(;;) {
		if (*result->Node() == *node)
			return result;

		if (result->fNext < 0)
			break;

		result = const_cast<NodeCacheEntry*>(&fElementArray.At(result->fNext));
	}

	return NULL;
}


NodeCacheEntry*
NodeIconCache::AddItem(const node_ref* node, bool permanent)
{
	NodeCacheEntry* result = fHashTable.Add(NodeCacheEntry::Hash(node));
	result->SetTo(node);
	if (permanent)
		result->MakePermanent();

	return result;
}


NodeCacheEntry*
NodeIconCache::AddItem(NodeCacheEntry** outstandingEntry, const node_ref* node)
{
	int32 entryToken = fHashTable.ElementIndex(*outstandingEntry);

	NodeCacheEntry* result = fHashTable.Add(NodeCacheEntry::Hash(node));
	result->SetTo(node);
	*outstandingEntry = fHashTable.ElementAt(entryToken);

	return result;
}


void
NodeIconCache::Deleting(const node_ref* node)
{
	NodeCacheEntry* entry = FindItem(node);
	ASSERT(entry);
	if (!entry || entry->Permanent())
		return;

	fHashTable.Remove(entry);
}


void
NodeIconCache::Removing(const node_ref* node)
{
	NodeCacheEntry* entry = FindItem(node);
	ASSERT(entry);
	if (!entry)
		return;

	fHashTable.Remove(entry);
}


void
NodeIconCache::Deleting(const BView*)
{
#ifdef NODE_CACHE_ASYNC_DRAWS
	TRESPASS();
#endif
}


void
NodeIconCache::IconChanged(const Model* model)
{
	Deleting(model->NodeRef());
}


void
NodeIconCache::RemoveAliasesTo(int32 aliasIndex)
{
	int32 count = fHashTable.VectorSize();
	for (int32 index = 0; index < count; index++) {
		NodeCacheEntry* entry = fHashTable.ElementAt(index);
		if (entry->fAliasForIndex == aliasIndex)
			fHashTable.Remove(entry);
	}
}


//	#pragma mark -


NodeCacheEntryArray::NodeCacheEntryArray(int32 initialSize)
	:	OpenHashElementArray<NodeCacheEntry>(initialSize)
{
}


NodeCacheEntry*
NodeCacheEntryArray::Add()
{
	return OpenHashElementArray<NodeCacheEntry>::Add();
}


//	#pragma mark -


SimpleIconCache::SimpleIconCache(const char* name)
	:	fLock(name)
{
}


void
SimpleIconCache::Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode ,
	icon_size , bool )
{
	TRESPASS();
	// pure virtual, do nothing
}


void
SimpleIconCache::Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode, icon_size,
	void(*)(BView*, BPoint, BBitmap*, void*), void*)
{
	TRESPASS();
	// pure virtual, do nothing
}


bool
SimpleIconCache::Lock()
{
	return fLock.Lock();
}


void
SimpleIconCache::Unlock()
{
	fLock.Unlock();
}


bool
SimpleIconCache::IsLocked() const
{
	return fLock.IsLocked();
}


//	#pragma mark -


LazyBitmapAllocator::LazyBitmapAllocator(icon_size size, color_space colorSpace,
	bool preallocate)
	:	fBitmap(NULL),
		fSize(size),
		fColorSpace(colorSpace)
{
	if (preallocate)
		Get();
}


LazyBitmapAllocator::~LazyBitmapAllocator()
{
	delete fBitmap;
}


BBitmap*
LazyBitmapAllocator::Get()
{
	if (!fBitmap)
		fBitmap = new BBitmap(BRect(0, 0, fSize - 1, fSize - 1), fColorSpace);

	return fBitmap;
}


BBitmap*
LazyBitmapAllocator::Adopt()
{
	if (!fBitmap)
		Get();

	BBitmap* result = fBitmap;
	fBitmap = NULL;
	return result;
}


IconCache* IconCache::sIconCache;
