/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <ethernet.h>

#include <ether_driver.h>
#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <KernelExport.h>

#include <errno.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>


struct ethernet_device : net_device, DoublyLinkedListLinkImpl<ethernet_device> {
	int		fd;
	uint32	frame_size;
};

static const bigtime_t kLinkCheckInterval = 1000000;
	// 1 second

net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;

static mutex sListLock;
static DoublyLinkedList<ethernet_device> sCheckList;
static sem_id sLinkChangeSemaphore;
static thread_id sLinkCheckerThread;


static status_t
update_link_state(ethernet_device *device, bool notify = true)
{
	ether_link_state state;
	if (ioctl(device->fd, ETHER_GET_LINK_STATE, &state,
			sizeof(ether_link_state)) < 0) {
		// This device does not support retrieving the link
		return B_NOT_SUPPORTED;
	}

	state.media |= IFM_ETHER;
		// make sure the media type is returned correctly

	if (device->media != state.media
		|| device->link_quality != state.quality
		|| device->link_speed != state.speed) {
		device->media = state.media;
		device->link_quality = state.quality;
		device->link_speed = state.speed;

		if (device->media & IFM_ACTIVE)
			device->flags |= IFF_LINK;
		else
			device->flags &= ~IFF_LINK;

		dprintf("%s: media change, media 0x%0x quality %u speed %u\n",
				device->name, (unsigned int)device->media,
				(unsigned int)device->link_quality,
				(unsigned int)device->link_speed);

		if (notify)
			sStackModule->device_link_changed(device);
	}

	return B_OK;
}


static status_t
ethernet_link_checker(void *)
{
	while (true) {
		status_t status = acquire_sem_etc(sLinkChangeSemaphore, 1,
			B_RELATIVE_TIMEOUT, kLinkCheckInterval);
		if (status == B_BAD_SEM_ID)
			break;

		MutexLocker _(sListLock);

		if (sCheckList.IsEmpty())
			break;

		// check link state of all existing devices

		DoublyLinkedList<ethernet_device>::Iterator iterator
			= sCheckList.GetIterator();
		while (iterator.HasNext()) {
			update_link_state(iterator.Next());
		}
	}

	return B_OK;
}


//	#pragma mark -


