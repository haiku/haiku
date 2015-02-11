/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Atis Elsts, the.kfx@gmail.com
 */
#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H


#include <stddef.h>


const char* get_media_type_name(size_t index);
const char* get_media_subtype_name(size_t typeIndex, size_t subIndex);
bool media_parse_subtype(const char* string, int media, int* type);
const char* media_type_to_string(int media);


#endif // MEDIA_TYPES_H
