/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <platform/openfirmware/openfirmware.h>

#include <stdarg.h>


// OpenFirmware entry function
static int (*gCallOpenFirmware)(void *) = 0;
int gChosen;


status_t
of_init(int (*openFirmwareEntry)(void *))
{
	gCallOpenFirmware = openFirmwareEntry;

	gChosen = of_finddevice("/chosen");
	if (gChosen == OF_FAILED)
		return B_ERROR;

	return B_OK;
}


int
of_call_client_function(const char *method, int numArgs, int numReturns, ...)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
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


int
of_interpret(const char *command, int numArgs, int numReturns, ...)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
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


int
of_call_method(int handle, const char *method, int numArgs, int numReturns, ...)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
			// "IN:	[string] method, ihandle, stack_arg1, ..., stack_argP
			// OUT:	catch-result, stack_result1, ..., stack_resultQ
			// [...]
			// An implementation shall allow at least six stack_arg and six
			// stack_result items."
		const char	*method;
		int			handle;
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


int
of_finddevice(const char *device)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		const char	*device;
		int			handle;
	} args = {"finddevice", 1, 1, device, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.handle;
}


/** Returns the first child of the given node
 */

int
of_child(int node)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			node;
		int			child;
	} args = {"child", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.child;
}


/** Returns the next sibling of the given node
 */

int
of_peer(int node)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			node;
		int			next_sibling;
	} args = {"peer", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.next_sibling;
}


/** Returns the parent of the given node
 */

int
of_parent(int node)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			node;
		int			parent;
	} args = {"parent", 1, 1, node, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.parent;
}


int
of_instance_to_path(int instance, char *pathBuffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			instance;
		char		*path_buffer;
		int			buffer_size;
		int			size;
	} args = {"instance-to-path", 3, 1, instance, pathBuffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_instance_to_package(int instance)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			instance;
		int			package;
	} args = {"instance-to-package", 1, 1, instance, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.package;
}


int
of_getprop(int package, const char *property, void *buffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			package;
		const char	*property;
		void		*buffer;
		int			buffer_size;
		int			size;
	} args = {"getprop", 4, 1, package, property, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_setprop(int package, const char *property, const void *buffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			package;
		const char	*property;
		const void	*buffer;
		int			buffer_size;
		int			size;
	} args = {"setprop", 4, 1, package, property, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_getproplen(int package, const char *property)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			package;
		const char	*property;
		int			size;
	} args = {"getproplen", 2, 1, package, property, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_nextprop(int package, const char *previousProperty, char *nextProperty)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			package;
		const char	*previous_property;
		char		*next_property;
		int			flag;
	} args = {"nextprop", 3, 1, package, previousProperty, nextProperty, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.flag;
}


int
of_package_to_path(int package, char *pathBuffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			package;
		char		*path_buffer;
		int			buffer_size;
		int			size;
	} args = {"package-to-path", 3, 1, package, pathBuffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


//	I/O functions


int
of_open(const char *nodeName)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		const char	*node_name;
		int			handle;
	} args = {"open", 1, 1, nodeName, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED || args.handle == 0)
		return OF_FAILED;

	return args.handle;
}


void
of_close(int handle)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			handle;
	} args = {"close", 1, 0, handle};

	gCallOpenFirmware(&args);
}


int
of_read(int handle, void *buffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			handle;
		void		*buffer;
		int			buffer_size;
		int			size;
	} args = {"read", 3, 1, handle, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_write(int handle, const void *buffer, int bufferSize)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			handle;
		const void	*buffer;
		int			buffer_size;
		int			size;
	} args = {"write", 3, 1, handle, buffer, bufferSize, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.size;
}


int
of_seek(int handle, long long pos)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			handle;
		int64		pos;
		int			status;
	} args = {"seek", 3, 1, handle, pos, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.status;
}


// memory functions


int
of_release(void *virtualAddress, int size)
{
	struct {
		const char *name;
		int			num_args;
		int			num_returns;
		void		*virtualAddress;
		int			size;
	} args = {"release", 2, 0, virtualAddress, size};

	return gCallOpenFirmware(&args);
}


void *
of_claim(void *virtualAddress, int size, int align)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		void		*virtualAddress;
		int			size;
		int			align;
		void		*address;
	} args = {"claim", 3, 1, virtualAddress, size, align};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return NULL;

	return args.address;
}


// misc functions


/** tests if the given service is missing
 */

int
of_test(const char *service)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		const char	*service;
		int			missing;
	} args = {"test", 1, 1, service, 0};

	if (gCallOpenFirmware(&args) == OF_FAILED)
		return OF_FAILED;

	return args.missing;
}


/** Returns the millisecond counter
 */

int
of_milliseconds(void)
{
	struct {
		const char	*name;
		int			num_args;
		int			num_returns;
		int			milliseconds;
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
		int			num_args;
		int			num_returns;
	} args = {"exit", 0, 0};

	gCallOpenFirmware(&args);
}

