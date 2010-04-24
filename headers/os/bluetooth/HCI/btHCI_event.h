/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTHCI_EVENT_H_
#define _BTHCI_EVENT_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>

#define HCI_EVENT_HDR_SIZE	2

struct hci_event_header {
	uint8		ecode;
	uint8		elen;
} __attribute__ ((packed));


/* ---- HCI Events ---- */
#define HCI_EVENT_INQUIRY_COMPLETE					0x01

#define HCI_EVENT_INQUIRY_RESULT					0x02
struct inquiry_info {
	bdaddr_t	bdaddr;
	uint8		pscan_rep_mode;
	uint8		pscan_period_mode;
	uint8		pscan_mode;
	uint8		dev_class[3];
	uint16		clock_offset;
} __attribute__ ((packed));

#define HCI_EVENT_CONN_COMPLETE						0x03
struct hci_ev_conn_complete {
	uint8		status;
	uint16		handle;
	bdaddr_t	bdaddr;
	uint8		link_type;
	uint8		encrypt_mode;
} __attribute__ ((packed));

#define HCI_EVENT_CONN_REQUEST						0x04
struct hci_ev_conn_request {
	bdaddr_t	bdaddr;
	uint8		dev_class[3];
	uint8		link_type;
} __attribute__ ((packed));

#define HCI_EVENT_DISCONNECTION_COMPLETE			0x05
struct hci_ev_disconnection_complete_reply {
	uint8		status;
	uint16		handle;
	uint8		reason;
} __attribute__ ((packed));

#define HCI_EVENT_AUTH_COMPLETE						0x06
struct hci_ev_auth_complete {
	uint8		status;
	uint16		handle;
} __attribute__ ((packed));

#define HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE		0x07
struct hci_ev_remote_name_request_complete_reply {
	uint8		status;
	bdaddr_t	bdaddr;
	char		remote_name[248];
} __attribute__ ((packed));

#define HCI_EVENT_ENCRYPT_CHANGE					0x08
struct hci_ev_encrypt_change {
	uint8		status;
	uint16		handle;
	uint8		encrypt;
} __attribute__ ((packed));

#define HCI_EVENT_CHANGE_CONN_LINK_KEY_COMPLETE		0x09
struct hci_ev_change_conn_link_key_complete {
	uint8		status;
	uint16		handle;
} __attribute__ ((packed));

#define HCI_EVENT_MASTER_LINK_KEY_COMPL				0x0a
struct hci_ev_master_link_key_complete {
	uint8		status;	 /* 0x00 - success */
	uint16		handle;	 /* Connection handle */
	uint8		key_flag;   /* Key flag */
} __attribute__ ((packed));

#define HCI_EVENT_RMT_FEATURES						0x0B
struct hci_ev_rmt_features {
	uint8		status;
	uint16		handle;
	uint8		features[8];
} __attribute__ ((packed));

#define HCI_EVENT_RMT_VERSION						0x0C
struct hci_ev_rmt_version {
	uint8		status;
	uint16		handle;
	uint8		lmp_ver;
	uint16		manufacturer;
	uint16		lmp_subver;
} __attribute__ ((packed));

#define HCI_EVENT_QOS_SETUP_COMPLETE				0x0D
struct hci_qos {
	uint8		service_type;
	uint32		token_rate;
	uint32		peak_bandwidth;
	uint32		latency;
	uint32		delay_variation;
} __attribute__ ((packed));
struct hci_ev_qos_setup_complete {
	uint8		status;
	uint16		handle;
	struct		hci_qos qos;
} __attribute__ ((packed));

#define HCI_EVENT_CMD_COMPLETE 						0x0E
struct hci_ev_cmd_complete {
	uint8		ncmd;
	uint16		opcode;
} __attribute__ ((packed));

#define HCI_EVENT_CMD_STATUS 						0x0F
struct hci_ev_cmd_status {
	uint8		status;
	uint8		ncmd;
	uint16		opcode;
} __attribute__ ((packed));

#define HCI_EVENT_HARDWARE_ERROR					0x10
struct hci_ev_hardware_error {
	uint8		hardware_code; /* hardware error code */
} __attribute__ ((packed)) ;

#define HCI_EVENT_FLUSH_OCCUR						0x11
struct hci_ev_flush_occur {
	uint16		handle; /* connection handle */
} __attribute__ ((packed)) ;

#define HCI_EVENT_ROLE_CHANGE						0x12
struct hci_ev_role_change {
	uint8		status;
	bdaddr_t	bdaddr;
	uint8		role;
} __attribute__ ((packed));

#define HCI_EVENT_NUM_COMP_PKTS						0x13
struct handle_and_number {
	uint16		handle;
	uint16		num_completed;
} __attribute__ ((packed));

struct hci_ev_num_comp_pkts {
	uint8		num_hndl;
	// struct	handle_and_number; hardcoded...
} __attribute__ ((packed));

#define HCI_EVENT_MODE_CHANGE						0x14
struct hci_ev_mode_change {
	uint8		status;
	uint16		handle;
	uint8		mode;
	uint16		interval;
} __attribute__ ((packed));

#define HCI_EVENT_RETURN_LINK_KEYS					0x15
struct link_key_info {
	bdaddr_t	bdaddr;
	linkkey_t	link_key;
} __attribute__ ((packed)) ;
struct hci_ev_return_link_keys {
	uint8					num_keys;	/* # of keys */
	struct link_key_info	link_keys;   /* As much as num_keys param */
} __attribute__ ((packed)) ;

