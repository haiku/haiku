/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Lin Longzhou, linlongzhou@163.com
 */


#include <errno.h>
#include <stdlib.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <new>

#include <ethernet.h>
#include <ether_driver.h>
#include <net_buffer.h>
#include <net_datalink_protocol.h>

#include <NetBufferUtilities.h>
#include <net_stack.h>

#include <KernelExport.h>

#include <KPPPManager.h>
#include <ppp_device.h>

#define PPP_HEADER_LENGTH (8 + ETHER_HEADER_LENGTH)


net_buffer* create_buffer_for_frame(uint8 *frame, uint16 frame_size);

static const bigtime_t kLinkCheckInterval = 1000000;
	// 1 second

net_buffer_module_info *gBufferModule;
ppp_interface_module_info* gPPPInterfaceModule;
static net_stack_module_info *sStackModule;

static mutex sListLock;
static DoublyLinkedList<ppp_device> sCheckList;
static sem_id sLinkChangeSemaphore;
static thread_id sLinkCheckerThread;


//	#pragma mark -
//

status_t
ppp_init(const char *name, net_device **_device)
{
	// ppp device
	dprintf("%s: entering!\n", __func__);

	if (strncmp(name, "ppp", 3)) {
		dprintf("[%s] not ppp device\n", name);
		return B_BAD_VALUE;
	}

	dprintf("[%s] is ppp device\n", name);

	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		return status;

	// ppp_device *device = new (std::nothrow) ppp_device;
	ppp_interface_id idInterface = gPPPInterfaceModule->CreateInterfaceWithName(name, 0);
	KPPPInterface * pppInterface = gPPPInterfaceModule->GetInterface(idInterface);

	if (idInterface <= 0 || pppInterface == NULL) {
		if (idInterface <= 0)
			dprintf("%s: idInterface: %" B_PRIu32 "\n", __func__, idInterface);
		else
			dprintf("%s: pppInterface == NULL %" B_PRIu32 "\n", __func__, idInterface);
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	// Ifnet available until interface up phase
	ppp_device *device = (ppp_device *)pppInterface->Ifnet();
	if (device == NULL) {
		dprintf("%s: can not get ppp_device\n", __func__);
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	strcpy(device->name, name);
	device->flags = (IFF_BROADCAST | IFF_LINK) & (~IFF_UP);
	device->type = IFT_PPP;
	device->mtu = 1492;
	device->frame_size = 1500;
	device->media = IFM_ACTIVE | IFM_ETHER;
	device->header_length = PPP_HEADER_LENGTH;

	status =sStackModule->init_fifo(&(device->ppp_fifo), "ppp_fifo", 10 * 1500);
		// 10 ppp packet at most
	if (status < B_OK)
		return(status);

	TRACE("[%s] finish\n", "allocate ppp_device");
	*_device = device;

	return B_OK;
}


status_t
ppp_uninit(net_device *device)
{
	dprintf("%s:  uninit ppp\n", __func__);
	ppp_device *ppp_dev=(ppp_device *)device;

	sStackModule->uninit_fifo(&(ppp_dev->ppp_fifo));
	put_module(NET_BUFFER_MODULE_NAME);

	// delete ppp_dev->KPPP_Interface;
	delete ppp_dev;

	return B_OK;
}


status_t
ppp_up(net_device *_device)
{
	dprintf("ppp_up\n");
	ppp_device *device = (ppp_device *)_device;

	if (device->KPPP_Interface == NULL) {
		dprintf("%s: warning! can not find pppinterface KPPP_Interface\n", __func__);
		return B_ERROR;
	}

	if (device->KPPP_Interface->Up() != true) {
		dprintf("%s: warning! KPPP_Interface->Up() failure\n", __func__);
		return B_ERROR;
	}

	device->mtu = device->frame_size - device->header_length;
	// s_pppoe_dev = device;

	dprintf("%s: congratulations! Find pppinterface (device->KPPP_Interface)\n", __func__);

	return B_OK;
}


void
ppp_down(net_device *_device)
{
	dprintf("%s\n", __func__);
	ppp_device *device = (ppp_device *)_device;
	device->KPPP_Interface->Down();

	MutexLocker _(sListLock);

	// if the device is still part of the list, remove it
	if (device->GetDoublyLinkedListLink()->next != NULL
		|| device->GetDoublyLinkedListLink()->previous != NULL
		|| device == sCheckList.Head())
		sCheckList.Remove(device);

	// device->KPPP_Interface = NULL;

	return;
}


status_t
ppp_control(net_device *_device, int32 op, void *argument,
	size_t length)
{
	TRACE("%s op:%" B_PRId32 "\n", __func__, op);
	ppp_device *device = (ppp_device *)_device;

	if (device->KPPP_Interface == NULL) {
		dprintf("%s: can not find KPPP_Interface for ppp\n", __func__);
		return B_OK;
	}

	return device->KPPP_Interface->Control(op, argument, length);

	return B_OK;
}


status_t
ppp_send_data(net_device *_device, net_buffer *buffer)
{
	TRACE("%s\n", __func__);
	ppp_device *device = (ppp_device *)_device;

	if (buffer->size > device->frame_size || buffer->size < device->header_length) {
		device->stats.send.errors++;
		dprintf("sorry! fail send ppp packet, size wrong!\n");
		return B_BAD_VALUE;
	}

	if (device->KPPP_Interface == NULL) {
		device->stats.send.errors++;
		dprintf("Fail send ppp packet, no eth for ppp!\n");
		return B_BAD_VALUE;
	}

	size_t net_buffer_size = buffer->size;
		// store buffer->size in case buffer freed in KPPP_Interface->Send
	status_t status = device->KPPP_Interface->Send(buffer, 0x0021); // IP_PROTOCOL 0x0021

	if (status != B_OK) {
		device->stats.send.errors++;
		dprintf("KPPP_Interface->Send(buffer, 0x0021 IP) fail\n");
		return B_BAD_VALUE;
	}

	device->stats.send.packets++;
	device->stats.send.bytes += net_buffer_size;

	return B_OK;
}


status_t
ppp_receive_data(net_device *_device, net_buffer **_buffer)
{
	TRACE("%s\n", __func__);
	ppp_device *device = (ppp_device *)_device;

	if (device->KPPP_Interface == NULL)
		return B_FILE_ERROR;

	TRACE("%s: trying fifo_dequeue_buffer\n", __func__);
	status_t status = sStackModule->fifo_dequeue_buffer(&(device->ppp_fifo), 0, 10000000, _buffer);

	if (status < B_OK) {
		TRACE("sorry! can not fifo_dequeue_buffer!\n");
		return status;
	}

	// (*_buffer)->interface_address = NULL; // strange need to put here
	device->stats.receive.bytes += (*_buffer)->size;
	device->stats.receive.packets++;

	return B_OK;
}


status_t
ppp_set_mtu(net_device *_device, size_t mtu)
{
	ppp_device *device = (ppp_device *)_device;

	if (mtu > device->frame_size - ETHER_HEADER_LENGTH - 8
		|| mtu <= ETHER_HEADER_LENGTH + 8 + 10)
		return B_BAD_VALUE;

	device->mtu = mtu;
	return B_OK;
}


status_t
ppp_set_promiscuous(net_device *_device, bool promiscuous)
{
	return B_NOT_SUPPORTED;
}


status_t
ppp_set_media(net_device *device, uint32 media)
{
	return B_NOT_SUPPORTED;
}


status_t
ppp_add_multicast(struct net_device *_device, const sockaddr *_address)
{
	// ppp_device *device = (ppp_device *)_device;

	if (_address->sa_family != AF_LINK)
		return B_BAD_VALUE;

	const sockaddr_dl *address = (const sockaddr_dl *)_address;
	if (address->sdl_type != IFT_ETHER)
		return B_BAD_VALUE;

	return B_NOT_SUPPORTED;
}


status_t
ppp_remove_multicast(struct net_device *_device, const sockaddr *_address)
{
	// ppp_device *device = (ppp_device *)_device;

	if (_address->sa_family != AF_LINK)
		return B_BAD_VALUE;

	const sockaddr_dl *address = (const sockaddr_dl *)_address;
	if (address->sdl_type != IFT_ETHER)
		return B_BAD_VALUE;

	return B_NOT_SUPPORTED;
}


static status_t
ppp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME,
				(module_info **)&sStackModule);
			if (status < B_OK)
				return status;

			status = get_module(PPP_INTERFACE_MODULE_NAME,
				(module_info**)&gPPPInterfaceModule);
			if (status < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return status;
			}

			new (&sCheckList) DoublyLinkedList<ppp_device>;
				// static C++ objects are not initialized in the module startup

			sLinkCheckerThread = -1;

			sLinkChangeSemaphore = create_sem(0, "ppp link change");
			if (sLinkChangeSemaphore < B_OK) {
				put_module(PPP_INTERFACE_MODULE_NAME);
				put_module(NET_STACK_MODULE_NAME);
				return sLinkChangeSemaphore;
			}

			mutex_init(&sListLock, "ppp devices");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			delete_sem(sLinkChangeSemaphore);

			status_t status;
			wait_for_thread(sLinkCheckerThread, &status);

			mutex_destroy(&sListLock);

			put_module(PPP_INTERFACE_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


net_device_module_info spppModule = {
	{
		"network/devices/ppp/v1",
		0,
		ppp_std_ops
	},
	ppp_init,
	ppp_uninit,
	ppp_up,
	ppp_down,
	ppp_control,
	ppp_send_data,
	ppp_receive_data,
	ppp_set_mtu,
	ppp_set_promiscuous,
	ppp_set_media,
	ppp_add_multicast,
	ppp_remove_multicast,
};


module_info *modules[] = {
	(module_info *)&spppModule,
	NULL
};
