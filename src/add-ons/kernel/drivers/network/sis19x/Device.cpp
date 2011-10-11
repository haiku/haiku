/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Device.h"

#include <net/if_media.h>
#include <lock.h>

#include "Driver.h"
#include "Settings.h"
#include "Registers.h"


Device::Device(Device::Info &DeviceInfo, pci_info &PCIInfo)
		:	
		fStatus(B_ERROR),
		fPCIInfo(PCIInfo),
		fInfo(DeviceInfo),
		fIOBase(0),
		fHWSpinlock(0),
		fInterruptsNest(0),
		fFrameSize(MaxFrameSize),
		fMII(this),
		fOpen(false),
		fBlockFlag(0),		
		fLinkStateChangeSem(-1),
		fHasConnection(false),
		fTxDataRing(this, true),
		fRxDataRing(this, false)
{
	memset((struct timer*)this, 0, sizeof(struct timer));	

	uint32 cmdRegister = gPCIModule->read_pci_config(PCIInfo.bus,
			PCIInfo.device,	PCIInfo.function, PCI_command, 2);
	TRACE_ALWAYS("cmdRegister:%#010x\n", cmdRegister);
	cmdRegister |= PCI_command_io | PCI_command_memory | PCI_command_master;
	gPCIModule->write_pci_config(PCIInfo.bus, PCIInfo.device,
			PCIInfo.function, PCI_command, 2, cmdRegister);

	fIOBase = PCIInfo.u.h0.base_registers[1];
	TRACE_ALWAYS("fIOBase:%#010x\n", fIOBase);

	fStatus = B_OK;
}


Device::~Device()
{
}


status_t
Device::Open(uint32 flags)
{
	TRACE("flags:%x\n", flags);
	if (fOpen) {
		TRACE_ALWAYS("An attempt to re-open device ignored.\n");
		return B_BUSY;
	}

	status_t result = fMII.Init();
	if (result != B_OK) {
		TRACE_ALWAYS("MII initialization failed: %#010x.\n", result);
		return result;
	}

	_Reset();

	if ((fMII.LinkState().media & IFM_ACTIVE) == 0/*fNegotiationComplete*/) {
		fMII.UpdateLinkState();
	}

	fMII.SetMedia();

	WritePCI32(RxMACAddress, 0);
	_InitRxFilter();

	fRxDataRing.Open();
	fTxDataRing.Open();

	if (atomic_add(&fInterruptsNest, 1) == 0) {
		install_io_interrupt_handler(fPCIInfo.u.h0.interrupt_line,
				InterruptHandler, this, 0);
		TRACE("Interrupt handler installed at line %d.\n",
				fPCIInfo.u.h0.interrupt_line);
	}

	_SetRxMode(false);

	// enable al known interrupts
	WritePCI32(IntMask, knownInterruptsMask);

	// enable Rx and Tx
	uint32 control = ReadPCI32(RxControl);
	control |= RxControlEnable | RxControlPoll;
	WritePCI32(RxControl, control);

	control = ReadPCI32(TxControl);
	control |= TxControlEnable /*| TxControlPoll*/;
	WritePCI32(TxControl, control);

	add_timer((timer*)this, _TimerHandler, 1000000LL, B_PERIODIC_TIMER);

	//fNonBlocking = (flags & O_NONBLOCK) == O_NONBLOCK;
	fOpen = true;
	return B_OK;
}


status_t
Device::Close()
{
	TRACE("closed!\n");

	// disable interrupts
	WritePCI32(IntMask, 0);
	spin(2000);

	// Stop Tx / Rx status machine
	uint32 status = ReadPCI32(IntControl);
	status |= 0x00008000;
	WritePCI32(IntControl, status);
	spin(50);
	status &= ~0x00008000;
	WritePCI32(IntControl, status);

	if (atomic_add(&fInterruptsNest, -1) == 1) {
		remove_io_interrupt_handler(fPCIInfo.u.h0.interrupt_line,
				InterruptHandler, this);
		TRACE("Interrupt handler at line %d uninstalled.\n",
				fPCIInfo.u.h0.interrupt_line);
	}

	fRxDataRing.Close();
	fTxDataRing.Close();

	cancel_timer((timer*)this);

	TRACE("timer cancelled\n");

	fOpen = false;

	return B_OK;
}


