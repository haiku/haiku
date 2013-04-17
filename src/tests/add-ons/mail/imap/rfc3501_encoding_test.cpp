/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include "Response.h"


void
assertEquals(const char* expected, const char* result)
{
	if (strcmp(expected, result) != 0) {
		printf("Expected \"%s\", got \"%s\"\n", expected, result);
		exit(EXIT_FAILURE);
	}
}


int
main()
{
	const char* samples[] = {
		"Gelöscht", "Gel&APY-scht",
		"&äöß", "&-&AOQA9gDf-"
	};

	IMAP::RFC3501Encoding encoding;

	for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i += 2) {
		BString encoded = encoding.Encode(samples[i]);
		assertEquals(samples[i + 1], encoded);
		BString decoded = encoding.Decode(encoded);
		assertEquals(samples[i], decoded);
	}
}
