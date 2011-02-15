/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__REPOSITORY_READER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__REPOSITORY_READER_IMPL_H_


#include <package/hpkg/ReaderImplBase.h>

#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BHPKG {


class BRepositoryContentHandler;


namespace BPrivate {


class RepositoryReaderImpl : public ReaderImplBase {
	typedef	ReaderImplBase		inherited;
public:
								RepositoryReaderImpl(
									BErrorOutput* errorOutput);
								~RepositoryReaderImpl();

			status_t			Init(const char* fileName);
			status_t			Init(int fd, bool keepFD);

			status_t			GetRepositoryInfo(
									BRepositoryInfo* _repositoryInfo) const;

			status_t			ParseContent(
									BRepositoryContentHandler* contentHandler);

private:
			struct RootAttributeHandler;

private:
			SectionInfo			fRepositoryInfoSection;
			BRepositoryInfo		fRepositoryInfo;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__REPOSITORY_READER_IMPL_H_
