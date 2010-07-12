/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
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
#include <KernelExport.h>
#include <string.h>

#include <NetBufferUtilities.h>


#include <l2cap.h>
#include "l2cap_internal.h"
#include "l2cap_signal.h"
#include "l2cap_command.h"
#include "l2cap_upper.h"
#include "l2cap_lower.h"

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "signal"
#define SUBMODULE_COLOR 36
#include <btDebug.h>

#include <bluetooth/HCI/btHCI_command.h>

typedef enum _option_status {
	OPTION_NOT_PRESENT = 0,
	OPTION_PRESENT = 1,
	HEADER_TOO_SHORT = -1,
	BAD_OPTION_LENGTH = -2,
	OPTION_UNKNOWN = -3
} option_status;


l2cap_flow_t default_qos = {
		/* flags */ 0x0,
		/* service_type */ HCI_SERVICE_TYPE_BEST_EFFORT,
		/* token_rate */ 0xffffffff, /* maximum */
		/* token_bucket_size */ 0xffffffff, /* maximum */
		/* peak_bandwidth */ 0x00000000, /* maximum */
		/* latency */ 0xffffffff, /* don't care */
		/* delay_variation */ 0xffffffff  /* don't care */
};


static status_t
l2cap_process_con_req(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_con_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_cfg_req(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_cfg_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_discon_req(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_discon_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_echo_req(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_echo_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_info_req(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_info_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer);

static status_t
l2cap_process_cmd_rej(HciConnection* conn, uint8 ident, net_buffer* buffer);


/*
 * Process L2CAP signaling command. We already know that destination channel ID
 * is 0x1 that means we have received signaling command from peer's L2CAP layer.
 * So get command header, decode and process it.
 *
 * XXX do we need to check signaling MTU here?
 */

status_t
l2cap_process_signal_cmd(HciConnection* conn, net_buffer* buffer)
{
	net_buffer* m = buffer;

	debugf("Signal size=%ld\n", buffer->size);

	while (m != NULL) {

		/* Verify packet length */
		if (buffer->size < sizeof(l2cap_cmd_hdr_t)) {
			debugf("small L2CAP signaling command len=%ld\n", buffer->size);
			gBufferModule->free(buffer);
			return EMSGSIZE;
		}

		/* Get COMMAND header */
		NetBufferHeaderReader<l2cap_cmd_hdr_t> commandHeader(buffer);
		status_t status = commandHeader.Status();
		if (status < B_OK) {
			return ENOBUFS;
		}

		uint8 processingCode = commandHeader->code;
		uint8 processingIdent = commandHeader->ident;
		uint16 processingLength = le16toh(commandHeader->length);

		/* Verify command length */
		if (buffer->size < processingLength) {
			debugf("invalid L2CAP signaling command, code=%#x, ident=%d,"
				" length=%d, buffer size=%ld\n", processingCode,
				processingIdent, processingLength, buffer->size);
			gBufferModule->free(buffer);
			return (EMSGSIZE);
		}


		commandHeader.Remove(); // pulling the header of the command

		/* Process command processors responsible to delete the command*/
		switch (processingCode) {
			case L2CAP_CMD_REJ:
				l2cap_process_cmd_rej(conn, processingIdent, buffer);
				break;

			case L2CAP_CON_REQ:
				l2cap_process_con_req(conn, processingIdent, buffer);
				break;

			case L2CAP_CON_RSP:
				l2cap_process_con_rsp(conn, processingIdent, buffer);
				break;

			case L2CAP_CFG_REQ:
				l2cap_process_cfg_req(conn, processingIdent, buffer);
				break;

			case L2CAP_CFG_RSP:
				l2cap_process_cfg_rsp(conn, processingIdent, buffer);
				break;

			case L2CAP_DISCON_REQ:
				l2cap_process_discon_req(conn, processingIdent, buffer);
				break;

			case L2CAP_DISCON_RSP:
				l2cap_process_discon_rsp(conn, processingIdent, buffer);
				break;

			case L2CAP_ECHO_REQ:
				l2cap_process_echo_req(conn, processingIdent, buffer);
				break;

			case L2CAP_ECHO_RSP:
				l2cap_process_echo_rsp(conn, processingIdent, buffer);
				break;

			case L2CAP_INFO_REQ:
				l2cap_process_info_req(conn, processingIdent, buffer);
				break;

			case L2CAP_INFO_RSP:
				l2cap_process_info_rsp(conn, processingIdent, buffer);
				break;

			default:
				debugf("unknown L2CAP signaling command, code=%#x, ident=%d\n",
						processingCode, processingIdent);

				// Send L2CAP_CommandRej. Do not really care about the result
				// ORD: Remiaining commands in the same packet are also to
				// be rejected?
				// send_l2cap_reject(con, processingIdent,
				// L2CAP_REJ_NOT_UNDERSTOOD, 0, 0, 0);
				gBufferModule->free(m);
				break;
		}

		// Is there still remaining size? processors should have pulled its content...
		if (m->size == 0) {
			// free the buffer
			gBufferModule->free(m);
			m = NULL;
		}
	}

	return B_OK;
}


#if 0
#pragma mark - Processing Incoming signals
#endif


/* Process L2CAP_ConnectReq command */
static status_t
l2cap_process_con_req(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capChannel* channel;
	uint16 dcid, psm;

	/* Get con req data */
	NetBufferHeaderReader<l2cap_con_req_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	psm = le16toh(command->psm);
	dcid = le16toh(command->scid);

	command.Remove(); // pull the command body

	// Create new channel and send L2CA_ConnectInd
	// notification to the upper layer protocol.
	channel = btCoreData->AddChannel(conn, psm /*, dcid, ident*/);
	if (channel == NULL) {
		flowf("No resources to create channel\n");
		return (send_l2cap_con_rej(conn, ident, 0, dcid, L2CAP_NO_RESOURCES));
	} else {
		debugf("New channel created scid=%d\n", channel->scid);
	}

	channel->dcid = dcid;
	channel->ident = ident;

	status_t indicationStatus = l2cap_l2ca_con_ind(channel);

	if ( indicationStatus == B_OK ) {
		// channel->state = L2CAP_CHAN_W4_L2CA_CON_RSP;

	} else if (indicationStatus == B_NO_MEMORY) {
		/* send_l2cap_con_rej(con, ident, channel->scid, dcid, L2CAP_NO_RESOURCES);*/
		btCoreData->RemoveChannel(conn, channel->scid);

	} else {
		send_l2cap_con_rej(conn, ident, channel->scid, dcid, L2CAP_PSM_NOT_SUPPORTED);
		btCoreData->RemoveChannel(conn, channel->scid);
	}

	return (indicationStatus);
}


static status_t
l2cap_process_con_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame* cmd = NULL;
	uint16 scid, dcid, result, status;
	status_t error = 0;

	/* Get command parameters */
	NetBufferHeaderReader<l2cap_con_rsp_cp> command(buffer);
	if (command.Status() < B_OK) {
		return ENOBUFS;
	}

	dcid = le16toh(command->dcid);
	scid = le16toh(command->scid);
	result = le16toh(command->result);
	status = le16toh(command->status);

	command.Remove(); // pull the command body

	debugf("dcid=%d scid=%d result=%d status%d\n", dcid, scid, result, status);

	/* Check if we have pending command descriptor */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd == NULL) {
		debugf("unexpected L2CAP_ConnectRsp command. ident=%d, "
		"con_handle=%d\n", ident, conn->handle);
		return ENOENT;
	}

	/* Verify channel state, if invalid - do nothing */
	if (cmd->channel->state != L2CAP_CHAN_W4_L2CAP_CON_RSP) {
		debugf("unexpected L2CAP_ConnectRsp. Invalid channel state, "
		"cid=%d, state=%d\n", scid,	cmd->channel->state);
		goto reject;
	}

	/* Verify CIDs and send reject if does not match */
	if (cmd->channel->scid != scid) {
		debugf("unexpected L2CAP_ConnectRsp. Channel IDs do not match, "
		"scid=%d(%d)\n", cmd->channel->scid, scid);
		goto reject;
	}

	/*
	 * Looks good. We got confirmation from our peer. Now process
	 * it. First disable RTX timer. Then check the result and send
	 * notification to the upper layer. If command timeout already
	 * happened then ignore response.
	 */

	if ((error = btCoreData->UnTimeoutSignal(cmd)) != 0)
		return error;

	if (result == L2CAP_PENDING) {
		/*
		 * Our peer wants more time to complete connection. We shall
		 * start ERTX timer and wait. Keep command in the list.
		 */

		cmd->channel->dcid = dcid;
		btCoreData->TimeoutSignal(cmd, bluetooth_l2cap_ertx_timeout);

		// TODO:
		// INDICATION error = ng_l2cap_l2ca_con_rsp(cmd->channel, cmd->token,
		// result, status);
		// if (error != B_OK)
		// btCoreData->RemoveChannel(conn, cmd->channel->scid);

	} else {

		if (result == L2CAP_SUCCESS) {
			/*
			 * Channel is open. Complete command and move to CONFIG
			 * state. Since we have sent positive confirmation we
			 * expect to receive L2CA_Config request from the upper
			 * layer protocol.
			 */

			cmd->channel->dcid = dcid;
			cmd->channel->state = L2CAP_CHAN_CONFIG;
		}

		error = l2cap_con_rsp_ind(conn, cmd->channel);

		/* XXX do we have to remove the channel on error? */
		if (error != 0 || result != L2CAP_SUCCESS) {
			debugf("failed to open L2CAP channel, result=%d, status=%d\n",
				result, status);
			btCoreData->RemoveChannel(conn, cmd->channel->scid);
		}

		btCoreData->AcknowledgeSignal(cmd);
	}

	return error;

reject:
	/* Send reject. Do not really care about the result */
	send_l2cap_reject(conn, ident, L2CAP_REJ_INVALID_CID, 0, scid, dcid);

	return 0;
}


static option_status
getNextSignalOption(net_buffer* nbuf, size_t* off, l2cap_cfg_opt_t* hdr,
	l2cap_cfg_opt_val_t* val)
{
	int hint;
	size_t len = nbuf->size - (*off);

	if (len == 0)
		return OPTION_NOT_PRESENT;
	if (len < 0 || len < sizeof(*hdr))
		return HEADER_TOO_SHORT;

	gBufferModule->read(nbuf, *off, hdr, sizeof(*hdr));
	*off += sizeof(*hdr);
	len  -= sizeof(*hdr);

	hint = L2CAP_OPT_HINT(hdr->type);
	hdr->type &= L2CAP_OPT_HINT_MASK;

	switch (hdr->type) {
		case L2CAP_OPT_MTU:
			if (hdr->length != L2CAP_OPT_MTU_SIZE || len < hdr->length)
				return BAD_OPTION_LENGTH;

			gBufferModule->read(nbuf, *off, val, L2CAP_OPT_MTU_SIZE);
			val->mtu = le16toh(val->mtu);
			*off += L2CAP_OPT_MTU_SIZE;
			debugf("mtu %d specified\n", val->mtu);
		break;

		case L2CAP_OPT_FLUSH_TIMO:
			if (hdr->length != L2CAP_OPT_FLUSH_TIMO_SIZE || len < hdr->length)
				return BAD_OPTION_LENGTH;

			gBufferModule->read(nbuf, *off, val, L2CAP_OPT_FLUSH_TIMO_SIZE);
			val->flush_timo = le16toh(val->flush_timo);
			flowf("flush specified\n");
			*off += L2CAP_OPT_FLUSH_TIMO_SIZE;
		break;

		case L2CAP_OPT_QOS:
			if (hdr->length != L2CAP_OPT_QOS_SIZE || len < hdr->length)
				return BAD_OPTION_LENGTH;

			gBufferModule->read(nbuf, *off, val, L2CAP_OPT_QOS_SIZE);
			val->flow.token_rate = le32toh(val->flow.token_rate);
			val->flow.token_bucket_size =	le32toh(val->flow.token_bucket_size);
			val->flow.peak_bandwidth = le32toh(val->flow.peak_bandwidth);
			val->flow.latency = le32toh(val->flow.latency);
			val->flow.delay_variation = le32toh(val->flow.delay_variation);
			*off += L2CAP_OPT_QOS_SIZE;
			flowf("qos specified\n");
		break;

		default:
			if (hint)
				*off += hdr->length;
			else
				return OPTION_UNKNOWN;
		break;
	}

	return OPTION_PRESENT;
}


static status_t
l2cap_process_cfg_req(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capChannel* channel = NULL;
	uint16 dcid;
	uint16 respond;
	uint16 result;

	l2cap_cfg_opt_t hdr;
	l2cap_cfg_opt_val_t val;

	size_t off;
	status_t error = 0;

	debugf("configuration=%ld\n", buffer->size);

	/* Get command parameters */
	NetBufferHeaderReader<l2cap_cfg_req_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	dcid = le16toh(command->dcid);
	respond = L2CAP_OPT_CFLAG(le16toh(command->flags));

	command.Remove(); // pull configuration header

	/* Check if we have this channel and it is in valid state */
	channel = btCoreData->ChannelBySourceID(conn, dcid);
	if (channel == NULL) {
		debugf("unexpected L2CAP_ConfigReq command. "
		"Channel does not exist, cid=%d\n", dcid);
		goto reject;
	}

	/* Verify channel state */
	if (channel->state != L2CAP_CHAN_CONFIG
		&& channel->state != L2CAP_CHAN_OPEN) {
		debugf("unexpected L2CAP_ConfigReq. Invalid channel state, "
		"cid=%d, state=%d\n", dcid, channel->state);
		goto reject;
	}

	if (channel->state == L2CAP_CHAN_OPEN) { /* Re-configuration */
		channel->cfgState = 0; // Reset configuration state
		channel->state = L2CAP_CHAN_CONFIG;
	}

	for (result = 0, off = 0; ; ) {
		error = getNextSignalOption(buffer, &off, &hdr, &val);

		if (error == OPTION_NOT_PRESENT) { /* We done with this packet */
			// TODO: configurations should have been pulled
			break;
		} else if (error > 0) { /* Got option */
			switch (hdr.type) {
			case L2CAP_OPT_MTU:
				channel->configuration->omtu = val.mtu;
				break;

			case L2CAP_OPT_FLUSH_TIMO:
				channel->configuration->flush_timo = val.flush_timo;
				break;

			case L2CAP_OPT_QOS:
				memcpy(&val.flow, &channel->configuration->iflow,
					sizeof(channel->configuration->iflow));
				break;

			default: /* Ignore unknown hint option */
				break;
			}
		} else { /* Oops, something is wrong */
			respond = 1;
			if (error == OPTION_UNKNOWN) {
				// TODO: Remote to get the next possible option
				result = L2CAP_UNKNOWN_OPTION;
			} else {
				/* XXX FIXME Send other reject codes? */
				gBufferModule->free(buffer);
				result = L2CAP_REJECT;
			}

			break;
		}
	}

	debugf("Pulled %ld of configuration fields respond=%d remaining=%ld\n",
		off, respond, buffer->size);
	gBufferModule->remove_header(buffer, off);


	/*
	 * Now check and see if we have to respond. If everything was OK then
	 * respond contain "C flag" and (if set) we will respond with empty
	 * packet and will wait for more options.
	 *
	 * Other case is that we did not like peer's options and will respond
	 * with L2CAP_Config response command with Reject error code.
	 *
	 * When "respond == 0" than we have received all options and we will
	 * sent L2CA_ConfigInd event to the upper layer protocol.
	 */

	if (respond) {
		flowf("Refusing cfg\n");
		error = send_l2cap_cfg_rsp(conn, ident, channel->dcid, result, buffer);
		if (error != 0) {
			// TODO:
			// INDICATION ng_l2cap_l2ca_discon_ind(ch);
			btCoreData->RemoveChannel(conn, channel->scid);
		}
	} else {
		/* Send L2CA_ConfigInd event to the upper layer protocol */
		channel->cfgState |= L2CAP_CFG_IN;
		channel->ident = ident; // sent ident to reply
		error = l2cap_cfg_req_ind(channel);
		if (error != 0)
			btCoreData->RemoveChannel(conn, channel->scid);
	}

	return error;

reject:
	/* Send reject. Do not really care about the result */
	gBufferModule->free(buffer);

	send_l2cap_reject(conn, ident, L2CAP_REJ_INVALID_CID, 0, 0, dcid);

	return B_OK;
}


/* Process L2CAP_ConfigRsp command */
static status_t
l2cap_process_cfg_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame* cmd = NULL;
	uint16 scid, cflag, result;

	l2cap_cfg_opt_t hdr;
	l2cap_cfg_opt_val_t val;

	size_t off;
	status_t error = 0;

	NetBufferHeaderReader<l2cap_cfg_rsp_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	scid = le16toh(command->scid);
	cflag = L2CAP_OPT_CFLAG(le16toh(command->flags));
	result = le16toh(command->result);

	command.Remove();

	debugf("scid=%d cflag=%d result=%d\n", scid, cflag, result);

	/* Check if we have this command */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd == NULL) {
		debugf("unexpected L2CAP_ConfigRsp command. ident=%d, con_handle=%d\n",
			ident, conn->handle);
		gBufferModule->free(buffer);
		return ENOENT;
	}

	/* Verify CIDs and send reject if does not match */
	if (cmd->channel->scid != scid) {
		debugf("unexpected L2CAP_ConfigRsp.Channel ID does not match, "
		"scid=%d(%d)\n", cmd->channel->scid, scid);
		goto reject;
	}

	/* Verify channel state and reject if invalid */
	if (cmd->channel->state != L2CAP_CHAN_CONFIG) {
		debugf("unexpected L2CAP_ConfigRsp. Invalid channel state, scid=%d, "
		"state=%d\n", cmd->channel->scid, cmd->channel->state);
		goto reject;
	}

	/*
	 * Looks like it is our response, so process it. First parse options,
	 * then verify C flag. If it is set then we shall expect more
	 * configuration options from the peer and we will wait. Otherwise we
	 * have received all options and we will send L2CA_ConfigRsp event to
	 * the upper layer protocol. If command timeout already happened then
	 * ignore response.
	 */

	if ((error = btCoreData->UnTimeoutSignal(cmd)) != 0) {
		gBufferModule->free(buffer);
		return error;
	}

	for (off = 0; ; ) {
		error = getNextSignalOption(buffer, &off, &hdr, &val);
		// TODO: pull the option

		if (error == OPTION_NOT_PRESENT) /* We done with this packet */
			break;
		else if (error > 0) { /* Got option */
			switch (hdr.type) {
			case L2CAP_OPT_MTU:
				cmd->channel->configuration->imtu = val.mtu;
			break;

			case L2CAP_OPT_FLUSH_TIMO:
				cmd->channel->configuration->flush_timo = val.flush_timo;
				break;

			case L2CAP_OPT_QOS:
				memcpy(&val.flow, &cmd->channel->configuration->oflow,
					sizeof(cmd->channel->configuration->oflow));
			break;

			default: /* Ignore unknown hint option */
				break;
			}
		} else {
			/*
			 * XXX FIXME What to do here?
			 *
			 * This is really BAD :( options packet was broken, or
			 * peer sent us option that we did not understand. Let
			 * upper layer know and do not wait for more options.
			 */

			debugf("fail parsing configuration options, error=%ld\n", error);
			// INDICATION
			result = L2CAP_UNKNOWN;
			cflag = 0;

			break;
		}
	}

	if (cflag) /* Restart timer and wait for more options */
		btCoreData->TimeoutSignal(cmd, bluetooth_l2cap_rtx_timeout);
	else {
		/* Send L2CA_Config response to the upper layer protocol */
		error = l2cap_cfg_rsp_ind(cmd->channel /*, cmd->token, result*/);
		if (error != 0) {
			/*
			 * XXX FIXME what to do here? we were not able to send
			 * response to the upper layer protocol, so for now
			 * just close the channel. Send L2CAP_Disconnect to
			 * remote peer?
			 */

			debugf("failed to send L2CA_Config response, error=%ld\n", error);

			btCoreData->RemoveChannel(conn, cmd->channel->scid);
		}

		btCoreData->AcknowledgeSignal(cmd);
	}

	return error;

reject:
	/* Send reject. Do not really care about the result */
	gBufferModule->free(buffer);
	send_l2cap_reject(conn, ident, L2CAP_REJ_INVALID_CID, 0, scid, 0);

	return B_OK;
}


