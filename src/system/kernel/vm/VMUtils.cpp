/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamish Morrison, hamish@lavabit.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "VMUtils.h"

#include <stdio.h>

#include <StackOrHeapArray.h>


status_t
get_mount_point(KPartition* partition, KPath* mountPoint)
{
	if (!mountPoint || !partition->ContainsFileSystem())
		return B_BAD_VALUE;

	int nameLength = 0;
	const char* volumeName = partition->ContentName();
	if (volumeName != NULL)
		nameLength = strlen(volumeName);
	if (nameLength == 0) {
		volumeName = partition->Name();
		if (volumeName != NULL)
			nameLength = strlen(volumeName);
		if (nameLength == 0) {
			volumeName = "unnamed volume";
			nameLength = strlen(volumeName);
		}
	}

	BStackOrHeapArray<char, 128> basePath(nameLength + 2);
	if (!basePath.IsValid())
		return B_NO_MEMORY;
	int32 len = snprintf(basePath, nameLength + 2, "/%s", volumeName);
	for (int32 i = 1; i < len; i++)
		if (basePath[i] == '/')
			basePath[i] = '-';
	char* path = mountPoint->LockBuffer();
	int32 pathLen = mountPoint->BufferSize();
	strncpy(path, basePath, pathLen);

	struct stat dummy;
	for (int i = 1; ; i++) {
		if (stat(path, &dummy) != 0)
			break;
		snprintf(path, pathLen, "%s%d", (char*)basePath, i);
	}

	mountPoint->UnlockBuffer();
	return B_OK;
}
