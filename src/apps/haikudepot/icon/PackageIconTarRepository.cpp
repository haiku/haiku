/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageIconTarRepository.h"

#include <AutoDeleter.h>
#include <File.h>
#include <IconUtils.h>
#include <StopWatch.h>
#include <TranslationUtils.h>

#include "DataIOUtils.h"
#include "Logger.h"
#include "TarArchiveService.h"


#define LIMIT_ICON_CACHE 50
#define ICON_BUFFER_SIZE_INITIAL 4096
#define ICON_BUFFER_SIZE_MAX 32768


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
	fileName.CopyInto(leafName, secondSlashIdx + 1, fileName.Length() - (secondSlashIdx + 1));
	BitmapSize storedSize;

	if (_LeafNameToBitmapSize(leafName, &storedSize) == B_OK)
		fPackageIconTarRepository->AddIconTarPtr(packageName, storedSize, offset);

	return B_OK;
}


status_t
IconTarPtrEntryListener::_LeafNameToBitmapSize(BString& leafName, BitmapSize* storedSize)
{
	if (leafName == "icon.hvif") {
		*storedSize = BITMAP_SIZE_ANY;
		return B_OK;
	}
	if (leafName == "64.png") {
		*storedSize = BITMAP_SIZE_64;
		return B_OK;
	}
	if (leafName == "32.png") {
		*storedSize = BITMAP_SIZE_32;
		return B_OK;
	}
	if (leafName == "16.png") {
		*storedSize = BITMAP_SIZE_16;
		return B_OK;
	}
	return B_BAD_VALUE;
}


PackageIconTarRepository::PackageIconTarRepository(BPath& tarPath)
	:
	fTarPath(tarPath),
	fTarIo(NULL),
	fIconCache(LIMIT_ICON_CACHE),
	fIconDataBuffer(new BMallocIO()),
	fFallbackRepository(PackageIconDefaultRepository())
{
}


PackageIconTarRepository::~PackageIconTarRepository()
{
	_Close();
	delete fIconDataBuffer;
}


/*!	This method will reconfigure itself using the data in the tar file supplied.
	Any existing data will be flushed and the new tar will be scanned for
	offsets to usable files.
*/

status_t
PackageIconTarRepository::Init()
{
	_Close();

	status_t result = B_OK;

	if (fTarPath.Path() == NULL) {
		HDINFO("empty path to tar-ball");
		result = B_BAD_VALUE;
	}

	BFile* tarIo = NULL;

	if (result == B_OK) {
		HDINFO("will init icon model from tar [%s]", fTarPath.Path());
		tarIo = new BFile(fTarPath.Path(), O_RDONLY);

		if (!tarIo->IsReadable()) {
			HDERROR("unable to read the tar [%s]", fTarPath.Path());
			result = B_IO_ERROR;
		}
	}

	// will fill the model up with records from the tar-ball.

	if (result == B_OK) {
		BStopWatch watch("PackageIconTarRepository::Init", true);
		HDINFO("will read [%s] and collect the tar pointers", fTarPath.Path());

		IconTarPtrEntryListener* listener = new IconTarPtrEntryListener(this);
		ObjectDeleter<IconTarPtrEntryListener> listenerDeleter(listener);
		TarArchiveService::ForEachEntry(*tarIo, listener);

		double secs = watch.ElapsedTime() / 1000000.0;
		HDINFO("did collect %" B_PRIi32 " tar pointers (%6.3g secs)", fIconTarPtrs.Size(), secs);
	}

	if (result == B_OK)
		fTarIo = tarIo;
	else
		delete tarIo;

	if (result == B_OK)
		result = fIconDataBuffer->SetSize(ICON_BUFFER_SIZE_INITIAL);

	return result;
}


void
PackageIconTarRepository::_Close()
{
	fIconCache.Clear();
	delete fTarIo;
	fTarIo = NULL;
	fIconTarPtrs.Clear();
}


/*!	This method is used to populate data from the parsing of the tar headers.
	It is called from the listener above.
*/
// TODO; should be friend to the listener
void
PackageIconTarRepository::AddIconTarPtr(const BString& packageName, BitmapSize storedSize,
	off_t offset)
{
	IconTarPtrRef tarPtrRef = _GetOrCreateIconTarPtr(packageName);
	tarPtrRef->SetOffset(storedSize, offset);
}


