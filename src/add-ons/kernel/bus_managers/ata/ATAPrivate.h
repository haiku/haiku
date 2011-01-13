/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008, Marcus Overhagen.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ATA_PRIVATE_H
#define ATA_PRIVATE_H


#include <ata_types.h>
#include <bus/ATA.h>
#include <bus/SCSI.h>
#include <device_manager.h>
#include <lock.h>
#include <new>
#include <safemode.h>
#include <scsi_cmds.h>
#include <stdio.h>
#include <string.h>
#include <util/AutoLock.h>

#include "ATACommands.h"
#include "ATAInfoBlock.h"
#include "ATATracing.h"


#define ATA_MAX_DMA_FAILURES		3
#define ATA_STANDARD_TIMEOUT		10 * 1000 * 1000
#define ATA_RELEASE_TIMEOUT			10 * 1000 * 1000
#define ATA_SIGNATURE_ATA			0x0000
#define ATA_SIGNATURE_ATAPI			0xeb14
#define ATA_SIGNATURE_SATA			0xc33c
#define ATA_SIM_MODULE_NAME			"bus_managers/ata/sim/driver_v1"
#define ATA_CHANNEL_ID_GENERATOR	"ide/channel_id"
#define ATA_CHANNEL_ID_ITEM			"ide/channel_id"

enum {
	ATA_DEVICE_READY_REQUIRED	= 0x01,
	ATA_IS_WRITE				= 0x02,
	ATA_DMA_TRANSFER			= 0x04,
	ATA_CHECK_ERROR_BIT			= 0x08,
	ATA_WAIT_FINISH				= 0x10,
	ATA_WAIT_ANY_BIT			= 0x20,
	ATA_CHECK_DEVICE_FAULT		= 0x40
};


class ATADevice;
class ATARequest;

extern scsi_for_sim_interface *gSCSIModule;
extern device_manager_info *gDeviceManager;

bool copy_sg_data(scsi_ccb *ccb, uint offset, uint allocationLength,
	void *buffer, int size, bool toBuffer);
void swap_words(void *data, size_t size);


class ATAChannel {
public:
								ATAChannel(device_node *node);
								~ATAChannel();

			status_t			InitCheck();
			uint32				ChannelID() const { return fChannelID; }

			// SCSI stuff
			void				SetBus(scsi_bus bus);
			scsi_bus			Bus() const { return fSCSIBus; }
			status_t			ScanBus();

			void				PathInquiry(scsi_path_inquiry *info);
			void				GetRestrictions(uint8 targetID, bool *isATAPI,
									bool *noAutoSense, uint32 *maxBlocks);
			status_t			ExecuteIO(scsi_ccb *ccb);
			status_t			Control(uint8 targetID, uint32 op, void *buffer,
									size_t length);

			// ATA stuff
			status_t			SelectDevice(uint8 index);
			uint8				SelectedDevice();

			status_t			Reset();

			bool				UseDMA() const { return fUseDMA; }

			status_t			Wait(uint8 setBits, uint8 clearedBits,
									uint32 flags, bigtime_t timeout);
			status_t			WaitDataRequest(bool high);
			status_t			WaitDeviceReady();
			status_t			WaitForIdle();

			void				PrepareWaitingForInterrupt();
			status_t			WaitForInterrupt(bigtime_t timeout);
			status_t			RecoverLostInterrupt();

			// request handling
			status_t			SendRequest(ATARequest *request, uint32 flags);
			status_t			FinishRequest(ATARequest *request, uint32 flags,
									uint8 errorMask);

			// data transfers
			status_t			PrepareDMA(ATARequest *request);
			status_t			StartDMA();
			status_t			FinishDMA();

			status_t			ExecutePIOTransfer(ATARequest *request);

			status_t			ReadRegs(ATADevice *device);
			uint8				AltStatus();

			status_t			ReadPIO(uint8 *buffer, size_t length);
			status_t			WritePIO(uint8 *buffer, size_t length);

			status_t			Interrupt(uint8 status);

private:
			status_t			_ReadRegs(ata_task_file *taskFile,
									ata_reg_mask mask);
			status_t			_WriteRegs(ata_task_file *taskFile,
									ata_reg_mask mask);
			status_t			_WriteControl(uint8 value);
			uint8				_Status();

			void				_FlushAndWait(bigtime_t waitTime);

			bool				_DevicePresent(int device);

			status_t			_ReadPIOBlock(ATARequest *request,
									size_t length);
			status_t			_WritePIOBlock(ATARequest *request,
									size_t length);
			status_t			_TransferPIOBlock(ATARequest *request,
									size_t length, size_t *transferred);
			status_t			_TransferPIOPhysical(ATARequest *request,
									addr_t physicalAddress, size_t length,
									size_t *transferred);
			status_t			_TransferPIOVirtual(ATARequest *request,
									uint8 *virtualAddress, size_t length,
									size_t *transferred);

			const char *		_DebugContext() { return fDebugContext; }

private:
			device_node *		fNode;
			uint32				fChannelID;
			ata_controller_interface *fController;
			void *				fCookie;

			spinlock			fInterruptLock;
			ConditionVariable	fInterruptCondition;
			ConditionVariableEntry fInterruptConditionEntry;
			bool				fExpectsInterrupt;

