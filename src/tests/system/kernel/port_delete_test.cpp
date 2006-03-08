/*
 * Copyright 2006, Marcus Overhagen, <marcus@overhagen.de>
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <stdio.h>
#include <string.h>

int
main()
{
	port_id id;
	status_t s;
	ssize_t size;
	int32 code;
	
	char data[100];
	
	
	id = create_port(10, "test port");
	printf("created port %ld\n", id);
	
	s = write_port(id, 0x1234, data, 10);
	printf("write port result 0x%08lx (%s)\n", s, strerror(s));

	s = write_port(id, 0x5678, data, 20);
	printf("write port result 0x%08lx (%s)\n", s, strerror(s));
	
	s = delete_port(id);
	printf("delete port result 0x%08lx (%s)\n", s, strerror(s));

	printf("everything should fail now\n");

	// BeBook: does return B_BAD_PORT_ID if port was closed
	s = write_port(id, 0x5678, data, 20);
	printf("write port result 0x%08lx (%s)\n", s, strerror(s));

	// BeBook: does block when port is empty, and unblocks when port is written to or deleted
	size = port_buffer_size(id); 
	printf("port_buffer_size %ld (0x%08lx) (%s)\n", size, size, strerror(size));

	// BeBook: does block when port is empty, and unblocks when port is written to or deleted
	size = read_port(id, &code, data, sizeof(data)); 
	printf("read port code %lx, size %ld (0x%08lx) (%s)\n", code, size, size, strerror(size));
	
	return 0;
}
