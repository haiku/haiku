#ifndef _K_PPP_PROTOCOL__H
#define _K_PPP_PROTOCOL__H

#include "KPPPInterface.h"


class PPPProtocol {
	public:
		PPPProtocol(const char *name, PPP_PHASE phase, uint16 protocol,
			int32 addressFamily, PPPInterface *interface,
			driver_parameter *settings, int32 flags = PPP_NO_FLAGS);
		virtual ~PPPProtocol();
		
		virtual status_t InitCheck() const = 0;
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		const char *Name() const
			{ return fName; }
		
		PPP_PHASE Phase() const
			{ return fPhase; }
		
		driver_parameter *Settings()
			{ return fSettings; }
		
		uint16 Protocol() const
			{ return fProtocol; }
		int32 AddressFamily() const
			{ return fAddressFamily; }
				// negative values and values > 0xFF are ignored
		int32 Flags() const
			{ return fFlags; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		virtual bool Reset() = 0;
			// reset to initial (down) state without sending something
		bool IsUp() const
			{ return fIsUp; }
		
		virtual status_t Send(mbuf *packet) = 0;
		virtual status_t Receive(mbuf *packet) = 0;

	protected:
		void SetUp(bool isUp);
			// report up/down events

	private:
		char *name;
		PPP_PHASE fPhase;
		uint16 fProtocol;
		int32 fAddressFamily, fFlags;
		PPPInterface fInterface;
		driver_parameter fSettings;
		
		bool fIsUp;
		bool fEnabled;
};


#endif