/* Process L2CAP_DisconnectReq command */
static status_t
l2cap_process_discon_req(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capChannel* channel = NULL;
	L2capFrame* cmd = NULL;
	net_buffer* buff = NULL;
	uint16 scid;
	uint16 dcid;

	NetBufferHeaderReader<l2cap_discon_req_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	dcid = le16toh(command->dcid);
	scid = le16toh(command->scid);

	command.Remove();

	/* Check if we have this channel and it is in valid state */
	channel = btCoreData->ChannelBySourceID(conn, dcid);
	if (channel == NULL) {
		debugf("unexpected L2CAP_DisconnectReq message.Channel does not exist, "
			"cid=%x\n", dcid);
		goto reject;
	}

	/* XXX Verify channel state and reject if invalid -- is that true? */
	if (channel->state != L2CAP_CHAN_OPEN
		&& channel->state != L2CAP_CHAN_CONFIG
		&& channel->state != L2CAP_CHAN_W4_L2CAP_DISCON_RSP) {
		debugf("unexpected L2CAP_DisconnectReq. Invalid channel state, cid=%d, "
			"state=%d\n", dcid, channel->state);
		goto reject;
	}

	/* Match destination channel ID */
	if (channel->dcid != scid || channel->scid != dcid) {
		debugf("unexpected L2CAP_DisconnectReq. Channel IDs does not match, "
			"channel: scid=%d, dcid=%d, request: scid=%d, dcid=%d\n",
			channel->scid, channel->dcid, scid, dcid);
		goto reject;
	}

	/*
	 * Looks good, so notify upper layer protocol that channel is about
	 * to be disconnected and send L2CA_DisconnectInd message. Then respond
	 * with L2CAP_DisconnectRsp.
	 */

	flowf("Responding\n");

	// inform upper if we were not actually already waiting
	if (channel->state != L2CAP_CHAN_W4_L2CAP_DISCON_RSP) {
		l2cap_discon_req_ind(channel); // do not care about result
	}

	/* Send L2CAP_DisconnectRsp */
	buff = l2cap_discon_rsp(ident, dcid, scid);
	cmd = btCoreData->SpawnSignal(conn, channel, buff, ident, L2CAP_DISCON_RSP);
	if (cmd == NULL)
		return ENOMEM;

	/* Link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;

reject:
	flowf("Rejecting\n");
	/* Send reject. Do not really care about the result */
	send_l2cap_reject(conn, ident, L2CAP_REJ_INVALID_CID, 0, scid, dcid);

	return B_OK;
}


