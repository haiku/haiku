/*
 * Copyright 2004-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_H
#define _MIME_H


#include <sys/types.h>

#include <SupportDefs.h>
#include <StorageDefs.h>
#include <TypeConstants.h>


enum icon_size {
	B_LARGE_ICON	= 32,
	B_MINI_ICON		= 16
};

/* values for the "force" parameter of update_mime_info() (Haiku only) */
enum {
	B_UPDATE_MIME_INFO_NO_FORCE			= 0,
	B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE	= 1,
	B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL	= 2,
};


/* C functions */

#ifdef __cplusplus
extern "C" {
#endif

int update_mime_info(const char* path, int recursive, int synchronous,
	int force);
status_t create_app_meta_mime(const char* path, int recursive, int synchronous,
	int force);
status_t get_device_icon(const char* device, void* icon, int32 size);

#ifdef __cplusplus
}

/* C++ functions, Haiku only! */

class BBitmap;

status_t get_device_icon(const char* device, BBitmap* icon, icon_size which);
status_t get_device_icon(const char* device, uint8** _data, size_t* _size,
	type_code* _type);

status_t get_named_icon(const char* name, BBitmap* icon, icon_size which);
status_t get_named_icon(const char* name, uint8** _data, size_t* _size,
	type_code* _type);

/* include MimeType.h for convenience */
#	include <MimeType.h>
#endif	/* __cplusplus */

#endif	/* _MIME_H */
