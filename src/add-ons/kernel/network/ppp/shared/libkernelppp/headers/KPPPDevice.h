//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include <KPPPDefs.h>
#include <KPPPLayer.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class PPPDevice : public PPPLayer {
	protected:
		// PPPDevice must be subclassed
		PPPDevice(const char *name, PPPInterface& interface,
			driver_parameter *settings);

	public:
		virtual ~PPPDevice();
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		uint32 MTU() const
			{ return fMTU; }
		
		// these calls must not block
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		virtual bool Listen() = 0;
		bool IsUp() const
			{ return fIsUp; }
		
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
		bool UpStarted() const;
		bool DownStarted() const;
		
		// report up/down events
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();

	protected:
		uint32 fMTU;
			// always hold this value up-to-date!

	private:
		char *fName;
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		
		bool fIsUp;
};


#endif
