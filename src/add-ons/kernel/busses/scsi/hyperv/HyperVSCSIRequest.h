/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_SCSI_REQUEST_H_
#define _HYPERV_SCSI_REQUEST_H_


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bus/SCSI.h>
#include <condition_variable.h>
#include <lock.h>
#include <scsi_cmds.h>
#include <util/DoublyLinkedList.h>

#include "HyperVSCSIProtocol.h"


class HyperVSCSIRequest : public DoublyLinkedListLinkImpl<HyperVSCSIRequest> {
public:
									HyperVSCSIRequest();
									~HyperVSCSIRequest();
			status_t				InitCheck() const { return fStatus; }

			void					Reset();
			status_t				PrepareData();
			void					Complete(uint8 status);

			bigtime_t				GetTimeout() const { return fTimeout; }
			void					SetTimeout(bigtime_t timeout) { fTimeout = timeout; }

			void					SetCCB(scsi_ccb* ccb) { fCCB = ccb; }
			scsi_ccb*				GetCCB() { return fCCB; }
			hv_scsi_msg*			GetMessage() { return &fMessage; }
			uint32					GetMessageStatus() { return fMessage.header.status; }
			void					SetMessageType(uint32 type) { fMessage.header.type = type; }
			void					SetMessageData(hv_scsi_msg* message);

			vmbus_gpa_range*		GetGPARange() { return fGPARange; }
			uint32					GetGPARangeLength() { return fGPARangeLength; }

			void					AddWaiter(ConditionVariableEntry* entry);
			void					Notify();

private:
			status_t				_FillGPARange(const physical_entry* sgList, uint32 sgCount,
										uint32 dataLength);

private:
			mutex					fLock;
			status_t				fStatus;
			bigtime_t				fTimeout;
			scsi_ccb*				fCCB;

			area_id					fBounceArea;
			void*					fBounceBuffer;
			phys_addr_t				fBounceBufferPhys;
			bool					fBounceBufferInUse;

			vmbus_gpa_range*		fGPARange;
			uint32					fGPARangeLength;
			hv_scsi_msg				fMessage;
			ConditionVariable		fConditionVariable;
};
typedef DoublyLinkedList<HyperVSCSIRequest> HyperVSCSIRequestList;


#endif
