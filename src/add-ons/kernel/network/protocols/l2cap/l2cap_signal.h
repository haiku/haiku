/* 
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_SIGNAL_H
#define L2CAP_SIGNAL_H

// Processing signals
status_t l2cap_process_signal_cmd(HciConnection* conn, net_buffer* buffer);

#if 0
#pragma - Signals Responses
#endif

status_t
send_l2cap_reject(HciConnection *conn, uint8 ident, uint16 reason, uint16 mtu, uint16 scid, uint16 dcid);
status_t
send_l2cap_con_rej(HciConnection *conn, uint8 ident, uint16 scid, uint16 dcid, uint16 result);
status_t
send_l2cap_cfg_rsp(HciConnection *conn, uint8 ident, uint16 scid, uint16 result, net_buffer *opt);

#endif
