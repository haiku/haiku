/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_


#include <Entry.h>

#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/RepositoryWriter.h>
#include <package/hpkg/WriterImplBase.h>
#include <package/PackageInfo.h>


namespace BPackageKit {


namespace BHPKG {


class BPackageEntry;
class BPackageEntryAttribute;
class BPackageInfoAttributeValue;


namespace BPrivate {


struct hpkg_header;

class RepositoryWriterImpl : public WriterImplBase {
	typedef	WriterImplBase		inherited;

public:
								RepositoryWriterImpl(
									BRepositoryWriterListener* listener,
									BRepositoryInfo* repositoryInfo);
								~RepositoryWriterImpl();

			status_t			Init(const char* fileName);
			status_t			AddPackage(const BEntry& packageEntry);
			status_t			Finish();

private:
			status_t			_Init(const char* fileName);
			status_t			_AddPackage(const BEntry& packageEntry);
			status_t			_Finish();

			status_t			_RegisterCurrentPackageInfo();
			status_t			_WriteRepositoryInfo(hpkg_repo_header& header,
									ssize_t& _infoLengthCompressed);
			off_t				_WritePackageAttributes(
									hpkg_repo_header& header, off_t startOffset,
									ssize_t& _packagesLengthCompressed);

			struct PackageNameSet;

private:
			BRepositoryWriterListener*	fListener;

			BRepositoryInfo*	fRepositoryInfo;

			BPackageInfo		fPackageInfo;
			uint32				fPackageCount;
			PackageNameSet*		fPackageNames;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
