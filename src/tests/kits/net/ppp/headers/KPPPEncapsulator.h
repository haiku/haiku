#ifndef _K_PPP_ENCAPSULATOR__H
#define _K_PPP_ENCAPSULATOR__H

#include "KPPPInterface.h"


class PPPEncapsulator {
	public:
		PPPEncapsulator(const char *name, PPP_ENCAPSULATION_LEVEL level,
			uint16 protocol, int32 addressFamily, uint32 overhead,
			PPPInterface *interface, driver_parameter *settings,
			int32 flags = PPP_NO_FLAGS);
		virtual ~PPPEncapsulator();
		
		virtual status_t InitCheck() const = 0;
		
		const char *Name() const
			{ return fName; }
		
		PPP_ENCAPSULATION_LEVEL Level() const
			{ return fLevel; }
		uint32 Overhead() const
			{ return fOverhead; }
		
		PPPInterface *Interface() const
			{ return fInterface; }
		driver_parameter *Settings()
			{ return fSettings; }
		
		uint16 Protocol() const
			{ return fProtocol; }
		int32 AddressFamily() const
			{ return fAddressFamily; }
				// negative values and values > 0xFF are ignored
		int32 Flags() const
			{ return fFlags; }
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		void SetUpRequested(bool requested = true);
		bool IsUpRequested() const
			{ return fUpRequested; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		void SetNext(PPPEncapsulator *next)
			{ fNext = next; }
		PPPEncapsulator *Next() const
			{ return fNext; }
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		virtual bool Reset() = 0;
			// reset to initial (down) state without sending something
		bool IsUp() const
			{ return fIsUp; }
		
		virtual status_t Send(mbuf *packet, uint16 protocol) = 0;
		virtual status_t Receive(mbuf *packet, uint16 protocol) = 0;
		
		status_t SendToNext(mbuf *packet, uint16 protocol);
			// this will send your packet to the next (up) encapsulator
			// if there is no next encapsulator (==NULL), it will
			// call the interface's SendToDevice function

	protected:
		void UpStarted();
		void DownStarted();
		
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();
			// report up/down events

	protected:
		uint32 fOverhead;

	private:
		char *fName;
		PPP_ENCAPSULATION_LEVEL fLevel;
		PPPInterface *fInterface;
		driver_parameter *fSettings;
		uint16 fProtocol;
		int32 fAddressFamily, fFlags;
		
		PPPEncapsulator *fNext;
		
		bool fEnabled;
		bool fUpRequested;
		bool fIsUp;
}:


#endif
