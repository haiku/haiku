/*
	Driver for Intel(R) PRO/Wireless 2100 devices.
	Copyright (C) 2006 Michael Lotz <mmlr@mlotz.ch>
	Released under the terms of the MIT license.
*/

#ifndef _IPW2100_H_
#define _IPW2100_H_

#include "ipw2100_hw.h"

class IPW2100 {
public:
							IPW2100(int32 deviceID, pci_info *info,
								pci_module_info *module);
							~IPW2100();

		status_t			InitCheck();

		int32				DeviceID() { return fDeviceID; };

		status_t			Open(uint32 flags);
		status_t			Close();
		status_t			Free();

		status_t			Read(off_t position, void *buffer,
								size_t *numBytes);
		status_t			Write(off_t position, const void *buffer,
								size_t *numBytes);
		status_t			Control(uint32 op, void *args, size_t length);

private:
static	int32				InterruptHandler(void *data);
		int32				Interrupt();

		void				HandleTXInterrupt();
		void				HandleRXInterrupt();

		uint32				ReadReg(uint32 offset);
		void				WriteReg(uint32 offset, uint32 value);

		uint8				ReadMem8(uint32 address);
		uint16				ReadMem16(uint32 address);
		uint32				ReadMem32(uint32 address);

		void				WriteMem8(uint32 address, uint8 value);
		void				WriteMem16(uint32 address, uint16 value);
		void				WriteMem32(uint32 address, uint32 value);

		status_t			ReadOrdinal(uint32 index, uint8 *buffer,
								size_t *size);
		status_t			WriteOrdinal(uint32 index, const uint8 *buffer,
								size_t *size);

		status_t			SendCommand(uint32 commandId, const uint8 *buffer,
								size_t length);

		area_id				MapPhysicalMemory(const char *name,
								uint32 physicalAddress, void **logicalAddress,
								size_t size);
		area_id				AllocateContiguous(const char *name,
								void **logicalAddress, uint32 *physicalAddress,
								size_t size);

		void				SetInterrupts(uint32 interrupts);
		uint32				Interrupts();

		status_t			CheckAdapterAccess();
		status_t			SoftResetAdapter();
		status_t			ResetAdapter();
		status_t			StopMaster();
		status_t			SetupBuffers();
		status_t			ReadMACAddress();
		status_t			LoadMicrocodeAndFirmware();
		status_t			LoadMicrocode(const uint8 *buffer, uint32 size);
		status_t			LoadFirmware(const uint8 *buffer, uint32 size);
		status_t			ClearSharedMemory();
		status_t			StartFirmware();
		status_t			EtherInit();
		status_t			DisableAdapter();
		status_t			EnableAdapter();
		status_t			ScanForNetworks();
		status_t			DisableRadio();
		status_t			PowerDown();

		status_t			fStatus;
		bool				fClosing;

		sem_id				fFWInitDoneSem;
		sem_id				fAdapterDisabledSem;
		sem_id				fTXTransferSem;
		sem_id				fRXTransferSem;
		sem_id				fCommandDoneSem;

		int32				fDeviceID;
		pci_info			*fPCIInfo;
		pci_module_info		*fPCIModule;

		uint32				fRegisterBase;
		uint8				*fRegisters;
		area_id				fRegistersArea;

		area_id				fTXRingArea;
		ipw_bd				*fTXRingLog;
		uint32				fTXRingPhy;

		area_id				fTXPacketsArea;
		ipw_tx				*fTXPacketsLog;
		uint32				fTXPacketsPhy;

		area_id				fRXRingArea;
		ipw_bd				*fRXRingLog;
		uint32				fRXRingPhy;

		area_id				fRXPacketsArea;
		ipw_rx				*fRXPacketsLog;
		uint32				fRXPacketsPhy;

		area_id				fStatusRingArea;
		ipw_status			*fStatusRingLog;
		uint32				fStatusRingPhy;

		area_id				fCommandArea;
		ipw_command			*fCommandLog;
		uint32				fCommandPhy;

		uint32				fInterruptMask;
		uint32				fTXPosition;
		uint32				fRXPosition;
		bool				fAssociated;

		uint32				fReadIndex;
		bool				*fReadQueue;

		uint32				fOrdinalTable1Base;
		uint32				fOrdinalTable2Base;
		uint32				fOrdinalTable1Length;
		uint32				fOrdinalTable2Length;

		// settings
		int32				fInterruptLine;
		uint8				fMACAddress[6];
		uint32				fMode;
		uint32				fChannel;
		uint32				fTXRates;
		uint32				fPowerMode;
		uint32				fRTSThreshold;
		uint32				fScanOptions;
		uint8				fAuthMode;
		uint32				fCiphers;
		char				*fESSID;
		ipw_wep_key			fWEPKeys[4];
		uint32				fWEPKeyIndex;
		uint32				fWEPFlags;
		bool				fDumpTXPackets;
		bool				fDumpRXPackets;
};

#endif // _IPW2100_H_
