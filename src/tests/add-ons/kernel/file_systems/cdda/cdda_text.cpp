/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cdda.h"
#include "cddb.h"


extern const char* __progname;


extern "C" status_t
user_memcpy(void *dest, const void *source, size_t length)
{
	memcpy(dest, source, length);
	return B_OK;
}


extern "C" void
dprintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	fflush(stdout);
	va_end(args);
}


int
main(int argc, char** argv)
{
	if (argc < 2)
		return -1;
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		return -1;

	uint8 buffer[1024];
	scsi_toc_toc *toc = (scsi_toc_toc *)buffer;

	status_t status = read_table_of_contents(fd, toc, sizeof(buffer));
	if (status != B_OK) {
		fprintf(stderr, "%s: Retrieving TOC failed: %s\n", __progname,
			strerror(status));
		return -1;
	}

	cdtext text;
	read_cdtext(fd, text);

	uint32 id = compute_cddb_disc_id(*toc);
	printf("CDDB disc ID: %lx\n", id);

	close(fd);
}
