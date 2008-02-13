/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include "h2generic.h"
#include "h2transactions.h"
#include "h2upper.h"
#include "h2util.h"

#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_acl.h>

#include <ByteOrder.h>

#include <string.h>

#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


/* Forward declaration */

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
void acl_tx_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void acl_rx_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void command_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void event_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
#else
void acl_tx_complete(void* cookie, status_t status, void* data, size_t actual_len);
void acl_rx_complete(void* cookie, status_t status, void* data, size_t actual_len);
void command_complete(void* cookie, status_t status, void* data, size_t actual_len);
void event_complete(void* cookie, status_t status, void* data, size_t actual_len);
#endif


static status_t 
assembly_rx(bt_usb_dev* bdev, bt_packet_t type, void *data, int count)
{
    
    net_buffer*		nbuf = NULL;
    snet_buffer*	snbuf = NULL;
    
    size_t      currentPacketLen = 0;
    size_t      expectedPacketLen = 0;

    bdev->stat.bytesRX += count;
	
	if (type == BT_EVENT)
		snbuf = bdev->eventRx;
	else
	    nbuf = bdev->nbufferRx[type]; 			

    while (count) {  

		debugf("count %d %p %p\n",count, nbuf, nb);		


        if ( (type != BT_EVENT && nbuf == NULL) ||
             (type == BT_EVENT && snb_completed(snbuf)) ) {

            /* new buffer incoming */                        
            switch (type) {
    			case BT_EVENT:
    				if (count >= HCI_EVENT_HDR_SIZE) {
    				    
    					struct hci_event_header* headerPkt = data;
    					expectedPacketLen = HCI_EVENT_HDR_SIZE + headerPkt->elen;
    					snbuf = bdev->eventRx = snb_fetch(&bdev->snetBufferRecycleTrash, expectedPacketLen);
    					
    				} else {
    					flowf("EVENT frame corrupted\n");
    					return -EILSEQ;
    				}
    			break;
    
    			case BT_ACL:
    				if (count >= HCI_ACL_HDR_SIZE) {
    				    
    					struct hci_acl_header* headerPkt = data;
    					expectedPacketLen = HCI_ACL_HDR_SIZE + B_LENDIAN_TO_HOST_INT16(headerPkt->alen);
    					
    					/* Create the buffer */
			            bdev->nbufferRx[type] = nbuf = nb->create(expectedPacketLen);
            			nbuf->protocol = type;

    					
    				} else {
    					flowf("ACL frame corrupted\n");
    					return -EILSEQ;
    				}
    			break;
    
    			case BT_SCO:    			    
        		break;
        		
        		default:
        		    panic("unkown packet type in assembly");
        		break;
            }
#if 0            
            if (nb == NULL) {            	
            	port_id port;
            	/* Coded for test purpose only we should panic here */
            	debugf("net_buffers are not ready post manually %d/%d\n",count,expectedPacketLen);       
           		port = find_port(BT_USERLAND_PORT_NAME);
	            if (port != B_NAME_NOT_FOUND) {
            		(void)write_port(port, PACK_HEADER_PORT(bdev->num,type), data, count);
            		return B_OK;
				}
                panic("Algorithm just ready to arrive here");
        		return B_ERROR;
            }
#endif            

            currentPacketLen = expectedPacketLen;

        } 
        else {
            /* Continuation */
            if (type == BT_EVENT)
	            currentPacketLen = get_expected_size(nbuf) - nbuf->size;        
	        else
	        	currentPacketLen = snb_remaining_to_put(snbuf);
        }

    	currentPacketLen = min(currentPacketLen, count);    	

        if (type == BT_EVENT)
        	snb_put(snbuf, data, currentPacketLen);
		else		
			nb->append(nbuf, data, currentPacketLen);				

		/* Complete frame? */
        if (type == BT_EVENT && snb_completed(snbuf)) {

			flowf("Frame goes up!\n");
			snbuf = bdev->eventRx = NULL;
			post_packet_up(bdev, type, snbuf);


        }
	
		if (type != BT_EVENT && (get_expected_size(nbuf) - nbuf->size) == 0 ) {

			flowf("Frame goes up!\n");
			bdev->nbufferRx[type] = nbuf = NULL;
			post_packet_up(bdev, type, nbuf);
		}
		        
        /* in case in the pipe there is info about the next buffer ... */
		count -= currentPacketLen; 
		data  += currentPacketLen;
    }
    
    return B_OK;
}


#if 0
#pragma mark --- RX Complete ---
#endif


void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
event_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
event_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
    
    bt_usb_dev* bdev = cookie;    
    status_t    err;
    
    /* TODO: check are we running? */

    if (status != B_OK || actual_len == 0)
        goto resubmit;

    if ( assembly_rx(cookie, BT_EVENT, data, actual_len) == B_OK ) {
        bdev->stat.successfulTX++;
    } else {
        bdev->stat.errorRX++;
    }
    
resubmit:

    err = usb->queue_interrupt(bdev->intr_in_ep->handle, 
                               data, bdev->max_packet_size_intr_in , 
				               event_complete, bdev);

   if (err != B_OK )   {
        reuse_room(&bdev->eventRoom, data);
        bdev->stat.rejectedRX++;
        debugf("RX event resubmittion failed %s\n",strerror(err));
    }
    else {
        bdev->stat.acceptedRX++;    
    }				     
    
}


