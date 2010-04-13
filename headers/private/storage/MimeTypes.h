/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_TYPES_H
#define _MIME_TYPES_H


#include <Mime.h>


// Additional MIME types that are not defined in Mime.h but are
// standard values. We should move them into a public space some
// day.

#define	B_DIRECTORY_MIME_TYPE	"application/x-vnd.Be-directory"
#define	B_VOLUME_MIME_TYPE		"application/x-vnd.Be-volume"
#define	B_SYMLINK_MIME_TYPE		"application/x-vnd.Be-symlink"
#define	B_ROOT_MIME_TYPE		"application/x-vnd.Be-root"

#endif	/* _MIME_TYPES_H */
