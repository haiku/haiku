/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */


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

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "bluetooth_device"
#define SUBMODULE_COLOR 34
#include <btDebug.h>


struct bluetooth_device : net_device, DoublyLinkedListLinkImpl<bluetooth_device> {
	int		fd;
	uint16	frame_size;
};

/* Modules references */
net_buffer_module_info *gBufferModule = NULL;
static net_stack_module_info *sStackModule = NULL;

static mutex sListLock;
static DoublyLinkedList<bluetooth_device> sDeviceList;
static sem_id sLinkChangeSemaphore;

//	#pragma mark -

status_t
bluetooth_init(const char *name, net_device **_device)
{
	debugf("Initializing bluetooth device %s\n",name);
	
	// TODO: make sure this is a device in /dev/bluetooth
	if (strncmp(name, "bluetooth/h", 11))
		return B_BAD_VALUE;

	if (gBufferModule == NULL) { // lazy allocation
		status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
		if (status < B_OK)
			return status;
	}

	bluetooth_device *device = new (std::nothrow) bluetooth_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(bluetooth_device));

	// Fill
	strcpy(device->name, name);

	MutexLocker _(&sListLock);
	
	if (sDeviceList.IsEmpty())
		device->index = 0x0000007C; // REVIEW: dev index
	else
		device->index = (sDeviceList.Tail())->index + 1; // TODO: index will be assigned by netstack

	// TODO: add to list whould be done in up hook
	sDeviceList.Add(device);

	debugf("Device %s %x\n", device->name, device->index );

	*_device = device;
	return B_OK;
}


status_t
bluetooth_uninit(net_device *_device)
{
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %x\n",device->index);

	// if the device is still part of the list, remove it
	if (device->GetDoublyLinkedListLink()->next != NULL
		|| device->GetDoublyLinkedListLink()->previous != NULL
		|| device == sDeviceList.Head())
		sDeviceList.Remove(device);

	put_module(NET_BUFFER_MODULE_NAME);
	delete device;

	return B_OK;
}


status_t
bluetooth_up(net_device *_device)
{	
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld\n",device->index);

	device->fd = open(device->name, O_RDWR);
	if (device->fd < 0)
		goto err;

	return B_OK;

err:
	close(device->fd);
	device->fd = -1;
	return errno;
}


void
bluetooth_down(net_device *_device)
{	
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld\n",device->index);

	close(device->fd);
	device->fd = -1;
}


status_t
bluetooth_control(net_device *_device, int32 op, void *argument,
	size_t length)
{	
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld\n",device->index);
	
	// Forward the call to the driver
	return ioctl(device->fd, op, argument, length);
}


status_t
bluetooth_send_data(net_device *_device, net_buffer *buffer)
{
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld try to send bt packet of %lu bytes (flags %ld):\n",device->index, buffer->size, buffer->flags);
	
	/* TODO:

	*/

//  MTU and size possiblities to be checked in l2cap....
//	if (buffer->size > device->frame_size)
//		return B_BAD_VALUE;

	return B_OK;
}


status_t
bluetooth_receive_data(net_device *_device, net_buffer **_buffer)
{
	bluetooth_device *device = (bluetooth_device *)_device;
	status_t status = B_OK;

	debugf("index %ld try to send bt packet of %lu bytes (flags %ld):\n", device->index, (*_buffer)->size, (*_buffer)->flags);

	if (device->fd == -1)
		return B_FILE_ERROR;

	/* TODO:

	*/

	return status;
}


status_t
bluetooth_set_mtu(net_device *_device, size_t mtu)
{
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld mtu %ld\n",device->index, mtu);

	device->mtu = mtu;

	return B_OK;
}


status_t
bluetooth_set_promiscuous(net_device *_device, bool promiscuous)
{
	bluetooth_device *device = (bluetooth_device *)_device;

	debugf("index %ld promiscuous %d\n",device->index, promiscuous);

	return EOPNOTSUPP;
}


status_t
bluetooth_set_media(net_device *device, uint32 media)
{
	debugf("index %ld media %ld\n",device->index, media);
	
	return EOPNOTSUPP;
}


status_t
bluetooth_add_multicast(struct net_device *_device, const sockaddr *_address)
{
	bluetooth_device* device = (bluetooth_device *)_device;

	debugf("index %ld\n",device->index);

	return EOPNOTSUPP;
}

status_t
bluetooth_remove_multicast(struct net_device *_device, const sockaddr *_address)
{
	bluetooth_device* device = (bluetooth_device *)_device;

	debugf("index %ld\n",device->index);

	return EOPNOTSUPP;
}



static int
dump_bluetooth_devices(int argc, char** argv)
{
	bluetooth_device*	device;

	DoublyLinkedList<bluetooth_device>::Iterator iterator = sDeviceList.GetIterator();
	while (iterator.HasNext()) {

		device = iterator.Next();
		kprintf("\tname=%s index=%#x\n",device->name, device->index);
	}
	
	return 0;
}


static status_t
bluetooth_std_ops(int32 op, ...)
{

	flowf("\n");
	
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME,
				(module_info **)&sStackModule);
			if (status < B_OK)
				return status;

			new (&sDeviceList) DoublyLinkedList<bluetooth_device>;
				// static C++ objects are not initialized in the module startup

			sLinkChangeSemaphore = create_sem(0, "bt sem");
			if (sLinkChangeSemaphore < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return sLinkChangeSemaphore;
			}

			mutex_init(&sListLock, "bluetooth devices");

			add_debugger_command("btLocalDevices", &dump_bluetooth_devices, "Lists Bluetooth LocalDevices registered in the Stack");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			delete_sem(sLinkChangeSemaphore);

			mutex_destroy(&sListLock);
			put_module(NET_STACK_MODULE_NAME);
			
			remove_debugger_command("btLocalDevices", &dump_bluetooth_devices);
			
			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


net_device_module_info sBluetoothModule = {
	{
		"network/devices/bluetooth/v1",
		0,
		bluetooth_std_ops
	},
	bluetooth_init,
	bluetooth_uninit,
	bluetooth_up,
	bluetooth_down,
	bluetooth_control,
	bluetooth_send_data,
	bluetooth_receive_data,
	bluetooth_set_mtu,
	bluetooth_set_promiscuous,
	bluetooth_set_media,
    bluetooth_add_multicast,
	bluetooth_remove_multicast,
};

module_info *modules[] = {
	(module_info *)&sBluetoothModule,

	NULL
};