status_t
Device::Free()
{
	//	fRxDataRing.Free();
	//	fTxDataRing.Free();

	TRACE("freed\n");
	return B_OK;
}


status_t
Device::Read(uint8 *buffer, size_t *numBytes)
{
	return fRxDataRing.Read(buffer, numBytes);
}


status_t
Device::Write(const uint8 *buffer, size_t *numBytes)
{
	if ((fMII.LinkState().media & IFM_ACTIVE) == 0) {
		TRACE_ALWAYS("Write failed. link is inactive!\n");
		return B_OK; // return OK because of well-known DHCP "moustreap"!
	}

	return fTxDataRing.Write(buffer, numBytes);
}


status_t
Device::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			TRACE("ETHER_INIT\n");
			return B_OK;

		case ETHER_GETADDR:
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			TRACE("ETHER_GETADDR %#02x:%#02x:%#02x:%#02x:%#02x:%#02x\n",
					fMACAddress.ebyte[0], fMACAddress.ebyte[1],
					fMACAddress.ebyte[2], fMACAddress.ebyte[3],
					fMACAddress.ebyte[4], fMACAddress.ebyte[5]);
			return B_OK;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)buffer = fFrameSize;
			TRACE("ETHER_ETHER_GETFRAMESIZE:%d\n",fFrameSize);
			return B_OK;

		case ETHER_NONBLOCK:
			TRACE("ETHER_NONBLOCK\n");
			fBlockFlag = *((uint32*)buffer) ? B_TIMEOUT : 0;
			return B_OK;

		case ETHER_SETPROMISC:
			TRACE("ETHER_SETPROMISC\n");
			return _SetRxMode(*((uint8*)buffer));

		case ETHER_ADDMULTI:
		case ETHER_REMMULTI:
			TRACE_ALWAYS("Multicast operations are not implemented.\n");
			return B_ERROR;

		case ETHER_SET_LINK_STATE_SEM:
			fLinkStateChangeSem = *(sem_id *)buffer;
			TRACE_ALWAYS("ETHER_SET_LINK_STATE_SEM\n");
			return B_OK;

		case ETHER_GET_LINK_STATE:
			return GetLinkState((ether_link_state *)buffer);

		default:
			TRACE_ALWAYS("Unhandled IOCTL catched: %#010x\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


status_t
Device::SetupDevice()
{
	ether_address address;
	status_t result = ReadMACAddress(address);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading MAC address:%#010x\n", result);
		return result;
	}

	TRACE("MAC address is:%02x:%02x:%02x:%02x:%02x:%02x\n",
			address.ebyte[0], address.ebyte[1], address.ebyte[2],
			address.ebyte[3], address.ebyte[4], address.ebyte[5]);

	fMACAddress = address;

	uint16 info = _ReadEEPROM(EEPROMInfo);
	fMII.SetRGMII((info & 0x0080) != 0);

	TRACE("RGMII is '%s'. EEPROM info word:%#06x.\n",
			fMII.HasRGMII() ? "on" : "off", info);

	fMII.SetGigagbitCapable(fInfo.Id() == SiS191);

	return B_OK;
}


void
Device::TeardownDevice()
{

}


uint8
Device::ReadPCI8(int offset)
{
	return gPCIModule->read_io_8(fIOBase + offset);
}


uint16
Device::ReadPCI16(int offset)
{
	return gPCIModule->read_io_16(fIOBase + offset);
}


uint32
Device::ReadPCI32(int offset)
{
	return gPCIModule->read_io_32(fIOBase + offset);
}


void
Device::WritePCI8(int offset, uint8 value)
{
	gPCIModule->write_io_8(fIOBase + offset, value);
}


void
Device::WritePCI16(int offset, uint16 value)
{
	gPCIModule->write_io_16(fIOBase + offset, value);
}


