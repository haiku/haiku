//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_LAYER__H
#define _K_PPP_LAYER__H

#include <KPPPDefs.h>


class KPPPLayer {
	protected:
		// KPPPLayer must be subclassed
		KPPPLayer(const char *name, ppp_level level, uint32 overhead);

	public:
		virtual ~KPPPLayer();
		
		virtual status_t InitCheck() const;
		
		const char *Name() const
			{ return fName; }
		ppp_level Level() const
			{ return fLevel; }
				// should be PPP_PROTOCOL_LEVEL if not encapsulator
		uint32 Overhead() const
			{ return fOverhead; }
		
		void SetNext(KPPPLayer *next)
			{ fNext = next; }
		KPPPLayer *Next() const
			{ return fNext; }
		
		virtual void ProfileChanged();
		
		virtual bool Up() = 0;
		virtual bool Down() = 0;
		
		virtual bool IsAllowedToSend() const = 0;
		
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber) = 0;
		
		status_t SendToNext(struct mbuf *packet, uint16 protocolNumber) const;
			// send the packet to the next layer
		
		virtual void Pulse();

	protected:
		void SetName(const char *name);
		
		status_t fInitStatus;
		uint32 fOverhead;

	private:
		char *fName;
		ppp_level fLevel;
		
		KPPPLayer *fNext;
};


#endif
