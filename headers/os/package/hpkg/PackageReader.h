/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_READER_H_
#define _PACKAGE__HPKG__PACKAGE_READER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class PackageReaderImpl;
}
using BPrivate::PackageReaderImpl;

class BErrorOutput;
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


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_READER_H_
