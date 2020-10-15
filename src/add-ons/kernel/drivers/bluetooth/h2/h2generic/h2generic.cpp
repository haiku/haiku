/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "h2generic.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <KernelExport.h>
#include <ByteOrder.h>
#include <Drivers.h>

#include <btModules.h>

#include "snet_buffer.h"
#include "h2cfg.h"
#include "h2debug.h"
#include "h2transactions.h"
#include "h2util.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

// Modules
static const char* usb_name = B_USB_MODULE_NAME;
static const char* hci_name = BT_HCI_MODULE_NAME;
static const char* btDevices_name = BT_HCI_MODULE_NAME;


usb_module_info* usb = NULL;
bt_hci_module_info* hci = NULL; // TODO remove / clean
struct bt_hci_module_info* btDevices = NULL;
struct net_buffer_module_info* nb = NULL;
struct bluetooth_core_data_module_info* btCoreData = NULL;

// Driver Global data
static char* publish_names[MAX_BT_GENERIC_USB_DEVICES];

int32 dev_count = 0; // number of connected devices
static bt_usb_dev* bt_usb_devices[MAX_BT_GENERIC_USB_DEVICES];
sem_id dev_table_sem = -1; // sem to synchronize access to device table

status_t submit_nbuffer(hci_id hid, net_buffer* nbuf);

usb_support_descriptor supported_devices[] = {
	// Generic Bluetooth USB device
	// Class, SubClass, and Protocol codes that describe a Bluetooth device
	{ UDCLASS_WIRELESS, UDSUBCLASS_RF, UDPROTO_BLUETOOTH, 0, 0 },

	// Broadcom BCM2035
	{ 0, 0, 0, 0x0a5c, 0x200a },
	{ 0, 0, 0, 0x0a5c, 0x2009 },

	// Devices taken from the linux Driver
	// MediaTek MT76x0E
	{ 0, 0, 0, 0x0e8d, 0x763f },
	// Broadcom SoftSailing reporting vendor specific
	{ 0, 0, 0, 0x0a5c, 0x21e1 },

	// Apple MacBookPro 7,1
	{ 0, 0, 0, 0x05ac, 0x8213 },
	// Apple iMac11,1
	{ 0, 0, 0, 0x05ac, 0x8215 },
	// Apple MacBookPro6,2
	{ 0, 0, 0, 0x05ac, 0x8218 },
	// Apple MacBookAir3,1, MacBookAir3,2
	{ 0, 0, 0, 0x05ac, 0x821b },
	// Apple MacBookAir4,1
	{ 0, 0, 0, 0x05ac, 0x821f },
	// Apple MacBookPro8,2
	{ 0, 0, 0, 0x05ac, 0x821a },
	// Apple MacMini5,1
	{ 0, 0, 0, 0x05ac, 0x8281 },

	// AVM BlueFRITZ! USB v2.0
	{ 0, 0, 0, 0x057c, 0x3800 },
	// Bluetooth Ultraport Module from IBM
	{ 0, 0, 0, 0x04bf, 0x030a },
	// ALPS Modules with non-standard id
	{ 0, 0, 0, 0x044e, 0x3001 },
	{ 0, 0, 0, 0x044e, 0x3002 },
	// Ericsson with non-standard id
	{ 0, 0, 0, 0x0bdb, 0x1002 },

	// Canyon CN-BTU1 with HID interfaces
	{ 0, 0, 0, 0x0c10, 0x0000 },

	// Broadcom BCM20702A0
	{ 0, 0, 0, 0x413c, 0x8197 },

};

