/*
 * Copyright 2003-2006, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPPoE_DEVICE__H
#define PPPoE_DEVICE__H

#include "PPPoE.h"

#include <KPPPDevice.h>


enum pppoe_state {
	INITIAL,
		// the same as IsDown() == true
	PADI_SENT,
	PADR_SENT,
	OPENED
		// the same as IsUp() == true
};


class PPPoEDevice : public KPPPDevice {
	public:
		PPPoEDevice(KPPPInterface& interface, driver_parameter *settings);
		
		ifnet *EthernetIfnet() const
			{ return fEthernetIfnet; }
		
		const uint8 *Peer() const
			{ return fPeer; }
		uint16 SessionID() const
			{ return fSessionID; }
		uint32 HostUniq() const
			{ return fHostUniq; }
		
		const char *ACName() const
			{ return fACName; }
		const char *ServiceName() const
			{ return fServiceName; }
		
		virtual status_t InitCheck() const;
		
		virtual bool Up();
		virtual bool Down();
		
		virtual uint32 InputTransferRate() const;
		virtual uint32 OutputTransferRate() const;
		
		virtual uint32 CountOutputBytes() const;
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber = 0);
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber = 0);
		
		virtual void Pulse();

	private:
		ifnet *fEthernetIfnet;
		uint8 fPeer[6];
		uint16 fSessionID;
		uint32 fHostUniq;
		const char *fACName, *fServiceName;
		
		uint32 fAttempts;
		bigtime_t fNextTimeout;
		pppoe_state fState;
};


#endif
