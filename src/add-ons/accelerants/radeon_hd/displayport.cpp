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
#include "dp_raw.h"
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


uint32
dp_get_link_clock_encode(uint32 dpLinkClock)
{
	switch (dpLinkClock) {
		case 270000:
			return DP_LINK_RATE_270;
		case 540000:
			return DP_LINK_RATE_540;
	}

	return DP_LINK_RATE_162;
}


uint32
dp_get_link_clock_decode(uint32 dpLinkClock)
{
	switch (dpLinkClock) {
		case DP_LINK_RATE_270:
			return 270000;
		case DP_LINK_RATE_540:
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

		uint32 auxPin = gGPIOInfo[gpioID]->hwPin;
		gDPInfo[index]->auxPin = auxPin;
		gDPInfo[index]->connectorIndex = index;

		uint8 auxMessage[25];
		int result;

		result = dp_aux_read(auxPin, DP_DPCD_REV, auxMessage, 8, 0);
		if (result > 0) {
			gDPInfo[index]->valid = true;
			memcpy(gDPInfo[index]->config, auxMessage, 8);
		}

		gDPInfo[index]->clock = dp_get_link_clock(index);
	}
}


static bool
dp_get_link_status(dp_info* dp)
{
	int result = dp_aux_read(dp->auxPin, DP_LANE0_1_STATUS,
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
	int i = DP_LANE0_1_STATUS + (lane >> 1);
	int s = (lane & 1) * 4;
	uint8 l = dp->linkStatus[i - DP_LANE0_1_STATUS];
	return (l >> s) & 0xf;
}


static bool
dp_clock_recovery_ok(dp_info* dp)
{
	int lane;
	uint8 laneStatus;

	for (lane = 0; lane < dp->laneCount; lane++) {
		laneStatus = dp_get_lane_status(dp, lane);
		if ((laneStatus & DP_LANE_CR_DONE) == 0)
			return false;
	}
	return true;
}


static void
dp_update_vs_emph(dp_info* dp)
{
	// Set initial vs and emph on source
	transmitter_dig_setup(dp->connectorIndex, dp->clock, 0, dp->trainingSet[0],
		ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH);

	// Set vs and emph on the sink
	dp_aux_write(dp->auxPin, DP_LINK_TRAIN_LANE0,
		dp->trainingSet, dp->laneCount, 0);
}


static uint8
dp_get_adjust_request_voltage(dp_info* dp, int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = (((lane & 1) != 0) ? DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT
		: DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT);
	uint8 l = dp->linkStatus[i - DP_LANE0_1_STATUS];

	return ((l >> s) & 0x3) << DP_LINK_TRAIN_LANE_VCCSWING_SHIFT;
}


static uint8
dp_get_adjust_request_pre_emphasis(dp_info* dp, int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = (((lane & 1) != 0) ? DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT
		: DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT);
	uint8 l = dp->linkStatus[i - DP_LANE0_1_STATUS];

	return ((l >> s) & 0x3) << DP_LINK_TRAIN_LANE_PREE_SHIFT;
}


#define DP_VOLTAGE_MAX         DP_TRAIN_VOLTAGE_SWING_1200
#define DP_PRE_EMPHASIS_MAX    DP_TRAIN_PRE_EMPHASIS_9_5


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
			preEmphasisNames[lanePreEmphasis >> DP_LINK_TRAIN_LANE_PREE_SHIFT],
			voltageNames[laneVoltage >> DP_LINK_TRAIN_LANE_VCCSWING_SHIFT],
			lane);

		if (laneVoltage > voltage)
			voltage = laneVoltage;
		if (lanePreEmphasis > preEmphasis)
			preEmphasis = lanePreEmphasis;
	}

	if (voltage >= DP_VOLTAGE_MAX)
		voltage |= DP_TRAIN_MAX_SWING_REACHED;

	if (preEmphasis >= DP_PRE_EMPHASIS_MAX)
		preEmphasis |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

	for (lane = 0; lane < 4; lane++)
		dp->trainingSet[lane] = voltage | preEmphasis;
}


