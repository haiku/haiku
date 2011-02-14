/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__REPOSITORY_WRITER_H_
#define _PACKAGE__HPKG__REPOSITORY_WRITER_H_


#include <package/hpkg/ErrorOutput.h>


class BEntry;


namespace BPackageKit {


class BPackageInfo;
class BRepositoryInfo;


namespace BHPKG {


namespace BPrivate {
	class RepositoryWriterImpl;
}
using BPrivate::RepositoryWriterImpl;


class BRepositoryWriterListener : public BErrorOutput {
public:
	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args) = 0;

	virtual	void				OnPackageAdded(
									const BPackageInfo& packageInfo) = 0;

	virtual void				OnRepositoryInfoSectionDone(
									uint32 uncompressedSize) = 0;
	virtual void				OnPackageAttributesSectionDone(
									uint32 stringCount,
									uint32 uncompressedSize) = 0;
	virtual void				OnRepositoryDone(uint32 headerSize,
									uint32 repositoryInfoLength,
									uint32 licenseCount,
									uint32 packageCount,
									uint32 packageAttributesSize,
									uint64 totalSize) = 0;
};


class BRepositoryWriter {
public:
public:
								BRepositoryWriter(
									BRepositoryWriterListener* listener,
									BRepositoryInfo* repositoryInfo);
								~BRepositoryWriter();

			status_t			Init(const char* fileName);
			status_t			AddPackage(const BEntry& packageEntry);
			status_t			Finish();

private:
			RepositoryWriterImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__REPOSITORY_WRITER_H_
