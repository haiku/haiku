/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTHCI_COMMAND_H_
#define _BTHCI_COMMAND_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>

#define HCI_COMMAND_HDR_SIZE 3

struct hci_command_header {
	uint16	opcode;		/* OCF & OGF */
	uint8	clen;
} __attribute__ ((packed));


/* Command opcode pack/unpack */
#define PACK_OPCODE(ogf, ocf)	(uint16)((ocf & 0x03ff)|(ogf << 10))
#define GET_OPCODE_OGF(op)		(op >> 10)
#define GET_OPCODE_OCF(op)		(op & 0x03ff)


/* - Informational Parameters Command definition - */
#define OGF_INFORMATIONAL_PARAM	0x04

	#define OCF_READ_LOCAL_VERSION		0x0001
	struct hci_rp_read_loc_version {
		uint8		status;
		uint8		hci_version;
		uint16		hci_revision;
		uint8		lmp_version;
		uint16		manufacturer;
		uint16		lmp_subversion;
	} __attribute__ ((packed));

	#define OCF_READ_LOCAL_FEATURES		0x0003
	struct hci_rp_read_loc_features {
		uint8		status;
		uint8		features[8];
	} __attribute__ ((packed));

	#define OCF_READ_BUFFER_SIZE		0x0005
	struct hci_rp_read_buffer_size {
		uint8		status;
		uint16		acl_mtu;
		uint8		sco_mtu;
		uint16		acl_max_pkt;
		uint16		sco_max_pkt;
	} __attribute__ ((packed));

	#define OCF_READ_BD_ADDR			0x0009
	struct hci_rp_read_bd_addr {
		uint8		status;
		bdaddr_t	bdaddr;
	} __attribute__ ((packed));

