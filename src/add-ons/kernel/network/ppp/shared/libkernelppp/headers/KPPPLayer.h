//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_LAYER__H
#define _K_PPP_LAYER__H

#include <KPPPDefs.h>


class PPPLayer {
	protected:
		// PPPLayer must be subclassed
		PPPLayer(const char *name, ppp_level level);

	public:
		virtual ~PPPLayer();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		ppp_level Level() const
			{ return fLevel; }
				// should be PPP_PROTOCOL_LEVEL if not encapsulator
		
		void SetNext(PPPLayer *next)
			{ fNext = next; }
		PPPLayer *Next() const
			{ return fNext; }
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		
		virtual bool IsAllowedToSend() const = 0;
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber) = 0;
		
		status_t SendToNext(struct mbuf *packet, uint16 protocolNumber) const;
			// send the packet to the next layer
		
		virtual void Pulse();

	protected:
		status_t fInitStatus;

	private:
		char *fName;
		ppp_level fLevel;
		
		PPPLayer *fNext;
};


#endif
