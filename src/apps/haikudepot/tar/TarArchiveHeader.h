/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
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
									TarArchiveHeader(const BString& fileName,
										uint64 length, tar_file_type fileType);

		static TarArchiveHeader*	CreateFromBlock(const unsigned char *block);

			const BString&			GetFileName() const;
			size_t					GetLength() const;
			tar_file_type			GetFileType() const;

private:
		static uint32				_CalculateChecksum(
										const unsigned char* data);
		static const BString		_ReadString(const unsigned char* data,
										size_t dataLength);
		static uint32				_ReadNumeric(const unsigned char* data,
										size_t dataLength);
		static tar_file_type		_ReadFileType(unsigned char data);

			const BString			fFileName;
			uint64					fLength;
			tar_file_type			fFileType;

};

#endif // TAR_ARCHIVE_HEADER_H
