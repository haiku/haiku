//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_PROTOCOL__H
#define _K_PPP_PROTOCOL__H

#include "KPPPInterface.h"


class PPPProtocol {
	public:
		PPPProtocol(const char *name, PPP_PHASE phase, uint16 protocol,
			int32 addressFamily, PPPInterface *interface,
			driver_parameter *settings, int32 flags = PPP_NO_FLAGS);
		virtual ~PPPProtocol();
		
		virtual status_t InitCheck() const;
		
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
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		void SetUpRequested(bool requested = true)
			{ fUpRequested = requested; }
		bool IsUpRequested() const
			{ return fUpRequested; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		bool IsUp() const
			{ return fConnectionStatus == PPP_ESTABLISHED_PHASE; }
		bool IsDown const
			{ return fConectionStatus == PPP_DOWN_PHASE; }
		bool IsGoingUp() const
			{ return fConnectionStatus == PPP_ESTABLISHMENT_PHASE; }
		bool IsGoingDown() const
			{ return fConnectionStatus == PPP_TERMINATION_PHASE; }
		
		virtual status_t Send(mbuf *packet) = 0;
		virtual status_t Receive(mbuf *packet, uint16 protocol) = 0;
		
		virtual void Pulse();

	protected:
		void UpStarted();
		void DownStarted();
		
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();
			// report up/down events

	private:
		char *name;
		PPP_PHASE fPhase;
		uint16 fProtocol;
		int32 fAddressFamily;
		PPPInterface fInterface;
		driver_parameter fSettings;
		int32 fFlags;
		
		bool fEnabled;
		bool fUpRequested;
		PPP_PHASE fConnectionStatus;
};


#endif
