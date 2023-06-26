/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008-2011, Michael Lotz <mmlr@mlotz.ch>
 * Copyright 2023 Vladimir Serbinenko <phcoder@gmail.com>
 * Distributed under the terms of the MIT license.
 */
#ifndef I2C_ELAN_DEVICE_H
#define I2C_ELAN_DEVICE_H


#include <condition_variable.h>
#include <i2c.h>
#include <keyboard_mouse_driver.h>
#include "I2CHIDProtocol.h"

#define TRANSFER_BUFFER_SIZE 48

class ELANDevice {
public:
								ELANDevice(device_node* parent, i2c_device_interface* i2c,
									i2c_device i2cCookie);
								~ELANDevice();

			status_t			InitCheck() const { return fStatus; }

			bool				IsOpen() const { return fOpenCount > 0; }
			status_t			Open(uint32 flags);
			status_t			Close();
			int32				OpenCount() const { return fOpenCount; }

			void				Removed();
			bool				IsRemoved() const { return fRemoved; }

			void				SetPublishPath(char *publishPath);
			const char *			PublishPath() { return fPublishPath; }
			status_t			Control(uint32 op, void *buffer, size_t length);
			device_node*			Parent() { return fParent; }
private:
	static	void				_TransferCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);
	static	void				_UnstallCallback(void *cookie,
									status_t status, void *data,
									size_t actualLength);

			status_t			_MaybeScheduleTransfer(int type, int id, int reportSize);
			status_t			_Reset();
			status_t			_SetPower(uint8 power);
			status_t			_FetchBuffer(uint8* cmd, size_t cmdLength,
									void* buffer, size_t bufferLength);
			status_t			_FetchReport(uint8 type, uint8 id,
									size_t reportSize);
			status_t			_ExecCommand(i2c_op op, uint8* cmd,
									size_t cmdLength, void* buffer,
									size_t bufferLength);
			void				_SetReport(status_t status, uint8 *report, size_t length);
			status_t			_ReadAndParseReport(touchpad_movement *buffer,
									bigtime_t timeout, int& zero_report_count);
			status_t			_WaitForReport(bigtime_t timeout);
			status_t			_ReadRegister(uint16_t reg, size_t len, void *val);
			status_t			_WriteRegister(uint16_t reg, uint16_t val);
			status_t			_SetAbsoluteMode(bool enable);
private:
			status_t			fStatus;

			bigtime_t			fTransferLastschedule;
			int32				fTransferScheduled;
			uint8				fTransferBuffer[TRANSFER_BUFFER_SIZE];

			int32				fOpenCount;
			bool				fRemoved;

			char *				fPublishPath;

			uint8				fReportID;
			bool				fHighPrecision;

			i2c_hid_descriptor		fDescriptor;

			device_node*			fParent;

			i2c_device_interface*		fI2C;
			i2c_device			fI2CCookie;

			uint32				fLastButtons;
			uint32				fClickCount;
			bigtime_t			fLastClickTime;
			bigtime_t			fClickSpeed;

			status_t			fReportStatus;
			uint8				fCurrentReport[TRANSFER_BUFFER_SIZE];
			int				fCurrentReportLength;
			int32				fBusyCount;

			ConditionVariable		fConditionVariable;

			touchpad_specs			fHardwareSpecs;
};


#endif // I2C_ELAN_DEVICE_H
