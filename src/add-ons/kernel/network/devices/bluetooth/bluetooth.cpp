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
#include <SupportDefs.h>

#include <errno.h>

#include <new>
#include <stdlib.h>
#include <string.h>
#include <NetBufferUtilities.h>

#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/bdaddrUtils.h>

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "bluetooth_device"
#define SUBMODULE_COLOR 34
#include <btDebug.h>
#include <btCoreData.h>
#include <btModules.h>
#include <CodeHandler.h>

#include <bluetooth/HCI/btHCI_sco.h>
#include <bluetooth/HCI/btHCI_acl.h>
#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_command.h>

#include "acl.h"



struct bluetooth_device : net_device, DoublyLinkedListLinkImpl<bluetooth_device> {
	net_buffer*	 	fBuffersRx[HCI_NUM_PACKET_TYPES];
	size_t 			fExpectedPacketSize[HCI_NUM_PACKET_TYPES];

	int fd;
	uint16 mtu;
};

/* Modules references */
net_buffer_module_info* gBufferModule = NULL;
static net_stack_module_info* sStackModule = NULL;
struct bluetooth_core_data_module_info* btCoreData = NULL;


static mutex sListLock;
static DoublyLinkedList<bluetooth_device> sDeviceList;
static sem_id sLinkChangeSemaphore;

// forward declarations
status_t bluetooth_receive_data(net_device* _device, net_buffer** _buffer);


bluetooth_device*
FindDeviceByID(hci_id hid)
{
	bluetooth_device* device;

	DoublyLinkedList<bluetooth_device>::Iterator iterator = sDeviceList.GetIterator();
	while (iterator.HasNext()) {

		device = iterator.Next();
		if (device->index == (uint32 )hid)
			return device;
	}

	return NULL;
}


status_t
Assemble(net_device* netDevice, bt_packet_t type, void* data, size_t count)
{
	bluetooth_device* bluetoothDevice = (bluetooth_device*)netDevice;
	net_buffer* nbuf = bluetoothDevice->fBuffersRx[type];

	size_t currentPacketLen = 0;

	while (count) {

		if (nbuf == NULL) {
			/* new buffer incoming */
			switch (type) {
				case BT_EVENT:
					if (count >= HCI_EVENT_HDR_SIZE) {
						struct hci_event_header* headerPkt = (struct hci_event_header*)data;
						bluetoothDevice->fExpectedPacketSize[type] = HCI_EVENT_HDR_SIZE + headerPkt->elen;

						if (count > bluetoothDevice->fExpectedPacketSize[type]) {
							// the whole packet is here so it can be already posted.
							btCoreData->PostEvent(bluetoothDevice, data, bluetoothDevice->fExpectedPacketSize[type]);
						} else {
							bluetoothDevice->fBuffersRx[type] = nbuf =
								gBufferModule->create(bluetoothDevice->fExpectedPacketSize[type]);
							nbuf->protocol = type;
						}

					} else {
						flowf("EVENT frame corrupted\n");
						return EILSEQ;
					}
					break;

				case BT_ACL:
					if (count >= HCI_ACL_HDR_SIZE) {
						struct hci_acl_header* headerPkt = (struct hci_acl_header*)data;

						bluetoothDevice->fExpectedPacketSize[type] = HCI_ACL_HDR_SIZE
							+ B_LENDIAN_TO_HOST_INT16(headerPkt->alen);

						// Create the buffer -> TODO: this allocation can fail
						bluetoothDevice->fBuffersRx[type] = nbuf = gBufferModule->create(bluetoothDevice->fExpectedPacketSize[type]);
						nbuf->protocol = type;

					} else {
						flowf("ACL frame corrupted\n");
						return EILSEQ;
					}
				break;

				case BT_SCO:

				break;

				default:
					panic("unknown packet type in assembly");
				break;
			}

			currentPacketLen = bluetoothDevice->fExpectedPacketSize[type];

		} else {
			// Continuation of a packet
			currentPacketLen = bluetoothDevice->fExpectedPacketSize[type] - nbuf->size;
		}

		if (nbuf != NULL) {

			currentPacketLen = min_c(currentPacketLen, count);

			gBufferModule->append(nbuf, data, currentPacketLen);

			if ((bluetoothDevice->fExpectedPacketSize[type] - nbuf->size) == 0 ) {

				switch(nbuf->protocol) {
					case BT_EVENT:
						btCoreData->PostEvent(netDevice, data, bluetoothDevice->fExpectedPacketSize[type]);
						break;
					case BT_ACL:
						bluetooth_receive_data(netDevice, &nbuf);
						break;
					default:

						break;
				}

				bluetoothDevice->fBuffersRx[type] = nbuf = NULL;
				bluetoothDevice->fExpectedPacketSize[type] = 0;
			} else {
	#if DEBUG_ACL
				if (type == BT_ACL)
					debugf("ACL Packet not filled size=%ld expected=%ld\n",
						nbuf->size, bluetoothDevice->fExpectedPacketSize[type]);
	#endif
			}

		}
		/* in case in the pipe there is info about the next buffer ... */
		count -= currentPacketLen;
		data = (void*)((uint8*)data + currentPacketLen);
	}

	return B_OK;
}


