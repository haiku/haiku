/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _L2CAP_CMDS_H_
#define _L2CAP_CMDS_H_

#include <NetBufferUtilities.h>

#include <l2cap.h>


extern net_buffer_module_info *gBufferModule;

net_buffer*
l2cap_cmd_rej(uint8 _ident, uint16 _reason, uint16 _mtu, uint16 _scid, uint16 _dcid);

net_buffer*
l2cap_con_req(uint8 _ident, uint16 _psm, uint16 _scid);

net_buffer*
l2cap_con_rsp(uint8 _ident, uint16 _dcid, uint16 _scid, uint16 _result, uint16 _status);

net_buffer*
l2cap_cfg_req(uint8 _ident, uint16 _dcid, uint16 _flags, net_buffer* _data);

net_buffer*
l2cap_cfg_rsp(uint8 _ident, uint16 _scid, uint16 _flags, uint16 _result, net_buffer* _data);

net_buffer*
l2cap_build_cfg_options(uint16* _mtu, uint16* _flush_timo, l2cap_flow_t* _flow);

net_buffer*
l2cap_discon_req(uint8 _ident, uint16 _dcid, uint16 _scid);

net_buffer*
l2cap_discon_rsp(uint8 _ident, uint16 _dcid, uint16 _scid);

net_buffer*
l2cap_echo_req(uint8 _ident, void* _data, size_t _size);

net_buffer*
l2cap_info_req(uint8 _ident, uint16 _type);

net_buffer*
l2cap_info_rsp(uint8 _ident, uint16 _type, uint16 _result, uint16 _mtu);

#endif
