//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class PPPDevice {
	public:
		PPPDevice(const char *name, PPPInterface& interface,
			driver_parameter *settings);
		virtual ~PPPDevice();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		uint32 MTU() const
			{ return fMTU; }
		
		// these calls must not block
		virtual void Up() = 0;
		virtual void Down() = 0;
		virtual void Listen() = 0;
		bool IsUp() const
			{ return fIsUp; }
		
		// The biggest of the two tranfer rates will be set in ifnet.
		// These methods should return default values when disconnected.
		virtual uint32 InputTransferRate() const = 0;
		virtual uint32 OutputTransferRate() const = 0;
		
		virtual uint32 CountOutputBytes() const = 0;
			// how many bytes are waiting to be sent?
		
		virtual status_t Send(struct mbuf *packet) = 0;
			// This should enqueue the packet and return immediately.
			// The device is responsible for freeing the packet.
		status_t PassToInterface(struct mbuf *packet);
			// This will pass the packet to the interface's queue.
			// Do not call Interface::ReceiveFromDevice directly
			// if this can block a Send()!
		
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
		bool fIsUp;
		status_t fInitStatus;

	private:
		char *fName;
		PPPInterface& fInterface;
		driver_parameter *fSettings;
};


#endif
