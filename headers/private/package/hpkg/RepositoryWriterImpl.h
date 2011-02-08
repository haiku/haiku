/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_


#include <Entry.h>

#include <package/hpkg/RepositoryWriter.h>
#include <package/hpkg/WriterImplBase.h>


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


struct hpkg_header;

class RepositoryWriterImpl : public WriterImplBase {
public:
								RepositoryWriterImpl(
									BRepositoryWriterListener* listener);
								~RepositoryWriterImpl();

			status_t			Init(const char* fileName);
			status_t			AddPackage(const BEntry& packageEntry);
			status_t			Finish();

private:
			status_t			_Init(const char* fileName);
			status_t			_AddPackage(const BEntry& packageEntry);
			status_t			_Finish();

			off_t				_WritePackageAttributes(
									hpkg_repo_header& header);

private:
			BRepositoryWriterListener*	fListener;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
