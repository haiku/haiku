/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__V1__PACKAGE_READER_H_
#define _PACKAGE__HPKG__V1__PACKAGE_READER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput;


namespace V1 {


namespace BPrivate {
	class PackageReaderImpl;
}
using BPrivate::PackageReaderImpl;

class BLowLevelPackageContentHandler;
class BPackageContentHandler;


class BPackageReader {
public:
								BPackageReader(
									BErrorOutput* errorOutput);
								~BPackageReader();

			status_t			Init(const char* fileName);
			status_t			Init(int fd, bool keepFD);
			status_t			ParseContent(
									BPackageContentHandler* contentHandler);
			status_t			ParseContent(BLowLevelPackageContentHandler*
										contentHandler);

			int					PackageFileFD();
private:
			PackageReaderImpl*	fImpl;
};


}	// namespace V1

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__V1__PACKAGE_READER_H_
