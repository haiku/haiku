/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageScreenshotRepository.h"

#include <unistd.h>

#include <AutoDeleter.h>
#include <TranslationUtils.h>

#include "FileIO.h"
#include "Logger.h"
#include "Model.h"
#include "StorageUtils.h"
#include "WebAppInterface.h"


static const uint32 kMaxRetainedCachedScreenshots = 25;


PackageScreenshotRepository::PackageScreenshotRepository(
	PackageScreenshotRepositoryListenerRef listener, Model* model)
	:
	fListener(listener),
	fModel(model)
{
	_Init();
}


PackageScreenshotRepository::~PackageScreenshotRepository()
{
	fListener.Unset();
	_CleanCache();
}


/*!	This method will load the specified screenshot from remote, but will by-pass
	the cache. It will load the data into a file before loading it in order to
	avoid the data needing to be resident in memory twice; thus saving memory
	use.
*/

status_t
PackageScreenshotRepository::LoadScreenshot(const ScreenshotCoordinate& coord,
	BitmapHolderRef& bitmapHolderRef)
{
	if (!coord.IsValid())
		return B_BAD_VALUE;

	BPath temporaryFilePath(tmpnam(NULL), NULL, true);
	status_t result = _DownloadToLocalFile(coord, temporaryFilePath);
	const char* temporaryFilePathStr = temporaryFilePath.Path();

	if (result == B_OK) {
		FILE* file = fopen(temporaryFilePathStr, "rb");

		if (file == NULL) {
			HDERROR("unable to open the screenshot file for read at [%s]", temporaryFilePathStr);
			result = B_IO_ERROR;
		}

		if (result == B_OK) {
			BFileIO fileIo(file, true); // takes ownership
			BBitmap* bitmap = BTranslationUtils::GetBitmap(&fileIo);

			if (bitmap == NULL)
				return B_IO_ERROR;

			bitmapHolderRef.SetTo(new(std::nothrow) BitmapHolder(bitmap), true);
		}
	}

	// even if the temporary file cannot be deleted, still return that it was OK.

	if (remove(temporaryFilePathStr) != 0)
		HDERROR("unable to delete the temporary file [%s]", temporaryFilePathStr);

	return result;
}


status_t
PackageScreenshotRepository::CacheAndLoadScreenshot(const ScreenshotCoordinate& coord,
	BitmapHolderRef& bitmapHolderRef)
{
	if (!coord.IsValid())
		return B_BAD_VALUE;

	CacheScreenshot(coord);

	BPositionIO* data = NULL;
	status_t result = _CreateCachedData(coord, &data);

	if (result == B_OK) {
		ObjectDeleter<BPositionIO> dataDeleter(data);
		BBitmap* bitmap = BTranslationUtils::GetBitmap(data);

		if (bitmap == NULL)
			return B_IO_ERROR;

		bitmapHolderRef.SetTo(new(std::nothrow) BitmapHolder(bitmap), true);
	}

	return result;
}


status_t
PackageScreenshotRepository::HasCachedScreenshot(const ScreenshotCoordinate& coord, bool* value)
{
	if (value == NULL)
		debugger("expected the value to be supplied");

	*value = false;

	if (!coord.IsValid())
		return B_BAD_VALUE;

	BPath path = _DeriveCachePath(coord);
	const char* pathStr = path.Path();
	BEntry entry(pathStr);

	struct stat s = {};
	status_t result = entry.GetStat(&s);

	switch (result) {
		case B_ENTRY_NOT_FOUND:
			*value = false;
			return B_OK;
		case B_OK:
			*value = (s.st_size > 0);
			return B_OK;
		default:
			return result;
	}
}


status_t
PackageScreenshotRepository::CacheScreenshot(const ScreenshotCoordinate& coord)
{
	if (!coord.IsValid())
		return B_BAD_VALUE;

	bool present = false;
	status_t result = HasCachedScreenshot(coord, &present);

	if (result == B_OK && present)
		return result;

	if (result == B_OK)
		result = _DownloadToLocalFile(coord, _DeriveCachePath(coord));

	return result;
}


status_t
PackageScreenshotRepository::_Init()
{
	fBaseDirectory.Unset();

	status_t result = StorageUtils::LocalWorkingDirectoryPath("screenshot_cache", fBaseDirectory);

	if (B_OK != result)
		HDERROR("unable to setup the cache directory");

	_CleanCache();

	return result;
}


/*! Gets all of the cached files and looks to see the last access on them. The
	most recent files are retained and the rest are deleted from the disk.
*/

status_t
PackageScreenshotRepository::_CleanCache()
{
	HDINFO("will clean the screenshot cache");
	// get all of the files and then order them by last accessed and then
	// delete the older ones.
	return StorageUtils::RemoveDirectoryContentsRetainingLatestFiles(fBaseDirectory,
		kMaxRetainedCachedScreenshots);
}


status_t
PackageScreenshotRepository::_DownloadToLocalFile(const ScreenshotCoordinate& coord,
	const BPath& path)
{
	const char* pathStr = path.Path();
	FILE* file = fopen(pathStr, "wb");

	if (file == NULL) {
		HDERROR("unable to open the screenshot file for writing at [%s]", pathStr);
		return B_IO_ERROR;
	}

	BFileIO outputDataStream(file, true); // takes ownership
	status_t result = _WebApp()->RetrieveScreenshot(coord.Code(), coord.Width(), coord.Height(),
		&outputDataStream);

	if (result == B_OK)
		result = outputDataStream.Flush();

	if (result == B_OK)
		fListener->ScreenshotCached(coord);

	return result;
}


BPath
PackageScreenshotRepository::_DeriveCachePath(const ScreenshotCoordinate& coord) const
{
	BPath path(fBaseDirectory);
	path.Append(coord.CacheFilename());
	return path;
}


status_t
PackageScreenshotRepository::_CreateCachedData(const ScreenshotCoordinate& coord,
	BPositionIO** data)
{
	status_t result = B_OK;
	BPath path = _DeriveCachePath(coord);
	const char* pathStr = path.Path();
	FILE* file = fopen(pathStr, "rb");

	if (file == NULL) {
		HDERROR("unable to open the screenshot file for read at [%s]", pathStr);
		result = B_IO_ERROR;
	}

	if (result == B_OK)
		*data = new BFileIO(file, true); // takes ownership

	return result;
}


WebAppInterfaceRef
PackageScreenshotRepository::_WebApp()
{
	return fModel->WebApp();
}

