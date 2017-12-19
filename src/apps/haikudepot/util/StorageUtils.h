/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <Path.h>

class StorageUtils {

public:
	static status_t			RemoveDirectoryContents(BPath& path);
	static status_t			AppendToString(BPath& path, BString& result);
	static status_t			ExistsObject(BPath& directory,
								bool* exists,
								bool* isDirectory,
								off_t* size);
};

#endif // PATH_UTILS_H
