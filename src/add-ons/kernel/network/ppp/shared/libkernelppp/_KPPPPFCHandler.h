//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef __K_PPP_PFC_HANDLER__H
#define __K_PPP_PFC_HANDLER__H

#include <KPPPOptionHandler.h>


class _PPPPFCHandler : public PPPOptionHandler {
	public:
		_PPPPFCHandler(ppp_pfc_state& localPFCState, ppp_pfc_state& peerPFCState,
			PPPInterface& interface);
		
		virtual status_t AddToRequest(PPPConfigurePacket& request);
		virtual status_t ParseNak(const PPPConfigurePacket& nak);
		virtual status_t ParseReject(const PPPConfigurePacket& reject);
		virtual status_t ParseAck(const PPPConfigurePacket& ack);
		
		virtual status_t ParseRequest(const PPPConfigurePacket& request,
			int32 index, PPPConfigurePacket& nak, PPPConfigurePacket& reject);
		virtual status_t SendingAck(const PPPConfigurePacket& ack);
		
		virtual void Reset();

	private:
		ppp_pfc_state &fLocalPFCState, &fPeerPFCState;
};


#endif
