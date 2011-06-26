/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingAttributeDirectoryCookie.h"

#include "DebugSupport.h"
#include "PackageNode.h"
#include "Utils.h"


UnpackingAttributeDirectoryCookie::UnpackingAttributeDirectoryCookie(
	PackageNode* packageNode)
	:
	fPackageNode(packageNode),
	fAttribute(NULL),
	fState(AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST)
{
	if (fPackageNode != NULL) {
		fPackageNode->AcquireReference();
		fAttribute = fPackageNode->Attributes().Head();
	}
}


UnpackingAttributeDirectoryCookie::~UnpackingAttributeDirectoryCookie()
{
	if (fPackageNode != NULL)
		fPackageNode->ReleaseReference();
}


/*static*/ status_t
UnpackingAttributeDirectoryCookie::Open(PackageNode* packageNode,
	AttributeDirectoryCookie*& _cookie)
{
	UnpackingAttributeDirectoryCookie* cookie = new(std::nothrow)
		UnpackingAttributeDirectoryCookie(packageNode);
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
UnpackingAttributeDirectoryCookie::Read(dev_t volumeID, ino_t nodeID,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	uint32 maxCount = *_count;
	uint32 count = 0;

	dirent* previousEntry = NULL;

	while (fState < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT || fAttribute != NULL) {
		// don't read more entries than requested
		if (count >= maxCount)
			break;

		// align the buffer for subsequent entries
		if (count > 0) {
			addr_t offset = (addr_t)buffer % 8;
			if (offset > 0) {
				offset = 8 - offset;
				if (bufferSize <= offset)
					break;

				previousEntry->d_reclen += offset;
				buffer = (dirent*)((addr_t)buffer + offset);
				bufferSize -= offset;
			}
		}

		// get the attribute name
		const char* name;
		if (fState < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT) {
			name = AutoPackageAttributes::NameForAttribute(
				(AutoPackageAttribute)fState);
		} else
			name = fAttribute->Name();

		// fill in the entry name -- checks whether the entry fits into the
		// buffer
		if (!set_dirent_name(buffer, bufferSize, name, strlen(name))) {
			if (count == 0)
				RETURN_ERROR(B_BUFFER_OVERFLOW);
			break;
		}

		// fill in the other data
		buffer->d_dev = volumeID;
		buffer->d_ino = nodeID;

		count++;
		previousEntry = buffer;
		bufferSize -= buffer->d_reclen;
		buffer = (dirent*)((addr_t)buffer + buffer->d_reclen);

		if (fState < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT)
			fState++;
		else
			fAttribute = fPackageNode->Attributes().GetNext(fAttribute);
	}

	*_count = count;
	return B_OK;
}


status_t
UnpackingAttributeDirectoryCookie::Rewind()
{
	if (fPackageNode != NULL)
		fAttribute = fPackageNode->Attributes().Head();

	fState = AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST;

	return B_OK;
}
