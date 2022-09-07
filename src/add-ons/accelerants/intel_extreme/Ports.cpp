/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Rudolf Cornelissen, ruud@highsand-juicylake.nl
 */


#include "Ports.h"

#include <ddc.h>
#include <dp_raw.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <KernelExport.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "FlexibleDisplayInterface.h"
#include "intel_extreme.h"
#include "PanelFitter.h"

#include <new>


#undef TRACE
#define TRACE_PORTS
#ifdef TRACE_PORTS
#   define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#   define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static bool
wait_for_set(addr_t address, uint32 mask, uint32 timeout)
{
	int interval = 50;
	uint32 i = 0;
	for(i = 0; i <= timeout; i += interval) {
		spin(interval);
		if ((read32(address) & mask) != 0)
			return true;
	}
	return false;
}


static bool
wait_for_clear(addr_t address, uint32 mask, uint32 timeout)
{
	int interval = 50;
	uint32 i = 0;
	for(i = 0; i <= timeout; i += interval) {
		spin(interval);
		if ((read32(address) & mask) == 0)
			return true;
	}
	return false;
}


static uint32
wait_for_clear_status(addr_t address, uint32 mask, uint32 timeout)
{
	int interval = 50;
	uint32 i = 0;
	uint32 status = 0;
	for(i = 0; i <= timeout; i += interval) {
		spin(interval);
		status = read32(address);
		if ((status & mask) == 0)
			return status;
	}
	return status;
}


Port::Port(port_index index, const char* baseName)
	:
	fPipe(NULL),
	fEDIDState(B_NO_INIT),
	fPortIndex(index),
	fPortName(NULL)
{
	char portID[2];
	portID[0] = 'A' + index - INTEL_PORT_A;
	portID[1] = 0;

	char buffer[32];
	buffer[0] = 0;

	strlcat(buffer, baseName, sizeof(buffer));
	strlcat(buffer, " ", sizeof(buffer));
	strlcat(buffer, portID, sizeof(buffer));
	fPortName = strdup(buffer);
}


Port::~Port()
{
	free(fPortName);
}


bool
Port::HasEDID()
{
	if (fEDIDState == B_NO_INIT)
		GetEDID(NULL);

	return fEDIDState == B_OK;
}


status_t
Port::SetPipe(Pipe* pipe)
{
	CALLED();

	if (pipe == NULL) {
		ERROR("%s: Invalid pipe provided!\n", __func__);
		return B_ERROR;
	}

	uint32 portRegister = _PortRegister();
	if (portRegister == 0) {
		ERROR("%s: Invalid PortRegister ((0x%" B_PRIx32 ") for %s\n", __func__,
			portRegister, PortName());
		return B_ERROR;
	}

	// TODO: UnAssignPipe?  This likely needs reworked a little
	if (fPipe != NULL) {
		ERROR("%s: Can't reassign display pipe (yet)\n", __func__);
		return B_ERROR;
	}

	switch (pipe->Index()) {
		case INTEL_PIPE_B:
			TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe B\n", __func__,
				PortName(), portRegister);
			break;
		case INTEL_PIPE_C:
			TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe C\n", __func__,
				PortName(), portRegister);
			break;
		case INTEL_PIPE_D:
			TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe D\n", __func__,
				PortName(), portRegister);
			break;
		default:
			TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe A\n", __func__,
				PortName(), portRegister);
			break;
	}

	uint32 portState = read32(portRegister);

	// generation 6 gfx SandyBridge/SNB non-DP use the same 2 bits on all ports (eDP = 1 bit).
	// generation 7 gfx IvyBridge/IVB non-DP use the same 2 bits on all ports (eDP = 2 bits).
	// DP ports/all DDI ports: Pipe selections works differently, via own SetPipe() implementation.
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		portState &= ~PORT_TRANS_SEL_MASK;
		switch (pipe->Index()) {
			case INTEL_PIPE_B:
				write32(portRegister, portState | PORT_TRANS_B_SEL_CPT);
				break;
			case INTEL_PIPE_C:
				write32(portRegister, portState | PORT_TRANS_C_SEL_CPT);
				break;
			default:
				write32(portRegister, portState | PORT_TRANS_A_SEL_CPT);
				break;
		}
	} else {
		// generation 3/4/5 gfx uses the same single bit on all ports
		if (pipe->Index() == INTEL_PIPE_A)
			write32(portRegister, portState & ~DISPLAY_MONITOR_PIPE_B);
		else
			write32(portRegister, portState | DISPLAY_MONITOR_PIPE_B);
	}
	fPipe = pipe;

	if (fPipe == NULL)
		return B_NO_MEMORY;

	// Disable display pipe until modesetting enables it
	if (fPipe->IsEnabled())
		fPipe->Enable(false);

	read32(portRegister);

	return B_OK;
}


status_t
Port::Power(bool enabled)
{
	if (fPipe == NULL) {
		ERROR("%s: Setting power mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	fPipe->Enable(enabled);

	return B_OK;
}


status_t
Port::GetEDID(edid1_info* edid, bool forceRead)
{
	CALLED();

	if (fEDIDState == B_NO_INIT || forceRead) {
		TRACE("%s: trying to read EDID\n", PortName());

		i2c_bus bus;
		if (SetupI2c(&bus) != B_OK)
			return fEDIDState;

		fEDIDState = ddc2_read_edid1(&bus, &fEDIDInfo, NULL, NULL);

		if (fEDIDState == B_OK) {
			TRACE("%s: found EDID information!\n", PortName());
			edid_dump(&fEDIDInfo);
		} else if (SetupI2cFallback(&bus) == B_OK) {
			fEDIDState = ddc2_read_edid1(&bus, &fEDIDInfo, NULL, NULL);

			if (fEDIDState == B_OK) {
				TRACE("%s: found EDID information!\n", PortName());
				edid_dump(&fEDIDInfo);
			}
		}
	}

	if (fEDIDState != B_OK) {
		TRACE("%s: no EDID information found.\n", PortName());
		return fEDIDState;
	}

	if (edid != NULL)
		memcpy(edid, &fEDIDInfo, sizeof(edid1_info));

	return B_OK;
}


status_t
Port::SetupI2c(i2c_bus *bus)
{
	addr_t ddcRegister = _DDCRegister();
	if (ddcRegister == 0) {
		TRACE("%s: no DDC register found\n", PortName());
		fEDIDState = B_ERROR;
		return fEDIDState;
	}

	TRACE("%s: using ddc @ 0x%" B_PRIxADDR "\n", PortName(), ddcRegister);

	ddc2_init_timing(bus);
	bus->cookie = (void*)ddcRegister;
	bus->set_signals = &_SetI2CSignals;
	bus->get_signals = &_GetI2CSignals;

	return B_OK;
}


status_t
Port::SetupI2cFallback(i2c_bus *bus)
{
	return B_ERROR;
}


status_t
Port::GetPLLLimits(pll_limits& limits)
{
	return B_ERROR;
}


pipe_index
Port::PipePreference()
{
	CALLED();
	// Ideally we could just return INTEL_PIPE_ANY for all devices by default, but
	// this doesn't quite work yet. We need to use the BIOS presetup pipes for now.
	if (gInfo->shared_info->device_type.Generation() < 4)
		return INTEL_PIPE_ANY;

	// Notes:
	// - The BIOSes seen sofar do not use PIPE C by default.
	// - The BIOSes seen sofar program transcoder A to PIPE A, etc.
	// - Later devices add a pipe C alongside the added transcoder C.

	if ((gInfo->shared_info->device_type.Generation() <= 7) &&
		(!gInfo->shared_info->device_type.HasDDI())) {
		uint32 portState = read32(_PortRegister());
		if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
			portState &= PORT_TRANS_SEL_MASK;
			if (portState == PORT_TRANS_B_SEL_CPT)
				return INTEL_PIPE_B;
			else
				return INTEL_PIPE_A;
		} else {
			if (portState & DISPLAY_MONITOR_PIPE_B)
				return INTEL_PIPE_B;
			else
				return INTEL_PIPE_A;
		}
	}

	if (gInfo->shared_info->device_type.HasDDI()) {
		// scan all our pipes to find the one connected to the current port
		uint32 pipeState = 0;
		for (uint32 pipeCnt = 0; pipeCnt < 4; pipeCnt++) {
			switch (pipeCnt) {
				case 0:
					pipeState = read32(PIPE_DDI_FUNC_CTL_A);
					break;
				case 1:
					pipeState = read32(PIPE_DDI_FUNC_CTL_B);
					break;
				case 2:
					pipeState = read32(PIPE_DDI_FUNC_CTL_C);
					break;
				default:
					pipeState = read32(PIPE_DDI_FUNC_CTL_EDP);
					break;
			}

			if ((((pipeState & PIPE_DDI_SELECT_MASK) >> PIPE_DDI_SELECT_SHIFT) + 1)
				== (uint32)PortIndex()) {
				switch (pipeCnt) {
					case 0:
						return INTEL_PIPE_A;
					case 1:
						return INTEL_PIPE_B;
					case 2:
						return INTEL_PIPE_C;
					default:
						return INTEL_PIPE_D;
				}
			}
		}
	}

	return INTEL_PIPE_ANY;
}


status_t
Port::_GetI2CSignals(void* cookie, int* _clock, int* _data)
{
	addr_t ioRegister = (addr_t)cookie;
	uint32 value = read32(ioRegister);

	*_clock = (value & I2C_CLOCK_VALUE_IN) != 0;
	*_data = (value & I2C_DATA_VALUE_IN) != 0;

	return B_OK;
}


status_t
Port::_SetI2CSignals(void* cookie, int clock, int data)
{
	addr_t ioRegister = (addr_t)cookie;
	uint32 value;

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_83x)) {
		// on these chips, the reserved values are fixed
		value = 0;
	} else {
		// on all others, we have to preserve them manually
		value = read32(ioRegister) & I2C_RESERVED;
	}

	// if we send clk or data, we always send low logic level;
	// if we want to send high level, we actually receive and let the
	// external pullup resistors create the high level on the bus.
	value |= I2C_DATA_VALUE_MASK;  //sets data = 0, always latch
	value |= I2C_CLOCK_VALUE_MASK; //sets clock = 0, always latch

	if (data != 0)
		value |= I2C_DATA_DIRECTION_MASK;
	else {
		value |= I2C_DATA_DIRECTION_MASK | I2C_DATA_DIRECTION_OUT;
	}

	if (clock != 0)
		value |= I2C_CLOCK_DIRECTION_MASK;
	else {
		value |= I2C_CLOCK_DIRECTION_MASK | I2C_CLOCK_DIRECTION_OUT;
	}

	write32(ioRegister, value);
	read32(ioRegister);
		// make sure the PCI bus has flushed the write

	return B_OK;
}


