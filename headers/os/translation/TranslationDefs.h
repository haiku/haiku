/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATION_DEFS_H
#define _TRANSLATION_DEFS_H


#include <SupportDefs.h>


#define B_TRANSLATION_CURRENT_VERSION	B_BEOS_VERSION
#define B_TRANSLATION_MIN_VERSION		161

#define B_TRANSLATION_MAKE_VERSION(major, minor, revision) \
	((major << 8) | ((minor << 4) & 0xf0) | (revision & 0x0f))
#define B_TRANSLATION_MAJOR_VERSION(v)		(v >> 8)
#define B_TRANSLATION_MINOR_VERSION(v)		((v >> 4) & 0xf)
#define B_TRANSLATION_REVISION_VERSION(v)	(v & 0xf)


extern const char* B_TRANSLATOR_MIME_TYPE;

typedef unsigned long translator_id;


struct translation_format {
	uint32		type;				// type_code
	uint32		group;
	float		quality;			// between 0.0 and 1.0
	float		capability;			// between 0.0 and 1.0
	char		MIME[251];
	char		name[251];
};

struct translator_info {
	uint32			type;
	translator_id	translator;
	uint32			group;
	float			quality;
	float			capability;
	char			name[251];
	char			MIME[251];
};


#endif	// _TRANSLATION_DEFS_H
