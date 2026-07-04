/*
 * Copyright 2007 Oliver Ruiz Dorantes. All rights reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "l2cap_command.h"

#include <NetBufferUtilities.h>


net_buffer*
make_l2cap_command_reject(uint8& code, uint16 reason, uint16 mtu, uint16 scid, uint16 dcid)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	if (reason == l2cap_command_reject::REJECTED_MTU_EXCEEDED) {
		NetBufferPrepend<uint16> data(buffer.Get());
		*data = htole16(mtu);
	} else if (reason == l2cap_command_reject::REJECTED_INVALID_CID) {
		NetBufferPrepend<l2cap_command_reject_data> data(buffer.Get());
		data->invalid_cid.scid = htole16(scid);
		data->invalid_cid.dcid = htole16(dcid);
	}

	NetBufferPrepend<l2cap_command_reject> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_COMMAND_REJECT_RSP;
	command->reason = (uint16)htole16(reason);

	return buffer.Detach();
}


net_buffer*
make_l2cap_connection_req(uint8& code, uint16 psm, uint16 scid)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_connection_req> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_CONNECTION_REQ;
	command->psm = htole16(psm);
	command->scid = htole16(scid);

	return buffer.Detach();
}


net_buffer*
make_l2cap_connection_rsp(uint8& code, uint16 dcid, uint16 scid, uint16 result, uint16 status)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_connection_rsp> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_CONNECTION_RSP;
	command->dcid = htole16(dcid);
	command->scid = htole16(scid);
	command->result = htole16(result);
	command->status = htole16(status);

	return buffer.Detach();
}


net_buffer*
make_l2cap_configuration_req(uint8& code, uint16 dcid, uint16 flags,
	uint16* mtu, uint16* flush_timeout, l2cap_qos* flow)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	if (mtu != NULL) {
		struct config_option_mtu {
			l2cap_configuration_option header;
			uint16 value;
		} _PACKED;
		NetBufferPrepend<config_option_mtu> option(buffer.Get());
		if (option.Status() != B_OK)
			return NULL;

		option->header.type = l2cap_configuration_option::OPTION_MTU;
		option->header.length = sizeof(option->value);
		option->value = htole16(*mtu);
	}

	if (flush_timeout != NULL) {
		struct config_option_flush_timeout {
			l2cap_configuration_option header;
			uint16 value;
		} _PACKED;
		NetBufferPrepend<config_option_flush_timeout> option(buffer.Get());
		if (option.Status() != B_OK)
			return NULL;

		option->header.type = l2cap_configuration_option::OPTION_FLUSH_TIMEOUT;
		option->header.length = sizeof(option->value);
		option->value = htole16(*flush_timeout);
	}

	if (flow != NULL) {
		struct config_option_flow {
			l2cap_configuration_option header;
			l2cap_qos value;
		} _PACKED;
		NetBufferPrepend<config_option_flow> option(buffer.Get());
		if (option.Status() != B_OK)
			return NULL;

		option->header.type = l2cap_configuration_option::OPTION_QOS;
		option->header.length = sizeof(option->value);
		option->value.flags = flow->flags;
		option->value.service_type = flow->service_type;
		option->value.token_rate = htole32(flow->token_rate);
		option->value.token_bucket_size = htole32(flow->token_bucket_size);
		option->value.peak_bandwidth = htole32(flow->peak_bandwidth);
		option->value.access_latency = htole32(flow->access_latency);
		option->value.delay_variation = htole32(flow->delay_variation);
	}

	NetBufferPrepend<l2cap_configuration_req> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_CONFIGURATION_REQ;
	command->dcid = htole16(dcid);
	command->flags = htole16(flags);

	return buffer.Detach();
}


net_buffer*
make_l2cap_configuration_rsp(uint8& code, uint16 scid, uint16 flags,
	uint16 result, net_buffer* opt)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_configuration_rsp> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_CONFIGURATION_RSP;
	command->scid = htole16(scid);
	command->flags = htole16(flags);
	command->result = htole16(result);

	if (opt != NULL) {
		if (gBufferModule->append_cloned(buffer.Get(), opt, 0, opt->size) != B_OK)
			return NULL;
	}

	return buffer.Detach();
}


net_buffer*
make_l2cap_disconnection_req(uint8& code, uint16 dcid, uint16 scid)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_disconnection_req> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_DISCONNECTION_REQ;
	command->dcid = htole16(dcid);
	command->scid = htole16(scid);

	return buffer.Detach();
}


net_buffer*
make_l2cap_disconnection_rsp(uint8& code, uint16 dcid, uint16 scid)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_disconnection_rsp> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_DISCONNECTION_RSP;
	command->dcid = htole16(dcid);
	command->scid = htole16(scid);

	return buffer.Detach();
}


net_buffer*
make_l2cap_information_req(uint8& code, uint16 type)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_information_req> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_INFORMATION_REQ;

	command->type = htole16(type);

	return buffer.Detach();
}


net_buffer*
make_l2cap_information_rsp(uint8& code, uint16 type, uint16 result, uint16 _mtu)
{
	NetBufferDeleter<> buffer(gBufferModule->create(128));
	if (!buffer.IsSet())
		return NULL;

	NetBufferPrepend<l2cap_information_rsp> command(buffer.Get());
	if (command.Status() != B_OK)
		return NULL;

	code = L2CAP_INFORMATION_RSP;

	command->type = htole16(type);
	command->result = htole16(result);

	if (result == l2cap_information_rsp::RESULT_SUCCESS) {
		switch (type) {
		case l2cap_information_req::TYPE_CONNECTIONLESS_MTU: {
			uint16 mtu = htole16(_mtu);
			gBufferModule->append(buffer.Get(), &mtu, sizeof(mtu));
			break;
		}
		}
	}

	return buffer.Detach();
}
