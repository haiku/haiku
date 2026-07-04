/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _L2CAP_COMMAND_H_
#define _L2CAP_COMMAND_H_

#include <ByteOrder.h>
#include <NetBufferUtilities.h>

#include <bluetooth/l2cap.h>


#define htole16(x) B_HOST_TO_LENDIAN_INT16(x)
#define le16toh(x) B_LENDIAN_TO_HOST_INT16(x)
#define htole32(x) B_HOST_TO_LENDIAN_INT16(x)
#define le32toh(x) B_LENDIAN_TO_HOST_INT32(x)


net_buffer*	make_l2cap_command_reject(uint8& code,
	uint16 reason, uint16 mtu, uint16 scid, uint16 dcid);

net_buffer*	make_l2cap_connection_req(uint8& code, uint16 psm, uint16 scid);
net_buffer*	make_l2cap_connection_rsp(uint8& code,
	uint16 dcid, uint16 scid, uint16 result, uint16 status);

net_buffer*	make_l2cap_configuration_req(uint8& code, uint16 dcid, uint16 flags,
	uint16* mtu, uint16* flush_timeout, l2cap_qos* flow);
net_buffer*	make_l2cap_configuration_rsp(uint8& code, uint16 scid, uint16 flags,
	uint16 result, net_buffer* opt);

net_buffer*	make_l2cap_disconnection_req(uint8& code, uint16 dcid, uint16 scid);
net_buffer*	make_l2cap_disconnection_rsp(uint8& code, uint16 dcid, uint16 scid);

net_buffer*	make_l2cap_information_req(uint8& code, uint16 type);
net_buffer*	make_l2cap_information_rsp(uint8& code, uint16 type, uint16 result, uint16 mtu);


#endif /* _L2CAP_COMMAND_H_ */