static void
dp_set_tp(dp_info* dp, int trainingPattern)
{
	TRACE("%s\n", __func__);

	radeon_shared_info &info = *gInfo->shared_info;

	int rawTrainingPattern = 0;

	/* set training pattern on the source */
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		switch (trainingPattern) {
			case DP_LINK_TRAIN_PATTERN_1:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN1;
				break;
			case DP_LINK_TRAIN_PATTERN_2:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN2;
				break;
			case DP_LINK_TRAIN_PATTERN_3:
				rawTrainingPattern = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN3;
				break;
		}
		// TODO: PixelClock 0 ok?
		encoder_dig_setup(dp->connectorIndex, 0, rawTrainingPattern);
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
	dpcd_reg_write(dp->auxPin, DP_LINK_TRAIN, trainingPattern);
}


status_t
dp_link_train_cr(dp_info* dp)
{
	TRACE("%s\n", __func__);

	// Display Port Clock Recovery Training

	bool clockRecovery = false;
	uint8 voltage = 0xff;
	int lane;

	dp_set_tp(dp, DP_LINK_TRAIN_PATTERN_1);
	memset(dp->trainingSet, 0, 4);
	dp_update_vs_emph(dp);

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
			if ((dp->trainingSet[lane] & DP_TRAIN_MAX_SWING_REACHED) == 0)
				break;
		}

		if (lane == dp->laneCount) {
			ERROR("%s: clock recovery reached max voltage\n", __func__);
			break;
		}

		if ((dp->trainingSet[0] & DP_TRAIN_VOLTAGE_SWING_MASK) == voltage) {
			dp->trainingAttempts++;
			if (dp->trainingAttempts >= 5) {
				ERROR("%s: clock recovery tried 5 times\n", __func__);
				break;
			}
		} else
			dp->trainingAttempts = 0;

		voltage = dp->trainingSet[0] & DP_TRAIN_VOLTAGE_SWING_MASK;

		// Compute new trainingSet as requested by sink
		dp_get_adjust_train(dp);

		dp_update_vs_emph(dp);
	}

	if (!clockRecovery) {
		ERROR("%s: clock recovery failed\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: clock recovery at voltage %d pre-emphasis %d\n",
		__func__, dp->trainingSet[0] & DP_TRAIN_VOLTAGE_SWING_MASK,
		(dp->trainingSet[0] & DP_LINK_TRAIN_LANE_PREE_MASK)
		>> DP_LINK_TRAIN_LANE_PREE_SHIFT);
	return B_OK;
}


status_t
dp_link_train(uint8 crtcID, display_mode* mode)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	dp_info* dp = gDPInfo[connectorIndex];

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
	if (dp->config[0] >= 0x11)
		dpcd_reg_write(hwPin, DP_SET_POWER, DP_SET_POWER_D0);

	// Possibly enable downspread on the sink
	if ((dp->config[3] & 0x1) != 0)
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, DP_DOWNSPREAD_CTRL_AMP_EN);
	else
		dpcd_reg_write(hwPin, DP_DOWNSPREAD_CTRL, 0);

	encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
		ATOM_ENCODER_CMD_SETUP_PANEL_MODE);

	if (dp->config[0] >= 0x11)
		sandbox |= DP_ENHANCED_FRAME_EN_MASK;
	dpcd_reg_write(hwPin, DP_LANE_COUNT, sandbox);

	// Set the link rate on the DP sink
	sandbox = dp_get_link_clock_encode(dp->clock);
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
	dpcd_reg_write(hwPin, DP_LINK_TRAIN, DP_LINK_TRAIN_PATTERN_DISABLED);

	dp_link_train_cr(dp);
	// TODO: dp_link_train_ce


	// *** DisplayPort link training finish
	snooze(400);

	// Disable the training pattern on the sink
	dpcd_reg_write(hwPin, DP_LINK_TRAIN, DP_LINK_TRAIN_PATTERN_DISABLED);

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
