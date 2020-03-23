/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _H2CFG_H_
#define _H2CFG_H_


#define BT_DRIVER_SUPPORTS_CMD  1
#define BT_DRIVER_SUPPORTS_EVT  1
#define BT_DRIVER_SUPPORTS_ACL  1
#define BT_DRIVER_SUPPORTS_SCO  1
#define BT_DRIVER_SUPPORTS_ESCO 0


// TODO: move exclusive header for drivers
#define BT_DRIVER_RXCOVERAGE (BT_DRIVER_SUPPORTS_EVT + BT_DRIVER_SUPPORTS_ACL \
	+ BT_DRIVER_SUPPORTS_SCO + BT_DRIVER_SUPPORTS_ESCO)
#define BT_DRIVER_TXCOVERAGE (BT_DRIVER_SUPPORTS_CMD + BT_DRIVER_SUPPORTS_ACL \
	+ BT_DRIVER_SUPPORTS_SCO + BT_DRIVER_SUPPORTS_ESCO)

#if BT_DRIVER_RXCOVERAGE < 1 || BT_DRIVER_TXCOVERAGE < 1
#error incomplete Bluetooth driver Commands and Events should be implemented
#endif

#define BT_SURVIVE_WITHOUT_HCI
//#define BT_SURVIVE_WITHOUT_NET_BUFFERS

#ifndef BLUETOOTH_DEVICE_TRANSPORT
#error BLUETOOTH_DEVICE_TRANSPORT must be defined to build the publishing path
#endif

#ifndef BLUETOOTH_DEVICE_NAME
#error BLUETOOTH_DEVICE_NAME must be defined to build the publishing path
#endif

#define BLUETOOTH_DEVICE_DEVFS_NAME BLUETOOTH_DEVICE_TRANSPORT \
	BLUETOOTH_DEVICE_NAME
#define BLUETOOTH_DEVICE_PATH "bluetooth/" BLUETOOTH_DEVICE_TRANSPORT "/" \
	BLUETOOTH_DEVICE_DEVFS_NAME


#endif

