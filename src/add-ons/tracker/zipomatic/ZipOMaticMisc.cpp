/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "ZipOMaticMisc.h"

#include <string.h>

#include <Debug.h>


status_t  
FindAndCreateDirectory(directory_which which, BVolume* volume,
	const char* relativePath, BPath* fullPath)
{
	BPath path;
	status_t status = find_directory(which, &path, true, volume);
	if (status != B_OK)
		return status;

	if (relativePath != NULL) {
		path.Append(relativePath);

		mode_t mask = umask(0);
		umask(mask);

		status = create_directory(path.Path(), mask);
		if (status != B_OK)
			return status;
	}

	if (fullPath != NULL) {
		status = fullPath->SetTo(path.Path());
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


void
ErrorMessage(const char* text, int32 status)
{
	PRINT(("%s: %s\n", text, strerror(status)));
}