/* add a device to the list of connected devices */
static bt_usb_dev*
spawn_device(usb_device usb_dev)
{
	CALLED();

	int32 i;
	status_t err = B_OK;
	bt_usb_dev* new_bt_dev = NULL;

	// 16 usb dongles...
	if (dev_count >= MAX_BT_GENERIC_USB_DEVICES) {
		ERROR("%s: Device table full\n", __func__);
		goto exit;
	}

	// try the allocation
	new_bt_dev = (bt_usb_dev*)malloc(sizeof(bt_usb_dev));
	if (new_bt_dev == NULL) {
		ERROR("%s: Unable to malloc new bt device\n", __func__);
		goto exit;
	}
	memset(new_bt_dev, 0, sizeof(bt_usb_dev));

	// We will need this sem for some flow control
	new_bt_dev->cmd_complete = create_sem(1,
		BLUETOOTH_DEVICE_DEVFS_NAME "cmd_complete");
	if (new_bt_dev->cmd_complete < 0) {
		err = new_bt_dev->cmd_complete;
		goto bail0;
	}

	// and this for something else
	new_bt_dev->lock = create_sem(1, BLUETOOTH_DEVICE_DEVFS_NAME "lock");
	if (new_bt_dev->lock < 0) {
		err = new_bt_dev->lock;
		goto bail1;
	}

	// find a free slot and fill out the name
	acquire_sem(dev_table_sem);
	for (i = 0; i < MAX_BT_GENERIC_USB_DEVICES; i++) {
		if (bt_usb_devices[i] == NULL) {
			bt_usb_devices[i] = new_bt_dev;
			sprintf(new_bt_dev->name, "%s/%" B_PRId32,
				BLUETOOTH_DEVICE_PATH, i);
			new_bt_dev->num = i;
			TRACE("%s: added device %p %" B_PRId32 " %s\n", __func__,
				bt_usb_devices[i], new_bt_dev->num, new_bt_dev->name);
			break;
		}
	}
	release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);

	// In the case we cannot find a free slot
	if (i >= MAX_BT_GENERIC_USB_DEVICES) {
		ERROR("%s: Device could not be added\n", __func__);
		goto bail2;
	}

	new_bt_dev->dev = usb_dev;
	// TODO: currently only server opens
	new_bt_dev->open_count = 0;

	dev_count++;
	return new_bt_dev;

bail2:
	delete_sem(new_bt_dev->lock);
bail1:
	delete_sem(new_bt_dev->cmd_complete);
bail0:
	free(new_bt_dev);
	new_bt_dev = NULL;
exit:
	return new_bt_dev;
}


// remove a device from the list of connected devices
static void
kill_device(bt_usb_dev* bdev)
{
	if (bdev != NULL) {
		TRACE("%s: (%p)\n", __func__, bdev);

		delete_sem(bdev->lock);
		delete_sem(bdev->cmd_complete);

		// mark it free
		bt_usb_devices[bdev->num] = NULL;

		free(bdev);
		dev_count--;
	}
}


bt_usb_dev*
fetch_device(bt_usb_dev* dev, hci_id hid)
{
	int i;

//	TRACE("%s: (%p) or %d\n", __func__, dev, hid);

	acquire_sem(dev_table_sem);
	if (dev != NULL) {
		for (i = 0; i < MAX_BT_GENERIC_USB_DEVICES; i++) {
			if (bt_usb_devices[i] == dev) {
				release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);
				return bt_usb_devices[i];
			}
		}
	} else {
		for (i = 0; i < MAX_BT_GENERIC_USB_DEVICES; i++) {
			if (bt_usb_devices[i] != NULL && bt_usb_devices[i]->hdev == hid) {
				release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);
				return bt_usb_devices[i];
			}
		}
	}

	release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);

	return NULL;
}


#if 0
#pragma mark -
#endif