/* - Host Controller and Baseband Command definition - */
#define OGF_CONTROL_BASEBAND			0x03

	#define OCF_RESET					0x0003
  /*struct hci_reset {
		void no_fields;
	} __attribute__ ((packed));*/

	#define OCF_SET_EVENT_FLT			0x0005
	struct hci_cp_set_event_flt {
		uint8		flt_type;
		uint8		cond_type;
		uint8		condition[0];
	} __attribute__ ((packed));

	#define OCF_READ_STORED_LINK_KEY	0x000D
	struct hci_read_stored_link_key {
		bdaddr_t	bdaddr;
		uint8		all_keys_flag;
	} __attribute__ ((packed));
	struct hci_read_stored_link_key_reply {
		uint8		status;
		uint16		max_num_keys;
		uint16		num_keys_read;
	} __attribute__ ((packed));

	#define OCF_WRITE_STORED_LINK_KEY	0x0011
	struct hci_write_stored_link_key {
		uint8		num_keys_to_write;
		// these are repeated "num_keys_write" times
		bdaddr_t	bdaddr;
		uint8		key[HCI_LINK_KEY_SIZE];
	} __attribute__ ((packed));
	struct hci_write_stored_link_key_reply {
		uint8		status;
		uint8		num_keys_written;
	} __attribute__ ((packed));


	#define OCF_WRITE_LOCAL_NAME		0x0013
	struct hci_write_local_name {
		char		local_name[HCI_DEVICE_NAME_SIZE];
	} __attribute__ ((packed));

	#define OCF_READ_LOCAL_NAME			0x0014
	struct hci_rp_read_local_name {
		uint8		status;
		char		local_name[HCI_DEVICE_NAME_SIZE];
	} __attribute__ ((packed));

	#define OCF_READ_CA_TIMEOUT			0x0015
	#define OCF_WRITE_CA_TIMEOUT		0x0016
	#define OCF_READ_PG_TIMEOUT			0x0017
	struct hci_rp_read_page_timeout {
		uint8		status;
		uint16		page_timeout;
	} __attribute__ ((packed));
	#define OCF_WRITE_PG_TIMEOUT		0x0018

	#define OCF_READ_SCAN_ENABLE		0x0019
	struct hci_read_scan_enable {
		uint8		status;
		uint8		enable;
	} __attribute__ ((packed));

	#define OCF_WRITE_SCAN_ENABLE		0x001A
		#define HCI_SCAN_DISABLED		0x00
		#define HCI_SCAN_INQUIRY		0x01 //Page Scan disabled
		#define HCI_SCAN_PAGE			0x02 //Inquiry Scan disabled
		#define HCI_SCAN_INQUIRY_PAGE	0x03 //All enabled
	struct hci_write_scan_enable {
		uint8		scan;
	} __attribute__ ((packed));

	#define OCF_READ_AUTH_ENABLE		0x001F
	#define OCF_WRITE_AUTH_ENABLE		0x0020
		#define HCI_AUTH_DISABLED		0x00
		#define HCI_AUTH_ENABLED		0x01
	struct hci_write_authentication_enable {
		uint8		authentication;
	} __attribute__ ((packed));

	#define OCF_READ_ENCRYPT_MODE		0x0021
	#define OCF_WRITE_ENCRYPT_MODE		0x0022
		#define HCI_ENCRYPT_DISABLED	0x00
		#define HCI_ENCRYPT_P2P			0x01
		#define HCI_ENCRYPT_BOTH		0x02
	struct hci_write_encryption_mode_enable {
		uint8		encryption;
	} __attribute__ ((packed));

	/* Filter types */
	#define HCI_FLT_CLEAR_ALL			0x00
	#define HCI_FLT_INQ_RESULT			0x01
	#define HCI_FLT_CONN_SETUP			0x02

	/* CONN_SETUP Condition types */
	#define HCI_CONN_SETUP_ALLOW_ALL	0x00
	#define HCI_CONN_SETUP_ALLOW_CLASS	0x01
	#define HCI_CONN_SETUP_ALLOW_BDADDR	0x02

	/* CONN_SETUP Conditions */
	#define HCI_CONN_SETUP_AUTO_OFF		0x01
	#define HCI_CONN_SETUP_AUTO_ON		0x02

	#define OCF_READ_CLASS_OF_DEV		0x0023

	struct hci_read_dev_class_reply {
		uint8		status;
		uint8		dev_class[3];
	} __attribute__ ((packed));

	#define OCF_WRITE_CLASS_OF_DEV		0x0024
	struct hci_write_dev_class {
		uint8		dev_class[3];
	} __attribute__ ((packed));

	#define OCF_READ_VOICE_SETTING		0x0025
	struct hci_rp_read_voice_setting {
		uint8		status;
		uint16		voice_setting;
	} __attribute__ ((packed));

	#define OCF_WRITE_VOICE_SETTING		0x0026
	struct hci_cp_write_voice_setting {
		uint16		voice_setting;
	} __attribute__ ((packed));

	#define OCF_HOST_BUFFER_SIZE		0x0033
	struct hci_cp_host_buffer_size {
		uint16		acl_mtu;
		uint8		sco_mtu;
		uint16		acl_max_pkt;
		uint16		sco_max_pkt;
	} __attribute__ ((packed));

	/* Link Control Command definition */
	#define OGF_LINK_CONTROL			0x01

	#define OCF_INQUIRY					0x0001
	struct hci_cp_inquiry {
		uint8		lap[3];
		uint8		length;
		uint8		num_rsp;
	} __attribute__ ((packed));

	#define OCF_INQUIRY_CANCEL			0x0002

	#define OCF_CREATE_CONN				0x0005
	struct hci_cp_create_conn {
		bdaddr_t bdaddr;
		uint16		pkt_type;
		uint8		pscan_rep_mode;
		uint8		pscan_mode;
		uint16		clock_offset;
		uint8		role_switch;
	} __attribute__ ((packed));

	#define OCF_DISCONNECT				0x0006
	struct hci_disconnect {
		uint16		handle;
		uint8		reason;
	} __attribute__ ((packed));

	#define OCF_ADD_SCO					0x0007
	struct hci_cp_add_sco {
		uint16		handle;
		uint16		pkt_type;
	} __attribute__ ((packed));

	#define OCF_ACCEPT_CONN_REQ			0x0009
	struct hci_cp_accept_conn_req {
		bdaddr_t	bdaddr;
		uint8		role;
	} __attribute__ ((packed));

	#define OCF_REJECT_CONN_REQ			0x000a
	struct hci_cp_reject_conn_req {
		bdaddr_t	bdaddr;
		uint8		reason;
	} __attribute__ ((packed));

	#define OCF_LINK_KEY_REPLY			0x000B
	struct hci_cp_link_key_reply {
		bdaddr_t	bdaddr;
		uint8		link_key[16];
	} __attribute__ ((packed));

	#define OCF_LINK_KEY_NEG_REPLY		0x000C
	struct hci_cp_link_key_neg_reply {
		bdaddr_t	bdaddr;
	} __attribute__ ((packed));

	#define OCF_PIN_CODE_REPLY			0x000D
	struct hci_cp_pin_code_reply {
		bdaddr_t bdaddr;
		uint8		pin_len;
		uint8		pin_code[HCI_PIN_SIZE];
	} __attribute__ ((packed));

	struct hci_cp_link_key_reply_reply {
		uint8	status;
		bdaddr_t bdaddr;
	} __attribute__ ((packed));

	#define OCF_PIN_CODE_NEG_REPLY		0x000E
	struct hci_cp_pin_code_neg_reply {
		bdaddr_t	bdaddr;
	} __attribute__ ((packed));

	#define OCF_CHANGE_CONN_PTYPE		0x000F
	struct hci_cp_change_conn_ptype {
		uint16		handle;
		uint16		pkt_type;
	} __attribute__ ((packed));

	#define OCF_AUTH_REQUESTED			0x0011
	struct hci_cp_auth_requested {
		uint16		handle;
	} __attribute__ ((packed));

	#define OCF_SET_CONN_ENCRYPT		0x0013
	struct hci_cp_set_conn_encrypt {
		uint16		handle;
		uint8		encrypt;
	} __attribute__ ((packed));

	#define OCF_CHANGE_CONN_LINK_KEY	0x0015
	struct hci_cp_change_conn_link_key {
		uint16		handle;
	} __attribute__ ((packed));

	#define OCF_REMOTE_NAME_REQUEST		0x0019
	struct hci_remote_name_request {
		bdaddr_t bdaddr;
		uint8		pscan_rep_mode;
		uint8		reserved;
		uint16		clock_offset;
	} __attribute__ ((packed));

	#define OCF_READ_REMOTE_FEATURES	0x001B
	struct hci_cp_read_rmt_features {
		uint16		handle;
	} __attribute__ ((packed));

	#define OCF_READ_REMOTE_VERSION		0x001D
	struct hci_cp_read_rmt_version {
		uint16		handle;
	} __attribute__ ((packed));


