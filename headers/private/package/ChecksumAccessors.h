/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_
#define _HAIKU__PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_


#include <Entry.h>
#include <String.h>


namespace Haiku {

namespace Package {

namespace Private {


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


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__CHECKSUM_ACCESSORS_H_
