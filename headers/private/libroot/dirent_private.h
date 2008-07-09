/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRENT_PRIVATE_H
#define DIRENT_PRIVATE_H


#include <dirent.h>


#define DIR_BUFFER_SIZE	4096
#define DIRENT_BUFFER_SIZE (DIR_BUFFER_SIZE - 2 * sizeof(int))

struct __DIR {
	int				fd;
	short			next_entry;
	unsigned short	entries_left;
	struct dirent	first_entry;
};

#endif	/* DIRENT_PRIVATE_H */
