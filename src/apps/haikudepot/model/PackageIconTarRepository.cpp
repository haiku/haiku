/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageIconTarRepository.h"

#include <Autolock.h>
#include <AutoDeleter.h>
#include <File.h>
#include <support/StopWatch.h>

#include "Logger.h"
#include "TarArchiveService.h"


#define LIMIT_ICON_CACHE 50


BitmapRef
PackageIconTarRepository::sDefaultIcon(new(std::nothrow) SharedBitmap(
	"application/x-vnd.haiku-package"), true);


/*!	An instance of this class can be provided to the TarArchiveService to
	be called each time a tar entry is found.  This way it is able to capture
	the offsets of the icons in the tar file against the package names.
*/

class IconTarPtrEntryListener : public TarEntryListener
{
public:
								IconTarPtrEntryListener(
									PackageIconTarRepository*
									fPackageIconTarRepository);
	virtual						~IconTarPtrEntryListener();

	virtual status_t			Handle(
									const TarArchiveHeader& header,
									size_t offset,
									BDataIO *data);
private:
			status_t			_LeafNameToBitmapSize(BString& leafName,
									BitmapSize* bitmapSize);

private:
			PackageIconTarRepository*
								fPackageIconTarRepository;
};


IconTarPtrEntryListener::IconTarPtrEntryListener(
	PackageIconTarRepository* packageIconTarRepository)
	:
	fPackageIconTarRepository(packageIconTarRepository)
{
}


IconTarPtrEntryListener::~IconTarPtrEntryListener()
{
}


/*!	The format of the filenames in the archive are of the format;
	\code
	hicn/ardino/icon.hvif
	\endcode
	The leafname (the last part of the path) determines the type of the file as
	it could be either in HVIF format or a PNG of various sizes.
*/

status_t
IconTarPtrEntryListener::Handle(const TarArchiveHeader& header,
	size_t offset, BDataIO *data)
{
	if (header.FileType() != TAR_FILE_TYPE_NORMAL)
		return B_OK;

	BString fileName = header.FileName();
	if (!fileName.StartsWith("hicn/"))
		return B_OK;

	int32 secondSlashIdx = fileName.FindFirst("/", 5);
	if (secondSlashIdx == B_ERROR || secondSlashIdx == 5)
		return B_OK;

	BString packageName;
	BString leafName;
	fileName.CopyInto(packageName, 5, secondSlashIdx - 5);
	fileName.CopyInto(leafName, secondSlashIdx + 1,
		fileName.Length() - (secondSlashIdx + 1));
	BitmapSize bitmapSize;

	if (_LeafNameToBitmapSize(leafName, &bitmapSize) == B_OK) {
		fPackageIconTarRepository->AddIconTarPtr(packageName, bitmapSize,
			offset);
	}

	return B_OK;
}


status_t
IconTarPtrEntryListener::_LeafNameToBitmapSize(BString& leafName,
	BitmapSize* bitmapSize)
{
	if (leafName == "icon.hvif") {
		*bitmapSize = BITMAP_SIZE_ANY;
		return B_OK;
	}
	if (leafName == "64.png") {
		*bitmapSize = BITMAP_SIZE_64;
		return B_OK;
	}
	if (leafName == "32.png") {
		*bitmapSize = BITMAP_SIZE_32;
		return B_OK;
	}
	if (leafName == "16.png") {
		*bitmapSize = BITMAP_SIZE_16;
		return B_OK;
	}
	return B_BAD_VALUE;
}


PackageIconTarRepository::PackageIconTarRepository()
	:
	fTarIo(NULL),
	fIconCache(LIMIT_ICON_CACHE)
{
}


PackageIconTarRepository::~PackageIconTarRepository()
{
}


/*!	This method will reconfigure itself using the data in the tar file supplied.
	Any existing data will be flushed and the new tar will be scanned for
	offsets to usable files.
*/

status_t
PackageIconTarRepository::Init(BPath& tarPath)
{
	BAutolock locker(&fLock);
	_Close();
	status_t result = B_OK;

	if (tarPath.Path() == NULL) {
		HDINFO("empty path to tar-ball");
		result = B_BAD_VALUE;
	}

	BFile *tarIo = NULL;

	if (result == B_OK) {
		HDINFO("will init icon model from tar [%s]", tarPath.Path());
		tarIo = new BFile(tarPath.Path(), O_RDONLY);

		if (!tarIo->IsReadable()) {
			HDERROR("unable to read the tar [%s]", tarPath.Path());
			result = B_IO_ERROR;
		}
	}

	// will fill the model up with records from the tar-ball.

	if (result == B_OK) {
		BStopWatch watch("PackageIconTarRepository::Init", true);
		HDINFO("will read [%s] and collect the tar pointers", tarPath.Path());

		IconTarPtrEntryListener* listener = new IconTarPtrEntryListener(this);
		ObjectDeleter<IconTarPtrEntryListener> listenerDeleter(listener);
		TarArchiveService::ForEachEntry(*tarIo, listener);

		double secs = watch.ElapsedTime() / 1000000.0;
		HDINFO("did collect %" B_PRIi32 " tar pointers (%6.3g secs)",
			fIconTarPtrs.Size(), secs);
	}

	if (result == B_OK)
		fTarIo = tarIo;
	else
		delete tarIo;

	return result;
}


void
PackageIconTarRepository::_Close()
{
	delete fTarIo;
	fTarIo = NULL;
	fIconTarPtrs.Clear();
	fIconCache.Clear();
}


