#ifndef _K_PPP_DEVICE__H
#define _K_PPP_DEVICE__H

#include "KPPPInterface.h"


class PPPDevice {
	public:
		PPPDevice(const char *fName, uint32 overhead, PPPInterface *interface, driver_parameter *settings);
		virtual ~PPPDevice();
		
		virtual status_t InitCheck() const = 0;
		
		const char *Name() const
			{ return fName; }
		
		uint32 Overhead() const
			{ return fOverhead; }
		
		PPPInterface *Interface() const
			{ return fInterface; }
		driver_parameter *Settings()
			{ return fSettings; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual bool SetMTU(uint32 mtu) = 0;
		uint32 MTU() const
			{ return fMTU; }
		virtual uint32 PreferredMTU() const = 0;
		
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
		
		virtual status_t Send(mbuf *packet) = 0;
			// this should enqueue the packet and return immediately
		status_t PassToInterface(mbuf *packet);
			// This will pass the packet to the interface's queue.
			// Do not call Interface::ReceiveFromDevice directly
			// if this can block a Send()!

	protected:
		// Report that we are going up/down
		// (from now on, the Up() process can be aborted).
		// Abort if false is returned!
		bool UpStarted();
		bool DownStarted();
		
		// report up/down events
		void UpEvent();
		void UpFailedEvent();
		void DownEvent();

	protected:
		bool fIsUp;

	private:
		char *fName;
		uint32 fOverhead;
		PPPInterface *fInterface;
		driver_parameter *fSettings;
		
		uint32 fMTU;
};


#endif
