/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef TAR_ARCHIVE_SERVICE_H
#define TAR_ARCHIVE_SERVICE_H

#include "AbstractServerProcess.h"
#include "TarArchiveHeader.h"

#include <String.h>
#include <Path.h>


class TarArchiveService {
public:
		static status_t			Unpack(BDataIO& tarDataIo,
									BPath& targetDirectoryPath);

private:
		static status_t			_EnsurePathToTarItemFile(
									BPath& targetDirectoryPath,
									BString &tarItemPath);
		static status_t			_ValidatePathComponent(
									const BString& component);
		static status_t			_UnpackItem(BDataIO& tarDataIo,
									BPath& targetDirectory,
									TarArchiveHeader& header);
		static status_t			_UnpackItemData(BDataIO& tarDataIo,
									BPath& targetFilePath,
									uint32 length);

};

#endif // TAR_ARCHIVE_SERVICE_H
