/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Sean Brady, swangeon@gmail.com
 */

#include <ethernet.h>
#include <net_buffer.h>
#include <net_datalink.h>
#include <net_device.h>
#include <net_stack.h>
#include <net_tun.h>

#include <ByteOrder.h>
#include <Drivers.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <KernelExport.h>
#include <NetBufferUtilities.h>

#include <debug.h>
#include <errno.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_tun.h>
#include <net/if_types.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util/Random.h>


struct tun_device : net_device, DoublyLinkedListLinkImpl<tun_device>
{
	~tun_device()
	{
		free(read_buffer);
		free(write_buffer);
	}

	int fd;
	uint32 frame_size;

	void *read_buffer, *write_buffer;
	mutex read_buffer_lock, write_buffer_lock;
};

struct net_buffer_module_info* gBufferModule;
static struct net_stack_module_info* sStackModule;


//	#pragma mark -


/**
 * @brief Prepends an ethernet frame to the given net buffer packet.
 *
 * @param buffer pointer to the net buffer to prepend the ethernet frame to
 *
 * @return the status of the operation
 */
static status_t
prepend_ethernet_frame(net_buffer* buffer)
{
	NetBufferPrepend<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();
	ether_header &header = bufferHeader.Data();
	header.type = B_HOST_TO_BENDIAN_INT16(ETHER_TYPE_IP);
	memset(header.source, 0, ETHER_ADDRESS_LENGTH);
	memset(header.destination, 0, ETHER_ADDRESS_LENGTH);
	bufferHeader.Sync();
	return B_OK;
}


/**
 * @brief Removes the ethernet header from the given network buffer.
 *
 * @param buffer Pointer to the network buffer.
 *
 * @return The status of the operation.
 */
static status_t
ethernet_header_deframe(net_buffer* buffer)
{
	NetBufferHeaderRemover<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	return B_OK;
}


/**
 * @brief Initializes a TUN/TAP interface with the given name.
 *
 * @param name The name of the TUN/TAP interface.
 * @param _device A pointer to a net_device pointer for the newly created interface.
 *
 * @return The status of the initialization. Returns B_OK if successful, or an error code otherwise.
 */
status_t
tun_init(const char* name, net_device** _device)
{
	if (strncmp(name, "tun", 3)
		&& strncmp(name, "tap", 3)
		&& strncmp(name, "dns", 3)) /* iodine uses that */
		return B_BAD_VALUE;

	tun_device* device = new (std::nothrow) tun_device;
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(tun_device));
	strcpy(device->name, name);

	if (strncmp(name, "tun", 3) == 0) {
		device->flags = IFF_POINTOPOINT | IFF_LINK;
		device->type = IFT_ETHER;
	} else if (strncmp(name, "tap", 3) == 0) {
		device->flags = IFF_BROADCAST | IFF_ALLMULTI | IFF_LINK;
		device->type = IFT_ETHER;
		/* Set the first two bits to prevent it from becoming a multicast address */
		device->address.data[0] = 0x00;
		device->address.data[1] = 0xFF;
		for (int i = 2; i < ETHER_ADDRESS_LENGTH; i++) {
			int val = random_value();
			device->address.data[i] = val * 0x11;
		}
		device->address.length = ETHER_ADDRESS_LENGTH;
	} else
		return B_BAD_VALUE;

	device->mtu = 1500;
	device->media = IFM_ACTIVE | IFM_ETHER;
	device->frame_size = ETHER_MAX_FRAME_SIZE;
	device->header_length = ETHER_HEADER_LENGTH;
	device->fd = -1;
	mutex_init(&device->read_buffer_lock, "tun read_buffer");
	mutex_init(&device->write_buffer_lock, "tun write_buffer");

	*_device = device;
	return B_OK;
}


/**
 * @brief Uninitializes a TUN/TAP interface.
 *
 * @param _device the net_device to uninitialize
 *
 * @return the status of the uninitialization process
 */
status_t
tun_uninit(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	close(device->fd);
	device->fd = -1;
	mutex_destroy(&device->read_buffer_lock);
	mutex_destroy(&device->write_buffer_lock);
	delete device;
	return B_OK;
}


/**
 * @brief Sets up the TUN/TAP network interface to be used.
 *
 * @param _device the network device to set up
 *
 * @return the status of the setup process
 */
status_t
tun_up(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	device->fd = open("/dev/tap/0", O_RDWR);
	if (device->fd < 0)
		return errno;
	ioctl(device->fd, TUNSETIFF);
	return B_OK;
}


/**
 * @brief Closes the TUN/TAP network interface to not be used.
 *
 * @param _device A pointer to the net_device structure representing the tun_device.
 *
 * @return void
 */
void tun_down(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	close(device->fd);
	device->fd = -1;
}


status_t
tun_control(net_device* device, int32 op, void* argument,
			size_t length)
{
	return B_BAD_VALUE;
}


/**
 * @brief Writes data to the TUN/TAP network driver.
 *
 * @param _device the network device to get data from network stack
 * @param buffer the buffer containing the data to write
 *
 * @return the status of the operation
 *
 * @throws ErrorType if an error occurs during the operation
 */
