/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _H2CFG_H_
#define _H2CFG_H_

/* TODO: move exclusive header for drivers*/
#define BT_DRIVER_SUPPORTS_CMD  1
#define BT_DRIVER_SUPPORTS_EVT  1
#define BT_DRIVER_SUPPORTS_ESCO 0
#define BT_DRIVER_SUPPORTS_SCO  0
#define BT_DRIVER_SUPPORTS_ACL  0

#define BT_DRIVER_RXCOVERAGE (BT_DRIVER_SUPPORTS_EVT+BT_DRIVER_SUPPORTS_ACL+BT_DRIVER_SUPPORTS_SCO+BT_DRIVER_SUPPORTS_ESCO)
#define BT_DRIVER_TXCOVERAGE (BT_DRIVER_SUPPORTS_CMD+BT_DRIVER_SUPPORTS_ACL+BT_DRIVER_SUPPORTS_SCO+BT_DRIVER_SUPPORTS_ESCO)

#if BT_DRIVER_RXCOVERAGE<1 || BT_DRIVER_TXCOVERAGE<1
#error incomplete Bluetooth driver Commands and Events should be implemented
#endif


////////////////////////////////////

#define BT_SURVIVE_WITHOUT_HCI
#define BT_SURVIVE_WITHOUT_NET_BUFFERS


#endif
