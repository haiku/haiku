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


//#if DEBUG
//#	define LOG_DISK_HITS
//	the LOG_DISK_HITS define is used to check that the disk is not hit more
//	than needed - enable it, open a window with a bunch of poses, force
//	it to redraw, shouldn't recache
//#	define LOG_ADD_ITEM
//#endif

// set up a few printing macros to get rid of a ton of debugging ifdefs
// in the code
#ifdef LOG_DISK_HITS
#	define PRINT_DISK_HITS(ARGS) _debugPrintf ARGS
#else
#	define PRINT_DISK_HITS(ARGS) (void)0
#endif

#ifdef LOG_ADD_ITEM
#	define PRINT_ADD_ITEM(ARGS) _debugPrintf ARGS
#else
#	define PRINT_ADD_ITEM(ARGS) (void)0
#endif

#undef NODE_CACHE_ASYNC_DRAWS


IconCacheEntry::IconCacheEntry()
	:
	fLargeIcon(NULL),
	fHighlightedLargeIcon(NULL),
	fMiniIcon(NULL),
	fHighlightedMiniIcon(NULL),
	fAliasTo(NULL)
{
}


IconCacheEntry::~IconCacheEntry()
{
	if (fAliasTo == NULL) {
		delete fLargeIcon;
		delete fHighlightedLargeIcon;
		delete fMiniIcon;
		delete fHighlightedMiniIcon;

		// clean up a bit to leave the hash table entry in an initialized state
		fLargeIcon = NULL;
		fHighlightedLargeIcon = NULL;
		fMiniIcon = NULL;
		fHighlightedMiniIcon = NULL;
	}
	fAliasTo = NULL;
}


void
IconCacheEntry::SetAliasFor(const SharedIconCache* sharedCache,
	const SharedCacheEntry* entry)
{
	sharedCache->SetAliasFor(this, entry);
	ASSERT(fAliasTo != NULL);
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
	if (entry == NULL)
		return NULL;

	return sharedCache->ResolveIfAlias(entry);
}


bool
IconCacheEntry::CanConstructBitmap(IconDrawMode mode, icon_size) const
{
	if (mode == kSelected) {
		// for now only
		return true;
	}

	return false;
}


bool
IconCacheEntry::HaveIconBitmap(IconDrawMode mode, icon_size size) const
{
	ASSERT(mode == kSelected || mode == kNormalIcon);
		// for now only

	if (mode == kNormalIcon) {
		return size == B_MINI_ICON ? fMiniIcon != NULL
			: fLargeIcon != NULL
				&& fLargeIcon->Bounds().IntegerWidth() + 1 == size;
	} else if (mode == kSelected) {
		return size == B_MINI_ICON ? fHighlightedMiniIcon != NULL
			: fHighlightedLargeIcon != NULL
				&& fHighlightedLargeIcon->Bounds().IntegerWidth() + 1 == size;
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
			return fHighlightedMiniIcon;
		else
			return fHighlightedLargeIcon;
	}

	return NULL;
}


