/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Copyright 2020, 2022 Vladimir Kondratyev <wulf@FreeBSD.org>
 * Copyright 2023 Vladimir Serbinenko <phcoder@gmail.com>
 * Distributed under the terms of the MIT license.
 */


//!	Driver for I2C Elan Devices.
// Partially based on FreeBSD ietp driver


#include "Driver.h"
#include "ELANDevice.h"
#include "HIDReport.h"
#include <keyboard_mouse_driver.h>
#include <kernel.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>

#define LO_ELAN_REPORT_SIZE 32
#define HI_ELAN_REPORT_SIZE 37
#define MIN_ELAN_REPORT  11

#define	IETP_MAX_X_AXIS		0x0106
#define	IETP_MAX_Y_AXIS		0x0107

#define	IETP_CONTROL		0x0300
#define	IETP_CTRL_ABSOLUTE	0x0001
#define	IETP_CTRL_STANDARD	0x0000

ELANDevice::ELANDevice(device_node* parent, i2c_device_interface* i2c,
	i2c_device i2cCookie)
	:	fStatus(B_NO_INIT),
		fTransferLastschedule(0),
		fTransferScheduled(0),
		fOpenCount(0),
		fRemoved(false),
		fPublishPath(nullptr),
		fReportID(0x5d),
		fHighPrecision(false),
		fParent(parent),
		fI2C(i2c),
		fI2CCookie(i2cCookie),
		fLastButtons(0),
 		fClickCount(0),
		fLastClickTime(0),
		fClickSpeed(250000),
		fReportStatus(B_NO_INIT),
		fCurrentReportLength(0),
		fBusyCount(0)
{
	fConditionVariable.Init(this, "elan report");

	uint16 descriptorAddress = 1;
	// fetch HID descriptor
	CALLED();
	fStatus = _FetchBuffer((uint8*)&descriptorAddress,
		sizeof(descriptorAddress), &fDescriptor, sizeof(fDescriptor));
	if (fStatus != B_OK) {
		ERROR("failed to fetch HID descriptor\n");
		return;
	}

	if (_SetAbsoluteMode(true) != B_OK) {
		TRACE_ALWAYS("failed to set absolute mode\n");
		return;
	}

	fHardwareSpecs.areaStartX = 0;
	fHardwareSpecs.areaStartY = 0;

	uint16_t buf;

	if (_ReadRegister(IETP_MAX_X_AXIS, sizeof(buf), &buf) != B_OK) {
		TRACE_ALWAYS("failed reading max x\n");
		return;
	}
	fHardwareSpecs.areaEndX = buf;

	if (_ReadRegister(IETP_MAX_Y_AXIS, sizeof(buf), &buf) != B_OK) {
		TRACE_ALWAYS("failed reading max y\n");
		return;
	}
	fHardwareSpecs.areaEndY = buf;

	fHardwareSpecs.edgeMotionWidth = 55;

	fHardwareSpecs.minPressure = 0;
	fHardwareSpecs.realMaxPressure = 50;
	fHardwareSpecs.maxPressure = 255;

	TRACE("Dimensions %dx%d\n", fHardwareSpecs.areaEndX, fHardwareSpecs.areaEndY);

	fStatus = B_OK;
}


ELANDevice::~ELANDevice()
{
}


status_t
ELANDevice::Open(uint32 flags)
{
	atomic_add(&fOpenCount, 1);
	_Reset();

	return B_OK;
}


status_t
ELANDevice::Close()
{
	atomic_add(&fOpenCount, -1);
	_SetPower(I2C_HID_POWER_OFF);

	return B_OK;
}


void
ELANDevice::Removed()
{
	fRemoved = true;
}


status_t
ELANDevice::_MaybeScheduleTransfer(int type, int id, int reportSize)
{
	if (fRemoved)
		return ENODEV;

	if (atomic_get_and_set(&fTransferScheduled, 1) != 0) {
		// someone else already caused a transfer to be scheduled
		return B_OK;
	}

	snooze_until(fTransferLastschedule, B_SYSTEM_TIMEBASE);
	fTransferLastschedule = system_time() + 10000;

	TRACE("scheduling interrupt transfer of %u bytes\n",
		reportSize);
	return _FetchReport(type, id, reportSize);
}

