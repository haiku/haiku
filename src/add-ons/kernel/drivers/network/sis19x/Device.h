/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS19X_DEVICE_H_
#define _SiS19X_DEVICE_H_


#include <util/Vector.h>

#include "Driver.h"
#include "MIIBus.h"
#include "Registers.h"
#include "DataRing.h"

#include "Settings.h" //!!!


const uint32	MaxFrameSize	= 1514; // 1536??
const bigtime_t	TransmitTimeout = 5000000;


const uint32 TxDescriptorsCount = 65;//34;//32;
const uint32 RxDescriptorsCount = 65;//64;
const uint32 TxDescriptorsMask = TxDescriptorsCount - 1;
const uint32 RxDescriptorsMask = RxDescriptorsCount - 1;


typedef DataRing<TxDescriptor, TxDescriptorsCount> TxDataRing;
typedef DataRing<RxDescriptor, RxDescriptorsCount> RxDataRing;


class Device : private timer {
public:
		class Info {
		public:	
			const uint32		fId;
			const char*			fName;
			const char*			fDescription;
			inline const char*	Name()			{ return fName; }
			inline const char*	Description()	{ return fName; }
			inline uint16		DeviceId()		{ return DEVICEID(fId); }
			inline uint16		VendorId()		{ return VENDORID(fId); }
			inline uint32		Id()			{ return fId; }
		};

							Device(Info &DeviceInfo, pci_info &PCIInfo);
		virtual				~Device();

		status_t			InitCheck() { return fStatus; };

		status_t			Open(uint32 flags);
//		bool				IsOpen() { return fOpen; };

		status_t			Close();
		status_t			Free();

		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);
		
		status_t			SetupDevice();
		void				TeardownDevice();
	
		status_t			_Reset();

		uint8				ReadPCI8(int offset);
		uint16				ReadPCI16(int offset);
		uint32				ReadPCI32(int offset);
		void				WritePCI8(int offset, uint8 value);
		void				WritePCI16(int offset, uint16 value);
		void				WritePCI32(int offset, uint32 value);
	
		cpu_status			Lock();
		void				Unlock(cpu_status st);

static	int32				InterruptHandler(void *InterruptParam);
const	ether_link_state&	LinkState() const { return fMII.LinkState(); }	

protected:

		status_t			GetLinkState(ether_link_state *state);		
		status_t			ReadMACAddress(ether_address_t& address);
		uint16				_ReadEEPROM(uint32 address);
		void				_InitRxFilter();
		status_t			_SetRxMode(uint8* setPromiscuousOn = NULL);
		uint32				_EthernetCRC32(const uint8* buffer, size_t length);
		status_t			_ModifyMulticastTable(bool add,
								ether_address_t* group);

static	int32				_TimerHandler(struct timer* timer);		
		
		// state tracking
		status_t			fStatus;
		pci_info			fPCIInfo;
		Info&				fInfo;
		int					fIOBase;
		int32				fHWSpinlock;
		int32				fInterruptsNest;
		
		// interface and device infos
		uint16				fFrameSize;

		// MII bus handler
		MIIBus				fMII;

		// connection data
		ether_address_t		fMACAddress;
		
		Vector<uint32>		fMulticastHashes;
		
public:
				
		bool				fOpen;
		uint32				fBlockFlag;

		// connection data
		sem_id				fLinkStateChangeSem;
		bool				fHasConnection;
		
		TxDataRing			fTxDataRing;
		RxDataRing			fRxDataRing;

		void				DumpRegisters();

#if STATISTICS
		Statistics			fStatistics;
#endif
};

#endif //_SiS19X_DEVICE_H_