status_t
ethernet_init(const char *name, net_device **_device)
{
	// make sure this is a device in /dev/net, but not the
	// networking (userland) stack driver
	if (strncmp(name, "/dev/net/", 9)
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
	device->flags = IFF_BROADCAST | IFF_LINK;
	device->type = IFT_ETHER;
	device->mtu = 1500;
	device->media = IFM_ACTIVE | IFM_ETHER;
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

	uint64 dummy;
	if (ioctl(device->fd, ETHER_INIT, &dummy, sizeof(dummy)) < 0)
		goto err;

	if (ioctl(device->fd, ETHER_GETADDR, device->address.data, ETHER_ADDRESS_LENGTH) < 0)
		goto err;

	if (ioctl(device->fd, ETHER_GETFRAMESIZE, &device->frame_size, sizeof(uint32)) < 0) {
		// this call is obviously optional
		device->frame_size = ETHER_MAX_FRAME_SIZE;
	}

	if (update_link_state(device, false) == B_OK) {
		// device supports retrieval of the link state

		// Set the change notification semaphore; doesn't matter
		// if this is supported by the device or not
		ioctl(device->fd, ETHER_SET_LINK_STATE_SEM, &sLinkChangeSemaphore,
			sizeof(sem_id));

		MutexLocker _(&sListLock);

		if (sCheckList.IsEmpty()) {
			// start thread
			sLinkCheckerThread = spawn_kernel_thread(ethernet_link_checker,
				"ethernet link state checker", B_LOW_PRIORITY, NULL);
			if (sLinkCheckerThread >= B_OK)
				resume_thread(sLinkCheckerThread);
		}

		sCheckList.Add(device);
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

	MutexLocker _(sListLock);

	// if the device is still part of the list, remove it
	if (device->GetDoublyLinkedListLink()->next != NULL
		|| device->GetDoublyLinkedListLink()->previous != NULL
		|| device == sCheckList.Head())
		sCheckList.Remove(device);

	close(device->fd);
	device->fd = -1;
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

//dprintf("try to send ethernet packet of %lu bytes (flags %ld):\n", buffer->size, buffer->flags);
	if (buffer->size > device->frame_size || buffer->size < ETHER_HEADER_LENGTH)
		return B_BAD_VALUE;

	net_buffer *allocated = NULL;
	net_buffer *original = buffer;

	if (gBufferModule->count_iovecs(buffer) > 1) {
		// TODO: for now, create a new buffer containing the data
		buffer = gBufferModule->duplicate(original);
		if (buffer == NULL)
			return ENOBUFS;

		allocated = buffer;

		if (gBufferModule->count_iovecs(buffer) > 1) {
			dprintf("scattered I/O is not yet supported by ethernet device.\n");
			gBufferModule->free(buffer);
			device->stats.send.errors++;
			return B_NOT_SUPPORTED;
		}
	}

	struct iovec iovec;
	gBufferModule->get_iovecs(buffer, &iovec, 1);

//dump_block((const char *)iovec.iov_base, buffer->size, "  ");
	ssize_t bytesWritten = write(device->fd, iovec.iov_base, iovec.iov_len);
//dprintf("sent: %ld\n", bytesWritten);

	if (bytesWritten < 0) {
		device->stats.send.errors++;
		if (allocated)
			gBufferModule->free(allocated);
		return errno;
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

	if (device->fd == -1)
		return B_FILE_ERROR;

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
		status = errno;
		goto err;
	}
//dump_block((const char *)data, bytesRead, "rcv: ");

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
ethernet_set_promiscuous(net_device *_device, bool promiscuous)
{
	ethernet_device *device = (ethernet_device *)_device;

	int32 value = (int32)promiscuous;
	if (ioctl(device->fd, ETHER_SETPROMISC, &value, sizeof(value)) < 0)
		return B_NOT_SUPPORTED;

	return B_OK;
}


status_t
ethernet_set_media(net_device *device, uint32 media)
{
	return B_NOT_SUPPORTED;
}


status_t
ethernet_add_multicast(struct net_device *_device, const sockaddr *_address)
{
	ethernet_device *device = (ethernet_device *)_device;

	if (_address->sa_family != AF_LINK)
		return B_BAD_VALUE;

	const sockaddr_dl *address = (const sockaddr_dl *)_address;
	if (address->sdl_type != IFT_ETHER)
		return B_BAD_VALUE;

	return ioctl(device->fd, ETHER_ADDMULTI, LLADDR(address), 6);
}


status_t
ethernet_remove_multicast(struct net_device *_device, const sockaddr *_address)
{
	ethernet_device *device = (ethernet_device *)_device;

	if (_address->sa_family != AF_LINK)
		return B_BAD_VALUE;

	const sockaddr_dl *address = (const sockaddr_dl *)_address;
	if (address->sdl_type != IFT_ETHER)
		return B_BAD_VALUE;

	return ioctl(device->fd, ETHER_REMMULTI, LLADDR(address), 6);
}


static status_t
ethernet_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME,
				(module_info **)&sStackModule);
			if (status < B_OK)
				return status;

			new (&sCheckList) DoublyLinkedList<ethernet_device>;
				// static C++ objects are not initialized in the module startup

			sLinkCheckerThread = -1;

			sLinkChangeSemaphore = create_sem(0, "ethernet link change");
			if (sLinkChangeSemaphore < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return sLinkChangeSemaphore;
			}

			mutex_init(&sListLock, "ethernet devices");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			delete_sem(sLinkChangeSemaphore);

			status_t status;
			wait_for_thread(sLinkCheckerThread, &status);

			mutex_destroy(&sListLock);
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		}

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
	ethernet_add_multicast,
	ethernet_remove_multicast,
};

module_info *modules[] = {
	(module_info *)&sEthernetModule,
	NULL
};
