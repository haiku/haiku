//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_OPTION_HANDLER__H
#define _K_PPP_OPTION_HANDLER__H

#include <driver_settings.h>

#include <KPPPDefs.h>

#ifndef _K_PPP_INTERFACE__H
#include <KPPPInterface.h>
#endif

class PPPConfigurePacket;


class PPPOptionHandler {
	public:
		PPPOptionHandler(const char *name, PPPInterface& interface,
			driver_parameter *settings);
		virtual ~PPPOptionHandler();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		
		PPPInterface& Interface() const
			{ return fInterface; }
		driver_parameter *Settings() const
			{ return fSettings; }
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		virtual status_t Control(uint32 op, void *data, size_t length);
		
		virtual void Reset() = 0;
			// e.g.: remove list of rejected values
		
		// we want to send a configure request or we received a reply
		virtual status_t AddToRequest(PPPConfigurePacket *request) = 0;
		virtual void ParseNak(const PPPConfigurePacket *nak) = 0;
			// create next request based on these and previous values
		virtual void ParseReject(const PPPConfigurePacket *reject) = 0;
			// create next request based on these and previous values
		virtual void ParseAck(const PPPConfigurePacket *ack) = 0;
			// this is called for all handlers
		
		// peer sent configure request
		virtual status_t ParseRequest(const PPPConfigurePacket *request,
			int32 item, PPPConfigurePacket *nak, PPPConfigurePacket *reject) = 0;
			// item may be behind the last item which means we can add
			// additional values
		virtual void SendingAck(const PPPConfigurePacket *ack) = 0;
			// notification that we ack these values

	private:
		char fName[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
		PPPInterface& fInterface;
		driver_parameter *fSettings;
		
		bool fEnabled;
};


#endif