status_t
tun_send_data(net_device* _device, net_buffer* buffer)
{
	tun_device* device = (tun_device*)_device;
	status_t status;

	if (strncmp(device->name, "tun", 3) == 0) {
		status = ethernet_header_deframe(buffer);
		if (status != B_OK)
			return B_ERROR;
	}

	if (buffer->size > device->mtu)
		return B_BAD_VALUE;

	net_buffer* allocated = NULL;
	net_buffer* original = buffer;

	MutexLocker bufferLocker;
	struct iovec iovec;
	if (gBufferModule->count_iovecs(buffer) > 1) {
		if (device->write_buffer != NULL) {
			bufferLocker.SetTo(device->write_buffer_lock, false);
			status_t status = gBufferModule->read(buffer, 0,
								device->write_buffer, buffer->size);
			if (status != B_OK)
				return status;
			iovec.iov_base = device->write_buffer;
			iovec.iov_len = buffer->size;
		} else {
			// Fall back to creating a new buffer.
			allocated = gBufferModule->duplicate(original);
			if (allocated == NULL)
				return ENOBUFS;

			buffer = allocated;

			if (gBufferModule->count_iovecs(allocated) > 1) {
				dprintf("tun_send_data: no write buffer, cannot perform scatter I/O\n");
				gBufferModule->free(allocated);
				device->stats.send.errors++;
				return B_NOT_SUPPORTED;
			}

			gBufferModule->get_iovecs(buffer, &iovec, 1);
		}
	} else {
		gBufferModule->get_iovecs(buffer, &iovec, 1);
	}

	ssize_t bytesWritten = write(device->fd, iovec.iov_base, iovec.iov_len);
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


/**
 * @brief Reads data from the TUN/TAP network driver.
 *
 * @param _device the network device to put the data into the network stack
 * @param buffer the packet to be filled and sent to the network stack
 *
 * @return the status of the operation
 *
 * @throws ErrorType if an error occurs during the operation
 */
status_t
tun_receive_data(net_device* _device, net_buffer** _buffer)
{
	/* Recieve Data */
	tun_device* device = (tun_device*)_device;
	if (device->fd == -1)
		return B_FILE_ERROR;

	// TODO: better header space
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return ENOBUFS;

	MutexLocker bufferLocker;
	struct iovec iovec;
	size_t bytesRead;
	status_t status;
	if (device->read_buffer != NULL) {
		bufferLocker.SetTo(device->read_buffer_lock, false);

		iovec.iov_base = device->read_buffer;
		iovec.iov_len = device->frame_size;
	} else {
		void* data;
		status = gBufferModule->append_size(buffer, device->mtu, &data);
		if (status == B_OK && data == NULL) {
			dprintf("tun_receive_data: no read buffer, cannot perform scattered I/O!\n");
			status = B_NOT_SUPPORTED;
		}
		if (status < B_OK) {
			gBufferModule->free(buffer);
			return status;
		}
		iovec.iov_base = data;
		iovec.iov_len = device->frame_size;
	}

	bytesRead = read(device->fd, iovec.iov_base, iovec.iov_len);
	if (bytesRead < 0 || iovec.iov_base == NULL) {
		device->stats.receive.errors++;
		status = errno;
		gBufferModule->free(buffer);
		return status;
	}

	if (strncmp(device->name, "tun", 3) == 0) {
		status = prepend_ethernet_frame(buffer);
		if (status != B_OK)
			return status;
	}

	if (iovec.iov_base == device->read_buffer)
		status = gBufferModule->append(buffer, iovec.iov_base, buffer->size);
	else
		status = gBufferModule->trim(buffer, buffer->size);

	if (status < B_OK) {
		device->stats.receive.dropped++;
		gBufferModule->free(buffer);
		return status;
	}

	device->stats.receive.bytes += bytesRead;
	device->stats.receive.packets++;

	*_buffer = buffer;
	return B_OK;
}


/**
 * @brief Sets the maximum transmission unit (MTU) for a network interface.
 *
 * @param device a pointer to the network interface
 * @param mtu the desired MTU value
 *
 * @return the status of the operation (B_OK if successful, B_BAD_VALUE if the
 *         provided MTU is out of range)
 */
status_t
tun_set_mtu(net_device* device, size_t mtu)
{
	if (mtu > 65536 || mtu < 16)
		return B_BAD_VALUE;
	device->mtu = mtu;
	return B_OK;
}

status_t
tun_set_promiscuous(net_device* device, bool promiscuous)
{
	return EOPNOTSUPP;
}


status_t
tun_set_media(net_device* device, uint32 media)
{
	return EOPNOTSUPP;
}


status_t
tun_add_multicast(net_device* device, const sockaddr* address)
{
	/* Nothing to do for multicast filters as we always accept all frames. */
	return B_OK;
}


status_t
tun_remove_multicast(net_device* device, const sockaddr* address)
{
	return B_OK;
}


/**
 * @brief Performs various operations on the TUN/TAP network interface.
 *
 * @param op The operation to be performed.
 * @param ... Additional arguments based on the operation.
 *
 * @return The status of the operation.
 */
static status_t
tun_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME, (module_info**)&sStackModule);
			if (status < B_OK)
				return status;
			status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
			if (status < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return status;
			}
			return B_OK;
		}
		case B_MODULE_UNINIT:
			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		default:
			return B_ERROR;
	}
}


net_device_module_info sTunModule = {
	{
		"network/devices/tun/v1",
		0,
		tun_std_ops
	},
	tun_init,
	tun_uninit,
	tun_up,
	tun_down,
	tun_control,
	tun_send_data,
	tun_receive_data,
	tun_set_mtu,
	tun_set_promiscuous,
	tun_set_media,
	tun_add_multicast,
	tun_remove_multicast,
};


module_info* modules[] = {
	(module_info*)&sTunModule,
	NULL};
