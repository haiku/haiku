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
#include "connector.h"
#include "mode.h"


#undef TRACE

#define TRACE_DP
#ifdef TRACE_DP
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


static int
dp_aux_speak(uint32 hwPin, uint8* send, int sendBytes,
	uint8* recv, int recvBytes, uint8 delay, uint8* ack)
{
	if (hwPin == 0) {
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
	args.v1.ucChannelID = hwPin;
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
dp_aux_write(uint32 hwPin, uint16 address,
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
		int result = dp_aux_speak(hwPin, auxMessage, auxMessageBytes,
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
dp_aux_read(uint32 hwPin, uint16 address,
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
		int result = dp_aux_speak(hwPin, auxMessage, auxMessageBytes,
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


static void
dpcd_reg_write(uint32 hwPin, uint16 address, uint8 value)
{
	dp_aux_write(hwPin, address, &value, 1, 0);
}


static uint8
dpcd_reg_read(uint32 hwPin, uint16 address)
{
	uint8 value = 0;
	dp_aux_read(hwPin, address, &value, 1, 0);

	return value;
}


status_t
dp_aux_get_i2c_byte(uint32 hwPin, uint16 address, uint8* data, bool end)
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

		int result = dp_aux_speak(hwPin, auxMessage, auxMessageBytes,
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
dp_aux_set_i2c_byte(uint32 hwPin, uint16 address, uint8* data, bool end)
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

		int result = dp_aux_speak(hwPin, auxMessage, auxMessageBytes,
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


uint32
dp_get_link_clock_encode(uint32 dpLinkClock)
{
	switch (dpLinkClock) {
		case 270000:
			return DP_LINK_BW_2_7;
		case 540000:
			return DP_LINK_BW_5_4;
	}

	return DP_LINK_BW_1_62;
}


uint32
dp_get_link_clock_decode(uint32 dpLinkClock)
{
	switch (dpLinkClock) {
		case DP_LINK_BW_2_7:
			return 270000;
		case DP_LINK_BW_5_4:
			return 540000;
	}
	return 162000;
}


uint32
dp_get_lane_count(uint32 connectorIndex, display_mode* mode)
{
	uint32 bitsPerPixel = get_mode_bpp(mode);

	uint32 maxLaneCount = gDPInfo[connectorIndex]->config[DP_MAX_LANE_COUNT]
		& DP_MAX_LANE_COUNT_MASK;

	uint32 maxLinkRate = dp_get_link_clock_decode(
			gDPInfo[connectorIndex]->config[DP_MAX_LINK_RATE]);

	uint32 lane;
	for (lane = 1; lane < maxLaneCount; lane <<= 1) {
		uint32 maxDPPixelClock = (maxLinkRate * lane * 8) / bitsPerPixel;
		if (mode->timing.pixel_clock <= maxDPPixelClock)
			break;
	}
	TRACE("%s: connector: %" B_PRIu32 ", lanes: %" B_PRIu32 "\n", __func__,
		connectorIndex, lane);

	return lane;
}


void
dp_setup_connectors()
{
	TRACE("%s\n", __func__);

	for (uint32 index = 0; index < ATOM_MAX_SUPPORTED_DEVICE; index++) {
		gDPInfo[index]->valid = false;
		if (gConnector[index]->valid == false) {
			gDPInfo[index]->config[0] = 0;
			continue;
		}

		if (connector_is_dp(index) == false) {
			gDPInfo[index]->config[0] = 0;
			continue;
		}

		uint32 gpioID = gConnector[index]->gpioID;
		uint32 hwPin = gGPIOInfo[gpioID]->hwPin;

		uint8 auxMessage[25];
		int result;

		result = dp_aux_read(hwPin, DP_DPCD_REV, auxMessage, 8, 0);
		if (result > 0) {
			gDPInfo[index]->valid = true;
			memcpy(gDPInfo[index]->config, auxMessage, 8);
		}

		gDPInfo[index]->clock = dp_get_link_clock(index);
	}
}


status_t
dp_link_train(uint8 crtcID, display_mode* mode)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	if (gDPInfo[connectorIndex]->valid != true) {
		ERROR("%s: started on non-DisplayPort connector #%" B_PRIu32 "\n",
			__func__, connectorIndex);
		return B_ERROR;
	}

	int index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);
	// Table version
	uint8 tableMajor;
	uint8 tableMinor;

	bool dpUseEncoder = true;
	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		== B_OK) {
		if (tableMinor > 1) {
			// The AtomBIOS DPEncoderService greater then 1.1 can't program the
			// training pattern properly.
			dpUseEncoder = false;
		}
	}

	uint32 linkEnumeration
		= gConnector[connectorIndex]->encoder.linkEnumeration;
	uint32 gpioID = gConnector[connectorIndex]->gpioID;
	uint32 hwPin = gGPIOInfo[gpioID]->hwPin;

	uint32 dpEncoderID = 0;
	if (encoder_pick_dig(connectorIndex) > 0)
		dpEncoderID |= ATOM_DP_CONFIG_DIG2_ENCODER;
	else
		dpEncoderID |= ATOM_DP_CONFIG_DIG1_ENCODER;
	if (linkEnumeration == GRAPH_OBJECT_ENUM_ID2)
		dpEncoderID |= ATOM_DP_CONFIG_LINK_B;
	else
		dpEncoderID |= ATOM_DP_CONFIG_LINK_A;

	//uint8 dpReadInterval = dpcd_reg_read(hwPin, DP_TRAINING_AUX_RD_INTERVAL);
	uint8 sandbox = dpcd_reg_read(hwPin, DP_MAX_LANE_COUNT);

	radeon_shared_info &info = *gInfo->shared_info;
	bool dpTPS3Supported = false;
	if (info.dceMajor >= 5 && (sandbox & DP_TPS3_SUPPORTED) != 0)
		dpTPS3Supported = true;

	// DisplayPort training initialization

	// Power up the DP sink
	if (gDPInfo[connectorIndex]->config[0] >= 0x11)
		dpcd_reg_write(hwPin, DP_SET_POWER, DP_SET_POWER_D0);

	// Possibly enable downspread on the sink
	if ((gDPInfo[connectorIndex]->config[3] & 0x1) != 0)
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, DP_SPREAD_AMP_0_5);
	else
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, 0);

	encoder_dig_setup(connectorIndex, 0,
		ATOM_ENCODER_CMD_SETUP_PANEL_MODE);

	if (gDPInfo[connectorIndex]->config[0] >= 0x11)
		sandbox |= DP_LANE_COUNT_ENHANCED_FRAME_EN;
	dpcd_reg_write(hwPin, DP_LANE_COUNT_SET, sandbox);

	// Set the link rate on the DP sink
	sandbox = dp_get_link_clock_encode(gDPInfo[connectorIndex]->clock);
	dpcd_reg_write(hwPin, DP_LINK_BW_SET, sandbox);

	// Start link training on source
	if (info.dceMajor >= 4 || !dpUseEncoder) {
		encoder_dig_setup(connectorIndex, 0,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_START);
	} else {
		ERROR("%s: TODO: cannot use AtomBIOS DPEncoderService on card!\n",
			__func__);
	}

	/* disable the training pattern on the sink */
	dpcd_reg_write(hwPin, DP_TRAINING_PATTERN_SET,
		DP_TRAINING_PATTERN_DISABLE);

	// TODO: dp_link_train_cr
	// TODO: dp_link_train_ce

	snooze(400);

	/* disable the training pattern on the sink */
	dpcd_reg_write(hwPin, DP_TRAINING_PATTERN_SET,
		DP_TRAINING_PATTERN_DISABLE);

	/* disable the training pattern on the source */
	if (info.dceMajor >= 4 || !dpUseEncoder) {
		encoder_dig_setup(connectorIndex, 0,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_COMPLETE);
	} else {
		ERROR("%s: TODO: cannot use AtomBIOS DPEncoderService on card!\n",
			__func__);
	}

	return B_OK;
}