void
ELANDevice::SetPublishPath(char *publishPath)
{
	free(fPublishPath);
	fPublishPath = publishPath;
}

status_t
ELANDevice::Control(uint32 op, void *buffer,
	size_t length)
{
	switch (op) {

		case B_GET_DEVICE_NAME:
		{
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;

			if (user_strlcpy((char *)buffer, "Elantech I2C touchpad", length) > 0)
				return B_OK;

			return B_ERROR;
		}

		case MS_IS_TOUCHPAD:
			TRACE("ELANTECH: MS_IS_TOUCHPAD\n");
			if (buffer == NULL)
				return B_OK;
			return user_memcpy(buffer, &fHardwareSpecs, sizeof(fHardwareSpecs));

		case MS_READ_TOUCHPAD:
		{
			touchpad_read read;
			int zero_report_count = 0;
			if (length < sizeof(touchpad_read))
				return B_BUFFER_OVERFLOW;

			if (user_memcpy(&read.timeout, &(((touchpad_read*)buffer)->timeout),
					sizeof(bigtime_t)) != B_OK)
				return B_BAD_ADDRESS;

			read.event = MS_READ_TOUCHPAD;

			while (true) {
				status_t result = _ReadAndParseReport(
					&read.u.touchpad, read.timeout,
					zero_report_count);
				if (result == B_INTERRUPTED)
					continue;

				if (!IS_USER_ADDRESS(buffer)
					|| user_memcpy(buffer, &read, sizeof(read))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}

				TRACE("Returning MS_READ_TOUCHPAD: %x\n", result);

				return result;
			}
		}
	}

	return B_ERROR;
}


status_t
ELANDevice::_SetAbsoluteMode(bool enable)
{
	return _WriteRegister(IETP_CONTROL, enable ? IETP_CTRL_ABSOLUTE : IETP_CTRL_STANDARD);
}


status_t
ELANDevice::_WaitForReport(bigtime_t timeout)
{
	CALLED();
	while (atomic_get(&fBusyCount) != 0)
		snooze(1000);

	ConditionVariableEntry conditionVariableEntry;
	fConditionVariable.Add(&conditionVariableEntry);
	TRACE("Starting report wait\n");
	status_t result = _MaybeScheduleTransfer(
		HID_REPORT_TYPE_INPUT, fReportID,
		fHighPrecision ? HI_ELAN_REPORT_SIZE : LO_ELAN_REPORT_SIZE);
	if (result != B_OK) {
		TRACE_ALWAYS("scheduling transfer failed\n");
		conditionVariableEntry.Wait(B_RELATIVE_TIMEOUT, 0);
		return result;
	}

	result = conditionVariableEntry.Wait(B_RELATIVE_TIMEOUT, timeout);
	if (result != B_OK)
		return result;

	if (fReportStatus != B_OK)
		return fReportStatus;

	atomic_add(&fBusyCount, 1);
	return B_OK;
}


