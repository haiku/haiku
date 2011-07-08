/*
 * Copyright 2004-2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!
	\file Mime.h
	Mime type C functions interface declarations.
*/
#ifndef _MIME_H
#define _MIME_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <sys/types.h>
#include <SupportDefs.h>
#include <StorageDefs.h>

// C functions

#ifdef __cplusplus
extern "C" {
#endif

int update_mime_info(const char *path, int recursive, int synchronous,
					 int force);

status_t create_app_meta_mime(const char *path, int recursive, int synchronous,
							  int force);

status_t get_device_icon(const char *dev, void *icon, int32 size);


enum icon_size {
	B_LARGE_ICON	= 32,
	B_MINI_ICON		= 16
};

// values for the "force" parameter of update_mime_info() (Haiku only)
enum {
	B_UPDATE_MIME_INFO_NO_FORCE			= 0,
	B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE	= 1,
	B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL	= 2,
};

#ifdef __cplusplus
}
#endif

// Haiku only!
#ifdef __cplusplus

class BBitmap;

status_t get_device_icon(const char *dev, BBitmap *icon, icon_size which);

#endif

// include the C++ API
#ifdef __cplusplus
#include <MimeType.h>
#endif

#endif	// _MIME_H
