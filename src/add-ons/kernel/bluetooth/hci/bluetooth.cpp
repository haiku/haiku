/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */

#include <new>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <lock.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "hci"
#define SUBMODULE_COLOR 34
#include <btDebug.h>
#include <btCoreData.h>
#include <btModules.h>
#include <CodeHandler.h>
#define KERNEL_LAND
#include <PortListener.h>
#undef KERNEL_LAND

#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_acl.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <bluetooth/HCI/btHCI_sco.h>
#include <bluetooth/bdaddrUtils.h>

#include "acl.h"


typedef PortListener<void,
	HCI_MAX_FRAME_SIZE, // Event Body can hold max 255 + 2 header
	24					// Some devices have sent chunks of 24 events(inquiry result)
	> BluetoothRawDataPort;


// Modules references
net_buffer_module_info* gBufferModule = NULL;
struct bluetooth_core_data_module_info* btCoreData = NULL;

static mutex sListLock;
static sem_id sLinkChangeSemaphore;
static DoublyLinkedList<bluetooth_device> sDeviceList;

BluetoothRawDataPort* BluetoothRXPort;

// forward declarations
status_t HciPacketHandler(void* data, int32 code, size_t size);


bluetooth_device*
FindDeviceByID(hci_id hid)
{
	bluetooth_device* device;

	DoublyLinkedList<bluetooth_device>::Iterator iterator
		= sDeviceList.GetIterator();

	while (iterator.HasNext()) {
		device = iterator.Next();
		if (device->index == hid)
			return device;
	}

	return NULL;
}


status_t
PostTransportPacket(hci_id hid, bt_packet_t type, void* data, size_t count)
{
	uint32 code = 0;

	Bluetooth::CodeHandler::SetDevice(&code, hid);
	Bluetooth::CodeHandler::SetProtocol(&code, type);

	return BluetoothRXPort->Trigger(code, data, count);
}