status_t
HciPacketHandler(void* data, int32 code, size_t size)
{
	bluetooth_device* bluetoothDevice =
		FindDeviceByID(Bluetooth::CodeHandler::Device(code));

	if (bluetoothDevice != NULL)
		return Assemble(bluetoothDevice, Bluetooth::CodeHandler::Protocol(code),
			data, size);

	return B_ERROR;
}

//	#pragma mark -

status_t
bluetooth_init(const char* name, net_device** _device)
{
	debugf("Initializing bluetooth device %s\n",name);

	// TODO: make sure this is a device in /dev/bluetooth
	if (strncmp(name, "bluetooth/h", 11))
		return B_BAD_VALUE;

	if (gBufferModule == NULL) { // lazy allocation
		status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
		if (status < B_OK)
			return status;
	}

	bluetooth_device* device = new (std::nothrow) bluetooth_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(bluetooth_device));

	// Fill
	strcpy(device->name, name);

	MutexLocker _(&sListLock);

	if (sDeviceList.IsEmpty())
		device->index = HCI_DEVICE_INDEX_OFFSET; // REVIEW: dev index
	else
		device->index = (sDeviceList.Tail())->index + 1; // TODO: index will be assigned by netstack

	// TODO: add to list whould be done in up hook
	sDeviceList.Add(device);

	debugf("Device %s %lx\n", device->name, device->index );

	*_device = device;
	return B_OK;
}


