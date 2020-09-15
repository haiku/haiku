/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TAR_ARCHIVE_SERVICE_H
#define TAR_ARCHIVE_SERVICE_H

#include "Stoppable.h"
#include "TarArchiveHeader.h"

#include <String.h>
#include <Path.h>


class TarEntryListener {
public:
	virtual status_t			Handle(
									const TarArchiveHeader& header,
									size_t offset,
									BDataIO* data) = 0;
};


class TarArchiveService {
public:
	static	status_t			ForEachEntry(BPositionIO& tarIo,
									TarEntryListener* listener);
	static	status_t			GetEntry(BPositionIO& tarIo,
									TarArchiveHeader& header);

private:
	static	status_t			_ValidatePathComponent(
									const BString& component);

	static	off_t				_BytesRoundedToBlocks(off_t value);
	static	uint32				_CalculateBlockChecksum(
									const unsigned char* data);

	static	status_t			_ReadHeader(const uint8* data,
										TarArchiveHeader& header);
	static	int32				_ReadHeaderStringLength(const uint8* data,
									size_t maxStringLength);
	static	void				_ReadHeaderString(const uint8* data,
									size_t dataLength, BString& result);
	static uint32				_ReadHeaderNumeric(const uint8* data,
									size_t dataLength);
	static tar_file_type		_ReadHeaderFileType(uint8 data);

};

#endif // TAR_ARCHIVE_SERVICE_H
