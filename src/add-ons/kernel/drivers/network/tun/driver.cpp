/*
 * TUN/TAP Network Tunnel Driver for Haiku
 * Copyright 2003 mmu_man, revol@free.fr
 * Copyright 2023 Sean Brady, swangeon@gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <Select.h>

#include "BufferQueue.h"
#include <net_buffer.h>

#include <condition_variable.h>
#include <fs/select_sync_pool.h>
#include <net/if_tun.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/AutoLock.h>


// #define TUN_DRIVER_NAME "tun/0"
#define TAP_DRIVER_NAME "tap/0"
#define NET_TUN_MODULE_NAME "network/devices/tun/v1"
#define BUFFER_QUEUE_MAX 30000

const char* device_names[] = {TAP_DRIVER_NAME, NULL};

typedef struct tun_struct {
	uint32_t           name[3];
	unsigned long      flags;
	BufferQueue*       sendQueue;
	BufferQueue*       recvQueue;
	ConditionVariable* readWait;
	mutex              readLock;
	union {
		struct {
			mutex      selectLock;
		} app;
	};
} tun_struct;

int32 api_version = B_CUR_DRIVER_API_VERSION;
static BufferQueue gAppQueue(BUFFER_QUEUE_MAX);
static BufferQueue gIntQueue(BUFFER_QUEUE_MAX);
static ConditionVariable gIntWait;
static select_sync_pool* gSelectPool = NULL;
struct net_buffer_module_info* gBufferModule;


/**
 * @brief Creates a filled net_buffer based on the given data and size.
 *
 * @param data the pointer to the data that will be copied to the net_buffer
 * @param bytes the size of the data in bytes
 *
 * @return a net_buffer pointer containing the copied data, or NULL if creation fails
 */
static net_buffer*
create_filled_buffer(uint8* data, size_t bytes)
{
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return NULL;

	status_t status = gBufferModule->append(buffer, data, bytes);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return NULL;
	}

	return buffer;
}


/**
 * @brief Retrieves a packet from the given buffer queue.
 *
 * @param queueToUse The buffer queue to retrieve the packet from.
 * @param data A pointer to the memory location where the packet data will be copied.
 * @param numbytes A pointer to the variable that will store the size of the retrieved packet.
 *
 * @return The status of the retrieval operation or B_OK if successful.
 */
static status_t
retrieve_packet(BufferQueue* queueToUse, void* data, size_t* numbytes)
{
	net_buffer* buffer;
	status_t status = B_OK;
	if (queueToUse->Used() > 0) {
		status_t status = queueToUse->Get(*numbytes, true, &buffer);
		if (status != B_OK)
			return status;
		*numbytes = buffer->size;
		status = gBufferModule->read(buffer, 0, data, *numbytes);
		gBufferModule->free(buffer);
	} else
		*numbytes = 0;
	return status;
}


/**
 * A helper function to notify events based on the readability and writability
 * of a file descriptor.
 *
 * @param readable A boolean indicating if the file descriptor is readable.
 * @param writable A boolean indicating if the file descriptor is writable.
 *
 * @return void
 *
 * @throws None
 */
static void
notify_select_helper(bool readable, bool writable)
{
	if (readable) {
		notify_select_event_pool(gSelectPool, B_SELECT_READ);
	}

	if (writable) {
		notify_select_event_pool(gSelectPool, B_SELECT_WRITE);
	}
}


/**
 * @brief Notifies the select event pool if the tun device is readable or writable.
 *
 * @param tun The tun_struct object representing the tun device.
 */
static void
tun_notify(tun_struct* tun)
{
	/* This function is just for the APP side but both sides need to call it */
	bool select_pool_check = gSelectPool != NULL;
	if (select_pool_check) {
		bool readable = tun->recvQueue->Used() > 0;
		bool writable = tun->sendQueue->Used() < BUFFER_QUEUE_MAX;
		if ((tun->flags & O_NONBLOCK) != 0) // APP Side
			notify_select_helper(readable, writable);
		else /* IFACE side switches since its queues are flipped from APP*/
			notify_select_helper(writable, readable);
	}
}