void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
acl_rx_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
acl_rx_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
    bt_usb_dev* bdev = cookie;    
    status_t    err;
    
    /* TODO: check are we running? */

    if (status != B_OK || actual_len == 0)
        goto resubmit;

    if ( assembly_rx(cookie, BT_ACL, data, actual_len) == B_OK ) {
        bdev->stat.successfulRX++;
    } else {
        bdev->stat.errorRX++;
    }
    
resubmit:

	err = usb->queue_bulk(bdev->bulk_in_ep->handle, data, 
	                      max(HCI_MAX_FRAME_SIZE,bdev->max_packet_size_bulk_in), 
	                      acl_rx_complete, (void*) bdev);

    if (err != B_OK )   {
        reuse_room(&bdev->aclRoom, data);
        bdev->stat.rejectedRX++;
        debugf("RX acl resubmittion failed %s\n", strerror(err));
    }
    else {
        bdev->stat.acceptedRX++;    
    }				                
}


#if 0
#pragma mark --- RX ---
#endif


status_t
submit_rx_event(bt_usb_dev* bdev)
{   
    status_t    err;
    size_t      size = bdev->max_packet_size_intr_in;
    void*       buf = alloc_room(&bdev->eventRoom, size);
    
    if (buf == NULL)
        return ENOMEM;           

    err = usb->queue_interrupt(bdev->intr_in_ep->handle, 
                                   buf, size , 
				                   event_complete, (void*) bdev);
    if (err != B_OK )   {
        reuse_room(&bdev->eventRoom, buf);
        bdev->stat.rejectedRX++;
    }
    else {
        bdev->stat.acceptedRX++;
        debugf("Accepted RX Event %d\n", bdev->stat.acceptedRX);
    }
        
    return err;
}


status_t
submit_rx_acl(bt_usb_dev* bdev)
{

    status_t    err;
    size_t      size = max(HCI_MAX_FRAME_SIZE,bdev->max_packet_size_bulk_in);
    void*       buf = alloc_room(&bdev->aclRoom, size);

    if (buf == NULL)
        return ENOMEM; 
        
	err = usb->queue_bulk(bdev->bulk_in_ep->handle, buf, size , 
				                acl_rx_complete, bdev);

    if (err != B_OK )   {
        reuse_room(&bdev->aclRoom, buf);
        bdev->stat.rejectedRX++;
    }
    else {
        bdev->stat.acceptedRX++;    
    }				                
				                
    return B_ERROR;
}


status_t
submit_rx_sco(bt_usb_dev* bdev)
{

    /* not yet implemented */ 
    return B_ERROR;
}



#if 0
#pragma mark --- TX Complete ---
#endif

void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
command_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
command_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
    snet_buffer* snbuf = (snet_buffer*) cookie;
    bt_usb_dev* bdev = snb_cookie(snbuf);  

    debugf("%ld %02x:%02x:%02x:\n", actual_len, ((uint8*)data)[0],((uint8*)data)[1],((uint8*)data)[2]);

    if (status != B_OK) {
                
        bdev->stat.successfulTX++; 
        bdev->stat.bytesTX += actual_len;
    }
    else {
        bdev->stat.errorTX++;
        /* the packet has been lost */
        /* too late to requeue it? */
    }        

    snb_park(&bdev->snetBufferRecycleTrash, snbuf);

#ifdef BT_RESCHEDULING_AFTER_COMPLETITIONS 
    // TODO: check just the empty queues?
    schedTxProcessing(bdev);
#endif    
}


void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
acl_tx_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
acl_tx_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif

{

    net_buffer* nbuf = (net_buffer*) cookie;
    bt_usb_dev* bdev = GET_DEVICE(nbuf);    

    if (status != B_OK) {
                
        bdev->stat.successfulTX++; 
        bdev->stat.bytesTX += actual_len;
    }
    else {
        bdev->stat.errorTX++;
        /* the packet has been lost */
        /* too late to requeue it? */        
    }
    
    nb_destroy(nbuf);
#ifdef BT_RESCHEDULING_AFTER_COMPLETITIONS 
    schedTxProcessing(bdev);
#endif    
}


#if 0
#pragma mark --- TX ---
#endif

status_t
submit_tx_command(bt_usb_dev* bdev, snet_buffer* snbuf)
{    
    status_t err;
    
    uint8   bRequestType = bdev->ctrl_req;
	uint8   bRequest = 0;	
	uint16  wIndex = 0;	
	uint16  value = 0;
	uint16  wLength = B_HOST_TO_LENDIAN_INT16(snb_size(snbuf));

    /* set cookie */
    snb_set_cookie(snbuf, bdev);
    
	err = usb->queue_request(bdev->dev, bRequestType, bRequest,
		                     value, wIndex, wLength, 
		                     snb_get(snbuf), wLength //???
		                     ,command_complete, (void*) snbuf);
	
	if (err != B_OK )   {
        bdev->stat.rejectedTX++;
    }
    else {
        bdev->stat.acceptedTX++;    
    }		
	
    return err;
}

status_t
submit_tx_acl(bt_usb_dev* bdev, net_buffer* nbuf)
{
    status_t err;
        
    /* set cookie */
    SET_DEVICE(nbuf,bdev->hdev);
    
    err = usb->queue_bulk(bdev->bulk_out_ep->handle, 
                          nb_get_whole_buffer(nbuf), nbuf->size,    
				          acl_tx_complete, (void*) nbuf);
				          
	if (err != B_OK )   {
        bdev->stat.rejectedTX++;
    }
    else {
        bdev->stat.acceptedTX++;    
    }					          

    return err;
}


status_t
submit_tx_sco(bt_usb_dev* bdev)
{

    /* not yet implemented */ 
    return B_ERROR;
}