status_t
ELANDevice::_ReadAndParseReport(touchpad_movement *info, bigtime_t timeout, int &zero_report_count)
{
	CALLED();
	status_t result = _WaitForReport(timeout);
	if (result != B_OK) {
		if (IsRemoved()) {
			TRACE("device has been removed\n");
			return B_DEV_NOT_READY;
		}

		if (result != B_INTERRUPTED) {
			// interrupts happen when other reports come in on the same
			// input as ours
			TRACE_ALWAYS("error waiting for report: %s\n", strerror(result));
		}

		if (result == B_TIMED_OUT) {
			return result;
		}

		// signal that we simply want to try again
		return B_INTERRUPTED;
	}

	if (fCurrentReportLength == 0 && ++zero_report_count < 3) {
		atomic_add(&fBusyCount, -1);
		return B_INTERRUPTED;
	}

	uint8 report_copy[TRANSFER_BUFFER_SIZE];
	memset(report_copy, 0, TRANSFER_BUFFER_SIZE);
	memcpy(report_copy, fCurrentReport, MIN(TRANSFER_BUFFER_SIZE, fCurrentReportLength));
	atomic_add(&fBusyCount, -1);

	memset(info, 0, sizeof(*info));

	if (report_copy[0] == 0 || report_copy[0] == 0xff)
		return B_OK;

	info->buttons = report_copy[0] & 0x7;
	uint8 fingers = (report_copy[0] >> 3) & 0x1f;
	info->fingers = fingers;
	TRACE("buttons=%x, fingers=%x\n", info->buttons, fingers);
	const uint8 *fingerData = fCurrentReport + 1;
	int fingerCount = 0;
	int sumx = 0, sumy = 0, sumz = 0, sumw = 0;
	for (int finger = 0; finger < 5; finger++, fingerData += 5)
		if (fingers & (1 << finger)) {
			TRACE("finger %d:\n", finger);
			uint8 wh;
			int x, y, w;
			if (fHighPrecision) {
				x = fingerData[0] << 8 | fingerData[1];
				y = fingerData[2] << 8 | fingerData[3];
				wh = report_copy[30 + finger];
			} else {
				x = (fingerData[0] & 0xf0) << 4 | fingerData[1];
				y = (fingerData[0] & 0x0f) << 8 | fingerData[2];
				wh = fingerData[3];
			}

			int z = fingerData[4];

			w = MAX((wh >> 4) & 0xf, wh & 0xf);

			sumw += w;
			sumx += x;
			sumy += y;
			sumz += z;

			TRACE("x=%d, y=%d, z=%d, w=%d, wh=0x%x\n", x, y, z, w, wh);

			fingerCount++;
		}
	if (fingerCount > 0) {
		info->xPosition = sumx / fingerCount;
		info->yPosition = sumy / fingerCount;
		info->zPressure = sumz / fingerCount;
		info->fingerWidth = MIN(12, sumw / fingerCount);
	}

	TRACE("Resulting position=[%d, %d, %d, %d]\n",
		info->xPosition, info->yPosition, info->zPressure,
		info->fingerWidth);

	return B_OK;
}


void
ELANDevice::_UnstallCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	ELANDevice *device = (ELANDevice *)cookie;
	if (status != B_OK) {
		TRACE_ALWAYS("Unable to unstall device: %s\n", strerror(status));
	}

	// Now report the original failure, since we're ready to retry
	_TransferCallback(cookie, B_ERROR, device->fTransferBuffer, 0);
}


void
ELANDevice::_TransferCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	ELANDevice *device = (ELANDevice *)cookie;

	atomic_set(&device->fTransferScheduled, 0);
	device->_SetReport(status, device->fTransferBuffer, actualLength);
}


status_t
ELANDevice::_Reset()
{
	CALLED();
	status_t status = _SetPower(I2C_HID_POWER_ON);
	if (status != B_OK)
		return status;

	snooze(1000);

	uint8 cmd[] = {
		(uint8)(fDescriptor.wCommandRegister & 0xff),
		(uint8)(fDescriptor.wCommandRegister >> 8),
		0,
		I2C_HID_CMD_RESET,
	};

	status = _ExecCommand(I2C_OP_WRITE_STOP, cmd, sizeof(cmd), NULL, 0);
	if (status != B_OK) {
		_SetPower(I2C_HID_POWER_OFF);
		return status;
	}

	snooze(1000);
	return B_OK;
}


status_t
ELANDevice::_SetPower(uint8 power)
{
	CALLED();
	uint8 cmd[] = {
		(uint8)(fDescriptor.wCommandRegister & 0xff),
		(uint8)(fDescriptor.wCommandRegister >> 8),
		power,
		I2C_HID_CMD_SET_POWER
	};

	return _ExecCommand(I2C_OP_WRITE_STOP, cmd, sizeof(cmd), NULL, 0);
}


status_t
ELANDevice::_ReadRegister(uint16_t reg, size_t length, void *value)
{
	uint8_t cmd[2] = {
		(uint8_t) (reg & 0xff), (uint8_t) ((reg >> 8) & 0xff) };
	status_t status = _FetchBuffer(cmd, sizeof(cmd), fTransferBuffer,
		length);
	TRACE("Read register 0x%04x with value 0x%02x 0x%02x status=%d\n",
		reg, fTransferBuffer[0], fTransferBuffer[1], status);
	if (status != B_OK)
		return status;
	memcpy(value, fTransferBuffer, length);
	return B_OK;
}