/* Process L2CAP_DisconnectRsp command */
static status_t
l2cap_process_discon_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame* cmd = NULL;
	int16 scid, dcid;
	status_t error = 0;

	/* Get command parameters */
	NetBufferHeaderReader<l2cap_discon_rsp_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	dcid = le16toh(command->dcid);
	scid = le16toh(command->scid);

	command.Remove();
	//? NG_FREE_M(con->rx_pkt);

	/* Check if we have pending command descriptor */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd == NULL) {
		debugf("unexpected L2CAP_DisconnectRsp command. ident=%d, "
			"con_handle=%d\n", ident, conn->handle);
		goto out;
	}

	/* Verify channel state, do nothing if invalid */
	if (cmd->channel->state != L2CAP_CHAN_W4_L2CA_DISCON_RSP) {
		debugf("unexpected L2CAP_DisconnectRsp. Invalid state, cid=%d, "
		"state=%d\n", scid, cmd->channel->state);
		goto out;
	}

	/* Verify CIDs and send reject if does not match */
	if (cmd->channel->scid != scid || cmd->channel->dcid != dcid) {
		debugf("unexpected L2CAP_DisconnectRsp. Channel IDs do not match, "
		"scid=%d(%d), dcid=%d(%d)\n", cmd->channel->scid, scid,
			cmd->channel->dcid, dcid);
		goto out;
	}

	/*
	* Looks like we have successfuly disconnected channel, so notify
	* upper layer. If command timeout already happened then ignore
	* response.
	*/

	if ((error = btCoreData->UnTimeoutSignal(cmd)) != 0)
		goto out;

	l2cap_discon_rsp_ind(cmd->channel/* results? */);
	btCoreData->RemoveChannel(conn, scid); /* this will free commands too */