void
Device::WritePCI32(int offset, uint32 value)
{
	gPCIModule->write_io_32(fIOBase + offset, value);
}

/*
   cpu_status
   Device::Lock() 
   {
   cpu_status st = disable_interrupts();
   acquire_spinlock(&fHWSpinlock);
   return st;
   }


   void
   Device::Unlock(cpu_status st) 
   {
   release_spinlock(&fHWSpinlock);
   restore_interrupts(st);
   }
   */

int32
Device::InterruptHandler(void *InterruptParam)
{
	Device *device = (Device*)InterruptParam;
	if(device == 0) {
		TRACE_ALWAYS("Invalid parameter in the interrupt handler.\n");
		return B_HANDLED_INTERRUPT;
	}

	int32 result = B_UNHANDLED_INTERRUPT;

	acquire_spinlock(&device->fHWSpinlock);

	// disable interrupts...
	device->WritePCI32(IntMask, 0);

	//int maxWorks = 40;

	//do {
	uint32 status = device->ReadPCI32(IntSource);

#if STATISTICS
	device->fStatistics.PutStatus(status);
#endif
	device->WritePCI32(IntSource, status);

	if ((status & knownInterruptsMask) != 0) {
		//break;
		//}

		// XXX: ????
		result = B_HANDLED_INTERRUPT;

	if ((status & (/*INT_TXIDLE |*/ INT_TXDONE)) != 0 ) {
		result = device->fTxDataRing.InterruptHandler();
	}

	if ((status & (/*INT_RXIDLE |*/ INT_RXDONE)) != 0 ) {
		result = device->fRxDataRing.InterruptHandler();
	}

	/*if ((status & (INT_LINK)) != 0 ) {
	//if (!device->fMII.isLinkUp()) {
	device->fTxDataRing.CleanUp();
	//}
	}*/
	}

	//} while (--maxWorks > 0);

	// enable interrupts...
	device->WritePCI32(IntMask, knownInterruptsMask);

	release_spinlock(&device->fHWSpinlock);

	return result;
}


status_t
Device::GetLinkState(ether_link_state *linkState)
{
	status_t result = user_memcpy(linkState, &fMII.LinkState(),
			sizeof(ether_link_state));

#if STATISTICS
	fStatistics.Trace();
	fRxDataRing.Trace();
	fTxDataRing.Trace();
	uint32 rxControl = ReadPCI32(RxControl);
	uint32 txControl = ReadPCI32(TxControl);
	TRACE_ALWAYS("RxControl:%#010x;TxControl:%#010x\n", rxControl, txControl);
#endif

	TRACE_FLOW("Medium state: %s, %lld MBit/s, %s duplex.\n", 
			(linkState->media & IFM_ACTIVE) ? "active" : "inactive",
			linkState->speed / 1000,
			(linkState->media & IFM_FULL_DUPLEX) ? "full" : "half");

	return result;
}


status_t
Device::_SetRxMode(bool isPromiscuousModeOn)
{
	// clean the Rx MAC Control register
	WritePCI16(RxMACControl, (ReadPCI16(RxMACControl) & ~RXM_Mask));

	uint16 rxMode = RXM_Broadcast | RXM_Multicast | RXM_Physical;
	if (isPromiscuousModeOn) {
		rxMode |= RXM_AllPhysical;
	}

	// set multicast filters
	WritePCI32(RxHashTable, 0xffffffff);
	WritePCI32(RxHashTable + 4, 0xffffffff);

	// update rx mode
	WritePCI16(RxMACControl, ReadPCI16(RxMACControl) | rxMode);

	return B_OK;
}