			status_t			fStatus;
			scsi_bus			fSCSIBus;
			uint8				fDeviceCount;
			ATADevice **		fDevices;
			bool				fUseDMA;
			ATARequest *		fRequest;

			char				fDebugContext[16];
};


class ATADevice {
public:
								ATADevice(ATAChannel *channel, uint8 index);
	virtual						~ATADevice();

	// SCSI stuff
			status_t			ModeSense(ATARequest *request);
			status_t			TestUnitReady(ATARequest *request);
			status_t			SynchronizeCache(ATARequest *request);
			status_t			Eject(ATARequest *request);
			status_t			Inquiry(ATARequest *request);
			status_t			ReadCapacity(ATARequest *request);
			status_t			ReadCapacity16(ATARequest *request);
	virtual	status_t			ExecuteIO(ATARequest *request);

			void				GetRestrictions(bool *noAutoSense,
									uint32 *maxBlocks);
			status_t			Control(uint32 op, void *buffer, size_t length);

	// ATA stuff
	virtual	bool				IsATAPI() const { return false; }

			bool				UseDMA() const { return fUseDMA; }
			bool				Use48Bits() const { return fUse48Bits; }
			size_t				BlockSize() const { return fBlockSize; }

			status_t			Select();

			ata_task_file *		TaskFile() { return &fTaskFile; }
			ata_reg_mask		RegisterMask() const { return fRegisterMask; }

			status_t			SetFeature(int feature);
			status_t			DisableCommandQueueing();
			status_t			ConfigureDMA();

	virtual	status_t			Configure();
			status_t			Identify();

			status_t			ExecuteReadWrite(ATARequest *request,
									uint64 address, uint32 sectorCount);

	protected:
			const char *		_DebugContext() { return fDebugContext; }

			ATAChannel *		fChannel;
			ata_device_infoblock fInfoBlock;
			ata_task_file		fTaskFile;
			ata_reg_mask		fRegisterMask;

			bool				fUseDMA;
			uint8				fDMAMode;
			uint8				fDMAFailures;

	private:
			status_t			_FillTaskFile(ATARequest *request,
									uint64 address);

			uint64				fTotalSectors;
			size_t				fBlockSize;
			size_t				fPhysicalBlockSize;
			size_t				fBlockOffset;
			uint8				fIndex;
			bool				fUse48Bits;

			char				fDebugContext[16];
};


class ATAPIDevice : public ATADevice {
public:
								ATAPIDevice(ATAChannel *channel, uint8 index);
	virtual						~ATAPIDevice();

			status_t			SendPacket(ATARequest *request);
	virtual	status_t			ExecuteIO(ATARequest *request);

	virtual	bool				IsATAPI() const { return true; }

	virtual	status_t			Configure();

	private:
			status_t			_FillTaskFilePacket(ATARequest *request);
			status_t			_FinishRequest(ATARequest *request,
									uint32 flags);

private:
			uint8				fPacket[12];
};


class ATARequest {
public:
								ATARequest(bool hasLock);
								~ATARequest();

			void				SetStatus(uint8 status);
			uint8				Status() const { return fStatus; }

			void				ClearSense();
			void				SetSense(uint8 key, uint16 codeQualifier);
			uint8				SenseKey() const { return fSenseKey; }
			uint8				SenseCode() const { return fSenseCode; }
			uint8				SenseQualifier() const
									{ return fSenseQualifier; }

			void				SetDevice(ATADevice *device);
			ATADevice *			Device() const { return fDevice; }

			void				SetTimeout(bigtime_t timeout);
			bigtime_t			Timeout() const { return fTimeout; }

			void				SetIsWrite(bool isWrite);
			bool				IsWrite() const { return fIsWrite; }

			void				SetUseDMA(bool useDMA);
			bool				UseDMA() const { return fUseDMA; }

			void				SetBytesLeft(uint32 bytesLeft);
			size_t *			BytesLeft() { return &fBytesLeft; }

			bool				HasData() const
									{ return fCCB->data_length > 0; }
			bool				HasSense() const { return fSenseKey != 0; }

			status_t			Finish(bool resubmit);

			// SCSI stuff
			status_t			Start(scsi_ccb *ccb);
			scsi_ccb *			CCB() { return fCCB; }

			void				PrepareSGInfo();
			void				AdvanceSG(uint32 bytes);

			uint32				SGElementsLeft() const
									{ return fSGElementsLeft; }
			const physical_entry *CurrentSGElement() const
									{ return fCurrentSGElement; }
			uint32				CurrentSGOffset() const
									{ return fCurrentSGOffset; }

			void				SetOddByte(uint8 byte);
			bool				GetOddByte(uint8 *byte);

			void				RequestSense();

private:
			void				_FillSense(scsi_sense *sense);

			const char *		_DebugContext() { return " request"; };

			mutex				fLock;
			bool				fHasLock;

			uint8				fStatus;
			uint8				fSenseKey;
			uint8				fSenseCode;
			uint8				fSenseQualifier;

			ATADevice *			fDevice;
			bigtime_t			fTimeout;
			size_t				fBytesLeft;
			bool				fIsWrite;
			bool				fUseDMA;
			scsi_ccb *			fCCB;

			uint32				fSGElementsLeft;
			const physical_entry *fCurrentSGElement;
			uint32				fCurrentSGOffset;
			bool				fHasOddByte;
			uint8				fOddByte;
};

#endif // ATA_PRIVATE_H
