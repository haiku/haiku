/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AutoPackageAttributeDirectoryCookie.h"

#include "DebugSupport.h"
#include "Utils.h"


AutoPackageAttributeDirectoryCookie::AutoPackageAttributeDirectoryCookie()
	:
	fState(AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST)
{
}


AutoPackageAttributeDirectoryCookie::~AutoPackageAttributeDirectoryCookie()
{
}


status_t
AutoPackageAttributeDirectoryCookie::Read(dev_t volumeID, ino_t nodeID,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	uint32 maxCount = *_count;
	uint32 count = 0;

	dirent* previousEntry = NULL;

	String customAttributeName = CurrentCustomAttributeName();

	while (fState < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT
		|| !customAttributeName.IsEmpty()) {
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
		const String& name = fState < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT
			? AutoPackageAttributes::NameForAttribute(
				(AutoPackageAttribute)fState)
			: customAttributeName;

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
			customAttributeName = NextCustomAttributeName();
	}

	*_count = count;
	return B_OK;
}


status_t
AutoPackageAttributeDirectoryCookie::Rewind()
{
	fState = AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST;

	return B_OK;
}


String
AutoPackageAttributeDirectoryCookie::CurrentCustomAttributeName()
{
	return String();
}


String
AutoPackageAttributeDirectoryCookie::NextCustomAttributeName()
{
	return String();
}