bool
IconCacheEntry::IconHitTest(BPoint where, IconDrawMode mode,
	icon_size size) const
{
	ASSERT(where.x < size && where.y < size);
	BBitmap* bitmap = IconForMode(mode, size);
	if (bitmap == NULL)
		return false;

	uchar* bits = (uchar*)bitmap->Bits();
	ASSERT(bits != NULL);

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
IconCacheEntry::ConstructBitmap(BBitmap* constructFrom,
	IconDrawMode requestedMode, IconDrawMode constructFromMode,
	icon_size size, LazyBitmapAllocator* lazyBitmap)
{
	ASSERT(requestedMode == kSelected && constructFromMode == kNormalIcon);
		// for now

	if (requestedMode == kSelected && constructFromMode == kNormalIcon) {
		return IconCache::sIconCache->MakeSelectedIcon(constructFrom, size,
			lazyBitmap);
	}

	return NULL;
}


BBitmap*
IconCacheEntry::ConstructBitmap(IconDrawMode requestedMode, icon_size size,
	LazyBitmapAllocator* lazyBitmap)
{
	BBitmap* source = (size == B_MINI_ICON) ? fMiniIcon : fLargeIcon;
	ASSERT(source != NULL);

	return ConstructBitmap(source, requestedMode, kNormalIcon, size,
		lazyBitmap);
}


bool
IconCacheEntry::AlternateModeForIconConstructing(IconDrawMode requestedMode,
	IconDrawMode &alternate, icon_size)
{
	if ((requestedMode & kSelected) != 0) {
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
			fHighlightedMiniIcon = bitmap;
		else
			fHighlightedLargeIcon = bitmap;
	} else
		TRESPASS();
}


IconCache::IconCache()
	:
	fInitHighlightTable(true)
{
	InitHighlightTable();
}


// The following calls use the icon lookup sequence node-prefered app for
// node-metamime-preferred app for metamime to find an icon;
// if we are trying to get a specialized icon, we will first look for a normal
// icon in each of the locations, if we get a hit, we look for the
// specialized, if we don't find one, we try to auto-construct one, if we
// can't we assume the icon is not available for now the code only looks for
// normal icons, selected icons are auto-generated.


IconCacheEntry*
IconCache::GetIconForPreferredApp(const char* fileTypeSignature,
	const char* preferredApp, IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry)
{
	ASSERT(fSharedCache.IsLocked());

	if (preferredApp == NULL || *preferredApp == '\0')
		return NULL;

	if (entry == NULL) {
		entry = fSharedCache.FindItem(fileTypeSignature, preferredApp);
		if (entry != NULL) {
			entry = entry->ResolveIfAlias(&fSharedCache, entry);
#if xDEBUG
			PRINT(("File %s; Line %d # looking for %s, type %s, found %x\n",
				__FILE__, __LINE__, preferredApp, fileTypeSignature, entry));
#endif
			if (entry->HaveIconBitmap(mode, size))
				return entry;
		}
	}

	if (entry == NULL || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		PRINT_DISK_HITS(
			("File %s; Line %d # hitting disk for preferredApp %s, type %s\n",
			__FILE__, __LINE__, preferredApp, fileTypeSignature));

		BMimeType preferredAppType(preferredApp);
		BString signature(fileTypeSignature);
		signature.ToLower();
		if (preferredAppType.GetIconForType(signature.String(),
				lazyBitmap->Get(), size) != B_OK) {
			return NULL;
		}

		BBitmap* bitmap = lazyBitmap->Adopt();
		if (entry == NULL) {
			PRINT_ADD_ITEM(
				("File %s; Line %d # adding entry for preferredApp %s, "
				 "type %s\n", __FILE__, __LINE__, preferredApp,
				fileTypeSignature));
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

	if (entry == NULL)
		entry = fSharedCache.FindItem(fileType);

	if (entry != NULL) {
		entry = entry->ResolveIfAlias(&fSharedCache, entry);
		// metamime defines an icon and we have it cached
		if (entry->HaveIconBitmap(mode, size))
			return entry;
	}

	if (entry == NULL || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
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
			if (entry != NULL) {
				aliasTo
					= (SharedCacheEntry*)entry->ResolveIfAlias(&fSharedCache);
			}

			// look for icon defined by preferred app from metamime
			aliasTo = (SharedCacheEntry*)GetIconForPreferredApp(fileType,
				preferredAppSig, mode, size, lazyBitmap, aliasTo);

			if (aliasTo == NULL)
				return NULL;

			// make an aliased entry so that the next time we get a
			// hit on the first FindItem in here
			if (entry == NULL) {
				PRINT_ADD_ITEM(
					("File %s; Line %d # adding entry as alias for type %s\n",
					__FILE__, __LINE__, fileType));
				entry = fSharedCache.AddItem(fileType);
				entry->SetAliasFor(&fSharedCache, aliasTo);
			}
			ASSERT(aliasTo->HaveIconBitmap(mode, size));
			return aliasTo;
		}

		// at this point, we've found an icon for the MIME type
		BBitmap* bitmap = lazyBitmap->Adopt();
		if (entry == NULL) {
			PRINT_ADD_ITEM(("File %s; Line %d # adding entry for type %s\n",
				__FILE__, __LINE__, fileType));
			entry = fSharedCache.AddItem(fileType);
		}
		entry->SetIcon(bitmap, kNormalIcon, size);
	}

	ASSERT(entry != NULL);

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
			if (entry != NULL) {
				source = kPreferredAppForNode;
				ASSERT(entry->HaveIconBitmap(mode, size));

				return entry;
			}
		}
		if (source == kPreferredAppForNode)
			source = kUnknownSource;
	}

	entry = GetIconFromMetaMime(fileType, mode, size, lazyBitmap, entry);
	if (entry == NULL) {
		// Try getting a supertype handler icon
		BMimeType mime(fileType);
		if (!mime.IsSupertypeOnly()) {
			BMimeType superType;
			mime.GetSupertype(&superType);
			const char* superTypeFileType = superType.Type();
			if (superTypeFileType != NULL) {
				entry = GetIconFromMetaMime(superTypeFileType, mode, size,
					lazyBitmap, entry);
			}
#if DEBUG
			else {
				PRINT(("File %s; Line %d # failed to get supertype for "
					"type %s\n", __FILE__, __LINE__, fileType));
			}
#endif
		}
	}

	ASSERT(entry == NULL || entry->HaveIconBitmap(mode, size));
	if (entry != NULL) {
		if (nodePreferredApp != NULL && *nodePreferredApp != '\0') {
			// we got a miss using GetIconForPreferredApp before, cache this
			// fileType/preferredApp combo with an aliased entry

			// make an aliased entry so that the next time we get a
			// hit and substitute a generic icon right away

			PRINT_ADD_ITEM(
				("File %s; Line %d # adding entry as alias for "
				 "preferredApp %s, type %s\n",
				__FILE__, __LINE__, nodePreferredApp, fileType));
			IconCacheEntry* aliasedEntry
				= fSharedCache.AddItem(fileType, nodePreferredApp);
			aliasedEntry->SetAliasFor(&fSharedCache,
				(SharedCacheEntry*)entry);
				// OK to cast here, have a runtime check
			source = kPreferredAppForNode;
				// set source as preferred for node, so that next time we
				// get a hit in the initial find that uses
				// GetIconForPreferredApp
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
		if (entry != NULL) {
			entry = IconCacheEntry::ResolveIfAlias(&fSharedCache, entry);

			if (source == kTrackerDefault) {
				// if tracker default, resolved entry is from shared cache
				// this could be done a little cleaner if entry had a way to
				// reach the cache it is in
				*resultingOpenCache = sharedCacheLocker;
				sharedCacheLocker->Lock();
			}

			if (entry->HaveIconBitmap(mode, size))
				return entry;
		}
	}

	// try getting using the BVolume::GetIcon call; if miss,
	// go for the default mime based icon
	if (entry == NULL || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		BVolume volume(model->NodeRef()->device);

		if (volume.IsShared()) {
			// check if it's a network share and give it a special icon
			BBitmap* bitmap = lazyBitmap->Get();
			GetTrackerResources()->GetIconResource(R_ShareIcon, size, bitmap);
			if (entry == NULL) {
				PRINT_ADD_ITEM(
					("File %s; Line %d # adding entry for model %s\n",
					__FILE__, __LINE__, model->Name()));
				entry = fNodeCache.AddItem(model->NodeRef());
			}
			entry->SetIcon(lazyBitmap->Adopt(), kNormalIcon, size);
		} else if (volume.GetIcon(lazyBitmap->Get(), size) == B_OK) {
			// ask the device for an icon
			BBitmap* bitmap = lazyBitmap->Adopt();
			ASSERT(bitmap != NULL);
			if (entry == NULL) {
				PRINT_ADD_ITEM(
					("File %s; Line %d # adding entry for model %s\n",
					__FILE__, __LINE__, model->Name()));
				entry = fNodeCache.AddItem(model->NodeRef());
			}
			ASSERT(entry != NULL);
			entry->SetIcon(bitmap, kNormalIcon, size);
			source = kVolume;
		} else {
			*resultingOpenCache = sharedCacheLocker;
			sharedCacheLocker->Lock();

			// if the volume doesnt have a device it gets the generic icon
			entry = GetIconFromMetaMime(B_VOLUME_MIMETYPE, mode,
				size, lazyBitmap, entry);
		}
	}

	if (mode != kNormalIcon && entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
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
	if (wellKnownEntry == NULL)
		return NULL;

	BString type("tracker/active_");
	type += wellKnownEntry->name;

	*resultingOpenCache = sharedCacheLocker;
	(*resultingOpenCache)->Lock();

	source = kTrackerSupplied;

	IconCacheEntry* entry = fSharedCache.FindItem(type.String());
	if (entry != NULL) {
		entry = entry->ResolveIfAlias(&fSharedCache, entry);
		if (entry->HaveIconBitmap(mode, size))
			return entry;
	}

	if (entry == NULL || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		// match up well known entries in the file system with specialized
		// icons stored in Tracker's resources
		int32 resourceId = -1;
		switch ((uint32)wellKnownEntry->which) {
			case B_BOOT_DISK:
				resourceId = R_BootVolumeIcon;
				break;

			case B_BEOS_DIRECTORY:
				resourceId = R_BeosFolderIcon;
				break;

			case B_USER_DIRECTORY:
				resourceId = R_HomeDirIcon;
				break;

			case B_SYSTEM_FONTS_DIRECTORY:
			case B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY:
			case B_USER_FONTS_DIRECTORY:
			case B_USER_NONPACKAGED_FONTS_DIRECTORY:
				resourceId = R_FontDirIcon;
				break;

			case B_BEOS_APPS_DIRECTORY:
			case B_APPS_DIRECTORY:
			case B_USER_DESKBAR_APPS_DIRECTORY:
				resourceId = R_AppsDirIcon;
				break;

			case B_BEOS_PREFERENCES_DIRECTORY:
			case B_PREFERENCES_DIRECTORY:
			case B_USER_DESKBAR_PREFERENCES_DIRECTORY:
				resourceId = R_PrefsDirIcon;
				break;

			case B_USER_MAIL_DIRECTORY:
				resourceId = R_MailDirIcon;
				break;

			case B_USER_QUERIES_DIRECTORY:
				resourceId = R_QueryDirIcon;
				break;

			case B_SYSTEM_DEVELOP_DIRECTORY:
			case B_SYSTEM_NONPACKAGED_DEVELOP_DIRECTORY:
			case B_USER_DESKBAR_DEVELOP_DIRECTORY:
				resourceId = R_DevelopDirIcon;
				break;

			case B_USER_CONFIG_DIRECTORY:
				resourceId = R_ConfigDirIcon;
				break;

			case B_USER_PEOPLE_DIRECTORY:
				resourceId = R_PersonDirIcon;
				break;

			case B_USER_DOWNLOADS_DIRECTORY:
				resourceId = R_DownloadDirIcon;
				break;

			default:
				return NULL;
		}

		entry = fSharedCache.AddItem(type.String());

		BBitmap* bitmap = lazyBitmap->Get();
		GetTrackerResources()->GetIconResource(resourceId, size, bitmap);
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
	Model* model, IconSource& source,
	IconDrawMode mode, icon_size size,
	LazyBitmapAllocator* lazyBitmap, IconCacheEntry* entry, bool permanent)
{
	*resultingOpenCache = nodeCacheLocker;
	(*resultingOpenCache)->Lock();

	entry = fNodeCache.FindItem(model->NodeRef());
	if (entry == NULL || !entry->HaveIconBitmap(NORMAL_ICON_ONLY, size)) {
		modelOpener->OpenNode();

		BFile* file = NULL;

		// if we are dealing with an application, use the BAppFileInfo
		// superset of node; this makes GetIcon grab the proper icon for
		// an app
		if (model->IsExecutable())
			file = dynamic_cast<BFile*>(model->Node());

		PRINT_DISK_HITS(("File %s; Line %d # hitting disk for node %s\n",
			__FILE__, __LINE__, model->Name()));

		status_t result = file != NULL
			? GetAppIconFromAttr(file, lazyBitmap->Get(), size)
			: GetFileIconFromAttr(model->Node(), lazyBitmap->Get(), size);

		if (result == B_OK) {
			// node has its own icon, use it
			BBitmap* bitmap = lazyBitmap->Adopt();
			PRINT_ADD_ITEM(("File %s; Line %d # adding entry for model %s\n",
				__FILE__, __LINE__, model->Name()));
			entry = fNodeCache.AddItem(model->NodeRef(), permanent);
			ASSERT(entry != NULL);
			entry->SetIcon(bitmap, kNormalIcon, size);
			if (mode != kNormalIcon) {
				entry->ConstructBitmap(mode, size, lazyBitmap);
				entry->SetIcon(lazyBitmap->Adopt(), mode, size);
			}
			source = kNode;
		}
	}

	if (entry == NULL) {
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

	entry = GetIconFromMetaMime(B_FILE_MIMETYPE, mode, size, lazyBitmap, 0);
	if (entry == NULL)
		return NULL;

	// make an aliased entry so that the next time we get a
	// hit and substitute a generic icon right away
	PRINT_ADD_ITEM(
		("File %s; Line %d # adding entry for preferredApp %s, type %s\n",
		__FILE__, __LINE__, model->PreferredAppSignature(),
		model->MimeType()));
	IconCacheEntry* aliasedEntry = fSharedCache.AddItem(
		model->MimeType(), model->PreferredAppSignature());
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

	{
		// scope for modelOpener

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
				if (entry == NULL || !entry->HaveIconBitmap(mode, size)) {
					// look for volume defined icon
					entry = GetVolumeIcon(nodeCacheLocker, sharedCacheLocker,
						&resultingOpenCache, model, source, mode,
						size, &lazyBitmap);
				}
			} else if (model->IsRoot()) {
				entry = GetRootIcon(nodeCacheLocker, sharedCacheLocker,
					&resultingOpenCache, model, source, mode, size,
						&lazyBitmap);
				ASSERT(entry != NULL);
			} else {
				if (source == kUnknownSource) {
					// look for node icons first
					entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
						&resultingOpenCache, model, source,
						mode, size, &lazyBitmap, entry, permanent);
				}

				if (entry == NULL) {
					// no node icon, look for file type based one
					modelOpener.OpenNode();
					// use file types to get the icon
					resultingOpenCache = sharedCacheLocker;
					resultingOpenCache->Lock();

					entry = GetIconFromFileTypes(&modelOpener, source, mode,
						size, &lazyBitmap, 0);
					if (entry == NULL) {
						// we don't have an icon, go with the generic
						entry = GetGenericIcon(sharedCacheLocker,
							&resultingOpenCache, model, source, mode,
							size, &lazyBitmap, entry);
					}
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
					if (entry != NULL) {
						entry = IconCacheEntry::ResolveIfAlias(&fSharedCache,
							entry);
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
						entry = GetWellKnownIcon(nodeCacheLocker,
							sharedCacheLocker, &resultingOpenCache, model,
							source, mode, size, &lazyBitmap);
						if (entry != NULL)
							break;
					}
					// fall through
				case kTrackerDefault:
				case kVolume:
					if (model->IsVolume()) {
						entry = GetNodeIcon(&modelOpener, nodeCacheLocker,
							&resultingOpenCache, model, source,
							mode, size, &lazyBitmap, entry, permanent);
						if (entry == NULL
							|| !entry->HaveIconBitmap(mode, size)) {
							entry = GetVolumeIcon(nodeCacheLocker,
								sharedCacheLocker, &resultingOpenCache, model,
								source, mode, size, &lazyBitmap);
						}
						break;
					}
					// fall through
				case kMetaMime:
				case kPreferredAppForType:
				case kPreferredAppForNode:
					resultingOpenCache = sharedCacheLocker;
					resultingOpenCache->Lock();

					entry = GetIconFromFileTypes(&modelOpener, source, mode,
						size, &lazyBitmap, 0);

					if (entry == NULL || !entry->HaveIconBitmap(mode, size)) {
						// we don't have an icon, go with the generic
						entry = GetGenericIcon(sharedCacheLocker,
							&resultingOpenCache, model, source, mode, size,
							&lazyBitmap, entry);
					}

					model->SetIconFrom(source);
						// The source shouldn't change in this case; if it does
						// though we might never be hitting the correct icon and
						// instead keep leaking entries after each miss this now
						// happens if an app defines an icon but a
						// GetIconForType() fails and we fall back to generic
						// icon.
						// ToDo: fix this and add an assert to the effect

					ASSERT(entry != NULL);
					ASSERT(entry->HaveIconBitmap(mode, size));
					break;

				default:
					TRESPASS();
					break;
			}
		}

		if (entry == NULL || !entry->HaveIconBitmap(mode, size)) {
			// we don't have an icon, go with the generic
			PRINT(
				("icon cache complete miss, falling back on generic icon "
				 "for %s\n", model->Name()));
			entry = GetGenericIcon(sharedCacheLocker, &resultingOpenCache,
				model, source, mode, size, &lazyBitmap, entry);

			// we don't even have generic, something is really broken,
			// go with hardcoded generic icon
			if (entry == NULL || !entry->HaveIconBitmap(mode, size)) {
				PRINT(
				("icon cache complete miss, falling back on generic "
				 "icon for %s\n", model->Name()));
				entry = GetFallbackIcon(sharedCacheLocker,
					&resultingOpenCache, model, mode, size, &lazyBitmap,
					entry);
			}

			// force icon pick up next time around because we probably just
			// hit a node in transition
			model->SetIconFrom(kUnknownSource);
		}
	}

	ASSERT(entry != NULL && entry->HaveIconBitmap(mode, size));

	if (resultingCache != NULL)
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

	if (entry == NULL)
		return;

	ASSERT(entry != NULL);
	ASSERT(entry->HaveIconBitmap(mode, size));
	// got the entry, now draw it
	resultingCacheLocker->LockedItem()->Draw(entry, view, where, mode,
		size, async);

	// either of the two cache lockers that got locked down by this call get
	// unlocked at this point
}


void
IconCache::SyncDraw(Model* model, BView* view, BPoint where,
	IconDrawMode mode, icon_size size,
	void (*blitFunc)(BView*, BPoint, BBitmap*, void*),
	void* passThruState)
{
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	AutoLock<SimpleIconCache>* resultingCacheLocker;
	IconCacheEntry* entry = Preload(&nodeCacheLocker, &sharedCacheLocker,
		&resultingCacheLocker, model, mode, size, false);

	if (entry == NULL)
		return;

	ASSERT(entry != NULL);
	ASSERT(entry->HaveIconBitmap(mode, size));
	resultingCacheLocker->LockedItem()->Draw(entry, view, where,
		mode, size, blitFunc, passThruState);
}


void
IconCache::Preload(Model* model, IconDrawMode mode, icon_size size,
	bool permanent)
{
	AutoLock<SimpleIconCache> nodeCacheLocker(&fNodeCache, false);
	AutoLock<SimpleIconCache> sharedCacheLocker(&fSharedCache, false);

	Preload(&nodeCacheLocker, &sharedCacheLocker, 0, model, mode, size,
		permanent);
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
	if (entry != NULL)
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
	if (entry == NULL)
		return;

	AutoLock<SimpleIconCache> nodeLock(&fNodeCache);

	entry = (SharedCacheEntry*)fSharedCache.ResolveIfAlias(entry);
	ASSERT(entry != NULL);

	fNodeCache.RemoveAliasesTo(entry);
	fSharedCache.RemoveAliasesTo(entry);

	fSharedCache.IconChanged(entry);
}


BBitmap*
IconCache::MakeSelectedIcon(const BBitmap* normal, icon_size size,
	LazyBitmapAllocator* lazyBitmap)
{
	return MakeTransformedIcon(normal, size, fHighlightTable, lazyBitmap);
}


#if xDEBUG
static void
DumpBitmap(const BBitmap* bitmap)
{
	if (bitmap == NULL) {
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
IconCache::InitHighlightTable()
{
	// build the color transform tables for different icon modes
	BScreen screen(B_MAIN_SCREEN_ID);
	rgb_color color;
	for (int32 index = 0; index < kColorTransformTableSize; index++) {
		color = screen.ColorForIndex((uchar)index);
		fHighlightTable[index] = screen.IndexForColor(tint_color(color, 1.3f));
	}

	fHighlightTable[B_TRANSPARENT_8_BIT] = B_TRANSPARENT_8_BIT;
	fInitHighlightTable = false;
}


BBitmap*
IconCache::MakeTransformedIcon(const BBitmap* source, icon_size /*size*/,
	int32 colorTransformTable[], LazyBitmapAllocator* lazyBitmap)
{
	if (fInitHighlightTable)
		InitHighlightTable();

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

	if (entry != NULL)
		return entry->IconHitTest(where, mode, size);

	return false;
}


void
IconCacheEntry::RetireIcons(BObjectList<BBitmap>* retiredBitmapList)
{
	if (fLargeIcon != NULL) {
		retiredBitmapList->AddItem(fLargeIcon);
		fLargeIcon = NULL;
	}
	if (fHighlightedLargeIcon != NULL) {
		retiredBitmapList->AddItem(fHighlightedLargeIcon);
		fHighlightedLargeIcon = NULL;
	}
	if (fMiniIcon != NULL) {
		retiredBitmapList->AddItem(fMiniIcon);
		fMiniIcon = NULL;
	}
	if (fHighlightedMiniIcon != NULL) {
		retiredBitmapList->AddItem(fHighlightedMiniIcon);
		fHighlightedMiniIcon = NULL;
	}

	int32 count = retiredBitmapList->CountItems();
	if (count > 10 * 1024) {
		PRINT(("nuking old icons from the retired bitmap list\n"));
		for (count = 512; count > 0; count--)
			delete retiredBitmapList->RemoveItemAt(0);
	}
}


//	#pragma mark - SharedIconCache


SharedIconCache::SharedIconCache()
	:
	SimpleIconCache("Tracker shared icon cache"),
	fHashTable(),
	fRetiredBitmaps(256, true)
{
	fHashTable.Init(256);
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
SharedIconCache::FindItem(const char* fileType,
	const char* appSignature) const
{
	ASSERT(fileType);
	if (!fileType)
		fileType = B_FILE_MIMETYPE;

	return fHashTable.Lookup(SharedCacheEntry::TypeAndSignature(fileType,
		appSignature));
}


SharedCacheEntry*
SharedIconCache::AddItem(const char* fileType, const char* appSignature)
{
	ASSERT(fileType != NULL);
	if (fileType == NULL)
		fileType = B_FILE_MIMETYPE;

	SharedCacheEntry* entry = new SharedCacheEntry(fileType, appSignature);
	if (fHashTable.Insert(entry) == B_OK)
		return entry;

	delete entry;
	return NULL;
}


void
SharedIconCache::IconChanged(SharedCacheEntry* entry)
{
	// by now there should be no aliases to entry, just remove entry
	// itself
	ASSERT(entry->fAliasTo == NULL);
	entry->RetireIcons(&fRetiredBitmaps);
	fHashTable.Remove(entry);
}


void
SharedIconCache::RemoveAliasesTo(SharedCacheEntry* alias)
{
	EntryHashTable::Iterator it = fHashTable.GetIterator();
	while (it.HasNext()) {
		SharedCacheEntry* entry = it.Next();
		if (entry->fAliasTo == alias)
			fHashTable.RemoveUnchecked(entry);
	}
}


void
SharedIconCache::SetAliasFor(IconCacheEntry* entry,
	const SharedCacheEntry* original) const
{
	entry->fAliasTo = original;
}


SharedCacheEntry::SharedCacheEntry()
	:
	fNext(NULL)
{
}


SharedCacheEntry::SharedCacheEntry(const char* fileType,
	const char* appSignature)
	:
	fNext(NULL),
	fFileType(fileType),
	fAppSignature(appSignature)
{
}


void
SharedCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode,
	icon_size size, bool async)
{
	BBitmap* bitmap = IconForMode(mode, size);
	ASSERT(bitmap != NULL);

	drawing_mode oldMode = view->DrawingMode();

	if (bitmap->ColorSpace() == B_RGBA32) {
		if (oldMode != B_OP_ALPHA) {
			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
	} else
		view->SetDrawingMode(B_OP_OVER);

	if (async)
		view->DrawBitmapAsync(bitmap, where);
	else
		view->DrawBitmap(bitmap, where);

	view->SetDrawingMode(oldMode);
}


void
SharedCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode,
	icon_size size, void (*blitFunc)(BView*, BPoint, BBitmap*, void*),
	void* passThruState)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (bitmap == NULL)
		return;

	(blitFunc)(view, where, bitmap, passThruState);
}


/* static */ size_t
SharedCacheEntry::Hash(const TypeAndSignature& typeAndSignature)
{
	size_t hash = HashString(typeAndSignature.type, 0);
	if (typeAndSignature.signature != NULL
			&& *typeAndSignature.signature != '\0')
		hash = HashString(typeAndSignature.signature, hash);

	return hash;
}


size_t
SharedCacheEntry::Hash() const
{
	return Hash(TypeAndSignature(fFileType.String(), fAppSignature.String()));
}


bool
SharedCacheEntry::operator==(const TypeAndSignature& typeAndSignature) const
{
	return fFileType == typeAndSignature.type
		&& fAppSignature == typeAndSignature.signature;
}


//	#pragma mark - NodeCacheEntry


NodeCacheEntry::NodeCacheEntry(bool permanent)
	:
	fNext(NULL),
	fPermanent(permanent)
{
}


NodeCacheEntry::NodeCacheEntry(const node_ref* node, bool permanent)
	:
	fNext(NULL),
	fRef(*node),
	fPermanent(permanent)
{
}


void
NodeCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode,
	icon_size size, bool async)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (bitmap == NULL)
		return;

	drawing_mode oldMode = view->DrawingMode();

	if (bitmap->ColorSpace() == B_RGBA32) {
		if (oldMode != B_OP_ALPHA) {
			view->SetDrawingMode(B_OP_ALPHA);
			view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}
	} else
		view->SetDrawingMode(B_OP_OVER);

	if (false && async) {
		TRESPASS();
		// need to copy the bits first in here
		view->DrawBitmapAsync(bitmap, where);
	} else
		view->DrawBitmap(bitmap, where);

	view->SetDrawingMode(oldMode);
}


void
NodeCacheEntry::Draw(BView* view, BPoint where, IconDrawMode mode,
	icon_size size, void (*blitFunc)(BView*, BPoint, BBitmap*, void*),
	void* passThruState)
{
	BBitmap* bitmap = IconForMode(mode, size);
	if (bitmap == NULL)
		return;

	(blitFunc)(view, where, bitmap, passThruState);
}


const node_ref*
NodeCacheEntry::Node() const
{
	return &fRef;
}


size_t
NodeCacheEntry::Hash() const
{
	return Hash(&fRef);
}


/* static */ size_t
NodeCacheEntry::Hash(const node_ref* node)
{
	return node->device ^ ((uint32*)&node->node)[0]
		^ ((uint32*)&node->node)[1];
}


bool
NodeCacheEntry::operator==(const node_ref* node) const
{
	return fRef == *node;
}


bool
NodeCacheEntry::Permanent() const
{
	return fPermanent;
}


//	#pragma mark - NodeIconCache


NodeIconCache::NodeIconCache()
	:
	SimpleIconCache("Tracker node icon cache")
{
	fHashTable.Init(100);
}


void
NodeIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size, bool async)
{
	((NodeCacheEntry*)entry)->Draw(view, where, mode, size, async);
}