status_t
PackageIconTarRepository::GetIcon(const BString& pkgName, uint32 size,
	BitmapHolderRef& bitmapHolderRef)
{
	if (0 == size || size > MAX_IMAGE_SIZE) {
		HDERROR("request to get icon for pkg [%s] with bad size %" B_PRIu32, pkgName.String(),
			size);
		return B_BAD_VALUE;
	}

	status_t result = B_OK;
	bitmapHolderRef.Unset();

	if (fTarIo == NULL) {
		// get the default icon in this case.
		HDDEBUG("the tar io was not configured indicating a possible initialization error");
	} else {
		off_t iconDataTarOffset = -1;
		const IconTarPtrRef tarPtrRef = _GetIconTarPtr(pkgName);
		BitmapSize storedSize;

		if (tarPtrRef.IsSet()) {
			storedSize = _BestStoredSize(tarPtrRef, size);
			iconDataTarOffset = tarPtrRef->Offset(storedSize);
		}

		if (iconDataTarOffset >= 0) {
			HashString key = _ToIconCacheKey(pkgName, storedSize, size);

			if (fIconCache.ContainsKey(key)) {
				bitmapHolderRef.SetTo(fIconCache.Get(key).Get());
			} else {
				result = _CreateIconFromTarOffset(iconDataTarOffset, storedSize, size,
					bitmapHolderRef);
				if (result == B_OK) {
					HDTRACE("loaded package icon [%s] of size %" B_PRId32, pkgName.String(), size);
					fIconCache.Put(key, bitmapHolderRef);
					bitmapHolderRef.SetTo(fIconCache.Get(key).Get());
				} else {
					HDERROR("failure to read image for package [%s] at offset %" B_PRIdOFF,
						pkgName.String(), iconDataTarOffset);
				}
			}
		}
	}

	if (!bitmapHolderRef.IsSet())
		result = fFallbackRepository.GetIcon(pkgName, size, bitmapHolderRef);

	return result;
}


IconTarPtrRef
PackageIconTarRepository::_GetIconTarPtr(const BString& pkgName) const
{
	return fIconTarPtrs.Get(HashString(pkgName));
}


const char*
PackageIconTarRepository::_ToIconCacheKeyPart(BitmapSize storedSize)
{
	switch (storedSize) {
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
PackageIconTarRepository::_ToIconCacheKey(const BString& pkgName, BitmapSize storedSize,
	uint32 size)
{
	return HashString(
		BString(pkgName) << "__s" << _ToIconCacheKeyPart(storedSize) << "__x" << size);
}


status_t
PackageIconTarRepository::_CreateIconFromTarOffset(off_t offset, BitmapSize storedSize, uint32 size,
	BitmapHolderRef& bitmapHolderRef)
{
	fTarIo->Seek(offset, SEEK_SET);
	fIconDataBuffer->Seek(0, SEEK_SET);

	TarArchiveHeader header;
	status_t result = TarArchiveService::GetEntry(*fTarIo, header);

	if (result == B_OK && (header.Length() <= 0 || header.Length() > ICON_BUFFER_SIZE_MAX)) {
		HDERROR("the icon tar entry at %" B_PRIdOFF " has a bad length %" B_PRIdSSIZE, offset,
			header.Length());
		result = B_BAD_DATA;
	}

	off_t iconDataBufferSize = 0;

	if (result == B_OK)
		result = fIconDataBuffer->GetSize(&iconDataBufferSize);

	if (result == B_OK && static_cast<size_t>(iconDataBufferSize) < header.Length())
		result = fIconDataBuffer->SetSize(header.Length());

	if (result == B_OK) {
		BDataIO* tarImageIO = new ConstraintedDataIO(fTarIo, header.Length());
		result = DataIOUtils::CopyAll(fIconDataBuffer, tarImageIO);
		delete tarImageIO;
	} else {
		HDERROR("unable to extract data from tar for icon image");
	}

	fIconDataBuffer->Seek(0, SEEK_SET);

	BBitmap* bitmap = NULL;

	if (result == B_OK) {
		if (BITMAP_SIZE_ANY == storedSize) {
			bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0, B_RGBA32);
			result = bitmap->InitCheck();
			result = BIconUtils::GetVectorIcon(
				reinterpret_cast<const uint8*>(fIconDataBuffer->Buffer()), header.Length(), bitmap);
		} else {
			bitmap = BTranslationUtils::GetBitmap(fIconDataBuffer);

			if (bitmap == NULL) {
				HDERROR("unable to decode data from tar for icon image");
				result = B_ERROR;
			}
		}
	}

	if (result != B_OK)
		delete bitmap;
	else
		bitmapHolderRef.SetTo(new(std::nothrow) BitmapHolder(bitmap), true);

	return result;
}


/*!	If there is a vector representation (HVIF) then this will be the best
	option.  If there are only bitmap images to choose from then consider what
	the target size is and choose the best image to match.
*/
/*static*/ BitmapSize
PackageIconTarRepository::_BestStoredSize(const IconTarPtrRef iconTarPtrRef, int32 desiredSize)
{
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_ANY))
		return BITMAP_SIZE_ANY;
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_64) && desiredSize >= 64)
		return BITMAP_SIZE_64;
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_32) && desiredSize >= 32)
		return BITMAP_SIZE_32;
	if (iconTarPtrRef->HasOffset(BITMAP_SIZE_32))
		return BITMAP_SIZE_16;
	HDFATAL("unable to get the best stored icon for size");
}


IconTarPtrRef
PackageIconTarRepository::_GetOrCreateIconTarPtr(const BString& pkgName)
{
	HashString key(pkgName);
	if (!fIconTarPtrs.ContainsKey(key)) {
		IconTarPtrRef value(new IconTarPtr(pkgName));
		fIconTarPtrs.Put(key, value);
		return value;
	}
	return fIconTarPtrs.Get(key);
}