#define HCI_EVENT_PIN_CODE_REQ						0x16
struct hci_ev_pin_code_req {
	bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_EVENT_LINK_KEY_REQ						0x17
struct hci_ev_link_key_req {
	bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_EVENT_LINK_KEY_NOTIFY					0x18
struct hci_ev_link_key_notify {
	bdaddr_t bdaddr;
	linkkey_t	link_key;
	uint8	 key_type;
} __attribute__ ((packed));

#define HCI_EVENT_LOOPBACK_COMMAND					0x19
struct hci_ev_loopback_command {
	uint8		command[0]; /* depends of command */
} __attribute__ ((packed)) ;

#define HCI_EVENT_DATA_BUFFER_OVERFLOW				0x1a
struct hci_ev_data_buffer_overflow {
	uint8		link_type; /* Link type */
} __attribute__ ((packed)) ;

#define HCI_EVENT_MAX_SLOT_CHANGE					0x1b
struct hci_ev_max_slot_change {
	uint16	handle;	/* connection handle */
	uint8	lmp_max_slots; /* Max. # of slots allowed */
} __attribute__ ((packed)) ;

#define HCI_EVENT_READ_CLOCK_OFFSET_COMPL			0x1c
struct hci_ev_read_clock_offset_compl {
	uint8		status;	   /* 0x00 - success */
	uint16		handle;	   /* Connection handle */
	uint16		clock_offset; /* Clock offset */
} __attribute__ ((packed)) ;

#define HCI_EVENT_CON_PKT_TYPE_CHANGED				0x1d
struct hci_ev_con_pkt_type_changed {
	uint8		status;	 /* 0x00 - success */
	uint16		handle; /* connection handle */
	uint16		pkt_type;   /* packet type */
} __attribute__ ((packed));

#define HCI_EVENT_QOS_VIOLATION						0x1e
struct hci_ev_qos_violation {
	uint16		handle; /* connection handle */
} __attribute__ ((packed)) ;

#define HCI_EVENT_PAGE_SCAN_REP_MODE_CHANGE			0x20
struct hci_ev_page_scan_rep_mode_change {
	bdaddr_t	bdaddr;			 /* destination address */
	uint8		page_scan_rep_mode; /* page scan repetition mode */
} __attribute__ ((packed));

/* Events Beyond Bluetooth 1.1 */
#define HCI_EVENT_FLOW_SPECIFICATION				0x21
struct hci_ev_flow_specification {
	uint8		status;
	uint16		handle;
	uint8		flags;
	uint8		flow_direction;
	uint8		service_type;
	uint32		token_rate;
	uint32 		token_bucket_size;
	uint32		peak_bandwidth;
	uint32		access_latency;
} __attribute__ ((packed));

#define HCI_EVENT_INQUIRY_RESULT_WITH_RSSI			0x22
struct hci_ev_inquiry_info_with_rssi {
	bdaddr_t bdaddr;
	uint8		pscan_rep_mode;
	uint8		pscan_period_mode;
	uint8		dev_class[3];
	uint16		clock_offset;
	int8		rssi;
} __attribute__ ((packed));

#define HCI_EVENT_REMOTE_EXTENDED_FEATURES			0x23
struct hci_ev_remote_extended_features {
	uint8		status;
	uint16		handle;
	uint8		page_number;
	uint8		maximun_page_number;
	uint64		extended_lmp_features;
} __attribute__ ((packed));

#define HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETED	0x2C
struct hci_ev_sychronous_connection_completed {
	uint8		status;
	uint16		handle;
	bdaddr_t	bdaddr;
	uint8		link_type;
	uint8		transmission_interval;
	uint8		retransmission_window;
	uint16		rx_packet_length;
	uint16		tx_packet_length;
	uint8		air_mode;
} __attribute__ ((packed));

#define HCI_EVENT_SYNCHRONOUS_CONNECTION_CHANGED	0x2D
struct hci_ev_sychronous_connection_changed {
	uint8		status;
	uint16		handle;
	uint8		transmission_interval;
	uint8		retransmission_window;
	uint16		rx_packet_length;
	uint16		tx_packet_length;
} __attribute__ ((packed));

// TODO: Define remaining Bluetooth 2.1 events structures
#define HCI_EVENT_EXTENDED_INQUIRY_RESULT			0x2F

#define HCI_EVENT_ENCRYPTION_KEY_REFRESH_COMPLETE	0x30

#define HCI_EVENT_IO_CAPABILITY_REQUEST				0x31

#define HCI_EVENT_IO_CAPABILITY_RESPONSE			0x32

#define HCI_EVENT_USER_CONFIRMATION_REQUEST			0x33

#define HCI_EVENT_USER_PASSKEY_REQUEST				0x34

#define HCI_EVENT_OOB_DATA_REQUEST					0x35

#define HCI_EVENT_SIMPLE_PAIRING_COMPLETE			0x36

#define HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED	0x38

#define HCI_EVENT_ENHANCED_FLUSH_COMPLETE			0x39

#define HCI_EVENT_KEYPRESS_NOTIFICATION				0x3C

#define HCI_EVENT_REMOTE_HOST_SUPPORTED_FEATURES_NOTIFICATION	0x3D


/* HAIKU Internal Events, not produced by the transport devices but
 * by some entity of the Haiku Bluetooth Stack.
 * The MSB 0xE is chosen for this purpose
 */

#define HCI_HAIKU_EVENT_SERVER_QUITTING				0xE0
#define HCI_HAIKU_EVENT_DEVICE_REMOVED				0xE1


#endif // _BTHCI_EVENT_H_
