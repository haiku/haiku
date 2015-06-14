/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Constants.h"

#include <package/PackagesDirectoryDefs.h>


const char* const kPackageFileNameExtension = ".hpkg";
const char* const kAdminDirectoryName = PACKAGES_DIRECTORY_ADMIN_DIRECTORY;
const char* const kActivationFileName = PACKAGES_DIRECTORY_ACTIVATION_FILE;
const char* const kTemporaryActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE ".tmp";
const char* const kWritableFilesDirectoryName = "writable-files";
const char* const kPackageFileAttribute = "SYS:PACKAGE";
const char* const kQueuedScriptsDirectoryName = "queued-scripts";
