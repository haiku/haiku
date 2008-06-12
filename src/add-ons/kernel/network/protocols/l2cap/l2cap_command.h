/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _L2CAP_CMDS_H_
#define _L2CAP_CMDS_H_

//net
#include <net_buffer.h>

//bt
#include "l2cap.h"


extern net_buffer_module_info *gBufferModule;

static inline net_buffer*
l2cap_cmd_rej(uint8 _ident, uint16 _reason, uint16 _mtu, uint16 _scid, uint16 _dcid);

static inline net_buffer*
l2cap_con_req(uint8 _ident, uint16 _psm, uint16 _scid);

static inline net_buffer*
l2cap_con_rsp(uint8 _ident, uint16 _dcid, uint16 _scid, uint16 _result, uint16 _status);

static inline net_buffer*
l2cap_cfg_req(uint8 _ident, uint16 _dcid, uint16 _flags, net_buffer* _data);

static inline net_buffer*
l2cap_cfg_rsp(uint8 _ident, uint16 _scid, uint16 _flags, uint16 _result, net_buffer* _data);

static inline net_buffer*
l2cap_discon_req(uint8 _ident, uint16 _dcid, uint16 _scid);

static inline net_buffer*
l2cap_discon_rsp(uint8 _ident, uint16 _dcid, uint16 _scid);

static inline net_buffer*
l2cap_echo_req(uint8 _ident, void* _data, size_t _size);

static inline net_buffer*
l2cap_info_req(uint8 _ident, uint16 _type);

static inline net_buffer*
l2cap_info_rsp(uint8 _ident, uint16 _type, uint16 _result, uint16 _mtu);

#endif /* L2CAP_CMDS_H_ */

