/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include "stage2_priv.h"

static int (*of)(void *) = 0; // openfirmware entry

int of_init(void *of_entry)
{
	of = of_entry;
	return 0;
}

int of_open(const char *node_name)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		const char *node_name;
		int handle;
	} args;

	args.name = "open";
	args.num_args = 1;
	args.num_returns = 1;
	args.node_name = node_name;

	of(&args);

	return args.handle;
}

int of_finddevice(const char *dev)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		const char *device;
		int handle;
	} args;

	args.name = "finddevice";
	args.num_args = 1;
	args.num_returns = 1;
	args.device = dev;

	of(&args);

	return args.handle;
}

int of_instance_to_package(int in_handle)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int in_handle;
		int handle;
	} args;

	args.name = "instance-to-package";
	args.num_args = 1;
	args.num_returns = 1;
	args.in_handle = in_handle;

	of(&args);

	return args.handle;
}

int of_getprop(int handle, const char *prop, void *buf, int buf_len)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int handle;
		const char *prop;
		void *buf;
		int buf_len;
		int size;
	} args;

	args.name = "getprop";
	args.num_args = 4;
	args.num_returns = 1;
	args.handle = handle;
	args.prop = prop;
	args.buf = buf;
	args.buf_len = buf_len;

	of(&args);

	return args.size;
}

int of_setprop(int handle, const char *prop, const void *buf, int buf_len)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int handle;
		const char *prop;
		const void *buf;
		int buf_len;
		int size;
	} args;

	args.name = "setprop";
	args.num_args = 4;
	args.num_returns = 1;
	args.handle = handle;
	args.prop = prop;
	args.buf = buf;
	args.buf_len = buf_len;

	of(&args);

	return args.size;
}

int of_read(int handle, void *buf, int buf_len)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int handle;
		void *buf;
		int buf_len;
		int size;
	} args;

	args.name = "read";
	args.num_args = 3;
	args.num_returns = 1;
	args.handle = handle;
	args.buf = buf;
	args.buf_len = buf_len;

	of(&args);

	return args.size;
}

int of_write(int handle, void *buf, int buf_len)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int handle;
		void *buf;
		int buf_len;
		int size;
	} args;

	args.name = "write";
	args.num_args = 3;
	args.num_returns = 1;
	args.handle = handle;
	args.buf = buf;
	args.buf_len = buf_len;

	of(&args);

	return args.size;
}

int of_seek(int handle, long long pos)
{
	struct {
		const char *name;
		int num_args;
		int num_returns;
		int handle;
		long long pos;
		int status;
	} args;

	args.name= "seek";
	args.num_args = 3;
	args.num_returns = 1;
	args.handle = handle;
	args.pos = pos;

	of(&args);

	return args.status;
}