status_t
ELANDevice::_WriteRegister(uint16_t reg, uint16_t value)
{
	uint8_t cmd[4] = { (uint8_t) (reg & 0xff),
		(uint8_t) ((reg >> 8) & 0xff),
		(uint8_t) (value & 0xff),
		(uint8_t) ((value >> 8) & 0xff) };
	TRACE("Write register 0x%04x with value 0x%04x\n", reg, value);
	status_t status = _FetchBuffer(cmd, sizeof(cmd), fTransferBuffer, 0);
	TRACE("status=%d\n", status);
	return status;
}


status_t
ELANDevice::_FetchReport(uint8 type, uint8 id, size_t reportSize)
{
	uint8 reportId = id > 15 ? 15 : id;
	size_t cmdLength = 6;
	uint8 cmd[] = {
		(uint8)(fDescriptor.wCommandRegister & 0xff),
		(uint8)(fDescriptor.wCommandRegister >> 8),
		(uint8)(reportId | (type << 4)),
		I2C_HID_CMD_GET_REPORT,
		0, 0, 0,
	};

	int dataOffset = 4;
	int reportIdLength = 1;
	if (reportId == 15) {
		cmd[dataOffset++] = id;
		cmdLength++;
		reportIdLength++;
	}
	cmd[dataOffset++] = fDescriptor.wDataRegister & 0xff;
	cmd[dataOffset++] = fDescriptor.wDataRegister >> 8;

	size_t bufferLength = reportSize + reportIdLength + 2;

	status_t status = _FetchBuffer(cmd, cmdLength, fTransferBuffer,
		bufferLength);
	if (status != B_OK) {
		atomic_set(&fTransferScheduled, 0);
		return status;
	}

	uint16 actualLength = fTransferBuffer[0] | (fTransferBuffer[1] << 8);
	TRACE("_FetchReport %" B_PRIuSIZE " %" B_PRIu16 "\n", reportSize,
		actualLength);

	if (actualLength <= 2 || actualLength == 0xffff || bufferLength == 0)
		actualLength = 0;
	else
		actualLength -= 2;

	atomic_set(&fTransferScheduled, 0);
	_SetReport(status,
		(uint8*)((addr_t)fTransferBuffer + 2), actualLength);
	return B_OK;
}


void
ELANDevice::_SetReport(status_t status, uint8 *report, size_t length)
{
	if (status != B_OK) {
		report = NULL;
		length = 0;
	}

	if (length < MIN_ELAN_REPORT && length != 0 && status == B_OK)
		status = B_ERROR;

	if (status == B_OK && length != 0 && report[0] != fReportID) {
		report = NULL;
		length = 0;
		status = B_INTERRUPTED;
	}

	if (report && length) {
		report++;
		length--;
	}

	fReportStatus = status;
	fCurrentReportLength = length;
	memset(fCurrentReport, 0, sizeof(fCurrentReport));
	if (report && status == B_OK)
		memcpy(fCurrentReport, report, MIN(sizeof(fCurrentReport), length));
	fConditionVariable.NotifyAll();
}


status_t
ELANDevice::_FetchBuffer(uint8* cmd, size_t cmdLength, void* buffer,
	size_t bufferLength)
{
	return _ExecCommand(I2C_OP_READ_STOP, cmd, cmdLength,
		buffer, bufferLength);
}


status_t
ELANDevice::_ExecCommand(i2c_op op, uint8* cmd, size_t cmdLength, void* buffer,
	size_t bufferLength)
{
	status_t status = fI2C->acquire_bus(fI2CCookie);
	if (status != B_OK)
		return status;
	status = fI2C->exec_command(fI2CCookie, I2C_OP_READ_STOP, cmd, cmdLength,
		buffer, bufferLength);
	fI2C->release_bus(fI2CCookie);
	return status;
}
