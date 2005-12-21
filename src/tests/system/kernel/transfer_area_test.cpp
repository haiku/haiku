/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <syscalls.h>

#include <stdio.h>
#include <string.h>


static const char *kPortName = "transfer area test";


int
main(int argc, char **argv)
{
	port_id port;
	area_id area;

	if (argc > 1) {
		// we're the sender
		port = find_port(kPortName);
		if (port < B_OK) {
			fprintf(stderr, "Area receiver is not yet running.\n");
			return -1;
		}

		port_info info;
		get_port_info(port, &info);
		
		char *address;
		area = create_area("test transfer area", (void **)&address, B_ANY_ADDRESS, B_PAGE_SIZE,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			fprintf(stderr, "Could not create area: %s.\n", strerror(area));
			return -1;
		}

		sprintf(address, "Oh my god - it's working! (%s)", argv[1]);

		status_t status = _kern_transfer_area(area, (void **)&address, B_ANY_ADDRESS, info.team);
		if (status < B_OK) {
			fprintf(stderr, "Could not transfer area: %s.\n", strerror(status));
			return -1;
		}

		write_port(port, area, NULL, 0);
	} else {
		// we're the receiver
		port = create_port(1, kPortName);
		if (port < B_OK) {
			fprintf(stderr, "Could not create port: %s.\n", strerror(area));
			return -1;
		}

		puts("Waiting for an area to be received (start same command again with an argument)...");

		ssize_t size;
		if ((size = read_port(port, (int32 *)&area, NULL, 0)) < B_OK) {
			fprintf(stderr, "Reading from port failed: %s.\n", strerror(size));
			return -1;
		}

		printf("Received Area %ld\n", area);

		area_info info;
		get_area_info(area, &info);
		printf("  name = \"%s\", base = %p, size = %#lx, team = %ld\n",
			info.name, info.address, info.size, info.team);
		printf("  contents: %s\n", (char *)info.address);

		delete_area(area);
	}

	return 0;
}
