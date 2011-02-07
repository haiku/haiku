/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__REPOSITORY_WRITER_H_
#define _PACKAGE__HPKG__REPOSITORY_WRITER_H_


#include <SupportDefs.h>

#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

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

	virtual void				OnPackageAttributesSizeInfo(
									uint32 uncompressedSize) = 0;
	virtual void				OnRepositorySizeInfo(uint32 headerSize,
									uint32 packageAttributesSize,
									uint64 totalSize) = 0;
};


class BRepositoryWriter {
public:
public:
								BRepositoryWriter(
									BRepositoryWriterListener* listener);
								~BRepositoryWriter();

			status_t			Init(const char* fileName);
			status_t			AddPackage(const BPackageInfo& packageInfo);
			status_t			Finish();

private:
			RepositoryWriterImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__REPOSITORY_WRITER_H_