/* Link Policy Command definition */
#define OGF_LINK_POLICY					0x02

	#define OCF_ROLE_DISCOVERY			0x0009
	struct hci_cp_role_discovery {
		uint16		handle;
	} __attribute__ ((packed));
	struct hci_rp_role_discovery {
		uint8		status;
		uint16		handle;
		uint8		role;
	} __attribute__ ((packed));

	#define OCF_FLOW_SPECIFICATION
	struct hci_cp_flow_specification {
		uint16		handle;
		uint8		flags;
		uint8		flow_direction;
		uint8		service_type;
		uint32		token_rate;
		uint32		token_bucket;
		uint32		peak;
		uint32		latency;
	} __attribute__ ((packed));
	/* Quality of service types */
	#define HCI_SERVICE_TYPE_NO_TRAFFIC		0x00
	#define HCI_SERVICE_TYPE_BEST_EFFORT		0x01
	#define HCI_SERVICE_TYPE_GUARANTEED		0x02
	/* 0x03 - 0xFF - reserved for future use */

	#define OCF_READ_LINK_POLICY		0x000C
	struct hci_cp_read_link_policy {
		uint16		handle;
	} __attribute__ ((packed));
	struct hci_rp_read_link_policy {
		uint8		status;
		uint16		handle;
		uint16		policy;
	} __attribute__ ((packed));

	#define OCF_SWITCH_ROLE				0x000B
	struct hci_cp_switch_role {
		bdaddr_t	bdaddr;
		uint8		role;
	} __attribute__ ((packed));

	#define OCF_WRITE_LINK_POLICY		0x000D
	struct hci_cp_write_link_policy {
		uint16		handle;
		uint16		policy;
	} __attribute__ ((packed));
	struct hci_rp_write_link_policy {
		uint8		status;
		uint16		handle;
	} __attribute__ ((packed));

/* Status params */
#define OGF_STATUS_PARAM				0x05

/* Testing commands */
#define OGF_TESTING_CMD					0x06

/* Vendor specific commands */
#define OGF_VENDOR_CMD					0x3F

#define OCF_WRITE_BCM2035_BDADDR		0x01
	struct hci_write_bcm2035_bdaddr {
		bdaddr_t bdaddr;
	} _PACKED;

#endif // _BTHCI_COMMAND_H_
