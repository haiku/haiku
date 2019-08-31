/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_SCSI_PRIVATE_H
#define VIRTIO_SCSI_PRIVATE_H


#include <bus/SCSI.h>
#include <condition_variable.h>
#include <lock.h>
#include <scsi_cmds.h>
#include <virtio.h>

#include "virtio_scsi.h"


//#define TRACE_VIRTIO_SCSI
#ifdef TRACE_VIRTIO_SCSI
#	define TRACE(x...) dprintf("virtio_scsi: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_scsi:\33[0m " x)
#define CALLED()			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

extern device_manager_info* gDeviceManager;
extern scsi_for_sim_interface *gSCSI;

bool copy_sg_data(scsi_ccb *ccb, uint offset, uint allocationLength,
	void *buffer, int size, bool toBuffer);
void swap_words(void *data, size_t size);


#define VIRTIO_SCSI_STANDARD_TIMEOUT		10 * 1000 * 1000
#define VIRTIO_SCSI_INITIATOR_ID	7

#define VIRTIO_SCSI_NUM_EVENTS		4


class VirtioSCSIRequest;


class VirtioSCSIController {
public:
								VirtioSCSIController(device_node* node);
								~VirtioSCSIController();

			status_t			InitCheck();

			void				SetBus(scsi_bus bus);
			scsi_bus			Bus() const { return fBus; }

			void				PathInquiry(scsi_path_inquiry* info);
			void				GetRestrictions(uint8 targetID, bool* isATAPI,
									bool* noAutoSense, uint32* maxBlocks);
			uchar				ResetDevice(uchar targetID, uchar targetLUN);
			status_t			ExecuteRequest(scsi_ccb* request);
			uchar				AbortRequest(scsi_ccb* request);
			uchar				TerminateRequest(scsi_ccb* request);
			status_t			Control(uint8 targetID, uint32 op,
									void* buffer, size_t length);

private:
	static	void				_RequestCallback(void* driverCookie,
									void *cookie);
			void				_RequestInterrupt();
	static	void				_EventCallback(void *driverCookie,
									void *cookie);
			void				_EventInterrupt(
									struct virtio_scsi_event* event);
	static	void				_RescanChildBus(void *cookie);

			void				_SubmitEvent(uint32 event);

			device_node*		fNode;
			scsi_bus			fBus;

			virtio_device_interface* fVirtio;
			virtio_device*		fVirtioDevice;

			status_t			fStatus;
			struct virtio_scsi_config	fConfig;
			uint32				fFeatures;
			::virtio_queue		fControlVirtioQueue;
			::virtio_queue		fEventVirtioQueue;
			::virtio_queue		fRequestVirtioQueue;

			area_id				fArea;
			struct virtio_scsi_event*	fEvents;

			VirtioSCSIRequest*	fRequest;

			int32				fCurrentRequest;
			ConditionVariable	fInterruptCondition;
			ConditionVariableEntry fInterruptConditionEntry;

			scsi_dpc_cookie		fEventDPC;
			struct virtio_scsi_event fEventBuffers[VIRTIO_SCSI_NUM_EVENTS];
};


class VirtioSCSIRequest {
public:
								VirtioSCSIRequest(bool hasLock);
								~VirtioSCSIRequest();

			void				SetStatus(uint8 status);
			uint8				Status() const { return fStatus; }

			void				SetTimeout(bigtime_t timeout);
			bigtime_t			Timeout() const { return fTimeout; }

			bool				HasSense() {
									return (fResponse->sense_len > 0); }

			void				SetIsWrite(bool isWrite);
			bool				IsWrite() const { return fIsWrite; }

			void				SetBytesLeft(uint32 bytesLeft);
			size_t*				BytesLeft() { return &fBytesLeft; }

			bool				HasData() const
									{ return fCCB->data_length > 0; }

			status_t			Finish(bool resubmit);
			void				Abort();

			// SCSI stuff
			status_t			Start(scsi_ccb *ccb);
			scsi_ccb*			CCB() { return fCCB; }

			void				RequestSense();

			void				FillRequest(uint32 inCount, uint32 outCount,
									physical_entry *entries);

private:
			void				_FillSense(scsi_sense *sense);
			uchar				_ResponseStatus();

			mutex				fLock;
			bool				fHasLock;

			uint8				fStatus;

			bigtime_t			fTimeout;
			size_t				fBytesLeft;
			bool				fIsWrite;
			scsi_ccb*			fCCB;

			// virtio scsi
			void*				fBuffer;
			struct virtio_scsi_cmd_req *fRequest;
			struct virtio_scsi_cmd_resp *fResponse;
};


#endif // VIRTIO_SCSI_PRIVATE_H

