/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_WRITER_H_
#define _PACKAGE__HPKG__PACKAGE_WRITER_H_


#include <SupportDefs.h>

#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class PackageWriterImpl;
}
using BPrivate::PackageWriterImpl;


class BPackageWriterListener : public BErrorOutput {
public:
	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args) = 0;

	virtual	void				OnEntryAdded(const char* path) = 0;

	virtual void				OnTOCSizeInfo(uint64 uncompressedStringsSize,
									uint64 uncompressedMainSize,
									uint64 uncompressedTOCSize) = 0;
	virtual void				OnPackageAttributesSizeInfo(uint32 stringCount,
									uint32 uncompressedSize) = 0;
	virtual void				OnPackageSizeInfo(uint32 headerSize,
									uint64 heapSize, uint64 tocSize,
									uint32 packageAttributesSize,
									uint64 totalSize) = 0;
};


class BPackageWriter {
public:
								BPackageWriter(
									BPackageWriterListener* listener);
								~BPackageWriter();

			status_t			Init(const char* fileName, uint32 flags = 0);
			status_t			AddEntry(const char* fileName, int fd = -1);
			status_t			Finish();

private:
			PackageWriterImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_WRITER_H_
