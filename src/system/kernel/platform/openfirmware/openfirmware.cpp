/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */

#include <platform/openfirmware/openfirmware.h>

#include <stdarg.h>


// OpenFirmware entry function
static intptr_t (*gCallOpenFirmware)(void *) = 0;
intptr_t gChosen;


status_t
of_init(intptr_t (*openFirmwareEntry)(void *))
{
	gCallOpenFirmware = openFirmwareEntry;

	gChosen = of_finddevice("/chosen");
	if (gChosen == OF_FAILED)
		return B_ERROR;

	return B_OK;
}


intptr_t
of_call_client_function(const char *method, intptr_t numArgs,
	intptr_t numReturns, ...)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		void		*args[10];
	} args = {method, numArgs, numReturns};
	va_list list;
	int i;

	// iterate over all arguments and copy them into the
	// structure passed over to the OpenFirmware

	va_start(list, numReturns);
	for (i = 0; i < numArgs; i++) {
		// copy args
		args.args[i] = (void *)va_arg(list, void *);
	}
	for (i = numArgs; i < numArgs + numReturns; i++) {
		// clear return values
		args.args[i] = NULL;
	}

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	if (numReturns > 0) {
		// copy return values over to the provided location

		for (i = numArgs; i < numArgs + numReturns; i++) {
			void **store = va_arg(list, void **);
			if (store)
				*store = args.args[i];
		}
	}
	va_end(list);

	return 0;
}


intptr_t
of_interpret(const char *command, intptr_t numArgs, intptr_t numReturns, ...)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
			// "IN:	[string] cmd, stack_arg1, ..., stack_argP
			// OUT:	catch-result, stack_result1, ..., stack_resultQ
			// [...]
			// An implementation shall allow at least six stack_arg and six
			// stack_result items."
		const char	*command;
		void		*args[13];
	} args = {"interpret", numArgs + 1, numReturns + 1, command};
	va_list list;
	int i;

	// iterate over all arguments and copy them into the
	// structure passed over to the OpenFirmware

	va_start(list, numReturns);
	for (i = 0; i < numArgs; i++) {
		// copy args
		args.args[i] = (void *)va_arg(list, void *);
	}
	for (i = numArgs; i < numArgs + numReturns + 1; i++) {
		// clear return values
		args.args[i] = NULL;
	}

	// args.args[numArgs] is the "catch-result" return value
	if (gCallOpenFirmware(&args) == OF_FAILED || args.args[numArgs])
		return OF_FAILED;

	if (numReturns > 0) {
		// copy return values over to the provided location

		for (i = numArgs + 1; i < numArgs + 1 + numReturns; i++) {
			void **store = va_arg(list, void **);
			if (store)
				*store = args.args[i];
		}
	}
	va_end(list);

	return 0;
}


intptr_t
of_call_method(uint32_t handle, const char *method, intptr_t numArgs,
	intptr_t numReturns, ...)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
			// "IN:	[string] method, ihandle, stack_arg1, ..., stack_argP
			// OUT:	catch-result, stack_result1, ..., stack_resultQ
			// [...]
			// An implementation shall allow at least six stack_arg and six
			// stack_result items."
		const char	*method;
		intptr_t	handle;
		void		*args[13];
	} args = {"call-method", numArgs + 2, numReturns + 1, method, handle};
	va_list list;
	int i;

	// iterate over all arguments and copy them into the
	// structure passed over to the OpenFirmware

	va_start(list, numReturns);
	for (i = 0; i < numArgs; i++) {
		// copy args
		args.args[i] = (void *)va_arg(list, void *);
	}
	for (i = numArgs; i < numArgs + numReturns + 1; i++) {
		// clear return values
		args.args[i] = NULL;
	}

	// args.args[numArgs] is the "catch-result" return value
	if (gCallOpenFirmware(&args) == OF_FAILED || args.args[numArgs])
		return OF_FAILED;

	if (numReturns > 0) {
		// copy return values over to the provided location

		for (i = numArgs + 1; i < numArgs + 1 + numReturns; i++) {
			void **store = va_arg(list, void **);
			if (store)
				*store = args.args[i];
		}
	}
	va_end(list);

	return 0;
}


intptr_t
of_finddevice(const char *device)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		const char	*device;
		intptr_t	handle;
	} args = {"finddevice", 1, 1, device, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.handle;
}


