/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS7018_DEVICE_H_
#define _SiS7018_DEVICE_H_


#include <KernelExport.h>

#include "Driver.h"
#include "Mixer.h"
#include "Registers.h"
#include "Settings.h"
#include "Stream.h"


// card ids for supported audio hardware
const uint32 SiS7018	= 0x70181039;
const uint32 ALi5451	= 0x545110b9;
const uint32 TridentDX	= 0x20001023;
const uint32 TridentNX	= 0x20011023;


class Device {

public:
		class Info {
		public:	
			const uint32		fId;
			const char*			fName;
			inline const char*	Name()		{ return fName; }
			inline uint16		DeviceId()	{ return (fId >> 16) & 0xffff; }
			inline uint16		VendorId()	{ return (fId) & 0xffff; }
			inline uint32		Id()		{ return fId; }
		};

		uint32				HardwareId() { return fInfo.Id(); }
		pci_info&			PCIInfo() { return fPCIInfo; }


							Device(Info &DeviceInfo, pci_info &PCIInfo);
							~Device();

		status_t			Setup();
		status_t			InitCheck() { return fStatus; };

		status_t			Open(uint32 flags);
		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);
		status_t			Close();
		status_t			Free();

static	int32				InterruptHandler(void *interruptParam);
		void				SignalReadyBuffers();

		class Mixer&		Mixer() { return fMixer; }
		
		cpu_status			Lock();
		void				Unlock(cpu_status st);

		uint8				ReadPCI8(int offset);
		uint16				ReadPCI16(int offset);
		uint32				ReadPCI32(int offset);
		void				WritePCI8(int offset, uint8 value);
		void				WritePCI16(int offset, uint16 value);
		void				WritePCI32(int offset, uint32 value);

private:
		status_t			_ReserveDeviceOnBus(bool reserve);
		void				_ResetCard(uint32 resetMask, uint32 releaseMask);

		status_t			_MultiGetDescription(multi_description *Description);
		status_t			_MultiGetEnabledChannels(multi_channel_enable *Enable);
		status_t			_MultiSetEnabledChannels(multi_channel_enable *Enable);
		status_t			_MultiGetBuffers(multi_buffer_list* List);
		status_t			_MultiGetGlobalFormat(multi_format_info *Format);
		status_t			_MultiSetGlobalFormat(multi_format_info *Format);
		status_t			_MultiGetMix(multi_mix_value_info *Info);
		status_t			_MultiSetMix(multi_mix_value_info *Info);
		status_t			_MultiListMixControls(multi_mix_control_info* Info);
		status_t			_MultiBufferExchange(multi_buffer_info* Info);

		status_t			fStatus;
		pci_info			fPCIInfo;
		Info&				fInfo;
		int					fIOBase;
		int32				fHWSpinlock;
		int32				fInterruptsNest;
		sem_id				fBuffersReadySem;

		class Mixer			fMixer;
		Stream				fPlaybackStream;
		Stream				fRecordStream;
};

#endif // _SiS7018_DEVICE_H_

