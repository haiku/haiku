/*
 * Copyright 2020, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__FETCH_UTILS_H_
#define _PACKAGE__PRIVATE__FETCH_UTILS_H_


#include "SupportDefs.h"
#include <Node.h>

namespace BPackageKit {

namespace BPrivate {


class FetchUtils {
public:
	static	bool				IsDownloadCompleted(const char* path);
	static	bool				IsDownloadCompleted(BNode& node);

	static	status_t			MarkDownloadComplete(const char* path);
	static	status_t			MarkDownloadComplete(BNode& node);

	static	status_t			SetFileType(BNode& node, const char* type);

private:
	static	status_t			_SetAttribute(BNode& node,
									const char* attrName,
									type_code type, const void* data,
									size_t size);
	static	status_t			_GetAttribute(BNode& node,
									const char* attrName,
									type_code type, void* data,
									size_t size);
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__FETCH_UTILS_H_
