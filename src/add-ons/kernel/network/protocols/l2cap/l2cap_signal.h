/* 
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_SIGNAL_H
#define L2CAP_SIGNAL_H

#include <l2cap.h>


status_t l2cap_handle_signaling_command(struct HciConnection* connection, net_buffer* buffer);


status_t send_l2cap_command(HciConnection* conn, uint8 code, uint8 ident, net_buffer* command);

status_t send_l2cap_command_reject(HciConnection* conn, uint8 ident,
	uint16 reason, uint16 mtu, uint16 scid, uint16 dcid);
status_t send_l2cap_configuration_req(HciConnection* conn, uint8 ident, uint16 dcid, uint16 flags,
	uint16* mtu, uint16* flush_timeout, l2cap_qos* flow);
status_t send_l2cap_connection_req(HciConnection* conn, uint8 ident, uint16 psm, uint16 scid);
status_t send_l2cap_connection_rsp(HciConnection* conn, uint8 ident,
	uint16 dcid, uint16 scid, uint16 result, uint16 status);
status_t send_l2cap_configuration_rsp(HciConnection* conn, uint8 ident,
	uint16 scid, uint16 flags, uint16 result, net_buffer *opt);
status_t send_l2cap_disconnection_req(HciConnection* conn, uint8 ident, uint16 dcid, uint16 scid);
status_t send_l2cap_disconnection_rsp(HciConnection* conn, uint8 ident, uint16 dcid, uint16 scid);


#endif /* L2CAP_SIGNAL_H */
