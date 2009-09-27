/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdio.h>
#include <stdlib.h>


#define LINE_LENGTH 4096


char *
fgetln(FILE *stream, size_t *_length)
{
	// TODO: this function is not thread-safe
	static size_t sBufferSize;
	static char *sBuffer;

	size_t length, left;
	char *line;

	if (sBuffer == NULL) {
		sBuffer = (char *)malloc(LINE_LENGTH);
		if (sBuffer == NULL)
			return NULL;

		sBufferSize = LINE_LENGTH;
	}

	line = sBuffer;
	left = sBufferSize;
	if (_length != NULL)
		*_length = 0;

	for (;;) {
		line = fgets(line, left, stream);
		if (line == NULL) {
			free(sBuffer);
			sBuffer = NULL;
			return NULL;
		}

		length = strlen(line);
		if (_length != NULL)
			*_length += length;
		if (line[length - 1] != '\n' && length == sBufferSize - 1) {
			// more data is following, enlarge buffer
			char *newBuffer = realloc(sBuffer, sBufferSize + LINE_LENGTH);
			if (newBuffer == NULL) {
				free(sBuffer);
				sBuffer = NULL;
				return NULL;
			}

			sBuffer = newBuffer;
			sBufferSize += LINE_LENGTH;
			line = sBuffer + length;
			left += 1;
		} else
			break;
	}

	return sBuffer;
}

