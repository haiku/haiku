/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */
#ifndef OPEN_FIRMWARE_H
#define OPEN_FIRMWARE_H


#include <SupportDefs.h>


#define OF_FAILED	(-1)


/* global device tree/properties access */
extern intptr_t gChosen;


template<typename AddressType, typename SizeType>
struct of_region {
	AddressType base;
	SizeType size;
} _PACKED;

struct of_arguments {
	const char	*name;
	intptr_t	num_args;
	intptr_t	num_returns;
	intptr_t	data[0];

#ifdef __cplusplus
	intptr_t &Argument(int index) { return data[index]; }
	intptr_t &ReturnValue(int index) { return data[num_args + index]; }
#endif
};


#ifdef __cplusplus
extern "C" {
#endif

extern status_t of_init(intptr_t (*openFirmwareEntry)(void *));

/* device tree functions */
extern intptr_t of_finddevice(const char *device);
extern intptr_t of_child(intptr_t node);
extern intptr_t of_peer(intptr_t node);
extern intptr_t of_parent(intptr_t node);
extern intptr_t of_instance_to_path(uint32_t instance, char *pathBuffer,
	intptr_t bufferSize);
extern intptr_t of_instance_to_package(uint32_t instance);
extern intptr_t of_getprop(intptr_t package, const char *property, void *buffer,
	intptr_t bufferSize);
extern intptr_t of_setprop(intptr_t package, const char *property, const void *buffer,
	intptr_t bufferSize);
extern intptr_t of_nextprop(intptr_t package, const char *previousProperty,
	char *nextProperty);
extern intptr_t of_getproplen(intptr_t package, const char *property);
extern intptr_t of_package_to_path(intptr_t package, char *pathBuffer,
	intptr_t bufferSize);

/* I/O functions */
extern intptr_t of_open(const char *nodeName);
extern void of_close(intptr_t handle);
extern intptr_t of_read(intptr_t handle, void *buffer, intptr_t bufferSize);
extern intptr_t of_write(intptr_t handle, const void *buffer, intptr_t bufferSize);
extern intptr_t of_seek(intptr_t handle, off_t pos);

/* memory functions */
extern intptr_t of_release(void *virtualAddress, intptr_t size);
extern void *of_claim(void *virtualAddress, intptr_t size, intptr_t align);

/* misc functions */
extern intptr_t of_call_client_function(const char *method, intptr_t numArgs,
	intptr_t numReturns, ...);
extern intptr_t of_interpret(const char *command, intptr_t numArgs,
	intptr_t numReturns, ...);
extern intptr_t of_call_method(uint32_t handle, const char *method,
	intptr_t numArgs, intptr_t numReturns, ...);
extern intptr_t of_test(const char *service);
extern intptr_t of_milliseconds(void);
extern void of_exit(void);

#ifdef __cplusplus
}
#endif

#endif	/* OPEN_FIRMWARE_H */
