//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef __K_PPP_LCP_EXTENSION__H
#define __K_PPP_LCP_EXTENSION__H

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class PPPLCPExtension {
	public:
		PPPLCPExtension(const char *name, uint8 code, PPPInterface& interface,
			driver_parameter *settings);
		virtual ~PPPLCPExtension();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		void SetEnabled(bool enabled = true)
			{ fEnabled = enabled; }
		bool IsEnabled() const
			{ return fEnabled; }
		
		uint8 Code() const
			{ return fCode; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual void Reset();
		
		virtual status_t Receive(struct mbuf *packet, uint8 code) = 0;
		
		virtual void Pulse();

	private:
		char fName[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		uint8 fCode;
		
		bool fEnabled;
		status_t fInitStatus;
};


#endif
