/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

/*-
 * Copyright (c) Maksim Yevmenkin <m_evmenkin@yahoo.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
*/


/**************************************************************************
 **************************************************************************
 **                   Common defines and types (L2CAP)
 **************************************************************************
 **************************************************************************/

#ifndef _L2CAP_
#define _L2CAP_

#include <bluetooth/bluetooth.h>

// TODO: from BSD compatibility layer
#define htole16(x) (x)
#define le16toh(x) (x)
#define le32toh(x) (x)
#define htole32(x) (x)

#define HZ	1000000 // us per second TODO: move somewhere more generic
#define bluetooth_l2cap_ertx_timeout (60 * HZ)
#define bluetooth_l2cap_rtx_timeout  (300 * HZ)

/*
 * Channel IDs are assigned relative to the instance of L2CAP node, i.e.
 * relative to the unit. So the total number of channels that unit can have
 * open at the same time is 0xffff - 0x0040 = 0xffbf (65471). This number
 * does not depend on number of connections.
 */
#define L2CAP_NULL_CID		0x0000	/* DO NOT USE THIS CID */
#define L2CAP_SIGNAL_CID	0x0001	/* signaling channel ID */
#define L2CAP_CLT_CID		0x0002	/* connectionless channel ID */
/* 0x0003 - 0x003f Reserved */
#define L2CAP_FIRST_CID		0x0040	/* dynamically alloc. (start) */
#define L2CAP_LAST_CID		0xffff	/* dynamically alloc. (end) */


/*
 * L2CAP signaling command ident's are assigned relative to the connection,
 * because there is only one signaling channel (cid == 0x01) for every
 * connection. So up to 254 (0xff - 0x01) L2CAP commands can be pending at the
 * same time for the same connection.
 */
#define L2CAP_NULL_IDENT		0x00	/* DO NOT USE THIS IDENT */
#define L2CAP_FIRST_IDENT		0x01	/* dynamically alloc. (start) */
#define L2CAP_LAST_IDENT		0xff	/* dynamically alloc. (end) */


/* L2CAP MTU */
#define L2CAP_MTU_MINIMUM		48
#define L2CAP_MTU_DEFAULT		672
#define L2CAP_MTU_MAXIMUM		0xffff

/* L2CAP flush and link timeouts */
#define L2CAP_FLUSH_TIMO_DEFAULT	0xffff /* always retransmit */
#define L2CAP_LINK_TIMO_DEFAULT		0xffff

/* L2CAP Command Reject reasons */
#define L2CAP_REJ_NOT_UNDERSTOOD	0x0000
#define L2CAP_REJ_MTU_EXCEEDED		0x0001
#define L2CAP_REJ_INVALID_CID		0x0002
/* 0x0003 - 0xffff - reserved for future use */

/* Protocol/Service Multioplexor (PSM) values */
#define L2CAP_PSM_ANY		0x0000	/* Any/Invalid PSM */
#define L2CAP_PSM_SDP		0x0001	/* Service Discovery Protocol */
#define L2CAP_PSM_RFCOMM	0x0003	/* RFCOMM protocol */
#define L2CAP_PSM_TCP		0x0005	/* Telephony Control Protocol */
#define L2CAP_PSM_TCS		0x0007	/* TCS cordless */
#define L2CAP_PSM_BNEP		0x000F	/* BNEP */
#define L2CAP_PSM_HID_CTRL	0x0011	/* HID Control */
#define L2CAP_PSM_HID_INT	0x0013	/* HID Interrupt */
#define L2CAP_PSM_UPnP		0x0015	/* UPnP (ESDP) */
#define L2CAP_PSM_AVCTP		0x0017	/* AVCTP */
#define L2CAP_PSM_AVDTP		0x0019	/* AVDTP */
/*  < 0x1000 - reserved for future use */
/*  0x1001 < x < 0xFFFF dinamically assigned */

/* L2CAP Connection response command result codes */
#define L2CAP_SUCCESS		0x0000
#define L2CAP_PENDING		0x0001
#define L2CAP_PSM_NOT_SUPPORTED	0x0002
#define L2CAP_SEQUIRY_BLOCK	0x0003
#define L2CAP_NO_RESOURCES	0x0004
#define L2CAP_TIMEOUT		0xeeee
#define L2CAP_UNKNOWN		0xffff
/* 0x0005 - 0xffff - reserved for future use */

