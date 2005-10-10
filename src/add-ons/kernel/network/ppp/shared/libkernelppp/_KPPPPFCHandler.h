/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef __K_PPP_PFC_HANDLER__H
#define __K_PPP_PFC_HANDLER__H

#include <KPPPOptionHandler.h>


class _KPPPPFCHandler : public KPPPOptionHandler {
	public:
		_KPPPPFCHandler(ppp_pfc_state& localPFCState, ppp_pfc_state& peerPFCState,
			KPPPInterface& interface);
		
		virtual status_t AddToRequest(KPPPConfigurePacket& request);
		virtual status_t ParseNak(const KPPPConfigurePacket& nak);
		virtual status_t ParseReject(const KPPPConfigurePacket& reject);
		virtual status_t ParseAck(const KPPPConfigurePacket& ack);
		
		virtual status_t ParseRequest(const KPPPConfigurePacket& request,
			int32 index, KPPPConfigurePacket& nak, KPPPConfigurePacket& reject);
		virtual status_t SendingAck(const KPPPConfigurePacket& ack);
		
		virtual void Reset();

	private:
		ppp_pfc_state &fLocalPFCState, &fPeerPFCState;
};


#endif
