/*
 * Copyright 2017-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <Path.h>

#include "Language.h"

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

	static status_t			AppendToString(const BPath& path, BString& result);
	static status_t			AppendToFile(const BString& input,
								const BPath& path);

	static status_t			RemoveWorkingDirectoryContents();
	static status_t			RemoveDirectoryContents(BPath& path);
	static status_t			RemoveDirectoryContentsRetainingLatestFiles(BPath& path,
								uint32 countLatestRetained);

	static status_t			ExistsObject(const BPath& path,
								bool* exists,
								bool* isDirectory,
								off_t* size);

	static	status_t		SwapExtensionOnPath(BPath& path,
								const char* extension);
	static	BString			SwapExtensionOnPathComponent(
								const char* pathComponent,
								const char* extension);

	static	status_t		IconTarPath(BPath& path);
    static	status_t		DumpExportReferenceDataPath(BPath& path, const LanguageRef language);
    static	status_t		DumpExportRepositoryDataPath(BPath& path, const LanguageRef language);
    static	status_t		DumpExportPkgDataPath(BPath& path, const BString& repositorySourceCode,
    							const LanguageRef language);

};

#endif // PATH_UTILS_H
