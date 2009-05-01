/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _UNICODE_PROPERTIES_H_
#define _UNICODE_PROPERTIES_H_


#include <SupportDefs.h>
#include <ByteOrder.h>


#define PROPERTIES_DIRECTORY	"locale"
#define PROPERTIES_FILE_NAME	"unicode.properties"
#define PROPERTIES_FORMAT		'UPro'


typedef struct {
	uint8	size;
    uint8	isBigEndian;
    uint32	format;
    uint8	version[3];
} UnicodePropertiesHeader;

#endif	/* _UNICODE_PROPERTIES_H_ */
