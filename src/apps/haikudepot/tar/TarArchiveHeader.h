/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TAR_ARCHIVE_HEADER_H
#define TAR_ARCHIVE_HEADER_H

#include <String.h>


enum tar_file_type {
	TAR_FILE_TYPE_NORMAL,
	TAR_FILE_TYPE_OTHER
};


/* Each file in a tar-archive has a header on it describing the next entry in
 * the stream.  This class models the data in the header.
 */

class TarArchiveHeader {
public:
									TarArchiveHeader();
	virtual							~TarArchiveHeader();

	const	BString&				FileName() const;
			size_t					Length() const;
			tar_file_type			FileType() const;

			void					SetFileName(const BString& value);
			void					SetLength(size_t value);
			void					SetFileType(tar_file_type value);
private:
			BString					fFileName;
			uint64					fLength;
			tar_file_type			fFileType;
};

#endif // TAR_ARCHIVE_HEADER_H