out:
	return error;
}


static status_t
l2cap_process_echo_req(HciConnection *conn, uint8 ident, net_buffer *buffer)
{
	L2capFrame* cmd = NULL;

	cmd = btCoreData->SpawnSignal(conn, NULL, l2cap_echo_req(ident, NULL, 0),
		ident, L2CAP_ECHO_RSP);
	if (cmd == NULL) {
		gBufferModule->free(buffer);
		return ENOMEM;
	}

	/* Attach data and link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;
}


/* Process L2CAP_EchoRsp command */
static status_t
l2cap_process_echo_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame* cmd = NULL;
	status_t error = 0;

	/* Check if we have this command */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd != NULL) {
		/* If command timeout already happened then ignore response */
		if ((error = btCoreData->UnTimeoutSignal(cmd)) != 0) {
			return error;
		}

		// INDICATION error = ng_l2cap_l2ca_ping_rsp(cmd->conn, cmd->token,
		// L2CAP_SUCCESS, conn->rx_pkt);
		btCoreData->AcknowledgeSignal(cmd);

	} else {
		debugf("unexpected L2CAP_EchoRsp command. ident does not exist, "
		"ident=%d\n", ident);
		gBufferModule->free(buffer);
		error = B_ERROR;
	}

	return error;
}


/* Process L2CAP_InfoReq command */
static status_t
l2cap_process_info_req(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame* cmd = NULL;
	net_buffer*	buf = NULL;
	uint16 type;

	/* Get command parameters */
    NetBufferHeaderReader<l2cap_info_req_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	// ??
	// command->type = le16toh(mtod(conn->rx_pkt,
	// ng_l2cap_info_req_cp *)->type);
    type = le16toh(command->type);

	command.Remove();

	switch (type) {
	    case L2CAP_CONNLESS_MTU:
		    buf = l2cap_info_rsp(ident, L2CAP_CONNLESS_MTU, L2CAP_SUCCESS,
		    	L2CAP_MTU_DEFAULT);
		break;

	    default:
		    buf = l2cap_info_rsp(ident, type, L2CAP_NOT_SUPPORTED, 0);
		break;
	}

	cmd = btCoreData->SpawnSignal(conn, NULL, buf, ident, L2CAP_INFO_RSP);
	if (cmd == NULL)
		return ENOMEM;

	/* Link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;
}


/* Process L2CAP_InfoRsp command */
static status_t
l2cap_process_info_rsp(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	l2cap_info_rsp_cp* cp = NULL;
	L2capFrame* cmd = NULL;
	status_t error = B_OK;

	/* Get command parameters */
	NetBufferHeaderReader<l2cap_info_rsp_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	command->type = le16toh(command->type);
	command->result = le16toh(command->result);

	command.Remove();

	/* Check if we have pending command descriptor */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd == NULL) {
		debugf("unexpected L2CAP_InfoRsp command. Requested ident does not "
		"exist, ident=%d\n", ident);
		gBufferModule->free(buffer);
		return ENOENT;
	}

	/* If command timeout already happened then ignore response */
	if ((error = btCoreData->UnTimeoutSignal(cmd)) != 0) {
		gBufferModule->free(buffer);
		return error;
	}

	if (command->result == L2CAP_SUCCESS) {
		switch (command->type) {

    		case L2CAP_CONNLESS_MTU:
				/* TODO: Check specs ??
	    	    if (conn->rx_pkt->m_pkthdr.len == sizeof(uint16)) {
				    *mtod(conn->rx_pkt, uint16 *) = le16toh(*mtod(conn->rx_pkt,uint16 *));
			    } else {
				    cp->result = L2CAP_UNKNOWN;  XXX
				    debugf("invalid L2CAP_InfoRsp command. Bad connectionless MTU parameter, len=%d\n", conn->rx_pkt->m_pkthdr.len);
			    }*/
			break;

		default:
			debugf("invalid L2CAP_InfoRsp command. Unknown info type=%d\n", cp->type);
			break;
		}
	}

	//INDICATION error = ng_l2cap_l2ca_get_info_rsp(cmd->conn, cmd->token, cp->result, conn->rx_pkt);
	btCoreData->AcknowledgeSignal(cmd);

	return error;
}


