/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Bill Randle, billr@neocat.org
 */


#include "displayport.h"

#include <Debug.h>

#include "accelerant_protos.h"
#include "connector.h"
#include "mode.h"
#include "edid.h"
#include "encoder.h"


#undef TRACE

#define TRACE_DP
#ifdef TRACE_DP
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


static ssize_t
dp_aux_speak(uint32 connectorIndex, uint8* send, int sendBytes,
	uint8* recv, int recvBytes, uint8 delay, uint8* ack)
{
	dp_info* dpInfo = &gConnector[connectorIndex]->dpInfo;
	if (dpInfo->auxPin == 0) {
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

	args.v2.lpAuxRequest = B_HOST_TO_LENDIAN_INT16(0 + 4);
	args.v2.lpDataOut = B_HOST_TO_LENDIAN_INT16(16 + 4);
	args.v2.ucDataOutLen = 0;
	args.v2.ucChannelID = dpInfo->auxPin;
	args.v2.ucDelay = delay / 10;

	// Careful! This value differs in different atombios calls :-|
	args.v2.ucHPD_ID = connector_pick_atom_hpdid(connectorIndex);

	unsigned char* base = (unsigned char*)(gAtomContext->scratch + 1);

	// TODO: This isn't correct for big endian systems!
	// send needs to be swapped on big endian.
	memcpy(base, send, sendBytes);

	atom_execute_table(gAtomContext, index, (uint32*)&args);

	*ack = args.v2.ucReplyStatus;

	switch (args.v2.ucReplyStatus) {
		case 1:
			ERROR("%s: dp_aux channel timeout!\n", __func__);
			return B_TIMED_OUT;
		case 2:
			ERROR("%s: dp_aux channel flags not zero!\n", __func__);
			return B_BUSY;
		case 3:
			ERROR("%s: dp_aux channel error!\n", __func__);
			return B_IO_ERROR;
	}

	int recvLength = args.v1.ucDataOutLen;
	if (recvLength > recvBytes)
		recvLength = recvBytes;

	// TODO: This isn't correct for big endian systems!
	// recv needs to be swapped on big endian.
	if (recv && recvBytes)
		memcpy(recv, base + 16, recvLength);

	return recvLength;
}


status_t
dp_aux_transaction(uint32 connectorIndex, dp_aux_msg* message)
{
	uint8 delay = 0;
	if (message == NULL) {
		ERROR("%s: DP message is invalid!\n", __func__);
		return B_ERROR;
	}

	if (message->size > 16) {
		ERROR("%s: Too many bytes! (%" B_PRIuSIZE ")\n", __func__,
			message->size);
		return B_ERROR;
	}

	uint8 transactionSize = 4;

	switch(message->request & ~DP_AUX_I2C_MOT) {
		case DP_AUX_NATIVE_WRITE:
		case DP_AUX_I2C_WRITE:
			transactionSize += message->size;
			break;
	}

	// If not bare address, check for buffer
	if (message->size > 0 && message->buffer == NULL) {
		ERROR("%s: DP message uninitalized buffer!\n", __func__);
		return B_ERROR;
	}

	uint8 auxMessage[20];
	auxMessage[0] = message->address & 0xff;
	auxMessage[1] = message->address >> 8;
	auxMessage[2] = message->request << 4;
	auxMessage[3] = message->size ? (message->size - 1) : 0;

	if (message->size == 0)
		auxMessage[3] |= 3 << 4;
	else
		auxMessage[3] |= transactionSize << 4;

	uint8 retry;
	for (retry = 0; retry < 7; retry++) {
		uint8 ack;
		ssize_t result = B_ERROR;
		switch(message->request & ~DP_AUX_I2C_MOT) {
			case DP_AUX_NATIVE_WRITE:
			case DP_AUX_I2C_WRITE:
				memcpy(auxMessage + 4, message->buffer, message->size);
				result = dp_aux_speak(connectorIndex, auxMessage,
					transactionSize, NULL, 0, delay, &ack);
				break;
			case DP_AUX_NATIVE_READ:
			case DP_AUX_I2C_READ:
				result = dp_aux_speak(connectorIndex, auxMessage,
					transactionSize, (uint8*)message->buffer, message->size,
					delay, &ack);
				break;
			default:
				ERROR("%s: Unknown dp_aux_msg request!\n", __func__);
				return B_ERROR;
		}

		if (result == B_BUSY)
			continue;
		else if (result < B_OK)
			return result;

		ack >>= 4;
		message->reply = ack;
		switch(message->reply & DP_AUX_NATIVE_REPLY_MASK) {
			case DP_AUX_NATIVE_REPLY_ACK:
				return B_OK;
			case DP_AUX_NATIVE_REPLY_DEFER:
				TRACE("%s: aux reply defer received. Snoozing.\n", __func__);
				snooze(400);
				break;
			default:
				TRACE("%s: aux invalid native reply: 0x%02x\n", __func__,
					message->reply);
				return B_IO_ERROR;
		}
	}

	ERROR("%s: IO Error. %" B_PRIu8 " attempts\n", __func__, retry);
	return B_IO_ERROR;
}


void
dpcd_reg_write(uint32 connectorIndex, uint16 address, uint8 value)
{
	//TRACE("%s: connector(%" B_PRId32 "): 0x%" B_PRIx16 " -> 0x%" B_PRIx8 "\n",
	//	__func__, connectorIndex, address, value);
	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.address = address;
	message.buffer = &value;
	message.request = DP_AUX_NATIVE_WRITE;
	message.size = 1;

	status_t result = dp_aux_transaction(connectorIndex, &message);
	if (result != B_OK) {
		ERROR("%s: error on DisplayPort aux write (0x%" B_PRIx32 ")\n",
			__func__, result);
	}
}


uint8
dpcd_reg_read(uint32 connectorIndex, uint16 address)
{
	//TRACE("%s: connector(%" B_PRId32 "): read 0x%" B_PRIx16 ".\n",
	//	__func__, connectorIndex, address);
	uint8 response[3];

	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.address = address;
	message.buffer = &response;
	message.request = DP_AUX_NATIVE_READ;
	message.size = 1;

	status_t result = dp_aux_transaction(connectorIndex, &message);
	if (result != B_OK) {
		ERROR("%s: error on DisplayPort aux read (0x%" B_PRIx32 ")\n",
			__func__, result);
	}

	return response[0];
}


status_t
dp_aux_get_i2c_byte(uint32 connectorIndex, uint16 address, uint8* data,
	bool start, bool stop)
{
	uint8 reply[3];
	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.address = address;
	message.buffer = reply;
	message.request = DP_AUX_I2C_READ;
	message.size = 1;
	if (stop == false) {
		// Remove Middle-Of-Transmission on final transaction
		message.request |= DP_AUX_I2C_MOT;
	}
	if  (stop || start) {
		// Bare address packet
		message.buffer = NULL;
		message.size = 0;
	}

	for (int attempt = 0; attempt < 7; attempt++) {
		status_t result = dp_aux_transaction(connectorIndex, &message);
		if (result != B_OK) {
			ERROR("%s: aux_ch transaction failed!\n", __func__); 
			return result;
		}

		switch (message.reply & DP_AUX_I2C_REPLY_MASK) {
			case DP_AUX_I2C_REPLY_ACK:
				*data = reply[0];
				return B_OK;
			case DP_AUX_I2C_REPLY_NACK:
				TRACE("%s: aux i2c nack\n", __func__);
				return B_IO_ERROR;
			case DP_AUX_I2C_REPLY_DEFER:
				TRACE("%s: aux i2c defer\n", __func__);
				snooze(400);
				break;
			default:
				TRACE("%s: aux invalid I2C reply: 0x%02x\n",
					__func__, message.reply);
				return B_ERROR;
		}
	}
	return B_ERROR;
}


status_t
dp_aux_set_i2c_byte(uint32 connectorIndex, uint16 address, uint8* data,
	bool start, bool stop)
{
	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.address = address;
	message.buffer = data;
	message.request = DP_AUX_I2C_WRITE;
	message.size = 1;
	if (stop == false)
		message.request |= DP_AUX_I2C_MOT;
	if (stop || start) {
		message.buffer = NULL;
		message.size = 0;
	}

	for (int attempt = 0; attempt < 7; attempt++) {
		status_t result = dp_aux_transaction(connectorIndex, &message);
		if (result != B_OK) {
			ERROR("%s: aux_ch transaction failed!\n", __func__); 
			return result;
		}
		switch (message.reply & DP_AUX_I2C_REPLY_MASK) {
			case DP_AUX_I2C_REPLY_ACK:
				return B_OK;
			case DP_AUX_I2C_REPLY_NACK:
				ERROR("%s: aux i2c nack\n", __func__);
				return B_IO_ERROR;
			case DP_AUX_I2C_REPLY_DEFER:
				TRACE("%s: aux i2c defer\n", __func__);
				snooze(400);
				break;
			default:
				ERROR("%s: aux invalid I2C reply: 0x%02x\n", __func__,
					message.reply);
				return B_IO_ERROR;
		}
	}

	return B_ERROR;
}


uint32
dp_get_lane_count(uint32 connectorIndex, display_mode* mode)
{
	// Radeon specific
	dp_info* dpInfo = &gConnector[connectorIndex]->dpInfo;

	size_t pixelChunk;
	size_t pixelsPerChunk;
	status_t result = dp_get_pixel_size_for((color_space)mode->space,
		&pixelChunk, NULL, &pixelsPerChunk);

	if (result != B_OK) {
		TRACE("%s: Invalid color space!\n", __func__);
		return 0;
	}

	uint32 bitsPerPixel = (pixelChunk / pixelsPerChunk) * 8;

	uint32 dpMaxLinkRate = dp_get_link_rate_max(dpInfo);
	uint32 dpMaxLaneCount = dp_get_lane_count_max(dpInfo);

	uint32 lane;
	// don't go below 2 lanes or display is jittery
	for (lane = 2; lane < dpMaxLaneCount; lane <<= 1) {
		uint32 maxPixelClock = dp_get_pixel_clock_max(dpMaxLinkRate, lane,
			bitsPerPixel);
		if (mode->timing.pixel_clock <= maxPixelClock)
			break;
	}

	TRACE("%s: Lanes: %" B_PRIu32 "\n", __func__, lane);
	return lane;
}


uint32
dp_get_link_rate(uint32 connectorIndex, display_mode* mode)
{
	uint16 encoderID = gConnector[connectorIndex]->encoderExternal.objectID;

	if (encoderID == ENCODER_OBJECT_ID_NUTMEG)
		return 270000;

	dp_info* dpInfo = &gConnector[connectorIndex]->dpInfo;
	uint32 laneCount = dp_get_lane_count(connectorIndex, mode);

	size_t pixelChunk;
	size_t pixelsPerChunk;
	status_t result = dp_get_pixel_size_for((color_space)mode->space,
		&pixelChunk, NULL, &pixelsPerChunk);

	if (result != B_OK) {
		TRACE("%s: Invalid color space!\n", __func__);
		return 0;
	}

	uint32 bitsPerPixel = (pixelChunk / pixelsPerChunk) * 8;

	uint32 maxPixelClock
		= dp_get_pixel_clock_max(162000, laneCount, bitsPerPixel);
	if (mode->timing.pixel_clock <= maxPixelClock)
		return 162000;

	maxPixelClock = dp_get_pixel_clock_max(270000, laneCount, bitsPerPixel);
	if (mode->timing.pixel_clock <= maxPixelClock)
		return 270000;

	// TODO: DisplayPort 1.2
	#if 0
	if (dp_is_dp12_capable(connectorIndex)) {
		maxPixelClock = dp_get_pixel_clock_max(540000, laneCount, bitsPerPixel);
		if (mode->timing.pixel_clock <= maxPixelClock)
			return 540000;
	}
	#endif

	return dp_get_link_rate_max(dpInfo);
}


void
dp_setup_connectors()
{
	TRACE("%s\n", __func__);

	for (uint32 index = 0; index < ATOM_MAX_SUPPORTED_DEVICE; index++) {
		dp_info* dpInfo = &gConnector[index]->dpInfo;
		dpInfo->valid = false;
		if (gConnector[index]->valid == false
			|| connector_is_dp(index) == false) {
			dpInfo->config[0] = 0;
			continue;
		}

		TRACE("%s: found dp connector on index %" B_PRIu32 "\n",
			__func__, index);
		uint32 i2cPinIndex = gConnector[index]->i2cPinIndex;

		uint32 auxPin = gGPIOInfo[i2cPinIndex]->hwPin;
		dpInfo->auxPin = auxPin;

		dp_aux_msg message;
		memset(&message, 0, sizeof(message));

		message.address = DP_DPCD_REV;
		message.request = DP_AUX_NATIVE_READ;
			// TODO: validate
		message.size = DP_DPCD_SIZE;
		message.buffer = dpInfo->config;

		status_t result = dp_aux_transaction(index, &message);

		if (result == B_OK) {
			dpInfo->valid = true;
			TRACE("%s: connector(%" B_PRIu32 "): successful read of DPCD\n",
				__func__, index);
		} else {
			TRACE("%s: connector(%" B_PRIu32 "): failed read of DPCD\n",
				__func__, index);
		}
		/*
		TRACE("%s: DPCD is ", __func__);
		uint32 position;
		for (position = 0; position < message.size; position++)
			_sPrintf("%02x ", message.buffer + position);
		_sPrintf("\n");
		*/
	}
}


bool
dp_get_link_status(uint32 connectorIndex)
{
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.request = DP_AUX_NATIVE_READ;
	message.address = DP_LANE_STATUS_0_1;
	message.size = DP_LINK_STATUS_SIZE;
	message.buffer = dp->linkStatus;

	// TODO: Delay 100? Newer AMD code doesn't care about link status
	status_t result = dp_aux_transaction(connectorIndex, &message);

	if (result != B_OK) {
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

	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	message.request = DP_AUX_NATIVE_WRITE;
	message.address = DP_TRAIN_LANE0;
	message.buffer = dp->trainingSet;
	message.size = dp->laneCount;
		// TODO: Review laneCount as it sounds strange.
	dp_aux_transaction(connectorIndex, &message);
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
		: DP_ADJ_PRE_EMPHASIS_LANEA_SHIFT);
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


static uint8
dp_encoder_service(uint32 connectorIndex, int action, int linkRate,
	uint8 lane)
{
	DP_ENCODER_SERVICE_PARAMETERS args;
	int index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);

	memset(&args, 0, sizeof(args));
	args.ucLinkClock = linkRate;
	args.ucAction = action;
	args.ucLaneNum = lane;
	args.ucConfig = 0;
	args.ucStatus = 0;

	// We really can't do ATOM_DP_ACTION_GET_SINK_TYPE with the
	// way I designed this below. Not used though.

	// Calculate encoder_id config
	if (encoder_pick_dig(connectorIndex))
		args.ucConfig |= ATOM_DP_CONFIG_DIG2_ENCODER;
	else
		args.ucConfig |= ATOM_DP_CONFIG_DIG1_ENCODER;

	if (gConnector[connectorIndex]->encoder.linkEnumeration
			== GRAPH_OBJECT_ENUM_ID2) {
		args.ucConfig |= ATOM_DP_CONFIG_LINK_B;
	} else
		args.ucConfig |= ATOM_DP_CONFIG_LINK_A;

	atom_execute_table(gAtomContext, index, (uint32*)&args);

	return args.ucStatus;
}


static void
dp_set_tp(uint32 connectorIndex, int trainingPattern)
{
	TRACE("%s\n", __func__);

	radeon_shared_info &info = *gInfo->shared_info;
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	int rawTrainingPattern = 0;

	/* set training pattern on the source */
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		TRACE("%s: Training with encoder...\n", __func__);
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
		encoder_dig_setup(connectorIndex, pll->pixelClock, rawTrainingPattern);
	} else {
		TRACE("%s: Training with encoder service...\n", __func__);
		switch (trainingPattern) {
			case DP_TRAIN_PATTERN_1:
				rawTrainingPattern = 0;
				break;
			case DP_TRAIN_PATTERN_2:
				rawTrainingPattern = 1;
				break;
		}
		dp_encoder_service(connectorIndex, ATOM_DP_ACTION_TRAINING_PATTERN_SEL,
			dp->linkRate, rawTrainingPattern);
	}

	// Enable training pattern on the sink
	dpcd_reg_write(connectorIndex, DP_TRAIN, trainingPattern);
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
	snooze(400);

	while (1) {
		if (dp->trainingReadInterval == 0)
			snooze(100);
		else
			snooze(1000 * 4 * dp->trainingReadInterval);

		if (!dp_get_link_status(connectorIndex))
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
dp_link_train_ce(uint32 connectorIndex, bool tp3Support)
{
	TRACE("%s\n", __func__);

	dp_info* dp = &gConnector[connectorIndex]->dpInfo;

	if (tp3Support)
		dp_set_tp(connectorIndex, DP_TRAIN_PATTERN_3);
	else
		dp_set_tp(connectorIndex, DP_TRAIN_PATTERN_2);

	dp->trainingAttempts = 0;
	bool channelEqual = false;

	while (1) {
		if (dp->trainingReadInterval == 0)
			snooze(400);
		else
			snooze(1000 * 4 * dp->trainingReadInterval);

		if (!dp_get_link_status(connectorIndex)) {
			ERROR("%s: ERROR: Unable to get link status!\n", __func__);
			break;
		}

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
dp_link_train(uint8 crtcID)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	dp_info* dp = &gConnector[connectorIndex]->dpInfo;
	display_mode* mode = &gDisplay[crtcID]->currentMode;

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
		= dpcd_reg_read(connectorIndex, DP_TRAINING_AUX_RD_INTERVAL);

	uint8 sandbox = dpcd_reg_read(connectorIndex, DP_MAX_LANE_COUNT);

	radeon_shared_info &info = *gInfo->shared_info;
	bool dpTPS3Supported = false;
	if (info.dceMajor >= 5 && (sandbox & DP_TPS3_SUPPORTED) != 0)
		dpTPS3Supported = true;

	// *** DisplayPort link training initialization

	// Power up the DP sink
	if (dp->config[0] >= DP_DPCD_REV_11)
		dpcd_reg_write(connectorIndex, DP_SET_POWER, DP_SET_POWER_D0);

	// Possibly enable downspread on the sink
	if ((dp->config[3] & 0x1) != 0) {
		dpcd_reg_write(connectorIndex, DP_DOWNSPREAD_CTRL,
			DP_DOWNSPREAD_CTRL_AMP_EN);
	} else
		dpcd_reg_write(connectorIndex, DP_DOWNSPREAD_CTRL, 0);

	encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
		ATOM_ENCODER_CMD_SETUP_PANEL_MODE);

	// TODO: Doesn't this overwrite important dpcd info?
	sandbox = dp->laneCount;
	if ((dp->config[0] >= DP_DPCD_REV_11)
		&& (dp->config[2] & DP_ENHANCED_FRAME_CAP_EN))
		sandbox |= DP_ENHANCED_FRAME_EN;
	dpcd_reg_write(connectorIndex, DP_LANE_COUNT, sandbox);

	// Set the link rate on the DP sink
	sandbox = dp_encode_link_rate(dp->linkRate);
	dpcd_reg_write(connectorIndex, DP_LINK_RATE, sandbox);

	// Start link training on source
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_START);
	} else {
		dp_encoder_service(connectorIndex, ATOM_DP_ACTION_TRAINING_START,
			dp->linkRate, 0);
	}

	// Disable the training pattern on the sink
	dpcd_reg_write(connectorIndex, DP_TRAIN, DP_TRAIN_PATTERN_DISABLED);

	dp_link_train_cr(connectorIndex);
	dp_link_train_ce(connectorIndex, dpTPS3Supported);

	// *** DisplayPort link training finish
	snooze(400);

	// Disable the training pattern on the sink
	dpcd_reg_write(connectorIndex, DP_TRAIN, DP_TRAIN_PATTERN_DISABLED);

	// Disable the training pattern on the source
	if (info.dceMajor >= 4 || !dp->trainingUseEncoder) {
		encoder_dig_setup(connectorIndex, mode->timing.pixel_clock,
			ATOM_ENCODER_CMD_DP_LINK_TRAINING_COMPLETE);
	} else {
        dp_encoder_service(connectorIndex, ATOM_DP_ACTION_TRAINING_COMPLETE,
			dp->linkRate, 0);
	}

	return B_OK;
}