// called by USB Manager when device is added to the USB
static status_t
device_added(usb_device dev, void** cookie)
{
	const usb_interface_info* 		interface;
	const usb_device_descriptor* 	desc;
	const usb_configuration_info*	config;
	const usb_interface_info*		uif;
	const usb_endpoint_info*		ep;

	status_t 	err = B_ERROR;
	bt_usb_dev* new_bt_dev = spawn_device(dev);
	int e;

	TRACE("%s: device_added(%p)\n", __func__, new_bt_dev);

	if (new_bt_dev == NULL) {
		ERROR("%s: Couldn't allocate device record.\n", __func__);
		err = ENOMEM;
		goto bail_no_mem;
	}

	// we only have 1 configuration number 0
	config = usb->get_nth_configuration(dev, 0);
	// dump_usb_configuration_info(config);
	if (config == NULL) {
		ERROR("%s: Couldn't get default USB config.\n", __func__);
		err = B_ERROR;
		goto bail;
	}

	TRACE("%s: found %" B_PRIuSIZE " alt interfaces.\n", __func__,
		config->interface->alt_count);

	// set first interface
	interface = &config->interface->alt[0];
	err = usb->set_alt_interface(new_bt_dev->dev, interface);

	if (err != B_OK) {
		ERROR("%s: set_alt_interface() error.\n", __func__);
		goto bail;
	}

	// call set_configuration() only after calling set_alt_interface()
	err = usb->set_configuration(dev, config);
	if (err != B_OK) {
		ERROR("%s: set_configuration() error.\n", __func__);
		goto bail;
	}

	// Place to find out whats our concrete device and set up some special
	// info to our driver. If this code increases too much reconsider
	// this implementation
	desc = usb->get_device_descriptor(dev);
	if (desc->vendor_id == 0x0a5c
		&& (desc->product_id == 0x200a
			|| desc->product_id == 0x2009
			|| desc->product_id == 0x2035)) {

		new_bt_dev->driver_info = BT_WILL_NEED_A_RESET | BT_SCO_NOT_WORKING;

	}
	/*
	else if ( desc->vendor_id == YOUR_VENDOR_HERE
		&& desc->product_id == YOUR_PRODUCT_HERE ) {
		YOUR_SPECIAL_FLAGS_HERE
	}
	*/

	if (new_bt_dev->driver_info & BT_IGNORE_THIS_DEVICE) {
		err = ENODEV;
		goto bail;
	}

	// security check
	if (config->interface->active->descr->interface_number > 0) {
		ERROR("%s: Strange condition happened %d\n", __func__,
			config->interface->active->descr->interface_number);
		err = B_ERROR;
		goto bail;
	}

	TRACE("%s: Found %" B_PRIuSIZE " interfaces. Expected 3\n", __func__,
		config->interface_count);

	// Find endpoints that we need
	uif = config->interface->active;
	for (e = 0; e < uif->descr->num_endpoints; e++) {

		ep = &uif->endpoint[e];
		switch (ep->descr->attributes & USB_ENDPOINT_ATTR_MASK) {
			case USB_ENDPOINT_ATTR_INTERRUPT:
				if (ep->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				{
					new_bt_dev->intr_in_ep = ep;
					new_bt_dev->max_packet_size_intr_in
						= ep->descr->max_packet_size;
					TRACE("%s: INT in\n", __func__);
				} else {
					TRACE("%s: INT out\n", __func__);
				}
			break;

			case USB_ENDPOINT_ATTR_BULK:
				if (ep->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)	{
					new_bt_dev->bulk_in_ep  = ep;
					new_bt_dev->max_packet_size_bulk_in
						= ep->descr->max_packet_size;
					TRACE("%s: BULK int\n", __func__);
				} else	{
					new_bt_dev->bulk_out_ep = ep;
					new_bt_dev->max_packet_size_bulk_out
						= ep->descr->max_packet_size;
					TRACE("%s: BULK out\n", __func__);
				}
			break;
		}
	}

	if (!new_bt_dev->bulk_in_ep || !new_bt_dev->bulk_out_ep
		|| !new_bt_dev->intr_in_ep) {
		ERROR("%s: Minimal # endpoints for BT not found\n", __func__);
		goto bail;
	}

	// Look into the devices suported to understand this
	if (new_bt_dev->driver_info & BT_DIGIANSWER)
		new_bt_dev->ctrl_req = USB_TYPE_VENDOR;
	else
		new_bt_dev->ctrl_req = USB_TYPE_CLASS;

	new_bt_dev->connected = true;

	// set the cookie that will be passed to other USB
	// hook functions (currently device_removed() is the only other)
	*cookie = new_bt_dev;
	TRACE("%s: Ok %p\n", __func__, new_bt_dev);
	return B_OK;

bail:
	kill_device(new_bt_dev);
bail_no_mem:
	*cookie = NULL;

	return err;
}


// Called by USB Manager when device is removed from the USB
static status_t
device_removed(void* cookie)
{
	bt_usb_dev* bdev = fetch_device((bt_usb_dev*)cookie, 0);

	TRACE("%s: device_removed(%p)\n", __func__, bdev);

	if (bdev == NULL) {
		ERROR("%s: Device not present in driver.\n", __func__);
		return B_ERROR;
	}

	if (!TEST_AND_CLEAR(&bdev->state, RUNNING))
		ERROR("%s: wasnt running?\n", __func__);

	TRACE("%s: Cancelling queues...\n", __func__);
	if (bdev->intr_in_ep != NULL)
		usb->cancel_queued_transfers(bdev->intr_in_ep->handle);
	if (bdev->bulk_in_ep != NULL)
		usb->cancel_queued_transfers(bdev->bulk_in_ep->handle);
	if (bdev->bulk_out_ep != NULL)
		usb->cancel_queued_transfers(bdev->bulk_out_ep->handle);

	bdev->connected = false;

	return B_OK;
}


static bt_hci_transport_hooks bluetooth_hooks = {
	NULL,
	&submit_nbuffer,
	&submit_nbuffer,
	NULL,
	NULL,
	H2
};


static usb_notify_hooks notify_hooks = {
	&device_added,
	&device_removed
};

#if 0
#pragma mark -
#endif

status_t
submit_nbuffer(hci_id hid, net_buffer* nbuf)
{
	bt_usb_dev* bdev = NULL;

	bdev = fetch_device(NULL, hid);

	TRACE("%s: index=%" B_PRId32 " nbuf=%p bdev=%p\n", __func__, hid,
		nbuf, bdev);

	if (bdev != NULL) {
		switch (nbuf->protocol) {
			case BT_COMMAND:
				// not issued this way
			break;

			case BT_ACL:
				return submit_tx_acl(bdev, nbuf);
			break;

			default:
				panic("submit_nbuffer: no protocol");
			break;

		}
	}

	return B_ERROR;

}


// implements the POSIX open()
static status_t
device_open(const char* name, uint32 flags, void **cookie)
{
	CALLED();

	status_t err = ENODEV;
	bt_usb_dev* bdev = NULL;
	hci_id hdev;
	int i;

	acquire_sem(dev_table_sem);
	for (i = 0; i < MAX_BT_GENERIC_USB_DEVICES; i++) {
		if (bt_usb_devices[i] && !strcmp(name, bt_usb_devices[i]->name)) {
			bdev = bt_usb_devices[i];
			break;
		}
	}
	release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);

	if (bdev == NULL) {
		ERROR("%s: Device not found in the open list!", __func__);
		*cookie = NULL;
		return B_ERROR;
	}

	// Set RUNNING
	if (TEST_AND_SET(&bdev->state, RUNNING)) {
		ERROR("%s: dev already running! - reOpened device!\n", __func__);
		return B_ERROR;
	}

	acquire_sem(bdev->lock);
	// TX structures
	for (i = 0; i < BT_DRIVER_TXCOVERAGE; i++) {
		list_init(&bdev->nbuffersTx[i]);
		bdev->nbuffersPendingTx[i] = 0;
	}

	// RX structures
	bdev->eventRx = NULL;
	for (i = 0; i < BT_DRIVER_RXCOVERAGE; i++) {
		bdev->nbufferRx[i] = NULL;
	}

	// dumping the USB frames
	init_room(&bdev->eventRoom);
	init_room(&bdev->aclRoom);
	// init_room(new_bt_dev->scoRoom);

	list_init(&bdev->snetBufferRecycleTrash);

	// Allocate set and register the HCI device
	if (btDevices != NULL) {
		bluetooth_device* ndev;
		// TODO: Fill the transport descriptor
		err = btDevices->RegisterDriver(&bluetooth_hooks, &ndev);

		if (err == B_OK) {
			bdev->hdev = hdev = ndev->index; // Get the index
			bdev->ndev = ndev;  // Get the net_device

		} else {
			hdev = bdev->num; // XXX: Lets try to go on
		}
	} else {
		hdev = bdev->num; // XXX: Lets try to go on
	}

	bdev->hdev = hdev;

	*cookie = bdev;
	release_sem(bdev->lock);

	return B_OK;

}


