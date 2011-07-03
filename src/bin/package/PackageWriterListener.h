/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_WRITER_LISTENER_H
#define PACKAGE_WRITER_LISTENER_H


#include <package/hpkg/PackageWriter.h>


using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriter;


class PackageWriterListener	: public BPackageWriterListener {
public:
								PackageWriterListener(bool verbose, bool quiet);

	virtual	void				PrintErrorVarArgs(const char* format,
									va_list args);

	virtual	void				OnEntryAdded(const char* path);
	virtual	void				OnTOCSizeInfo(uint64 uncompressedStringsSize,
									uint64 uncompressedMainSize,
									uint64 uncompressedTOCSize);
	virtual	void				OnPackageAttributesSizeInfo(uint32 stringCount,
									uint32 uncompressedSize);
	virtual	void				OnPackageSizeInfo(uint32 headerSize,
									uint64 heapSize, uint64 tocSize,
									uint32 packageAttributesSize,
									uint64 totalSize);

private:
			bool				fVerbose;
			bool				fQuiet;
};


#endif	// PACKAGE_WRITER_LISTENER_H