status_t
Assemble(bluetooth_device* bluetoothDevice, bt_packet_t type, void* data,
	size_t count)
{
	net_buffer* nbuf = bluetoothDevice->fBuffersRx[type];

	size_t currentPacketLen = 0;

	while (count) {

		if (nbuf == NULL) {
			// new buffer incoming
			switch (type) {
				case BT_EVENT:
					if (count >= HCI_EVENT_HDR_SIZE) {
						struct hci_event_header* headerPacket
							= (struct hci_event_header*)data;
						bluetoothDevice->fExpectedPacketSize[type]
							= HCI_EVENT_HDR_SIZE + headerPacket->elen;

						if (count >= bluetoothDevice->fExpectedPacketSize[type]) {
							// the whole packet is here so it can be already posted.
							flowf("EVENT posted in HCI!!!\n");
							btCoreData->PostEvent(bluetoothDevice, data,
								bluetoothDevice->fExpectedPacketSize[type]);

						} else {
							nbuf = gBufferModule->create(
								bluetoothDevice->fExpectedPacketSize[type]);
							bluetoothDevice->fBuffersRx[type] = nbuf;

							nbuf->protocol = type;
						}

					} else {
						panic("EVENT frame corrupted\n");
						return EILSEQ;
					}
					break;

				case BT_ACL:
					if (count >= HCI_ACL_HDR_SIZE) {
						struct hci_acl_header* headerPkt = (struct hci_acl_header*)data;

						bluetoothDevice->fExpectedPacketSize[type] = HCI_ACL_HDR_SIZE
							+ B_LENDIAN_TO_HOST_INT16(headerPkt->alen);

						// Create the buffer -> TODO: this allocation can fail
						nbuf = gBufferModule->create(
							bluetoothDevice->fExpectedPacketSize[type]);
						bluetoothDevice->fBuffersRx[type] = nbuf;

						nbuf->protocol = type;
					} else {
						panic("ACL frame corrupted\n");
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

			if ((bluetoothDevice->fExpectedPacketSize[type] - nbuf->size) == 0) {

				switch (nbuf->protocol) {
					case BT_EVENT:
						panic("need to send full buffer to btdatacore!\n");
						btCoreData->PostEvent(bluetoothDevice, data,
							bluetoothDevice->fExpectedPacketSize[type]);

						break;
					case BT_ACL:
						// TODO: device descriptor has been fetched better not
						// pass id again
						flowf("ACL parsed in ACL!\n");
						AclAssembly(nbuf, bluetoothDevice->index);
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
		// in case in the pipe there is info about the next buffer
		count -= currentPacketLen;
		data = (void*)((uint8*)data + currentPacketLen);
	}

	return B_OK;
}


status_t
HciPacketHandler(void* data, int32 code, size_t size)
{
	hci_id deviceId = Bluetooth::CodeHandler::Device(code);

	bluetooth_device* bluetoothDevice = FindDeviceByID(deviceId);

	debugf("to assemble %ld bytes of %ld\n", size, deviceId);

	if (bluetoothDevice != NULL)
		return Assemble(bluetoothDevice, Bluetooth::CodeHandler::Protocol(code),
			data, size);
	else {
		debugf("Device %ld could not be matched\n", deviceId);
	}

	return B_ERROR;
}


//	#pragma mark -


status_t
RegisterDriver(bt_hci_transport_hooks* hooks, bluetooth_device** _device)
{

	bluetooth_device* device = new (std::nothrow) bluetooth_device;
	if (device == NULL)
		return B_NO_MEMORY;

	for (int index = 0; index < HCI_NUM_PACKET_TYPES; index++) {
		device->fBuffersRx[index] = NULL;
		device->fExpectedPacketSize[index] = 0;
	}

	device->info = NULL; // not yet used
	device->hooks = hooks;
	device->supportedPacketTypes = (HCI_DM1 | HCI_DH1 | HCI_HV1);
	device->linkMode = (HCI_LM_ACCEPT);
	device->mtu = L2CAP_MTU_MINIMUM; // TODO: ensure specs min value

	MutexLocker _(&sListLock);

	if (sDeviceList.IsEmpty())
		device->index = HCI_DEVICE_INDEX_OFFSET; // REVIEW: dev index
	else
		device->index = (sDeviceList.Tail())->index + 1; // REVIEW!
		flowf("List not empty\n");

	sDeviceList.Add(device);

	debugf("Device %lx\n", device->index );

	*_device = device;

	return B_OK;
}


status_t
UnregisterDriver(hci_id id)
{
	bluetooth_device* device = FindDeviceByID(id);

	if (device == NULL)
		return B_ERROR;

	if (device->GetDoublyLinkedListLink()->next != NULL
		|| device->GetDoublyLinkedListLink()->previous != NULL
		|| device == sDeviceList.Head())
		sDeviceList.Remove(device);

	delete device;

	return B_OK;
}


// PostACL
status_t
PostACL(hci_id hciId, net_buffer* buffer)
{
	net_buffer* curr_frame = NULL;
	net_buffer* next_frame = buffer;
	uint8 flag = HCI_ACL_PACKET_START;

	if (buffer == NULL)
		panic("passing null buffer");

	uint16 handle = buffer->type; // TODO: CodeHandler

	bluetooth_device* device = FindDeviceByID(hciId);

	if (device == NULL) {
		debugf("No device %lx", hciId);
		return B_ERROR;
	}

	debugf("index %lx try to send bt packet of %lu bytes (flags %ld):\n",
		device->index, buffer->size, buffer->flags);

	// TODO: ATOMIC! any other thread should stop here
	do {
		// Divide packet if big enough
		curr_frame = next_frame;

		if (curr_frame->size > device->mtu) {
			next_frame = gBufferModule->split(curr_frame, device->mtu);
		} else {
			next_frame = NULL;
		}

		// Attach acl header
		{
			NetBufferPrepend<struct hci_acl_header> bufferHeader(curr_frame);
			status_t status = bufferHeader.Status();
			if (status < B_OK) {
				// free the buffer
				continue;
			}

			bufferHeader->handle = pack_acl_handle_flags(handle, flag, 0);
			bufferHeader->alen = curr_frame->size - sizeof(struct hci_acl_header);
		}

		// Send to driver
		curr_frame->protocol = BT_ACL;

		debugf("Tolower nbuf %p!\n", curr_frame);
		// We could pass a cookie and avoid the driver fetch the Id
		device->hooks->SendACL(device->index, curr_frame);
		flag = HCI_ACL_PACKET_FRAGMENT;

	} while (next_frame != NULL);

	return B_OK;
}


status_t
PostSCO(hci_id hciId, net_buffer* buffer)
{
	return B_ERROR;
}


status_t
PostESCO(hci_id hciId, net_buffer* buffer)
{
	return B_ERROR;
}


static int
dump_bluetooth_devices(int argc, char** argv)
{
	bluetooth_device*	device;

	DoublyLinkedList<bluetooth_device>::Iterator iterator
		= sDeviceList.GetIterator();

	while (iterator.HasNext()) {
		device = iterator.Next();
		kprintf("\tindex=%#lx @%p hooks=%p\n",device->index, device, device->hooks);
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

			status = get_module(NET_BUFFER_MODULE_NAME,	(module_info**)&gBufferModule);
			if (status < B_OK) {
				panic("no way Dude we need that!");
				return status;
			}

			status = get_module(BT_CORE_DATA_MODULE_NAME,(module_info**)&btCoreData);
			if (status < B_OK) {
				flowf("problem getting datacore\n");
				return status;
			}

			new (&sDeviceList) DoublyLinkedList<bluetooth_device>;
				// static C++ objects are not initialized in the module startup

			BluetoothRXPort = new BluetoothRawDataPort(BT_RX_PORT_NAME,
				(BluetoothRawDataPort::port_listener_func)&HciPacketHandler);

			if (BluetoothRXPort->Launch() != B_OK) {
				flowf("RX thread creation failed!\n");
				// we Cannot do much here ... avoid registering
			} else {
				flowf("RX thread launched!\n");
			}

			sLinkChangeSemaphore = create_sem(0, "bt sem");
			if (sLinkChangeSemaphore < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				flowf("sem failed\n");
				return sLinkChangeSemaphore;
			}

			mutex_init(&sListLock, "bluetooth devices");

			// status = InitializeAclConnectionThread();
			debugf("Connection Thread error=%lx\n", status);

			add_debugger_command("btLocalDevices", &dump_bluetooth_devices,
				"Lists Bluetooth LocalDevices registered in the Stack");

			return B_OK;
		}

		case B_MODULE_UNINIT:
		{
			delete_sem(sLinkChangeSemaphore);

			mutex_destroy(&sListLock);
			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			put_module(BT_CORE_DATA_MODULE_NAME);
			remove_debugger_command("btLocalDevices", &dump_bluetooth_devices);
			// status_t status = QuitAclConnectionThread();

			return B_OK;
		}

		default:
			return B_ERROR;
	}
}


bt_hci_module_info sBluetoothModule = {
	{
		BT_HCI_MODULE_NAME,
		B_KEEP_LOADED,
		bluetooth_std_ops
	},
	RegisterDriver,
	UnregisterDriver,
	FindDeviceByID,
	PostTransportPacket,
	PostACL,
	PostSCO,
	PostESCO
};


module_info* modules[] = {
	(module_info*)&sBluetoothModule,
	NULL
};

