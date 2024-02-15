/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Sean Brady, swangeon@gmail.com
 */

#include <new>
#include <string.h>

#include <fs/select_sync_pool.h>
#include <fs/devfs.h>
#include <util/AutoLock.h>
#include <util/Random.h>

#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_tun.h>
#include <netinet/in.h>
#include <ethernet.h>


//#define TRACE_TUNNEL
#ifdef TRACE_TUNNEL
#	define TRACE(x...) dprintf("network/tunnel: " x)
#else
#   define TRACE(x...)
#endif

#define CALLED(x...)			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)
#define TRACE_ALWAYS(x...)		dprintf("network/tunnel: " x)


struct tunnel_device : net_device {
	bool				is_tap;

	net_fifo			send_queue, receive_queue;

	int32				open_count;

	mutex				select_lock;
	select_sync_pool*	select_pool;
};

#define TUNNEL_QUEUE_MAX (ETHER_MAX_FRAME_SIZE * 32)


struct net_buffer_module_info* gBufferModule;
static net_stack_module_info* gStackModule;


//	#pragma mark - devices array


static tunnel_device* gDevices[10] = {};
static mutex gDevicesLock = MUTEX_INITIALIZER("tunnel devices");


static tunnel_device*
find_tunnel_device(const char* name)
{
	ASSERT_LOCKED_MUTEX(&gDevicesLock);
	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] == NULL)
			continue;

		if (strcmp(gDevices[i]->name, name) == 0)
			return gDevices[i];
	}
	return NULL;
}


//	#pragma mark - devfs device


struct tunnel_cookie {
	tunnel_device*	device;
	uint32		flags;
};


status_t
tunnel_open(const char* name, uint32 flags, void** _cookie)
{
	MutexLocker devicesLocker(gDevicesLock);
	tunnel_device* device = find_tunnel_device(name);
	if (device == NULL)
		return ENODEV;
	if (atomic_or(&device->open_count, 1) != 0)
		return EBUSY;

	tunnel_cookie* cookie = new(std::nothrow) tunnel_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->device = device;
	cookie->flags = flags;

	*_cookie = cookie;
	return B_OK;
}


status_t
tunnel_close(void* _cookie)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	// Wake up the send queue, so that any threads waiting to read return at once.
	release_sem_etc(cookie->device->send_queue.notify, B_INTERRUPTED, B_RELEASE_ALL);

	return B_OK;
}


status_t
tunnel_free(void* _cookie)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;
	atomic_and(&cookie->device->open_count, 0);
	delete cookie;
	return B_OK;
}


status_t
tunnel_control(void* _cookie, uint32 op, void* data, size_t len)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	switch (op) {
		case B_SET_NONBLOCKING_IO:
			cookie->flags |= O_NONBLOCK;
			return B_OK;
		case B_SET_BLOCKING_IO:
			cookie->flags &= ~O_NONBLOCK;
			return B_OK;
	}

	return B_DEV_INVALID_IOCTL;
}


status_t
tunnel_read(void* _cookie, off_t position, void* data, size_t* _length)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	net_buffer* buffer = NULL;
	status_t status = gStackModule->fifo_dequeue_buffer(
		&cookie->device->send_queue, 0, B_INFINITE_TIMEOUT, &buffer);
	if (status != B_OK)
		return status;

	const size_t length = min_c(*_length, buffer->size);
	status = gBufferModule->read(buffer, 0, data, length);
	if (status != B_OK)
		return status;
	*_length = length;

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
tunnel_write(void* _cookie, off_t position, const void* data, size_t* _length)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = gBufferModule->append(buffer, data, *_length);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	if (!cookie->device->is_tap) {
		// TUN: Detect packet type.
		uint8 version;
		status = gBufferModule->read(buffer, 0, &version, 1);
		if (status != B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		version = (version & 0xF0) >> 4;
		if (version != 4 && version != 6) {
			// Not any IP packet we recognize.
			gBufferModule->free(buffer);
			return B_BAD_DATA;
		}
		buffer->type = (version == 6) ? B_NET_FRAME_TYPE_IPV6
			: B_NET_FRAME_TYPE_IPV4;

		struct sockaddr_in& src = *(struct sockaddr_in*)buffer->source;
		struct sockaddr_in& dst = *(struct sockaddr_in*)buffer->destination;
		src.sin_len		= dst.sin_len		= sizeof(sockaddr_in);
		src.sin_family	= dst.sin_family	= (version == 6) ? AF_INET6 : AF_INET;
		src.sin_port	= dst.sin_port		= 0;
		src.sin_addr.s_addr = dst.sin_addr.s_addr = 0;
	}

	// We use a queue and the receive_data() hook instead of device_enqueue_buffer()
	// for two reasons: 1. listeners (e.g. packet capture) are only processed by the
	// reader thread that calls receive_data(), and 2. device_enqueue_buffer() has
	// to look up the device interface every time, which is inefficient.
	status = gStackModule->fifo_enqueue_buffer(&cookie->device->receive_queue, buffer);
	if (status != B_OK)
		gBufferModule->free(buffer);

	return status;
}


