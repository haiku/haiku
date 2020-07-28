/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <Path.h>

class StorageUtils {

public:
	static bool				AreWorkingFilesAvailable();
	static void				SetWorkingFilesUnavailable();

	static status_t			LocalWorkingFilesPath(const BString leaf,
								BPath& path,
								bool failOnCreateDirectory = true);
	static status_t			LocalWorkingDirectoryPath(const BString leaf,
								BPath& path,
								bool failOnCreateDirectory = true);

	static status_t			CheckCanWriteTo(const BPath& path);

	static status_t			RemoveDirectoryContents(BPath& path);
	static status_t			AppendToString(BPath& path, BString& result);
	static status_t			ExistsObject(const BPath& path,
								bool* exists,
								bool* isDirectory,
								off_t* size);

	static	status_t		SwapExtensionOnPath(BPath& path,
								const char* extension);
	static	BString			SwapExtensionOnPathComponent(
								const char* pathComponent,
								const char* extension);
};

#endif // PATH_UTILS_H
