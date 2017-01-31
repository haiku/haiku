/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef ICON_META_DATA_H
#define ICON_META_DATA_H

#include <DateTime.h>
#include <File.h>
#include <HttpHeaders.h>
#include <Locker.h>
#include <String.h>


/* This class models (some of) the meta-data that is bundled into the tar file
 * that is downloaded to the HaikuDepot client from the HaikuDepotServer
 * application server system.  The file is included in the tar-ball data.
 */


class IconMetaData {
public:
			uint64_t					GetCreateTimestamp();
			BDateTime					GetCreateTimestampAsDateTime();
			void						SetCreateTimestamp(uint64_t value);

			uint64_t					GetDataModifiedTimestamp();
			BDateTime					GetDataModifiedTimestampAsDateTime();
			void						SetDataModifiedTimestamp(
											uint64_t value);

private:
			BDateTime					_CreateDateTime(
											uint64_t millisSinceEpoc);
			uint64_t					fCreateTimestamp;
			uint64_t					fDataModifiedTimestamp;
};


#endif // ICON_META_DATA_H