bool
ddc2_dp_read_edid1(uint32 connectorIndex, edid1_info* edid)
{
	TRACE("%s\n", __func__);

	dp_info* dpInfo = &gConnector[connectorIndex]->dpInfo;

	if (!dpInfo->valid) {
		ERROR("%s: connector(%" B_PRIu32 ") missing valid DisplayPort data!\n",
			__func__, connectorIndex);
		return false;
	}

	edid1_raw raw;
	uint8* rdata = (uint8*)&raw;
	uint8 sdata = 0;

	// The following sequence is from a trace of the Linux kernel
	// radeon code; not sure if the initial writes to address 0 are
	// requried.
	// TODO: This surely cane be cleaned up
	dp_aux_set_i2c_byte(connectorIndex, 0x00, &sdata, true, false);
	dp_aux_set_i2c_byte(connectorIndex, 0x00, &sdata, false, true);

	dp_aux_set_i2c_byte(connectorIndex, 0x50, &sdata, true, false);
	dp_aux_set_i2c_byte(connectorIndex, 0x50, &sdata, false, false);
	dp_aux_get_i2c_byte(connectorIndex, 0x50, rdata, true, false);
	dp_aux_get_i2c_byte(connectorIndex, 0x50, rdata, false, false);
	dp_aux_get_i2c_byte(connectorIndex, 0x50, rdata, false, true);

	dp_aux_set_i2c_byte(connectorIndex, 0x50, &sdata, true, false);
	dp_aux_set_i2c_byte(connectorIndex, 0x50, &sdata, false, false);
	dp_aux_get_i2c_byte(connectorIndex, 0x50, rdata, true, false);

	for (uint32 i = 0; i < sizeof(raw); i++) {
		status_t result = dp_aux_get_i2c_byte(connectorIndex, 0x50,
			rdata++, false, false);
		if (result != B_OK) {
			TRACE("%s: error reading EDID data at index %" B_PRIu32 ", "
				"result = 0x%" B_PRIx32 "\n", __func__, i, result);
			dp_aux_get_i2c_byte(connectorIndex, 0x50, &sdata, false, true);
			return false;
		}
	}
	dp_aux_get_i2c_byte(connectorIndex, 0x50, &sdata, false, true);

	if (raw.version.version != 1 || raw.version.revision > 4) {
		ERROR("%s: EDID version or revision out of range\n", __func__);
		return false;
	}

	edid_decode(edid, &raw);

	return true;
}