/**
 * @brief First initialization step for the driver.
 *
 * @return the status of the initialization process.
 */
status_t
init_hardware(void)
{
	/* No Hardware */
	dprintf("tun:init_hardware()\n");
	return B_NO_ERROR;
}


/**
 * @brief Initializes the driver and the net buffer module for packet operations.
 *
 * @return the status of the initialization.
 */
status_t
init_driver(void)
{
	/* Init driver */
	dprintf("tun:init_driver()\n");
	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
	if (status != B_OK)
		return status;
	return B_OK;
}


/**
 * @brief Uninitializes the driver.
 *
 * @return void
 */
void
uninit_driver(void)
{
	dprintf("tun:uninit_driver()\n");
	put_module(NET_BUFFER_MODULE_NAME);
}


/**
 * @brief Sets the tun_struct for this session of open.
 *
 * @param name the name of the interface
 * @param flags the flags for the interface
 * @param cookie a pointer to store the cookie
 *
 * @return the status of the function
 */
status_t
tun_open(const char* name, uint32 flags, void** cookie)
{
	/* Setup driver for interface here */
	tun_struct* tun = new tun_struct();
	memcpy(tun->name, "app", sizeof(tun->name));
	tun->sendQueue = &gIntQueue;
	tun->recvQueue = &gAppQueue;
	tun->flags = 0;
	tun->readWait = &gIntWait;
	mutex_init(&tun->readLock, "read_avail");
	mutex_init(&tun->app.selectLock, "select_lock");
	*cookie = static_cast<void*>(tun);
	return B_OK;
}


/**
 * @brief Close the open instance.
 *
 * @param cookie a pointer to the tun_struct object to be used
 *
 * @return The status of the function
 */
status_t
tun_close(void* cookie)
{
	/* Close interface here */
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	if ((tun->flags & O_NONBLOCK) == 0) {
		tun->readWait->NotifyAll(B_ERROR);
		snooze(10); // Due to a lock timing issue, we sleep for 10ms
	}
	return B_OK;
}


/**
 * @brief Frees the memory allocated for a tun_struct object.
 *
 * @param cookie a pointer to the tun_struct object to be freed
 *
 * @return a status_t indicating the result of the operation
 */
status_t
tun_free(void* cookie)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	mutex_destroy(&tun->readLock);
	if ((tun->flags & O_NONBLOCK) != 0)
		mutex_destroy(&tun->app.selectLock);
	delete tun;
	return B_OK;
}


/**
 * @brief Updates the tun interface based on the given IOCTL operation and data.
 *
 * @param cookie a pointer to the tun_struct object
 * @param op the IOCTL operation to perform
 * @param data a pointer to the data to be used in the IOCTL operation
 * @param len the size of the data in bytes
 *
 * @return the status of the IOCTL operation
 */
status_t
tun_ioctl(void* cookie, uint32 op, void* data, size_t len)
{
	/* IOCTL for driver */
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	switch (op) {
		case TUNSETIFF: // Reconfigures tun_struct to interface settings
			memcpy(tun->name, "int", sizeof(tun->name));
			tun->sendQueue = &gAppQueue;
			tun->recvQueue = &gIntQueue;
			mutex_destroy(&tun->app.selectLock);
			memset(&tun->app, 0, sizeof(tun->app));
			return B_OK;
		case B_SET_NONBLOCKING_IO:
			tun->flags |= O_NONBLOCK;
			return B_OK;
		default:
			return B_DEV_INVALID_IOCTL;
	};

	return B_OK;
}


/**
 * @brief Reads data from the TUN/TAP device.
 *
 * @param cookie a pointer to the tun_struct
 * @param position the position in the data stream to read from
 * @param data a pointer to the buffer where the read data will be copied
 * @param numbytes a pointer to the size to read, updated with the number of bytes read
 *
 * @return the status of the read operation
 */
