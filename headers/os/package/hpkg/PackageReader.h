/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_READER_H_
#define _PACKAGE__HPKG__PACKAGE_READER_H_


#include <SupportDefs.h>


class BPositionIO;


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class PackageReaderImpl;
}
using BPrivate::PackageReaderImpl;


class BAbstractBufferedDataReader;
class BErrorOutput;
class BLowLevelPackageContentHandler;
class BPackageContentHandler;
class BPackageWriter;


class BPackageReader {
public:
								BPackageReader(BErrorOutput* errorOutput);
								~BPackageReader();

			status_t			Init(const char* fileName, uint32 flags = 0);
			status_t			Init(int fd, bool keepFD, uint32 flags = 0);
			status_t			Init(BPositionIO* file, bool keepFile,
									uint32 flags = 0);
			status_t			ParseContent(
									BPackageContentHandler* contentHandler);
			status_t			ParseContent(BLowLevelPackageContentHandler*
										contentHandler);

			BPositionIO*		PackageFile() const;

			BAbstractBufferedDataReader* HeapReader() const;
									// Only valid as long as the reader lives.

private:
			friend class BPackageWriter;

private:
			PackageReaderImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_READER_H_