/** Returns the first child of the given node
 */

intptr_t
of_child(intptr_t node)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	node;
		intptr_t	child;
	} args = {"child", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.child;
}


/** Returns the next sibling of the given node
 */

intptr_t
of_peer(intptr_t node)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	node;
		intptr_t	next_sibling;
	} args = {"peer", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.next_sibling;
}


/** Returns the parent of the given node
 */

intptr_t
of_parent(intptr_t node)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	node;
		intptr_t	parent;
	} args = {"parent", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.parent;
}


intptr_t
of_instance_to_path(uint32_t instance, char *pathBuffer, intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	instance;
		char		*path_buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"instance-to-path", 3, 1, instance, pathBuffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_instance_to_package(uint32_t instance)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	instance;
		intptr_t	package;
	} args = {"instance-to-package", 1, 1, instance, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.package;
}


intptr_t
of_getprop(intptr_t package, const char *property, void *buffer, intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	package;
		const char	*property;
		void		*buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"getprop", 4, 1, package, property, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_setprop(intptr_t package, const char *property, const void *buffer,
	intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	package;
		const char	*property;
		const void	*buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"setprop", 4, 1, package, property, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_getproplen(intptr_t package, const char *property)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	package;
		const char	*property;
		intptr_t	size;
	} args = {"getproplen", 2, 1, package, property, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_nextprop(intptr_t package, const char *previousProperty, char *nextProperty)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	package;
		const char	*previous_property;
		char		*next_property;
		intptr_t	flag;
	} args = {"nextprop", 3, 1, package, previousProperty, nextProperty, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.flag;
}


intptr_t
of_package_to_path(intptr_t package, char *pathBuffer, intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	package;
		char		*path_buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"package-to-path", 3, 1, package, pathBuffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


//	I/O functions


intptr_t
of_open(const char *nodeName)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		const char	*node_name;
		intptr_t	handle;
	} args = {"open", 1, 1, nodeName, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED || args.handle == 0)
		return OF_FAILED;

	return args.handle;
}


void
of_close(intptr_t handle)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	handle;
	} args = {"close", 1, 0, handle};

	gCallOpenFirmware(&args);
}


intptr_t
of_read(intptr_t handle, void *buffer, intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	handle;
		void		*buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"read", 3, 1, handle, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_write(intptr_t handle, const void *buffer, intptr_t bufferSize)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	handle;
		const void	*buffer;
		intptr_t	buffer_size;
		intptr_t	size;
	} args = {"write", 3, 1, handle, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


intptr_t
of_seek(intptr_t handle, off_t pos)
{
	intptr_t pos_hi = 0;
	if (sizeof(off_t) > sizeof(intptr_t))
		pos_hi = pos >> ((sizeof(off_t) - sizeof(intptr_t)) * CHAR_BIT);

	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	handle;
		intptr_t	pos_hi;
		intptr_t	pos;
		intptr_t	status;
	} args = {"seek", 3, 1, handle, pos_hi, pos, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.status;
}


// memory functions


intptr_t
of_release(void *virtualAddress, intptr_t size)
{
	struct {
		const char *name;
		intptr_t	num_args;
		intptr_t	num_returns;
		void		*virtualAddress;
		intptr_t	size;
	} args = {"release", 2, 0, virtualAddress, size};

	return gCallOpenFirmware(&args);
}


void *
of_claim(void *virtualAddress, intptr_t size, intptr_t align)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		void		*virtualAddress;
		intptr_t	size;
		intptr_t	align;
		void		*address;
	} args = {"claim", 3, 1, virtualAddress, size, align};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return NULL;

	return args.address;
}


// misc functions


/** tests if the given service is missing
 */

intptr_t
of_test(const char *service)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		const char	*service;
		intptr_t	missing;
	} args = {"test", 1, 1, service, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.missing;
}


/** Returns the millisecond counter
 */

intptr_t
of_milliseconds(void)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
		intptr_t	milliseconds;
	} args = {"milliseconds", 0, 1, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.milliseconds;
}


void
of_exit(void)
{
	struct {
		const char	*name;
		intptr_t	num_args;
		intptr_t	num_returns;
	} args = {"exit", 0, 0};

	gCallOpenFirmware(&args);
}

