/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DIRECT_WINDOW_PRIVATE_H
#define _DIRECT_WINDOW_PRIVATE_H


#include <OS.h>


#define DIRECT_BUFFER_INFO_AREA_SIZE B_PAGE_SIZE


struct direct_window_sync_data {
	area_id	area;
	sem_id	disable_sem;
	sem_id	disable_sem_ack;
};


#endif	// _DIRECT_WINDOW_PRIVATE_H
