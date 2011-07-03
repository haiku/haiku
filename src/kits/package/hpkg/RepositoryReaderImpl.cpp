/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryReaderImpl.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <Message.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/RepositoryContentHandler.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


//#define TRACE(format...)	printf(format)
#define TRACE(format...)	do {} while (false)


// maximum repository info size we support reading
static const size_t kMaxRepositoryInfoSize		= 1 * 1024 * 1024;

// maximum package attributes size we support reading
static const size_t kMaxPackageAttributesSize	= 64 * 1024 * 1024;


RepositoryReaderImpl::RepositoryReaderImpl(BErrorOutput* errorOutput)
	:
	inherited(errorOutput),
	fRepositoryInfoSection("repository info")
{
}


RepositoryReaderImpl::~RepositoryReaderImpl()
{
}


status_t
RepositoryReaderImpl::Init(const char* fileName)
{
	// open file
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		ErrorOutput()->PrintError(
			"Error: Failed to open repository file \"%s\": %s\n", fileName,
			strerror(errno));
		return errno;
	}

	return Init(fd, true);
}


status_t
RepositoryReaderImpl::Init(int fd, bool keepFD)
{
	status_t error = inherited::Init(fd, keepFD);
	if (error != B_OK)
		return error;

	// stat it
	struct stat st;
	if (fstat(FD(), &st) < 0) {
		ErrorOutput()->PrintError(
			"Error: Failed to access repository file: %s\n", strerror(errno));
		return errno;
	}

	// read the header
	hpkg_repo_header header;
	if ((error = ReadBuffer(0, &header, sizeof(header))) != B_OK)
		return error;

	// check the header

	// magic
	if (B_BENDIAN_TO_HOST_INT32(header.magic) != B_HPKG_REPO_MAGIC) {
		ErrorOutput()->PrintError("Error: Invalid repository file: Invalid "
			"magic\n");
		return B_BAD_DATA;
	}

	// header size
	size_t headerSize = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if (headerSize < sizeof(hpkg_repo_header)) {
		ErrorOutput()->PrintError("Error: Invalid repository file: Invalid "
			"header size (%llu)\n", headerSize);
		return B_BAD_DATA;
	}

	// version
	if (B_BENDIAN_TO_HOST_INT16(header.version) != B_HPKG_REPO_VERSION) {
		ErrorOutput()->PrintError("Error: Invalid/unsupported repository file "
			"version (%d)\n", B_BENDIAN_TO_HOST_INT16(header.version));
		return B_BAD_DATA;
	}

	// total size
	uint64 totalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (totalSize != (uint64)st.st_size) {
		ErrorOutput()->PrintError("Error: Invalid repository file: Total size "
			"in header (%llu) doesn't agree with total file size (%lld)\n",
			totalSize, st.st_size);
		return B_BAD_DATA;
	}

	// repository info length and compression
	fRepositoryInfoSection.compression
		= B_BENDIAN_TO_HOST_INT32(header.info_compression);
	fRepositoryInfoSection.compressedLength
		= B_BENDIAN_TO_HOST_INT32(header.info_length_compressed);
	fRepositoryInfoSection.uncompressedLength
		= B_BENDIAN_TO_HOST_INT32(header.info_length_uncompressed);

	if (const char* errorString = CheckCompression(fRepositoryInfoSection)) {
		ErrorOutput()->PrintError(
			"Error: Invalid repository file: info section: %s\n", errorString);
		return B_BAD_DATA;
	}

	// package attributes length and compression
	fPackageAttributesSection.compression
		= B_BENDIAN_TO_HOST_INT32(header.packages_compression);
	fPackageAttributesSection.compressedLength
		= B_BENDIAN_TO_HOST_INT64(header.packages_length_compressed);
	fPackageAttributesSection.uncompressedLength
		= B_BENDIAN_TO_HOST_INT64(header.packages_length_uncompressed);
	fPackageAttributesSection.stringsLength
		= B_BENDIAN_TO_HOST_INT64(header.packages_strings_length);
	fPackageAttributesSection.stringsCount
		= B_BENDIAN_TO_HOST_INT64(header.packages_strings_count);

	if (const char* errorString = CheckCompression(
		fPackageAttributesSection)) {
		ErrorOutput()->PrintError("Error: Invalid repository file: package "
			"attributes section: %s\n", errorString);
		return B_BAD_DATA;
	}

	// check whether the sections fit together
	if (fPackageAttributesSection.compressedLength > totalSize
		|| fRepositoryInfoSection.compressedLength
			> totalSize - fPackageAttributesSection.compressedLength) {
		ErrorOutput()->PrintError("Error: Invalid repository file: The sum of "
			"the sections sizes is greater than the repository size\n");
		return B_BAD_DATA;
	}

	fPackageAttributesSection.offset
		= totalSize - fPackageAttributesSection.compressedLength;
	fRepositoryInfoSection.offset = fPackageAttributesSection.offset
		- fRepositoryInfoSection.compressedLength;

	// repository info size sanity check
	if (fRepositoryInfoSection.uncompressedLength > kMaxRepositoryInfoSize) {
		ErrorOutput()->PrintError("Error: Repository file info section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fRepositoryInfoSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// package attributes size sanity check
	if (fPackageAttributesSection.uncompressedLength
			> kMaxPackageAttributesSize) {
		ErrorOutput()->PrintError(
			"Error: Package file package attributes section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fPackageAttributesSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// read in the complete repository info section
	fRepositoryInfoSection.data
		= new(std::nothrow) uint8[fRepositoryInfoSection.uncompressedLength];
	if (fRepositoryInfoSection.data == NULL) {
		ErrorOutput()->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = ReadCompressedBuffer(fRepositoryInfoSection);
	if (error != B_OK)
		return error;

	// read in the complete package attributes section
	fPackageAttributesSection.data
		= new(std::nothrow) uint8[fPackageAttributesSection.uncompressedLength];
	if (fPackageAttributesSection.data == NULL) {
		ErrorOutput()->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = ReadCompressedBuffer(fPackageAttributesSection);
	if (error != B_OK)
		return error;

	// unarchive repository info
	BMessage repositoryInfoArchive;
	error = repositoryInfoArchive.Unflatten((char*)fRepositoryInfoSection.data);
	if (error != B_OK) {
		ErrorOutput()->PrintError(
			"Error: Unable to unflatten repository info archive!\n");
		return error;
	}
	error = fRepositoryInfo.SetTo(&repositoryInfoArchive);
	if (error != B_OK) {
		ErrorOutput()->PrintError(
			"Error: Unable to unarchive repository info!\n");
		return error;
	}

	// parse strings from package attributes section
	fPackageAttributesSection.currentOffset = 0;
	SetCurrentSection(&fPackageAttributesSection);

	// strings
	error = ParseStrings();
	if (error != B_OK)
		return error;

	SetCurrentSection(NULL);

	return B_OK;
}


status_t
RepositoryReaderImpl::GetRepositoryInfo(BRepositoryInfo* _repositoryInfo) const
{
	if (_repositoryInfo == NULL)
		return B_BAD_VALUE;

	*_repositoryInfo = fRepositoryInfo;
	return B_OK;
}


status_t
RepositoryReaderImpl::ParseContent(BRepositoryContentHandler* contentHandler)
{
	status_t result = contentHandler->HandleRepositoryInfo(fRepositoryInfo);
	if (result == B_OK) {
		AttributeHandlerContext context(ErrorOutput(), contentHandler,
			B_HPKG_SECTION_PACKAGE_ATTRIBUTES);
		PackageAttributeHandler rootAttributeHandler;
		result = ParsePackageAttributesSection(&context, &rootAttributeHandler);
	}
	return result;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