static status_t
l2cap_process_cmd_rej(HciConnection* conn, uint8 ident, net_buffer* buffer)
{
	L2capFrame*	cmd = NULL;

	/* TODO: review this command... Get command data */
	NetBufferHeaderReader<l2cap_cmd_rej_cp> command(buffer);
	status_t status = command.Status();
	if (status < B_OK) {
		return ENOBUFS;
	}

	command->reason = le16toh(command->reason);

	debugf("reason=%d\n", command->reason);

	command.Remove();

	/* Check if we have pending command descriptor */
	cmd = btCoreData->SignalByIdent(conn, ident);
	if (cmd != NULL) {
		/* If command timeout already happened then ignore reject */
		if (btCoreData->UnTimeoutSignal(cmd) != 0) {
			gBufferModule->free(buffer);
			return ETIMEDOUT;
		}


		switch (cmd->code) {
			case L2CAP_CON_REQ:
				//INDICATION l2cap_l2ca_con_rsp(cmd->channel, cmd->token, cp->reason, 0);
				btCoreData->RemoveChannel(conn, cmd->channel->scid);
			break;

			case L2CAP_CFG_REQ:
				//INDICATION l2cap_l2ca_cfg_rsp(cmd->channel, cmd->token, cp->reason);
			break;

			case L2CAP_DISCON_REQ:
				//INDICATION l2cap_l2ca_discon_rsp(cmd->channel, cmd->token, cp->reason);
				btCoreData->RemoveChannel(conn, cmd->channel->scid);
			break;

			case L2CAP_ECHO_REQ:
				//INDICATION l2cap_l2ca_ping_rsp(cmd->con, cmd->token, cp->reason, NULL);
			break;

			case L2CAP_INFO_REQ:
				//INDICATION l2cap_l2ca_get_info_rsp(cmd->con, cmd->token, cp->reason, NULL);
			break;

		default:
			debugf("unexpected L2CAP_CommandRej. Unexpected opcode=%d\n", cmd->code);
			break;
		}

		btCoreData->AcknowledgeSignal(cmd);

	} else
		debugf("unexpected L2CAP_CommandRej command. Requested ident does not exist, ident=%d\n", ident);

	return B_OK;
}


