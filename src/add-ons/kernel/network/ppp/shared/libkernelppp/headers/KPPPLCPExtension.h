/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

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
		
		//!	Returns the name of this LCP extension.
		const char *Name() const
			{ return fName; }
		
		//!	Returns the owning interface.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the LCP extension's settings.
		driver_parameter *Settings() const
			{ return fSettings; }
		
		//!	Enables or disables this extension.
		void SetEnabled(bool enabled = true)
			{ fEnabled = enabled; }
		//!	Returns if the extension is enabled.
		bool IsEnabled() const
			{ return fEnabled; }
		
		//!	Returns the LCP packet code this extension can handle.
		uint8 Code() const
			{ return fCode; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by KPPPInterface)
		
		//!	Must be overridden. Called when an LCP packet with your code is received.
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
