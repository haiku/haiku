/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "displayport.h"

#include <Debug.h>

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
	uint16 encoderID = gConnector[connectorIndex]->encoderExternal.objectID;

	if (encoderID == ENCODER_OBJECT_ID_NUTMEG)
		return 270000;

	// TODO: calculate DisplayPort max pixel clock based on bpp and DP channels
	return 162000;
}


void
dp_setup_connectors()
{
	TRACE("%s\n", __func__);

	for (uint32 index = 0; index < ATOM_MAX_SUPPORTED_DEVICE; index++) {
		dp_info* dpInfo = &gConnector[index]->dpInfo;
		dpInfo->valid = false;
		if (gConnector[index]->valid == false) {
			dpInfo->config[0] = 0;
			continue;
		}

		if (connector_is_dp(index) == false) {
			dpInfo->config[0] = 0;
			continue;
		}

		uint32 gpioID = gConnector[index]->gpioID;

		uint32 auxPin = gGPIOInfo[gpioID]->hwPin;
		dpInfo->auxPin = auxPin;

		uint8 auxMessage[25];
		int result;

		result = dp_aux_read(auxPin, DP_DPCD_REV, auxMessage, 8, 0);
		if (result > 0) {
			dpInfo->valid = true;
			memcpy(dpInfo->config, auxMessage, 8);
		}

		dpInfo->linkRate = dp_get_link_clock(index);
	}
}


static bool
dp_get_link_status(dp_info* dp)
{
	int result = dp_aux_read(dp->auxPin, DP_LANE_STATUS_0_1,
		dp->linkStatus, DP_LINK_STATUS_SIZE, 100);

	if (result <= 0) {
		ERROR("%s: DisplayPort link status failed\n", __func__);
		return false;
	}

	return true;
}


static uint8
dp_get_lane_status(dp_info* dp, int lane)
{
	int i = DP_LANE_STATUS_0_1 + (lane >> 1);
	int s = (lane & 1) * 4;
	uint8 l = dp->linkStatus[i - DP_LANE_STATUS_0_1];
	return (l >> s) & 0xf;
}


static bool
dp_clock_recovery_ok(dp_info* dp)
{
	int lane;
	uint8 laneStatus;

	for (lane = 0; lane < dp->laneCount; lane++) {
		laneStatus = dp_get_lane_status(dp, lane);
		if ((laneStatus & DP_LANE_STATUS_CR_DONE_A) == 0)
			return false;
	}
	return true;
}


static bool
dp_clock_equalization_ok(dp_info* dp)
{
	uint8 laneAlignment
		= dp->linkStatus[DP_LANE_ALIGN - DP_LANE_STATUS_0_1];

	if ((laneAlignment & DP_LANE_ALIGN_DONE) == 0)
		return false;

	int lane;
	for (lane = 0; lane < dp->laneCount; lane++) {
		uint8 laneStatus = dp_get_lane_status(dp, lane);
		if ((laneStatus & DP_LANE_STATUS_EQUALIZED_A)
			!= DP_LANE_STATUS_EQUALIZED_A) {
			return false;
		}
	}
	return true;
}


static void
dp_update_vs_emph(uint32 connectorIndex)
{
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	// Set initial vs and emph on source
	transmitter_dig_setup(connectorIndex, dp->linkRate, 0,
		dp->trainingSet[0], ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH);

	// Set vs and emph on the sink
	dp_aux_write(dp->auxPin, DP_TRAIN_LANE0,
		dp->trainingSet, dp->laneCount, 0);
}


static uint8
dp_get_adjust_request_voltage(dp_info* dp, int lane)
{
	int i = DP_ADJ_REQUEST_0_1 + (lane >> 1);
	int s = (((lane & 1) != 0) ? DP_ADJ_VCC_SWING_LANEB_SHIFT
		: DP_ADJ_VCC_SWING_LANEA_SHIFT);
	uint8 l = dp->linkStatus[i - DP_LANE_STATUS_0_1];

	return ((l >> s) & 0x3) << DP_TRAIN_VCC_SWING_SHIFT;
}


static uint8
dp_get_adjust_request_pre_emphasis(dp_info* dp, int lane)
{
	int i = DP_ADJ_REQUEST_0_1 + (lane >> 1);
	int s = (((lane & 1) != 0) ? DP_ADJ_PRE_EMPHASIS_LANEB_SHIFT
		: DP_ADJ_PRE_EMPHASIS_LANEB_SHIFT);
	uint8 l = dp->linkStatus[i - DP_LANE_STATUS_0_1];

	return ((l >> s) & 0x3) << DP_TRAIN_PRE_EMPHASIS_SHIFT;
}