status_t
dp_get_pixel_size_for(color_space space, size_t *pixelChunk,
	size_t *rowAlignment, size_t *pixelsPerChunk)
{
	status_t result = get_pixel_size_for(space, pixelChunk, NULL,
		pixelsPerChunk);

	if ((space == B_RGB32) || (space == B_RGBA32) || (space == B_RGB32_BIG)
		|| (space == B_RGBA32_BIG)) {
		*pixelChunk = 3;
	}

	return result;
}


bool
dp_is_dp12_capable(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;

	uint32 capabilities = gConnector[connectorIndex]->encoder.capabilities;

	if (info.dceMajor >= 5
		&& gInfo->dpExternalClock >= 539000
		&& (capabilities & ATOM_ENCODER_CAP_RECORD_HBR2) != 0) {
		return true;
	}

	return false;
}


void
debug_dp_info()
{
	ERROR("Current DisplayPort Info =================\n");
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			dp_info* dp = &gConnector[id]->dpInfo;
			ERROR("Connector #%" B_PRIu32 ") DP: %s\n", id,
				dp->valid ? "true" : "false");

			if (!dp->valid)
				continue;
			ERROR(" + DP Config Data\n");
			ERROR("   - max lane count:          %d\n",
				dp->config[DP_MAX_LANE_COUNT] & DP_MAX_LANE_COUNT_MASK);
			ERROR("   - max link rate:           %d\n",
				dp->config[DP_MAX_LINK_RATE]);
			ERROR("   - receiver port count:     %d\n",
				dp->config[DP_NORP] & DP_NORP_MASK);
			ERROR("   - downstream port present: %s\n",
				(dp->config[DP_DOWNSTREAMPORT] & DP_DOWNSTREAMPORT_EN)
				? "yes" : "no");
			ERROR("   - downstream port count:   %d\n",
				dp->config[DP_DOWNSTREAMPORT_COUNT]
				& DP_DOWNSTREAMPORT_COUNT_MASK);
			ERROR(" + Training\n");
			ERROR("   - use encoder:             %s\n",
				dp->trainingUseEncoder ? "true" : "false");
			ERROR("   - attempts:                %" B_PRIu8 "\n",
				dp->trainingAttempts);
			ERROR("   - delay:                   %d\n",
				dp->trainingReadInterval);
			ERROR(" + Data\n");
			ERROR("   - auxPin:                  0x%" B_PRIX32"\n", dp->auxPin);
			ERROR(" + Video\n");
			ERROR("   - laneCount:               %d\n", dp->laneCount);
			ERROR("   - linkRate:                %" B_PRIu32 "\n",
				dp->linkRate);
		}
	}
	ERROR("==========================================\n");
}
