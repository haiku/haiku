/* 	DPC module API
 *  Copyright 2007, Haiku Inc. All Rights Reserved.
 *  Distributed under the terms of the MIT License
 */
 
#ifndef _DPC_MODULE_H_
#define _DPC_MODULE_H_

#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

#define B_DPC_MODULE_NAME "generic/dpc/v1"

typedef void (*dpc_func) (void *arg);

typedef struct {
	module_info info;
	void * 		(*new_dpc_queue)(const char *name, long priority, int queue_size);
	status_t	(*delete_dpc_queue)(void *queue);
	status_t	(*queue_dpc)(void *queue, dpc_func dpc_name, void *arg);
} dpc_module_info;


#ifdef __cplusplus
}
#endif

#endif
