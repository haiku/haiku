//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef PROTOCOL__H
#define PROTOCOL__H

#include <driver_settings.h>

#include <KPPPOptionHandler.h>
#include <KPPPProtocol.h>
#include <Locker.h>

#include <arpa/inet.h>


#define PAP_PROTOCOL	0xC023


enum pap_state {
	INITIAL,
	REQ_SENT,
	WAITING_FOR_REQ,
	ACCEPTED
};


class PAP;

class PAPHandler : public KPPPOptionHandler {
	public:
		PAPHandler(PAP& owner, KPPPInterface& interface);
		
		PAP& Owner() const
			{ return fOwner; }
		
		virtual status_t AddToRequest(KPPPConfigurePacket& request);
		virtual status_t ParseRequest(const KPPPConfigurePacket& request,
			int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject);

	private:
		PAP& fOwner;
};


class PAP : public KPPPProtocol {
	public:
		PAP(KPPPInterface& interface, driver_parameter *settings);
		virtual ~PAP();
		
		virtual status_t InitCheck() const;
		
		pap_state State() const
			{ return fState; }
		
		virtual void ProfileChanged();
		
		virtual bool Up();
		virtual bool Down();
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber);
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber);
		virtual void Pulse();

	private:
		bool ParseSettings(const driver_parameter *settings);
		
		// for state machine
		void NewState(pap_state next);
		uint8 NextID();
			// return the next id for PAP packets
		
		// events
		void TOGoodEvent();
		void TOBadEvent();
		void RREvent(struct mbuf *packet);
		void RAEvent(struct mbuf *packet);
		void RNEvent(struct mbuf *packet);
		
		// actions
		void ReportUpFailedEvent();
		void ReportUpEvent();
		void ReportDownEvent();
		void InitializeRestartCount();
		bool SendRequest();
		bool SendAck(struct mbuf *packet);
		bool SendNak(struct mbuf *packet);

	private:
		char fUser[256], fPassword[256];
		
		// for state machine
		pap_state fState;
		vint32 fID;
		
		// counters and timers
		int32 fMaxRequest;
		int32 fRequestCounter;
		uint8 fRequestID;
			// the ID we used for the last configure/terminate request
		bigtime_t fNextTimeout;
		
		BLocker fLock;
};


#endif
