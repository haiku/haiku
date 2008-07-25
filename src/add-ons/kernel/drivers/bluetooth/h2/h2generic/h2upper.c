/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI_transport.h>

#include "h2generic.h"
#include "h2upper.h"
#include "h2transactions.h"
#include "snet_buffer.h"

#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


/* TODO: split for commands and comunication(ACL&SCO) */
void 
sched_tx_processing(bt_usb_dev* bdev)
{
    
    net_buffer* nbuf;
    snet_buffer* snbuf;
	status_t 	err;
	flowf("sched\n")    
    if (!TEST_AND_SET(&bdev->state, PROCESSING)) {
        /* We are not processing in another thread so... START!! */
                
        do {
            /* Do while this bit is on... so someone should set it before we stop the iterations*/
            CLEAR_BIT(bdev->state, SENDING);
            /* check Commands*/
    #ifdef EMPTY_COMMAND_QUEUE        
            while (!list_is_empty(&bdev->nbuffersTx[BT_COMMAND]) ) {            
    #else
            if (!list_is_empty(&bdev->nbuffersTx[BT_COMMAND]) ) {
    #endif            
                snbuf = list_remove_head_item(&bdev->nbuffersTx[BT_COMMAND]);       
                err = submit_tx_command(bdev, snbuf);
                if (err != B_OK) {
                    /* re-head it*/
                    list_insert_item_before(&bdev->nbuffersTx[BT_COMMAND], 
                                            list_get_first_item(&bdev->nbuffersTx[BT_COMMAND]), snbuf);
                }                                                
            }
            
            /* check Acl */
    #define EMPTY_ACL_QUEUE
    #ifdef EMPTY_ACL_QUEUE
            while (!list_is_empty(&bdev->nbuffersTx[BT_ACL]) ) {
    #else            
            if (!list_is_empty(&bdev->nbuffersTx[BT_ACL]) ) {
    #endif             
                nbuf = list_remove_head_item(&bdev->nbuffersTx[BT_ACL]);                
                err = submit_tx_acl(bdev, nbuf);
                if (err != B_OK) {
                    /* re-head it*/
                    list_insert_item_before(&bdev->nbuffersTx[BT_ACL], 
                                            list_get_first_item(&bdev->nbuffersTx[BT_ACL]), nbuf);
                }                                                                
                
            }
    
            if (!list_is_empty(&bdev->nbuffersTx[BT_SCO]) ) {
                /* TODO to be implemented */    
                
            }
        
        
        } while (GET_BIT(bdev->state, SENDING));
        
        CLEAR_BIT(bdev->state, PROCESSING);

    } else {        
        /* We are processing so MARK that we need to still go on with that ... */    
        SET_BIT(bdev->state, SENDING);
    }

}

status_t 
post_packet_up(bt_usb_dev* bdev, bt_packet_t type, void* buf)
{
    
    status_t err = B_OK;
    port_id port;

	debugf("Frame up type=%d\n", type);
            
    if (hci == NULL) {
		
        err = B_ERROR;

	    // ERROR but we will try to send if its a event!
	    if (type == BT_EVENT) {

			snet_buffer* snbuf = (snet_buffer*) buf;
			
	        flowf("HCI not present for event, Posting to userland\n");
			port = find_port(BT_USERLAND_PORT_NAME);
	        if (port != B_NAME_NOT_FOUND) {
	            
				err = write_port_etc(port, PACK_PORTCODE(type,bdev->hdev, -1),
	                                snb_get(snbuf), snb_size(snbuf), B_TIMEOUT, 1*1000*1000);
				if (err != B_OK) 	            
	                debugf("Error posting userland %s\n", strerror(err));

				snb_park(&bdev->snetBufferRecycleTrash, snbuf);

	        }
	        else {
	            flowf("ERROR:bluetooth_server not found for posting\n");
	            err = B_NAME_NOT_FOUND;
	        }
        } else {
			net_buffer* nbuf = (net_buffer*) buf;
  			/* No need to free the buffer at allocation is gonna be reused */
  			flowf("HCI not present for acl posting to net_device\n");
   	        btDevices->receive_data(bdev->ndev, &nbuf);
			
        }
    }
    else {
        // TODO: Upper layer comunication
        /* Not freeing because is being used by upper layers*/    
    }

    
    return err;
}


status_t
send_packet(hci_id hid, bt_packet_t type, net_buffer* nbuf)
{
    bt_usb_dev* bdev = fetch_device(NULL, hid);
    status_t err = B_OK;

    if (bdev == NULL)
    	return B_ERROR;
    
    // TODO: check if device is actually ready for this
    // TODO: Lock Device

    if (nbuf != NULL) {
        if (type != nbuf->protocol) // a bit strict maybe?
            panic("Upper layer has not filled correctly a packet");
    
        switch (type) {
            case BT_COMMAND:
            case BT_ACL:
            case BT_SCO:
                    list_add_item(&bdev->nbuffersTx[type],nbuf);
                    bdev->nbuffersPendingTx[type]++;
            break;
            default:
                debugf("Unkown packet type for sending %d\n",type);
                // TODO: free the net_buffer -> no, allow upper layer 
                // handle it with the given error
                err = B_BAD_VALUE;
            break;
        }
    } else {
        flowf("tx sched provoked");
    }

    // TODO: check if device is actually ready for this
    // TODO: unLock device
        
    /* sched in All cases even if nbuf is null (hidden way to provoke re-scheduling)*/
    sched_tx_processing(bdev);
    
    return err;
}


status_t
send_command(hci_id hid, snet_buffer* snbuf)
{

    bt_usb_dev* bdev = fetch_device(NULL, hid);
    status_t err = B_OK;

    if (bdev == NULL)
    	return B_ERROR;
    
    // TODO: check if device is actually ready for this
    // TODO: mutex?

    if (snbuf != NULL) {
    
        list_add_item(&bdev->nbuffersTx[BT_COMMAND],snbuf);
        bdev->nbuffersPendingTx[BT_COMMAND]++;
    } else {
        err = B_BAD_VALUE;
        flowf("tx sched provoked");
    }

    // TODO: check if device is actually ready for this
    // TODO: mutex?
        
    /* sched in All cases even if nbuf is null (hidden way to provoke re-scheduling)*/
    sched_tx_processing(bdev);
    
    return err;
}
