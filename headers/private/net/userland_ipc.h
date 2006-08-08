/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef USERLAND_IPC_H
#define USERLAND_IPC_H

/*! userland_ipc - Communication between the network driver
		and the userland stack.
*/

#include <OS.h>
#include <Drivers.h>

#include "net_stack_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NET_STACK_PORTNAME "net_server connection"

enum {
	NET_STACK_OPEN = NET_STACK_IOCTL_MAX,
	NET_STACK_CLOSE,
	NET_STACK_NEW_CONNECTION,
};

#define MAX_NET_AREAS 16

typedef struct {
	area_id id;
	uint8	*offset;
} net_area_info;

typedef struct {
	int32	op;
//	int32	buffer;
	uint8	*data;
	int32	length;
	int32	result;
	net_area_info	area[MAX_NET_AREAS];
} net_command;

#define CONNECTION_QUEUE_LENGTH 128
#define CONNECTION_COMMAND_SIZE 2048

typedef struct {
	port_id	port;
	area_id area;
	thread_id socket_thread;

	sem_id	commandSemaphore;	// command queue
	uint32	numCommands,bufferSize;
} net_connection;


extern status_t init_userland_ipc(void);
extern void shutdown_userland_ipc(void);


#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif	/* USERLAND_IPC_H */
