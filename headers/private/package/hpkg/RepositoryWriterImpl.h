/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <package/PackageInfo.h>
#include <package/hpkg/Strings.h>
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
			status_t			AddPackage(const BPackageInfo& packageInfo);
			status_t			Finish();

private:
			status_t			_Init(const char* fileName);
			status_t			_AddPackage(const BPackageInfo& packageInfo);
			status_t			_Finish();

private:
			BRepositoryWriterListener*	fListener;

};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
