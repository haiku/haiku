/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXIF_PARSER_H
#define EXIF_PARSER_H


#include <DataIO.h>
#include <TypeConstants.h>

class BMessage;


struct convert_tag {
	uint16		tag;
	type_code	type;
	const char*	name;
};

status_t convert_exif_to_message(BPositionIO& source, BMessage& target);
status_t convert_exif_to_message_etc(BPositionIO& source, BMessage& target,
	const convert_tag* tags, size_t tagCount);

#endif	// EXIF_PARSER_H
