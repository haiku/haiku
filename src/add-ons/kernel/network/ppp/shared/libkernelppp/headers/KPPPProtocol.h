/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_PROTOCOL__H
#define _K_PPP_PROTOCOL__H

#include <KPPPDefs.h>
#include <KPPPLayer.h>

class KPPPInterface;
class KPPPOptionHandler;


class KPPPProtocol : public KPPPLayer {
	protected:
		// KPPPProtocol must be subclassed
		KPPPProtocol(const char *name, ppp_phase activationPhase,
			uint16 protocolNumber, ppp_level level, int32 addressFamily,
			uint32 overhead, KPPPInterface& interface,
			driver_parameter *settings, int32 flags = PPP_NO_FLAGS,
			const char *type = NULL, KPPPOptionHandler *optionHandler = NULL);

	public:
		virtual ~KPPPProtocol();
		
		virtual void Uninit();
		
		//!	Returns the interface that owns this protocol.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the protocol's settings.
		driver_parameter *Settings() const
			{ return fSettings; }
		
		//!	The activation phase is the phase when Up() should be called.
		ppp_phase ActivationPhase() const
			{ return fActivationPhase; }
		
		//!	The protocol number.
		uint16 ProtocolNumber() const
			{ return fProtocolNumber; }
		//!	The address family. Negative values and values > 0xFF are ignored.
		int32 AddressFamily() const
			{ return fAddressFamily; }
		//!	This protocol's flags.
		int32 Flags() const
			{ return fFlags; }
		//!	Which side this protocol works for.
		ppp_side Side() const
			{ return fSide; }
				
		//!	Protocol type. May be NULL.
		const char *Type() const
			{ return fType; }
		//!	The (optional) KPPPOptionHandler object of this protocol.
		KPPPOptionHandler *OptionHandler() const
			{ return fOptionHandler; }
		
		//!	Sets the next protocol in the list.
		void SetNextProtocol(KPPPProtocol *protocol)
			{ fNextProtocol = protocol; SetNext(protocol); }
		//!	Returns the next protocol in the list.
		KPPPProtocol *NextProtocol() const
			{ return fNextProtocol; }
		
		void SetEnabled(bool enabled = true);
		//!	Returns whether this protocol is enabled.
		bool IsEnabled() const
			{ return fEnabled; }
		
		//!	Returns whether an Up() is requested.
		bool IsUpRequested() const
			{ return fUpRequested; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by KPPPInterface)
		
		/*!	\brief Bring this protocol up.
			
			You must call \c UpStarted() from here.
		*/
		virtual bool Up() = 0;
		/*!	\brief Bring this protocol down.
			
			You must call DownStarted() from here. \n
			If ConnectOnDemand is supported check for ConnectOnDemand settings change.
		*/
		virtual bool Down() = 0;
		//!	Is this protocol up?
		bool IsUp() const
			{ return fConnectionPhase == PPP_ESTABLISHED_PHASE; }
		//!	Is this protocol down?
		bool IsDown() const
			{ return fConnectionPhase == PPP_DOWN_PHASE; }
		//!	Is this protocol going up?
		bool IsGoingUp() const
			{ return fConnectionPhase == PPP_ESTABLISHMENT_PHASE; }
		//!	Is this protocol going down?
		bool IsGoingDown() const
			{ return fConnectionPhase == PPP_TERMINATION_PHASE; }
		
		virtual bool IsAllowedToSend() const;
		
		//!	Encapsulate the packet with the given protocol number.
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		//!	Receive a packet with the given protocol number.
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber) = 0;

	protected:
		//!	\brief Requests Up() to be called.
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
		ppp_side fSide;

	private:
		ppp_phase fActivationPhase;
		uint16 fProtocolNumber;
		int32 fAddressFamily;
		KPPPInterface& fInterface;
		driver_parameter *fSettings;
		int32 fFlags;
		char *fType;
		KPPPOptionHandler *fOptionHandler;
		
		KPPPProtocol *fNextProtocol;
		bool fEnabled;
		bool fUpRequested;
		ppp_phase fConnectionPhase;
};


#endif