#if 0
#pragma mark - Queuing Outgoing signals
#endif


status_t
send_l2cap_reject(HciConnection* conn, uint8 ident, uint16 reason,
	uint16 mtu, uint16 scid, uint16 dcid)
{
	L2capFrame*	cmd = NULL;

	cmd = btCoreData->SpawnSignal(conn, NULL, l2cap_cmd_rej(ident, reason,
		mtu, scid, dcid), ident, L2CAP_CMD_REJ);
	if (cmd == NULL)
		return ENOMEM;

	/* Link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;
}


status_t
send_l2cap_con_rej(HciConnection* conn, uint8 ident, uint16 scid, uint16 dcid,
	uint16 result)
{
	L2capFrame*	cmd = NULL;

	cmd = btCoreData->SpawnSignal(conn, NULL,
		l2cap_con_rsp(ident, scid, dcid, result, 0), ident, L2CAP_CON_RSP);
	if (cmd == NULL)
		return ENOMEM;

	/* Link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;
}


status_t
send_l2cap_cfg_rsp(HciConnection* conn, uint8 ident, uint16 scid,
	uint16 result, net_buffer* opt)
{
	L2capFrame*	cmd = NULL;

	cmd = btCoreData->SpawnSignal(conn, NULL,
		l2cap_cfg_rsp(ident, scid, 0, result, opt),	ident, L2CAP_CFG_RSP);
	if (cmd == NULL) {
		gBufferModule->free(opt);
		return ENOMEM;
	}

	/* Link command to the queue */
	SchedConnectionPurgeThread(conn);

	return B_OK;
}
