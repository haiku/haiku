/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTHCI_H_
#define _BTHCI_H_

/* typedefs */
typedef int32 hci_id;
#define HCI_DEVICE_INDEX_OFFSET 0x7c

typedef enum { H2 = 2, H3, H4, H5 } transport_type;

typedef enum { 	BT_COMMAND = 0,
				BT_EVENT,
				BT_ACL,
				BT_SCO,
				BT_ESCO,
				// more packets here
				HCI_NUM_PACKET_TYPES
} bt_packet_t;

const char* BluetoothCommandOpcode(uint16 opcode);
const char* BluetoothEvent(uint8 event);
const char* BluetoothManufacturer(uint16 manufacturer);
const char* BluetoothHciVersion(uint16 ver);
const char* BluetoothLmpVersion(uint16 ver);
const char* BluetoothError(uint8 error);

/* packets sizes */
#define HCI_MAX_ACL_SIZE		1024
#define HCI_MAX_SCO_SIZE		255
#define HCI_MAX_EVENT_SIZE		260
#define HCI_MAX_FRAME_SIZE		(HCI_MAX_ACL_SIZE + 4)

/* fields sizes */
#define HCI_LAP_SIZE			3	/* LAP */
#define HCI_LINK_KEY_SIZE		16	/* link key */
#define HCI_PIN_SIZE			16	/* PIN */
#define HCI_EVENT_MASK_SIZE		8	/* event mask */
#define HCI_CLASS_SIZE			3	/* class */
#define HCI_FEATURES_SIZE		8	/* LMP features */
#define HCI_DEVICE_NAME_SIZE	248	/* unit name size */

// HCI Packet types
#define HCI_2DH1        0x0002
#define HCI_3DH1        0x0004
#define HCI_DM1         0x0008
#define HCI_DH1         0x0010
#define HCI_2DH3        0x0100
#define HCI_3DH3        0x0200
#define HCI_DM3         0x0400
#define HCI_DH3         0x0800
#define HCI_2DH5        0x1000
#define HCI_3DH5        0x2000
#define HCI_DM5         0x4000
#define HCI_DH5         0x8000

#define HCI_HV1         0x0020
#define HCI_HV2         0x0040
#define HCI_HV3         0x0080

#define HCI_EV3         0x0008
#define HCI_EV4         0x0010
#define HCI_EV5         0x0020
#define HCI_2EV3        0x0040
#define HCI_3EV3        0x0080
#define HCI_2EV5        0x0100
#define HCI_3EV5        0x0200

#define SCO_PTYPE_MASK  (HCI_HV1 | HCI_HV2 | HCI_HV3)
#define ACL_PTYPE_MASK  (HCI_DM1 | HCI_DH1 | HCI_DM3 | HCI_DH3 | HCI_DM5 | HCI_DH5)


// LMP features
#define LMP_3SLOT       0x01
#define LMP_5SLOT       0x02
#define LMP_ENCRYPT     0x04
#define LMP_SOFFSET     0x08
#define LMP_TACCURACY   0x10
#define LMP_RSWITCH     0x20
#define LMP_HOLD        0x40
#define LMP_SNIFF       0x80

#define LMP_PARK        0x01
#define LMP_RSSI        0x02
#define LMP_QUALITY     0x04
#define LMP_SCO         0x08
#define LMP_HV2         0x10
#define LMP_HV3         0x20
#define LMP_ULAW        0x40
#define LMP_ALAW        0x80

#define LMP_CVSD        0x01
#define LMP_PSCHEME     0x02
#define LMP_PCONTROL    0x04
#define LMP_TRSP_SCO    0x08
#define LMP_BCAST_ENC   0x80

#define LMP_EDR_ACL_2M  0x02
#define LMP_EDR_ACL_3M  0x04
#define LMP_ENH_ISCAN   0x08
#define LMP_ILACE_ISCAN 0x10
#define LMP_ILACE_PSCAN 0x20
#define LMP_RSSI_INQ    0x40
#define LMP_ESCO        0x80

#define LMP_EV4         0x01
#define LMP_EV5         0x02
#define LMP_AFH_CAP_SLV 0x08
#define LMP_AFH_CLS_SLV 0x10
#define LMP_EDR_3SLOT   0x80

#define LMP_EDR_5SLOT   0x01
#define LMP_SNIFF_SUBR  0x02
#define LMP_PAUSE_ENC   0x04
#define LMP_AFH_CAP_MST 0x08
#define LMP_AFH_CLS_MST 0x10
#define LMP_EDR_ESCO_2M 0x20
#define LMP_EDR_ESCO_3M 0x40
#define LMP_EDR_3S_ESCO 0x80

#define LMP_EXT_INQ     0x01
#define LMP_SIMPLE_PAIR 0x08
#define LMP_ENCAPS_PDU  0x10
#define LMP_ERR_DAT_REP 0x20
#define LMP_NFLUSH_PKTS 0x40

#define LMP_LSTO        0x01
#define LMP_INQ_TX_PWR  0x02
#define LMP_EXT_FEAT    0x80

// Link policies
#define HCI_LP_RSWITCH  0x0001
#define HCI_LP_HOLD     0x0002
#define HCI_LP_SNIFF    0x0004
#define HCI_LP_PARK     0x0008

// Link mode
#define HCI_LM_ACCEPT   0x8000
#define HCI_LM_MASTER   0x0001
#define HCI_LM_AUTH     0x0002
#define HCI_LM_ENCRYPT  0x0004
#define HCI_LM_TRUSTED  0x0008
#define HCI_LM_RELIABLE 0x0010
#define HCI_LM_SECURE   0x0020


#endif // _BTHCI_H_

