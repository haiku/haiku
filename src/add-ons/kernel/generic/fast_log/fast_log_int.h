/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Fast logging facilities.

	Internal header.
*/

#include <drivers/fast_log.h>
#include <malloc.h>
#include <pnp_devfs.h>
#include <device_manager.h>

// maximum number of long buffer
#define FAST_LOG_MAX_LONG_BUFFERS 256;
// size of one long buffer
#define FAST_LOG_LONG_BUFFER_SIZE (16*1024)
// name of dev interface module
#define FAST_LOG_DEVFS_MODULE_NAME "generic/fast_log/devfs/v1"


extern device_manager_info *pnp;


// long_buffers.c


void fast_log_write_long_log(const char *str);
void fast_log_flush_buffer(void);
size_t fast_log_read_long(char *buffer, size_t len);
status_t fast_log_init_long_buffer(void);
void fast_log_uninit_long_buffer(void);



// device.c


status_t fast_log_init_dev(void);
void fast_log_uninit_dev(void);

extern pnp_devfs_driver_info fast_log_devfs_module;
