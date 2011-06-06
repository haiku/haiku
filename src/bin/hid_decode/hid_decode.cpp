/*
 * Copyright 2010, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include "HIDCollection.h"
#include "HIDParser.h"
#include "HIDReport.h"

#include <File.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: %s <hid_descriptor_file>\n", argv[0]);
		return 1;
	}

	BFile file(argv[1], B_READ_ONLY);
	if (!file.IsReadable()) {
		printf("can't open file \"%s\" for reading\n", argv[1]);
		return 2;
	}

	off_t descriptorLength;
	file.GetSize(&descriptorLength);

	uint8 *reportDescriptor = (uint8 *)malloc(descriptorLength);
	if (reportDescriptor == NULL) {
		printf("failed to allocate buffer of %lld bytes\n", descriptorLength);
		return 3;
	}

	ssize_t read = file.Read(reportDescriptor, descriptorLength);
	if (read != descriptorLength) {
		printf("failed to read file of %lld bytes: %s\n", descriptorLength,
			strerror(read));
		return 4;
	}

	HIDParser parser(NULL);
	status_t result = parser.ParseReportDescriptor(reportDescriptor,
		descriptorLength);

	free(reportDescriptor);
	if (result != B_OK) {
		printf("failed to parse descriptor: %s\n", strerror(result));
		return 5;
	}

	parser.PrintToStream();
	return 0;
}
