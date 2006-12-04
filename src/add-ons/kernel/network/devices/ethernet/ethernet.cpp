/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <ether_driver.h>
#include <ethernet.h>
#include <net_buffer.h>
#include <net_device.h>

#include <KernelExport.h>

#include <errno.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <new>
#include <stdlib.h>
#include <string.h>


struct ethernet_device : net_device {
	int		fd;
	uint32	frame_size;
};


struct net_buffer_module_info *gBufferModule;


status_t
ethernet_init(const char *name, net_device **_device)
{
	// make sure this is a device in /dev/net, but not the
	// networking (userland) stack driver
	if (strncmp(name, "/dev/net/", 9) || !strcmp(name, "/dev/net/stack")
		|| !strcmp(name, "/dev/net/userland_server"))
		return B_BAD_VALUE;

	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		return status;

	ethernet_device *device = new (std::nothrow) ethernet_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(ethernet_device));

	strcpy(device->name, name);
	device->flags = IFF_BROADCAST;
	device->type = IFT_ETHER;
	device->mtu = 1500;
	device->header_length = ETHER_HEADER_LENGTH;
	device->fd = -1;

	*_device = device;
	return B_OK;
}


status_t
ethernet_uninit(net_device *device)
{
	put_module(NET_BUFFER_MODULE_NAME);
	delete device;

	return B_OK;
}


status_t
ethernet_up(net_device *_device)
{
	ethernet_device *device = (ethernet_device *)_device;

	device->fd = open(device->name, O_RDWR);
	if (device->fd < 0)
		return errno;

	ether_init_params params;
	memset(&params, 0, sizeof(ether_init_params));
	if (ioctl(device->fd, ETHER_INIT, &params, sizeof(ether_init_params)) < 0)
		goto err;

	if (ioctl(device->fd, ETHER_GETADDR, device->address.data, ETHER_ADDRESS_LENGTH) < 0)
		goto err;

	if (ioctl(device->fd, ETHER_GETFRAMESIZE, &device->frame_size, sizeof(uint32)) < 0) {
		// this call is obviously optional
		device->frame_size = ETHER_MAX_FRAME_SIZE;
	}

	device->address.length = ETHER_ADDRESS_LENGTH;
	device->mtu = device->frame_size - device->header_length;
	return B_OK;

err:
	close(device->fd);
	device->fd = -1;
	return errno;
}


void
ethernet_down(net_device *_device)
{
	ethernet_device *device = (ethernet_device *)_device;
	close(device->fd);
}


status_t
ethernet_control(net_device *_device, int32 op, void *argument,
	size_t length)
{
	ethernet_device *device = (ethernet_device *)_device;
	return ioctl(device->fd, op, argument, length);
}


status_t
ethernet_send_data(net_device *_device, net_buffer *buffer)
{
	ethernet_device *device = (ethernet_device *)_device;

dprintf("try to send ethernet packet of %lu bytes (flags %ld):\n", buffer->size, buffer->flags);
	if (buffer->size > device->frame_size || buffer->size < ETHER_HEADER_LENGTH)
		return B_BAD_VALUE;

	net_buffer *allocated = NULL;
	net_buffer *original = buffer;

	if (gBufferModule->count_iovecs(buffer) > 1) {
		// TODO: for now, create a new buffer containing the data
		net_buffer *original = buffer;

		buffer = gBufferModule->duplicate(original);
		if (buffer == NULL)
			return ENOBUFS;

		allocated = buffer;

		if (gBufferModule->count_iovecs(buffer) > 1) {
			dprintf("scattered I/O is not yet supported by ethernet device.\n");
			return B_NOT_SUPPORTED;
		}
	}

	struct iovec iovec;
	gBufferModule->get_iovecs(buffer, &iovec, 1);

dump_block((const char *)iovec.iov_base, buffer->size, "  ");
	ssize_t bytesWritten = write(device->fd, iovec.iov_base, iovec.iov_len);
dprintf("sent: %ld\n", bytesWritten);

	if (bytesWritten < 0) {
		device->stats.send.errors++;
		if (allocated)
			gBufferModule->free(allocated);
		return bytesWritten;
	}

	device->stats.send.packets++;
	device->stats.send.bytes += bytesWritten;

	gBufferModule->free(original);
	if (allocated)
		gBufferModule->free(allocated);
	return B_OK;
}


status_t
ethernet_receive_data(net_device *_device, net_buffer **_buffer)
{
	ethernet_device *device = (ethernet_device *)_device;

	// TODO: better header space
	net_buffer *buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return ENOBUFS;

	// TODO: this only works for standard ethernet frames - we need iovecs
	//	for jumbo frame support (or a separate read buffer)!
	//	It would be even nicer to get net_buffers from the ethernet driver
	//	directly.

	ssize_t bytesRead;
	void *data;

	status_t status = gBufferModule->append_size(buffer, device->frame_size, &data);
	if (status == B_OK && data == NULL) {
		dprintf("scattered I/O is not yet supported by ethernet device.\n");
		status = B_NOT_SUPPORTED;
	}
	if (status < B_OK)
		goto err;

	bytesRead = read(device->fd, data, device->frame_size);
	if (bytesRead < 0) {
		device->stats.receive.errors++;
		status = bytesRead;
		goto err;
	}

	status = gBufferModule->trim(buffer, bytesRead);
	if (status < B_OK) {
		device->stats.receive.dropped++;
		goto err;
	}

	device->stats.receive.bytes += bytesRead;
	device->stats.receive.packets++;

	*_buffer = buffer;
	return B_OK;

err:
	gBufferModule->free(buffer);
	return status;
}


status_t
ethernet_set_mtu(net_device *_device, size_t mtu)
{
	ethernet_device *device = (ethernet_device *)_device;

	if (mtu > device->frame_size - ETHER_HEADER_LENGTH
		|| mtu <= ETHER_HEADER_LENGTH + 10)
		return B_BAD_VALUE;

	device->mtu = mtu;
	return B_OK;
}


status_t
ethernet_set_promiscuous(net_device *device, bool promiscuous)
{
	return EOPNOTSUPP;
}


status_t
ethernet_set_media(net_device *device, uint32 media)
{
	return EOPNOTSUPP;
}


status_t
ethernet_get_multicast_addrs(struct net_device *device,
	net_hardware_address **addressArray, uint32 count)
{
	return EOPNOTSUPP;
}


status_t
ethernet_set_multicast_addrs(struct net_device *device, 
	const net_hardware_address **addressArray, uint32 count)
{
	return EOPNOTSUPP;
}


static status_t
ethernet_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_device_module_info sEthernetModule = {
	{
		"network/devices/ethernet/v1",
		0,
		ethernet_std_ops
	},
	ethernet_init,
	ethernet_uninit,
	ethernet_up,
	ethernet_down,
	ethernet_control,
	ethernet_send_data,
	ethernet_receive_data,
	ethernet_set_mtu,
	ethernet_set_promiscuous,
	ethernet_set_media,
	ethernet_get_multicast_addrs,
	ethernet_set_multicast_addrs
};

module_info *modules[] = {
	(module_info *)&sEthernetModule,
	NULL
};