static void
dp_get_adjust_train(dp_info* dp)
{
	TRACE("%s\n", __func__);

	const char* voltageNames[] = {
		"0.4V", "0.6V", "0.8V", "1.2V"
	};
	const char* preEmphasisNames[] = {
		"0dB", "3.5dB", "6dB", "9.5dB"
	};

	uint8 voltage = 0;
	uint8 preEmphasis = 0;
	int lane;

	for (lane = 0; lane < dp->laneCount; lane++) {
		uint8 laneVoltage = dp_get_adjust_request_voltage(dp, lane);
		uint8 lanePreEmphasis = dp_get_adjust_request_pre_emphasis(dp, lane);

		TRACE("%s: Requested %s at %s for lane %d\n", __func__,
			preEmphasisNames[lanePreEmphasis >> DP_TRAIN_PRE_EMPHASIS_SHIFT],
			voltageNames[laneVoltage >> DP_TRAIN_VCC_SWING_SHIFT],
			lane);

		if (laneVoltage > voltage)
			voltage = laneVoltage;
		if (lanePreEmphasis > preEmphasis)
			preEmphasis = lanePreEmphasis;
	}

	// Check for maximum voltage and toggle max if reached
	if (voltage >= DP_TRAIN_VCC_SWING_1200)
		voltage |= DP_TRAIN_MAX_SWING_EN;

	// Check for maximum pre-emphasis and toggle max if reached
	if (preEmphasis >= DP_TRAIN_PRE_EMPHASIS_9_5)
		preEmphasis |= DP_TRAIN_MAX_EMPHASIS_EN;

	for (lane = 0; lane < 4; lane++)
		dp->trainingSet[lane] = voltage | preEmphasis;
}


static void
dp_set_tp(uint32 connectorIndex, int trainingPattern)
{
	TRACE("%s\n", __func__);

	radeon_shared_info &info = *gInfo->shared_info;
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	int rawTrainingPattern = 0;

	/* set training pattern on the source */
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		switch (trainingPattern) {
			case DP_TRAIN_PATTERN_1:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN1;
				break;
			case DP_TRAIN_PATTERN_2:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN2;
				break;
			case DP_TRAIN_PATTERN_3:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN3;
				break;
		}
		// TODO: PixelClock 0 ok?
		encoder_dig_setup(connectorIndex, 0, rawTrainingPattern);
	} else {
		ERROR("%s: TODO: dp_encoder_service\n", __func__);
		return;
		#if 0
		switch (trainingPattern) {
			case DP_TRAINING_PATTERN_1:
				rawTrainingPattern = 0;
				break;
			case DP_TRAINING_PATTERN_2:
				rawTrainingPattern = 1;
				break;
		}
		radeon_dp_encoder_service(dp_info->rdev,
			ATOM_DP_ACTION_TRAINING_PATTERN_SEL, dp_info->dp_clock,
			dp_info->enc_id, rawTrainingPattern);
		#endif
	}

	// Enable training pattern on the sink
	dpcd_reg_write(dp->auxPin, DP_TRAIN, trainingPattern);
}


status_t
dp_link_train_cr(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);

	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	// Display Port Clock Recovery Training

	bool clockRecovery = false;
	uint8 voltage = 0xff;
	int lane;

	dp_set_tp(connectorIndex, DP_TRAIN_PATTERN_1);
	memset(dp->trainingSet, 0, 4);
	dp_update_vs_emph(connectorIndex);

	while (1) {
		if (dp->trainingReadInterval == 0)
			snooze(100);
		else
			snooze(1000 * 4 * dp->trainingReadInterval);

		if (!dp_get_link_status(dp))
			break;

		if (dp_clock_recovery_ok(dp)) {
			clockRecovery = true;
			break;
		}

		for (lane = 0; lane < dp->laneCount; lane++) {
			if ((dp->trainingSet[lane] & DP_TRAIN_MAX_SWING_EN) == 0)
				break;
		}

		if (lane == dp->laneCount) {
			ERROR("%s: clock recovery reached max voltage\n", __func__);
			break;
		}

		if ((dp->trainingSet[0] & DP_TRAIN_VCC_SWING_MASK) == voltage) {
			dp->trainingAttempts++;
			if (dp->trainingAttempts >= 5) {
				ERROR("%s: clock recovery tried 5 times\n", __func__);
				break;
			}
		} else
			dp->trainingAttempts = 0;

		voltage = dp->trainingSet[0] & DP_TRAIN_VCC_SWING_MASK;

		// Compute new trainingSet as requested by sink
		dp_get_adjust_train(dp);

		dp_update_vs_emph(connectorIndex);
	}

	if (!clockRecovery) {
		ERROR("%s: clock recovery failed\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: clock recovery at voltage %d pre-emphasis %d\n",
		__func__, dp->trainingSet[0] & DP_TRAIN_VCC_SWING_MASK,
		(dp->trainingSet[0] & DP_TRAIN_PRE_EMPHASIS_MASK)
		>> DP_TRAIN_PRE_EMPHASIS_SHIFT);
	return B_OK;
}


