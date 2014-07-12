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

#include <FdIO.h>

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


// #pragma mark - PackagesAttributeHandler


class RepositoryReaderImpl::PackagesAttributeHandler
	: public AttributeHandler {
private:
	typedef AttributeHandler super;
public:
	PackagesAttributeHandler(BRepositoryContentHandler* contentHandler)
		:
		fContentHandler(contentHandler),
		fPackageName(NULL)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context, uint8 id,
		const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_PACKAGE:
			{
				status_t error = _NotifyPackageDone();
				if (error != B_OK)
					return error;

				if (_handler != NULL) {
					if (fContentHandler != NULL) {
						error = fContentHandler->HandlePackage(value.string);
						if (error != B_OK)
							return error;
					}

					*_handler = new(std::nothrow) PackageAttributeHandler;
					if (*_handler == NULL)
						return B_NO_MEMORY;

					fPackageName = value.string;
				}
				break;
			}

			default:
				if (context->ignoreUnknownAttributes)
					break;

				context->errorOutput->PrintError(
					"Error: Invalid package attribute section: unexpected "
					"top level attribute id %d encountered\n", id);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t NotifyDone(AttributeHandlerContext* context)
	{
		status_t result = _NotifyPackageDone();
		if (result == B_OK)
			result = super::NotifyDone(context);
		return result;
	}

private:
	status_t _NotifyPackageDone()
	{
		if (fPackageName == NULL || fContentHandler == NULL)
			return B_OK;

		status_t error = fContentHandler->HandlePackageDone(fPackageName);
		fPackageName = NULL;
		return error;
	}

private:
	BRepositoryContentHandler*	fContentHandler;
	const char*					fPackageName;
};


// #pragma mark - PackageContentHandlerAdapter


class RepositoryReaderImpl::PackageContentHandlerAdapter
	: public BPackageContentHandler {
public:
	PackageContentHandlerAdapter(BRepositoryContentHandler* contentHandler)
		:
		fContentHandler(contentHandler)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return fContentHandler->HandlePackageAttribute(value);
	}

	virtual void HandleErrorOccurred()
	{
		return fContentHandler->HandleErrorOccurred();
	}

private:
	BRepositoryContentHandler*	fContentHandler;
};


// #pragma mark - RepositoryReaderImpl


RepositoryReaderImpl::RepositoryReaderImpl(BErrorOutput* errorOutput)
	:
	inherited("repository", errorOutput)
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
	BFdIO* file = new(std::nothrow) BFdIO(fd, keepFD);
	if (file == NULL) {
		if (keepFD && fd >= 0)
			close(fd);
		return B_NO_MEMORY;
	}

	return Init(file, true);
}


status_t
RepositoryReaderImpl::Init(BPositionIO* file, bool keepFile)
{
	hpkg_repo_header header;
	status_t error = inherited::Init<hpkg_repo_header, B_HPKG_REPO_MAGIC,
		B_HPKG_REPO_VERSION, B_HPKG_REPO_MINOR_VERSION>(file, keepFile, header,
		0);
	if (error != B_OK)
		return error;

	// init package attributes section
	error = InitSection(fPackageAttributesSection,
		UncompressedHeapSize(),
		B_BENDIAN_TO_HOST_INT64(header.packages_length),
		kMaxPackageAttributesSize,
		B_BENDIAN_TO_HOST_INT64(header.packages_strings_length),
		B_BENDIAN_TO_HOST_INT64(header.packages_strings_count));
	if (error != B_OK)
		return error;

	// init repository info section
	PackageFileSection repositoryInfoSection("repository info");
	error = InitSection(repositoryInfoSection,
		fPackageAttributesSection.offset,
		B_BENDIAN_TO_HOST_INT32(header.info_length), kMaxRepositoryInfoSize, 0,
		0);
	if (error != B_OK)
		return error;

	// prepare the sections for use
	error = PrepareSection(repositoryInfoSection);
	if (error != B_OK)
		return error;

	error = PrepareSection(fPackageAttributesSection);
	if (error != B_OK)
		return error;

	// unarchive repository info
	BMessage repositoryInfoArchive;
	error = repositoryInfoArchive.Unflatten((char*)repositoryInfoSection.data);
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
		PackageContentHandlerAdapter contentHandlerAdapter(contentHandler);
		AttributeHandlerContext context(ErrorOutput(),
			contentHandler != NULL ? &contentHandlerAdapter : NULL,
			B_HPKG_SECTION_PACKAGE_ATTRIBUTES,
			MinorFormatVersion() > B_HPKG_REPO_MINOR_VERSION);
		PackagesAttributeHandler rootAttributeHandler(contentHandler);
		result = ParsePackageAttributesSection(&context, &rootAttributeHandler);
	}
	return result;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
