/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern const char *__progname;


static void
usage(void)
{
	fprintf(stderr, "usage: %s <error number>\n"
		"Prints clear text error messages for given error numbers.\n", __progname);
	exit(-1);
}


static void
print_error(char *number)
{
	char *end;
	int32 error = (int32)strtoll(number, &end, 0);
		// strtol() cuts off hex numbers that have the highest bit set

	if (end[0]) {
		fprintf(stderr, "%s: invalid number (%s)\n", __progname, number);
		exit(1);
	}

	printf("0x%lx: %s\n", error, strerror(error));
}


int
main(int argc, char *argv[])
{
	int32 i;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++) {
		print_error(argv[i]);
	}

	return 0;
}
