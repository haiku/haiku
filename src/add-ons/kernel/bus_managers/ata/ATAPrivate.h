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
#include "ATATracing.h"
#include "ata_device_infoblock.h"

#define ATA_STANDARD_TIMEOUT		10 * 1000 * 1000
#define ATA_RELEASE_TIMEOUT			10 * 1000 * 1000
#define ATA_SIGNATURE_ATAPI			0xeb140101
#define ATA_SIM_MODULE_NAME			"bus_managers/ata/sim/driver_v1"
#define ATA_CHANNEL_ID_GENERATOR	"ata/channel_id"
#define ATA_CHANNEL_ID_ITEM			"ata/channel_id"

enum {
	ATA_DEVICE_READY_REQUIRED	= 0x01,
	ATA_IS_WRITE				= 0x02,
	ATA_DMA_TRANSFER			= 0x03,
	ATA_CHECK_ERROR_BIT			= 0x04,
	ATA_WAIT_FINISH				= 0x08,
	ATA_WAIT_ANY_BIT			= 0x10
};


class ATADevice;
class ATARequest;

extern scsi_for_sim_interface *gSCSIModule;
extern device_manager_info *gDeviceManager;

bool copy_sg_data(scsi_ccb *ccb, uint offset, uint allocationLength,
	void *buffer, int size, bool toBuffer);

class ATAChannel {
public:
									ATAChannel(device_node *node);
									~ATAChannel();

		status_t					InitCheck();
		uint32						ChannelID() { return fChannelID; };

		// SCSI stuff
		void						SetBus(scsi_bus bus);
		scsi_bus					Bus() { return fSCSIBus; };
		status_t					ScanBus();

		void						PathInquiry(scsi_path_inquiry *info);
		void						GetRestrictions(uint8 targetID,
										bool *isATAPI, bool *noAutoSense,
										uint32 *maxBlocks);
		status_t					ExecuteIO(scsi_ccb *ccb);

		// ATA stuff
		status_t					SelectDevice(uint8 index);
		bool						IsDevicePresent(uint8 index);

		status_t					Reset(bool *presence, uint32 *signatures);

		bool						UseDMA() { return fUseDMA; };

		status_t					Wait(uint8 setBits, uint8 clearedBits,
										uint32 flags, bigtime_t timeout);
		status_t					WaitDataRequest(bool high);
		status_t					WaitDeviceReady();
		status_t					WaitForIdle();

		// request handling
		status_t					SendRequest(ATARequest *request,
										uint32 flags);
		status_t					FinishRequest(ATARequest *request,
										uint32 flags, uint8 errorMask);

		// data transfers
		status_t					ExecuteDMATransfer(ATARequest *request);
		status_t					ExecutePIOTransfer(ATARequest *request);

		status_t					ReadPIO(uint8 *buffer, size_t length);

private:
		status_t					_ReadRegs(ata_task_file *taskFile,
										ata_reg_mask mask);
		status_t					_WriteRegs(ata_task_file *taskFile,
										ata_reg_mask mask);
		status_t					_WriteControl(uint8 value);

		uint8						_AltStatus();
		void						_FlushAndWait(bigtime_t waitTime);

		status_t					_ReadPIOBlock(ATARequest *request,
										size_t length);
		status_t					_WritePIOBlock(ATARequest *request,
										size_t length);
		status_t					_TransferPIOBlock(ATARequest *request,
										size_t length, size_t *transferred);
		status_t					_TransferPIOPhysical(ATARequest *request,
										addr_t physicalAddress, size_t length,
										size_t *transferred);
		status_t					_TransferPIOVirtual(ATARequest *request,
										uint8 *virtualAddress, size_t length,
										size_t *transferred);

		const char *				_DebugContext() { return fDebugContext; };

		device_node *				fNode;
		uint32						fChannelID;
		ata_controller_interface *	fController;
		void *						fCookie;
		mutex						fLock;
		status_t					fStatus;
		scsi_bus					fSCSIBus;
		uint8						fDeviceCount;
		ATADevice **				fDevices;
		bool						fUseDMA;
		ATARequest *				fRequest;

		char						fDebugContext[16];
};


class ATADevice {
public:
									ATADevice(ATAChannel *channel,
										uint8 index);
virtual								~ATADevice();