status_t
bluetooth_uninit(net_device* _device)
{
	bluetooth_device* device = (bluetooth_device *)_device;

	debugf("index %lx\n",device->index);

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
bluetooth_up(net_device* _device)
{
	bluetooth_device* device = (bluetooth_device*)_device;

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
bluetooth_down(net_device* _device)
{
	bluetooth_device *device = (bluetooth_device*)_device;

	debugf("index %ld\n",device->index);

	close(device->fd);
	device->fd = -1;
}


status_t
bluetooth_control(net_device* _device, int32 op, void* argument,
	size_t length)
{
	bluetooth_device* device = (bluetooth_device*)_device;

	debugf("index %ld\n",device->index);

	// Forward the call to the driver
	return ioctl(device->fd, op, argument, length);
}


status_t
bluetooth_send_data(net_device *_device, net_buffer* buffer)
{
	bluetooth_device* device = (bluetooth_device*)_device;
	net_buffer* curr_frame = NULL;
	net_buffer* next_frame = buffer;
	uint16 handle = buffer->type; //TODO: CodeHandler?
	uint8 flag = HCI_ACL_PACKET_START;

	debugf("index %ld try to send bt packet of %lu bytes (flags %ld):\n",device->index, buffer->size, buffer->flags);

	//TODO: ATOMIC!!! any other thread should stop here
	do {
		// Divide packet if big enough
		curr_frame = next_frame;

		if (next_frame->size > device->mtu) {
			next_frame = gBufferModule->split(curr_frame, device->mtu);
		} else {
			next_frame = NULL;
		}

		// Attach acl header
		NetBufferPrepend<struct hci_acl_header> bufferHeader(curr_frame);
		status_t status = bufferHeader.Status();
		if (status < B_OK) {
			// free the buffer
			continue;
		}

		bufferHeader->handle = pack_acl_handle_flags(handle, flag, 0); /* Handle & Flags(PB, BC) */
		bufferHeader->alen = curr_frame->size - sizeof(struct hci_acl_header);

		bufferHeader.Sync();

		// Send to driver  XXX: another inter-layer trick
		debugf("tolower nbuf %p\n",curr_frame);
		curr_frame->protocol = BT_ACL;
		((status_t(*)(hci_id id, net_buffer* nbuf))device->media)(device->index, curr_frame);

		flag = HCI_ACL_PACKET_FRAGMENT;

	} while (next_frame == NULL);

	return B_OK;
}


status_t
bluetooth_receive_data(net_device* _device, net_buffer** _buffer)
{
	bluetooth_device *device = (bluetooth_device *)_device;
	status_t status = B_OK;

	debugf("index %ld packet of %lu bytes (flags %ld):\n", device->index, (*_buffer)->size, (*_buffer)->flags);

	if (device->fd == -1)
		return B_FILE_ERROR;

	switch ((*_buffer)->protocol) {
		case BT_ACL:
			status = AclAssembly(*_buffer, device->index);
		break;
		default:
			panic("Protocol not supported");
		break;
	}

	return status;
}


status_t
bluetooth_set_mtu(net_device* _device, size_t mtu)
{
	bluetooth_device* device = (bluetooth_device*)_device;

	debugf("index %ld mtu %ld\n",device->index, mtu);

	device->mtu = mtu;

	return B_OK;
}


status_t
bluetooth_set_promiscuous(net_device* _device, bool promiscuous)
{
	bluetooth_device* device = (bluetooth_device*)_device;

	debugf("index %ld promiscuous %d\n",device->index, promiscuous);

	return EOPNOTSUPP;
}


status_t
bluetooth_set_media(net_device* device, uint32 media)
{
	debugf("index %ld media %ld\n",device->index, media);

	return EOPNOTSUPP;
}


status_t
bluetooth_add_multicast(struct net_device* _device, const sockaddr* _address)
{
	bluetooth_device* device = (bluetooth_device*)_device;

	debugf("index %ld\n",device->index);

	return EOPNOTSUPP;
}

status_t
bluetooth_remove_multicast(struct net_device* _device, const sockaddr* _address)
{
	bluetooth_device* device = (bluetooth_device*)_device;

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
		kprintf("\tname=%s index=%#lx @%p\n",device->name, device->index, device);
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
			status_t status;

			status = get_module(NET_STACK_MODULE_NAME,(module_info**)&sStackModule);
			if (status < B_OK) {
				flowf("problem getting netstack module\n");
				return status;
			}

			status = get_module(BT_CORE_DATA_MODULE_NAME,(module_info**)&btCoreData);
			if (status < B_OK) {
				flowf("problem getting datacore\n");
				return status;
			}

			new (&sDeviceList) DoublyLinkedList<bluetooth_device>;
				// static C++ objects are not initialized in the module startup

			sLinkChangeSemaphore = create_sem(0, "bt sem");
			if (sLinkChangeSemaphore < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				flowf("sem failed\n");
				return sLinkChangeSemaphore;
			}

			mutex_init(&sListLock, "bluetooth devices");

			//status = InitializeAclConnectionThread();
			debugf("Connection Thread error=%lx\n", status);

			add_debugger_command("btLocalDevices", &dump_bluetooth_devices, "Lists Bluetooth LocalDevices registered in the Stack");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			delete_sem(sLinkChangeSemaphore);

			mutex_destroy(&sListLock);
			put_module(NET_STACK_MODULE_NAME);
			put_module(BT_CORE_DATA_MODULE_NAME);
			remove_debugger_command("btLocalDevices", &dump_bluetooth_devices);
			//status_t status = QuitAclConnectionThread();

			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


net_device_module_info sBluetoothModule = {
	{
		NET_BLUETOOTH_DEVICE_NAME,
		B_KEEP_LOADED,
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
