//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_ENCAPSULATOR__H
#define _K_PPP_ENCAPSULATOR__H

#include <driver_settings.h>

#include <KPPPDefs.h>

class PPPInterface;

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class PPPEncapsulator {
	public:
		PPPEncapsulator(const char *name, PPP_PHASE phase,
			PPP_ENCAPSULATION_LEVEL level, uint16 protocol,
			int32 addressFamily, uint32 overhead,
			PPPInterface& interface, driver_parameter *settings,
			int32 flags = PPP_NO_FLAGS);
		virtual ~PPPEncapsulator();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		PPP_PHASE Phase() const
			{ return fPhase; }
		
		PPP_ENCAPSULATION_LEVEL Level() const
			{ return fLevel; }
		uint32 Overhead() const
			{ return fOverhead; }
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
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
		
		bool IsUpRequested() const
			{ return fUpRequested; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		void SetNext(PPPEncapsulator *next)
			{ fNext = next; }
		PPPEncapsulator *Next() const
			{ return fNext; }
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		bool IsUp() const
			{ return fConnectionStatus == PPP_ESTABLISHED_PHASE; }
		bool IsDown() const
			{ return fConnectionStatus == PPP_DOWN_PHASE; }
		bool IsGoingUp() const
			{ return fConnectionStatus == PPP_ESTABLISHMENT_PHASE; }
		bool IsGoingDown() const
			{ return fConnectionStatus == PPP_TERMINATION_PHASE; }
		
		virtual status_t Send(struct mbuf *packet, uint16 protocol) = 0;
		virtual status_t Receive(struct mbuf *packet, uint16 protocol) = 0;
		
		status_t SendToNext(struct mbuf *packet, uint16 protocol) const;
			// this will send your packet to the next (up) encapsulator
			// if there is no next encapsulator (==NULL), it will
			// call the interface's SendToDevice function
		
		virtual void Pulse();

	protected:
		void SetUpRequested(bool requested = true)
			{ fUpRequested = requested; }
		
		void UpStarted();
		void DownStarted();
		
		void UpFailedEvent();
		void UpEvent();
		void DownEvent();
			// report up/down events

	protected:
		uint32 fOverhead;

	private:
		char fName[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
		PPP_PHASE fPhase;
		PPP_ENCAPSULATION_LEVEL fLevel;
		uint16 fProtocol;
		int32 fAddressFamily;
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		int32 fFlags;
		
		PPPEncapsulator *fNext;
		
		bool fEnabled;
		bool fUpRequested;
		PPP_PHASE fConnectionStatus;
};


#endif