/*!	This method should be treated private and only called from a situation
	in which the class's lock is acquired.  It is used to populate data from
	the parsing of the tar headers.  It is called from the listener above.
*/

void
PackageIconTarRepository::AddIconTarPtr(const BString& packageName,
	BitmapSize bitmapSize, off_t offset)
{
	IconTarPtrRef tarPtrRef = _GetOrCreateIconTarPtr(packageName);
	tarPtrRef->SetOffset(bitmapSize, offset);
}


bool
PackageIconTarRepository::HasAnyIcon(const BString& pkgName)
{
	BAutolock locker(&fLock);
	HashString key(pkgName);
	return fIconTarPtrs.ContainsKey(key);
}


status_t
PackageIconTarRepository::GetIcon(const BString& pkgName, BitmapSize size,
	BitmapRef& bitmap)
{
	BAutolock locker(&fLock);
	status_t result = B_OK;
	BitmapSize actualSize;
	off_t iconDataTarOffset = -1;
	const IconTarPtrRef tarPtrRef = _GetIconTarPtr(pkgName);

	if (tarPtrRef.Get() != NULL) {
		iconDataTarOffset = _OffsetToBestRepresentation(tarPtrRef, size,
			&actualSize);
	}

	if (iconDataTarOffset < 0)
		bitmap.SetTo(sDefaultIcon);
	else {
		HashString key = _ToIconCacheKey(pkgName, actualSize);

		// TODO; need to implement an LRU cache so that not too many icons are
		// in memory at the same time.

		if (!fIconCache.ContainsKey(key)) {
			result = _CreateIconFromTarOffset(iconDataTarOffset, bitmap);
			if (result == B_OK)
				fIconCache.Put(key, bitmap);
			else {
				HDERROR("failure to read image for package [%s] at offset %"
					B_PRIdSSIZE, pkgName.String(), iconDataTarOffset);
				fIconCache.Put(key, sDefaultIcon);
			}
		}
		bitmap.SetTo(fIconCache.Get(key).Get());
	}
	return result;
}


IconTarPtrRef
PackageIconTarRepository::_GetIconTarPtr(const BString& pkgName) const
{
	return fIconTarPtrs.Get(HashString(pkgName));
}


const char*
PackageIconTarRepository::_ToIconCacheKeySuffix(BitmapSize size)
{
	switch (size)
	{
		case BITMAP_SIZE_16:
			return "16";
		// note that size 22 is not supported.
		case BITMAP_SIZE_32:
			return "32";
		case BITMAP_SIZE_64:
			return "64";
		case BITMAP_SIZE_ANY:
			return "any";
		default:
			HDFATAL("unsupported bitmap size");
			break;
	}
}


const HashString
PackageIconTarRepository::_ToIconCacheKey(const BString& pkgName,
	BitmapSize size)
{
	return HashString(BString(pkgName) << "__x" << _ToIconCacheKeySuffix(size));
}


status_t
PackageIconTarRepository::_CreateIconFromTarOffset(off_t offset,
	BitmapRef& bitmap)
{
	fTarIo->Seek(offset, SEEK_SET);
	TarArchiveHeader header;
	status_t result = TarArchiveService::GetEntry(*fTarIo, header);

	if (result == B_OK && header.Length() <= 0)
		result = B_BAD_DATA;

	if (result == B_OK)
		bitmap.SetTo(new(std::nothrow)SharedBitmap(*fTarIo, header.Length()));

	return result;
}


/*!	If there is a vector representation (HVIF) then this will be the best
	option.  If there are only bitmap images to choose from then consider what
	the target size is and choose the best image to match.
*/

/*static*/ off_t
PackageIconTarRepository::_OffsetToBestRepresentation(
	const IconTarPtrRef iconTarPtrRef, BitmapSize desiredSize,
	BitmapSize* actualSize)
{
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_ANY)) {
		*actualSize = BITMAP_SIZE_ANY;
		return iconTarPtrRef->Offset(BITMAP_SIZE_ANY);
	}
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_64)
			&& desiredSize >= BITMAP_SIZE_64) {
		*actualSize = BITMAP_SIZE_64;
		return iconTarPtrRef->Offset(BITMAP_SIZE_64);
	}
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_32)
			&& desiredSize >= BITMAP_SIZE_32) {
		*actualSize = BITMAP_SIZE_32;
		return iconTarPtrRef->Offset(BITMAP_SIZE_32);
	}
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_22)
			&& desiredSize >= BITMAP_SIZE_22) {
		*actualSize = BITMAP_SIZE_22;
		return iconTarPtrRef->Offset(BITMAP_SIZE_22);
	}
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_16)) {
		*actualSize = BITMAP_SIZE_16;
		return iconTarPtrRef->Offset(BITMAP_SIZE_16);
	}
	return -1;
}


IconTarPtrRef
PackageIconTarRepository::_GetOrCreateIconTarPtr(const BString& pkgName)
{
	BAutolock locker(&fLock);
	HashString key(pkgName);
	if (!fIconTarPtrs.ContainsKey(key)) {
		IconTarPtrRef value(new IconTarPtr(pkgName));
		fIconTarPtrs.Put(key, value);
		return value;
	}
	return fIconTarPtrs.Get(key);
}


/*static*/ void
PackageIconTarRepository::CleanupDefaultIcon()
{
	sDefaultIcon.Unset();
}
