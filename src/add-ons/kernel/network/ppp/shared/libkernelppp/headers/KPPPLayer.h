/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_LAYER__H
#define _K_PPP_LAYER__H

#include <KPPPDefs.h>


class KPPPLayer {
	protected:
		//!	KPPPLayer must be subclassed
		KPPPLayer(const char *name, ppp_level level, uint32 overhead);

	public:
		virtual ~KPPPLayer();
		
		virtual status_t InitCheck() const;
		
		//!	Layer name.
		const char *Name() const
			{ return fName; }
		/*!	\brief Level of this layer (level is defined in enum: \c ppp_level).
			
			This should be \c PPP_PROTOCOL_LEVEL if this layer is not an encapsulator.
		*/
		ppp_level Level() const
			{ return fLevel; }
		//!	Length of header that will be prepended to outgoing packets.
		uint32 Overhead() const
			{ return fOverhead; }
		
		//!	Sets the next layer. This will be the target of \c SendToNext().
		void SetNext(KPPPLayer *next)
			{ fNext = next; }
		//!	Next layer in chain.
		KPPPLayer *Next() const
			{ return fNext; }
		
		//!	Brings this layer up.
		virtual bool Up() = 0;
		//!	Brings this layer down.
		virtual bool Down() = 0;
		
		//!	Returns whether this layer is allowed to send packets.
		virtual bool IsAllowedToSend() const = 0;
		
		//!	Send a packet with the given protocol number.
		virtual status_t Send(struct mbuf *packet, uint16 protocolNumber) = 0;
		//!	Receive a packet with the given protocol number.
		virtual status_t Receive(struct mbuf *packet, uint16 protocolNumber) = 0;
		
		status_t SendToNext(struct mbuf *packet, uint16 protocolNumber) const;
			// send the packet to the next layer
		
		virtual void Pulse();

	protected:
		void SetName(const char *name);

	protected:
		status_t fInitStatus;
		uint32 fOverhead;

	private:
		char *fName;
		ppp_level fLevel;
		
		KPPPLayer *fNext;
};


#endif
