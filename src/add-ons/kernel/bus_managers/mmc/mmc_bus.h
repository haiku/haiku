/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#ifndef MMC_BUS_H
#define MMC_BUS_H


#include <new>
#include <stdio.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>
#include "mmc.h"


#define MMCBUS_TRACE
#ifdef MMCBUS_TRACE
#	define TRACE(x...)		dprintf("\33[33mmmc_bus:\33[0m " x)
#else
#	define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mmmc_bus:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33mmmc_bus:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


class CidWrapper {
	public:
		CidWrapper(const uint32 cid[4])
		{
			fWords[0] = cid[0];
			fWords[1] = cid[1];
			fWords[2] = cid[2];
			fWords[3] = cid[3];
		}

		uint32_t VendorID() const
		{
			return (fWords[3] >> 24) & 0xFF;
		}

		uint32_t ProductSerial() const
		{
			return (fWords[1] << 16) | (fWords[0] >> 16);
		}

		uint16_t ProductRevision() const
		{
			uint16 rev = (fWords[1] >> 20) & 0xF;
			rev *= 100;
			rev += (fWords[1] >> 16) & 0xF;
			return rev;
		}

	protected:
		uint32_t fWords[4];
};


// per Physical Layer Simplified Specification Version 9.10
// sec 5.2
class SDCid : public CidWrapper {
	public:
		SDCid(const uint32 cid[4])
			: 
			CidWrapper(cid)
		{
		}

		void ProductName(char out[6]) const
		{
			out[0] = (char)(fWords[2] >> 24);
			out[1] = (char)(fWords[2] >> 16);
			out[2] = (char)(fWords[2] >> 8);
			out[3] = (char)(fWords[2]);
			out[4] = (char)(fWords[1] >> 24);
			out[5] = '\0';
		}

		uint8_t  ManufactureMonth() const { return (fWords[0] >> 8) & 0xF; }
		uint16_t ManufactureYear()  const { return 2000 + ((fWords[0] >> 12) & 0xFF); }
};


// per JEDEC Standard No. 84-B51, sec 7.2
class MMCCid : public CidWrapper {
	public:
		MMCCid(const uint32 cid[4])
			:
			CidWrapper(cid)
		{
		}

		void ProductName(char out[7]) const
		{
			out[0] = (char)(fWords[3] & 0xFF);
			out[1] = (char)(fWords[2] >> 24);
			out[2] = (char)(fWords[2] >> 16);
			out[3] = (char)(fWords[2] >> 8);
			out[4] = (char)(fWords[2]);
			out[5] = (char)(fWords[1] >> 24);
			out[6] = '\0';
		}

		uint8_t ManufactureMonth() const
		{
			// detailed in sec 7.2.7 MDT[15:8]
			return (fWords[0] >> 12) & 0x0F;
		}

		uint16_t ManufactureYear(bool modern) const
		{
			// For eMMC 4.41 and later devices,
			// indicated by a value larger than 4 in EXT_CSD_REV[192]
			// year offset is 2013 instead of 1997.
			uint8_t yearOffset = (fWords[0] >> 8) & 0x0F;
			return  yearOffset + (modern? 2013 : 1997);
		}
};


extern device_manager_info *gDeviceManager;


class MMCBus;

class MMCBus {
public:

								MMCBus(device_node *node);
								~MMCBus();
				status_t		InitCheck();
				void			Rescan();

				status_t		ExecuteCommand(uint16_t rca, uint8_t command,
									uint32_t argument, uint32_t* response);
				status_t		DoIO(uint16_t rca, uint8_t command,
									IOOperation* operation,
									bool offsetAsSectors);

				void			SetClock(int frequency);
				void			SetBusWidth(int width);
				void			SetCardType(card_type type);

				void			AcquireBus() { acquire_sem(fLockSemaphore); }
				void			ReleaseBus() { release_sem(fLockSemaphore); }
private:
				status_t		_ActivateDevice(uint16_t rca);
				void			_AcquireScanSemaphore();
				void			_TerminateBus();
		static	status_t		_WorkerThread(void*);

private:

		device_node* 			fNode;
		mmc_bus_interface* 		fController;
		void* 					fCookie;
		status_t				fStatus;
		thread_id				fWorkerThread;
		sem_id					fScanSemaphore;
		sem_id					fLockSemaphore;
		uint16					fActiveDevice;
		card_type				fCardType;
		// FIXME card type shall be extended to map card type to RCA.
		// In theory RCA is 16 bit allowing for 2^16 devices on signle bus
		// Practically this is limited by bus max capacitance(ver low speed)
		// which narrows down to theoritical 40 cards, imperically 10 cards only.
		// numbers qouted from untrusted sources but seems more than reasonable.
		// Thus, an array of 40 card_types where RCA is the index shall work.
};


#endif /*MMC_BUS_H*/