/* called when a client calls POSIX close() on the driver, but I/O
 * requests may still be pending
 */
static status_t
device_close(void* cookie)
{
	CALLED();

	int32 i;
	void* item;
	bt_usb_dev* bdev = (bt_usb_dev*)cookie;

	if (bdev == NULL)
		panic("bad cookie");

	// Clean queues

	if (bdev->connected == true) {
		TRACE("%s: Cancelling queues...\n", __func__);

		if (bdev->intr_in_ep != NULL)
			usb->cancel_queued_transfers(bdev->intr_in_ep->handle);

		if (bdev->bulk_in_ep!=NULL)
			usb->cancel_queued_transfers(bdev->bulk_in_ep->handle);

		if (bdev->bulk_out_ep!=NULL)
			usb->cancel_queued_transfers(bdev->bulk_out_ep->handle);
	}

	// TX
	for (i = 0; i < BT_DRIVER_TXCOVERAGE; i++) {
		if (i == BT_COMMAND) {
			while ((item = list_remove_head_item(&bdev->nbuffersTx[i])) != NULL)
				snb_free((snet_buffer*)item);
		} else {
			while ((item = list_remove_head_item(&bdev->nbuffersTx[i])) != NULL)
				nb_destroy((net_buffer*)item);
		}
	}
	// RX
	for (i = 0; i < BT_DRIVER_RXCOVERAGE; i++) {
		nb_destroy(bdev->nbufferRx[i]);
	}
	snb_free(bdev->eventRx);

	purge_room(&bdev->eventRoom);
	purge_room(&bdev->aclRoom);

	// Device no longer in our Stack
	if (btDevices != NULL)
		btDevices->UnregisterDriver(bdev->hdev);

	// unSet RUNNING
	if (TEST_AND_CLEAR(&bdev->state, RUNNING)) {
		ERROR("%s: %s not running?\n", __func__, bdev->name);
		return B_ERROR;
	}

	return B_OK;
}


