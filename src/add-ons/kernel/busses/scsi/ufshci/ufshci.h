/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha98@gmail.com>
 */

#ifndef _UFSHCI_H
#define _UFSHCI_H

#include "ufshci_spec.h"

#include <atomic>

#include "KernelExport.h"
#include "OS.h"

#include "bus/PCI.h"
#include "bus/SCSI.h"
#include "util/SinglyLinkedList.h"

// #define TRACE_UFSHCI
#ifdef TRACE_UFSHCI
#define TRACE(x...) dprintf("UFSHCI::" x)
#else
#define TRACE(x...)
#endif

#define CALLED() TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define UFSHCI_DRIVER_MODULE_NAME "busses/scsi/ufshci/driver_v1"
#define UFSHCI_SIM_MODULE_NAME "busses/scsi/ufshci/sim/driver_v1"
#define UFSHCI_DEVICE_ID_GENERATOR "ufs/device_id"

/* UFSHCI PCI Vendor Id */
#define UFS_PCI_VENDOR_ID_REDHAT 0x1b36
#define UFS_PCI_VENDOR_ID_SAMSUNG 0x144d
#define UFS_PCI_VENDOR_ID_AMD 0x1022

typedef struct KernelBufferInfo {
	physical_entry phys_entry;
	void* virtual_address;
	area_id area;
} KernelBufferInfo;


typedef struct UfsSlot : SinglyLinkedListLinkImpl<UfsSlot> {
	struct UtpTransferRequestDescriptor* descriptor;
	struct UtpCommandDescriptor* ucd;
	sem_id sem;
	uint8 slotIndex;
} UfsSlot;

typedef SinglyLinkedList<UfsSlot> UfsSlotList;

class UfsHciController {
public:
								UfsHciController(pci_device_module_info* pci, pci_device* pci_dev,
									struct pci_info* pci_device_info);

								~UfsHciController();
				status_t 		Init();
				UfsHciRegisters* Registers() const {return fRegisters;}

				void 			AcquireFreeQueueLock() const { acquire_sem(fFreeQueueSem); }
				void 			ReleaseFreeQueueLock() const { release_sem(fFreeQueueSem); }
				sem_id 			CompletionSem() const {return fDeviceCommandCompletionSem;}

				status_t 		HandleUtrlInterrupt();
				void			ExecuteRequest(scsi_ccb* request);
				scsi_bus		GetBus() const { return fBus; }
				void			SetBus(scsi_bus bus) { fBus = bus; }
				uchar			PathInquiry(scsi_path_inquiry* info);
				void			GetRestrictions(bool* isATAPI, bool* noAutoSense, uint32* maxBlocks);


private:
			UfsHciRegisters* 	fRegisters;
			area_id 			fRegisterArea;
			pci_device_module_info* fPci;
			pci_device* 		fPciDev;
			struct pci_info* 	fPciDeviceInfo;
			scsi_bus			fBus;

			KernelBufferInfo* 	fUtrlBuffer;
			KernelBufferInfo* 	fUtmrlBuffer;
			KernelBufferInfo* 	fUcdBuffer;

			UfsSlotList 		fFreeSlotsQueue;
			UfsSlotList 		fActiveSlotsQueue;

			sem_id 				fFreeQueueSem;
			sem_id 				fDeviceCommandCompletionSem;
			sem_id 				fFreeSlotsSem;
			sem_id 				fFreeBuffersSem;
			spinlock 			fSlotLock;

			int32 				fIRQ;
			uint8 				fUtrlBufferEntries;
			uint8 				fUtmrlBufferEntries;
			std::atomic<uint8> 	fTaskTag;



private:
			status_t 			_InitBus();
			status_t 			_AllocateBuffers(uint32 physical_alignment, size_t size,
									const char* name, KernelBufferInfo* info);

			UfsSlot*			_AcquireFreeSlot();

			UfsSlot*			_SubmitUtpTransferRequest(scsi_ccb* request);

			status_t			_VerifyLinkStartup();

			void				_SubmitRequest(scsi_ccb* request);

			void				_ProcessCompletedUtpRequest(UfsSlot* ufsSlot, scsi_ccb* request);
			void 				_ReleaseSlot(UfsSlot* ufsSlot);
			void				_FreeSlotsQueue(UfsSlotList& queue);

	static	int32				_Interrupt(void* data);
};

#endif // !_UFSHCI_H
