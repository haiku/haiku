/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "PackageWriterListener.h"

#include <stdio.h>


using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriter;


PackageWriterListener::PackageWriterListener(bool verbose, bool quiet)
	:
	fVerbose(verbose), fQuiet(quiet)
{
}


void
PackageWriterListener::PrintErrorVarArgs(const char* format, va_list args)
{
	vfprintf(stderr, format, args);
}


void
PackageWriterListener::OnEntryAdded(const char* path)
{
	if (fQuiet || !fVerbose)
		return;

	printf("\t%s\n", path);
}


void
PackageWriterListener::OnTOCSizeInfo(uint64 uncompressedStringsSize,
	uint64 uncompressedMainSize, uint64 uncompressedTOCSize)
{
	if (fQuiet || !fVerbose)
		return;

	printf("----- TOC Info -----------------------------------\n");
	printf("cached strings size:     %10" B_PRIu64 " (uncompressed)\n",
		uncompressedStringsSize);
	printf("TOC main size:           %10" B_PRIu64 " (uncompressed)\n",
		uncompressedMainSize);
	printf("total TOC size:          %10" B_PRIu64 " (uncompressed)\n",
		uncompressedTOCSize);
}


void
PackageWriterListener::OnPackageAttributesSizeInfo(uint32 stringCount,
	uint32 uncompressedSize)
{
	if (fQuiet || !fVerbose)
		return;

	printf("----- Package Attribute Info ---------------------\n");
	printf("string count:            %10" B_PRIu32 "\n", stringCount);
	printf("package attributes size: %10" B_PRIu32 " (uncompressed)\n",
		uncompressedSize);
}


void
PackageWriterListener::OnPackageSizeInfo(uint32 headerSize, uint64 heapSize,
	uint64 tocSize, uint32 packageAttributesSize, uint64 totalSize)
{
	if (fQuiet)
		return;

	printf("----- Package Info ----------------\n");
	printf("header size:             %10" B_PRIu32 "\n", headerSize);
	printf("heap size:               %10" B_PRIu64 "\n", heapSize);
	printf("TOC size:                %10" B_PRIu64 "\n", tocSize);
	printf("package attributes size: %10" B_PRIu32 "\n",
		packageAttributesSize);
	printf("total size:              %10" B_PRIu64 "\n", totalSize);
	printf("-----------------------------------\n");
}
