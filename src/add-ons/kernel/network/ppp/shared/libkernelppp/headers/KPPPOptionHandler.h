/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_OPTION_HANDLER__H
#define _K_PPP_OPTION_HANDLER__H

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

class KPPPConfigurePacket;


class KPPPOptionHandler {
	protected:
		// KPPPOptionHandler must be subclassed
		KPPPOptionHandler(const char *name, uint8 type, KPPPInterface& interface,
			driver_parameter *settings);

	public:
		virtual ~KPPPOptionHandler();
		
		virtual status_t InitCheck() const;
		
		//!	Returns the name of this handler.
		const char *Name() const
			{ return fName; }
		
		//!	Returns the LCP item type this object can handle.
		uint8 Type() const
			{ return fType; }
		
		//!	Returns the owning interface.
		KPPPInterface& Interface() const
			{ return fInterface; }
		//!	Returns the handler's settings.
		driver_parameter *Settings() const
			{ return fSettings; }
		
		//!	Enables or disables this handler.
		void SetEnabled(bool enabled = true)
			{ fEnabled = enabled; }
		//!	Returns if the handler is enabled.
		bool IsEnabled() const
			{ return fEnabled; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		virtual status_t StackControl(uint32 op, void *data);
			// called by netstack (forwarded by KPPPInterface)
		
		// we want to send a configure request or we received a reply
		virtual status_t AddToRequest(KPPPConfigurePacket& request);
		virtual status_t ParseNak(const KPPPConfigurePacket& nak);
			// create next request based on these and previous values
		virtual status_t ParseReject(const KPPPConfigurePacket& reject);
			// create next request based on these and previous values
		virtual status_t ParseAck(const KPPPConfigurePacket& ack);
			// this is called for all handlers
		
		virtual status_t ParseRequest(const KPPPConfigurePacket& request,
			int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject);
		virtual status_t SendingAck(const KPPPConfigurePacket& ack);
		
		virtual void Reset();

	protected:
		status_t fInitStatus;

	private:
		char *fName;
		uint8 fType;
		KPPPInterface& fInterface;
		driver_parameter *fSettings;
		
		bool fEnabled;
};


#endif
