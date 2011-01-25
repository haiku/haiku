/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_
#define _PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_


#include <Entry.h>
#include <String.h>


namespace BPackageKit {

namespace BPrivate {


class ChecksumAccessor {
public:
	virtual						~ChecksumAccessor();

	virtual	status_t			GetChecksum(BString& checksum) const = 0;
};


class ChecksumFileChecksumAccessor : public ChecksumAccessor {
public:
								ChecksumFileChecksumAccessor(
									const BEntry& checksumFileEntry);

	virtual	status_t			GetChecksum(BString& checksum) const;

private:
			BEntry				fChecksumFileEntry;
};


class GeneralFileChecksumAccessor : public ChecksumAccessor {
public:
								GeneralFileChecksumAccessor(
									const BEntry& fileEntry,
									bool skipMissingFile = false);

	virtual	status_t			GetChecksum(BString& checksum) const;

private:
			BEntry				fFileEntry;
			bool				fSkipMissingFile;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_
