/*
 * Copyright 2014 Jonathan Schleifer <js@webkeks.org>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonathan Schleifer, js@webkeks.org
 *		John Scipione, jscipione@gmail.com
 */
#ifndef CONVERT_UTF_H
#define CONVERT_UTF_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif


ssize_t utf16le_to_utf8(const uint16* source, size_t sourceCodeUnitCount,
	char* target, size_t targetLength);

ssize_t utf16be_to_utf8(const uint16* source, size_t sourceCodeUnitCount,
	char* target, size_t targetLength);


#ifdef __cplusplus
};
#endif


#endif	// CONVERT_UTF_H