/* L2CAP Connection response status codes */
#define L2CAP_NO_INFO		0x0000
#define L2CAP_AUTH_PENDING		0x0001
#define L2CAP_AUTZ_PENDING		0x0002
/* 0x0003 - 0xffff - reserved for future use */

/* L2CAP Configuration response result codes */
#define L2CAP_UNACCEPTABLE_PARAMS	0x0001
#define L2CAP_REJECT			0x0002
#define L2CAP_UNKNOWN_OPTION		0x0003
/* 0x0003 - 0xffff - reserved for future use */

/* L2CAP Configuration options */
#define L2CAP_OPT_CFLAG_BIT		0x0001
#define L2CAP_OPT_CFLAG(flags)		((flags) & L2CAP_OPT_CFLAG_BIT)
#define L2CAP_OPT_HINT_BIT		0x80
#define L2CAP_OPT_HINT(type)		((type) & L2CAP_OPT_HINT_BIT)
#define L2CAP_OPT_HINT_MASK		0x7f
#define L2CAP_OPT_MTU			0x01
#define L2CAP_OPT_MTU_SIZE		sizeof(uint16)
#define L2CAP_OPT_FLUSH_TIMO		0x02
#define L2CAP_OPT_FLUSH_TIMO_SIZE	sizeof(uint16)
#define L2CAP_OPT_QOS			0x03
#define L2CAP_OPT_QOS_SIZE		sizeof(l2cap_flow_t)
/* 0x4 - 0xff - reserved for future use */

#define	L2CAP_CFG_IN	(1 << 0)	/* incoming path done */
#define	L2CAP_CFG_OUT	(1 << 1)	/* outgoing path done */
#define	L2CAP_CFG_BOTH  (L2CAP_CFG_IN | L2CAP_CFG_OUT)
#define	L2CAP_CFG_IN_SENT	(1 << 2)	/* L2CAP ConfigReq sent */
#define	L2CAP_CFG_OUT_SENT	(1 << 3)	/* ---/--- */

/* L2CAP Information request type codes */
#define L2CAP_CONNLESS_MTU		0x0001
#define L2CAP_EXTENDED_MASK	0x0002
/* 0x0003 - 0xffff - reserved for future use */

/* L2CAP Information response codes */
#define L2CAP_NOT_SUPPORTED		0x0001
/* 0x0002 - 0xffff - reserved for future use */

/* L2CAP flow (QoS) */
typedef struct {
	uint8	flags;             /* reserved for future use */
	uint8	service_type;      /* service type */
	uint32	token_rate;        /* bytes per second */
	uint32	token_bucket_size; /* bytes */
	uint32	peak_bandwidth;    /* bytes per second */
	uint32	latency;           /* microseconds */
	uint32	delay_variation;   /* microseconds */
} __attribute__ ((packed)) l2cap_flow_t;


/**************************************************************************
 **************************************************************************
 **                 Link level defines, headers and types
 **************************************************************************
 **************************************************************************/

/* L2CAP header */
typedef struct {
	uint16	length;	/* payload size */
	uint16	dcid;	/* destination channel ID */
} __attribute__ ((packed)) l2cap_hdr_t;


/* L2CAP ConnectionLess Traffic (CLT) (if destination cid == 0x2) */
typedef struct {
	uint16	psm; /* Protocol/Service Multiplexor */
} __attribute__ ((packed)) l2cap_clt_hdr_t;

#define L2CAP_CLT_MTU_MAXIMUM (L2CAP_MTU_MAXIMUM - sizeof(l2cap_clt_hdr_t))

/* L2CAP command header */
typedef struct {
	uint8	code;   /* command OpCode */
	uint8	ident;  /* identifier to match request and response */
	uint16	length; /* command parameters length */
} __attribute__ ((packed)) l2cap_cmd_hdr_t;


/* L2CAP Command Reject */
#define L2CAP_CMD_REJ	0x01
typedef struct {
	uint16	reason; /* reason to reject command */
/*	uint8	data[]; -- optional data (depends on reason) */
} __attribute__ ((packed)) l2cap_cmd_rej_cp;

