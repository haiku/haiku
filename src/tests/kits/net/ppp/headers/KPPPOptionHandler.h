#ifndef _K_PPP_OPTION_HANDLER__H
#define _K_PPP_OPTION_HANDLER__H

#include "KPPPConfigurePacket.h"
#include "KPPPInterface.h"


class PPPOptionHandler {
	public:
		PPPOptionHandler(const char *name, PPPInterface *interface,
			driver_parameter *settings);
		virtual ~PPPOptionHandler();
		
		virtual status_t InitCheck() = 0;
		
		void SetEnabled(bool enabled = true);
		bool IsEnabled() const
			{ return fEnabled; }
		
		const char *Name() const
			{ return fName; }
		
		PPPInterface *Interface() const
			{ return fInterface; }
		driver_parameter *Settings()
			{ return fSettings; }
		
		virtual void Reset() = 0;
			// e.g.: remove list of rejected values
		
		// we want to send a configure request or we received a reply
		virtual bool AddToRequest(PPPConfigurePacket *request) = 0;
		virtual void ParseNak(const PPPConfigurePacket *nak) = 0;
			// create next request based on these and previous values
		virtual void ParseReject(const PPPConfigurePacket *reject) = 0;
			// create next request based on these and previous values
		virtual void ParseAck(const PPPConfigurePacket *ack) = 0;
			// this is called for all handlers
		
		// peer sent configure request
		virtual bool ParseConfigureRequest(const PPPConfigurePacket *request,
			int32 item, PPPConfigurePacket *nak, PPPConfigurePacket *reject) = 0;
			// item may be behind the last item which means we can add
			// additional values

	private:
		const char *fName;
		PPPInterface *fInterface;
		driver_parameters *fSettings;
		
		bool fEnabled;
};


#endif
