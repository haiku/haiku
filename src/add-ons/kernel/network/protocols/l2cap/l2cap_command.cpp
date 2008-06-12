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


#include <NetBufferUtilities.h>

#include "l2cap_command.h"

/*
 * Note: All L2CAP implementations are required to support minimal signaling
 *       MTU of 48 bytes. In order to simplify things we will send one command
 *       per one L2CAP packet. Given evrything above we can assume that one
 *       signaling packet will fit into single mbuf.
 */


/* Private types */
struct _cmd_rej {
	l2cap_cmd_hdr_t	 	hdr;
	l2cap_cmd_rej_cp 	param;
	l2cap_cmd_rej_data_t	data;
} __attribute__ ((packed)) ;

struct _con_req {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_con_req_cp param;
} __attribute__ ((packed));

struct _con_rsp {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_con_rsp_cp param;
} __attribute__ ((packed));

struct _cfg_req {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_cfg_req_cp param;
} __attribute__ ((packed));

struct _cfg_rsp {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_cfg_rsp_cp param;
} __attribute__ ((packed));

struct _discon_req {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_discon_req_cp	 param;
} __attribute__ ((packed));

struct _discon_rsp {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_discon_rsp_cp	 param;
} __attribute__ ((packed));

struct _info_req {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_info_req_cp	 param;
} __attribute__ ((packed));

struct _info_rsp {
	l2cap_cmd_hdr_t	 hdr;
	l2cap_info_rsp_cp	 param;
	l2cap_info_rsp_data_t data;
} __attribute__ ((packed));


/* L2CAP_CommandRej */
static inline net_buffer*
l2cap_cmd_rej(uint8 _ident, uint16 _reason, uint16 _mtu, uint16 _scid, uint16 _dcid)
{
	
	net_buffer* _m = gBufferModule->create(sizeof(struct _cmd_rej));
	if ((_m) == NULL)
		return NULL;

	NetBufferPrepend<struct _cmd_rej> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		// free the buffer
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_CMD_REJ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = sizeof(bufferHeader->param);

	bufferHeader->param.reason = htole16((_reason));

	if ((_reason) == L2CAP_REJ_MTU_EXCEEDED) {
		bufferHeader->data.mtu.mtu = htole16((_mtu));
		bufferHeader->hdr.length += sizeof(bufferHeader->data.mtu);
	} else if ((_reason) == L2CAP_REJ_INVALID_CID) {
		bufferHeader->data.cid.scid = htole16((_scid));
		bufferHeader->data.cid.dcid = htole16((_dcid));
		bufferHeader->hdr.length += sizeof(bufferHeader->data.cid);
	}
	
	_m->size = sizeof(bufferHeader->hdr) + bufferHeader->hdr.length;	 /* TODO: needed ?*/

	bufferHeader->hdr.length = htole16(bufferHeader->hdr.length);

	bufferHeader.Sync();

	return _m;
}


/* L2CAP_ConnectReq */
static inline net_buffer*
l2cap_con_req(uint8 _ident, uint16 _psm, uint16 _scid)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _con_req));
	if ((_m) == NULL)
		return NULL;						

	NetBufferPrepend<struct _con_req> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}
	
	bufferHeader->hdr.code = L2CAP_CON_REQ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.psm = htole16((_psm));
	bufferHeader->param.scid = htole16((_scid));

	bufferHeader.Sync();

	return _m;
}


/* L2CAP_ConnectRsp */
static inline net_buffer*
l2cap_con_rsp(uint8 _ident, uint16 _dcid, uint16 _scid, uint16 _result, uint16 _status)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _con_rsp));
	if ((_m) == NULL)
		return NULL;						

	NetBufferPrepend<struct _con_rsp> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

   	bufferHeader->hdr.code = L2CAP_CON_RSP;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.dcid = htole16((_dcid));
	bufferHeader->param.scid = htole16((_scid));
	bufferHeader->param.result = htole16((_result));
	bufferHeader->param.status = htole16((_status)); /* reason */

	bufferHeader.Sync();

	return _m;
}


/* L2CAP_ConfigReq */
static inline net_buffer*
l2cap_cfg_req(uint8 _ident, uint16 _dcid, uint16 _flags, net_buffer* _data)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _cfg_req));
	if ((_m) == NULL){
		/* TODO free the _data buffer? */
		return NULL;	
	}

	(_m)->size = sizeof(struct _cfg_req);	  /* check if needed */

	NetBufferPrepend<struct _cfg_req> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_CFG_REQ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.dcid = htole16((_dcid));
	bufferHeader->param.flags = htole16((_flags));

	bufferHeader.Sync();

	/* Add the given data */
	gBufferModule->merge(_m, _data, true);

	return _m;
}


