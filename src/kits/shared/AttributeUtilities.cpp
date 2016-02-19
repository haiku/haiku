/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeUtilities.h"

#include <fs_attr.h>
#include <Node.h>
#include <StorageDefs.h>


static const int kCopyBufferSize = 64 * 1024;
	// 64 KB


namespace BPrivate {


status_t
CopyAttributes(BNode& source, BNode& destination)
{
	char attrName[B_ATTR_NAME_LENGTH];
	while (source.GetNextAttrName(attrName) == B_OK) {
		// get attr info
		attr_info attrInfo;
		status_t status = source.GetAttrInfo(attrName, &attrInfo);
		if (status != B_OK)
			return status;

		// copy the attribute
		char buffer[kCopyBufferSize];
		off_t offset = 0;
		off_t bytesLeft = attrInfo.size;

		// Go at least once through the loop, so that an empty attribute will be
		// created as well
		do {
			size_t toRead = kCopyBufferSize;
			if ((off_t)toRead > bytesLeft)
				toRead = bytesLeft;

			// read
			ssize_t bytesRead = source.ReadAttr(attrName, attrInfo.type,
				offset, buffer, toRead);
			if (bytesRead < 0)
				return bytesRead;

			// write
			ssize_t bytesWritten = destination.WriteAttr(attrName,
				attrInfo.type, offset, buffer, bytesRead);
			if (bytesWritten < 0)
				return bytesWritten;

			bytesLeft -= bytesRead;
			offset += bytesRead;
		} while (bytesLeft > 0);
	}
	return B_OK;
}


}	// namespace BPrivate

