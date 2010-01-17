/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _H2UTIL_H_
#define _H2UTIL_H_

#include <util/list.h>

#include "h2generic.h"

/* net buffer utils for ACL, to be reviewed */
#define DEVICEFIELD type
#define SET_DEVICE(nbuf, hid) \
	(nbuf->DEVICEFIELD=(nbuf->DEVICEFIELD & 0xFFFFFF00) | (hid & 0xFF))
#define GET_DEVICE(nbuf) fetch_device(NULL,(nbuf->DEVICEFIELD&0xFF))

#define COOKIEFIELD flags 
void* nb_get_whole_buffer(net_buffer* nbuf); 
void nb_destroy(net_buffer* nbuf);
size_t get_expected_size(net_buffer* nbuf);

/* Room utils */
void init_room(struct list* l);
void* alloc_room(struct list* l, size_t size);
void reuse_room(struct list* l, void* room);
void purge_room(struct list* l);

/* list utils */
#define list_purge(x) purge_room(x)

#endif
