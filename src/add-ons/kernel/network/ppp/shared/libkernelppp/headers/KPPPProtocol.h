//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_PROTOCOL__H
#define _K_PPP_PROTOCOL__H

#include <KPPPDefs.h>
#include <KPPPLayer.h>

class PPPInterface;
class PPPOptionHandler;


class PPPProtocol : public PPPLayer {
	protected:
		// PPPProtocol must be subclassed
		PPPProtocol(const char *name, ppp_phase activationPhase,
			uint16 protocolNumber, ppp_level level, int32 addressFamily,
			uint32 overhead, PPPInterface& interface,
			driver_parameter *settings, int32 flags = PPP_NO_FLAGS,
			const char *type = NULL, PPPOptionHandler *optionHandler = NULL);

	public:
		virtual ~PPPProtocol();
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		ppp_phase ActivationPhase() const
			{ return fActivationPhase; }
		
		uint32 Overhead() const
			{ return fOverhead; }
				// only useful for encapsulation protocols
		
		uint16 ProtocolNumber() const
			{ return fProtocolNumber; }
		int32 AddressFamily() const
			{ return fAddressFamily; }
				// negative values and values > 0xFF are ignored
		int32 Flags() const
			{ return fFlags; }
		ppp_side Side() const
			{ return fSide; }
				// which side this protocol works for
		
		const char *Type() const
			{ return fType; }
		PPPOptionHandler *OptionHandler() const
			{ return fOptionHandler; }
		
		void SetNextProtocol(PPPProtocol *protocol)
			{ fNextProtocol = protocol; SetNext(protocol); }
		PPPProtocol *NextProtocol() const
			{ return fNextProtocol; }
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		bool IsUpRequested() const
			{ return fUpRequested; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by PPPInterface)
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
			// if DialOnDemand is supported check for DialOnDemand settings change
		bool IsUp() const
			{ return fConnectionPhase == PPP_ESTABLISHED_PHASE; }
		bool IsDown() const
			{ return fConnectionPhase == PPP_DOWN_PHASE; }
		bool IsGoingUp() const
			{ return fConnectionPhase == PPP_ESTABLISHMENT_PHASE; }
		bool IsGoingDown() const
			{ return fConnectionPhase == PPP_TERMINATION_PHASE; }
		
		virtual bool IsAllowedToSend() const;
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber) = 0;

	protected:
		void SetUpRequested(bool requested = true)
			{ fUpRequested = requested; }
		
		// Report that we are going up/down
		// (from now on, the Up() process can be aborted).
		void UpStarted();
		void DownStarted();
		
		// report up/down events
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	protected:
		uint32 fOverhead;
		ppp_side fSide;

	private:
		ppp_phase fActivationPhase;
		uint16 fProtocolNumber;
		int32 fAddressFamily;
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		int32 fFlags;
		char *fType;
		PPPOptionHandler *fOptionHandler;
		
		PPPProtocol *fNextProtocol;
		bool fEnabled;
		bool fUpRequested;
		ppp_phase fConnectionPhase;
};


#endif
