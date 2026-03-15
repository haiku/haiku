/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_IC_BASE_H_
#define _HYPERV_IC_BASE_H_


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hyperv.h>
#include <hyperv_spec.h>

#include "ICDriver.h"
#include "ICProtocol.h"


class ICBase {
public:
								ICBase(device_node* node, uint32 packetLength, uint16 messageType,
									const uint32* messageVersions, uint32 messageVersionCount);
	virtual						~ICBase();
			status_t			InitCheck() const { return fStatus; }

protected:
			status_t			Connect(uint32 txLength, uint32 rxLength);
			void				Disconnect();

	virtual void				OnProtocolNegotiated();
	virtual void				OnMessageReceived(hv_ic_msg* icMessage) = 0;
	virtual void				OnMessageSent(hv_ic_msg* icMessage);

protected:
			status_t			fStatus;
			device_node*		fNode;
			uint32				fFrameworkVersion;
			uint32				fMessageVersion;

			hyperv_device_interface*	fHyperV;
			hyperv_device				fHyperVCookie;

private:
			status_t			_NegotiateProtocol(hv_ic_msg_negotiate* message);
	static	void				_CallbackHandler(void* arg);
			void				_Callback();

private:
			uint8*				fPacket;
			uint32				fPacketLength;
			uint16				fMessageType;
			const uint32*		fMessageVersions;
			uint32				fMessageVersionCount;
};


#endif // _HYPERV_IC_BASE_H_
