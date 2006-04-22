/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BITMAP_PRIVATE_H
#define _BITMAP_PRIVATE_H


#include <OS.h>


// This structure is placed in the client/server shared memory area.

struct overlay_client_data {
	sem_id	lock;
	uint8*	buffer;
};

#endif // _BITMAP_PRIVATE_H