bool
Port::_IsPortInVBT(uint32* foundIndex)
{
	// check VBT mapping
	bool found = false;
	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	for (uint32 i = 0; i < deviceConfigCount; i++) {
		child_device_config& config = gInfo->shared_info->device_configs[i];
		if (config.dvo_port > DVO_PORT_HDMII) {
			ERROR("%s: DVO port unknown\n", __func__);
			continue;
		}
		dvo_port port = (dvo_port)config.dvo_port;
		switch (PortIndex()) {
			case INTEL_PORT_A:
				found = port == DVO_PORT_HDMIA || port == DVO_PORT_DPA;
				break;
			case INTEL_PORT_B:
				found = port == DVO_PORT_HDMIB || port == DVO_PORT_DPB;
				break;
			case INTEL_PORT_C:
				found = port == DVO_PORT_HDMIC || port == DVO_PORT_DPC;
				break;
			case INTEL_PORT_D:
				found = port == DVO_PORT_HDMID || port == DVO_PORT_DPD;
				break;
			case INTEL_PORT_E:
				found = port == DVO_PORT_HDMIE || port == DVO_PORT_DPE || port == DVO_PORT_CRT;
				break;
			case INTEL_PORT_F:
				found = port == DVO_PORT_HDMIF || port == DVO_PORT_DPF;
				break;
			case INTEL_PORT_G:
				found = port == DVO_PORT_HDMIG || port == DVO_PORT_DPG;
				break;
			default:
				ERROR("%s: DDI port unknown\n", __func__);
				break;
		}
		if (found) {
			if (foundIndex != NULL)
				*foundIndex = i;
			break;
		}
	}
	return found;
}


bool
Port::_IsDisplayPortInVBT()
{
	uint32 foundIndex = 0;
	if (!_IsPortInVBT(&foundIndex))
		return false;
	child_device_config& config = gInfo->shared_info->device_configs[foundIndex];
	return config.aux_channel > 0 && (config.device_type & DEVICE_TYPE_DISPLAYPORT_OUTPUT) != 0;
}


bool
Port::_IsHdmiInVBT()
{
	uint32 foundIndex = 0;
	if (!_IsPortInVBT(&foundIndex))
		return false;
	child_device_config& config = gInfo->shared_info->device_configs[foundIndex];
	return config.ddc_pin > 0 && ((config.device_type & DEVICE_TYPE_NOT_HDMI_OUTPUT) == 0
			|| (config.device_type & DEVICE_TYPE_TMDS_DVI_SIGNALING) != 0);
}


bool
Port::_IsEDPPort()
{
	uint32 foundIndex = 0;
	if (!_IsPortInVBT(&foundIndex))
		return false;
	child_device_config& config = gInfo->shared_info->device_configs[foundIndex];
	return (config.device_type & (DEVICE_TYPE_INTERNAL_CONNECTOR | DEVICE_TYPE_DISPLAYPORT_OUTPUT))
		== (DEVICE_TYPE_INTERNAL_CONNECTOR | DEVICE_TYPE_DISPLAYPORT_OUTPUT);
}


addr_t
Port::_DDCPin()
{
	uint32 foundIndex = 0;
	if (!_IsPortInVBT(&foundIndex))
		return 0;
	child_device_config& config = gInfo->shared_info->device_configs[foundIndex];
	if (gInfo->shared_info->pch_info >= INTEL_PCH_ICP) {
		switch (config.ddc_pin) {
			case 1:
				return INTEL_I2C_IO_A;
			case 2:
				return INTEL_I2C_IO_B;
			case 3:
				return INTEL_I2C_IO_C;
			case 4:
				return INTEL_I2C_IO_I;
			case 5:
				return INTEL_I2C_IO_J;
			case 6:
				return INTEL_I2C_IO_K;
			case 7:
				return INTEL_I2C_IO_L;
			case 8:
				return INTEL_I2C_IO_M;
			case 9:
				return INTEL_I2C_IO_N;
			default:
				return 0;
		}
	} else if (gInfo->shared_info->pch_info >= INTEL_PCH_CNP) {
		switch (config.ddc_pin) {
			case 1:
				return INTEL_I2C_IO_A;
			case 2:
				return INTEL_I2C_IO_B;
			case 3:
				return INTEL_I2C_IO_D;
			case 4:
				return INTEL_I2C_IO_C;
			default:
				return 0;
		}
	} else if (gInfo->shared_info->device_type.Generation() == 9) {
		switch (config.ddc_pin) {
			case 4:
				return INTEL_I2C_IO_D;
			case 5:
				return INTEL_I2C_IO_E;
			case 6:
				return INTEL_I2C_IO_F;
			default:
				return 0;
		}
	} else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_BDW)) {
		switch (config.ddc_pin) {
			case 2:
				return INTEL_I2C_IO_A;
			case 4:
				return INTEL_I2C_IO_D;
			case 5:
				return INTEL_I2C_IO_E;
			case 6:
				return INTEL_I2C_IO_F;
			default:
				return 0;
		}
	} else {
		switch (config.ddc_pin) {
			case 1:
				return INTEL_I2C_IO_B;
			case 2:
				return INTEL_I2C_IO_A;
			case 3:
				return INTEL_I2C_IO_C;
			case 4:
				return INTEL_I2C_IO_D;
			case 5:
				return INTEL_I2C_IO_E;
			case 6:
				return INTEL_I2C_IO_F;
			default:
				return 0;
		}
	}

}


status_t
Port::_SetupDpAuxI2c(i2c_bus *bus)
{
	CALLED();

	ddc2_init_timing(bus);
	bus->cookie = this;
	bus->send_receive = &_DpAuxSendReceiveHook;

	if (gInfo->shared_info->device_type.Generation() >= 11) {
		uint32 value = read32(ICL_PWR_WELL_CTL_AUX2);
		if ((value & HSW_PWR_WELL_CTL_STATE(0)) != 0)
			return B_OK;

		write32(ICL_PWR_WELL_CTL_AUX2, value | HSW_PWR_WELL_CTL_REQ(0));
		if (!wait_for_set(ICL_PWR_WELL_CTL_AUX2, HSW_PWR_WELL_CTL_STATE(0), 1000))
			ERROR("%s: %s AUX didn't power on within 1000us!\n", __func__, PortName());
	}
	return B_OK;
}