status_t
tunnel_select(void* _cookie, uint8 event, uint32 ref, selectsync* sync)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;

	MutexLocker selectLocker(cookie->device->select_lock);
	status_t status = add_select_sync_pool_entry(&cookie->device->select_pool, sync, event);
	if (status != B_OK)
		return B_BAD_VALUE;
	selectLocker.Unlock();

	MutexLocker fifoLocker(cookie->device->send_queue.lock);
	if (event == B_SELECT_READ && cookie->device->send_queue.current_bytes != 0)
		notify_select_event(sync, event);
	if (event == B_SELECT_WRITE)
		notify_select_event(sync, event);

	return B_OK;
}


status_t
tunnel_deselect(void* _cookie, uint8 event, selectsync* sync)
{
	tunnel_cookie* cookie = (tunnel_cookie*)_cookie;

	MutexLocker selectLocker(cookie->device->select_lock);
	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;
	return remove_select_sync_pool_entry(&cookie->device->select_pool, sync, event);
}


static device_hooks sDeviceHooks = {
	tunnel_open,
	tunnel_close,
	tunnel_free,
	tunnel_control,
	tunnel_read,
	tunnel_write,
	tunnel_select,
	tunnel_deselect,
};


//	#pragma mark - network stack device


status_t
tunnel_init(const char* name, net_device** _device)
{
	const bool isTAP = strncmp(name, "tap/", 4) == 0;
	if (!isTAP && strncmp(name, "tun/", 4) != 0)
		return B_BAD_VALUE;
	if (strlen(name) >= sizeof(tunnel_device::name))
		return ENAMETOOLONG;

	// Make sure this device doesn't already exist.
	MutexLocker devicesLocker(gDevicesLock);
	if (find_tunnel_device(name) != NULL)
		return EEXIST;

	tunnel_device* device = new(std::nothrow) tunnel_device;
	if (device == NULL)
		return B_NO_MEMORY;

	ssize_t index = -1;
	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] != NULL)
			continue;

		gDevices[i] = device;
		index = i;
		break;
	}
	if (index < 0) {
		delete device;
		return ENOSPC;
	}
	devicesLocker.Unlock();

	memset(device, 0, sizeof(tunnel_device));
	strcpy(device->name, name);

	device->mtu = ETHER_MAX_FRAME_SIZE;
	device->media = IFM_ACTIVE;

	device->is_tap = isTAP;
	if (device->is_tap) {
		device->flags = IFF_BROADCAST | IFF_ALLMULTI | IFF_LINK;
		device->type = IFT_ETHER;

		// Generate a random MAC address.
		for (int i = 0; i < ETHER_ADDRESS_LENGTH; i++)
			device->address.data[i] = secure_get_random<uint8>();
		device->address.data[0] &= 0xFE; // multicast
		device->address.data[0] |= 0x02; // local assignment

		device->address.length = ETHER_ADDRESS_LENGTH;
	} else {
		device->flags = IFF_POINTOPOINT | IFF_LINK;
		device->type = IFT_TUNNEL;
	}

	status_t status = gStackModule->init_fifo(&device->send_queue,
		"tunnel send queue", TUNNEL_QUEUE_MAX);
	if (status != B_OK) {
		delete device;
		return status;
	}

	status = gStackModule->init_fifo(&device->receive_queue,
		"tunnel receive queue", TUNNEL_QUEUE_MAX);
	if (status != B_OK) {
		gStackModule->uninit_fifo(&device->send_queue);
		delete device;
		return status;
	}

	mutex_init(&device->select_lock, "tunnel select lock");

	status = devfs_publish_device(name, &sDeviceHooks);
	if (status != B_OK) {
		gStackModule->uninit_fifo(&device->send_queue);
		gStackModule->uninit_fifo(&device->receive_queue);
		delete device;
		return status;
	}

	*_device = device;
	return B_OK;
}