int32
Device::_TimerHandler(struct timer* timer)
{
	Device* device = (Device*)timer;

	bool linkChanged = false;
	int32 result = device->fMII.TimerHandler(&linkChanged);

	if (linkChanged) {
		if (device->fMII.IsLinkUp()) {
			device->fTxDataRing.CleanUp();
			//device->WritePCI32(IntControl, 0x8000);
			//device->ReadPCI32(IntControl);
			//spin(100);
			//device->WritePCI32(IntControl, 0x0);
		}
	}

	if (linkChanged && device->fLinkStateChangeSem > B_OK) {
		release_sem_etc(device->fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
	}

	return result;
}


status_t
Device::_Reset()
{
	// disable interrupts
	WritePCI32(IntMask, 0);
	WritePCI32(IntSource, 0xffffffff);

	// reset Rx & Tx
	WritePCI32(TxControl, 0x00001c00);
	WritePCI32(RxControl, 0x001e1c00);

	WritePCI32(IntControl, 0x8000);
	ReadPCI32(IntControl);
	spin(100);
	WritePCI32(IntControl, 0x0);

	WritePCI32(IntMask, 0);
	WritePCI32(IntSource, 0xffffffff);

	// initial values for all MAC registers
	WritePCI32(TxBase,			0x0);
	WritePCI32(TxReserved,		0x0);
	WritePCI32(RxBase,			0x0);
	WritePCI32(RxReserved,		0x0);

	WritePCI32(PowControl,		0xffc00000);
	WritePCI32(Reserved0,		0x0);

	WritePCI32(StationControl,	fMII.HasRGMII() ? 0x04008001 : 0x04000001);
	WritePCI32(GIoCR,			0x0);
	WritePCI32(GIoControl,		0x0);

	WritePCI32(TxMACControl,	0x00002364);
	WritePCI32(TxLimit,			0x0000000f);

	WritePCI32(RGDelay,			0x0);
	WritePCI32(Reserved1,		0x0);
	WritePCI32(RxMACControl,	0x00000252);

	WritePCI32(RxHashTable,		0x0);
	WritePCI32(RxHashTable + 4,	0x0);

	WritePCI32(RxWOLControl,	0x80ff0000);
	WritePCI32(RxWOLData,		0x80ff0000);
	WritePCI32(RxMPSControl,	0x0);
	WritePCI32(Reserved2,		0x0);

	return B_OK;
}


void
Device::_InitRxFilter()
{
	// store filter value
	uint16 filter = ReadPCI16(RxMACControl);

	// disable disable packet filtering before address is set
	WritePCI32(RxMACControl, (filter & ~RXM_Mask));

	for (size_t i = 0; i < _countof(fMACAddress.ebyte); i++) {
		WritePCI8(RxMACAddress + i, fMACAddress.ebyte[i]);
	}

	// enable packet filtering
	WritePCI16(RxMACControl, filter);
}


uint16
Device::_ReadEEPROM(uint32 address)
{
	if (address > EIOffset) {
		TRACE_ALWAYS("EEPROM address %#08x is invalid.\n", address);
		return EIInvalid;
	}

	WritePCI32(EEPROMInterface, EIReq | EIOpRead | (address << EIOffsetShift));

	spin(500); // 500 ms?

	for (size_t i = 0; i < 1000; i++) {
		uint32 data = ReadPCI32(EEPROMInterface);
		if ((data & EIReq) == 0) {
			return (data & EIData) >> EIDataShift;
		}
		spin(100); // 100 ms?
	}

	TRACE_ALWAYS("timeout reading EEPROM.\n");

	return EIInvalid;
}


status_t
Device::ReadMACAddress(ether_address_t& address)
{
	uint16 signature = _ReadEEPROM(EEPROMSignature);
	TRACE("EEPROM Signature: %#06x\n", signature);

	if (signature != 0x0000 && signature != EIInvalid) {
		for (size_t i = 0; i < _countof(address.ebyte) / 2; i++) {
			uint16 addr = _ReadEEPROM(EEPROMAddress + i);
			address.ebyte[i * 2 + 0] = (uint8)addr;
			address.ebyte[i * 2 + 1] = (uint8)(addr >> 8);
		}

		return B_OK;
	}

	// SiS96x can use APC CMOS RAM to store MAC address,
	// this is accessed through ISA bridge.
	uint32 register73 = gPCIModule->read_pci_config(fPCIInfo.bus,
			fPCIInfo.device, fPCIInfo.function, 0x73, 1);
	TRACE_ALWAYS("Config register x73:%#010x\n", register73);

	if ((register73 & 0x00000001) == 0)
		return B_ERROR;

	// look for PCI-ISA bridge
	uint16 ids[] = { 0x0965, 0x0966, 0x0968 };

	pci_info pciInfo = {0};
	for (long i = 0; B_OK == (*gPCIModule->get_nth_pci_info)(i, &pciInfo); i++) {
		if (pciInfo.vendor_id != 0x1039)
			continue;

		for (size_t idx = 0; idx < _countof(ids); idx++) {
			if (pciInfo.device_id == ids[idx]) {

				// enable ports 0x78 0x79 to access APC registers
				uint32 reg = gPCIModule->read_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1);
				reg &= ~0x02;
				gPCIModule->write_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1, reg);
				snooze(50);
				reg = gPCIModule->read_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1);

				// read factory MAC address
				for (size_t i = 0; i < _countof(address.ebyte); i++) {
					gPCIModule->write_io_8(0x78, 0x09 + i);
					address.ebyte[i] = gPCIModule->read_io_8(0x79);
				}

				// check MII/RGMII
				gPCIModule->write_io_8(0x78, 0x12);
				uint8 u8 = gPCIModule->read_io_8(0x79);
			// TODO: set RGMII in fMII correctly!
			//	bool bRGMII = (u8 & 0x80) != 0;
				TRACE_ALWAYS("RGMII: %#04x\n", u8);

				// close access to APC registers
				gPCIModule->write_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1, reg);

				return B_OK;
			}
		}
	}

	TRACE_ALWAYS("ISA bridge was not found.\n");
	return B_ERROR;
}