// Called after device_close(), when all pending I / O requests have returned
static status_t
device_free(void* cookie)
{
	CALLED();

	status_t err = B_OK;
	bt_usb_dev* bdev = (bt_usb_dev*)cookie;

	if (!bdev->connected)
		kill_device(bdev);

	return err;
}


// implements the POSIX ioctl()
static status_t
device_control(void* cookie, uint32 msg, void* params, size_t size)
{
	status_t 	err = B_ERROR;
	bt_usb_dev*	bdev = (bt_usb_dev*)cookie;
	snet_buffer* snbuf;
	#if BT_DRIVER_SUPPORTS_ACL // ACL
	int32	i;
	#endif

	TOUCH(size);
	TRACE("%s: ioctl() opcode %" B_PRId32 " size %" B_PRIuSIZE ".\n", __func__,
		msg, size);

	if (bdev == NULL) {
		TRACE("%s: Bad cookie\n", __func__);
		return B_BAD_VALUE;
	}

	if (params == NULL || !IS_USER_ADDRESS(params)) {
		TRACE("%s: Invalid pointer control\n", __func__);
		return B_BAD_VALUE;
	}

	acquire_sem(bdev->lock);

	switch (msg) {
		case ISSUE_BT_COMMAND: {
			if (size == 0) {
				TRACE("%s: Invalid size control\n", __func__);
				err = B_BAD_VALUE;
				break;
			}

			void* _params = alloca(size);
			if (user_memcpy(_params, params, size) != B_OK)
				return B_BAD_ADDRESS;

			// TODO: Reuse from some TXcompleted queue
			// snbuf = snb_create(size);
			snbuf = snb_fetch(&bdev->snetBufferRecycleTrash, size);
			snb_put(snbuf, _params, size);

			err = submit_tx_command(bdev, snbuf);
			TRACE("%s: command launched\n", __func__);
			break;
		}

		case BT_UP:
			//  EVENTS
			err = submit_rx_event(bdev);
			if (err != B_OK) {
				bdev->state = CLEAR_BIT(bdev->state, ANCILLYANT);
				ERROR("%s: Queuing failed device stops running\n", __func__);
				break;
			}

			#if BT_DRIVER_SUPPORTS_ACL // ACL
			for (i = 0; i < MAX_ACL_IN_WINDOW; i++) {
				err = submit_rx_acl(bdev);
				if (err != B_OK && i == 0) {
					bdev->state = CLEAR_BIT(bdev->state, ANCILLYANT);
						// Set the flaq in the HCI world
					ERROR("%s: Queuing failed device stops running\n",
						__func__);
					break;
				}
			}
			#endif

			bdev->state = SET_BIT(bdev->state, RUNNING);

			#if BT_DRIVER_SUPPORTS_SCO
				// TODO:  SCO / eSCO
			#endif

			ERROR("%s: Device online\n", __func__);
		break;

		case GET_STATS:
			err = user_memcpy(params, &bdev->stat, sizeof(bt_hci_statistics));
		break;

		case GET_HCI_ID:
			err = user_memcpy(params, &bdev->hdev, sizeof(hci_id));
		break;


	default:
		ERROR("%s: Invalid opcode.\n", __func__);
		err = B_DEV_INVALID_IOCTL;
		break;
	}

	release_sem(bdev->lock);
	return err;
}


