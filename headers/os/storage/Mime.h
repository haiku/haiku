//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Mime.h
	Mime type C functions interface declarations.
*/
#ifndef _sk_mime_h_
#define _sk_mime_h_

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

static const uint32 B_MIME_STRING_TYPE	= 'MIMS';

enum icon_size {
	B_LARGE_ICON	= 32,
	B_MINI_ICON		= 16
};

#ifdef __cplusplus
}
#endif

// include the C++ API
#ifdef __cplusplus
#include <MimeType.h>
#endif

#endif	// _sk_mime_h_
