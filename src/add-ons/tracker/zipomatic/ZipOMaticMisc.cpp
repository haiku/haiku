/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */


#include "ZipOMaticMisc.h"

#include <Debug.h>
#include <Path.h>

#include <string.h>


status_t  
find_and_create_directory(directory_which which, BVolume* volume,
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
error_message(const char* text, int32 status)
{
	PRINT(("%s: %s\n", text, strerror(status)));
}

