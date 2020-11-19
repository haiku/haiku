/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "FetchUtils.h"
#include "string.h"

#include <Entry.h>
#include <Node.h>
#include <TypeConstants.h>

namespace BPackageKit {

namespace BPrivate {


#ifdef HAIKU_TARGET_PLATFORM_HAIKU

#define DL_COMPLETE_ATTR "Meta:DownloadCompleted"


/*static*/ bool
FetchUtils::IsDownloadCompleted(const char* path)
{
	BEntry entry(path, true);
	BNode node(&entry);
	return IsDownloadCompleted(node);
}


/*static*/ bool
FetchUtils::IsDownloadCompleted(BNode& node)
{
    bool isComplete;
    status_t status = _GetAttribute(node, DL_COMPLETE_ATTR,
        B_BOOL_TYPE, &isComplete, sizeof(isComplete));
    if (status != B_OK) {
        // Most likely cause is that the attribute was not written,
        // for example by previous versions of the Package Kit.
        // Worst outcome of assuming a partial download should be
        // a no-op range request.
        isComplete = false;
    }
    return isComplete;
}


/*static*/ status_t
FetchUtils::MarkDownloadComplete(BNode& node)
{
    bool isComplete = true;
    return _SetAttribute(node, DL_COMPLETE_ATTR,
        B_BOOL_TYPE, &isComplete, sizeof(isComplete));
}


/*static*/ status_t
FetchUtils::SetFileType(BNode& node, const char* type)
{
	return _SetAttribute(node, "BEOS:TYPE",
        B_MIME_STRING_TYPE, type, strlen(type) + 1);
}

status_t
FetchUtils::_SetAttribute(BNode& node, const char* attrName,
    type_code type, const void* data, size_t size)
{
	if (node.InitCheck() != B_OK)
		return node.InitCheck();

	ssize_t written = node.WriteAttr(attrName, type, 0, data, size);
	if (written != (ssize_t)size) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}
	return B_OK;
}


status_t
FetchUtils::_GetAttribute(BNode& node, const char* attrName,
    type_code type, void* data, size_t size)
{
	if (node.InitCheck() != B_OK)
		return node.InitCheck();

	ssize_t read = node.ReadAttr(attrName, type, 0, data, size);
	if (read != (ssize_t)size) {
		if (read < 0)
			return (status_t)read;
		return B_IO_ERROR;
	}
	return B_OK;
}


#endif // HAIKU_TARGET_PLATFORM_HAIKU

}	// namespace BPrivate

}	// namespace BPackageKit
