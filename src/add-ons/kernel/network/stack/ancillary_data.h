/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_ANCILLARY_DATA_H
#define NET_ANCILLARY_DATA_H

#include <net_stack.h>


struct ancillary_data_container;

ancillary_data_container* create_ancillary_data_container();
void delete_ancillary_data_container(ancillary_data_container* container);

status_t add_ancillary_data(ancillary_data_container* container,
			const ancillary_data_header* header, const void* data,
			void (*destructor)(const ancillary_data_header*, void*),
			void** _allocatedData);
status_t remove_ancillary_data(ancillary_data_container* container, void* data,
			bool destroy);
void* move_ancillary_data(ancillary_data_container* from,
			ancillary_data_container* to);

void* next_ancillary_data(ancillary_data_container* container,
			void* previousData, ancillary_data_header* _header);


#endif	// NET_ANCILLARY_DATA_H
