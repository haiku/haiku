/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill, supernova@tycho.email
 */

#include "AppRefFilter.h"

#include <string.h>


/*	This class filters only applications in an open file panel */
AppRefFilter::AppRefFilter()
	:
	BRefFilter()
{
}


bool
AppRefFilter::Filter(const entry_ref *ref, BNode *node,
	struct stat_beos *st, const char *filetype)
{
	char* type = NULL;
	const char *constFileType;
	// resolve symlinks
	bool isSymlink = strcmp("application/x-vnd.Be-symlink", filetype) == 0;
	if (isSymlink) {
		BEntry linkedEntry(ref, true);
		if (linkedEntry.InitCheck()!=B_OK)
			return false;
		BNode linkedNode(&linkedEntry);
		if (linkedNode.InitCheck()!=B_OK)
			return false;
		BNodeInfo linkedNodeInfo(&linkedNode);
		if (linkedNodeInfo.InitCheck()!=B_OK)
			return false;
		type = new char[B_ATTR_NAME_LENGTH];
		if (linkedNodeInfo.GetType(type)!=B_OK) {
			delete[] type;
			return false;
		}
		constFileType = type;
	} else
		constFileType = filetype;

	bool pass = false;
	//folders
	if (strcmp("application/x-vnd.Be-directory", constFileType) == 0)
		pass = true;
	//volumes
	else if (strcmp("application/x-vnd.Be-volume", constFileType) == 0)
		pass = true;
	//apps
	else if (strcmp("application/x-vnd.Be-elfexecutable", constFileType) == 0)
		pass = true;
	//hack for Haiku?  Some apps are defined by MIME this way
	else if (strcmp("application/x-vnd.be-elfexecutable", constFileType) == 0)
		pass = true;

	if (isSymlink)
		delete[] type;
	return pass;
}