		// SCSI stuff
		status_t					ModeSense(ATARequest *request);
		status_t					TestUnitReady(ATARequest *request);
		status_t					SynchronizeCache(ATARequest *request);
		status_t					Eject(ATARequest *request);
		status_t					Inquiry(ATARequest *request);
		status_t					ReadCapacity(ATARequest *request);
virtual	status_t					ExecuteIO(ATARequest *request);

		// ATA stuff
virtual	bool						IsATAPI() { return false; };

		bool						UseDMA() { return fUseDMA; };
		bool						UseLBA() { return fUseLBA; };
		bool						Use48Bits() { return fUse48Bits; };

		status_t					Select();

		ata_task_file *				TaskFile() { return &fTaskFile; };
		ata_reg_mask				RegisterMask() { return fRegisterMask; };

		status_t					SetFeature(int feature);
		status_t					DisableCommandQueueing();
		status_t					ConfigureDMA();

		status_t					Configure();
		status_t					Identify();

		status_t					ExecuteReadWrite(ATARequest *request,
										uint64 address, uint32 sectorCount);

private:
		status_t					_FillTaskFile(ATARequest *request,
										uint64 address);

		status_t					_PrepareDMA(ATARequest *request);
		status_t					_PreparePIO(ATARequest *request);

		const char *				_DebugContext() { return fDebugContext; };

		ATAChannel *				fChannel;
		uint8						fIndex;
		bool						fUseLBA;
		bool						fUse48Bits;
		bool						fUseDMA;
		uint64						fTotalSectors;
		ata_device_infoblock		fInfoBlock;
		ata_task_file				fTaskFile;
		ata_reg_mask				fRegisterMask;

		char						fDebugContext[16];
};


class ATAPIDevice : public ATADevice {
public:
									ATAPIDevice(ATAChannel *channel,
										uint8 index);
virtual								~ATAPIDevice();

virtual	status_t					ExecuteIO(ATARequest *request);

virtual	bool						IsATAPI() { return true; };

private:
		uint8						fLastLun;
};


class ATARequest {
public:
									ATARequest();

		void						SetStatus(uint8 status);
		uint8						Status() { return fStatus; };

		void						ClearSense();
		void						SetSense(uint8 key, uint16 codeQualifier);
		uint8						SenseKey() { return fSenseKey; };
		uint8						SenseCode() { return fSenseCode; };
		uint8						SenseQualifier() { return fSenseQualifier; };

		void						SetDevice(ATADevice *device);
		ATADevice *					Device() { return fDevice; };

		void						SetTimeout(bigtime_t timeout);
		bigtime_t					Timeout() { return fTimeout; };

		void						SetIsWrite(bool isWrite);
		bool						IsWrite() { return fIsWrite; };

		void						SetUseDMA(bool useDMA);
		bool						UseDMA() { return fUseDMA; };

		void						SetBlocksLeft(uint32 blocksLeft);
		uint32 *					BlocksLeft() { return &fBlocksLeft; };

		status_t					Finish(bool resubmit);

		// SCSI stuff
		void						SetCCB(scsi_ccb *ccb);
		scsi_ccb *					CCB() { return fCCB; };

		void						PrepareSGInfo();
		void						AdvanceSG(uint32 bytes);

		uint32						SGElementsLeft()
										{ return fSGElementsLeft; };
		const physical_entry *		CurrentSGElement()
										{ return fCurrentSGElement; };
		uint32						CurrentSGOffset()
										{ return fCurrentSGOffset; };

		void						SetOddByte(uint8 byte);
		bool						GetOddByte(uint8 *byte);

		void						RequestSense();

private:
		void						_FillSense(scsi_sense *sense);

		const char *				_DebugContext() { return " request"; };

		uint8						fStatus;
		uint8						fSenseKey;
		uint8						fSenseCode;
		uint8						fSenseQualifier;

		ATADevice *					fDevice;
		bigtime_t					fTimeout;
		uint32						fBlocksLeft;
		bool						fIsWrite;
		bool						fUseDMA;
		scsi_ccb *					fCCB;

		uint32						fSGElementsLeft;
		const physical_entry *		fCurrentSGElement;
		uint32						fCurrentSGOffset;
		bool						fHasOddByte;
		uint8						fOddByte;
};

#endif // ATA_PRIVATE_H
