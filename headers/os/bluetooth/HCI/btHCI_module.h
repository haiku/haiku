/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _BTHCI_MODULE_H_
#define _BTHCI_MODULE_H_

/* includes */

#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <net/net_buffer.h>

/* defines */
#define	BT_HCI_MODULE_NAME	"bus_managers/hci/v1"

/* TODO: Possible definition of a bus manager of whatever is gonna be on the top */
typedef struct bt_hci_module_info {
   
    /* registration */
	status_t        (*RegisterDriver)(bt_hci_transport* desc, hci_id *id /*out*/, 
	                                  bt_hci_device* cookie /*out*/ );
    status_t        (*UnregisterDriver)(hci_id id);
    
    /* Transferences to be called from drivers */

    status_t		(*PostACL)(hci_id id, net_buffer* nbuf);
    status_t		(*PostSCO)(hci_id id, net_buffer* nbuf);
    
    /* Management */
    bt_hci_device*	(*GetHciDevice)(hci_id id);

} bt_hci_module_info ;


#endif