status_t
tunnel_uninit(net_device* _device)
{
	tunnel_device* device = (tunnel_device*)_device;

	MutexLocker devicesLocker(gDevicesLock);
	if (atomic_get(&device->open_count) != 0)
		return EBUSY;

	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] != device)
			continue;

		gDevices[i] = NULL;
		break;
	}
	status_t status = devfs_unpublish_device(device->name, false);
	if (status != B_OK)
		panic("devfs_unpublish_device failed: %" B_PRId32, status);

	gStackModule->uninit_fifo(&device->send_queue);
	gStackModule->uninit_fifo(&device->receive_queue);
	mutex_destroy(&device->select_lock);
	delete device;
	return B_OK;
}


status_t
tunnel_up(net_device* _device)
{
	return B_OK;
}


void
tunnel_down(net_device* _device)
{
	tunnel_device* device = (tunnel_device*)_device;

	// Wake up the receive queue, so that the reader thread returns at once.
	release_sem_etc(device->receive_queue.notify, B_INTERRUPTED, B_RELEASE_ALL);
}


status_t
tunnel_control(net_device* device, int32 op, void* argument, size_t length)
{
	return B_BAD_VALUE;
}


status_t
tunnel_send_data(net_device* _device, net_buffer* buffer)
{
	tunnel_device* device = (tunnel_device*)_device;

	status_t status = B_OK;
	if (!device->is_tap) {
		// Ensure this is an IP frame.
		struct sockaddr_in& dst = *(struct sockaddr_in*)buffer->destination;
		if (dst.sin_family != AF_INET && dst.sin_family != AF_INET6)
			return B_BAD_DATA;
	}

	status = gStackModule->fifo_enqueue_buffer(
		&device->send_queue, buffer);
	if (status == B_OK) {
		MutexLocker selectLocker(device->select_lock);
		notify_select_event_pool(device->select_pool, B_SELECT_READ);
	}

	return status;
}


status_t
tunnel_receive_data(net_device* _device, net_buffer** _buffer)
{
	tunnel_device* device = (tunnel_device*)_device;
	return gStackModule->fifo_dequeue_buffer(&device->receive_queue,
		0, B_INFINITE_TIMEOUT, _buffer);
}


status_t
tunnel_set_mtu(net_device* device, size_t mtu)
{
	if (mtu > 65536 || mtu < 16)
		return B_BAD_VALUE;

	device->mtu = mtu;
	return B_OK;
}


status_t
tunnel_set_promiscuous(net_device* device, bool promiscuous)
{
	return EOPNOTSUPP;
}


status_t
tunnel_set_media(net_device* device, uint32 media)
{
	return EOPNOTSUPP;
}


status_t
tunnel_add_multicast(net_device* device, const sockaddr* address)
{
	return B_OK;
}


status_t
tunnel_remove_multicast(net_device* device, const sockaddr* address)
{
	return B_OK;
}


net_device_module_info sTunModule = {
	{
		"network/devices/tunnel/v1",
		0,
		NULL
	},
	tunnel_init,
	tunnel_uninit,
	tunnel_up,
	tunnel_down,
	tunnel_control,
	tunnel_send_data,
	tunnel_receive_data,
	tunnel_set_mtu,
	tunnel_set_promiscuous,
	tunnel_set_media,
	tunnel_add_multicast,
	tunnel_remove_multicast,
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{}
};

module_info* modules[] = {
	(module_info*)&sTunModule,
	NULL
};