status_t
Port::_DpAuxSendReceive(uint32 slaveAddress,
	const uint8 *writeBuffer, size_t writeLength, uint8 *readBuffer, size_t readLength)
{
	size_t transferLength = 16;

	dp_aux_msg message;
	memset(&message, 0, sizeof(message));

	if (writeBuffer != NULL) {
		message.address = slaveAddress;
		message.buffer = NULL;
		message.request = DP_AUX_I2C_WRITE;
		message.size = 0;
		ssize_t result = _DpAuxTransfer(&message);
		if (result < 0)
			return result;

		for (size_t i = 0; i < writeLength;) {
			message.buffer = (void*)(writeBuffer + i);
			message.size = min_c(transferLength, writeLength - i);
			// Middle-Of-Transmission on final transaction
			if (writeLength - i > transferLength)
				message.request |= DP_AUX_I2C_MOT;
			else
				message.request &= ~DP_AUX_I2C_MOT;

			for (int attempt = 0; attempt < 7; attempt++) {
				ssize_t result = _DpAuxTransfer(&message);
				if (result < 0) {
					ERROR("%s: aux_ch transaction failed!\n", __func__);
					return result;
				}

				switch (message.reply & DP_AUX_I2C_REPLY_MASK) {
					case DP_AUX_I2C_REPLY_ACK:
						goto nextWrite;
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
nextWrite:
			if (result < 0)
				return result;
			i += message.size;
		}
	}


	if (readBuffer != NULL) {
		message.address = slaveAddress;
		message.buffer = NULL;
		message.request = DP_AUX_I2C_READ;
		message.size = 0;
		ssize_t result = _DpAuxTransfer(&message);
		if (result < 0)
			return result;

		for (size_t i = 0; i < readLength;) {
			message.buffer = readBuffer + i;
			message.size = min_c(transferLength, readLength - i);
			// Middle-Of-Transmission on final transaction
			if (readLength - i > transferLength)
				message.request |= DP_AUX_I2C_MOT;
			else
				message.request &= ~DP_AUX_I2C_MOT;

			for (int attempt = 0; attempt < 7; attempt++) {
				result = _DpAuxTransfer(&message);
				if (result < 0) {
					ERROR("%s: aux_ch transaction failed!\n", __func__);
					return result;
				}

				switch (message.reply & DP_AUX_I2C_REPLY_MASK) {
					case DP_AUX_I2C_REPLY_ACK:
						goto nextRead;
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
nextRead:
			if (result < 0)
				return result;
			if (result == 0)
				i += message.size;
		}
	}

	return B_OK;
}


status_t
Port::_DpAuxSendReceiveHook(const struct i2c_bus *bus, uint32 slaveAddress,
	const uint8 *writeBuffer, size_t writeLength, uint8 *readBuffer, size_t readLength)
{
	CALLED();
	Port* port = (Port*)bus->cookie;
	return port->_DpAuxSendReceive(slaveAddress, writeBuffer, writeLength, readBuffer, readLength);
}


ssize_t
Port::_DpAuxTransfer(dp_aux_msg* message)
{
	CALLED();
	if (message == NULL) {
		ERROR("%s: DP message is invalid!\n", __func__);
		return B_ERROR;
	}

	if (message->size > 16) {
		ERROR("%s: Too many bytes! (%" B_PRIuSIZE ")\n", __func__,
			message->size);
		return B_ERROR;
	}

	uint8 transmitSize = message->size > 0 ? 4 : 3;
	uint8 receiveSize;

	switch(message->request & ~DP_AUX_I2C_MOT) {
		case DP_AUX_NATIVE_WRITE:
		case DP_AUX_I2C_WRITE:
		case DP_AUX_I2C_WRITE_STATUS_UPDATE:
			transmitSize += message->size;
			break;
	}

	// If not bare address, check for buffer
	if (message->size > 0 && message->buffer == NULL) {
		ERROR("%s: DP message uninitalized buffer!\n", __func__);
		return B_ERROR;
	}

	uint8 receiveBuffer[20];
	uint8 transmitBuffer[20];
	transmitBuffer[0] = (message->request << 4) | ((message->address >> 16) & 0xf);
	transmitBuffer[1] = (message->address >> 8) & 0xff;
	transmitBuffer[2] = message->address & 0xff;
	transmitBuffer[3] = message->size != 0 ? (message->size - 1) : 0;

	uint8 retry;
	for (retry = 0; retry < 7; retry++) {
		ssize_t result = B_ERROR;
		switch(message->request & ~DP_AUX_I2C_MOT) {
			case DP_AUX_NATIVE_WRITE:
			case DP_AUX_I2C_WRITE:
			case DP_AUX_I2C_WRITE_STATUS_UPDATE:
				receiveSize = 2;
				if (message->buffer != NULL)
					memcpy(transmitBuffer + 4, message->buffer, message->size);
				result = _DpAuxTransfer(transmitBuffer,
					transmitSize, receiveBuffer, receiveSize);
				if (result > 0) {
					message->reply = receiveBuffer[0] >> 4;
					if (result > 1)
						result = min_c(receiveBuffer[1], message->size);
					else
						result = message->size;
				}
				break;
			case DP_AUX_NATIVE_READ:
			case DP_AUX_I2C_READ:
				receiveSize = message->size + 1;
				result = _DpAuxTransfer(transmitBuffer,
					transmitSize, receiveBuffer, receiveSize);
				if (result > 0) {
					message->reply = receiveBuffer[0] >> 4;
					result--;
					if (message->buffer != NULL)
						memcpy(message->buffer, receiveBuffer + 1, result);
				}
				break;
			default:
				ERROR("%s: Unknown dp_aux_msg request!\n", __func__);
				return B_ERROR;
		}

		if (result == B_BUSY)
			continue;
		else if (result < B_OK)
			return result;

		switch (message->reply & DP_AUX_NATIVE_REPLY_MASK) {
			case DP_AUX_NATIVE_REPLY_ACK:
				return B_OK;
			case DP_AUX_NATIVE_REPLY_NACK:
				TRACE("%s: aux native reply nack\n", __func__);
				return B_IO_ERROR;
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


ssize_t
Port::_DpAuxTransfer(uint8* transmitBuffer, uint8 transmitSize,
	uint8* receiveBuffer, uint8 receiveSize)
{
	addr_t channelControl;
	addr_t channelData[5];
	aux_channel channel = _DpAuxChannel();
	TRACE("%s: %s DpAuxChannel: 0x%x\n", __func__, PortName(), channel);
	if (gInfo->shared_info->device_type.Generation() >= 9
		|| (gInfo->shared_info->pch_info != INTEL_PCH_NONE && channel == AUX_CH_A)) {
		channelControl = DP_AUX_CH_CTL(channel);
		for (int i = 0; i < 5; i++)
			channelData[i] = DP_AUX_CH_DATA(channel, i);
	} else if (gInfo->shared_info->pch_info != INTEL_PCH_NONE) {
		channelControl = PCH_DP_AUX_CH_CTL(channel);
		for (int i = 0; i < 5; i++)
			channelData[i] = PCH_DP_AUX_CH_DATA(channel, i);
	} else {
		ERROR("Port::_DpAuxTransfer() unknown register config\n");
		return B_BUSY;
	}
	if (transmitSize > 20 || receiveSize > 20)
		return E2BIG;

	int tries = 0;
	while ((read32(channelControl) & INTEL_DP_AUX_CTL_BUSY) != 0) {
		if (tries++ == 3) {
			ERROR("%s: %s AUX channel is busy!\n", __func__, PortName());
			return B_BUSY;
		}
		snooze(1000);
	}

	uint32 sendControl = 0;
	if (gInfo->shared_info->device_type.Generation() >= 9) {
		sendControl = INTEL_DP_AUX_CTL_BUSY | INTEL_DP_AUX_CTL_DONE | INTEL_DP_AUX_CTL_INTERRUPT
			| INTEL_DP_AUX_CTL_TIMEOUT_ERROR | INTEL_DP_AUX_CTL_TIMEOUT_1600us | INTEL_DP_AUX_CTL_RECEIVE_ERROR
			| (transmitSize << INTEL_DP_AUX_CTL_MSG_SIZE_SHIFT) | INTEL_DP_AUX_CTL_FW_SYNC_PULSE_SKL(32)
			| INTEL_DP_AUX_CTL_SYNC_PULSE_SKL(32);
	} else {
		uint32 frequency = gInfo->shared_info->hw_cdclk;
		if (channel != AUX_CH_A)
			frequency = gInfo->shared_info->hraw_clock;
		uint32 aux_clock_divider = (frequency + 2000 / 2) / 2000;
		if (gInfo->shared_info->pch_info == INTEL_PCH_LPT && channel != AUX_CH_A)
			aux_clock_divider = 0x48; // or 0x3f
		uint32 timeout = INTEL_DP_AUX_CTL_TIMEOUT_400us;
		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_BDW))
			timeout = INTEL_DP_AUX_CTL_TIMEOUT_600us;
		sendControl = INTEL_DP_AUX_CTL_BUSY | INTEL_DP_AUX_CTL_DONE | INTEL_DP_AUX_CTL_INTERRUPT
			| INTEL_DP_AUX_CTL_TIMEOUT_ERROR | timeout | INTEL_DP_AUX_CTL_RECEIVE_ERROR
			| (transmitSize << INTEL_DP_AUX_CTL_MSG_SIZE_SHIFT) | (3 << INTEL_DP_AUX_CTL_PRECHARGE_2US_SHIFT)
			| (aux_clock_divider << INTEL_DP_AUX_CTL_BIT_CLOCK_2X_SHIFT);
	}

	uint8 retry;
	uint32 status = 0;
	for (retry = 0; retry < 5; retry++) {
		for (uint8 i = 0; i < transmitSize;) {
			uint8 index = i / 4;
			uint32 data = ((uint32)transmitBuffer[i++]) << 24;
			if (i < transmitSize)
				data |= ((uint32)transmitBuffer[i++]) << 16;
			if (i < transmitSize)
				data |= ((uint32)transmitBuffer[i++]) << 8;
			if (i < transmitSize)
				data |= transmitBuffer[i++];
			write32(channelData[index], data);
		}
		write32(channelControl, sendControl);

		// wait 10 ms reading channelControl until INTEL_DP_AUX_CTL_BUSY
		status = wait_for_clear_status(channelControl, INTEL_DP_AUX_CTL_BUSY, 10000);
		if ((status & INTEL_DP_AUX_CTL_BUSY) != 0) {
			ERROR("%s: %s AUX channel stayed busy for 10000us!\n", __func__, PortName());
		}

		write32(channelControl, status | INTEL_DP_AUX_CTL_DONE | INTEL_DP_AUX_CTL_TIMEOUT_ERROR
			| INTEL_DP_AUX_CTL_RECEIVE_ERROR);
		if ((status & INTEL_DP_AUX_CTL_TIMEOUT_ERROR) != 0)
			continue;
		if ((status & INTEL_DP_AUX_CTL_RECEIVE_ERROR) != 0) {
			snooze(400);
			continue;
		}
		if ((status & INTEL_DP_AUX_CTL_DONE) != 0)
			goto done;
	}

	if ((status & INTEL_DP_AUX_CTL_DONE) == 0) {
		ERROR("%s: Busy Error. %" B_PRIu8 " attempts\n", __func__, retry);
		return B_BUSY;
	}
done:
	if ((status & INTEL_DP_AUX_CTL_RECEIVE_ERROR) != 0)
		return B_IO_ERROR;
	if ((status & INTEL_DP_AUX_CTL_TIMEOUT_ERROR) != 0)
		return B_TIMEOUT;

	uint8 bytes = (status & INTEL_DP_AUX_CTL_MSG_SIZE_MASK) >> INTEL_DP_AUX_CTL_MSG_SIZE_SHIFT;
	if (bytes == 0 || bytes > 20) {
		ERROR("%s: Status byte count incorrect %u\n", __func__, bytes);
		return B_BUSY;
	}
	if (bytes > receiveSize)
		bytes = receiveSize;
	for (uint8 i = 0; i < bytes;) {
		uint32 data = read32(channelData[i / 4]);
		receiveBuffer[i++] = data >> 24;
		if (i < bytes)
			receiveBuffer[i++] = data >> 16;
		if (i < bytes)
			receiveBuffer[i++] = data >> 8;
		if (i < bytes)
			receiveBuffer[i++] = data;
	}

	return bytes;
}


aux_channel
Port::_DpAuxChannel()
{
	uint32 foundIndex = 0;
	if (!_IsPortInVBT(&foundIndex))
		return AUX_CH_A;
	child_device_config& config = gInfo->shared_info->device_configs[foundIndex];
	switch (config.aux_channel) {
		case DP_AUX_B:
			return AUX_CH_B;
		case DP_AUX_C:
			return AUX_CH_C;
		case DP_AUX_D:
			return AUX_CH_D;
		case DP_AUX_E:
			return AUX_CH_E;
		case DP_AUX_F:
			return AUX_CH_F;
		default:
			return AUX_CH_A;
	}
}


// #pragma mark - Analog Port


AnalogPort::AnalogPort()
	:
	Port(INTEL_PORT_A, "Analog")
{
}


bool
AnalogPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());
	return HasEDID();
}


addr_t
AnalogPort::_DDCRegister()
{
	// always fixed
	return INTEL_I2C_IO_A;
}


addr_t
AnalogPort::_PortRegister()
{
	// always fixed
	return INTEL_ANALOG_PORT;
}


status_t
AnalogPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	CALLED();
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->timing.h_display,
		target->timing.v_display);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	// Setup PanelFitter and Train FDI if it exists
	PanelFitter* fitter = fPipe->PFT();
	if (fitter != NULL)
		fitter->Enable(target->timing);
	FDILink* link = fPipe->FDI();
	if (link != NULL) {
		uint32 lanes = 0;
		uint32 linkBandwidth = 0;
		uint32 bitsPerPixel = 0;
		link->PreTrain(&target->timing, &linkBandwidth, &lanes, &bitsPerPixel);
		fPipe->SetFDILink(target->timing, linkBandwidth, lanes, bitsPerPixel);
		link->Train(&target->timing, lanes);
	}
	pll_divisors divisors;
	compute_pll_divisors(&target->timing, &divisors, false);

	uint32 extraPLLFlags = 0;
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_NORMAL;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's
	fPipe->ConfigureClocks(divisors, target->timing.pixel_clock, extraPLLFlags);

	write32(_PortRegister(), (read32(_PortRegister())
		& ~(DISPLAY_MONITOR_POLARITY_MASK | DISPLAY_MONITOR_VGA_POLARITY))
		| ((target->timing.flags & B_POSITIVE_HSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
		| ((target->timing.flags & B_POSITIVE_VSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

	// Program target display mode
	fPipe->ConfigureTimings(target);

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - LVDS Panel


LVDSPort::LVDSPort()
	:
	Port(INTEL_PORT_C, "LVDS")
{
	// Always unlock LVDS port as soon as we start messing with it.
	uint32 panelControl = INTEL_PANEL_CONTROL;
	if (gInfo->shared_info->pch_info != INTEL_PCH_NONE) {
		// FIXME writing there results in black screen on SandyBridge
		return;
		// panelControl = PCH_PANEL_CONTROL;
	}
	write32(panelControl, read32(panelControl) | PANEL_REGISTER_UNLOCK);
}


pipe_index
LVDSPort::PipePreference()
{
	CALLED();
	// Older devices have hardcoded pipe/port mappings, so just use that
	if (gInfo->shared_info->device_type.Generation() < 4)
		return INTEL_PIPE_B;

	// Ideally we could just return INTEL_PIPE_ANY for the newer devices, but
	// this doesn't quite work yet.

	// On SandyBridge and later, there is a transcoder C. On SandyBridge at least
	// that can't be used by the LVDS port (but A and B would be fine).
	// On Ibex Point, SandyBridge and IvyBridge (tested) changing pipes does not
	// work yet.
	// Notes:
	// - Switching Pipes only works reliably when a 'full modeswitch' is executed
	//   (FDI training) so we have to reuse the BIOS preset setup always for now.
	// - The BIOSes seen sofar do not use PIPE C by default.
	// - The BIOSes seen sofar program transcoder A to PIPE A, etc.
	// - Later devices add a pipe C alongside the added transcoder C.

	// FIXME How's this setup in newer gens? Currently return Pipe B fixed there..
	if (gInfo->shared_info->device_type.Generation() <= 7) {
		uint32 portState = read32(_PortRegister());
		if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
			portState &= PORT_TRANS_SEL_MASK;
			if (portState == PORT_TRANS_B_SEL_CPT)
				return INTEL_PIPE_B;
			else
				return INTEL_PIPE_A;
		} else {
			if (portState & DISPLAY_MONITOR_PIPE_B)
				return INTEL_PIPE_B;
			else
				return INTEL_PIPE_A;
		}
	}

	return INTEL_PIPE_B;
}


bool
LVDSPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());

	if (gInfo->shared_info->pch_info != INTEL_PCH_NONE) {
		uint32 registerValue = read32(_PortRegister());
		// there's a detection bit we can use
		if ((registerValue & PCH_LVDS_DETECTED) == 0) {
			TRACE("LVDS: Not detected\n");
			return false;
		}
		// TODO: Skip if eDP support
	} else if (gInfo->shared_info->device_type.Generation() <= 4) {
		// Older generations don't have LVDS detection. If not mobile skip.
		if (!gInfo->shared_info->device_type.IsMobile()) {
			TRACE("LVDS: Skipping LVDS detection due to gen and not mobile\n");
			return false;
		}
		// If mobile, try to grab EDID
		// Linux seems to look at lid status for LVDS port detection
		// If we don't get EDID, we can use vbios native mode or vesa?
		if (!HasEDID()) {
			if (gInfo->shared_info->has_vesa_edid_info) {
				TRACE("LVDS: Using VESA edid info\n");
				memcpy(&fEDIDInfo, &gInfo->shared_info->vesa_edid_info,
					sizeof(edid1_info));
				if (fEDIDState != B_OK) {
					fEDIDState = B_OK;
					// HasEDID now true
					edid_dump(&fEDIDInfo);
				}
			} else if (gInfo->shared_info->got_vbt) {
				TRACE("LVDS: No EDID, but force enabled as we have a VBT\n");
				return true;
			} else {
				TRACE("LVDS: Couldn't find any valid EDID!\n");
				return false;
			}
		}
	}

	// Try getting EDID, as the LVDS port doesn't overlap with anything else,
	// we don't run the risk of getting someone else's data.
	return HasEDID();
}


addr_t
LVDSPort::_DDCRegister()
{
	// always fixed
	return INTEL_I2C_IO_C;
}


addr_t
LVDSPort::_PortRegister()
{
	// always fixed
	return INTEL_DIGITAL_LVDS_PORT;
}


status_t
LVDSPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	CALLED();
	if (target == NULL) {
		ERROR("%s: Invalid target mode passed!\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: %s-%d %dx%d\n", __func__, PortName(), PortIndex(),
		target->timing.h_display, target->timing.v_display);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	addr_t panelControl = INTEL_PANEL_CONTROL;
	addr_t panelStatus = INTEL_PANEL_STATUS;
	if (gInfo->shared_info->pch_info != INTEL_PCH_NONE) {
		panelControl = PCH_PANEL_CONTROL;
		panelStatus = PCH_PANEL_STATUS;
	}

	if (gInfo->shared_info->device_type.Generation() != 4) {
		// TODO not needed on any generation if we are using the panel fitter
		// Power off Panel
		write32(panelControl,
			read32(panelControl) & ~PANEL_CONTROL_POWER_TARGET_ON);
		read32(panelControl);

		if (!wait_for_clear(panelStatus, PANEL_STATUS_POWER_ON, 1000)) {
			ERROR("%s: %s didn't power off within 1000ms!\n", __func__,
				PortName());
		}
	}

	// For LVDS panels, we may need to set the timings according to the panel
	// native video mode, and let the panel fitter do the scaling. But the
	// place where the scaling happens varies accross generations of devices.
	display_timing hardwareTarget;
	bool needsScaling = false;

	// TODO figure out how it's done (or if we need to configure something at
	// all) for other generations
	if (gInfo->shared_info->device_type.Generation() <= 6
		&& gInfo->shared_info->device_type.Generation() >= 3
		&& gInfo->shared_info->got_vbt) {
		// Set vbios hardware panel mode as base
		hardwareTarget = gInfo->shared_info->panel_timing;

		if (hardwareTarget.h_display == target->timing.h_display
				&& hardwareTarget.v_display == target->timing.v_display) {
			// We are setting the native video mode, nothing special to do
			// Note: this means refresh and timing might vary according to requested mode.
			hardwareTarget = target->timing;
			TRACE("%s: Setting LVDS to native resolution at %" B_PRIu32 "Hz\n", __func__,
				hardwareTarget.pixel_clock * 1000 / (hardwareTarget.h_total * hardwareTarget.v_total));
		} else {
			// We need to enable the panel fitter
			TRACE("%s: Hardware mode will actually be %dx%d at %" B_PRIu32 "Hz\n", __func__,
				hardwareTarget.h_display, hardwareTarget.v_display,
				hardwareTarget.pixel_clock * 1000 / (hardwareTarget.h_total * hardwareTarget.v_total));

			// FIXME we should also get the refresh frequency from the target
			// mode, and then "sanitize" the resulting mode we made up.
			needsScaling = true;
		}
	} else {
		TRACE("Setting LVDS mode without VBT info or on unhandled hardware "
			"generation, scaling may not work\n");
		// We don't have VBT data, try to set the requested mode directly
		// and hope for the best
		hardwareTarget = target->timing;
	}

	// Setup PanelFitter and Train FDI if it exists
	PanelFitter* fitter = fPipe->PFT();
	if (fitter != NULL)
		fitter->Enable(hardwareTarget);
	FDILink* link = fPipe->FDI();
	if (link != NULL) {
		uint32 lanes = 0;
		uint32 linkBandwidth = 0;
		uint32 bitsPerPixel = 0;
		link->PreTrain(&hardwareTarget, &linkBandwidth, &lanes, &bitsPerPixel);
		fPipe->SetFDILink(hardwareTarget, linkBandwidth, lanes, bitsPerPixel);
		link->Train(&hardwareTarget, lanes);
	}

	pll_divisors divisors;
	compute_pll_divisors(&hardwareTarget, &divisors, true);

	uint32 lvds = read32(_PortRegister())
		| LVDS_PORT_EN | LVDS_A0A2_CLKA_POWER_UP;

	if (gInfo->shared_info->device_type.Generation() == 4) {
		// LVDS_A3_POWER_UP == 24bpp
		// otherwise, 18bpp
		if ((lvds & LVDS_A3_POWER_MASK) != LVDS_A3_POWER_UP)
			lvds |= LVDS_18BIT_DITHER;
	}

	// LVDS on PCH needs set before display enable
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		lvds &= ~PORT_TRANS_SEL_MASK;
		if (fPipe->Index() == INTEL_PIPE_A)
			lvds |= PORT_TRANS_A_SEL_CPT;
		else
			lvds |= PORT_TRANS_B_SEL_CPT;
	}

	// Set the B0-B3 data pairs corresponding to whether we're going to
	// set the DPLLs for dual-channel mode or not.
	if (divisors.p2 == 5 || divisors.p2 == 7) {
		TRACE("LVDS: dual channel\n");
		lvds |= LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP;
	} else {
		TRACE("LVDS: single channel\n");
		lvds &= ~(LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP);
	}

	// LVDS port control moves polarity bits because Intel hates you.
	// Set LVDS sync polarity
	lvds &= ~(LVDS_HSYNC_POLARITY | LVDS_VSYNC_POLARITY);

	// set on - polarity.
	if ((target->timing.flags & B_POSITIVE_HSYNC) == 0)
		lvds |= LVDS_HSYNC_POLARITY;
	if ((target->timing.flags & B_POSITIVE_VSYNC) == 0)
		lvds |= LVDS_VSYNC_POLARITY;

	TRACE("%s: LVDS Write: 0x%" B_PRIx32 "\n", __func__, lvds);
	write32(_PortRegister(), lvds);
	read32(_PortRegister());

	uint32 extraPLLFlags = 0;

	// DPLL mode LVDS for i915+
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_LVDS | DISPLAY_PLL_2X_CLOCK;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's (using the hardware mode timings, since that's what
	// the PLL is used for)
	fPipe->ConfigureClocks(divisors, hardwareTarget.pixel_clock,
		extraPLLFlags);

	if (gInfo->shared_info->device_type.Generation() != 4) {
		// G45: no need to power the panel off
		// Power on Panel
		write32(panelControl,
			read32(panelControl) | PANEL_CONTROL_POWER_TARGET_ON);
		read32(panelControl);

		if (!wait_for_set(panelStatus, PANEL_STATUS_POWER_ON, 1000)) {
			ERROR("%s: %s didn't power on within 1000us!\n", __func__,
				PortName());
		}
	}

	// Program target display mode
	fPipe->ConfigureTimings(target, !needsScaling);

	if (needsScaling) {
		if (gInfo->shared_info->device_type.Generation() <= 4) {
			// Enable panel fitter in automatic mode. It will figure out
			// the scaling ratios automatically.
			uint32 panelFitterControl = read32(INTEL_PANEL_FIT_CONTROL);
			panelFitterControl |= PANEL_FITTER_ENABLED;
			panelFitterControl &= ~(PANEL_FITTER_SCALING_MODE_MASK
				| PANEL_FITTER_PIPE_MASK);
			panelFitterControl |= PANEL_FITTER_PIPE_B;
			// LVDS is always on pipe B.
			write32(INTEL_PANEL_FIT_CONTROL, panelFitterControl);
		}
		// TODO do we need to do anything on later generations?
	} else {
		if (gInfo->shared_info->device_type.Generation() == 4
			|| gInfo->shared_info->device_type.Generation() == 3) {
			// Bypass the panel fitter
			uint32 panelFitterControl = read32(INTEL_PANEL_FIT_CONTROL);
			panelFitterControl &= ~PANEL_FITTER_ENABLED;
			write32(INTEL_PANEL_FIT_CONTROL, panelFitterControl);
		} else {
			// We don't need to do anything more for later generations, the
			// scaling is handled at the transcoder level. We may want to
			// configure dithering, but the code below ignores the previous
			// value in the register and may mess things up so we should do
			// this in a safeer way. For now, assume the BIOS did the right
			// thing.
#if 0
			// Disable panel fitting, but enable 8 to 6-bit dithering
			write32(INTEL_PANEL_FIT_CONTROL, 0x4);
				// TODO: do not do this if the connected panel is 24-bit
				// (I don't know how to detect that)
#endif
		}
	}

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - DVI/SDVO/generic


DigitalPort::DigitalPort(port_index index, const char* baseName)
	:
	Port(index, baseName)
{
}


bool
DigitalPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());

	// As this port overlaps with pretty much everything, this must be called
	// after having ruled out all other port types.
	return HasEDID();
}


addr_t
DigitalPort::_DDCRegister()
{
	//TODO: IS BROXTON, B = B, C = C, D = NIL
	switch (PortIndex()) {
		case INTEL_PORT_B:
			return INTEL_I2C_IO_E;
		case INTEL_PORT_C:
			return INTEL_I2C_IO_D;
		case INTEL_PORT_D:
			return INTEL_I2C_IO_F;
		default:
			return 0;
	}

	return 0;
}


addr_t
DigitalPort::_PortRegister()
{
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DIGITAL_PORT_A;
		case INTEL_PORT_B:
			return INTEL_DIGITAL_PORT_B;
		case INTEL_PORT_C:
			return INTEL_DIGITAL_PORT_C;
		default:
			return 0;
	}
	return 0;
}


status_t
DigitalPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	CALLED();
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->timing.h_display,
		target->timing.v_display);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	// Setup PanelFitter and Train FDI if it exists
	PanelFitter* fitter = fPipe->PFT();
	if (fitter != NULL)
		fitter->Enable(target->timing);
	FDILink* link = fPipe->FDI();
	if (link != NULL) {
		uint32 lanes = 0;
		uint32 linkBandwidth = 0;
		uint32 bitsPerPixel = 0;
		link->PreTrain(&target->timing, &linkBandwidth, &lanes, &bitsPerPixel);
		fPipe->SetFDILink(target->timing, linkBandwidth, lanes, bitsPerPixel);
		link->Train(&target->timing, lanes);
	}

	pll_divisors divisors;
	compute_pll_divisors(&target->timing, &divisors, false);

	uint32 extraPLLFlags = 0;
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_NORMAL | DISPLAY_PLL_2X_CLOCK;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's
	fPipe->ConfigureClocks(divisors, target->timing.pixel_clock, extraPLLFlags);

	// Program target display mode
	fPipe->ConfigureTimings(target);

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - LVDS Panel
// #pragma mark - HDMI


