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

class RepositoryWriterImpl
	: public WriterImplBase, private BPackageContentHandler {
public:
								RepositoryWriterImpl(
									BRepositoryWriterListener* listener,
									const BRepositoryInfo* repositoryInfo);
								~RepositoryWriterImpl();

			status_t			Init(const char* fileName);
			status_t			AddPackage(const BEntry& packageEntry);
			status_t			Finish();

private:
								// BPackageContentHandler
	virtual	status_t			HandleEntry(BPackageEntry* entry);
	virtual	status_t			HandleEntryAttribute(BPackageEntry* entry,
									BPackageEntryAttribute* attribute);
	virtual	status_t			HandleEntryDone(BPackageEntry* entry);

	virtual	status_t			HandlePackageAttribute(
									const BPackageInfoAttributeValue& value
									);
	virtual	status_t			HandlePackageAttributesDone();

	virtual	void				HandleErrorOccurred();

private:
			status_t			_Init(const char* fileName);
			status_t			_AddPackage(const BEntry& packageEntry);
			status_t			_Finish();

			status_t			_RegisterCurrentPackageInfo();
			status_t			_WriteRepositoryInfo(
									ssize_t& _repositoryInfoLength);
			off_t				_WritePackageAttributes(
									hpkg_repo_header& header,
									off_t startOffset);

			struct PackageNameSet;

private:
			BRepositoryWriterListener*	fListener;

			const BRepositoryInfo*	fRepositoryInfo;

			BPackageInfo		fPackageInfo;
			uint32				fPackageCount;
			PackageNameSet*		fPackageNames;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__REPOSITORY_WRITER_IMPL_H_
