//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include <KPPPDefs.h>
#include <KPPPLayer.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class KPPPDevice : public KPPPLayer {
		friend class KPPPStateMachine;

	protected:
		// KPPPDevice must be subclassed
		KPPPDevice(const char *name, uint32 overhead, KPPPInterface& interface,
			driver_parameter *settings);

	public:
		virtual ~KPPPDevice();
		
		KPPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		uint32 MTU() const
			{ return fMTU; }
		
		virtual bool Up() = 0;
			// In server mode you should listen for incoming connections.
			// On error: either call UpFailedEvent() or return false.
		virtual bool Down() = 0;
		bool IsUp() const
			{ return fConnectionPhase == PPP_ESTABLISHED_PHASE; }
		bool IsDown() const
			{ return fConnectionPhase == PPP_DOWN_PHASE; }
		bool IsGoingUp() const
			{ return fConnectionPhase == PPP_ESTABLISHMENT_PHASE; }
		bool IsGoingDown() const
			{ return fConnectionPhase == PPP_TERMINATION_PHASE; }
		
		// The biggest of the two tranfer rates will be set in ifnet.
		// These methods should return default values when disconnected.
		virtual uint32 InputTransferRate() const = 0;
		virtual uint32 OutputTransferRate() const = 0;
		
		virtual uint32 CountOutputBytes() const = 0;
			// how many bytes are waiting to be sent?
		
		virtual bool IsAllowedToSend() const;
			// always returns true
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
			// This should enqueue the packet and return immediately (never block).
			// The device is responsible for freeing the packet.
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber);
			// this method is never used
		
		virtual void Pulse();

	protected:
		void SetMTU(uint32 MTU)
			{ fMTU = MTU; }
		
		// Report that we are going up/down
		// (from now on, the Up() process can be aborted).
		// Abort if false is returned!
		bool UpStarted();
		bool DownStarted();
		
		// report up/down events
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	protected:
		uint32 fMTU;
			// always hold this value up-to-date!

	private:
		char *fName;
		KPPPInterface& fInterface;
		driver_parameter *fSettings;
		
		ppp_phase fConnectionPhase;
};


#endif
