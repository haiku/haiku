//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

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
		virtual ~PPPoEDevice();
		
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
		
		BLocker fLock;
};


#endif
