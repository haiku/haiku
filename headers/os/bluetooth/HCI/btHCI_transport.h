/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
 
#ifndef _BTHCI_TRANSPORT_H_
#define _BTHCI_TRANSPORT_H_

#include <bluetooth/HCI/btHCI.h>

#include <net/net_buffer.h>


typedef enum { ANCILLYANT = 1, 
               RUNNING, 
               LEAVING,
               SENDING,
               PROCESSING
              } bt_transport_status_t;

typedef uint8 bt_stat_t; 
typedef struct bt_hci_statistics {
	
	bt_stat_t	acceptedTX;
	bt_stat_t	rejectedTX;
	bt_stat_t	successfulTX;
	bt_stat_t	errorTX;		

	bt_stat_t	acceptedRX;
	bt_stat_t	rejectedRX;
	bt_stat_t	successfulRX;
	bt_stat_t	errorRX;		

	bt_stat_t	commandTX;
	bt_stat_t	eventRX;
	bt_stat_t	aclTX;
	bt_stat_t	aclRX;	
	bt_stat_t	scoTX;
	bt_stat_t	scoRX;	
	bt_stat_t	escoTX;
	bt_stat_t	escoRX;
	
	bt_stat_t	bytesRX;
	bt_stat_t	bytesTX;

	
} bt_hci_statistics;
 	
/* TODO: Possible Hooks which drivers will have to provide */
typedef struct bt_hci_transport {

    status_t    (*SendCommand)(hci_id hci_dev, net_buffer* snbuf);
	status_t    (*SendPacket)(hci_id hci_dev, net_buffer* nbuf );
    status_t    (*SendSCO)(hci_id hci_dev, net_buffer* nbuf );
    status_t    (*DeliverStatistics)(bt_hci_statistics* statistics);
           
    transport_type   kind;
    char			 name[B_OS_NAME_LENGTH];
      
} bt_hci_transport;

typedef struct bt_hci_device {
      
    transport_type   kind;
    char			 realName[B_OS_NAME_LENGTH];
      
} bt_hci_device;


/* Here the transport driver have some flags that */
/* can be used to inform the upper layer about some */
/* special behaouvior to perform */

#define BT_IGNORE_THIS_DEVICE	(1<<0)
#define BT_SCO_NOT_WORKING		(1<<1)
#define BT_WILL_NEED_A_RESET	(1<<2)
#define BT_DIGIANSWER			(1<<4)

/* IOCTLS */
#define ISSUE_BT_COMMAND 		(1<<0)
#define GET_STATICS			    (1<<1)
#define GET_NOTIFICATION_PORT	(1<<2)
#define GET_HCI_ID              (1<<3)

#define PACK_PORTCODE(type,hid,data)   ((type&0xFF)<<24|(hid&0xFF)<<16|(data&0xFFFF))

/* Port drivers can use to send information (1 for all for 
   at moment refer to ioctl GET_NOTIFICATION_PORT)*/
#define BT_USERLAND_PORT_NAME "BT kernel-user Land"

#endif
