/*
 * Copyright 2005-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BOOT_REMOTE_DISK_DEFS_H
#define _BOOT_REMOTE_DISK_DEFS_H


#include <inttypes.h>


enum {
	REMOTE_DISK_SERVER_PORT	= 8765,
	REMOTE_DISK_BLOCK_SIZE	= 1024,
};

enum {
	// requests

	REMOTE_DISK_HELLO_REQUEST	= 0,
		// port: client port

	REMOTE_DISK_READ_REQUEST	= 1,
		// port: client port
		// offset: byte offset of data to read
		// size: number of bytes to read (server might serve more, though)

	REMOTE_DISK_WRITE_REQUEST	= 2,
		// port: client port
		// offset: byte offset of data to write
		// size: number of bytes to write
		// data: the data

	// replies

	REMOTE_DISK_HELLO_REPLY		= 3,
		// offset: disk size

	REMOTE_DISK_READ_REPLY		= 4,	// port unused
		// offset: byte offset of read data
		// size: number of bytes of data read; < 0 => error
		// data: read data

	REMOTE_DISK_WRITE_REPLY		= 5,	// port, data unused
		// offset: byte offset of data written
		// size: number of bytes of data written; < 0 => error
};

// errors
enum {
	REMOTE_DISK_IO_ERROR	= -1,
	REMOTE_DISK_BAD_REQUEST	= -2,
};

struct remote_disk_header {
	uint64_t	offset;
	uint64_t	request_id;
	int16_t		size;
	uint16_t	port;
	uint8_t		command;
	uint8_t		data[0];
} __attribute__ ((__packed__));

#endif	// _BOOT_REMOTE_DISK_DEFS_H