// implements the POSIX read()
static status_t
device_read(void* cookie, off_t pos, void* buffer, size_t* count)
{
	TRACE("%s: Reading... count = %" B_PRIuSIZE "\n", __func__, *count);

	*count = 0;
	return B_OK;
}


// implements the POSIX write()
static status_t
device_write(void* cookie, off_t pos, const void* buffer, size_t* count)
{
	CALLED();

	return B_ERROR;
}


#if 0
#pragma mark -
#endif


static int
dump_driver(int argc, char** argv)
{
	int i;
	snet_buffer* item = NULL;

	for (i = 0; i < MAX_BT_GENERIC_USB_DEVICES; i++) {

		if (bt_usb_devices[i] != NULL) {
			kprintf("%s : \n", bt_usb_devices[i]->name);
			kprintf("\taclroom = %d\teventroom = %d\tcommand & events =%d\n",
				snb_packets(&bt_usb_devices[i]->eventRoom),
				snb_packets(&bt_usb_devices[i]->aclRoom),
				snb_packets(&bt_usb_devices[i]->snetBufferRecycleTrash));

			while ((item = (snet_buffer*)list_get_next_item(
				&bt_usb_devices[i]->snetBufferRecycleTrash, item)) != NULL)
				snb_dump(item);
		}
	}

	return 0;
}