HDMIPort::HDMIPort(port_index index)
	:
	DigitalPort(index, "HDMI")
{
}


bool
HDMIPort::IsConnected()
{
	if (!gInfo->shared_info->device_type.SupportsHDMI())
		return false;

	addr_t portRegister = _PortRegister();
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	if (portRegister == 0)
		return false;

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0) {
		// check VBT mapping
		if (!_IsPortInVBT()) {
			TRACE("%s: %s: port not found in VBT\n", __func__, PortName());
			return false;
		} else
			TRACE("%s: %s: port found in VBT\n", __func__, PortName());
	}

	//Notes:
	//- DISPLAY_MONITOR_PORT_DETECTED does only tell you *some* sort of digital display is
	//  connected to the port *if* you have the AUX channel stuff under power. It does not
	//  tell you which -type- of digital display is connected.
	//- Since we rely on the BIOS anyway, let's just use the conclusions it made for us :)
	//  Beware though: set_display_power_mode() uses this DISPLAY_MONITOR_PORT_ENABLED bit
	//  for DPMS as well. So we should better buffer our findings here for i.e. possible
	//  accelerant clones starting up. For DPMS there's currently no problem as this bit
	//  is only programmed for LVDS, DVI and VGA while we detect presence only for DP and HDMI.
	//
	//if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0)
	if ((read32(portRegister) & DISPLAY_MONITOR_PORT_ENABLED) == 0)
		return false;

	return HasEDID();
}


