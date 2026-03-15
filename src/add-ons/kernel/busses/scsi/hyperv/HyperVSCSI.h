/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_SCSI_H_
#define _HYPERV_SCSI_H_


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>
#include <AutoDeleterDrivers.h>
#include <bus/SCSI.h>
#include <condition_variable.h>
#include <lock.h>
#include <scsi_cmds.h>
#include <util/AutoLock.h>

#include <hyperv.h>

#include "HyperVSCSIProtocol.h"
#include "HyperVSCSIRequest.h"


//#define TRACE_HYPERV_SCSI
#ifdef TRACE_HYPERV_SCSI
#	define TRACE(x...) dprintf("\33[94mhyperv_scsi:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_scsi:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_scsi:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define HYPERV_SCSI_ID_GENERATOR		"hyperv_scsi/id"
#define HYPERV_SCSI_ID_ITEM				"hyperv_scsi/id"
#define HYPERV_SCSI_DRIVER_MODULE_NAME	"busses/scsi/hyperv_scsi/driver_v1"
#define HYPERV_SCSI_SIM_MODULE_NAME		"busses/scsi/hyperv_scsi/sim/driver_v1"
#define HYPERV_SCSI_SIM_PRETTY_NAME		"Hyper-V SCSI Interface"


extern device_manager_info* gDeviceManager;
extern scsi_for_sim_interface* gSCSI;


class HyperVSCSI {
public:
									HyperVSCSI(device_node* node);
									~HyperVSCSI();
			status_t				InitCheck() const { return fStatus; }
			scsi_bus				GetBus() const { return fBus; }
			void					SetBus(scsi_bus bus) { fBus = bus; }

			status_t				StartIO(scsi_ccb* ccb);
			uchar					PathInquiry(scsi_path_inquiry* inquiry_data);
			uchar					ResetBus();
			uchar					ResetDevice(uchar targetID, uchar targetLUN);

private:
			bool					_IsLegacy() const { return fVersion < HV_SCSI_VERSION_WIN8; }
			uint8					_GetSenseLength() const;
			uint32					_GetMessageLengthDelta() const;
	static	void					_CallbackHandler(void* data);
			void					_Callback();
	static	void					_RescanDPCHandler(void* data);
			void					_RescanBus();
			status_t				_SendRequest(HyperVSCSIRequest* request, bool wait);
			HyperVSCSIRequest*		_GetRequest();
			void					_ReturnRequest(HyperVSCSIRequest* request);
			void					_CompleteIO(uint64 transactionID, hv_scsi_msg* message);
			status_t				_BeginInit();
			status_t				_NegotiateProtocol();
			status_t				_QueryProperties();
			status_t				_EndInit();

private:
			device_node*			fNode;
			scsi_bus				fBus;
			scsi_dpc_cookie			fBusDPC;
			status_t				fStatus;

			bool					fIsIDE;
			uint8					fTargetID;
			uint8					fMaxDevices;
			uint16					fVersion;
			uint16					fMaxSubChannels;
			uint32					fMaxTransferBytes;
			uint8*					fPacket;

			HyperVSCSIRequestList	fFreeRequests;
			HyperVSCSIRequestList	fActiveRequests;
			mutex					fRequestLock;

			hyperv_device_interface*	fHyperV;
			hyperv_device				fHyperVCookie;
};


#endif
