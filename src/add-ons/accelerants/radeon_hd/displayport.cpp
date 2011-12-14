/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "displayport.h"

#include <Debug.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "displayport_reg.h"


#undef TRACE

#define TRACE_DP
#ifdef TRACE_DP
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


static int
dp_aux_speak(uint32 hwLine, uint8* send, int sendBytes,
	uint8* recv, int recvBytes, uint8 delay, uint8* ack)
{
	if (hwLine == 0) {
		ERROR("%s: cannot speak on invalid GPIO pin!\n", __func__);
		return B_IO_ERROR;
	}

	int index = GetIndexIntoMasterTable(COMMAND, ProcessAuxChannelTransaction);

	// Build AtomBIOS Transaction
	union auxChannelTransaction {
		PROCESS_AUX_CHANNEL_TRANSACTION_PS_ALLOCATION v1;
		PROCESS_AUX_CHANNEL_TRANSACTION_PARAMETERS_V2 v2;
	};
	union auxChannelTransaction args;
	memset(&args, 0, sizeof(args));

	args.v1.lpAuxRequest = 0;
	args.v1.lpDataOut = 16;
	args.v1.ucDataOutLen = 0;
	args.v1.ucChannelID = hwLine;
	args.v1.ucDelay = delay / 10;

	//if (ASIC_IS_DCE4(rdev))
	//	args.v2.ucHPD_ID = chan->rec.hpd;

	unsigned char* base = (unsigned char*)gAtomContext->scratch;
	memcpy(base, send, sendBytes);

	atom_execute_table(gAtomContext, index, (uint32*)&args);

	*ack = args.v1.ucReplyStatus;

	switch (args.v1.ucReplyStatus) {
		case 1:
			ERROR("%s: dp_aux_ch timeout!\n", __func__);
			return B_TIMED_OUT;
		case 2:
			ERROR("%s: dp_aux_ch flags not zero!\n", __func__);
			return B_BUSY;
		case 3:
			ERROR("%s: dp_aux_ch error!\n", __func__);
			return B_IO_ERROR;
	}

	int recvLength = args.v1.ucDataOutLen;
	if (recvLength > recvBytes)
		recvLength = recvBytes;

	if (recv && recvBytes)
		memcpy(recv, base + 16, recvLength);

	return recvLength;
}


int
dp_aux_write(uint32 hwLine, uint16 address,
	uint8* send, uint8 sendBytes, uint8 delay)
{
	uint8 auxMessage[20];
	int auxMessageBytes = sendBytes + 4;

	if (sendBytes > 16)
		return -1;

	auxMessage[0] = address;
	auxMessage[1] = address >> 8;
	auxMessage[2] = AUX_NATIVE_WRITE << 4;
	auxMessage[3] = (auxMessageBytes << 4) | (sendBytes - 1);
	memcpy(&auxMessage[4], send, sendBytes);

	uint8 retry;
	for (retry = 0; retry < 4; retry++) {
		uint8 ack;
		int result = dp_aux_speak(hwLine, auxMessage, auxMessageBytes,
			NULL, 0, delay, &ack);

		if (result == B_BUSY)
			continue;
		else if (result < B_OK)
			return result;

		if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_ACK)
			return sendBytes;
		else if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_DEFER)
			snooze(400);
		else
			return B_IO_ERROR;
	}

	return B_IO_ERROR;
}


int
dp_aux_read(uint32 hwLine, uint16 address,
	uint8* recv, int recvBytes, uint8 delay)
{
	uint8 auxMessage[4];
	int auxMessageBytes = 4;

	auxMessage[0] = address;
	auxMessage[1] = address >> 8;
	auxMessage[2] = AUX_NATIVE_READ << 4;
	auxMessage[3] = (auxMessageBytes << 4) | (recvBytes - 1);

	uint8 retry;
	for (retry = 0; retry < 4; retry++) {
		uint8 ack;
		int result = dp_aux_speak(hwLine, auxMessage, auxMessageBytes,
			recv, recvBytes, delay, &ack);

		if (result == B_BUSY)
			continue;
		else if (result < B_OK)
			return result;

		if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_ACK)
			return result;
		else if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_DEFER)
			snooze(400);
		else
			return B_IO_ERROR;
	}

	return B_IO_ERROR;
}