// called each time the driver is loaded by the kernel
status_t
init_driver(void)
{
	CALLED();
	int j;

	if (get_module(BT_CORE_DATA_MODULE_NAME,
		(module_info**)&btCoreData) != B_OK) {
		ERROR("%s: cannot get module '%s'\n", __func__,
			BT_CORE_DATA_MODULE_NAME);
		return B_ERROR;
	}

	// BT devices MODULE INITS
	if (get_module(btDevices_name, (module_info**)&btDevices) != B_OK) {
		ERROR("%s: cannot get module '%s'\n", __func__, btDevices_name);
		goto err_release3;
	}

	// HCI MODULE INITS
	if (get_module(hci_name, (module_info**)&hci) != B_OK) {
		ERROR("%s: cannot get module '%s'\n", __func__, hci_name);
#ifndef BT_SURVIVE_WITHOUT_HCI
		goto err_release2;
#endif
	}

	// USB MODULE INITS
	if (get_module(usb_name, (module_info**)&usb) != B_OK) {
		ERROR("%s: cannot get module '%s'\n", __func__, usb_name);
		goto err_release1;
	}

	if (get_module(NET_BUFFER_MODULE_NAME, (module_info**)&nb) != B_OK) {
		ERROR("%s: cannot get module '%s'\n", __func__,
			NET_BUFFER_MODULE_NAME);
#ifndef BT_SURVIVE_WITHOUT_NET_BUFFERS
		goto err_release;
#endif
	}

	// GENERAL INITS
	dev_table_sem = create_sem(1, BLUETOOTH_DEVICE_DEVFS_NAME "dev_table_lock");
	if (dev_table_sem < 0) {
		goto err;
	}

	for (j = 0; j < MAX_BT_GENERIC_USB_DEVICES; j++) {
		bt_usb_devices[j] = NULL;
	}

	// Note: After here device_added and publish devices hooks are called
	usb->register_driver(BLUETOOTH_DEVICE_DEVFS_NAME, supported_devices, 1, NULL);
	usb->install_notify(BLUETOOTH_DEVICE_DEVFS_NAME, &notify_hooks);

	add_debugger_command("bth2generic", &dump_driver,
		"Lists H2 Transport device info");

	return B_OK;

err:	// Releasing
	put_module(NET_BUFFER_MODULE_NAME);
err_release:
	put_module(usb_name);
err_release1:
	put_module(hci_name);
#ifndef BT_SURVIVE_WITHOUT_HCI
err_release2:
#endif
	put_module(btDevices_name);
err_release3:
	put_module(BT_CORE_DATA_MODULE_NAME);

	return B_ERROR;
}


// called just before the kernel unloads the driver
void
uninit_driver(void)
{
	CALLED();

	int32 j;

	for (j = 0; j < MAX_BT_GENERIC_USB_DEVICES; j++) {

		if (publish_names[j] != NULL)
			free(publish_names[j]);

		if (bt_usb_devices[j] != NULL) {
			//	if (connected_dev != NULL) {
			//		debugf("Device %p still exists.\n",	connected_dev);
			//	}
			ERROR("%s: %s still present?\n", __func__, bt_usb_devices[j]->name);
			kill_device(bt_usb_devices[j]);
		}
	}

	usb->uninstall_notify(BLUETOOTH_DEVICE_DEVFS_NAME);

	remove_debugger_command("bth2generic", &dump_driver);

	// Releasing modules
	put_module(usb_name);
	put_module(hci_name);
	// TODO: netbuffers

	delete_sem(dev_table_sem);
}


const char**
publish_devices(void)
{
	CALLED();
	int32 j;
	int32 i = 0;

	char* str;

	for (j = 0; j < MAX_BT_GENERIC_USB_DEVICES; j++) {
		if (publish_names[j]) {
			free(publish_names[j]);
			publish_names[j] = NULL;
		}
	}

	acquire_sem(dev_table_sem);
	for (j = 0; j < MAX_BT_GENERIC_USB_DEVICES; j++) {
		if (bt_usb_devices[j] != NULL && bt_usb_devices[j]->connected) {
			str = strdup(bt_usb_devices[j]->name);
			if (str) {
				publish_names[i++] = str;
				TRACE("%s: publishing %s\n", __func__, bt_usb_devices[j]->name);
			}
		}
	}
	release_sem_etc(dev_table_sem, 1, B_DO_NOT_RESCHEDULE);

	publish_names[i] = NULL;
	TRACE("%s: published %" B_PRId32 " devices\n", __func__, i);

	// TODO: this method might make better memory use
	// dev_names = (char**)malloc(sizeof(char*) * (dev_count + 1));
	// if (dev_names) {
	// for (i = 0; i < MAX_NUM_DEVS; i++) {
	//	if ((dev != NULL) // dev + \n
	//	&& (dev_names[i] = (char*)malloc(strlen(DEVICE_PATH) + 2))) {
	//	sprintf(dev_names[i], "%s%ld", DEVICE_PATH, dev->num);
	//	debugf("publishing \"%s\"\n", dev_names[i]);
	//	}
	// }

	return (const char**)publish_names;
}


static device_hooks hooks = {
	device_open,
	device_close,
	device_free,
	device_control,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL
};


device_hooks*
find_device(const char* name)
{
	CALLED();

	return &hooks;
}
