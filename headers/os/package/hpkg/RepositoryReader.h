/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__REPOSITORY_READER_H_
#define _PACKAGE__HPKG__REPOSITORY_READER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class RepositoryReaderImpl;
}
using BPrivate::RepositoryReaderImpl;

class BErrorOutput;
class BRepositoryContentHandler;


class BRepositoryReader {
public:
								BRepositoryReader(BErrorOutput* errorOutput);
								~BRepositoryReader();

			status_t			Init(const char* fileName);
			status_t			ParseContent(
									BRepositoryContentHandler* contentHandler);

private:
			RepositoryReaderImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__REPOSITORY_READER_H_
