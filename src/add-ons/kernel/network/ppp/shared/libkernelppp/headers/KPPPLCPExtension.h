//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef __K_PPP_LCP_EXTENSION__H
#define __K_PPP_LCP_EXTENSION__H

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif


class KPPPLCPExtension {
	protected:
		// KPPPLCPExtension must be subclassed
		KPPPLCPExtension(const char *name, uint8 code, KPPPInterface& interface,
			driver_parameter *settings);

	public:
		virtual ~KPPPLCPExtension();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		KPPPInterface& Interface() const
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
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by KPPPInterface)
		
		virtual void ProfileChanged();
		
		virtual status_t Receive(struct mbuf *packet, uint8 code) = 0;
		
		virtual void Reset();
		virtual void Pulse();

	protected:
		status_t fInitStatus;

	private:
		char *fName;
		KPPPInterface& fInterface;
		driver_parameter *fSettings;
		uint8 fCode;
		
		bool fEnabled;
};


#endif
