/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _H2GENERIC_H_
#define _H2GENERIC_H_

#include <net_buffer.h>
#include <net_device.h>

#include <OS.h>
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#include <USB3.h>
#else
#include <USB_spec.h>
#include <USB.h>
#endif

#include <util/list.h>
#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_transport.h>

#include <btCoreData.h>

#include "snet_buffer.h"

// USB definitions for the generic device move to h2cfg
#define UDCLASS_WIRELESS	0xe0
#define UDSUBCLASS_RF		0x01
#define UDPROTO_BLUETOOTH	0x01

#define BLUETOOTH_DEVICE_TRANSPORT	"h2"
#define BLUETOOTH_DEVICE_NAME "generic"
#include "h2cfg.h"

#define USB_TYPE_CLASS			(0x01 << 5)  /// Check if it is in some other header
#define USB_TYPE_VENDOR			(0x02 << 5)


// Expecting nobody is gonna have 16 USB-BT dongles connected in their system
#define MAX_BT_GENERIC_USB_DEVICES	16

extern usb_module_info* usb;
extern bt_hci_module_info* hci;
extern struct bt_hci_module_info* btDevices;
extern struct net_buffer_module_info* nb;
extern struct bluetooth_core_data_module_info* btCoreData;

#define MAX_COMMAND_WINDOW 1
#define MAX_ACL_OUT_WINDOW 4
#define MAX_ACL_IN_WINDOW  1

#define MAX_NUM_QUEUED_PACKETS 1
#define NUM_BUFFERS 1

typedef struct bt_usb_dev bt_usb_dev;

struct bt_usb_dev {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	usb_device		dev;          /* opaque handle */
#else
	usb_device*		dev;          /* opaque handle */
#endif
	hci_id					hdev; /* HCI device id*/
	bluetooth_device*		ndev;

	char			name[B_OS_NAME_LENGTH];
	bool			connected;    /* is the device plugged into the USB? */
	int32			open_count;   /* number of clients of the device */
	int32			num;          /* instance number of the device */

	sem_id			lock;         /* synchronize access to the device */
	sem_id			cmd_complete; /* To synchronize completitions */

	size_t			actual_len;   /* length of data returned by command */
	status_t		cmd_status;   /* result of command */

	uint8				ctrl_req;
	uint8				driver_info;
	uint32				state;

	bt_hci_statistics	stat;

	const	usb_endpoint_info*	bulk_in_ep;
			uint16				max_packet_size_bulk_in;
	const	usb_endpoint_info*	bulk_out_ep;
			uint16				max_packet_size_bulk_out;
	const	usb_endpoint_info*	intr_in_ep;
			uint16				max_packet_size_intr_in;

#ifdef BLUETOOTH_SUPPORTS_SCO
	const usb_endpoint_info	*iso_in_ep;
	const usb_endpoint_info	*iso_out_ep;
#endif

	/* This so called rooms, are for dumping the USB RX frames
	 * and try to reuse the allocations. see util submodule
	 */
	struct list eventRoom;
	struct list aclRoom;

	// Tx buffers: net_buffers for BT_ACL and snet_buffers for BT_COMMAND
	// in the same array
	struct list	nbuffersTx[BT_DRIVER_TXCOVERAGE];
	uint32		nbuffersPendingTx[BT_DRIVER_TXCOVERAGE];

	// Rx buffer
	net_buffer*		nbufferRx[BT_DRIVER_RXCOVERAGE];
	snet_buffer*	eventRx;

	// for who ever needs preallocated buffers
	struct list		snetBufferRecycleTrash;

};

bt_usb_dev* fetch_device(bt_usb_dev* dev, hci_id hid);

#endif
