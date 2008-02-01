/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _BTHCI_H_
#define _BTHCI_H_

/* typedefs */
typedef int32 hci_id;

typedef enum { H2 = 2, H3, H4, H5 } transport_type; 

// TODO: something more authomatic here? 
#define HCI_NUM_PACKET_TYPES 5
typedef enum { BT_COMMAND = 0, 
               BT_EVENT, 
               BT_ACL, 
               BT_SCO,
               BT_ESCO
              } bt_packet_t;


/* packets sizes */
#define HCI_MAX_ACL_SIZE	1024
#define HCI_MAX_SCO_SIZE	255
#define HCI_MAX_EVENT_SIZE	260
#define HCI_MAX_FRAME_SIZE	(HCI_MAX_ACL_SIZE + 4)

/* fields sizes */
#define HCI_LAP_SIZE			3   /* LAP */
#define HCI_LINK_KEY_SIZE		16  /* link key */
#define HCI_PIN_SIZE			16  /* PIN */
#define HCI_EVENT_MASK_SIZE		8   /* event mask */
#define HCI_CLASS_SIZE			3   /* class */
#define HCI_FEATURES_SIZE		8   /* LMP features */
#define HCI_DEVICE_NAME_SIZE	248 /* unit name size */

#endif