status_t
dp_aux_get_i2c_byte(uint32 hwLine, uint16 address, uint8* data, bool end)
{
	uint8 auxMessage[5];
	int auxMessageBytes = 4; // 4 for read

	/* Set up the command byte */
	auxMessage[2] = AUX_I2C_READ << 4;
	if (end == false)
		auxMessage[2] |= AUX_I2C_MOT << 4;

	auxMessage[0] = address;
	auxMessage[1] = address >> 8;

	auxMessage[3] = auxMessageBytes << 4;

	int retry;
	for (retry = 0; retry < 4; retry++) {
		uint8 ack;
		uint8 reply[2];
		int replyBytes = 1;

		int result = dp_aux_speak(hwLine, auxMessage, auxMessageBytes,
			reply, replyBytes, 0, &ack);
		if (result == B_BUSY)
			continue;
		else if (result < 0) {
			ERROR("%s: aux_ch failed: %d\n", __func__, result);
			return B_ERROR;
		}

		switch (ack & AUX_NATIVE_REPLY_MASK) {
			case AUX_NATIVE_REPLY_ACK:
				// I2C-over-AUX Reply field is only valid for AUX_ACK
				break;
			case AUX_NATIVE_REPLY_NACK:
				TRACE("%s: aux_ch native nack\n", __func__);
				return B_IO_ERROR;
			case AUX_NATIVE_REPLY_DEFER:
				TRACE("%s: aux_ch native defer\n", __func__);
				snooze(400);
				continue;
			default:
				TRACE("%s: aux_ch invalid native reply: 0x%02x\n",
					__func__, ack);
				return B_ERROR;
		}

		switch (ack & AUX_I2C_REPLY_MASK) {
			case AUX_I2C_REPLY_ACK:
				*data = reply[0];
				return B_OK;
			case AUX_I2C_REPLY_NACK:
				TRACE("%s: aux_i2c nack\n", __func__);
				return B_IO_ERROR;
			case AUX_I2C_REPLY_DEFER:
				TRACE("%s: aux_i2c defer\n", __func__);
				snooze(400);
				break;
			default:
				TRACE("%s: aux_i2c invalid native reply: 0x%02x\n",
					__func__, ack);
				return B_ERROR;
		}
	}

	TRACE("%s: aux i2c too many retries, giving up.\n", __func__);
	return B_ERROR;
}


status_t
dp_aux_set_i2c_byte(uint32 hwLine, uint16 address, uint8* data, bool end)
{
	uint8 auxMessage[5];
	int auxMessageBytes = 5; // 5 for write

	/* Set up the command byte */
	auxMessage[2] = AUX_I2C_WRITE << 4;
	if (end == false)
		auxMessage[2] |= AUX_I2C_MOT << 4;

	auxMessage[0] = address;
	auxMessage[1] = address >> 8;

	auxMessage[3] = auxMessageBytes << 4;
	auxMessage[4] = *data;

	int retry;
	for (retry = 0; retry < 4; retry++) {
		uint8 ack;
		uint8 reply[2];
		int replyBytes = 1;

		int result = dp_aux_speak(hwLine, auxMessage, auxMessageBytes,
			reply, replyBytes, 0, &ack);
		if (result == B_BUSY)
			continue;
		else if (result < 0) {
			ERROR("%s: aux_ch failed: %d\n", __func__, result);
			return B_ERROR;
		}

		switch (ack & AUX_NATIVE_REPLY_MASK) {
			case AUX_NATIVE_REPLY_ACK:
				// I2C-over-AUX Reply field is only valid for AUX_ACK
				break;
			case AUX_NATIVE_REPLY_NACK:
				TRACE("%s: aux_ch native nack\n", __func__);
				return B_IO_ERROR;
			case AUX_NATIVE_REPLY_DEFER:
				TRACE("%s: aux_ch native defer\n", __func__);
				snooze(400);
				continue;
			default:
				TRACE("%s: aux_ch invalid native reply: 0x%02x\n",
					__func__, ack);
				return B_ERROR;
		}

		switch (ack & AUX_I2C_REPLY_MASK) {
			case AUX_I2C_REPLY_ACK:
				// Success!
				return B_OK;
			case AUX_I2C_REPLY_NACK:
				TRACE("%s: aux_i2c nack\n", __func__);
				return B_IO_ERROR;
			case AUX_I2C_REPLY_DEFER:
				TRACE("%s: aux_i2c defer\n", __func__);
				snooze(400);
				break;
			default:
				TRACE("%s: aux_i2c invalid native reply: 0x%02x\n",
					__func__, ack);
				return B_ERROR;
		}
	}

	TRACE("%s: aux i2c too many retries, giving up.\n", __func__);
	return B_OK;
}


uint32
dp_get_link_clock(uint32 connectorIndex)
{
	uint16 encoderID = gConnector[connectorIndex]->encoder.objectID;

	if (encoderID == ENCODER_OBJECT_ID_NUTMEG)
		return 270000;

	// TODO: calculate DisplayPort max pixel clock based on bpp and DP channels
	return 162000;
}


status_t
dp_link_train(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);

	int index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);
	// Table version
	uint8 tableMajor;
	uint8 tableMinor;

	bool useDpencoder = true; // ??
	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		== B_OK) {
		if (tableMinor > 1) {
			useDpencoder = false;
		}
	}

	// TODO: perform displayport training
	return B_ERROR;
}
