/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef MIME_TYPES_H
#define MIME_TYPES_H

struct ext_mime {
	char *extension;
	char *mime;
};

extern struct ext_mime mimes[];

#endif
