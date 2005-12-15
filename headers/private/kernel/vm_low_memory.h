/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_VM_LOW_MEMORY_H
#define _KERNEL_VM_LOW_MEMORY_H


#include <SupportDefs.h>


/* warning levels for low memory handlers */
enum {
	B_NO_LOW_MEMORY = 0,
	B_LOW_MEMORY_NOTE,
	B_LOW_MEMORY_WARNING,
	B_LOW_MEMORY_CRITICAL,
};

typedef void (*low_memory_func)(void *data, int32 level);

#ifdef __cplusplus
extern "C" {
#endif

status_t vm_low_memory_init(void);
int32 vm_low_memory_state(void);
void vm_low_memory(size_t requirements);

// these calls might get public some day
status_t register_low_memory_handler(low_memory_func function, void *data,
			int32 priority);
status_t unregister_low_memory_handler(low_memory_func function, void *data);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_LOW_MEMORY_H */