/* CommandReject data */
typedef union {
 	/* L2CAP_REJ_MTU_EXCEEDED */
	struct {
		uint16	mtu; /* actual signaling MTU */
	} __attribute__ ((packed)) mtu;
	/* L2CAP_REJ_INVALID_CID */
	struct {
		uint16	scid; /* local CID */
		uint16	dcid; /* remote CID */
	} __attribute__ ((packed)) cid;
} l2cap_cmd_rej_data_t;
typedef l2cap_cmd_rej_data_t * l2cap_cmd_rej_data_p;

/* L2CAP Connection Request */
#define L2CAP_CON_REQ	0x02
typedef struct {
	uint16	psm;  /* Protocol/Service Multiplexor (PSM) */
	uint16	scid; /* source channel ID */
} __attribute__ ((packed)) l2cap_con_req_cp;

/* L2CAP Connection Response */
#define L2CAP_CON_RSP	0x03
typedef struct {
	uint16	dcid;   /* destination channel ID */
	uint16	scid;   /* source channel ID */
	uint16	result; /* 0x00 - success */
	uint16	status; /* more info if result != 0x00 */
} __attribute__ ((packed)) l2cap_con_rsp_cp;

/* L2CAP Configuration Request */
#define L2CAP_CFG_REQ	0x04
typedef struct {
	uint16	dcid;  /* destination channel ID */
	uint16	flags; /* flags */
/*	uint8	options[] --  options */
} __attribute__ ((packed)) l2cap_cfg_req_cp;

/* L2CAP Configuration Response */
#define L2CAP_CFG_RSP	0x05
typedef struct {
	uint16	scid;   /* source channel ID */
	uint16	flags;  /* flags */
	uint16	result; /* 0x00 - success */
/*	uint8	options[] -- options */
} __attribute__ ((packed)) l2cap_cfg_rsp_cp;

/* L2CAP configuration option */
typedef struct {
	uint8	type;
	uint8	length;
/*	uint8	value[] -- option value (depends on type) */
} __attribute__ ((packed)) l2cap_cfg_opt_t;
typedef l2cap_cfg_opt_t * l2cap_cfg_opt_p;

/* L2CAP configuration option value */
typedef union {
	uint16		mtu;		/* L2CAP_OPT_MTU */
	uint16		flush_timo;	/* L2CAP_OPT_FLUSH_TIMO */
	l2cap_flow_t	flow;		/* L2CAP_OPT_QOS */
} l2cap_cfg_opt_val_t;
typedef l2cap_cfg_opt_val_t * l2cap_cfg_opt_val_p;

/* L2CAP Disconnect Request */
#define L2CAP_DISCON_REQ	0x06
typedef struct {
	uint16	dcid; /* destination channel ID */
	uint16	scid; /* source channel ID */
} __attribute__ ((packed)) l2cap_discon_req_cp;

/* L2CAP Disconnect Response */
#define L2CAP_DISCON_RSP	0x07
typedef l2cap_discon_req_cp	l2cap_discon_rsp_cp;

/* L2CAP Echo Request */
#define L2CAP_ECHO_REQ	0x08
/* No command parameters, only optional data */

/* L2CAP Echo Response */
#define L2CAP_ECHO_RSP	0x09
#define L2CAP_MAX_ECHO_SIZE \
	(L2CAP_MTU_MAXIMUM - sizeof(l2cap_cmd_hdr_t))
/* No command parameters, only optional data */

/* L2CAP Information Request */
#define L2CAP_INFO_REQ	0x0a
typedef struct {
	uint16	type; /* requested information type */
} __attribute__ ((packed)) l2cap_info_req_cp;

/* L2CAP Information Response */
#define L2CAP_INFO_RSP	0x0b
typedef struct {
	uint16	type;   /* requested information type */
	uint16	result; /* 0x00 - success */
/*	uint8	info[]  -- info data (depends on type)
 *
 * L2CAP_CONNLESS_MTU - 2 bytes connectionless MTU
 */
} __attribute__ ((packed)) l2cap_info_rsp_cp;

#define IS_SIGNAL_REQ(code) ((code & 1) == 0)
#define IS_SIGNAL_RSP(code) ((code & 1) == 1)


typedef union {
 	/* L2CAP_CONNLESS_MTU */
	struct {
		uint16	mtu;
	} __attribute__ ((packed)) mtu;
} l2cap_info_rsp_data_t;
typedef l2cap_info_rsp_data_t *	l2cap_info_rsp_data_p;


#endif