addr_t
HDMIPort::_PortRegister()
{
	// on PCH there's an additional port sandwiched in
	bool hasPCH = (gInfo->shared_info->pch_info != INTEL_PCH_NONE);
	bool fourthGen = gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV);

	switch (PortIndex()) {
		case INTEL_PORT_B:
			if (fourthGen)
				return GEN4_HDMI_PORT_B;
			return hasPCH ? PCH_HDMI_PORT_B : INTEL_HDMI_PORT_B;
		case INTEL_PORT_C:
			if (fourthGen)
				return GEN4_HDMI_PORT_C;
			return hasPCH ? PCH_HDMI_PORT_C : INTEL_HDMI_PORT_C;
		case INTEL_PORT_D:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV))
				return CHV_HDMI_PORT_D;
			return hasPCH ? PCH_HDMI_PORT_D : 0;
		default:
			return 0;
		}

	return 0;
}


// #pragma mark - DisplayPort


DisplayPort::DisplayPort(port_index index, const char* baseName)
	:
	Port(index, baseName)
{
}


pipe_index
DisplayPort::PipePreference()
{
	CALLED();
	if (gInfo->shared_info->device_type.Generation() <= 4)
		return INTEL_PIPE_ANY;

	// Notes:
	// - The BIOSes seen sofar do not use PIPE C by default.
	// - Looks like BIOS selected Transcoder (A,B,C) is not always same as selected Pipe (A,B,C)
	//   so these should probably be handled seperately. For now this is OK as we don't touch
	//   the pipe for DisplayPort, only the transcoder..
	uint32 TranscoderPort = INTEL_TRANS_DP_PORT_NONE;
	switch (PortIndex()) {
		case INTEL_PORT_A:
			if (gInfo->shared_info->device_type.Generation() == 6) {
				if (((read32(INTEL_DISPLAY_PORT_A) & INTEL_DISP_PORTA_SNB_PIPE_MASK)
					>> INTEL_DISP_PORTA_SNB_PIPE_SHIFT) == INTEL_DISP_PORTA_SNB_PIPE_A) {
					return INTEL_PIPE_A;
				} else {
					return INTEL_PIPE_B;
				}
			}
			if (gInfo->shared_info->device_type.Generation() == 7) {
				uint32 Pipe = (read32(INTEL_DISPLAY_PORT_A) & INTEL_DISP_PORTA_IVB_PIPE_MASK)
					>> INTEL_DISP_PORTA_IVB_PIPE_SHIFT;
				switch (Pipe) {
					case INTEL_DISP_PORTA_IVB_PIPE_A:
						return INTEL_PIPE_A;
					case INTEL_DISP_PORTA_IVB_PIPE_B:
						return INTEL_PIPE_B;
					case INTEL_DISP_PORTA_IVB_PIPE_C:
						return INTEL_PIPE_C;
					default:
						return INTEL_PIPE_ANY;
				}
			}
			return INTEL_PIPE_ANY;
		case INTEL_PORT_B:
			TranscoderPort = INTEL_TRANS_DP_PORT_B;
			break;
		case INTEL_PORT_C:
			TranscoderPort = INTEL_TRANS_DP_PORT_C;
			break;
		case INTEL_PORT_D:
			TranscoderPort = INTEL_TRANS_DP_PORT_D;
			break;
		default:
			return INTEL_PIPE_ANY;
	}

	for (uint32 Transcoder = 0; Transcoder < 3; Transcoder++) {
		if ((read32(INTEL_TRANSCODER_A_DP_CTL + (Transcoder << 12)) & INTEL_TRANS_DP_PORT_MASK) ==
			INTEL_TRANS_DP_PORT(TranscoderPort)) {
			switch (Transcoder) {
				case 0:
					return INTEL_PIPE_A;
				case 1:
					return INTEL_PIPE_B;
				case 2:
					return INTEL_PIPE_C;
			}
		}
	}

	return INTEL_PIPE_ANY;
}


status_t
DisplayPort::SetPipe(Pipe* pipe)
{
	CALLED();

	if (pipe == NULL) {
		ERROR("%s: Invalid pipe provided!\n", __func__);
		return B_ERROR;
	}

	// TODO: UnAssignPipe?  This likely needs reworked a little
	if (fPipe != NULL) {
		ERROR("%s: Can't reassign display pipe (yet)\n", __func__);
		return B_ERROR;
	}

	// generation 3/4/5 gfx have no eDP ports.
	// generation 6 gfx SandyBridge/SNB uses one bit (b30) on the eDP port.
	// generation 7 gfx IvyBridge/IVB uses 2 bits (b29-30) on the eDP port.
	// on all other DP ports pipe selections works differently (indirect).
	// fixme: implement..
	TRACE("%s: Assuming pipe is assigned by BIOS (fixme)\n", __func__);

	fPipe = pipe;

	if (fPipe == NULL)
		return B_NO_MEMORY;

	// Disable display pipe until modesetting enables it
	if (fPipe->IsEnabled())
		fPipe->Enable(false);

	return B_OK;
}


status_t
DisplayPort::SetupI2c(i2c_bus *bus)
{
	CALLED();

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0) {
		if (!_IsDisplayPortInVBT())
			return Port::SetupI2c(bus);
	}

	return _SetupDpAuxI2c(bus);
}


bool
DisplayPort::IsConnected()
{
	addr_t portRegister = _PortRegister();

	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	if (portRegister == 0)
		return false;

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0) {
		// check VBT mapping
		if (!_IsPortInVBT()) {
			TRACE("%s: %s: port not found in VBT\n", __func__, PortName());
			return false;
		} else
			TRACE("%s: %s: port found in VBT\n", __func__, PortName());
	}

	//Notes:
	//- DISPLAY_MONITOR_PORT_DETECTED does only tell you *some* sort of digital display is
	//  connected to the port *if* you have the AUX channel stuff under power. It does not
	//  tell you which -type- of digital display is connected.
	//- Since we rely on the BIOS anyway, let's just use the conclusions it made for us :)
	//  Beware though: set_display_power_mode() uses this DISPLAY_MONITOR_PORT_ENABLED bit
	//  for DPMS as well. So we should better buffer our findings here for i.e. possible
	//  accelerant clones starting up. For DPMS there's currently no problem as this bit
	//  is only programmed for LVDS, DVI and VGA while we detect presence only for DP and HDMI.
	//
	//if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0) {
	if ((read32(portRegister) & DISPLAY_MONITOR_PORT_ENABLED) == 0) {
		TRACE("%s: %s link not detected\n", __func__, PortName());
		return false;
	}

	TRACE("%s: %s link detected\n", __func__, PortName());
	bool edidDetected = HasEDID();

	// On laptops we always have an internal panel.. (this is on the eDP port)
	if ((gInfo->shared_info->device_type.IsMobile() || _IsEDPPort())
		&& (PortIndex() == INTEL_PORT_A) && !edidDetected) {
		if (gInfo->shared_info->has_vesa_edid_info) {
			TRACE("%s: Laptop. Using VESA edid info\n", __func__);
			memcpy(&fEDIDInfo, &gInfo->shared_info->vesa_edid_info,
				sizeof(edid1_info));
			if (fEDIDState != B_OK) {
				fEDIDState = B_OK;
				// HasEDID now true
				edid_dump(&fEDIDInfo);
			}
			return true;
		} else if (gInfo->shared_info->got_vbt) {
			TRACE("%s: Laptop. No EDID, but force enabled as we have a VBT\n", __func__);
			return true;
		}
	}

	//since EDID is not correctly implemented yet for this connection type we'll do without it for now
	//return HasEDID();
	return true;
}


