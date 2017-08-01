/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef STANDARD_META_DATA_H
#define STANDARD_META_DATA_H

#include <DateTime.h>
#include <File.h>
#include <HttpHeaders.h>
#include <Locker.h>
#include <String.h>


/* This class models (some of) the meta-data that is bundled into data that is
 * relayed from the HaikuDepotServer application server down to the client.
 * This includes the tar-ball of icons as well as "bulk data" such as streams of
 * information about repositories and packages.
 */


class StandardMetaData {
public:
										StandardMetaData();

			uint64_t					GetCreateTimestamp();
			BDateTime					GetCreateTimestampAsDateTime();
			void						SetCreateTimestamp(uint64_t value);

			uint64_t					GetDataModifiedTimestamp();
			BDateTime					GetDataModifiedTimestampAsDateTime();
			void						SetDataModifiedTimestamp(
											uint64_t value);

			bool						IsPopulated();
private:
			BDateTime					_CreateDateTime(
											uint64_t millisSinceEpoc);
			uint64_t					fCreateTimestamp;
			uint64_t					fDataModifiedTimestamp;
};


#endif // STANDARD_META_DATA_H