status_t
tun_read(void* cookie, off_t position, void* data, size_t* numbytes)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	status_t status;
	MutexLocker _(tun->readLock); // released on exit
	/* IFACE side is blocking I/O */
	if ((tun->flags & O_NONBLOCK) == 0) {
		while (tun->recvQueue->Used() == 0) {
			status = tun->readWait->Wait(&tun->readLock, B_CAN_INTERRUPT);
			if (status != B_OK)
				return status;
		}
	}
	status = retrieve_packet(tun->recvQueue, data, numbytes);
	tun_notify(tun);
	return status;
}


/**
 * @brief Writes data to the specified sending queue.
 *
 * @param cookie a pointer to the tun_struct
 * @param position the position in the file to write to (NOT USED)
 * @param data a pointer to the data to write
 * @param numbytes a pointer to the number of bytes to write
 *
 * @return a status code indicating the result of the write operation
 */
status_t
tun_write(void* cookie, off_t position, const void* data, size_t* numbytes)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	size_t used = tun->sendQueue->Used();
	// Buffer is full or will be so we have to drop the packet
	if ((used + *numbytes) >= BUFFER_QUEUE_MAX)
		return B_WOULD_BLOCK;
	net_buffer* packet = create_filled_buffer((uint8*)data, *numbytes);
	if (packet == NULL)
		return B_ERROR;
	tun->sendQueue->Add(packet);
	if ((tun->flags & O_NONBLOCK) != 0)
		tun->readWait->NotifyOne();
	tun_notify(tun);
	return B_OK;
}


/**
 * @brief Handles the selection of read and write events on a TUN/TAP device.
 *
 * @param cookie a pointer to the tun_struct object
 * @param event the event type (B_SELECT_READ or B_SELECT_WRITE are accepted)
 * @param ref the reference number
 * @param sync a pointer to the selectsync object
 *
 * @return the status of the select operation
 */
status_t
tun_select(void* cookie, uint8 event, uint32 ref, selectsync* sync)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	MutexLocker _(tun->app.selectLock);
	bool isReadable = tun->recvQueue->Used() > 0;
	bool isWritable = tun->sendQueue->Used() < BUFFER_QUEUE_MAX;

	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;

	status_t status = add_select_sync_pool_entry(&gSelectPool, sync, event);
	if (status != B_OK)
		return B_BAD_VALUE;

	if (event == B_SELECT_READ && isReadable)
		notify_select_event(sync, event);

	if (event == B_SELECT_WRITE && isWritable)
		notify_select_event(sync, event);

	return status;
}


/**
 * @brief Deselects a event in the select pool.
 *
 * @param cookie a pointer to the tun_struct object representing the tun device
 * @param event the event type to be deselected (B_SELECT_READ or B_SELECT_WRITE)
 * @param sync a pointer to the selectsync object to be removed from the select pool
 *
 * @return the status code indicating the result of the deselect operation
 */
status_t
tun_deselect(void* cookie, uint8 event, selectsync* sync)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	MutexLocker _(tun->app.selectLock);
	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;
	return remove_select_sync_pool_entry(&gSelectPool, sync, event);
}


/**
 * @brief Publishes the driver names to devfs.
 *
 * @return A pointer to a constant character pointer representing the device names.
 */
const char**
publish_devices()
{
	return device_names;
}


device_hooks tun_hooks = {
	tun_open,
	tun_close,
	tun_free,
	tun_ioctl,
	tun_read,
	tun_write,
	tun_select,
	tun_deselect,
	NULL,
	NULL
	};
/**
 * @brief Finds hooks for driver functions.
 *
 * @param name the name of the device.
 *
 * @return a pointer to the device hooks (all devices return the same hooks)
 */
device_hooks*
find_device(const char* name)
{
	return &tun_hooks;
}