addr_t
DisplayPort::_DDCRegister()
{
	return 0;
}


addr_t
DisplayPort::_PortRegister()
{
	// There are 6000 lines of intel linux code probing DP registers
	// to properly detect DP vs eDP to then in-turn properly figure out
	// what is DP and what is HDMI. It only takes 3 lines to
	// ignore DisplayPort on ValleyView / CherryView

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV)
		|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV)) {
		ERROR("TODO: DisplayPort on ValleyView / CherryView");
		return 0;
	}

	// Intel, are humans even involved anymore?
	// This is a lot more complex than this code makes it look. (see defines)
	// INTEL_DISPLAY_PORT_X moves around a lot based on PCH
	// except on ValleyView and CherryView.
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DISPLAY_PORT_A;
		case INTEL_PORT_B:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DISPLAY_PORT_B;
			return INTEL_DISPLAY_PORT_B;
		case INTEL_PORT_C:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DISPLAY_PORT_C;
			return INTEL_DISPLAY_PORT_C;
		case INTEL_PORT_D:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV))
				return CHV_DISPLAY_PORT_D;
			else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return 0;
			return INTEL_DISPLAY_PORT_D;
		default:
			return 0;
	}

	return 0;
}


status_t
DisplayPort::_SetPortLinkGen4(const display_timing& timing)
{
	// Khz / 10. ( each output octet encoded as 10 bits. 
	//fixme: always so?
	uint32 linkBandwidth = 270000; //khz
	uint32 fPipeOffset = 0;
	if (fPipe->Index() == INTEL_PIPE_B)
		fPipeOffset = 0x1000;

	TRACE("%s: DP M1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_DATA_M + fPipeOffset));
	TRACE("%s: DP N1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_DATA_N + fPipeOffset));
	TRACE("%s: DP M1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_LINK_M + fPipeOffset));
	TRACE("%s: DP N1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_LINK_N + fPipeOffset));

	uint32 bitsPerPixel = 24;	//fixme: always so?
	uint32 lanes = 4;			//fixme: always so?

	//Setup Data M/N
	uint64 linkspeed = lanes * linkBandwidth * 8;
	uint64 ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	uint64 ret_m = timing.pixel_clock * ret_n * bitsPerPixel / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	//Set TU size bits (to default, max) before link training so that error detection works
	write32(INTEL_PIPE_A_DATA_M + fPipeOffset, ret_m | FDI_PIPE_MN_TU_SIZE_MASK);
	write32(INTEL_PIPE_A_DATA_N + fPipeOffset, ret_n);

	//Setup Link M/N
	linkspeed = linkBandwidth;
	ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	ret_m = timing.pixel_clock * ret_n / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	write32(INTEL_PIPE_A_LINK_M + fPipeOffset, ret_m);
	//Writing Link N triggers all four registers to be activated also (on next VBlank)
	write32(INTEL_PIPE_A_LINK_N + fPipeOffset, ret_n);

	TRACE("%s: DP M1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_DATA_M + fPipeOffset));
	TRACE("%s: DP N1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_DATA_N + fPipeOffset));
	TRACE("%s: DP M1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_LINK_M + fPipeOffset));
	TRACE("%s: DP N1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_PIPE_A_LINK_N + fPipeOffset));

	return B_OK;
}


status_t
DisplayPort::_SetPortLinkGen6(const display_timing& timing)
{
	// Khz / 10. ( each output octet encoded as 10 bits.
	//note: (fixme) eDP is fixed option 162 or 270Mc, other DPs go via DPLL programming to one of the same vals.
	uint32 linkBandwidth = 270000; //khz
	TRACE("%s: DP link reference clock is %gMhz\n", __func__, linkBandwidth / 1000.0f);

	uint32 fPipeOffset = 0;
	switch (fPipe->Index()) {
		case INTEL_PIPE_B:
			fPipeOffset = 0x1000;
			break;
		case INTEL_PIPE_C:
			fPipeOffset = 0x2000;
			break;
		default:
			break;
	}

	TRACE("%s: DP M1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_DATA_M1 + fPipeOffset));
	TRACE("%s: DP N1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_DATA_N1 + fPipeOffset));
	TRACE("%s: DP M1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_LINK_M1 + fPipeOffset));
	TRACE("%s: DP N1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_LINK_N1 + fPipeOffset));

	uint32 bitsPerPixel =
		(read32(INTEL_TRANSCODER_A_DP_CTL + fPipeOffset) & INTEL_TRANS_DP_BPC_MASK) >> INTEL_TRANS_DP_COLOR_SHIFT;
	switch (bitsPerPixel) {
		case PIPE_DDI_8BPC:
			bitsPerPixel = 24;
			break;
		case PIPE_DDI_10BPC:
			bitsPerPixel = 30;
			break;
		case PIPE_DDI_6BPC:
			bitsPerPixel = 18;
			break;
		case PIPE_DDI_12BPC:
			bitsPerPixel = 36;
			break;
		default:
			ERROR("%s: DP illegal link colordepth set.\n", __func__);
			return B_ERROR;
	}
	TRACE("%s: DP link colordepth: %" B_PRIu32 "\n", __func__, bitsPerPixel);

	uint32 lanes = ((read32(_PortRegister()) & INTEL_DISP_PORT_WIDTH_MASK) >> INTEL_DISP_PORT_WIDTH_SHIFT) + 1;
	if (lanes > 4) {
		ERROR("%s: DP illegal number of lanes set.\n", __func__);
		return B_ERROR;
	}
	TRACE("%s: DP mode with %" B_PRIx32 " lane(s) in use\n", __func__, lanes);

	//Reserving 5% bandwidth for possible spread spectrum clock use
	uint32 bps = timing.pixel_clock * bitsPerPixel * 21 / 20;
	//use DIV_ROUND_UP:
	uint32 required_lanes = (bps + (linkBandwidth * 8) - 1) / (linkBandwidth * 8);
	TRACE("%s: DP mode needs %" B_PRIx32 " lane(s) in use\n", __func__, required_lanes);
	if (required_lanes > lanes) {
		//Note that we *must* abort as otherwise the PIPE/DP-link hangs forever (without retraining!).
		ERROR("%s: DP not enough lanes active for requested mode.\n", __func__);
		return B_ERROR;
	}

	//Setup Data M/N
	uint64 linkspeed = lanes * linkBandwidth * 8;
	uint64 ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	uint64 ret_m = timing.pixel_clock * ret_n * bitsPerPixel / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	//Set TU size bits (to default, max) before link training so that error detection works
	write32(INTEL_TRANSCODER_A_DATA_M1 + fPipeOffset, ret_m | INTEL_TRANSCODER_MN_TU_SIZE_MASK);
	write32(INTEL_TRANSCODER_A_DATA_N1 + fPipeOffset, ret_n);

	//Setup Link M/N
	linkspeed = linkBandwidth;
	ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	ret_m = timing.pixel_clock * ret_n / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	write32(INTEL_TRANSCODER_A_LINK_M1 + fPipeOffset, ret_m);
	//Writing Link N triggers all four registers to be activated also (on next VBlank)
	write32(INTEL_TRANSCODER_A_LINK_N1 + fPipeOffset, ret_n);

	TRACE("%s: DP M1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_DATA_M1 + fPipeOffset));
	TRACE("%s: DP N1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_DATA_N1 + fPipeOffset));
	TRACE("%s: DP M1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_LINK_M1 + fPipeOffset));
	TRACE("%s: DP N1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_TRANSCODER_A_LINK_N1 + fPipeOffset));

	return B_OK;
}


status_t
DisplayPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	CALLED();
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->timing.h_display,
		target->timing.v_display);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	status_t result = B_OK;
	if (gInfo->shared_info->device_type.Generation() <= 4) {
		fPipe->ConfigureTimings(target);
		result = _SetPortLinkGen4(target->timing);
	} else {
		display_timing hardwareTarget = target->timing;
		bool needsScaling = false;
		if ((PortIndex() == INTEL_PORT_A)
			&& (gInfo->shared_info->device_type.IsMobile() || _IsEDPPort())) {
			// For internal panels, we may need to set the timings according to the panel
			// native video mode, and let the panel fitter do the scaling.
			// note: upto/including generation 5 laptop panels are still LVDS types, handled elsewhere.

			if (gInfo->shared_info->got_vbt) {
				// Set vbios hardware panel mode as base
				hardwareTarget = gInfo->shared_info->panel_timing;

				if (hardwareTarget.h_display == target->timing.h_display
						&& hardwareTarget.v_display == target->timing.v_display) {
					// We are feeding the native video mode, nothing to do: disable scaling
					TRACE("%s: Requested mode is panel's native resolution: disabling scaling\n", __func__);
				} else {
					// We need to enable the panel fitter
					TRACE("%s: Requested mode is not panel's native resolution: enabling scaling\n", __func__);
					needsScaling = true;
				}
			} else {
				//fixme: We should now first try for EDID info detailed timing, highest res in list: that's the
				//native mode as well. If we also have no EDID, then fallback to setting mode directly as below.

				TRACE("%s: Setting internal panel mode without VBT info generation, scaling may not work\n",
					__func__);
				// We don't have VBT data, try to set the requested mode directly
				// and hope for the best
				hardwareTarget = target->timing;
			}
		}

		result = B_OK;
		if (PortIndex() != INTEL_PORT_A)
			result = _SetPortLinkGen6(hardwareTarget);

		if (result == B_OK) {
			// Setup PanelFitter and Train FDI if it exists
			PanelFitter* fitter = fPipe->PFT();
			if (fitter != NULL)
				fitter->Enable(hardwareTarget);

			uint32 lanes = 0;
			uint32 linkBandwidth = 0;
			uint32 bitsPerPixel = 0;
			if (PortIndex() != INTEL_PORT_A) {
				FDILink* link = fPipe->FDI();
				if (link != NULL) {
					link->PreTrain(&hardwareTarget, &linkBandwidth, &lanes, &bitsPerPixel);
					fPipe->SetFDILink(hardwareTarget, linkBandwidth, lanes, bitsPerPixel);
					link->Train(&hardwareTarget, lanes);
				}
			} else {
				// 'local' eDP port is in use
				linkBandwidth =
					(read32(INTEL_DISPLAY_PORT_A) & INTEL_DISP_EDP_PLL_FREQ_MASK) >> INTEL_DISP_EDP_PLL_FREQ_SHIFT;
				switch (linkBandwidth) {
					case INTEL_DISP_EDP_PLL_FREQ_270:
						linkBandwidth = 270000; //khz
						break;
					case INTEL_DISP_EDP_PLL_FREQ_162:
						linkBandwidth = 162000; //khz
						break;
					default:
						TRACE("%s: eDP illegal reference clock ID set, assuming 270Mhz.\n", __func__);
						linkBandwidth = 270000; //khz
				}

				bitsPerPixel =
					(read32(INTEL_DISPLAY_A_PIPE_CONTROL) & INTEL_PIPE_BPC_MASK) >> INTEL_PIPE_COLOR_SHIFT;
				switch (bitsPerPixel) {
					case INTEL_PIPE_8BPC:
						bitsPerPixel = 24;
						break;
					case INTEL_PIPE_10BPC:
						bitsPerPixel = 30;
						break;
					case INTEL_PIPE_6BPC:
						bitsPerPixel = 18;
						break;
					case INTEL_PIPE_12BPC:
						bitsPerPixel = 36;
						break;
					default:
						bitsPerPixel = 0;
				}

				lanes =
					((read32(INTEL_DISPLAY_PORT_A) & INTEL_DISP_PORT_WIDTH_MASK) >> INTEL_DISP_PORT_WIDTH_SHIFT) + 1;

				fPipe->SetFDILink(hardwareTarget, linkBandwidth, lanes, bitsPerPixel);
			}

			// Program general pipe config
			fPipe->Configure(target);

			// Pll programming is not needed for (e)DP..

			// Program target display mode
			fPipe->ConfigureTimings(target, !needsScaling, PortIndex());
		} else {
			TRACE("%s: Setting display mode via fallback: using scaling!\n", __func__);
			// Keep monitor at native mode and scale image to that
			fPipe->ConfigureScalePos(target);
		}
	}

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return result;
}


// #pragma mark - Embedded DisplayPort


EmbeddedDisplayPort::EmbeddedDisplayPort()
	:
	DisplayPort(INTEL_PORT_A, "Embedded DisplayPort")
{
}


bool
EmbeddedDisplayPort::IsConnected()
{
	addr_t portRegister = _PortRegister();

	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	if (!gInfo->shared_info->device_type.IsMobile()) {
		TRACE("%s: skipping eDP on non-mobile GPU\n", __func__);
		return false;
	}

	if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0) {
		TRACE("%s: %s link not detected\n", __func__, PortName());
		return false;
	}

	HasEDID();

	// If eDP has EDID, awesome. We use it.
	// No EDID? The modesetting code falls back to VBIOS panel_mode
	return true;
}


// #pragma mark - Digital Display Port


DigitalDisplayInterface::DigitalDisplayInterface(port_index index,
		const char* baseName)
	:
	Port(index, baseName)
{
	// As of Haswell, Intel decided to change eDP ports to a "DDI" bus...
	// on a dare because the hardware engineers were drunk one night.
}


addr_t
DigitalDisplayInterface::_PortRegister()
{
	// TODO: Linux does a DDI_BUF_CTL(INTEL_PORT_A) which is cleaner
	// (but we have to ensure the offsets + region base is correct)
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return DDI_BUF_CTL_A;
		case INTEL_PORT_B:
			return DDI_BUF_CTL_B;
		case INTEL_PORT_C:
			return DDI_BUF_CTL_C;
		case INTEL_PORT_D:
			return DDI_BUF_CTL_D;
		case INTEL_PORT_E:
			return DDI_BUF_CTL_E;
		case INTEL_PORT_F:
			if ((gInfo->shared_info->device_type.Generation() > 8) &&
				!gInfo->shared_info->device_type.InGroup(INTEL_GROUP_SKY))
				return DDI_BUF_CTL_F;
			return 0;
		case INTEL_PORT_G:
			if (gInfo->shared_info->device_type.Generation() >= 12)
				return DDI_BUF_CTL_G;
			return 0;
		default:
			return 0;
	}
	return 0;
}


addr_t
DigitalDisplayInterface::_DDCRegister()
{
	return Port::_DDCPin();
}


status_t
DigitalDisplayInterface::Power(bool enabled)
{
	if (fPipe == NULL) {
		ERROR("%s: Setting power without assigned pipe!\n", __func__);
		return B_ERROR;
	}
	TRACE("%s: %s DDI enabled: %s\n", __func__, PortName(),
		enabled ? "true" : "false");

	fPipe->Enable(enabled);

	//nogo currently.. (kills output forever)
#if 0
	addr_t portRegister = _PortRegister();
	uint32 state = read32(portRegister);
	write32(portRegister,
		enabled ? (state | DDI_BUF_CTL_ENABLE) : (state & ~DDI_BUF_CTL_ENABLE));
	read32(portRegister);
#endif

	return B_OK;
}


status_t
DigitalDisplayInterface::SetPipe(Pipe* pipe)
{
	CALLED();

	if (pipe == NULL) {
		ERROR("%s: Invalid pipe provided!\n", __func__);
		return B_ERROR;
	}

	// TODO: UnAssignPipe?  This likely needs reworked a little
	if (fPipe != NULL) {
		ERROR("%s: Can't reassign display pipe (yet)\n", __func__);
		return B_ERROR;
	}

	// all DDI ports pipe selections works differently than on the old port types (indirect).
	// fixme: implement..
	TRACE("%s: Assuming pipe is assigned by BIOS (fixme)\n", __func__);

	fPipe = pipe;

	if (fPipe == NULL)
		return B_NO_MEMORY;

	// Disable display pipe until modesetting enables it
	if (fPipe->IsEnabled())
		fPipe->Enable(false);

	return B_OK;
}


status_t
DigitalDisplayInterface::SetupI2c(i2c_bus *bus)
{
	CALLED();

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0) {
		if (!_IsDisplayPortInVBT())
			return Port::SetupI2c(bus);
	}

	return _SetupDpAuxI2c(bus);
}


status_t
DigitalDisplayInterface::SetupI2cFallback(i2c_bus *bus)
{
	CALLED();

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0
		&& _IsDisplayPortInVBT() && _IsHdmiInVBT()) {
		return Port::SetupI2c(bus);
	}

	return B_ERROR;
}


bool
DigitalDisplayInterface::IsConnected()
{
	addr_t portRegister = _PortRegister();

	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	// Please note: Skylake and up (Desktop) might use eDP for a seperate active VGA converter chip.
	if (portRegister == 0) {
		TRACE("%s: Port not implemented\n", __func__);
		return false;
	}

	// newer chipsets support 4 lanes on all ports
	fMaxLanes = 4;
	if ((gInfo->shared_info->device_type.Generation() < 9) ||
		gInfo->shared_info->device_type.InGroup(INTEL_GROUP_SKY)) {
		// Probe a little port info.
		if ((read32(DDI_BUF_CTL_A) & DDI_A_4_LANES) != 0) {
			switch (PortIndex()) {
				case INTEL_PORT_A:
					fMaxLanes = 4;
					break;
				case INTEL_PORT_E:
					fMaxLanes = 0;
					break;
				default:
					fMaxLanes = 4;
					break;
			}
		} else {
			switch (PortIndex()) {
				case INTEL_PORT_A:
					fMaxLanes = 2;
					break;
				case INTEL_PORT_E:
					fMaxLanes = 2;
					break;
				default:
					fMaxLanes = 4;
					break;
			}
		}
	}

	const uint32 deviceConfigCount = gInfo->shared_info->device_config_count;
	if (gInfo->shared_info->device_type.Generation() >= 6 && deviceConfigCount > 0) {
		// check VBT mapping
		if (!_IsPortInVBT()) {
			TRACE("%s: %s: port not found in VBT\n", __func__, PortName());
			return false;
		} else
			TRACE("%s: %s: port found in VBT\n", __func__, PortName());
	}

	TRACE("%s: %s Maximum Lanes: %" B_PRId8 "\n", __func__,
		PortName(), fMaxLanes);

	// fetch EDID but determine 'in use' later (below) so we also catch screens that fail EDID
	bool edidDetected = HasEDID();

	// On laptops we always have an internal panel.. (on the eDP port on DDI systems, fixed on eDP pipe)
	uint32 pipeState = 0;
	if ((gInfo->shared_info->device_type.IsMobile() || _IsEDPPort())
		&& (PortIndex() == INTEL_PORT_A)) {
		if (gInfo->shared_info->device_type.Generation() < 12) {
			// TODO: the pipe state isn't reliable after gen11
			pipeState = read32(PIPE_DDI_FUNC_CTL_EDP);
			TRACE("%s: PIPE_DDI_FUNC_CTL_EDP: 0x%" B_PRIx32 "\n", __func__, pipeState);
			if (!(pipeState & PIPE_DDI_FUNC_CTL_ENABLE)) {
				TRACE("%s: Laptop, but eDP port down\n", __func__);
				return false;
			}
		}
		if (edidDetected)
			return true;
		else if (gInfo->shared_info->has_vesa_edid_info) {
			TRACE("%s: Laptop. Using VESA edid info\n", __func__);
			memcpy(&fEDIDInfo, &gInfo->shared_info->vesa_edid_info,	sizeof(edid1_info));
			if (fEDIDState != B_OK) {
				fEDIDState = B_OK;
				// HasEDID now true
				edid_dump(&fEDIDInfo);
			}
			return true;
		} else if (gInfo->shared_info->got_vbt) {
			TRACE("%s: Laptop. No VESA EDID, but force enabled as we have a VBT\n", __func__);
			return true;
		}
		//should not happen:
		TRACE("%s: No (panel) type info found, assuming not connected\n", __func__);
		return false;
	}

	// scan all our non-eDP pipes to find the one connected to the current port and check it's enabled
	for (uint32 pipeCnt = 0; pipeCnt < 3; pipeCnt++) {
		switch (pipeCnt) {
			case 1:
				pipeState = read32(PIPE_DDI_FUNC_CTL_B);
				break;
			case 2:
				pipeState = read32(PIPE_DDI_FUNC_CTL_C);
				break;
			default:
				pipeState = read32(PIPE_DDI_FUNC_CTL_A);
				break;
		}
		if ((((pipeState & PIPE_DDI_SELECT_MASK) >> PIPE_DDI_SELECT_SHIFT) + 1) == (uint32)PortIndex()) {
			TRACE("%s: PIPE_DDI_FUNC_CTL nr %" B_PRIx32 ": 0x%" B_PRIx32 "\n", __func__, pipeCnt + 1, pipeState);
			// See if the BIOS enabled our output as it indicates it's in use
			if (pipeState & PIPE_DDI_FUNC_CTL_ENABLE) {
				TRACE("%s: Connected\n", __func__);
				return true;
			}
		}
	}

	if (edidDetected) {
		for (uint32 pipeCnt = 0; pipeCnt < 3; pipeCnt++) {
			uint32 pipeReg = 0;
			switch (pipeCnt) {
				case 1:
					pipeReg = PIPE_DDI_FUNC_CTL_B;
					break;
				case 2:
					pipeReg = PIPE_DDI_FUNC_CTL_C;
					break;
				default:
					pipeReg = PIPE_DDI_FUNC_CTL_A;
					break;
			}
			pipeState = read32(pipeReg);
			if ((pipeState & PIPE_DDI_FUNC_CTL_ENABLE) == 0) {
				TRACE("%s: Connected but port down\n", __func__);
				return false;
			}
			return true;
		}
		TRACE("%s: No pipe available, ignoring connected screen\n", __func__);
	}

	TRACE("%s: Not connected\n", __func__);
	return false;
}

status_t
DigitalDisplayInterface::_SetPortLinkGen8(const display_timing& timing, uint32 pllSel)
{
	//fixme: always so on pre gen 9?
	uint32 linkBandwidth = 270000; //khz

	if (gInfo->shared_info->device_type.Generation() >= 9) {
		if (pllSel != 0xff) {
			linkBandwidth = (read32(SKL_DPLL_CTRL1) >> (1 + 6 * pllSel)) & SKL_DPLL_DP_LINKRATE_MASK;
			switch (linkBandwidth) {
				case SKL_DPLL_CTRL1_2700:
					linkBandwidth = 2700000 / 5;
					break;
				case SKL_DPLL_CTRL1_1350:
					linkBandwidth = 1350000 / 5;
					break;
				case SKL_DPLL_CTRL1_810:
					linkBandwidth =  810000 / 5;
					break;
				case SKL_DPLL_CTRL1_1620:
					linkBandwidth = 1620000 / 5;
					break;
				case SKL_DPLL_CTRL1_1080:
					linkBandwidth = 1080000 / 5;
					break;
				case SKL_DPLL_CTRL1_2160:
					linkBandwidth = 2160000 / 5;
					break;
				default:
					linkBandwidth = 270000;
					ERROR("%s: DDI No known DP-link reference clock selected, assuming default\n", __func__);
					break;
			}
		} else {
			ERROR("%s: DDI No known PLL selected, assuming default DP-link reference\n", __func__);
		}
	}
	TRACE("%s: DDI DP-link reference clock is %gMhz\n", __func__, linkBandwidth / 1000.0f);

	uint32 fPipeOffset = 0;
	switch (fPipe->Index()) {
		case INTEL_PIPE_B:
			fPipeOffset = 0x1000;
			break;
		case INTEL_PIPE_C:
			fPipeOffset = 0x2000;
			break;
		case INTEL_PIPE_D:
			fPipeOffset = 0xf000;
			break;
		default:
			break;
	}

	TRACE("%s: DDI M1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_DATA_M + fPipeOffset));
	TRACE("%s: DDI N1 data before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_DATA_N + fPipeOffset));
	TRACE("%s: DDI M1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_LINK_M + fPipeOffset));
	TRACE("%s: DDI N1 link before: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_LINK_N + fPipeOffset));

	uint32 pipeFunc = read32(PIPE_DDI_FUNC_CTL_A + fPipeOffset);
	uint32 bitsPerPixel = (pipeFunc & PIPE_DDI_BPC_MASK) >> PIPE_DDI_COLOR_SHIFT;
	switch (bitsPerPixel) {
		case PIPE_DDI_8BPC:
			bitsPerPixel = 24;
			break;
		case PIPE_DDI_10BPC:
			bitsPerPixel = 30;
			break;
		case PIPE_DDI_6BPC:
			bitsPerPixel = 18;
			break;
		case PIPE_DDI_12BPC:
			bitsPerPixel = 36;
			break;
		default:
			ERROR("%s: DDI illegal link colordepth set.\n", __func__);
			return B_ERROR;
	}
	TRACE("%s: DDI Link Colordepth: %" B_PRIu32 "\n", __func__, bitsPerPixel);

	uint32 lanes = 4;
	// Only DP modes supports less than 4 lanes: read current config
	if (((pipeFunc & PIPE_DDI_MODESEL_MASK) >> PIPE_DDI_MODESEL_SHIFT) >= PIPE_DDI_MODE_DP_SST) {
		// On gen 9.5 IceLake 3x mode exists (DSI only), earlier models: reserved value.
		lanes = ((pipeFunc & PIPE_DDI_DP_WIDTH_MASK) >> PIPE_DDI_DP_WIDTH_SHIFT) + 1;
		TRACE("%s: DDI in DP mode with %" B_PRIx32 " lane(s) in use\n", __func__, lanes);
	} else {
		TRACE("%s: DDI in non-DP mode with %" B_PRIx32 " lane(s) in use\n", __func__, lanes);
	}

	//Setup Data M/N
	uint64 linkspeed = lanes * linkBandwidth * 8;
	uint64 ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	uint64 ret_m = timing.pixel_clock * ret_n * bitsPerPixel / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	//Set TU size bits (to default, max) before link training so that error detection works
	write32(INTEL_DDI_PIPE_A_DATA_M + fPipeOffset, ret_m | FDI_PIPE_MN_TU_SIZE_MASK);
	write32(INTEL_DDI_PIPE_A_DATA_N + fPipeOffset, ret_n);

	//Setup Link M/N
	linkspeed = linkBandwidth;
	ret_n = 1;
	while(ret_n < linkspeed) {
		ret_n *= 2;
	}
	if (ret_n > 0x800000) {
		ret_n = 0x800000;
	}
	ret_m = timing.pixel_clock * ret_n / linkspeed;
	while ((ret_n > 0xffffff) || (ret_m > 0xffffff)) {
		ret_m >>= 1;
		ret_n >>= 1;
	}
	write32(INTEL_DDI_PIPE_A_LINK_M + fPipeOffset, ret_m);
	//Writing Link N triggers all four registers to be activated also (on next VBlank)
	write32(INTEL_DDI_PIPE_A_LINK_N + fPipeOffset, ret_n);

	TRACE("%s: DDI M1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_DATA_M + fPipeOffset));
	TRACE("%s: DDI N1 data after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_DATA_N + fPipeOffset));
	TRACE("%s: DDI M1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_LINK_M + fPipeOffset));
	TRACE("%s: DDI N1 link after: 0x%" B_PRIx32 "\n", __func__, read32(INTEL_DDI_PIPE_A_LINK_N + fPipeOffset));

	return B_OK;
}

status_t
DigitalDisplayInterface::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	CALLED();
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->timing.h_display,
		target->timing.v_display);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	display_timing hardwareTarget = target->timing;
	bool needsScaling = false;
	if ((PortIndex() == INTEL_PORT_A)
		&& (gInfo->shared_info->device_type.IsMobile() || _IsEDPPort())) {
		// For internal panels, we may need to set the timings according to the panel
		// native video mode, and let the panel fitter do the scaling.

		if (gInfo->shared_info->got_vbt || HasEDID()) {
			// Set vbios hardware panel mode as base
			hardwareTarget = gInfo->shared_info->panel_timing;
			if (HasEDID()) {
				// the first detailed timing supposed to be the best supported one
				int i;
				for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
					edid1_detailed_monitor *monitor = &fEDIDInfo.detailed_monitor[i];
					if (monitor->monitor_desc_type == EDID1_IS_DETAILED_TIMING)
						break;
				}
				if (i < EDID1_NUM_DETAILED_MONITOR_DESC) {
					TRACE("%s: Using EDID detailed timing %d for the internal panel\n",
						__func__, i);
					const edid1_detailed_timing& timing
						= fEDIDInfo.detailed_monitor[i].data.detailed_timing;
					hardwareTarget.pixel_clock = timing.pixel_clock * 10;
					hardwareTarget.h_display = timing.h_active;
					hardwareTarget.h_sync_start = timing.h_active + timing.h_sync_off;
					hardwareTarget.h_sync_end = hardwareTarget.h_sync_start + timing.h_sync_width;
					hardwareTarget.h_total = timing.h_active + timing.h_blank;
					hardwareTarget.v_display = timing.v_active;
					hardwareTarget.v_sync_start = timing.v_active + timing.v_sync_off;
					hardwareTarget.v_sync_end = hardwareTarget.v_sync_start + timing.v_sync_width;
					hardwareTarget.v_total = timing.v_active + timing.v_blank;
					hardwareTarget.flags = 0;
					if (timing.sync == 3) {
						if (timing.misc & 1)
							hardwareTarget.flags |= B_POSITIVE_HSYNC;
						if (timing.misc & 2)
							hardwareTarget.flags |= B_POSITIVE_VSYNC;
					}
					if (timing.interlaced)
						hardwareTarget.flags |= B_TIMING_INTERLACED;
				}
			}
			if (hardwareTarget.h_display == target->timing.h_display
					&& hardwareTarget.v_display == target->timing.v_display) {
				// We are setting the native video mode, nothing special to do
				// Note: this means refresh and timing might vary according to requested mode.
				hardwareTarget = target->timing;
				TRACE("%s: Setting internal panel to native resolution at %" B_PRIu32 "Hz\n", __func__,
					hardwareTarget.pixel_clock * 1000 / (hardwareTarget.h_total * hardwareTarget.v_total));
			} else {
				// We need to enable the panel fitter
				TRACE("%s: Hardware mode will actually be %dx%d at %" B_PRIu32 "Hz\n", __func__,
					hardwareTarget.h_display, hardwareTarget.v_display,
					hardwareTarget.pixel_clock * 1000 / (hardwareTarget.h_total * hardwareTarget.v_total));

				// FIXME we should also get the refresh frequency from the target
				// mode, and then "sanitize" the resulting mode we made up.
				needsScaling = true;
			}
		} else {
			TRACE("%s: Setting internal panel mode without VBT info generation, scaling may not work\n",
				__func__);
			// We don't have VBT data, try to set the requested mode directly
			// and hope for the best
			hardwareTarget = target->timing;
		}
	}

	// Setup PanelFitter
	PanelFitter* fitter = fPipe->PFT();
	if (fitter != NULL)
		fitter->Enable(hardwareTarget);

	// skip FDI as it never applies to DDI (on gen7 and 8 only for the real analog VGA port)

	// Program general pipe config
	fPipe->Configure(target);

	uint32 pllSel = 0xff; // no PLL selected
	if (gInfo->shared_info->device_type.Generation() <= 8) {
		unsigned int r2_out, n2_out, p_out;
		hsw_ddi_calculate_wrpll(
			hardwareTarget.pixel_clock * 1000 /* in Hz */,
			&r2_out, &n2_out, &p_out);
	} else {
		skl_wrpll_params wrpll_params;
		skl_ddi_calculate_wrpll(
			hardwareTarget.pixel_clock * 1000 /* in Hz */,
			gInfo->shared_info->pll_info.reference_frequency,
			&wrpll_params);
		fPipe->ConfigureClocksSKL(wrpll_params,
			hardwareTarget.pixel_clock,
			PortIndex(),
			&pllSel);
	}

	// Program target display mode
	fPipe->ConfigureTimings(target, !needsScaling);
	_SetPortLinkGen8(hardwareTarget, pllSel);

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}