void
Device::DumpRegisters()
{
	struct RegisterEntry {
		uint32 Base;
		const char* Name;
		bool writeBack;
	} RegisterEntries[] = {
		{ TxControl,		"TxControl",	false },
		{ TxBase,			"TxBase\t",		false },
		{ TxStatus,			"TxStatus",		false },
		{ TxReserved,		"TxReserved",	false },
		{ RxControl,		"RxControl",	false },
		{ RxBase,			"RxBase\t",		false },
		{ RxStatus,			"RxStatus",		false },
		{ RxReserved,		"RxReserved",	false },
		{ IntSource,		"IntSource",	true },
		{ IntMask,			"IntMask",		false },
		{ IntControl,		"IntControl",	false },
		{ IntTimer,			"IntTimer",		false },
		{ PowControl,		"PowControl",	false },
		{ Reserved0,		"Reserved0",	false },
		{ EEPROMControl,	"EEPROMCntl",	false },
		{ EEPROMInterface,	"EEPROMIface",	false },
		{ StationControl,	"StationCntl",	false },
		{ SMInterface,		"SMInterface",	false },
		{ GIoCR,			"GIoCR\t",		false },
		{ GIoControl,		"GIoControl",	false },
		{ TxMACControl,		"TxMACCntl",	false },
		{ TxLimit,			"TxLimit",		false },
		{ RGDelay,			"RGDelay",		false },
		{ Reserved1,		"Reserved1",	false },
		{ RxMACControl,		"RxMACCntlEtc",	false },
		{ RxMACAddress + 2,	"RxMACAddr2",	false },
		{ RxHashTable,		"RxHashTable1",	false },
		{ RxHashTable + 4,	"RxHashTable2",	false },
		{ RxWOLControl,		"RxWOLControl",	false },
		{ RxWOLData,		"RxWOLData",	false },
		{ RxMPSControl,		"RxMPSControl",	false },
		{ Reserved2,		"Reserved2",	false }
	};

	for (size_t i = 0; i < _countof(RegisterEntries); i++) {
		uint32 registerContents = ReadPCI32(RegisterEntries[i].Base);
		kprintf("%s:\t%08lx\n", RegisterEntries[i].Name, registerContents);
		if (RegisterEntries[i].writeBack) {
			WritePCI32(RegisterEntries[i].Base, registerContents);
		}
	}
}