/* L2CAP_ConfigRsp */
static inline net_buffer*
l2cap_cfg_rsp(uint8 _ident, uint16 _scid, uint16 _flags, uint16 _result, net_buffer* _data)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _cfg_rsp));
	if ((_m) == NULL){
		/* TODO free the _data buffer */
		return NULL;	
	}

	NetBufferPrepend<struct _cfg_rsp> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_CFG_RSP;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.scid = htole16((_scid));
	bufferHeader->param.flags = htole16((_flags));
	bufferHeader->param.result = htole16((_result));

	bufferHeader.Sync();

	gBufferModule->merge(_m, _data, true);
	
	return _m;

}


/* L2CAP_DisconnectReq */
static inline net_buffer*
l2cap_discon_req(uint8 _ident, uint16 _dcid, uint16 _scid)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _discon_req));
	if ((_m) == NULL){
		return NULL;	
	}

	NetBufferPrepend<struct _discon_req> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_DISCON_REQ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.dcid = htole16((_dcid));
	bufferHeader->param.scid = htole16((_scid));

	bufferHeader.Sync();

	return _m;
}


/* L2CA_DisconnectRsp */
static inline net_buffer*
l2cap_discon_rsp(uint8 _ident, uint16 _dcid, uint16 _scid)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _discon_rsp));
	if ((_m) == NULL){
		return NULL;	
	}

	NetBufferPrepend<struct _discon_rsp> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_DISCON_RSP;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.dcid = htole16((_dcid));
	bufferHeader->param.scid = htole16((_scid));

	bufferHeader.Sync();

	return _m;
}


/* L2CAP_EchoReq */
static inline net_buffer*
l2cap_echo_req(uint8 _ident, void* _data, size_t _size)
{
	net_buffer* _m = gBufferModule->create(sizeof(l2cap_cmd_hdr_t));
	if ((_m) == NULL){
		/* TODO free the _data buffer */
		return NULL;	
	}

	NetBufferPrepend<l2cap_cmd_hdr_t> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->code = L2CAP_ECHO_REQ;
	bufferHeader->ident = (_ident);
	bufferHeader->length = htole16(0);

	bufferHeader.Sync();	

	if ((_data) != NULL) {
	   	gBufferModule->append(_m, _data, _size);
	}

	return _m;
}


/* L2CAP_InfoReq */
static inline net_buffer*
l2cap_info_req(uint8 _ident, uint16 _type)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _info_req));
	if ((_m) == NULL){
		return NULL;	
	}

	NetBufferPrepend<struct _info_req> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_INFO_REQ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = htole16(sizeof(bufferHeader->param));

	bufferHeader->param.type = htole16((_type));

	bufferHeader.Sync();

	return _m;
}


/* L2CAP_InfoRsp */
static inline net_buffer*
l2cap_info_rsp(uint8 _ident, uint16 _type, uint16 _result, uint16 _mtu)
{

	net_buffer* _m = gBufferModule->create(sizeof(struct _info_rsp));
	if ((_m) == NULL){
		return NULL;	
	}

	NetBufferPrepend<struct _info_rsp> bufferHeader(_m);
	status_t status = bufferHeader.Status();
	if (status < B_OK) {
		/* TODO free the buffer */
		return NULL;
	}

	bufferHeader->hdr.code = L2CAP_INFO_REQ;
	bufferHeader->hdr.ident = (_ident);
	bufferHeader->hdr.length = sizeof(bufferHeader->param);

	bufferHeader->param.type = htole16((_type));
	bufferHeader->param.result = htole16((_result));

	if ((_result) == L2CAP_SUCCESS) {
		switch ((_type)) {
		case L2CAP_CONNLESS_MTU:
			bufferHeader->data.mtu.mtu = htole16((_mtu));
			bufferHeader->hdr.length += sizeof((bufferHeader->data.mtu.mtu));
			break;				
		}						   
	}							   
									
	(_m)->size = sizeof(bufferHeader->hdr) + bufferHeader->hdr.length;

	bufferHeader->hdr.length = htole16(bufferHeader->hdr.length);

	bufferHeader.Sync();

	return _m;

}

#if 0
#pragma mark -
#endif


/* Build configuration options  */
static inline net_buffer*
l2cap_build_cfg_options(uint16* _mtu, uint16* _flush_timo, l2cap_flow_t* _flow)
{
	//TODO:

	return NULL;
}