status_t
dp_link_train_ce(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);

	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	// TODO: DisplayPort: Supports TP3?
	dp_set_tp(connectorIndex, DP_TRAIN_PATTERN_2);

	dp->trainingAttempts = 0;
	bool channelEqual = false;

	while (1) {
		if (dp->trainingReadInterval == 0)
			snooze(100);
		else
			snooze(1000 * 4 * dp->trainingReadInterval);

		if (!dp_get_link_status(dp))
			break;

		if (dp_clock_equalization_ok(dp)) {
			channelEqual = true;
			break;
		}

		if (dp->trainingAttempts > 5) {
			ERROR("%s: ERROR: failed > 5 times!\n", __func__);
			break;
		}

		dp_get_adjust_train(dp);

		dp_update_vs_emph(connectorIndex);
		dp->trainingAttempts++;
	}

	if (!channelEqual) {
		ERROR("%s: ERROR: failed\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: channels equalized at voltage %d pre-emphasis %d\n",
		__func__, dp->trainingSet[0] & DP_ADJ_VCC_SWING_LANEA_MASK,
		(dp->trainingSet[0] & DP_TRAIN_PRE_EMPHASIS_MASK)
		>> DP_TRAIN_PRE_EMPHASIS_SHIFT);

	return B_OK;
}


status_t
dp_link_train(uint8 crtcID, display_mode* mode)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	if (dp->valid != true) {
		ERROR("%s: started on invalid DisplayPort connector #%" B_PRIu32 "\n",
			__func__, connectorIndex);
		return B_ERROR;
	}

	int index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);
	// Table version
	uint8 tableMajor;
	uint8 tableMinor;

	dp->trainingUseEncoder = true;
	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		== B_OK) {
		if (tableMinor > 1) {
			// The AtomBIOS DPEncoderService greater then 1.1 can't program the
			// training pattern properly.
			dp->trainingUseEncoder = false;
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

	dp->trainingReadInterval
		= dpcd_reg_read(hwPin, DP_TRAINING_AUX_RD_INTERVAL);

	uint8 sandbox = dpcd_reg_read(hwPin, DP_MAX_LANE_COUNT);

	radeon_shared_info &info = *gInfo->shared_info;
	//bool dpTPS3Supported = false;
	//if (info.dceMajor >= 5 && (sandbox & DP_TPS3_SUPPORTED) != 0)
	//	dpTPS3Supported = true;

	// *** DisplayPort link training initialization

	// Power up the DP sink
	if (dp->config[0] >= DP_DPCD_REV_11)
		dpcd_reg_write(hwPin, DP_SET_POWER, DP_SET_POWER_D0);

	// Possibly enable downspread on the sink
	if ((dp->config[3] & 0x1) != 0)
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, DP_DOWNSPREAD_CTRL_AMP_EN);
	else
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, 0);

	encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
		ATOM_ENCODER_CMD_SETUP_PANEL_MODE);

	if (dp->config[0] >= DP_DPCD_REV_11)
		sandbox |= DP_ENHANCED_FRAME_EN;
	dpcd_reg_write(hwPin, DP_LANE_COUNT, sandbox);

	// Set the link rate on the DP sink
	sandbox = dp_encode_link_rate(dp->linkRate);
	dpcd_reg_write(hwPin, DP_LINK_RATE, sandbox);

	// Start link training on source
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_START);
	} else {
		ERROR("%s: TODO: cannot use AtomBIOS DPEncoderService on card!\n",
			__func__);
	}

	// Disable the training pattern on the sink
	dpcd_reg_write(hwPin, DP_TRAIN, DP_TRAIN_PATTERN_DISABLED);

	dp_link_train_cr(connectorIndex);
	dp_link_train_ce(connectorIndex);


	// *** DisplayPort link training finish
	snooze(400);

	// Disable the training pattern on the sink
	dpcd_reg_write(hwPin, DP_TRAIN, DP_TRAIN_PATTERN_DISABLED);

	// Disable the training pattern on the source
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_COMPLETE);
	} else {
		ERROR("%s: TODO: cannot use AtomBIOS DPEncoderService on card!\n",
			__func__);
	}

	return B_OK;
}
