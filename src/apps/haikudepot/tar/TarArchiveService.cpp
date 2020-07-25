/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "TarArchiveService.h"

#include <Directory.h>
#include <File.h>
#include <StringList.h>

#include "Logger.h"
#include "StorageUtils.h"


#define LENGTH_BLOCK 512


status_t
TarArchiveService::Unpack(BDataIO& tarDataIo, BPath& targetDirectory,
	Stoppable* stoppable)
{
	uint8 buffer[LENGTH_BLOCK];
	uint8 zero_buffer[LENGTH_BLOCK];
	status_t result = B_OK;
	uint32_t count_items_read = 0;

	HDINFO("will unpack to [%s]", targetDirectory.Path());

	memset(zero_buffer, 0, sizeof zero_buffer);

	while (B_OK == result
		&& (NULL == stoppable || !stoppable->WasStopped())
		&& B_OK == (result = tarDataIo.ReadExactly(buffer, LENGTH_BLOCK))) {

		count_items_read++;

		if (0 == memcmp(zero_buffer, buffer, sizeof zero_buffer)) {
			HDDEBUG("detected end of tar-ball");
			return B_OK; // end of tar-ball.
		} else {
			TarArchiveHeader* header = TarArchiveHeader::CreateFromBlock(
				buffer);

			if (NULL == header) {
				HDERROR("unable to parse a tar header");
				result = B_ERROR;
			}

			if (B_OK == result)
				result = _UnpackItem(tarDataIo, targetDirectory, *header);

			delete header;
		}
	}

	HDERROR("did unpack %d tar items", count_items_read);

	if (B_OK != result) {
		HDERROR("error occurred unpacking tar items; %s", strerror(result));
	}

	return result;
}


status_t
TarArchiveService::_EnsurePathToTarItemFile(
	BPath& targetDirectoryPath, BString &tarItemPath)
{
	if (tarItemPath.Length() == 0)
		return B_ERROR;

	BStringList components;
	tarItemPath.Split("/", false, components);

	for (int32 i = 0; i < components.CountStrings(); i++) {
		BString component = components.StringAt(i);

		if (_ValidatePathComponent(component) != B_OK) {
			HDERROR("malformed component; [%s]", component.String());
			return B_ERROR;
		}
	}

	BPath parentPath;
	BPath assembledPath(targetDirectoryPath);

	status_t result = assembledPath.Append(tarItemPath);

	if (result == B_OK)
		result = assembledPath.GetParent(&parentPath);

	if (result == B_OK)
		result = create_directory(parentPath.Path(), 0777);

	return result;
}


status_t
TarArchiveService::_UnpackItem(BDataIO& tarDataIo,
		BPath& targetDirectoryPath,
		TarArchiveHeader& header)
{
	status_t result = B_OK;
	BString entryFileName = header.GetFileName();
	uint32 entryLength = header.GetLength();

	HDDEBUG("will unpack item [%s] length [%" B_PRIu32 "]b",
		entryFileName.String(), entryLength);

	// if the path ends in "/" then it is a directory and there's no need to
	// unpack it although if there is a length, it will need to be skipped.

	if (!entryFileName.EndsWith("/") ||
			header.GetFileType() != TAR_FILE_TYPE_NORMAL) {

		result = _EnsurePathToTarItemFile(targetDirectoryPath,
			entryFileName);

		if (result == B_OK) {
			BPath targetFilePath(targetDirectoryPath);
			targetFilePath.Append(entryFileName, false);
			result = _UnpackItemData(tarDataIo, targetFilePath, entryLength);
		}
	} else {
		off_t blocksToSkip = (entryLength / LENGTH_BLOCK);

		if (entryLength % LENGTH_BLOCK > 0)
			blocksToSkip += 1;

		if (0 != blocksToSkip) {
			uint8 buffer[LENGTH_BLOCK];

			for (uint32 i = 0; B_OK == result && i < blocksToSkip; i++)
				tarDataIo.ReadExactly(buffer, LENGTH_BLOCK);
		}
	}

	return result;
}


status_t
TarArchiveService::_UnpackItemData(BDataIO& tarDataIo,
	BPath& targetFilePath, uint32 length)
{
	uint8 buffer[LENGTH_BLOCK];
	size_t remainingInItem = length;
	status_t result = B_OK;
	BFile targetFile(targetFilePath.Path(), O_WRONLY | O_CREAT);

	while (remainingInItem > 0 &&
		B_OK == result &&
		B_OK == (result = tarDataIo.ReadExactly(buffer, LENGTH_BLOCK))) {

		size_t writeFromBuffer = LENGTH_BLOCK;

		if (remainingInItem < LENGTH_BLOCK)
			writeFromBuffer = remainingInItem;

		result = targetFile.WriteExactly(buffer, writeFromBuffer);
		remainingInItem -= writeFromBuffer;
	}

	if (result != B_OK) {
		HDERROR("unable to unpack item data to; [%s]", targetFilePath.Path());
	}

	return result;
}


status_t
TarArchiveService::_ValidatePathComponent(const BString& component)
{
	if (component.Length() == 0)
		return	B_ERROR;

	if (component == ".." || component == "." || component == "~")
		return B_ERROR;

	return B_OK;
}