void
NodeIconCache::Draw(IconCacheEntry* entry, BView* view, BPoint where,
	IconDrawMode mode, icon_size size,
	void (*blitFunc)(BView*, BPoint, BBitmap*, void*), void* passThruState)
{
	((NodeCacheEntry*)entry)->Draw(view, where, mode, size,
		blitFunc, passThruState);
}


NodeCacheEntry*
NodeIconCache::FindItem(const node_ref* node) const
{
	return fHashTable.Lookup(node);
}


NodeCacheEntry*
NodeIconCache::AddItem(const node_ref* node, bool permanent)
{
	NodeCacheEntry* entry = new NodeCacheEntry(node, permanent);
	if (fHashTable.Insert(entry) == B_OK)
		return entry;

	delete entry;
	return NULL;
}


void
NodeIconCache::Deleting(const node_ref* node)
{
	NodeCacheEntry* entry = FindItem(node);
	ASSERT(entry != NULL);
	if (entry == NULL || entry->Permanent())
		return;

	fHashTable.Remove(entry);
}


void
NodeIconCache::Removing(const node_ref* node)
{
	NodeCacheEntry* entry = FindItem(node);
	ASSERT(entry != NULL);
	if (entry == NULL)
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
NodeIconCache::RemoveAliasesTo(SharedCacheEntry* alias)
{
	EntryHashTable::Iterator it = fHashTable.GetIterator();
	while (it.HasNext()) {
		NodeCacheEntry* entry = it.Next();
		if (entry->fAliasTo == alias)
			fHashTable.RemoveUnchecked(entry);
	}
}


//	#pragma mark - SimpleIconCache


SimpleIconCache::SimpleIconCache(const char* name)
	:
	fLock(name)
{
}


void
SimpleIconCache::Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
	icon_size, bool)
{
	TRESPASS();
	// pure virtual, do nothing
}


void
SimpleIconCache::Draw(IconCacheEntry*, BView*, BPoint, IconDrawMode,
	icon_size, void(*)(BView*, BPoint, BBitmap*, void*), void*)
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


//	#pragma mark - LazyBitmapAllocator


LazyBitmapAllocator::LazyBitmapAllocator(icon_size size,
	color_space colorSpace, bool preallocate)
	:
	fBitmap(NULL),
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
	if (fBitmap == NULL)
		fBitmap = new BBitmap(BRect(0, 0, fSize - 1, fSize - 1), fColorSpace);

	return fBitmap;
}


BBitmap*
LazyBitmapAllocator::Adopt()
{
	if (fBitmap == NULL)
		Get();

	BBitmap* bitmap = fBitmap;
	fBitmap = NULL;

	return bitmap;
}


IconCache* IconCache::sIconCache;
